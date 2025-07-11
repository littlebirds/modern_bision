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
        : yyFlexLexer(arg_yyin, arg_yyout), indent_lvl(0) {
            myloc.begin.filename = &file_name;
        }

    Scanner(std::istream* arg_yyin = nullptr, std::ostream* arg_yyout = nullptr)
        : yyFlexLexer(arg_yyin, arg_yyout), indent_lvl(0) {}

    void setFileName(std::string fname) { file_name = std::move(fname); }
    
    void startStr() { strBegin = myloc.begin; }
    void endStr()   { strEnd = myloc.end; }
    monkey::location strLocation() const  { return monkey::location(strBegin, strEnd); }
        
    int lex(Parser::semantic_type *yylval, location* loc); // note: this is the prototype we need



private:
    int indent_lvl;
    std::string file_name;
    monkey::location myloc;
    char builder_buf[4096];
    char * builder_ptr;
    monkey::position strBegin;
    monkey::position strEnd;
};
 
} // namespace monkey