#pragma once

#include <string>

namespace ast {

// Forward declarations
struct IntLitExpr;
struct FloatLitExpr;
struct StringLitExpr;
struct IdentExpr;
struct UnaryExpr;
struct BinOpExpr;
struct ArrayExpr;
struct ArrayDerefExpr;
struct LetExpr;
struct ExprSeq;
struct ExprStmt;
struct BlockStmt;
struct IfStmt;
struct StmtList;

struct ASTVisitor {
    ASTVisitor() = default;
    virtual ~ASTVisitor() = default;

    // Expressions
    virtual void visit(IntLitExpr& node) = 0;
    virtual void visit(FloatLitExpr& node) = 0;
    virtual void visit(StringLitExpr& node) = 0;
    virtual void visit(IdentExpr& node) = 0;
    virtual void visit(UnaryExpr& node) = 0;
    virtual void visit(BinOpExpr& node) = 0;
    virtual void visit(ArrayExpr& node) = 0;
    virtual void visit(ArrayDerefExpr& node) = 0;
    virtual void visit(LetExpr& node) = 0;
    virtual void visit(ExprSeq& node) = 0;

    // Statements
    virtual void visit(ExprStmt& node) = 0;
    virtual void visit(BlockStmt& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(StmtList& node) = 0;
};

} // namespace ast
