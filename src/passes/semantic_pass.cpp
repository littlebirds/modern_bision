#include "passes/semantic_pass.hpp"
#include "ast_visitor.hpp"
#include "symtab.hpp"
#include <optional>
#include <stdexcept>

using ast::SemanticInfo;

namespace eval {
namespace {

struct SemanticSym {
    TypeId type = TYPE_UNKNOWN;
    TypeId returnType = TYPE_UNKNOWN; // for function symbols: the return type
    std::vector<std::string> parameterNames;
    std::vector<TypeId> parameterTypes;
    bool isMutable = true;
    bool isFunction = false;
};

// Internal AST visitor — not exposed as a Pass, just an implementation detail.
class SemanticVisitor : public ast::ASTVisitor {
public:
    SemanticVisitor() : scope_(std::make_shared<SemScope>()) {}

    TypeId resolveType(ast::Expr& expr) { expr.accept(*this); return curType_; }

    // --- expressions ---
    void visit(ast::IntLitExpr& node) override {
        curType_ = TYPE_INT; node.setAnnotation(SemanticInfo{TYPE_INT}); }
    void visit(ast::FloatLitExpr& node) override {
        curType_ = TYPE_FLOAT; node.setAnnotation(SemanticInfo{TYPE_FLOAT}); }
    void visit(ast::BoolLitExpr& node) override {
        curType_ = TYPE_BOOL; node.setAnnotation(SemanticInfo{TYPE_BOOL}); }
    void visit(ast::StringLitExpr& node) override {
        curType_ = TYPE_STRING; node.setAnnotation(SemanticInfo{TYPE_STRING}); }

    void visit(ast::IdentExpr& node) override {
        const auto* sym = scope_->resolve(node.name);
        if (!sym) throw std::runtime_error("Undefined variable: " + node.name);
        curType_ = sym->type;
        node.setAnnotation(SemanticInfo{sym->type});
    }

