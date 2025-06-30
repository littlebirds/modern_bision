#include "fstream"
#include "Scanner.hpp"
#include "Parser.hpp"


int main() {
    std::ifstream inputFile; // Declare the ifstream object
    inputFile.open("test.txt"); // 
    monkey::Scanner scanner{inputFile , std::cerr };
    ast::Node* pAST = nullptr;
    monkey::Parser parser{ &scanner, pAST };
    std::cout.precision(10);
    parser.parse();
    if (pAST) {
        std::cout << pAST->str() << std::endl;
        delete pAST; // Clean up the allocated memory
    } else {
        std::cerr << "Parsing failed or produced no AST." << std::endl;
    }
}