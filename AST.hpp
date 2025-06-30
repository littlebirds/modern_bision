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
    virtual ~Expr() = default;
    virtual std::string str() const = 0;
};

struct Stmt : public Node {
    virtual ~Stmt() = default;
    virtual std::string str() const = 0;
};

struct Program : public Node {
    std::vector<std::unique_ptr<Stmt>> statements;

    Program() = default;

    void appendStmt(std::unique_ptr<Stmt> stmt) {
        statements.push_back(std::move(stmt));
    }

    std::string str() const override {
        std::string result {"program: \n"};
        for (const auto& stmt : statements) {
            result.append(stmt->str() + "\n");
        }
        return result;
    }
};

struct IntLitExpr : public Expr {
    long long value;

    IntLitExpr(long long value) : value(value) {}
    std::string str() const override {
        return std::to_string(value);   
    }
};

struct FloatLitExpr : public Expr {
    double value;
    FloatLitExpr(double value) : value(value) {}
    std::string str() const override {
        return std::to_string(value);
    }
}; 

struct StringLitExpr : public Expr {
    std::string value;
    StringLitExpr(std::string value) : value(std::move(value)) {}
    std::string str() const override {
        return "\"" + value + "\"";
    }
};

struct BinOpExpr : public Expr {
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    const char* const op;

    BinOpExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, const char* op)
        : left(std::move(left)), right(std::move(right)), op(op) {}

    std::string str() const override {
        return "(" + left->str() + " " + std::string(op) + " " + right->str() + ")";
    }
};

struct ExprStmt : public Stmt {
    std::unique_ptr<Expr> expr;

    ExprStmt(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}

    std::string str() const override {
        return expr->str() + ";";
    }
};

} // namespace ast