    void visit(ast::UnaryExpr& node) override {
        TypeId t = resolveType(*node.operand);
        std::string_view op = node.prefix;
        if (op == "-") {
            if (t != TYPE_INT && t != TYPE_FLOAT)
                throw std::runtime_error("Cannot negate non-numeric value");
            curType_ = t;
        } else if (op == "not") {
            curType_ = TYPE_BOOL;
        }
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::BinOpExpr& node) override {
        std::string_view op = node.op;
        TypeId l = resolveType(*node.left);
        if (op == "and" || op == "or") {
            resolveType(*node.right);
            curType_ = l; // or rhs type
            node.setAnnotation(SemanticInfo{curType_});
            return;
        }
        TypeId r = resolveType(*node.right);
        if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || op == "^") {
            if (!((l == TYPE_INT || l == TYPE_FLOAT) && (r == TYPE_INT || r == TYPE_FLOAT)))
                throw std::runtime_error(std::string("Non-numeric operands for '") + std::string(op) + "'");
            curType_ = (l == TYPE_FLOAT || r == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;
        } else if (op == ">" || op == "<" || op == ">=" || op == "<=" || op == "==" || op == "!=") {
            curType_ = TYPE_BOOL;
        }
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::LetExpr& node) override {
        if (TypeTable::isKnownTypeName(node.ident))
            throw std::runtime_error("Cannot use type name '" + node.ident + "' as identifier");
        TypeId t = resolveType(*node.value);
        auto* fn = dynamic_cast<ast::FnLitExpr*>(node.value.get());
        if (t == TYPE_VOID && !fn)
            throw std::runtime_error("Cannot bind a void value");
        // If the value is a function literal, record it as a function symbol with its return type
        bool isFn = fn != nullptr;
        std::vector<std::string> parameterNames;
        std::vector<TypeId> parameterTypes;
        if (isFn) {
            parameterNames.reserve(fn->params.size());
            parameterTypes.reserve(fn->params.size());
            for (const auto& [name, typeName] : fn->params) {
                parameterNames.push_back(name);
                parameterTypes.push_back(TypeTable::resolveTypeName(typeName));
            }
        }
        scope_->define(node.ident,
            SemanticSym{t, isFn ? t : TYPE_UNKNOWN,
                        std::move(parameterNames), std::move(parameterTypes), true, isFn});
        curType_ = t;
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::AssignExpr& node) override {
        auto* ex = scope_->resolveMut(node.ident);
        if (!ex) throw std::runtime_error("Undefined variable: " + node.ident);
        TypeId t = resolveType(*node.value);
        if (t != ex->type) throw std::runtime_error("Type mismatch in assignment to '" + node.ident + "'");
        curType_ = t;
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::FnLitExpr& node) override {
        std::vector<TypeId> ptids;
        for (auto& [name, tn] : node.params)
            ptids.push_back(TypeTable::resolveTypeName(tn));
        TypeId retTid = TypeTable::resolveTypeName(node.returnType);
        TypeId enclosingReturnType = declaredReturnType_;
        declaredReturnType_ = retTid;
        enterScope();
        for (size_t i = 0; i < node.params.size(); ++i)
            scope_->define(node.params[i].first,
                SemanticSym{ptids[i], TYPE_UNKNOWN, {}, {}, true, false});
        curType_ = TYPE_UNKNOWN;
        node.body->accept(*this);
        TypeId bodyType = curType_;
        exitScope();
        declaredReturnType_ = enclosingReturnType;

        // A trailing expression is the function's implicit return value. Explicit
        // return statements are checked in visit(ReturnStmt&).
        if (retTid != TYPE_VOID && !node.body->stmtList->statements.empty() &&
            dynamic_cast<ast::ExprStmt*>(node.body->stmtList->statements.back().get()) &&
            bodyType != TYPE_UNKNOWN && bodyType != retTid) {
            throwReturnTypeMismatch(retTid, bodyType);
        }

        // The function literal itself carries its return type for callers to use.
        curType_ = retTid;
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::CallExpr& node) override {
        resolveType(*node.callee);

        const size_t actualCount = node.arguments ? node.arguments->exprs.size() : 0;
        std::optional<size_t> expectedCount;
        std::vector<std::string> parameterNames;
        std::vector<TypeId> parameterTypes;

        // Resolve function metadata for named and immediately-invoked functions.
        TypeId retTid = TYPE_UNKNOWN;
        if (auto* ident = dynamic_cast<ast::IdentExpr*>(node.callee.get())) {
            const auto* sym = scope_->resolve(ident->name);
            if (sym && sym->isFunction) {
                retTid = sym->returnType;
                parameterNames = sym->parameterNames;
                parameterTypes = sym->parameterTypes;
                expectedCount = parameterTypes.size();
            }
        } else if (auto* fn = dynamic_cast<ast::FnLitExpr*>(node.callee.get())) {
            expectedCount = fn->params.size();
            parameterNames.reserve(fn->params.size());
            parameterTypes.reserve(fn->params.size());
            for (const auto& [name, typeName] : fn->params) {
                parameterNames.push_back(name);
                parameterTypes.push_back(TypeTable::resolveTypeName(typeName));
            }
            retTid = TypeTable::resolveTypeName(fn->returnType);
        }

        if (expectedCount && actualCount != *expectedCount) {
            throw std::runtime_error(
                "Function expects " + std::to_string(*expectedCount) +
                " arguments but got " + std::to_string(actualCount));
        }

        if (node.arguments) {
            for (size_t i = 0; i < node.arguments->exprs.size(); ++i) {
                TypeId actualType = resolveType(*node.arguments->exprs[i]);
                if (i < parameterTypes.size() && actualType != parameterTypes[i]) {
                    throw std::runtime_error(
                        "Argument " + std::to_string(i) + " (" + parameterNames[i] +
                        ") expected type " + TypeTable::instance().getTypeName(parameterTypes[i]) +
                        " but got " + TypeTable::instance().getTypeName(actualType));
                }
            }
        }

        curType_ = retTid;
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::ArrayExpr& node) override {
        TypeId elementType = TYPE_UNKNOWN;
        for (auto& element : node.expr_seq->exprs) {
            TypeId currentType = resolveType(*element);
            if (currentType == TYPE_VOID)
                throw std::runtime_error("Cannot use a void value as an array element");
            if (elementType == TYPE_UNKNOWN)
                elementType = currentType;
            else if (currentType != elementType)
                throw std::runtime_error("Array elements must all have the same type");
        }

        curType_ = TypeTable::instance().getArrayTypeId(
            elementType, node.expr_seq->exprs.size());
        node.setAnnotation(SemanticInfo{curType_});
    }

