#include "ast.hpp"
#include "ast_visitor.hpp"  

namespace ast {
    std::string IntLitExpr::str() const {
        return "iexp(" + std::to_string(value) + ")"; 
    }

    std::string FloatLitExpr::str() const {
        return "fexp(" + std::to_string(value) + ")";
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
        return expression->str() + ";";
    }

    std::string LetStmt::str() const {
        return "let " + ident + " = " + value->str() + ";";
    }
}