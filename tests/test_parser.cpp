#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <sstream>
#include <memory>
#include <iostream>
#include "Scanner.hpp"
#include "Parser.hpp"
#include "ast.hpp"
#include "pretty_printer.hpp"

std::string parse(const std::string& input) {
    std::istringstream iss(input);
    std::unique_ptr<ast::Node> pAST;
    monkey::Scanner scanner{iss, std::cerr};
    monkey::Parser parser{&scanner, pAST};

    if (parser.parse() || !pAST) {
        return "";
    }

    ast::PrettyPrinter printer;
    pAST->accept(printer);
    return printer.result();
}

TEST_CASE("Test 1: (2 -4) * (1+ 3*1*2)", "[parser]") {
    std::string input = "(2 -4) * (1+ 3*1*2);";
    std::string result = parse(input);
    std::cout << "Input: " << input << "\nResult: " << result << std::endl;
    REQUIRE(result != "");
}

TEST_CASE("Test 2: 3.1* 2 + 3", "[parser]") {
    std::string input = "3.1* 2 + 3;";
    std::string result = parse(input);
    std::cout << "Input: " << input << "\nResult: " << result << std::endl;
    REQUIRE(result != "");
}

TEST_CASE("Test 3: with comment", "[parser]") {
    std::string input = "4+1; // test\n";
    std::string result = parse(input);
    std::cout << "Input: " << input << "Result: " << result << std::endl;
    REQUIRE(result != "");
}

TEST_CASE("Test 4: let statement", "[parser]") {
    std::string input = "let x = 5;";
    std::string result = parse(input);
    std::cout << "Input: " << input << "\nResult: " << result << std::endl;
    REQUIRE(result != "");
}

TEST_CASE("Test 5: let statement with expression", "[parser]") {
    std::string input = "let result = 2 + 3 * 4;";
    std::string result = parse(input);
    std::cout << "Input: " << input << "\nResult: " << result << std::endl;
    REQUIRE(result != "");
}

TEST_CASE("Test 6: let statement with string", "[parser]") {
    std::string input = "let name = \"hello\";";
    std::string result = parse(input);
    std::cout << "Input: " << input << "\nResult: " << result << std::endl;
    REQUIRE(result != "");
}

int main(int argc, char* argv[]) {
    Catch::Session session;
    return session.run(argc, argv);
}
