#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <sstream>
#include <memory>
#include "Scanner.hpp"
#include "Parser.hpp"
#include "ast.hpp"

std::string parse(const std::string& input) {
    std::istringstream iss(input);
    std::unique_ptr<ast::Node> pAST;
    monkey::Scanner scanner{iss, std::cerr};
    monkey::Parser parser{&scanner, pAST};

    if (parser.parse() || !pAST) {
        return "";
    }
    return pAST->str();
}

TEST_CASE("Test 1: (2 -4) * (1+ 3*1*2)", "[parser]") {
    std::string input = "(2 -4) * (1+ 3*1*2);";
    std::string result = parse(input);
    REQUIRE(result != "");
    INFO("Input: " << input << " -> Result: " << result);
}

TEST_CASE("Test 2: 3.1* 2 + 3", "[parser]") {
    std::string input = "3.1* 2 + 3;";
    std::string result = parse(input);
    REQUIRE(result != "");
    INFO("Input: " << input << " -> Result: " << result);
}

TEST_CASE("Test 3: with comment", "[parser]") {
    std::string input = "4+1; // test\n";  // 先测混合
    std::string result = parse(input);
    REQUIRE(result != "");
    INFO("Input: " << input << " -> Result: " << result);
}

int main(int argc, char* argv[]) {
    Catch::Session session;
    return session.run(argc, argv);
}
