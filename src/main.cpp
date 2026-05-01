#include "fstream"
#include <cstring>
#include <sstream>
#include <llvm/Support/raw_ostream.h>
#include "Scanner.hpp"
#include "Parser.hpp"
#include "interpreter.hpp"
#include "compiler.hpp"
#include "passes/semantic_pass.hpp"
#include "passes/llvm_ir_pass.hpp"

void banner() {
    std::cout << "Monkey Programming Language R.E.P.L" << std::endl;
    std::cout << "Version 0.1"
              << "\n\n";
    std::cout << "Type ctrl-D to exit REPL." << std::endl;
    std::cout << "<---------------------------------------------------->" << std::endl;
}

void usage() {
    std::cout << " Usage: monkey [-i | -f <filename> | -c <filename>]" << std::endl;
    std::cout << " -i: Interactive REPL mode" << std::endl;
    std::cout << " -f <filename>: Interpret file" << std::endl;
    std::cout << " -c <filename>: Compile file to LLVM IR" << std::endl;
    std::cout << " Example: monkey -f test.txt" << std::endl;
    std::cout << " Example: monkey -i" << std::endl;
    std::cout << " Example: monkey -c test.txt" << std::endl;
}

static bool isInputComplete(const std::string& input) {
    int brace_bal = 0;
    int bracket_bal = 0;
    int paren_bal = 0;
    bool in_string = false;
    bool in_comment = false;
    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        if (in_comment) {
            if (c == '\n') in_comment = false;
            continue;
        }
        if (in_string) {
            if (c == '\\') { i++; continue; }
            if (c == '"') in_string = false;
            continue;
        }
        if (c == '"') { in_string = true; continue; }
        if (c == '/' && i + 1 < input.size() && input[i + 1] == '/') {
            in_comment = true;
            i++;
            continue;
        }
        if (c == '{') brace_bal++;
        else if (c == '}') brace_bal--;
        else if (c == '[') bracket_bal++;
        else if (c == ']') bracket_bal--;
        else if (c == '(') paren_bal++;
        else if (c == ')') paren_bal--;
    }

    if (brace_bal != 0 || bracket_bal != 0 || paren_bal != 0) return false;

    int j = static_cast<int>(input.size()) - 1;
    while (j >= 0 && (input[j] == ' ' || input[j] == '\t' || input[j] == '\n' || input[j] == '\r')) j--;
    if (j < 0) return true;
    return input[j] == ';' || input[j] == '}';
}

int main(int argc, char** argv) {

    if (argc < 2) {
        usage();
        return 1;
    }
    const char* mode = argv[1];
    if (strncmp(mode, "-i", 2) == 0) {
        banner();
        eval::Interpreter interpreter;
        std::string buffer;
        bool continuation = false;

        while (true) {
            std::cout << (continuation ? "... " : ">> ") << std::flush;

            std::string line;
            if (!std::getline(std::cin, line)) {
                std::cout << std::endl;
                break;
            }

            if (continuation && (line.empty() || (!line.empty() && line[0] == '\x1b'))) {
                std::cout << "canceled" << std::endl;
                buffer.clear();
                continuation = false;
                continue;
            }

            buffer += line + "\n";

            if (!isInputComplete(buffer)) {
                continuation = true;
                continue;
            }

            std::istringstream iss(buffer);
            std::unique_ptr<ast::Node> pAST;
            monkey::Scanner scanner{iss, std::cerr};
            monkey::Parser parser{&scanner, pAST};

            int parse_result = parser.parse();

            if (parse_result == 0 && pAST) {
                try {
                    pAST->accept(interpreter);
                    eval::Value result = interpreter.result();
                    if (!result.isNull()) {
                        std::cout << result.toString() << std::endl;
                    }
                } catch (const std::exception& ex) {
                    std::cerr << "Runtime error: " << ex.what() << std::endl;
                }
            } else {
                std::cerr << "Parse error." << std::endl;
            }

            buffer.clear();
            continuation = false;
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
        monkey::Parser parser{&scanner, pAST};
        parser.parse();
        if (pAST) {
            eval::Interpreter interpreter;
            try {
                pAST->accept(interpreter);
                eval::Value result = interpreter.result();
                if (!result.isNull()) {
                    std::cout << result.toString() << std::endl;
                }
            } catch (const std::exception& ex) {
                std::cerr << "Runtime error: " << ex.what() << std::endl;
                return 1;
            }
            return 0;
        } else {
            std::cerr << "Parsing failed to produce AST." << std::endl;
            return 1;
        }
    }

    if (strncmp(mode, "-c", 2) == 0) {
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
        monkey::Parser parser{&scanner, pAST};
        parser.parse();
        if (!pAST) {
            std::cerr << "Parsing failed to produce AST." << std::endl;
            return 1;
        }
        try {
            eval::Compiler c;
            c.ast = static_cast<ast::StmtList*>(pAST.get());
            c.addPass("semantic",   eval::semanticAnalysis);
            c.addPass("llvm-ir-gen", eval::llvmIRGen);
            c.run();
            if (c.module)
                c.module->print(llvm::errs(), nullptr);
        } catch (const std::exception& ex) {
            std::cerr << "Compilation error: " << ex.what() << std::endl;
            return 1;
        }
        return 0;
    }
}