    void visit(ast::ArrayDerefExpr& node) override {
        TypeId targetType = resolveType(*node.target);
        const auto* resolvedArrayType = TypeTable::instance().getArrayType(targetType);
        if (!resolvedArrayType)
            throw std::runtime_error("Cannot index non-array value");
        ArrayType arrayType = *resolvedArrayType;

        TypeId indexType = resolveType(*node.index);
        if (indexType != TYPE_INT)
            throw std::runtime_error("Array index must be an integer");

        if (auto index = constantInteger(*node.index);
            index && (*index < 0 || static_cast<size_t>(*index) >= arrayType.length)) {
            throw std::runtime_error(
                "Array index out of bounds: index " + std::to_string(*index) +
                " for array of length " + std::to_string(arrayType.length));
        }

        curType_ = arrayType.element_tid;
        node.setAnnotation(SemanticInfo{curType_});
    }
    void visit(ast::ExprSeq& node) override {
        for (auto& e : node.exprs) resolveType(*e); }

    // --- statements ---
    void visit(ast::ExprStmt& node) override { resolveType(*node.expression); }
    void visit(ast::BlockStmt& node) override {
        enterScope(); node.stmtList->accept(*this); exitScope(); }
    void visit(ast::IfStmt& node) override {
        resolveType(*node.cond);
        if (node.truthy_branch) node.truthy_branch->accept(*this);
        if (node.elseIfs)
            for (size_t i = 0; i < node.elseIfs->conditions.size(); ++i) {
                resolveType(*node.elseIfs->conditions[i]);
                node.elseIfs->branches[i]->accept(*this);
            }
        if (node.optElse) node.optElse.value()->accept(*this);
    }
    void visit(ast::WhileStmt& node) override {
        resolveType(*node.cond); node.body->accept(*this); }
    void visit(ast::StmtList& node) override {
        for (auto& s : node.statements) s->accept(*this); }
    void visit(ast::ReturnStmt& node) override {
        TypeId actualType = node.value ? resolveType(*node.value) : TYPE_VOID;
        if (declaredReturnType_ != TYPE_UNKNOWN && actualType != TYPE_UNKNOWN &&
            actualType != declaredReturnType_)
            throwReturnTypeMismatch(declaredReturnType_, actualType);
        curType_ = actualType;
    }

private:
    using SemScope = Scope<SemanticSym>;
    std::shared_ptr<SemScope> scope_;
    TypeId curType_ = TYPE_UNKNOWN;
    TypeId declaredReturnType_ = TYPE_UNKNOWN;
    void enterScope() { scope_ = std::make_shared<SemScope>(scope_); }
    void exitScope() { scope_ = scope_->parent(); }
    std::optional<int64_t> constantInteger(ast::Expr& expression) {
        if (auto* literal = dynamic_cast<ast::IntLitExpr*>(&expression))
            return std::stoll(literal->literal);
        if (auto* unary = dynamic_cast<ast::UnaryExpr*>(&expression);
            unary && unary->prefix == std::string_view("-")) {
            if (auto* literal = dynamic_cast<ast::IntLitExpr*>(unary->operand.get()))
                return -std::stoll(literal->literal);
        }
        return std::nullopt;
    }
    [[noreturn]] void throwReturnTypeMismatch(TypeId declaredType, TypeId actualType) {
        throw std::runtime_error(
            "Function declared return type " +
            TypeTable::instance().getTypeName(declaredType) +
            " but returned " + TypeTable::instance().getTypeName(actualType));
    }
};

} // anon namespace

void semanticAnalysis(Compiler& c) {
    SemanticVisitor sem;
    c.ast->accept(sem);
}

} // namespace eval
