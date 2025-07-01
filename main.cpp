#include "fstream"
#include <cstring>
#include "Scanner.hpp"
#include "Parser.hpp"



void banner() {
    std::cout << "Monkey Programming Language R.E.P.L" << std::endl;
    std::cout << "Version 0.1" << std::endl; 
    std::cout << "Type ctrl-c to exit." << std::endl;

}

void usage() {
    std::cout << " Usage: monkey [-i | -f <filename>]" << std::endl;
    std::cout << " -i: Interactive mode" << std::endl;
    std::cout << " -f <filename>: Read from file" << std::endl;
    std::cout << " Example: monkey -f test.txt" << std::endl;
    std::cout << " Example: monkey -i" << std::endl;
}

int main(int argc, char** argv) {
    ast::Node* pAST = nullptr;
    if (argc < 2) {
        usage();
        return 1;
    }
    const char* mode = argv[1];
    if (strncmp(mode, "-i", 2) == 0) {
        banner(); 
        monkey::Scanner scanner{std::cin, std::cerr};
        monkey::Parser parser{ &scanner, pAST };
        return 0; // Exit immediately for interactive mode
    }

    if (strncmp(mode, "-f", 2) == 0) {
        if (argc != 3) {
            std::cerr << "Error: Missing filename argument." << std::endl;
            return 1;
        }
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
            delete pAST;
            return 0; // Clean up the allocated memory
        } else {
            std::cerr << "Parsing failed to produce AST." << std::endl;
            return 1;
        }
    }


    std::ifstream inputFile; // Declare the ifstream object
    inputFile.open("test.txt"); // 
    monkey::Scanner scanner{inputFile , std::cerr };
    monkey::Parser parser{ &scanner, pAST };
    std::cout.precision(10);
    parser.parse();
    
}