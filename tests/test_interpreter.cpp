#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <sstream>
#include <memory>
#include "Scanner.hpp"
#include "Parser.hpp"
#include "ast.hpp"
#include "interpreter.hpp"

static eval::Value interpret(const std::string& input) {
    std::istringstream iss(input);
    std::unique_ptr<ast::Node> pAST;
    monkey::Scanner scanner{iss, std::cerr};
    monkey::Parser parser{&scanner, pAST};

    int result = parser.parse();
    if (result != 0 || !pAST) {
        throw std::runtime_error("Parse failed");
    }

    eval::Interpreter interp;
    pAST->accept(interp);
    return interp.result();
}

// --- Integer arithmetic ---

TEST_CASE("Interpreter: integer addition", "[interpreter]") {
    auto val = interpret("1 + 2;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 3);
}

TEST_CASE("Interpreter: integer subtraction", "[interpreter]") {
    auto val = interpret("10 - 3;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 7);
}

TEST_CASE("Interpreter: integer multiplication", "[interpreter]") {
    auto val = interpret("4 * 5;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 20);
}

TEST_CASE("Interpreter: integer division", "[interpreter]") {
    auto val = interpret("10 / 3;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 3);
}

TEST_CASE("Interpreter: integer modulo", "[interpreter]") {
    auto val = interpret("10 % 3;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 1);
}

TEST_CASE("Interpreter: integer exponent", "[interpreter]") {
    auto val = interpret("2 ^ 10;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 1024);
}

// --- Float arithmetic ---

TEST_CASE("Interpreter: float addition", "[interpreter]") {
    auto val = interpret("1.5 + 2.5;");
    REQUIRE(val.isFloat());
    CHECK(val.asFloat() == Catch::Approx(4.0));
}

TEST_CASE("Interpreter: float multiplication", "[interpreter]") {
    auto val = interpret("3.0 * 2.0;");
    REQUIRE(val.isFloat());
    CHECK(val.asFloat() == Catch::Approx(6.0));
}

// --- Mixed int/float promotion ---

TEST_CASE("Interpreter: mixed addition promotes to float", "[interpreter]") {
    auto val = interpret("1 + 2.5;");
    REQUIRE(val.isFloat());
    CHECK(val.asFloat() == Catch::Approx(3.5));
}

TEST_CASE("Interpreter: mixed division promotes to float", "[interpreter]") {
    auto val = interpret("10 / 3.0;");
    REQUIRE(val.isFloat());
    CHECK(val.asFloat() == Catch::Approx(3.333333).epsilon(0.001));
}

// --- Unary minus ---

TEST_CASE("Interpreter: unary minus integer", "[interpreter]") {
    auto val = interpret("-5;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == -5);
}

TEST_CASE("Interpreter: unary minus float", "[interpreter]") {
    auto val = interpret("-3.14;");
    REQUIRE(val.isFloat());
    CHECK(val.asFloat() == Catch::Approx(-3.14));
}

// --- Compound expressions with precedence ---

TEST_CASE("Interpreter: compound expression (2-4)*(1+3*1*2)", "[interpreter]") {
    auto val = interpret("(2 - 4) * (1 + 3 * 1 * 2);");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == -14);
}

TEST_CASE("Interpreter: compound float expression 3.1*2+3", "[interpreter]") {
    auto val = interpret("3.1 * 2 + 3;");
    REQUIRE(val.isFloat());
    CHECK(val.asFloat() == Catch::Approx(9.2));
}

// --- Comparison operators ---

TEST_CASE("Interpreter: greater than", "[interpreter]") {
    auto val = interpret("5 > 3;");
    REQUIRE(val.isBool());
    CHECK(val.asBool() == true);
}

TEST_CASE("Interpreter: greater or equal", "[interpreter]") {
    auto val = interpret("3 >= 3;");
    REQUIRE(val.isBool());
    CHECK(val.asBool() == true);
}

TEST_CASE("Interpreter: equality", "[interpreter]") {
    auto val = interpret("1 == 1;");
    REQUIRE(val.isBool());
    CHECK(val.asBool() == true);
}

TEST_CASE("Interpreter: inequality", "[interpreter]") {
    auto val = interpret("1 != 2;");
    REQUIRE(val.isBool());
    CHECK(val.asBool() == true);
}

TEST_CASE("Interpreter: less than false", "[interpreter]") {
    auto val = interpret("2 < 1;");
    REQUIRE(val.isBool());
    CHECK(val.asBool() == false);
}

// --- Logical operators ---

TEST_CASE("Interpreter: and truthy", "[interpreter]") {
    auto val = interpret("1 and 2;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 2);
}

TEST_CASE("Interpreter: and falsy short-circuit", "[interpreter]") {
    auto val = interpret("0 and 2;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 0);
}

TEST_CASE("Interpreter: or falsy", "[interpreter]") {
    auto val = interpret("0 or 5;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 5);
}

TEST_CASE("Interpreter: or truthy short-circuit", "[interpreter]") {
    auto val = interpret("1 or 5;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 1);
}

// --- Let binding ---

TEST_CASE("Interpreter: let binding evaluates to value", "[interpreter]") {
    auto val = interpret("let x = 42;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 42);
}

TEST_CASE("Interpreter: block-local let does not alter outer variable", "[interpreter]") {
    // let x = 10 in outer scope, let x = 20 in inner block, then read x in outer scope
    auto val = interpret("let x = 10; { let x = 20; } x;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 10);
}

// --- IdentExpr (variable reference) ---

TEST_CASE("Interpreter: IdentExpr resolves let-bound variable", "[interpreter]") {
    auto val = interpret("let x = 42; x;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 42);
}

TEST_CASE("Interpreter: IdentExpr resolves variable after arithmetic", "[interpreter]") {
    auto val = interpret("let a = 3; let b = 7; a + b;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 10);
}

TEST_CASE("Interpreter: IdentExpr undefined variable throws", "[interpreter]") {
    REQUIRE_THROWS_AS(interpret("y;"), std::runtime_error);
}

// --- Error cases ---

TEST_CASE("Interpreter: division by zero throws", "[interpreter]") {
    REQUIRE_THROWS_AS(interpret("1 / 0;"), std::runtime_error);
}

TEST_CASE("Interpreter: modulo by zero throws", "[interpreter]") {
    REQUIRE_THROWS_AS(interpret("1 % 0;"), std::runtime_error);
}

// --- String literals and operations ---

TEST_CASE("Interpreter: string literal creates string value", "[interpreter]") {
    auto val = interpret("\"hello\";");
    REQUIRE(val.isReference());
    auto obj = val.deref();
    REQUIRE(val.typeId() == eval::TYPE_STRING);
    REQUIRE(obj != nullptr);
}

TEST_CASE("Interpreter: string toString returns content", "[interpreter]") {
    auto val = interpret("\"hello world\";");
    REQUIRE(val.toString() == "hello world");
}

TEST_CASE("Interpreter: string equality - same content", "[interpreter]") {
    auto val = interpret("\"hello\" == \"hello\";");
    REQUIRE(val.isBool());
    CHECK(val.asBool() == true);
}

TEST_CASE("Interpreter: string inequality - different content", "[interpreter]") {
    auto val = interpret("\"hello\" != \"world\";");
    REQUIRE(val.isBool());
    CHECK(val.asBool() == true);
}

TEST_CASE("Interpreter: string equality - false for different strings", "[interpreter]") {
    auto val = interpret("\"hello\" == \"world\";");
    REQUIRE(val.isBool());
    CHECK(val.asBool() == false);
}

TEST_CASE("Interpreter: string inequality - false for same content", "[interpreter]") {
    auto val = interpret("\"hello\" != \"hello\";");
    REQUIRE(val.isBool());
    CHECK(val.asBool() == false);
}

TEST_CASE("Interpreter: string let binding", "[interpreter]") {
    auto val = interpret("let s = \"hello\"; s;");
    REQUIRE(val.isReference());
    REQUIRE(val.toString() == "hello");
}

TEST_CASE("Interpreter: string variable equality", "[interpreter]") {
    auto val = interpret("let s1 = \"hello\"; let s2 = \"hello\"; s1 == s2;");
    REQUIRE(val.isBool());
    CHECK(val.asBool() == true);
}

TEST_CASE("Interpreter: string with escape sequences", "[interpreter]") {
    auto val = interpret("\"hello\\nworld\";");
    REQUIRE(val.toString() == "hello\nworld");
}

TEST_CASE("Interpreter: empty string", "[interpreter]") {
    auto val = interpret("\"\";");
    REQUIRE(val.isReference());
    REQUIRE(val.toString() == "");
}

TEST_CASE("Interpreter: string with special characters", "[interpreter]") {
    auto val = interpret("\"!@#$%^&*()\";");
    REQUIRE(val.toString() == "!@#$%^&*()");
}

TEST_CASE("Interpreter: string truthiness - non-empty is truthy", "[interpreter]") {
    auto val = interpret("\"hello\" and 42;");
    REQUIRE(val.isInt());
    CHECK(val.asInt() == 42);
}

TEST_CASE("Interpreter: block-local string does not alter outer variable", "[interpreter]") {
    auto val = interpret("let s = \"outer\"; { let s = \"inner\"; } s;");
    REQUIRE(val.isReference());
    REQUIRE(val.toString() == "outer");
}

// --- Arrays ---

TEST_CASE("Interpreter: array literal creates array object", "[interpreter]") {
    auto val = interpret("[1, 2, 3];");
    REQUIRE(val.isReference());

    auto array = val.as<eval::ArrayObject>();
    REQUIRE(array != nullptr);
    REQUIRE(array->size() == 3);
    CHECK(array->elements()[0].asInt() == 1);
    CHECK(array->elements()[1].asInt() == 2);
    CHECK(array->elements()[2].asInt() == 3);
}

TEST_CASE("Interpreter: array type id is determined by element type and length", "[interpreter]") {
    auto a = interpret("[1, 2, 3];");
    auto b = interpret("[4, 5, 6];");
    auto c = interpret("[1, 2];");
    auto d = interpret("[1.0, 2.0, 3.0];");

    CHECK(a.typeId() == b.typeId());
    CHECK(a.typeId() != c.typeId());
    CHECK(a.typeId() != d.typeId());

    const eval::TypeId expected = eval::TypeTable::instance().getArrayTypeId(eval::TYPE_INT, 3);
    CHECK(a.typeId() == expected);
}

TEST_CASE("Interpreter: empty array has deterministic unknown-element type", "[interpreter]") {
    auto val = interpret("[];");
    REQUIRE(val.isReference());

    const eval::TypeId expected = eval::TypeTable::instance().getArrayTypeId(eval::TYPE_UNKNOWN, 0);
    CHECK(val.typeId() == expected);
}

TEST_CASE("Interpreter: mixed-type array throws", "[interpreter]") {
    REQUIRE_THROWS_AS(interpret("[1, 2.0];"), std::runtime_error);
}
