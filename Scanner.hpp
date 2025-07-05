#include <istream>
#include <ostream>
#include <string>
#include "Parser.hpp"
#include "location.hh"
#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif


namespace monkey { // note: depends upon FlexLexer.h and Parser2.hpp
 
class Scanner : public yyFlexLexer { 

public:
    Scanner(std::istream& arg_yyin, std::ostream& arg_yyout)
        : yyFlexLexer(arg_yyin, arg_yyout) {
            file_name = "test.txt";
            myloc.begin.filename = &file_name;
        }

    Scanner(std::istream* arg_yyin = nullptr, std::ostream* arg_yyout = nullptr)
        : yyFlexLexer(arg_yyin, arg_yyout) {}
        
    int lex(Parser::semantic_type *yylval, location* loc); // note: this is the prototype we need

private:
    monkey::location myloc;
    std::string file_name;
};
 
} // namespace monkey