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
var          ([:alpha:][[:alnum:]_]*)
 
%%
 
{integer}       { yylval->emplace<long long>(strtoll(YYText(), nullptr, 10)); return Parser::token::INTEGER; }
{float}         { yylval->emplace<double>(strtod(YYText(), nullptr)); return Parser::token::FLOAT; }
"let"           return Parser::token::LET;
"fn"            return Parser::token::FUNCTION;
"for"           return Parser::token::FOR;
"return"        return Parser::token::RETURN;
"if"            return Parser::token::IF;
"else"          return Parser::token::ELSE;
"elif"          return Parser::token::ELIF;
"true"          return Parser::token::TRUE;
"false"         return Parser::token::FALSE;
{intvar}        { yylval->emplace<std::string>(YYText()); return Parser::token::VAR; }
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
","             return Parser::token::COMMA;
"="             return Parser::token::ASSIGN;
\n              return Parser::token::EOL;
<<EOF>>         return Parser::token::YYEOF;
.               /* no action on unmatched input */
 
%%
 
int yyFlexLexer::yylex() {
    throw std::runtime_error("Bad call to yyFlexLexer::yylex()");
}