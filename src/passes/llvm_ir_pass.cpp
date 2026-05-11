#include "passes/llvm_ir_pass.hpp"
#include "ast_visitor.hpp"
#include "symtab.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Intrinsics.h>
#include <stdexcept>

using ast::SemanticInfo;

namespace eval {
namespace {

struct Sym {
    TypeId type = TYPE_UNKNOWN;
    llvm::Value* slot = nullptr;
    llvm::Function* func = nullptr;
    bool isMutable = true;
};

static llvm::Type* toLLVMType(TypeId tid, llvm::LLVMContext& ctx) {
    switch (tid) {
        case TYPE_INT:    return llvm::Type::getInt64Ty(ctx);
        case TYPE_FLOAT:  return llvm::Type::getDoubleTy(ctx);
        case TYPE_BOOL:   return llvm::Type::getInt1Ty(ctx);
        case TYPE_STRING: return llvm::PointerType::getUnqual(ctx);
        default:          return llvm::Type::getVoidTy(ctx);
    }
}

class LLVMGen : public ast::ASTVisitor {
public:
    LLVMGen(Compiler& c)
        : ctx_(c.llvmCtx.get()), mod_(c.module.get()), bld_(c.builder.get())
        , scope_(std::make_shared<SymScope>()) {}

    // --- top-level entry (called from free function, not visitor) ---
    void enterScope()   { scope_ = std::make_shared<SymScope>(scope_); }
    void exitScope()    { scope_ = scope_->parent(); }
    void beginFn(llvm::Function* fn, TypeId retTid);
    void finishFn(TypeId retTid);

    // --- expressions ---
    void visit(ast::IntLitExpr& n) override {
        val_ = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*ctx_), std::stoll(n.literal)); }
    void visit(ast::FloatLitExpr& n) override {
        val_ = llvm::ConstantFP::get(llvm::Type::getDoubleTy(*ctx_), std::stod(n.literal)); }
    void visit(ast::BoolLitExpr& n) override {
        val_ = llvm::ConstantInt::get(llvm::Type::getInt1Ty(*ctx_), n.value); }
    void visit(ast::StringLitExpr& n) override {
        val_ = bld_->CreateGlobalString(n.literal, ".str"); }

    void visit(ast::IdentExpr& n) override {
        auto* s = resolve(n.name);
        val_ = bld_->CreateLoad(toLLVMType(s->type, *ctx_), s->slot, n.name);
    }
    void visit(ast::UnaryExpr& n) override;
    void visit(ast::BinOpExpr& n) override;
    void visit(ast::LetExpr& n) override;
    void visit(ast::AssignExpr& n) override;
    void visit(ast::FnLitExpr& n) override;
    void visit(ast::CallExpr& n) override;
    void visit(ast::ArrayExpr&) override { throw std::runtime_error("arrays TBD"); }
    void visit(ast::ArrayDerefExpr&) override { throw std::runtime_error("deref TBD"); }
    void visit(ast::ExprSeq& n) override { for (auto& e : n.exprs) e->accept(*this); }

    void visit(ast::ExprStmt& n) override   { n.expression->accept(*this); }
    void visit(ast::BlockStmt& n) override  { enterScope(); n.stmtList->accept(*this); exitScope(); }
    void visit(ast::StmtList& n) override   { for (auto& s : n.statements) s->accept(*this); }
    void visit(ast::IfStmt& n) override;
    void visit(ast::WhileStmt& n) override;
    void visit(ast::ReturnStmt& n) override;

private:
    using SymScope = Scope<Sym>;
    llvm::LLVMContext* ctx_;
    llvm::Module* mod_;
    llvm::IRBuilder<>* bld_;
    std::shared_ptr<SymScope> scope_;
    llvm::Function* curFn_ = nullptr;
    llvm::BasicBlock* retBlock_ = nullptr;
    llvm::Value* retAlloca_ = nullptr;
    llvm::Value* val_ = nullptr;
    int fnNameCounter_ = 0;

