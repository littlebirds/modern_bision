#include "fstream"
#include <cstring>
#include "Scanner.hpp"
#include "Parser.hpp"



void banner() {
    std::cout << "Monkey Programming Language R.E.P.L" << std::endl;
    std::cout << "Version 0.1" << "\n\n"; 
    std::cout << "Type ctrl-D to parse input and type ctrl-c to exit REPL." << std::endl;
    std::cout << "<---------------------------------------------------->" << std::endl;

}

void usage() {
    std::cout << " Usage: monkey [-i | -f <filename>]" << std::endl;
    std::cout << " -i: Interactive mode" << std::endl;
    std::cout << " -f <filename>: Read from file" << std::endl;
    std::cout << " Example: monkey -f test.txt" << std::endl;
    std::cout << " Example: monkey -i" << std::endl;
}

int main(int argc, char** argv) {
   
    if (argc < 2) {
        usage();
        return 1;
    }
    const char* mode = argv[1];
    if (strncmp(mode, "-i", 2) == 0) {
        banner();
        ast::StmtList stmtList;
        while (true) {            
            std::unique_ptr<ast::Node> pAST;                
            monkey::Scanner scanner{std::cin, std::cerr};            
            monkey::Parser parser{ &scanner, pAST }; 
            parser.parse();
            if (pAST) {
                std::cout << pAST->str() << std::endl;
                if (auto pprog = static_cast<ast::StmtList*>(pAST.get())) {
                    for (auto& stmt: pprog->statements) {
                        stmtList.append(std::move(stmt));  
                    }
                } else {
                    std::cerr << "Error: Expected a Program node." << std::endl;
                }
            } else {
                std::cerr << "Parsing failed to produce AST." << std::endl;
            }
            clearerr(stdin);
        } 
    }

    if (strncmp(mode, "-f", 2) == 0) {
        if (argc != 3) {
            std::cerr << "Error: Missing filename argument." << std::endl;
            return 1;
        }
         std::unique_ptr<ast::Node> pAST; 
        std::ifstream input;
        input.open(argv[2], std::ios::in);
        if (!input.is_open()) {
            std::cerr << "Error: Could not open file " << argv[2] << " for reading." << std::endl;
            return 1; 
        }
        monkey::Scanner scanner{input, std::cerr};
        monkey::Parser parser{ &scanner, pAST };
        parser.parse();
        if (pAST) {
            std::cout << pAST->str() << std::endl; 
            return 0; // Clean up the allocated memory
        } else {
            std::cerr << "Parsing failed to produce AST." << std::endl;
            return 1;
        }
    }
    
}