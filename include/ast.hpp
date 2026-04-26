#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include "location.hh"

namespace ast {

struct ASTVisitor; // Forward declaration

struct Node {
    monkey::location loc;

    Node() = delete;
    Node(const monkey::location& location) : loc(location) {}
    virtual ~Node() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
};

struct Expr : public Node {
    using Node::Node;
};

struct ExprSeq : public Node {
    std::vector<std::unique_ptr<Expr>> exprs;

    ExprSeq() : Node(monkey::location()) {}

    void append(Expr* expr) { exprs.emplace_back(expr); }

    void append(std::unique_ptr<Expr>& expr) { exprs.emplace_back(std::move(expr)); }

    void accept(ASTVisitor& visitor) override;
};

struct Stmt : public Node {
    using Node::Node;
    int nested_lvl = 0;

    virtual void setIndentationLvl(int adjustment) { nested_lvl = adjustment; }
};

struct StmtList : public Node {
    std::vector<std::unique_ptr<Stmt>> statements;
    int lvlNested;
    StmtList() : Node(monkey::location()){};

    void append(Stmt* stmt) { statements.emplace_back(stmt); }

    void append(std::unique_ptr<Stmt>& stmt) { statements.emplace_back(std::move(stmt)); }

    void setIndentationLvl(int adjustment) {
        lvlNested = adjustment;
        for (auto& stmt : statements) {
            stmt->setIndentationLvl(adjustment);
        }
    }

    void accept(ASTVisitor& visitor) override;
};

struct LiteralExpr : public Expr {
    std::string literal;

    LiteralExpr(const monkey::location& loc, std::string value) : Expr(loc), literal(std::move(value)) {}
};

struct IntLitExpr : public LiteralExpr {
    using LiteralExpr::LiteralExpr;

    void accept(ASTVisitor& visitor) override;
};

struct FloatLitExpr : public LiteralExpr {
    using LiteralExpr::LiteralExpr;

    void accept(ASTVisitor& visitor) override;
};

struct StringLitExpr : public LiteralExpr {
    using LiteralExpr::LiteralExpr;

    void accept(ASTVisitor& visitor) override;
};

struct IdentExpr : public Expr {
    std::string name;

    IdentExpr(const monkey::location& loc, std::string name) : Expr(loc), name(std::move(name)) {}

    void accept(ASTVisitor& visitor) override;
};

struct UnaryExpr : public Expr {
    std::unique_ptr<Expr> operand;
    const char* prefix;

    UnaryExpr(const monkey::location& loc, Expr* expr, const char* op) : Expr(loc), operand(expr), prefix(op) {}
    UnaryExpr(const monkey::location& loc, std::unique_ptr<Expr> expr, const char* op)
        : Expr(loc), operand(std::move(expr)), prefix(op) {}

    void accept(ASTVisitor& visitor) override;
};

struct BinOpExpr : public Expr {
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    const char* op;

    BinOpExpr(const monkey::location& loc, Expr* left, Expr* right, const char* op)
        : Expr(loc), left(left), right(right), op(op) {}
    BinOpExpr(const monkey::location& loc, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, const char* op)
        : Expr(loc), left(std::move(left)), right(std::move(right)), op(op) {}

    void accept(ASTVisitor& visitor) override;
};

struct ArrayExpr : public Expr {
    std::unique_ptr<ExprSeq> expr_seq;
    ArrayExpr(const monkey::location& loc, ExprSeq* expr_seq) : Expr(loc), expr_seq(expr_seq) {}
    ArrayExpr(const monkey::location& loc, std::unique_ptr<ExprSeq>& expr_seq)
        : Expr(loc), expr_seq(std::move(expr_seq)) {}

    void accept(ASTVisitor& visitor) override;
};

struct ArrayDerefExpr : public Expr {
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;

    ArrayDerefExpr(const monkey::location& loc, Expr* target, Expr* index)
        : Expr(loc), target(target), index(index) {}
    ArrayDerefExpr(const monkey::location& loc, std::unique_ptr<Expr> target, std::unique_ptr<Expr> index)
        : Expr(loc), target(std::move(target)), index(std::move(index)) {}

    void accept(ASTVisitor& visitor) override;
};

struct ExprStmt : public Stmt {
    std::unique_ptr<Expr> expression;

    ExprStmt(const monkey::location& loc, Expr* expr) : Stmt(loc), expression(expr) {}
    ExprStmt(const monkey::location& loc, std::unique_ptr<Expr> expr) : Stmt(loc), expression(std::move(expr)) {}
    void accept(ASTVisitor& visitor) override;
};

struct LetExpr : public Expr {
    std::string ident;
    std::unique_ptr<Expr> value;

    LetExpr(const monkey::location& loc, std::string ident, Expr* value)
        : Expr(loc), ident(std::move(ident)), value(value) {}
    LetExpr(const monkey::location& loc, std::string ident, std::unique_ptr<Expr> value)
        : Expr(loc), ident(std::move(ident)), value(std::move(value)) {}
    void accept(ASTVisitor& visitor) override;
};

struct BlockStmt : public Stmt {
    std::unique_ptr<StmtList> stmtList;

    BlockStmt(const monkey::location& loc, StmtList* stmts, int adjustment) : Stmt(loc), stmtList(stmts) {
        stmtList->setIndentationLvl(adjustment);
    }
    BlockStmt(const monkey::location& loc, std::unique_ptr<StmtList> stmts, int adjustment)
        : Stmt(loc), stmtList(std::move(stmts)) {
        stmtList->setIndentationLvl(adjustment);
    }

    void accept(ASTVisitor& visitor) override;
};

struct ElifList {
    std::vector<std::unique_ptr<Expr>> conditions;
    std::vector<std::unique_ptr<BlockStmt>> branches;

    void append(Expr* cond, BlockStmt* branch) {
        conditions.emplace_back(cond);
        branches.emplace_back(branch);
    }

    void append(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> branch) {
        conditions.emplace_back(std::move(cond));
        branches.emplace_back(std::move(branch));
    }
};

struct IfStmt : public Stmt {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<BlockStmt> truthy_branch;
    std::unique_ptr<ElifList> elseIfs;
    std::optional<std::unique_ptr<BlockStmt>> optElse;

    IfStmt(const monkey::location& loc, Expr* cond, BlockStmt* truthy_branch, ElifList* elseIfs,
           std::optional<BlockStmt*> optElse)
        : Stmt(loc), cond(cond), truthy_branch(truthy_branch), elseIfs(elseIfs) {
        if (optElse) {
            this->optElse = std::make_optional(std::unique_ptr<BlockStmt>(*optElse));
        }
    }

    IfStmt(const monkey::location& loc, std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> truthy_branch,
           std::unique_ptr<ElifList> elseIfs, std::optional<std::unique_ptr<BlockStmt>> optElse)
        : Stmt(loc), cond(std::move(cond)), truthy_branch(std::move(truthy_branch)), elseIfs(std::move(elseIfs)),
          optElse(std::move(optElse)) {}

    void setIndentationLvl(int adjustment) override;
    void accept(ASTVisitor& visitor) override;
};

} // namespace ast