    llvm::Value* visitExpr(ast::Expr& e) { e.accept(*this); return val_; }
    const Sym* resolve(const std::string& n) {
        auto* s = scope_->resolve(n); if (!s) throw std::runtime_error("Undefined: " + n); return s;
    }
    llvm::Value* entryAlloca(llvm::Type* t, const std::string& n) {
        llvm::IRBuilder<> tmp(&curFn_->getEntryBlock(), curFn_->getEntryBlock().begin());
        return tmp.CreateAlloca(t, nullptr, n);
    }
    llvm::Function* declareFn(ast::FnLitExpr& n);
    void compileLetFn(ast::LetExpr& n, ast::FnLitExpr& fnLit);
};

// --- begin / finish ---

void LLVMGen::beginFn(llvm::Function* fn, TypeId retTid) {
    curFn_ = fn;
    auto* entry = llvm::BasicBlock::Create(*ctx_, "entry", fn);
    bld_->SetInsertPoint(entry);
    for (auto& arg : fn->args()) {
        auto* a = entryAlloca(arg.getType(), arg.getName().str());
        bld_->CreateStore(&arg, a);
        TypeId tid = TYPE_UNKNOWN;
        if (arg.getType()->isIntegerTy(64)) tid = TYPE_INT;
        else if (arg.getType()->isDoubleTy()) tid = TYPE_FLOAT;
        else if (arg.getType()->isIntegerTy(1)) tid = TYPE_BOOL;
        scope_->define(arg.getName().str(), Sym{tid, a, nullptr, true});
    }
    if (retTid != TYPE_NULL) {
        auto* retType = toLLVMType(retTid, *ctx_);
        retAlloca_ = entryAlloca(retType, "retval");
        // Initialize retAlloca to zero to avoid reading uninitialized memory
        bld_->CreateStore(llvm::Constant::getNullValue(retType), retAlloca_);
        retBlock_ = llvm::BasicBlock::Create(*ctx_, "return", fn);
    } else {
        retAlloca_ = nullptr;
        retBlock_ = nullptr;
    }
}

void LLVMGen::finishFn(TypeId retTid) {
    // If the current block has no terminator, branch to return block (or emit ret directly)
    if (!bld_->GetInsertBlock()->getTerminator()) {
        if (retTid == TYPE_NULL) {
            bld_->CreateRetVoid();
        } else if (retAlloca_) {
            // Store the last expression value as implicit return
            if (val_ && !val_->getType()->isVoidTy())
                bld_->CreateStore(val_, retAlloca_);
            bld_->CreateBr(retBlock_);
        }
    }
    // Emit the return block: load retval and return it
    if (retBlock_) {
        bld_->SetInsertPoint(retBlock_);
        bld_->CreateRet(bld_->CreateLoad(toLLVMType(retTid, *ctx_), retAlloca_, "retload"));
    }
}

// --- expression visitors ---

void LLVMGen::visit(ast::UnaryExpr& n) {
    auto* op = visitExpr(*n.operand);
    if (n.prefix == std::string("-"))
        val_ = op->getType()->isIntegerTy(64) ? bld_->CreateNeg(op) : bld_->CreateFNeg(op);
    else if (n.prefix == std::string("not")) val_ = bld_->CreateNot(op);
}

