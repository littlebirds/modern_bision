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
 
%define parse.trace true  // Enables runtime parsing traces
%verbose

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

    void print_loc(const char * prefix, const monkey::location& loc) {
        std::cout << prefix << " " <<loc << std::endl;
    }
}
 
%token <int>                    LBRACE 
%token                          EOL LPAREN RPAREN LBRACKET RBRACKET RBRACE COLON SEMICOLON COMMA DOT
%token                          LET FUNCTION FOR RETURN IF ELSE ELIF
%token                          TRUE FALSE
%token <std::string>            LIT_INT LIT_FLOAT LIT_STR 
%token <std::string>            Ident

%nterm <std::unique_ptr<ast::Expr>>                             expr
%nterm <std::unique_ptr<ast::ExprSeq>>                          expr_seq
%nterm <std::unique_ptr<ast::Stmt>>                             stmt
%nterm <std::unique_ptr<ast::StmtList>>                         stmt_list
%nterm <std::unique_ptr<ast::BlockStmt>>                        block_stmt
%nterm <std::unique_ptr<ast::ElifList>>                         elif_list
%nterm <std::optional<std::unique_ptr<ast::BlockStmt >>>        opt_else
%nterm <std::unique_ptr<ast::IfStmt>>                           if_stmt
%nterm                                                          program

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
program : stmt_list                                 { this->ppRoot =std::move($1); }
        ;       

stmt_list : %empty                                  { $$ = std::make_unique<ast::StmtList>(); }
        | stmt_list stmt                            { if ($2) { $1->append($2); }; $$ = std::move($1); }
        ; 

block_stmt :  LBRACE stmt_list RBRACE               { $$ = std::make_unique<ast::BlockStmt>(@$, std::move($2), $1); }
        ;       
if_stmt: IF expr block_stmt elif_list opt_else      { $$ = std::make_unique<ast::IfStmt>(@$, std::move($2), std::move($3), std::move($4), std::move($5)); }
        ;

elif_list: %empty                                   { $$ = std::make_unique<ast::ElifList>(); }
        | elif_list ELIF expr block_stmt            { $1->append(std::move($3), std::move($4)); $$ = std::move($1); }
        ;
opt_else: %empty                                    { $$ = std::nullopt; }
        | ELSE block_stmt                           { $$ = std::make_optional(std::move($2)); }
        ;

stmt    : EOL                                       { ; }
        | expr SEMICOLON                            { $$ = std::make_unique<ast::ExprStmt>(@$, std::move($1)); }        
        | block_stmt                                { $$ = std::move($1); }
        | if_stmt                                   { $$ = std::move($1); }
        | error EOL                                 { yyerrok; }
        ;       

expr_seq : %empty                                   { $$ = std::make_unique<ast::ExprSeq>(); }    
        | expr                                      { $$ = std::make_unique<ast::ExprSeq>(); $$->append($1); }      
        | expr_seq COMMA expr                       { $1->append($3); $$ = std::move($1); }
        ;
expr    : LIT_INT                                   { $$ = std::make_unique<ast::IntLitExpr>(@$, $1);}
        | LIT_FLOAT                                 { $$ = std::make_unique<ast::FloatLitExpr>(@$, $1); }
        | LIT_STR                                   { $$ = std::make_unique<ast::StringLitExpr>(@$, $1); }
        | MINUS expr %prec UMINUS                   { $$ = std::make_unique<ast::UnaryExpr>(@$, std::move($2), "-"); }
        | LBRACKET expr_seq RBRACKET                { $$ = std::make_unique<ast::ArrayExpr>(@$, $2);}    
        | expr AND expr                             { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3), "and"); }
        | expr OR expr                              { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3), "or"); }   
        | expr GT expr                              { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3),  ">"); }
        | expr LT expr                              { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3), "<"); }
        | expr GE expr                              { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3), ">="); }
        | expr LE expr                              { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3), "<="); }
        | expr EQ expr                              { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3), "=="); }
        | expr NOT_EQ expr                          { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3), "!="); }
        | expr PLUS expr                            { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3),"+"); } 
        | expr MINUS expr                           { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3), "-"); } 
        | expr DIVIDE expr                          { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3), "/"); }
        | expr MULTIPLY expr                        { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3), "*"); }
        | expr MODULO expr                          { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3), "%"); }
        | expr EXPONENT expr                        { $$ = std::make_unique<ast::BinOpExpr>(@$, std::move($1), std::move($3), "^"); }
        | LET Ident ASSIGN expr                     { $$ = std::make_unique<ast::LetExpr>(@$, $2, std::move($4)); }  
        | LPAREN expr RPAREN                        { $$ = std::move($2); }
        ;
 
%%
 
void monkey::Parser::error(const location_type& loc, const std::string& msg) {
    std::cerr << "[" << loc << "]:" << msg << '\n';
}