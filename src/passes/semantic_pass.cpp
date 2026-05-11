#include "passes/semantic_pass.hpp"
#include "ast_visitor.hpp"
#include "symtab.hpp"
#include <stdexcept>

using ast::SemanticInfo;

namespace eval {
namespace {

struct SemanticSym {
    TypeId type = TYPE_UNKNOWN;
    TypeId returnType = TYPE_UNKNOWN; // for function symbols: the return type
    bool isMutable = true;
    bool isFunction = false;
};

// Internal AST visitor — not exposed as a Pass, just an implementation detail.
class SemanticVisitor : public ast::ASTVisitor {
public:
    SemanticVisitor() : scope_(std::make_shared<SemScope>()) {}

    TypeId resolveType(ast::Expr& expr) { expr.accept(*this); return curType_; }

    // --- expressions ---
    void visit(ast::IntLitExpr& node) override {
        curType_ = TYPE_INT; node.setAnnotation(SemanticInfo{TYPE_INT}); }
    void visit(ast::FloatLitExpr& node) override {
        curType_ = TYPE_FLOAT; node.setAnnotation(SemanticInfo{TYPE_FLOAT}); }
    void visit(ast::BoolLitExpr& node) override {
        curType_ = TYPE_BOOL; node.setAnnotation(SemanticInfo{TYPE_BOOL}); }
    void visit(ast::StringLitExpr& node) override {
        curType_ = TYPE_STRING; node.setAnnotation(SemanticInfo{TYPE_STRING}); }

    void visit(ast::IdentExpr& node) override {
        const auto* sym = scope_->resolve(node.name);
        if (!sym) throw std::runtime_error("Undefined variable: " + node.name);
        curType_ = sym->type;
        node.setAnnotation(SemanticInfo{sym->type});
    }

    void visit(ast::UnaryExpr& node) override {
        TypeId t = resolveType(*node.operand);
        std::string_view op = node.prefix;
        if (op == "-") {
            if (t != TYPE_INT && t != TYPE_FLOAT)
                throw std::runtime_error("Cannot negate non-numeric value");
            curType_ = t;
        } else if (op == "not") {
            curType_ = TYPE_BOOL;
        }
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::BinOpExpr& node) override {
        std::string_view op = node.op;
        TypeId l = resolveType(*node.left);
        if (op == "and" || op == "or") {
            resolveType(*node.right);
            curType_ = l; // or rhs type
            node.setAnnotation(SemanticInfo{curType_});
            return;
        }
        TypeId r = resolveType(*node.right);
        if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || op == "^") {
            if (!((l == TYPE_INT || l == TYPE_FLOAT) && (r == TYPE_INT || r == TYPE_FLOAT)))
                throw std::runtime_error(std::string("Non-numeric operands for '") + std::string(op) + "'");
            curType_ = (l == TYPE_FLOAT || r == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;
        } else if (op == ">" || op == "<" || op == ">=" || op == "<=" || op == "==" || op == "!=") {
            curType_ = TYPE_BOOL;
        }
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::LetExpr& node) override {
        if (TypeTable::isKnownTypeName(node.ident))
            throw std::runtime_error("Cannot use type name '" + node.ident + "' as identifier");
        TypeId t = resolveType(*node.value);
        // If the value is a function literal, record it as a function symbol with its return type
        bool isFn = dynamic_cast<ast::FnLitExpr*>(node.value.get()) != nullptr;
        scope_->define(node.ident, SemanticSym{t, isFn ? t : TYPE_UNKNOWN, true, isFn});
        curType_ = t;
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::AssignExpr& node) override {
        auto* ex = scope_->resolveMut(node.ident);
        if (!ex) throw std::runtime_error("Undefined variable: " + node.ident);
        TypeId t = resolveType(*node.value);
        if (t != ex->type) throw std::runtime_error("Type mismatch in assignment to '" + node.ident + "'");
        curType_ = t;
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::FnLitExpr& node) override {
        std::vector<TypeId> ptids;
        for (auto& [name, tn] : node.params)
            ptids.push_back(TypeTable::resolveTypeName(tn));
        TypeId retTid = node.returnType ? TypeTable::resolveTypeName(*node.returnType) : TYPE_UNKNOWN;
        enterScope();
        for (size_t i = 0; i < node.params.size(); ++i)
            scope_->define(node.params[i].first, SemanticSym{ptids[i], TYPE_UNKNOWN, true, false});
        node.body->accept(*this);
        exitScope();
        // The function literal itself carries its return type for callers to use.
        curType_ = retTid;
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::CallExpr& node) override {
        resolveType(*node.callee);
        if (node.arguments) for (auto& a : node.arguments->exprs) resolveType(*a);
        // Try to resolve the callee's return type from the scope
        TypeId retTid = TYPE_UNKNOWN;
        if (auto* ident = dynamic_cast<ast::IdentExpr*>(node.callee.get())) {
            const auto* sym = scope_->resolve(ident->name);
            if (sym && sym->isFunction) {
                retTid = sym->returnType;
            }
        }
        curType_ = retTid;
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::ArrayExpr& node) override {
        throw std::runtime_error("[" + std::to_string(node.loc.begin.line) + ":"
            + std::to_string(node.loc.begin.column) + "] Array literals are not supported in compiled mode");
    }
    void visit(ast::ArrayDerefExpr& node) override {
        throw std::runtime_error("[" + std::to_string(node.loc.begin.line) + ":"
            + std::to_string(node.loc.begin.column) + "] Array indexing is not supported in compiled mode");
    }
    void visit(ast::ExprSeq& node) override {
        for (auto& e : node.exprs) resolveType(*e); }

    // --- statements ---
    void visit(ast::ExprStmt& node) override { resolveType(*node.expression); }
    void visit(ast::BlockStmt& node) override {
        enterScope(); node.stmtList->accept(*this); exitScope(); }
    void visit(ast::IfStmt& node) override {
        resolveType(*node.cond);
        if (node.truthy_branch) node.truthy_branch->accept(*this);
        if (node.elseIfs)
            for (size_t i = 0; i < node.elseIfs->conditions.size(); ++i) {
                resolveType(*node.elseIfs->conditions[i]);
                node.elseIfs->branches[i]->accept(*this);
            }
        if (node.optElse) node.optElse.value()->accept(*this);
    }
    void visit(ast::WhileStmt& node) override {
        resolveType(*node.cond); node.body->accept(*this); }
    void visit(ast::StmtList& node) override {
        for (auto& s : node.statements) s->accept(*this); }
    void visit(ast::ReturnStmt& node) override {
        if (node.value) resolveType(*node.value); }

private:
    using SemScope = Scope<SemanticSym>;
    std::shared_ptr<SemScope> scope_;
    TypeId curType_ = TYPE_UNKNOWN;
    void enterScope() { scope_ = std::make_shared<SemScope>(scope_); }
    void exitScope() { scope_ = scope_->parent(); }
};

} // anon namespace

void semanticAnalysis(Compiler& c) {
    SemanticVisitor sem;
    c.ast->accept(sem);
}

} // namespace eval