void LLVMGen::visit(ast::BinOpExpr& n) {
    std::string_view op = n.op;
    if (op == "and" || op == "or") {
        auto* lhs = visitExpr(*n.left);
        auto* cur = bld_->GetInsertBlock();
        auto* rhsBB = llvm::BasicBlock::Create(*ctx_, "logical.rhs", curFn_);
        auto* mrgBB = llvm::BasicBlock::Create(*ctx_, "logical.merge", curFn_);
        (op == "and") ? bld_->CreateCondBr(lhs, rhsBB, mrgBB) : bld_->CreateCondBr(lhs, mrgBB, rhsBB);
        bld_->SetInsertPoint(rhsBB);
        auto* rhs = visitExpr(*n.right);
        auto* re = bld_->GetInsertBlock();
        bld_->CreateBr(mrgBB);
        bld_->SetInsertPoint(mrgBB);
        auto* phi = bld_->CreatePHI(llvm::Type::getInt1Ty(*ctx_), 2, "logical");
        phi->addIncoming(op=="and" ? llvm::ConstantInt::getFalse(*ctx_) : llvm::ConstantInt::getTrue(*ctx_), cur);
        phi->addIncoming(rhs, re);
        val_ = phi;
        return;
    }
    auto* l = visitExpr(*n.left);
    auto* r = visitExpr(*n.right);
    bool fp = l->getType()->isDoubleTy() || r->getType()->isDoubleTy();
    if (fp) {
        if (l->getType()->isIntegerTy(64)) l = bld_->CreateSIToFP(l, llvm::Type::getDoubleTy(*ctx_));
        if (r->getType()->isIntegerTy(64)) r = bld_->CreateSIToFP(r, llvm::Type::getDoubleTy(*ctx_));
    }
    if (op == "+") val_ = fp ? bld_->CreateFAdd(l,r) : bld_->CreateAdd(l,r);
    else if (op == "-") val_ = fp ? bld_->CreateFSub(l,r) : bld_->CreateSub(l,r);
    else if (op == "*") val_ = fp ? bld_->CreateFMul(l,r) : bld_->CreateMul(l,r);
    else if (op == "/" || op == "%") {
        // Guard against division by zero for integer operations
        if (!fp) {
            auto* isZero = bld_->CreateICmpEQ(r, llvm::ConstantInt::get(r->getType(), 0), "divzero");
            auto* okBB = llvm::BasicBlock::Create(*ctx_, "div.ok", curFn_);
            auto* trapBB = llvm::BasicBlock::Create(*ctx_, "div.trap", curFn_);
            bld_->CreateCondBr(isZero, trapBB, okBB);
            bld_->SetInsertPoint(trapBB);
            auto* trapFn = llvm::Intrinsic::getOrInsertDeclaration(mod_, llvm::Intrinsic::trap);
            bld_->CreateCall(trapFn);
            bld_->CreateUnreachable();
            bld_->SetInsertPoint(okBB);
        }
        if (op == "/") val_ = fp ? bld_->CreateFDiv(l,r) : bld_->CreateSDiv(l,r);
        else           val_ = fp ? bld_->CreateFRem(l,r) : bld_->CreateSRem(l,r);
    }
    else if (op == "^") {
        // Exponent: use llvm.pow intrinsic (operates on doubles)
        if (!fp) {
            l = bld_->CreateSIToFP(l, llvm::Type::getDoubleTy(*ctx_));
            r = bld_->CreateSIToFP(r, llvm::Type::getDoubleTy(*ctx_));
        }
        auto* powFn = llvm::Intrinsic::getOrInsertDeclaration(
            mod_, llvm::Intrinsic::pow, {llvm::Type::getDoubleTy(*ctx_)});
        auto* result = bld_->CreateCall(powFn, {l, r}, "powtmp");
        if (!fp) val_ = bld_->CreateFPToSI(result, llvm::Type::getInt64Ty(*ctx_));
        else     val_ = result;
    }
    else if (op == "==") val_ = fp ? bld_->CreateFCmpOEQ(l,r) : bld_->CreateICmpEQ(l,r);
    else if (op == "!=") val_ = fp ? bld_->CreateFCmpONE(l,r) : bld_->CreateICmpNE(l,r);
    else if (op == ">")  val_ = fp ? bld_->CreateFCmpOGT(l,r) : bld_->CreateICmpSGT(l,r);
    else if (op == "<")  val_ = fp ? bld_->CreateFCmpOLT(l,r) : bld_->CreateICmpSLT(l,r);
    else if (op == ">=") val_ = fp ? bld_->CreateFCmpOGE(l,r) : bld_->CreateICmpSGE(l,r);
    else if (op == "<=") val_ = fp ? bld_->CreateFCmpOLE(l,r) : bld_->CreateICmpSLE(l,r);
}

