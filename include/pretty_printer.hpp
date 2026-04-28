#pragma once

#include "ast_visitor.hpp"
#include <sstream>
#include <string>

namespace ast {

class PrettyPrinter : public ASTVisitor {
public:
    PrettyPrinter() = default;
    ~PrettyPrinter() override = default;

    // Get the resulting string
    std::string result() const { return output_.str(); }

    // Expression visitors
    void visit(IntLitExpr& node) override;
    void visit(FloatLitExpr& node) override;
    void visit(StringLitExpr& node) override;
    void visit(IdentExpr& node) override;
    void visit(UnaryExpr& node) override;
    void visit(BinOpExpr& node) override;
    void visit(ArrayExpr& node) override;
    void visit(ArrayDerefExpr& node) override;
    void visit(LetExpr& node) override;
    void visit(ExprSeq& node) override;

    // Statement visitors
    void visit(ExprStmt& node) override;
    void visit(BlockStmt& node) override;
    void visit(IfStmt& node) override;
    void visit(StmtList& node) override;
    void visit(ReturnStmt& node) override;

    // Function visitors
    void visit(FnLitExpr& node) override;
    void visit(CallExpr& node) override;

private:
    std::ostringstream output_;
};

} // namespace ast
