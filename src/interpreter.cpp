#include "interpreter.hpp"
#include <cmath>
#include <stdexcept>
#include <string_view>

// TODO: Closures — capture the lexical scope (ctx_) at fn definition time instead of global_ctx_
// TODO: Built-in functions — puts(x), len(arr), push(arr, val), first(arr), last(arr), rest(arr)
// TODO: Hash literals — {key: val} syntax; requires HashObject + disambiguation from block { }
// TODO: String concatenation — "a" + "b" via + operator on StringObject
// TODO: for loop — for x in arr { } once iterator/range semantics are designed
// TODO: break / continue — requires BreakException / ContinueException similar to ReturnException

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

void Interpreter::visit(ast::BoolLitExpr& node) {
    result_ = Value(node.value);
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
    } else if (op == "not") {
        result_ = Value(!operand.isTruthy());
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

void Interpreter::visit(ast::ArrayDerefExpr& node) {
    Value target = evaluate(*node.target);
    Value index = evaluate(*node.index);

    const auto* arr = target.as<ArrayObject>();
    if (!arr) {
        throw std::runtime_error("Cannot index non-array value");
    }

    if (!index.isInt()) {
        throw std::runtime_error("Array index must be an integer");
    }

    int64_t idx = index.asInt();
    if (idx < 0 || static_cast<size_t>(idx) >= arr->size()) {
        throw std::runtime_error("Array index out of bounds");
    }

    result_ = arr->elements()[static_cast<size_t>(idx)];
}

void Interpreter::visit(ast::LetExpr& node) {
    Value val = evaluate(*node.value);
    ctx_->set(node.ident, val);
    result_ = val;
}

void Interpreter::visit(ast::AssignExpr& node) {
    Value val = evaluate(*node.value);
    ctx_->update(node.ident, val); // walks scope chain; enforces type consistency
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

void Interpreter::visit(ast::WhileStmt& node) {
    result_ = Value();
    while (true) {
        Value cond = evaluate(*node.cond);
        if (!cond.isTruthy()) break;
        // Execute body in a new scope each iteration so loop-local lets don't leak
        node.body->accept(*this);
        // ReturnException propagates naturally up to the enclosing function
    }
}

// --- Functions ---

void Interpreter::visit(ast::FnLitExpr& node) {
    std::vector<std::string> paramNames;
    std::vector<TypeId> paramTypeIds;
    paramNames.reserve(node.params.size());
    paramTypeIds.reserve(node.params.size());

    for (const auto& [name, typeName] : node.params) {
        paramNames.push_back(name);
        paramTypeIds.push_back(TypeTable::resolveTypeName(typeName));
    }

    TypeId declaredRetTid = TYPE_UNKNOWN;
    if (node.returnType) {
        declaredRetTid = TypeTable::resolveTypeName(*node.returnType);
    }

    auto fnObj = std::make_shared<FunctionObject>(
        std::move(paramNames), std::move(paramTypeIds),
        declaredRetTid, node.body.get());
    result_ = Value(std::move(fnObj));
}

void Interpreter::visit(ast::ReturnStmt& node) {
    Value val = evaluate(*node.value);
    throw ReturnException(std::move(val));
}

void Interpreter::visit(ast::CallExpr& node) {
    Value calleeVal = evaluate(*node.callee);

    auto* fnObj = const_cast<FunctionObject*>(calleeVal.as<FunctionObject>());
    if (!fnObj) {
        throw std::runtime_error("Cannot call non-function value");
    }

    // Evaluate arguments
    std::vector<Value> args;
    if (node.arguments) {
        args.reserve(node.arguments->exprs.size());
        for (auto& argExpr : node.arguments->exprs) {
            args.push_back(evaluate(*argExpr));
        }
    }

    // Check arity
    if (args.size() != fnObj->paramNames().size()) {
        throw std::runtime_error(
            "Function expects " + std::to_string(fnObj->paramNames().size()) +
            " arguments but got " + std::to_string(args.size()));
    }

    // Check argument types
    for (size_t i = 0; i < args.size(); ++i) {
        TypeId argTid = args[i].typeId();
        TypeId expectedTid = fnObj->paramTypeIds()[i];
        if (argTid != expectedTid) {
            throw std::runtime_error(
                "Argument " + std::to_string(i) + " (" + fnObj->paramNames()[i] +
                ") expected type " + TypeTable::instance().getTypeName(expectedTid) +
                " but got " + TypeTable::instance().getTypeName(argTid));
        }
    }

    // Create new scope: parent is global scope (no closures)
    auto fnCtx = std::make_shared<Context>(global_ctx_);
    for (size_t i = 0; i < args.size(); ++i) {
        fnCtx->set(fnObj->paramNames()[i], args[i]);
    }

    // Save and swap context
    auto savedCtx = ctx_;
    ctx_ = fnCtx;

    // Execute body, catching ReturnException
    Value returnVal;
    try {
        fnObj->body()->accept(*this);
        // Implicit return: result_ holds the last evaluated value
        returnVal = result_;
    } catch (const ReturnException& ret) {
        returnVal = ret.value;
    }

    // Restore context
    ctx_ = savedCtx;

    // Validate return type
    TypeId retTid = returnVal.typeId();
    TypeId declaredRetTid = fnObj->declaredReturnTypeId();

    if (declaredRetTid != TYPE_UNKNOWN) {
        // Declared return type: validate
        if (retTid != declaredRetTid) {
            throw std::runtime_error(
                "Function declared return type " +
                TypeTable::instance().getTypeName(declaredRetTid) +
                " but returned " + TypeTable::instance().getTypeName(retTid));
        }
    } else {
        // Inferred return type: set on first call, validate on subsequent
        TypeId inferredTid = fnObj->inferredReturnTypeId();
        if (inferredTid == TYPE_UNKNOWN) {
            fnObj->setInferredReturnTypeId(retTid);
        } else if (retTid != inferredTid) {
            throw std::runtime_error(
                "Function return type inferred as " +
                TypeTable::instance().getTypeName(inferredTid) +
                " but returned " + TypeTable::instance().getTypeName(retTid));
        }
    }

    result_ = returnVal;
}

} // namespace eval