void LLVMGen::visit(ast::LetExpr& n) {
    if (TypeTable::isKnownTypeName(n.ident))
        throw std::runtime_error("Cannot use type name '" + n.ident + "' as identifier");
    if (auto* fnLit = dynamic_cast<ast::FnLitExpr*>(n.value.get())) {
        compileLetFn(n, *fnLit);
        return;
    }
    auto* v = visitExpr(*n.value);
    auto* a = entryAlloca(v->getType(), n.ident);
    bld_->CreateStore(v, a);
    TypeId tid = TYPE_INT;
    if (v->getType()->isIntegerTy(64)) tid = TYPE_INT;
    else if (v->getType()->isDoubleTy()) tid = TYPE_FLOAT;
    else if (v->getType()->isIntegerTy(1)) tid = TYPE_BOOL;
    scope_->define(n.ident, Sym{tid, a, nullptr, true});
    val_ = v;
}

void LLVMGen::visit(ast::AssignExpr& n) {
    auto* s = resolve(n.ident);
    auto* v = visitExpr(*n.value);
    bld_->CreateStore(v, s->slot);
    val_ = v;
}

void LLVMGen::visit(ast::FnLitExpr& n) {
    auto* fn = declareFn(n);
    auto savedFn = curFn_; auto savedRA = retAlloca_; auto savedRB = retBlock_;
    auto savedIP = bld_->saveIP();
    TypeId retTid = n.returnType ? TypeTable::resolveTypeName(*n.returnType) : TYPE_INT;
    beginFn(fn, retTid);
    enterScope();
    n.body->accept(*this);
    finishFn(retTid);
    exitScope();
    bld_->restoreIP(savedIP);
    curFn_ = savedFn; retAlloca_ = savedRA; retBlock_ = savedRB;
    val_ = fn;
}

void LLVMGen::visit(ast::CallExpr& n) {
    llvm::Function* cf = nullptr;
    if (auto* id = dynamic_cast<ast::IdentExpr*>(n.callee.get())) {
        auto* s = resolve(id->name);
        if (s->func) cf = s->func;
    }
    if (!cf) throw std::runtime_error("Indirect calls not yet supported");
    std::vector<llvm::Value*> args;
    if (n.arguments) for (auto& a : n.arguments->exprs) args.push_back(visitExpr(*a));
    val_ = bld_->CreateCall(cf, args, "calltmp");
}

// --- statement visitors ---

void LLVMGen::visit(ast::IfStmt& n) {
    auto* cond = visitExpr(*n.cond);
    if (!cond->getType()->isIntegerTy(1)) cond = bld_->CreateIsNotNull(cond, "tobool");
    auto* mBB = llvm::BasicBlock::Create(*ctx_, "ifmerge", curFn_);

    // Determine the else/elif chain target
    auto* tBB = llvm::BasicBlock::Create(*ctx_, "then", curFn_);
    auto* eBB = llvm::BasicBlock::Create(*ctx_, "else", curFn_);
    bld_->CreateCondBr(cond, tBB, eBB);

    // Compile the truthy branch
    bld_->SetInsertPoint(tBB);
    n.truthy_branch->accept(*this);
    if (!bld_->GetInsertBlock()->getTerminator()) bld_->CreateBr(mBB);

    // Compile elif branches as a chain of condition checks
    bld_->SetInsertPoint(eBB);
    if (n.elseIfs && !n.elseIfs->conditions.empty()) {
        for (size_t i = 0; i < n.elseIfs->conditions.size(); ++i) {
            auto* elifCond = visitExpr(*n.elseIfs->conditions[i]);
            if (!elifCond->getType()->isIntegerTy(1))
                elifCond = bld_->CreateIsNotNull(elifCond, "tobool");
            auto* elifThenBB = llvm::BasicBlock::Create(*ctx_, "elif.then", curFn_);
            auto* elifNextBB = llvm::BasicBlock::Create(*ctx_, "elif.next", curFn_);
            bld_->CreateCondBr(elifCond, elifThenBB, elifNextBB);

            bld_->SetInsertPoint(elifThenBB);
            n.elseIfs->branches[i]->accept(*this);
            if (!bld_->GetInsertBlock()->getTerminator()) bld_->CreateBr(mBB);

            bld_->SetInsertPoint(elifNextBB);
        }
    }

    // Compile the else branch (if any) — insert point is at the final else/elif.next block
    if (n.optElse) n.optElse.value()->accept(*this);
    if (!bld_->GetInsertBlock()->getTerminator()) bld_->CreateBr(mBB);

    bld_->SetInsertPoint(mBB);
    val_ = nullptr;
}

