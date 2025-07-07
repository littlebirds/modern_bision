#include "ast.hpp"
#include "ast_visitor.hpp"
#include <sstream>
 

namespace ast {    
    
    std::string Node::str() const {

        std::ostringstream ss;
        ss << "@" << loc << ">>";
        return ss.str();
    }

    std::string IntLitExpr::str() const {
        return "(" + literal + ")" ; 
    }

    std::string FloatLitExpr::str() const { 
        return "(" + literal + ")";
    }

    std::string StringLitExpr::str() const {         
        return Node::str() + "\"" + literal + "\"";
    }   

    std::string UnaryExpr::str() const { 
        return std::string(prefix) + operand->str();
    }

    std::string BinOpExpr::str() const { 
        return std::string("(") + left->str() + " " + std::string(op) + " " + right->str() + ")";  
    }
 
    std::string Stmt::indentate(const char* content) const { return (std::string(nested_lvl, '\t') + content); }

    std::string BlockStmt::str() const {
        return indentate("{\n") + stmtList->str() + indentate("}\n");
    }

    std::string StmtList::str() const {
        std::string result;
        for (const auto& stmt : statements) {
            result += stmt->str() + "\n";
        }
        return result;
    }

    std::string ExprStmt::str() const {
        return indentate("") + expression->str() + ";";
    }

    std::string LetStmt::str() const {
        return indentate("let ") + ident + " = " + value->str() + ";";
    } 

    void IfStmt::setIndentationLvl(int adjustment) {
        truthy_branch->setIndentationLvl(adjustment);
        if (elseIfs) {
            for (const auto& [_, elseBlock] : elseIfs->branches) {
                elseBlock->setIndentationLvl(adjustment);
            }
        }
        if (optElse) {
            optElse.value()->setIndentationLvl(adjustment);
        }
    }

    std::string IfStmt::str() const { 
        std::string result = indentate("if")  + cond->str() + "\n";
        result.append(truthy_branch->str());
        if (elseIfs) {
            for (const auto& [elseCond, elseBlock] : elseIfs->branches) {
                result.append(indentate("elif") + elseCond->str() + "\n");
                result.append(elseBlock->str());
            }
        }
        if (optElse) {
            result.append(optElse.value()->str());
        }
        return result; 
    }
}