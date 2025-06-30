#include "fstream"
#include "Scanner.hpp"
#include "Parser.hpp"


int main() {
    std::ifstream inputFile; // Declare the ifstream object
    inputFile.open("test.txt"); // 
    monkey::Scanner scanner{inputFile , std::cerr };
    monkey::Parser parser{ &scanner };
    std::cout.precision(10);
    parser.parse();
}