void LLVMGen::visit(ast::WhileStmt& n) {
    auto* cBB = llvm::BasicBlock::Create(*ctx_, "while.cond", curFn_);
    auto* bBB = llvm::BasicBlock::Create(*ctx_, "while.body", curFn_);
    auto* xBB = llvm::BasicBlock::Create(*ctx_, "while.exit", curFn_);
    bld_->CreateBr(cBB); bld_->SetInsertPoint(cBB);
    auto* cond = visitExpr(*n.cond);
    if (!cond->getType()->isIntegerTy(1)) cond = bld_->CreateIsNotNull(cond, "tobool");
    bld_->CreateCondBr(cond, bBB, xBB);
    bld_->SetInsertPoint(bBB); n.body->accept(*this);
    if (!bld_->GetInsertBlock()->getTerminator()) bld_->CreateBr(cBB);
    bld_->SetInsertPoint(xBB); val_ = nullptr;
}

void LLVMGen::visit(ast::ReturnStmt& n) {
    auto* v = visitExpr(*n.value);
    if (retAlloca_) { bld_->CreateStore(v, retAlloca_); bld_->CreateBr(retBlock_); }
    else bld_->CreateRet(v);
    bld_->SetInsertPoint(llvm::BasicBlock::Create(*ctx_, "unreach", curFn_));
}

// --- helpers ---

llvm::Function* LLVMGen::declareFn(ast::FnLitExpr& n) {
    TypeId retTid = n.returnType ? TypeTable::resolveTypeName(*n.returnType) : TYPE_INT;
    std::vector<llvm::Type*> pts;
    std::vector<std::string> pns;
    for (auto& [name, tn] : n.params) {
        pts.push_back(toLLVMType(TypeTable::resolveTypeName(tn), *ctx_));
        pns.push_back(name);
    }
    auto* fnty = llvm::FunctionType::get(toLLVMType(retTid, *ctx_), pts, false);
    auto* fn = llvm::Function::Create(fnty, llvm::Function::InternalLinkage,
                                       "fn." + std::to_string(fnNameCounter_++), mod_);
    for (size_t i = 0; i < pns.size(); ++i) fn->getArg(i)->setName(pns[i]);
    return fn;
}

void LLVMGen::compileLetFn(ast::LetExpr& n, ast::FnLitExpr& fnLit) {
    auto* fn = declareFn(fnLit);
    auto* a = entryAlloca(fn->getType(), n.ident);
    bld_->CreateStore(fn, a);
    scope_->define(n.ident, Sym{TYPE_UNKNOWN, a, fn, true});
    auto savedFn = curFn_; auto savedRA = retAlloca_; auto savedRB = retBlock_;
    auto savedIP = bld_->saveIP();
    TypeId retTid = fnLit.returnType ? TypeTable::resolveTypeName(*fnLit.returnType) : TYPE_INT;
    beginFn(fn, retTid);
    enterScope();
    fnLit.body->accept(*this);
    finishFn(retTid);
    exitScope();
    bld_->restoreIP(savedIP);
    curFn_ = savedFn; retAlloca_ = savedRA; retBlock_ = savedRB;
    val_ = fn;
}

} // anon namespace

void llvmIRGen(Compiler& c) {
    c.llvmCtx = std::make_unique<llvm::LLVMContext>();
    c.module = std::make_unique<llvm::Module>("monkey", *c.llvmCtx);
    c.builder = std::make_unique<llvm::IRBuilder<>>(*c.llvmCtx);

    LLVMGen gen(c);
    auto* fnty = llvm::FunctionType::get(llvm::Type::getInt64Ty(*c.llvmCtx), false);
    auto* mainFn = llvm::Function::Create(fnty, llvm::Function::ExternalLinkage, "main", c.module.get());
    gen.beginFn(mainFn, TYPE_INT);
    gen.enterScope();
    c.ast->accept(gen);
    gen.finishFn(TYPE_INT);
    gen.exitScope();
}

} // namespace eval
