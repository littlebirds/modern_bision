#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "location.hh"

namespace ast {

struct Node {
    monkey::location loc;
    virtual ~Node() = default;
    virtual std::string str() const = 0;
};

struct Expr : public Node {

};

struct Stmt : public Node { 
};

struct StmtList : public Node {
    std::vector<std::unique_ptr<Stmt>> statements;

    void append(std::unique_ptr<Stmt> stmt) {
        statements.push_back(std::move(stmt));
    }
    std::string str() const override;
};

struct LiteralExpr : public Expr {
    std::string literal;

    LiteralExpr(std::string value) : literal(value) {}
};

struct IntLitExpr : public LiteralExpr { 
    IntLitExpr(std::string value) : LiteralExpr(value) {} 

    std::string str() const override;
};

struct FloatLitExpr : public LiteralExpr {    
    FloatLitExpr(std::string value) : LiteralExpr(value) {}

    std::string str() const override;
}; 

struct StringLitExpr : public LiteralExpr {    
    StringLitExpr(std::string value) : LiteralExpr(value) {}

    std::string str() const override;
};

struct BinOpExpr : public Expr {
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    const char* const op;

    BinOpExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, const char* op)
        : left(std::move(left)), right(std::move(right)), op(op) {}
    ~BinOpExpr() = default;

    std::string str() const override;
};

struct ExprStmt : public Stmt {
    std::unique_ptr<Expr> expression;

    ExprStmt(std::unique_ptr<Expr> expr) : expression(std::move(expr)) {} 
    std::string str() const override;
};

struct LetStmt : public Stmt {
    std::string ident;
    std::unique_ptr<Expr> value;

    LetStmt(std::string ident, std::unique_ptr<Expr> value) : ident(std::move(ident)), value(std::move(value)) {}
    std::string str() const override;
};

} // namespace ast