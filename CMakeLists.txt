cmake_minimum_required(VERSION 3.20) # Or a newer version like 3.28 for better C++23 module support

project(Monkey LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(BISON)
find_package(FLEX)
bison_target(MonkeyParser grammar.y)
flex_target(MonkeyScanner lexer.l)
add_flex_bison_dependency(MonkeyScanner MonkeyParser)

add_executable(parser main.cpp ${BISON_MonkeyParser_OUTPUTS} ${FLEX_MonkeyScanner_OUTPUTS})