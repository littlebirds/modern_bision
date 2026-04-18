#include "ast.hpp"
#include "ast_visitor.hpp"

namespace ast {

// accept() implementations
void IntLitExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
void FloatLitExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
void StringLitExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
void IdentExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
void UnaryExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
void BinOpExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
void ArrayExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
void LetExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
void ExprSeq::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
void ExprStmt::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
void BlockStmt::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
void IfStmt::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
void StmtList::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IfStmt::setIndentationLvl(int adjustment) {
    truthy_branch->setIndentationLvl(adjustment);
    if (elseIfs) {
        for (auto& elseBlock : elseIfs->branches) {
            elseBlock->setIndentationLvl(adjustment);
        }
    }
    if (optElse) {
        optElse.value()->setIndentationLvl(adjustment);
    }
}
} // namespace ast
