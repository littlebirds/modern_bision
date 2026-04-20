#pragma once

#include "ast_visitor.hpp"
#include "ast.hpp"
#include "context.hpp"
#include "value.hpp"
#include <memory>
#include <string>

namespace eval {

class Interpreter : public ast::ASTVisitor {
public:
    Interpreter() : ctx_(std::make_shared<Context>()) {}
    ~Interpreter() override = default;

    Value result() const { return result_; }

    void visit(ast::IntLitExpr& node) override;
    void visit(ast::FloatLitExpr& node) override;
    void visit(ast::StringLitExpr& node) override;
    void visit(ast::IdentExpr& node) override;
    void visit(ast::UnaryExpr& node) override;
    void visit(ast::BinOpExpr& node) override;
    void visit(ast::ArrayExpr& node) override;
    void visit(ast::LetExpr& node) override;
    void visit(ast::ExprSeq& node) override;
    void visit(ast::ExprStmt& node) override;
    void visit(ast::BlockStmt& node) override;
    void visit(ast::IfStmt& node) override;
    void visit(ast::StmtList& node) override;

private:
    Value result_;
    std::shared_ptr<Context> ctx_;

    Value evaluate(ast::Expr& expr);
    Value applyArithmetic(const Value& lhs, const Value& rhs, std::string_view op);
    Value applyComparison(const Value& lhs, const Value& rhs, std::string_view op);
};

} // namespace eval
