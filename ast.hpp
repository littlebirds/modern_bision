#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
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
    int nested_lvl = 0;

    virtual void ident(int adjustment) { nested_lvl += adjustment; }
    std::string str() const override;
};

struct StmtList : public Node {
    std::vector<std::unique_ptr<Stmt>> statements;
    
    StmtList() : Node(monkey::location()) {};

    void append(std::unique_ptr<Stmt> stmt) {
        statements.push_back(std::move(stmt));    
    }

    void ident(int adjustment) { for (auto& stmt : statements) { stmt->ident(adjustment); } }

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

struct UnaryExpr: public Expr {
    std::unique_ptr<Expr> operand;
    const char* prefix;

    UnaryExpr(const monkey::location& loc, std::unique_ptr<Expr> expr, const char* op) : Expr(loc), operand(std::move(expr)), prefix(op) {}

    std::string str() const;
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

struct BlockStmt : public Stmt {
    std::unique_ptr<StmtList> stmtList; 

    BlockStmt(const monkey::location& loc, std::unique_ptr<StmtList> stmts, int adjustment) : Stmt(loc), stmtList(std::move(stmts)) { ident(adjustment); }

    void ident(int adjustment) override { Stmt::ident(adjustment); stmtList->ident(adjustment); }

    std::string str() const override;
};


struct ElifList {
    std::vector<std::tuple<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> branches; 

    void append(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> branch) {
        branches.emplace_back(std::move(cond), std::move(branch));
    }
};

struct IfStmt : public Stmt {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<BlockStmt> truthy_branch;
    std::unique_ptr<ElifList> elseIfs;
    std::optional<std::unique_ptr<BlockStmt>> optElse;

    IfStmt(monkey::location loc, std::unique_ptr<BlockStmt> truthy_branch, std::unique_ptr<ElifList> elseIfs, std::optional<std::unique_ptr<BlockStmt>> optElse)
    : Stmt(loc), truthy_branch(std::move(truthy_branch)), elseIfs(std::move(elseIfs)), optElse(std::move(optElse)) {}
    
    std::string str() const override;
};

} // namespace ast