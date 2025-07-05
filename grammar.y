%{
#include <iostream>
#include <string>
#include <cmath>
#include <FlexLexer.h>
%}
 
%require "3.7.4"
%language "C++"
%header
%locations
%define api.parser.class {Parser}
%define api.namespace {monkey}
%define api.value.type variant
%define parse.assert
%parse-param {Scanner* scanner} {std::unique_ptr<ast::Node>& ppRoot}
 
%code requires
{
    #include "ast.hpp"

    namespace monkey {
        class Scanner;
    } // namespace monkey
} // %code requires
 
%code
{
    #include "Scanner.hpp"
    #define yylex(x, loc) scanner->lex(x, loc)
}
 
%token                          EOL LPAREN RPAREN LBRACKET RBRACKET LBRACE RBRACE COLON SEMICOLON COMMA DOT
%token                          LET FUNCTION FOR RETURN IF ELSE ELIF
%token                          TRUE FALSE
%token <std::string>            LIT_INT LIT_FLOAT LIT_STR
%token <const char*>            AND OR GT LT GE LE EQ NOT_EQ PLUS MINUS DIVIDE MULTIPLY MODULO EXPONENT
%token <std::string>            Ident
 
%nterm <const char*>                             binop
%nterm <std::unique_ptr<ast::Expr>>              expr
%nterm <std::unique_ptr<ast::Stmt>>              stmt
%nterm <std::unique_ptr<ast::StmtList>>          stmt_list
%nterm                                           program

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

%start program

%%
program : stmt_list                           { this->ppRoot =std::move($1); }
        ;

stmt_list : %empty                            { $$ = std::make_unique<ast::StmtList>(); }
        | stmt_list stmt                      { if ($2) { $1->append(std::move($2)); }; $$ = std::move($1); }
        ;
 
stmt    : EOL                               { ; }
        | expr SEMICOLON                    { $$ = std::make_unique<ast::ExprStmt>(std::move($1)); @$ = @1; }
        | LET Ident ASSIGN expr SEMICOLON   { $$ = std::make_unique<ast::LetStmt>($2, std::move($4)); } 
        | error EOL                         { yyerrok; }
        ; 

binop   : AND | OR | GT | LT | GE | LE | EQ | NOT_EQ | PLUS | MINUS | DIVIDE | MULTIPLY | MODULO | EXPONENT
                                            { $$ = $1; }
        ;
expr    : LIT_INT                           { $$ = std::make_unique<ast::IntLitExpr>($1); @$ = @1; }
        | LIT_FLOAT                         { $$ = std::make_unique<ast::FloatLitExpr>($1); @$ = @1; }
        | expr binop expr                   { $$ = std::make_unique<ast::BinOpExpr>(std::move($1), std::move($3), $2); }    
//        | iexp MODULO iexp                { $$ = $1 % $3; }
//        | MINUS iexp %prec UMINUS         { $$ = -$2; }
//        | iexp FACTORIAL                  { $$ = factorial($1); }
        | LPAREN expr RPAREN                { $$ = std::move($2); }
        ;
 
%%
 
void monkey::Parser::error(const location_type& loc, const std::string& msg) {
    std::cerr << "[" << loc << "]:" << msg << '\n';
}