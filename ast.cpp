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
        std::ostringstream ss;
        return "(" + literal + ")" ; 
    }

    std::string FloatLitExpr::str() const { 
        return "(" + literal + ")";
    }

    std::string StringLitExpr::str() const {
        return "\"" + literal + "\"";
    }   

    std::string UnaryExpr::str() const { 
        return std::string(prefix) + operand->str();
    }

    std::string BinOpExpr::str() const {
        auto prefix = Node::str();
        prefix += "(" + left->str() + " " + std::string(op) + " " + right->str() + ")"; 
        return prefix;
    }

    std::string Stmt::str() const { return std::string(nested_lvl, '\t'); }

    std::string BlockStmt::str() const {
        std::string identdation = Stmt::str();
        return identdation + "{\n" + stmtList->str() + identdation + "\n}";
    }

    std::string StmtList::str() const {
        std::string result;
        for (const auto& stmt : statements) {
            result += stmt->str() + "\n";
        }
        return result;
    }

    std::string ExprStmt::str() const {
        std::string identation = Stmt::str(); 
        return identation + expression->str() + ";";
    }

    std::string LetStmt::str() const {
        std::string identation = Stmt::str();
        return identation + "let " + ident + " = " + value->str() + ";";
    }
}