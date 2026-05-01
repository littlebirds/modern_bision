#pragma once

#include "ast_visitor.hpp"
#include "ast.hpp"
#include "symtab.hpp"
#include "value.hpp"
#include <memory>
#include <string>
#include <stdexcept>

namespace eval {

struct ReturnException : public std::exception {
    Value value;
    explicit ReturnException(Value val) : value(std::move(val)) {}
};

class Interpreter : public ast::ASTVisitor {
public:
    Interpreter()
        : ctx_(std::make_shared<ValueScope>()),
          global_ctx_(ctx_) {}
    ~Interpreter() override = default;

    Value result() const { return result_; }

    void visit(ast::IntLitExpr& node) override;
    void visit(ast::FloatLitExpr& node) override;
    void visit(ast::BoolLitExpr& node) override;
    void visit(ast::StringLitExpr& node) override;
    void visit(ast::IdentExpr& node) override;
    void visit(ast::UnaryExpr& node) override;
    void visit(ast::BinOpExpr& node) override;
    void visit(ast::ArrayExpr& node) override;
    void visit(ast::ArrayDerefExpr& node) override;
    void visit(ast::LetExpr& node) override;
    void visit(ast::AssignExpr& node) override;
    void visit(ast::ExprSeq& node) override;
    void visit(ast::ExprStmt& node) override;
    void visit(ast::BlockStmt& node) override;
    void visit(ast::IfStmt& node) override;
    void visit(ast::WhileStmt& node) override;
    void visit(ast::StmtList& node) override;
    void visit(ast::FnLitExpr& node) override;
    void visit(ast::CallExpr& node) override;
    void visit(ast::ReturnStmt& node) override;

private:
    using ValueScope = Scope<Value>;

    Value result_;
    std::shared_ptr<ValueScope> ctx_;
    std::shared_ptr<ValueScope> global_ctx_;

    Value evaluate(ast::Expr& expr);
    Value applyArithmetic(const Value& lhs, const Value& rhs, std::string_view op);
    Value applyComparison(const Value& lhs, const Value& rhs, std::string_view op);
};

} // namespace eval
