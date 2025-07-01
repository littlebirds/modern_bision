#include "ast.hpp"
#include "ast_visitor.hpp"  

namespace ast {
    std::string IntLitExpr::str() const {
        return "IntLitExpr(" + std::to_string(value) + ")"; 
    }

    std::string FloatLitExpr::str() const {
        return "FloatLitExpr(" + std::to_string(value) + ")";
    }

    std::string Program::str() const {
        std::string result{"Program:\n"};
        for (const auto& stmt : statements) {
            result += stmt->str() + "\n";
        }
        return result;
    }

    std::string StringLitExpr::str() const {
        return "\"" + value + "\"";
    }   

    std::string BinOpExpr::str() const {
        return "(" + left->str() + " " + std::string(op) + " " + right->str() + ")";
    }

    std::string ExprStmt::str() const {
        return stmt->str() + ";";
    }
}