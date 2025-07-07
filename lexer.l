%{
#include "Parser.hpp"
#include "Scanner.hpp"
#include "location.hh"

#undef YY_DECL
#define YY_DECL int monkey::Scanner::lex(monkey::Parser::semantic_type *yylval, monkey::location* loc)

#define YY_USER_ACTION      myloc.step(); myloc.columns(YYLeng()); *loc = myloc; 
#define FIX_MY_LINES        myloc.lines(YYLeng()); *loc = myloc


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

%x              STR

%%

\"               { this->builder_ptr = this->builder_buf; startStr(); BEGIN(STR); }
<STR>\"          { BEGIN(INITIAL); endStr(); *this->builder_ptr = '\0'; yylval->emplace<std::string>(this->builder_buf); *loc = this->strLocation(); return Parser::token::LIT_STR; }
<STR>\n          {;}
<STR>\\n         *builder_ptr++ = '\n';
<STR>\\t         *builder_ptr++ = '\t';
<STR>\\r         *builder_ptr++ = '\r';
<STR>\\b         *builder_ptr++ = '\b';
<STR>\\f         *builder_ptr++ = '\f';
<STR>\\(.|\n)    *builder_ptr++ = yytext[1];
<STR>[^\\\n\"]+  {
                    const char *yptr = YYText();
                    while (*yptr) {
                        *this->builder_ptr++ = *yptr++;
                    }
                }
 
{integer}       { yylval->emplace<std::string>(YYText()); return Parser::token::LIT_INT; }
{float}         { yylval->emplace<std::string>(YYText()); return Parser::token::LIT_FLOAT; }
"let"           { return Parser::token::LET; }
"fn"            { return Parser::token::FUNCTION; }
"for"           { return Parser::token::FOR; }
"return"        { return Parser::token::RETURN; }
"if"            { return Parser::token::IF; }
"else"          { return Parser::token::ELSE; }
"elif"          { return Parser::token::ELIF; }
"true"          { return Parser::token::TRUE; }
"false"         { return Parser::token::FALSE; }
"and"           { return Parser::token::AND; }
"or"            { return Parser::token::OR; }
"not"           { return Parser::token::NOT; }
"<="            { return Parser::token::LE; }
">="            { return Parser::token::GE; }
"=="            { return Parser::token::EQ; }
"!="            { return Parser::token::NOT_EQ;}
"<"             { return Parser::token::LT; }
">"             { return Parser::token::GT; }
"+"             { return Parser::token::PLUS; }
"-"             { return Parser::token::MINUS; }
"*"             { return Parser::token::MULTIPLY; }
"/"             { return Parser::token::DIVIDE; }
"%"             { return Parser::token::MODULO; }
"!"             { return Parser::token::FACTORIAL; }
"^"             { return Parser::token::EXPONENT; }
"("             { return Parser::token::LPAREN; }
")"             { return Parser::token::RPAREN; } 
"["             { return Parser::token::LBRACKET; }
"]"             { return Parser::token::RBRACKET; }
"{"             { ++this->indent_lvl; yylval->emplace<int>(indent_lvl); return Parser::token::LBRACE; }
"}"             { --this->indent_lvl; return Parser::token::RBRACE; }
":"             { return Parser::token::COLON; }
";"             { return Parser::token::SEMICOLON; }
","             { return Parser::token::COMMA; }
"="             { return Parser::token::ASSIGN; }
"."             { return Parser::token::DOT; }
\n+             { FIX_MY_LINES; return Parser::token::EOL; }
\/\/[^\n]*      { /* skip single line comment */ ; }
{identifier}    { yylval->emplace<std::string>(YYText()); return Parser::token::Ident; }
<<EOF>>         return Parser::token::YYEOF;
.               /* no action on unmatched input */
 
%%
 
int yyFlexLexer::yylex() {
    throw std::runtime_error("Bad call to yyFlexLexer::yylex()");
}