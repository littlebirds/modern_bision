%{
#include "Parser.hpp"
#include "Scanner.hpp"

#undef YY_DECL
#define YY_DECL int monkey::Scanner::lex(monkey::Parser::semantic_type *yylval)
%}
 
%option c++ interactive noyywrap noyylineno nodefault
 
dseq            ([[:digit:]]+)
dseq_opt        ({dseq}?)
frac            (({dseq_opt}"."{dseq})|{dseq}".")
exp             ([eE][+-]?{dseq})
exp_opt         ({exp}?)
 
integer         ({dseq})
float           (({frac}{exp_opt})|({dseq}{exp}))
identifier      ([:alpha:][[:alnum:]_]*)

%%
 
{integer}       { yylval->emplace<std::string>(YYText()); return Parser::token::LIT_INT; }
{float}         { yylval->emplace<std::string>(YYText()); return Parser::token::LIT_FLOAT; }
"let"           return Parser::token::LET;
"fn"            return Parser::token::FUNCTION;
"for"           return Parser::token::FOR;
"return"        return Parser::token::RETURN;
"if"            return Parser::token::IF;
"else"          return Parser::token::ELSE;
"elif"          return Parser::token::ELIF;
"true"          return Parser::token::TRUE;
"false"         return Parser::token::FALSE;
"and"           return Parser::token::AND;
"or"            return Parser::token::OR;
"not"           return Parser::token::NOT;
"<="            return Parser::token::LE;
">="            return Parser::token::GE;
"=="            return Parser::token::EQ;
"!="            return Parser::token::NOT_EQ;
"<"             return Parser::token::LT;
">"             return Parser::token::GT;
"+"             return Parser::token::PLUS;
"-"             return Parser::token::MINUS;
"*"             return Parser::token::MULTIPLY;
"/"             return Parser::token::DIVIDE;
"%"             return Parser::token::MODULO;
"!"             return Parser::token::FACTORIAL;
"^"             return Parser::token::EXPONENT;
"("             return Parser::token::LPAREN;
")"             return Parser::token::RPAREN;
"["             return Parser::token::LBRACKET;
"]"             return Parser::token::RBRACKET;
"{"             return Parser::token::LBRACE;
"}"             return Parser::token::RBRACE;
":"             return Parser::token::COLON;
";"             return Parser::token::SEMICOLON;
","             return Parser::token::COMMA;
"="             return Parser::token::ASSIGN;
"."             return Parser::token::DOT;
\n              return Parser::token::EOL;
\/\/[^\n]*     { /* skip single line comment */ return Parser::token::EOL; }
{identifier}    { yylval->emplace<std::string>(YYText()); return Parser::token::Ident; }
<<EOF>>         return Parser::token::YYEOF;
.               /* no action on unmatched input */
 
%%
 
int yyFlexLexer::yylex() {
    throw std::runtime_error("Bad call to yyFlexLexer::yylex()");
}