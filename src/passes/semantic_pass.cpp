#include "passes/semantic_pass.hpp"
#include "ast_visitor.hpp"
#include "symtab.hpp"
#include <stdexcept>

using ast::SemanticInfo;

namespace eval {
namespace {

struct SemanticSym {
    TypeId type = TYPE_UNKNOWN;
    bool isMutable = true;
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
        scope_->define(node.ident, SemanticSym{t, true});
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
        TypeId retTid = node.returnType ? TypeTable::resolveTypeName(*node.returnType) : TYPE_INT;
        enterScope();
        for (size_t i = 0; i < node.params.size(); ++i)
            scope_->define(node.params[i].first, SemanticSym{ptids[i], true});
        node.body->accept(*this);
        exitScope();
        curType_ = TYPE_UNKNOWN;
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::CallExpr& node) override {
        resolveType(*node.callee);
        if (node.arguments) for (auto& a : node.arguments->exprs) resolveType(*a);
        curType_ = TYPE_INT; // simplified
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::ArrayExpr&) override { curType_ = TYPE_UNKNOWN; }
    void visit(ast::ArrayDerefExpr&) override { curType_ = TYPE_UNKNOWN; }
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
