#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ast {

struct Node {
    virtual ~Node() = default;
    virtual std::string str() const = 0;
};

struct Expr : public Node { 
};

struct Stmt : public Node { 
};

struct Program : public Node {
    std::vector<std::unique_ptr<Stmt>> statements;
    Program() = default;
    ~Program() = default;

    void appendStmt(std::unique_ptr<Stmt> stmt) {
        statements.push_back(std::move(stmt));
    }

    std::string str() const override;
};

struct IntLitExpr : public Expr {
    long long value;

    IntLitExpr(long long value) : value(value) {}
    ~IntLitExpr() = default;

    std::string str() const override;
};

struct FloatLitExpr : public Expr {
    double value;
    
    FloatLitExpr(double value) : value(value) {}
    ~FloatLitExpr() = default;

    std::string str() const override;
}; 

struct StringLitExpr : public Expr {
    std::string value;
    
    StringLitExpr(std::string value) : value(std::move(value)) {}
    ~StringLitExpr() = default;
    
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