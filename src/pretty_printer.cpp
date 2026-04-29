#include "pretty_printer.hpp"
#include "ast.hpp"
#include <algorithm>

namespace ast {

void PrettyPrinter::visit(IntLitExpr& node) {
    output_ << "(" << node.literal << ")";
}

void PrettyPrinter::visit(FloatLitExpr& node) {
    output_ << "(" << node.literal << ")";
}

void PrettyPrinter::visit(BoolLitExpr& node) {
    output_ << (node.value ? "true" : "false");
}

void PrettyPrinter::visit(StringLitExpr& node) {
    output_ << "@" << node.loc << ">>\"" << node.literal << "\"";
}

void PrettyPrinter::visit(IdentExpr& node) {
    output_ << node.name;
}

void PrettyPrinter::visit(UnaryExpr& node) {
    output_ << node.prefix;
    node.operand->accept(*this);
}

void PrettyPrinter::visit(BinOpExpr& node) {
    output_ << "(";
    node.left->accept(*this);
    output_ << " " << node.op << " ";
    node.right->accept(*this);
    output_ << ")";
}

void PrettyPrinter::visit(ExprSeq& node) {
    for (size_t i = 0; i < node.exprs.size(); ++i) {
        node.exprs[i]->accept(*this);
        if (i < node.exprs.size() - 1) {
            output_ << ", ";
        }
    }
}

void PrettyPrinter::visit(ArrayExpr& node) {
    output_ << "[";
    node.expr_seq->accept(*this);
    output_ << "]";
}

void PrettyPrinter::visit(ArrayDerefExpr& node) {
    node.target->accept(*this);
    output_ << "[";
    node.index->accept(*this);
    output_ << "]";
}

void PrettyPrinter::visit(LetExpr& node) {
    output_ << "let " << node.ident << " = ";
    node.value->accept(*this);
}

void PrettyPrinter::visit(AssignExpr& node) {
    output_ << node.ident << " = ";
    node.value->accept(*this);
}

void PrettyPrinter::visit(ExprStmt& node) {
    // Indentation handled by parent context
    node.expression->accept(*this);
    output_ << ";";
}

void PrettyPrinter::visit(BlockStmt& node) {
    std::string indent(node.nested_lvl, '\t');
    output_ << indent << "{\n";
    node.stmtList->accept(*this);
    output_ << indent << "}\n";
}

void PrettyPrinter::visit(StmtList& node) {
    for (const auto& stmt : node.statements) {
        std::string indent(stmt->nested_lvl, '\t');
        output_ << indent;
        stmt->accept(*this);
        output_ << "\n";
    }
}

void PrettyPrinter::visit(IfStmt& node) {
    std::string indent(node.nested_lvl, '\t');
    output_ << indent << "if";
    node.cond->accept(*this);
    output_ << "\n";
    node.truthy_branch->accept(*this);

    if (node.elseIfs) {
        for (size_t i = 0; i < node.elseIfs->conditions.size(); ++i) {
            output_ << indent << "elif";
            node.elseIfs->conditions[i]->accept(*this);
            output_ << "\n";
            node.elseIfs->branches[i]->accept(*this);
        }
    }

    if (node.optElse) {
        node.optElse.value()->accept(*this);
    }
}

void PrettyPrinter::visit(FnLitExpr& node) {
    output_ << "fn(";
    for (size_t i = 0; i < node.params.size(); ++i) {
        if (i > 0) output_ << ", ";
        output_ << node.params[i].first << " " << node.params[i].second;
    }
    output_ << ")";
    if (node.returnType) {
        output_ << " " << *node.returnType;
    }
    output_ << " ";
    node.body->accept(*this);
}

void PrettyPrinter::visit(CallExpr& node) {
    node.callee->accept(*this);
    output_ << "(";
    if (node.arguments) {
        node.arguments->accept(*this);
    }
    output_ << ")";
}

void PrettyPrinter::visit(ReturnStmt& node) {
    output_ << "return ";
    node.value->accept(*this);
    output_ << ";";
}

void PrettyPrinter::visit(WhileStmt& node) {
    std::string indent(node.nested_lvl, '\t');
    output_ << indent << "while ";
    node.cond->accept(*this);
    output_ << "\n";
    node.body->accept(*this);
}

} // namespace ast
