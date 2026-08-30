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

TEST_CASE("Semantic analysis: accepts matching function argument types", "[semantic][function]") {
    CHECK_NOTHROW(analyze(
        "let convert = fn(count int, ratio float, enabled bool, label string) int { count; };"
        "convert(2, 0.5, true, \"ready\");"));
}

TEST_CASE("Semantic analysis: rejects a mismatched function argument type", "[semantic][function]") {
    CHECK_THROWS_WITH(
        analyze(
            "let add = fn(a int, b int) int { a + b; };"
            "add(1, false);"),
        "Argument 1 (b) expected type int but got bool");
}

TEST_CASE("Semantic analysis: validates immediate invocation argument types", "[semantic][function]") {
    CHECK_THROWS_WITH(
        analyze("fn(value float) float { value; }(\"wrong\");"),
        "Argument 0 (value) expected type float but got string");
}

TEST_CASE("Semantic analysis: accepts a matching declared return type", "[semantic][function][return]") {
    CHECK_NOTHROW(analyze("let increment = fn(value int) int { value + 1; };"));
    CHECK_NOTHROW(analyze("let positive = fn(value int) bool { return value > 0; };"));
    CHECK_NOTHROW(analyze(
        "let outer = fn() bool {"
        "  let inner = fn() int { return 1; };"
        "  return true;"
        "};"));
}

TEST_CASE("Semantic analysis: rejects a mismatched implicit return type", "[semantic][function][return]") {
    CHECK_THROWS_WITH(
        analyze("let increment = fn(value int) bool { value + 1; };"),
        "Function declared return type bool but returned int");
}

TEST_CASE("Semantic analysis: rejects a mismatched explicit return type", "[semantic][function][return]") {
    CHECK_THROWS_WITH(
        analyze("let label = fn() string { return false; };"),
        "Function declared return type string but returned bool");
}

TEST_CASE("Semantic analysis: does not validate inferred return types", "[semantic][function][return]") {
    CHECK_NOTHROW(analyze("let identity = fn(value int) { value; };"));
}
