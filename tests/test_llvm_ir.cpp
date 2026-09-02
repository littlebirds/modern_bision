#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include "Parser.hpp"
#include "Scanner.hpp"
#include "ast.hpp"
#include "compiler.hpp"
#include "passes/llvm_ir_pass.hpp"
#include "passes/semantic_pass.hpp"

namespace {

std::unique_ptr<ast::Node> parse(const std::string& input) {
    std::istringstream stream(input);
    std::unique_ptr<ast::Node> root;
    monkey::Scanner scanner{stream, std::cerr};
    monkey::Parser parser{&scanner, root};
    if (parser.parse() != 0 || !root)
        throw std::runtime_error("Parse failed");
    return root;
}

std::string compileToIR(const std::string& input) {
    auto root = parse(input);
    auto* program = dynamic_cast<ast::StmtList*>(root.get());
    if (!program)
        throw std::runtime_error("Expected program AST");

    eval::Compiler compiler;
    compiler.ast = program;
    eval::semanticAnalysis(compiler);
    eval::llvmIRGen(compiler);

    std::string verificationError;
    llvm::raw_string_ostream errorStream(verificationError);
    if (llvm::verifyModule(*compiler.module, &errorStream)) {
        errorStream.flush();
        throw std::runtime_error("Invalid LLVM IR: " + verificationError);
    }

    std::string ir;
    llvm::raw_string_ostream irStream(ir);
    compiler.module->print(irStream, nullptr);
    irStream.flush();
    return ir;
}

void analyze(const std::string& input) {
    auto root = parse(input);
    auto* program = dynamic_cast<ast::StmtList*>(root.get());
    if (!program)
        throw std::runtime_error("Expected program AST");

    eval::Compiler compiler;
    compiler.ast = program;
    eval::semanticAnalysis(compiler);
}

} // namespace

TEST_CASE("LLVM backend: compiles array literals and indexing", "[llvm][array]") {
    CHECK_NOTHROW(compileToIR("[10, 20, 30];"));
    std::string ir = compileToIR("let numbers = [10, 20, 30]; numbers[0];");
    CHECK(ir.find("[3 x i64]") != std::string::npos);
    CHECK(ir.find("array.element") != std::string::npos);
}

TEST_CASE("LLVM backend: emits dynamic array bounds checks", "[llvm][array]") {
    std::string ir = compileToIR(
        "let numbers = [10, 20, 30]; let index = 1; numbers[index];");
    CHECK(ir.find("index.out_of_bounds") != std::string::npos);
    CHECK(ir.find("llvm.trap") != std::string::npos);
    CHECK(ir.find("getelementptr inbounds") != std::string::npos);
}

TEST_CASE("LLVM backend: compiles nested and empty arrays", "[llvm][array]") {
    CHECK_NOTHROW(compileToIR(
        "let matrix = [[1, 2], [3, 4]]; matrix[1][0];"));
    CHECK(compileToIR("let empty = []; 0;").find("[0 x i8]") != std::string::npos);
}

TEST_CASE("Semantic analysis: rejects an out-of-bounds array index", "[semantic][array]") {
    CHECK_THROWS_WITH(
        analyze("let values = [1, 2]; values[2];"),
        "Array index out of bounds: index 2 for array of length 2");
    CHECK_THROWS_WITH(
        analyze("let values = [1, 2]; values[-1];"),
        "Array index out of bounds: index -1 for array of length 2");
}

TEST_CASE("Semantic analysis: rejects invalid array index types and targets", "[semantic][array]") {
    CHECK_THROWS_WITH(
        analyze("let values = [1, 2]; values[1.5];"),
        "Array index must be an integer");
    CHECK_THROWS_WITH(
        analyze("let value = 1; value[0];"),
        "Cannot index non-array value");
}
