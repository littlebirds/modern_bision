#include "interpreter.hpp"
#include <cmath>
#include <stdexcept>
#include <string_view>

namespace eval {

Value Interpreter::evaluate(ast::Expr& expr) {
    expr.accept(*this);
    return result_;
}

// --- Literals ---

void Interpreter::visit(ast::IntLitExpr& node) {
    result_ = Value(static_cast<int64_t>(std::stoll(node.literal)));
}

void Interpreter::visit(ast::FloatLitExpr& node) {
    result_ = Value(std::stod(node.literal));
}

void Interpreter::visit(ast::StringLitExpr& node) {
    result_ = Value(std::make_shared<StringObject>(node.literal));
}

// --- Identifier ---

void Interpreter::visit(ast::IdentExpr& node) {
    result_ = ctx_->get(node.name);
}

// --- Unary ---

void Interpreter::visit(ast::UnaryExpr& node) {
    Value operand = evaluate(*node.operand);
    std::string_view op = node.prefix;

    if (op == "-") {
        if (operand.isInt()) {
            result_ = Value(-operand.asInt());
        } else if (operand.isFloat()) {
            result_ = Value(-operand.asFloat());
        } else {
            throw std::runtime_error("Cannot negate non-numeric value");
        }
    } else {
        throw std::runtime_error(std::string("Unknown unary operator: ") + node.prefix);
    }
}

// --- Binary helpers ---

Value Interpreter::applyArithmetic(const Value& lhs, const Value& rhs, std::string_view op) {
    if (!lhs.isNumber() || !rhs.isNumber()) {
        throw std::runtime_error(std::string("Cannot apply '") + std::string(op) + "' to non-numeric values");
    }

    bool useFloat = lhs.isFloat() || rhs.isFloat();

    if (useFloat) {
        double a = lhs.asFloat();
        double b = rhs.asFloat();
        if (op == "+") return Value(a + b);
        if (op == "-") return Value(a - b);
        if (op == "*") return Value(a * b);
        if (op == "/") {
            if (b == 0.0) throw std::runtime_error("Division by zero");
            return Value(a / b);
        }
        if (op == "%") {
            if (b == 0.0) throw std::runtime_error("Division by zero");
            return Value(std::fmod(a, b));
        }
        if (op == "^") return Value(std::pow(a, b));
    } else {
        int64_t a = lhs.asInt();
        int64_t b = rhs.asInt();
        if (op == "+") return Value(a + b);
        if (op == "-") return Value(a - b);
        if (op == "*") return Value(a * b);
        if (op == "/") {
            if (b == 0) throw std::runtime_error("Division by zero");
            return Value(a / b);
        }
        if (op == "%") {
            if (b == 0) throw std::runtime_error("Division by zero");
            return Value(a % b);
        }
        if (op == "^") return Value(static_cast<int64_t>(std::pow(a, b)));
    }

    throw std::runtime_error(std::string("Unknown arithmetic operator: ") + std::string(op));
}

Value Interpreter::applyComparison(const Value& lhs, const Value& rhs, std::string_view op) {
    if (op == "==") return Value(lhs == rhs);
    if (op == "!=") return Value(lhs != rhs);

    if (!lhs.isNumber() || !rhs.isNumber()) {
        throw std::runtime_error(std::string("Cannot compare non-numeric values with '") + std::string(op) + "'");
    }

    bool useFloat = lhs.isFloat() || rhs.isFloat();

    if (useFloat) {
        double a = lhs.asFloat();
        double b = rhs.asFloat();
        if (op == ">") return Value(a > b);
        if (op == "<") return Value(a < b);
        if (op == ">=") return Value(a >= b);
        if (op == "<=") return Value(a <= b);
    } else {
        int64_t a = lhs.asInt();
        int64_t b = rhs.asInt();
        if (op == ">") return Value(a > b);
        if (op == "<") return Value(a < b);
        if (op == ">=") return Value(a >= b);
        if (op == "<=") return Value(a <= b);
    }

    throw std::runtime_error(std::string("Unknown comparison operator: ") + std::string(op));
}

// --- Binary ---

void Interpreter::visit(ast::BinOpExpr& node) {
    std::string_view op = node.op;

    Value lhs = evaluate(*node.left);

    // Short-circuit logical operators
    if (op == "and") {
        if (!lhs.isTruthy()) {
            result_ = lhs;
            return;
        }
        result_ = evaluate(*node.right);
        return;
    }
    if (op == "or") {
        if (lhs.isTruthy()) {
            result_ = lhs;
            return;
        }
        result_ = evaluate(*node.right);
        return;
    }

    Value rhs = evaluate(*node.right);

    // Arithmetic
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || op == "^") {
        result_ = applyArithmetic(lhs, rhs, op);
        return;
    }

    // Comparison
    if (op == ">" || op == "<" || op == ">=" || op == "<=" || op == "==" || op == "!=") {
        result_ = applyComparison(lhs, rhs, op);
        return;
    }

    throw std::runtime_error(std::string("Unknown binary operator: ") + std::string(op));
}

// --- Compound nodes ---

void Interpreter::visit(ast::ArrayExpr& node) {
    std::vector<Value> elements;
    elements.reserve(node.expr_seq->exprs.size());

    TypeId elementTypeId = TYPE_UNKNOWN;
    for (auto& expr : node.expr_seq->exprs) {
        Value element = evaluate(*expr);
        TypeId currentType = element.typeId();

        if (elementTypeId == TYPE_UNKNOWN) {
            elementTypeId = currentType;
        } else if (currentType != elementTypeId) {
            throw std::runtime_error("Array elements must all have the same type");
        }
        elements.push_back(std::move(element));
    }
    result_ = Value(std::make_shared<ArrayObject>(std::move(elements), elementTypeId));
}

void Interpreter::visit(ast::LetExpr& node) {
    Value val = evaluate(*node.value);
    ctx_->set(node.ident, val);
    result_ = val;
}

void Interpreter::visit(ast::ExprSeq& node) {
    result_ = Value();
    for (auto& expr : node.exprs) {
        expr->accept(*this);
    }
}

void Interpreter::visit(ast::ExprStmt& node) {
    node.expression->accept(*this);
}

void Interpreter::visit(ast::BlockStmt& node) {
    ctx_ = std::make_shared<Context>(ctx_);
    node.stmtList->accept(*this);
    ctx_ = ctx_->parent();
}

void Interpreter::visit(ast::StmtList& node) {
    result_ = Value();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void Interpreter::visit(ast::IfStmt& node) {
    Value cond = evaluate(*node.cond);
    if (cond.isTruthy()) {
        node.truthy_branch->accept(*this);
        return;
    }

    if (node.elseIfs) {
        for (size_t i = 0; i < node.elseIfs->conditions.size(); ++i) {
            Value elifCond = evaluate(*node.elseIfs->conditions[i]);
            if (elifCond.isTruthy()) {
                node.elseIfs->branches[i]->accept(*this);
                return;
            }
        }
    }

    if (node.optElse) {
        node.optElse.value()->accept(*this);
        return;
    }

    result_ = Value();
}

} // namespace eval
