#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "location.hh"

namespace ast {

struct Node {
    monkey::location loc;

    Node() = delete;
    Node(const monkey::location& location) : loc(location) {}
    virtual ~Node() = default;
    virtual std::string str() const;
};

struct Expr : public Node {
    using Node::Node;
};

struct Stmt : public Node { 
    using Node::Node;
};

struct StmtList : public Node {
    std::vector<std::unique_ptr<Stmt>> statements;
    
    StmtList() : Node(monkey::location()) {};

    void append(std::unique_ptr<Stmt> stmt) {
        statements.push_back(std::move(stmt));
    }
    std::string str() const override;
};

struct LiteralExpr : public Expr {
    std::string literal;    

    LiteralExpr(const monkey::location& loc, std::string value) : Expr(loc), literal(std::move(value)) {}
};

struct IntLitExpr : public LiteralExpr {  
    using LiteralExpr::LiteralExpr;
    
    std::string str() const override;
};

struct FloatLitExpr : public LiteralExpr {
    using LiteralExpr::LiteralExpr;

    std::string str() const override;
}; 

struct StringLitExpr : public LiteralExpr {    
    using LiteralExpr::LiteralExpr;

    std::string str() const override;
};

struct BinOpExpr : public Expr {
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    const char* op;

    BinOpExpr(const monkey::location& loc, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, const char* op)
        : Expr(loc), left(std::move(left)), right(std::move(right)), op(op) {}
    ~BinOpExpr() = default;

    std::string str() const override;
};

struct ExprStmt : public Stmt {
    std::unique_ptr<Expr> expression;

    ExprStmt(const monkey::location& loc, std::unique_ptr<Expr> expr) : Stmt(loc), expression(std::move(expr)) {} 
    std::string str() const override;
};

struct LetStmt : public Stmt {
    std::string ident;
    std::unique_ptr<Expr> value;

    LetStmt(const monkey::location& loc, std::string ident, std::unique_ptr<Expr> value) : Stmt(loc), ident(std::move(ident)), value(std::move(value)) {}
    std::string str() const override;
};

} // namespace ast