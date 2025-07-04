#include "ast.hpp"
#include "ast_visitor.hpp"
#include <format>  

namespace ast {    
    
    std::string IntLitExpr::str() const {
        return "lit(" + literal + ")"; 
    }

    std::string FloatLitExpr::str() const {
        return std::format("lit({})", literal);
    }

    std::string StringLitExpr::str() const {
        return "\"" + literal + "\"";
    }   

    std::string BinOpExpr::str() const {
        return "(" + left->str() + " " + std::string(op) + " " + right->str() + ")";
    }

    std::string StmtList::str() const {
        std::string result{"Program:\n"};
        for (const auto& stmt : statements) {
            result += stmt->str() + "\n";
        }
        return result;
    }

    std::string ExprStmt::str() const {
        return expression->str() + ";";
    }

    std::string LetStmt::str() const {
        return "let " + ident + " = " + value->str() + ";";
    }
}