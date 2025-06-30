%{
#include <iostream>
#include <string>
#include <cmath>
#include <FlexLexer.h>
%}
 
%require "3.7.4"
%language "C++"
// conflicts with CMake findBison
// %defines "Parser.hpp"
// %output "Parser.cpp"
 
%define api.parser.class {Parser}
%define api.namespace {monkey}
%define api.value.type variant
%define parse.assert
%parse-param {Scanner* scanner}
 
%code requires
{
    #include "AST.hpp"

    namespace monkey {
        class Scanner;
    } // namespace monkey
} // %code requires
 
%code
{
    #include "Scanner.hpp"
    #define yylex(x) scanner->lex(x)
}
 
%token                          EOL LPAREN RPAREN LBRACKET RBRACKET LBRACE RBRACE COLON COMMA DOT
%token                          LET FUNCTION FOR RETURN IF ELSE ELIF
%token                          TRUE FALSE
%token <long long>              INTEGER
%token <double>                 FLOAT
%token <std::string>            STRING
%token <std::string>            Ident
 
%nterm <std::unique_ptr<ast::Expr>>              expr
%nterm <std::unique_ptr<ast::Stmt>>              stmt
%nterm <std::unique_ptr<ast::Program>>           program
unique_ptr
%nonassoc             ASSIGN
%left                 OR
%left                 AND
%nonassoc             NOT
%nonassoc             GT LT GE LE EQ NOT_EQ
%left                 PLUS MINUS
%left                 MULTIPLY DIVIDE MODULO
%precedence           UMINUS
%precedence           FACTORIAL
%right                EXPONENT
 
%code
{
    namespace monkey {
 
        long long factorial(long long n) {
            if (n < 2) {
                return 1;
            }
            return n * factorial(n - 1);
        }
    } // namespace monkey
} // %code
 
%%
 
program : %empty                    { printf($$->str().c_str()); }
        | program stmt              { $1->appendStmt(std::move($2)); $$ = std::move($1); }
        ;
 
stmt    : EOL                       { ; }
        | expr EOL                  { $$ = std::make_unique<ast::ExprStmt>(std::move($1)); }
    //    | error EOL                 { yyerrok; }
        ;
 
expr    : INTEGER                   { $$ = std::make_unique<ast::IntLitExpr>($1); }
        | FLOAT                     { $$ = std::make_unique<ast::FloatLitExpr>($1); }
        | expr MINUS expr           { $$ = std::make_unique<ast::BinOpExpr>(std::move($1), std::move($3), "-"); }
        | expr PLUS expr            { $$ = std::make_unique<ast::BinOpExpr>(std::move($1), std::move($3), "+"); }
        | expr MULTIPLY expr        { $$ = std::make_unique<ast::BinOpExpr>(std::move($1), std::move($3), "*"); }
        | expr DIVIDE expr          { $$ = std::make_unique<ast::BinOpExpr>(std::move($1), std::move($3), "/"); }
//        | iexp MODULO iexp          { $$ = $1 % $3; }
//        | MINUS iexp %prec UMINUS   { $$ = -$2; }
//        | iexp FACTORIAL            { $$ = factorial($1); }
        | LPAREN expr RPAREN        { $$ = std::move($2); }
        ;
 
%%
 
void monkey::Parser::error(const std::string& msg) {
    std::cerr << msg << '\n';
}