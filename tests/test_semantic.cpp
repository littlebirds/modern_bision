#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <sstream>
#include <memory>
#include <stdexcept>
#include "Scanner.hpp"
#include "Parser.hpp"
#include "ast.hpp"
#include "compiler.hpp"
#include "passes/semantic_pass.hpp"

static void analyze(const std::string& input) {
    std::istringstream stream(input);
    std::unique_ptr<ast::Node> astRoot;
    monkey::Scanner scanner{stream, std::cerr};
    monkey::Parser parser{&scanner, astRoot};

    if (parser.parse() != 0 || !astRoot)
        throw std::runtime_error("Parse failed");

    auto* program = dynamic_cast<ast::StmtList*>(astRoot.get());
    if (!program)
        throw std::runtime_error("Expected program AST");

    eval::Compiler compiler;
    compiler.ast = program;
    eval::semanticAnalysis(compiler);
}

TEST_CASE("Semantic analysis: accepts correct function argument count", "[semantic][function]") {
    CHECK_NOTHROW(analyze(
        "let add = fn(a int, b int) int { a + b; };"
        "add(1, 2);"));
}

TEST_CASE("Semantic analysis: rejects too few function arguments", "[semantic][function]") {
    CHECK_THROWS_WITH(
        analyze(
            "let add = fn(a int, b int) int { a + b; };"
            "add(1);"),
        "Function expects 2 arguments but got 1");
}

TEST_CASE("Semantic analysis: rejects too many function arguments", "[semantic][function]") {
    CHECK_THROWS_WITH(
        analyze(
            "let identity = fn(value int) int { value; };"
            "identity(1, 2);"),
        "Function expects 1 arguments but got 2");
}

TEST_CASE("Semantic analysis: validates immediate function invocation arity", "[semantic][function]") {
    CHECK_THROWS_WITH(
        analyze("fn(value int) int { value; }();"),
        "Function expects 1 arguments but got 0");
}
