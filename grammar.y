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
%token                          LPAREN RPAREN LBRACKET RBRACKET RBRACE COLON SEMICOLON COMMA DOT ASSIGN
%token                          LET FUNCTION FOR WHILE RETURN IF ELSE ELIF
%token                          TRUE FALSE
%token <std::string>            LIT_INT LIT_FLOAT LIT_STR 
%token <std::string>            Ident

%nterm <ast::Expr*>                             expr
%nterm <ast::ExprSeq*>                          expr_seq
%nterm <ast::Stmt*>                             stmt
%nterm <ast::StmtList*>                         stmt_list
%nterm <ast::BlockStmt*>                        block_stmt
%nterm <ast::ElifList*>                         elif_list
%nterm <std::optional<ast::BlockStmt*>>        opt_else
%nterm <ast::IfStmt*>                           if_stmt
%nterm <ast::WhileStmt*>                        while_stmt
%nterm <std::string>                            type_name
%nterm <std::pair<std::string, std::string>>    typed_param
%nterm <std::vector<std::pair<std::string, std::string>>*>  typed_param_list
%nterm <std::optional<std::string>>             opt_return_type
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
%precedence           ARRAY_DEREF CALL

%start program

%%
program : stmt_list                                 { this->ppRoot = std::unique_ptr<ast::Node>($1); }
        ;       

stmt_list : %empty                                  { $$ = new ast::StmtList(); }
        | stmt_list stmt                            { if ($2) { $1->append($2); }; $$ = $1; }
        ; 

block_stmt :  LBRACE stmt_list RBRACE               { $$ = new ast::BlockStmt(@$, $2, $1); }
        ;       
if_stmt: IF expr block_stmt elif_list opt_else      { $$ = new ast::IfStmt(@$, $2, $3, $4, $5); }
        ;

elif_list: %empty                                   { $$ = new ast::ElifList(); }
        | elif_list ELIF expr block_stmt            { $1->append($3, $4); $$ = $1; }
        ;
opt_else: %empty                                    { $$ = std::nullopt; }
        | ELSE block_stmt                           { $$ = std::make_optional($2); }
        ;

type_name : Ident                                  { $$ = $1; }
        ;

typed_param : Ident type_name                       { $$ = std::make_pair($1, $2); }
        ;

typed_param_list : %empty                           { $$ = new std::vector<std::pair<std::string, std::string>>(); }
        | typed_param                               { $$ = new std::vector<std::pair<std::string, std::string>>(); $$->push_back($1); }
        | typed_param_list COMMA typed_param        { $1->push_back($3); $$ = $1; }
        ;

opt_return_type : %empty                            { $$ = std::nullopt; }
        | type_name                                 { $$ = std::make_optional($1); }
        ;

stmt    : expr SEMICOLON                            { $$ = new ast::ExprStmt(@$, $1); }        
        | Ident ASSIGN expr SEMICOLON               { $$ = new ast::ExprStmt(@$, new ast::AssignExpr(@$, $1, $3)); }
        | block_stmt                                { $$ = $1; }
        | if_stmt                                   { $$ = $1; }
        | while_stmt                                { $$ = $1; }
        | RETURN expr SEMICOLON                     { $$ = new ast::ReturnStmt(@$, $2); }
        | error SEMICOLON                           { yyerrok; }
        ;

// TODO: for loop: for Ident in expr block_stmt (requires iterator/range support)
// TODO: break / continue statements inside while loops

while_stmt : WHILE expr block_stmt               { $$ = new ast::WhileStmt(@$, $2, $3); }
           ;

expr_seq : %empty                                   { $$ = new ast::ExprSeq(); }    
        | expr                                      { $$ = new ast::ExprSeq(); $$->append($1); }      
        | expr_seq COMMA expr                       { $1->append($3); $$ = $1; }
        ;
expr    : LIT_INT                                   { $$ = new ast::IntLitExpr(@$, $1);}
        | LIT_FLOAT                                 { $$ = new ast::FloatLitExpr(@$, $1); }
        | LIT_STR                                   { $$ = new ast::StringLitExpr(@$, $1); }
        | TRUE                                      { $$ = new ast::BoolLitExpr(@$, true); }
        | FALSE                                     { $$ = new ast::BoolLitExpr(@$, false); }
        | MINUS expr %prec UMINUS                   { $$ = new ast::UnaryExpr(@$, $2, "-"); }
        | NOT expr %prec NOT                        { $$ = new ast::UnaryExpr(@$, $2, "not"); }
        | LBRACKET expr_seq RBRACKET                { $$ = new ast::ArrayExpr(@$, $2);}
        | expr LBRACKET expr RBRACKET %prec ARRAY_DEREF { $$ = new ast::ArrayDerefExpr(@$, $1, $3); }
        | expr AND expr                             { $$ = new ast::BinOpExpr(@$, $1, $3, "and"); }
        | expr OR expr                              { $$ = new ast::BinOpExpr(@$, $1, $3, "or"); }   
        | expr GT expr                              { $$ = new ast::BinOpExpr(@$, $1, $3,  ">"); }
        | expr LT expr                              { $$ = new ast::BinOpExpr(@$, $1, $3, "<"); }
        | expr GE expr                              { $$ = new ast::BinOpExpr(@$, $1, $3, ">="); }
        | expr LE expr                              { $$ = new ast::BinOpExpr(@$, $1, $3, "<="); }
        | expr EQ expr                              { $$ = new ast::BinOpExpr(@$, $1, $3, "=="); }
        | expr NOT_EQ expr                          { $$ = new ast::BinOpExpr(@$, $1, $3, "!="); }
        | expr PLUS expr                            { $$ = new ast::BinOpExpr(@$, $1, $3,"+"); } 
        | expr MINUS expr                           { $$ = new ast::BinOpExpr(@$, $1, $3, "-"); } 
        | expr DIVIDE expr                          { $$ = new ast::BinOpExpr(@$, $1, $3, "/"); }
        | expr MULTIPLY expr                        { $$ = new ast::BinOpExpr(@$, $1, $3, "*"); }
        | expr MODULO expr                          { $$ = new ast::BinOpExpr(@$, $1, $3, "%"); }
        | expr EXPONENT expr                        { $$ = new ast::BinOpExpr(@$, $1, $3, "^"); }
        | LET Ident ASSIGN expr                     { $$ = new ast::LetExpr(@$, $2, $4); }  
        | Ident                                     { $$ = new ast::IdentExpr(@$, $1); }
        | LPAREN expr RPAREN                        { $$ = $2; }
        | FUNCTION LPAREN typed_param_list RPAREN opt_return_type block_stmt { $$ = new ast::FnLitExpr(@$, $3, $5, $6); }
        | expr LPAREN expr_seq RPAREN %prec CALL    { $$ = new ast::CallExpr(@$, $1, $3); }
        ;
 
%%
 
void monkey::Parser::error(const location_type& loc, const std::string& msg) {
    std::cerr << "[" << loc << "]:" << msg << '\n';
}