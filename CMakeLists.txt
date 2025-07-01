cmake_minimum_required(VERSION 3.20) # Or a newer version like 3.28 for better C++23 module support

project(Monkey LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(BISON)
find_package(FLEX)
bison_target(MonkeyParser grammar.y Parser.cpp DEFINES_FILE Parser.hpp)
flex_target(MonkeyScanner lexer.l Scanner.cpp)
add_flex_bison_dependency(MonkeyScanner MonkeyParser)

add_executable(monkey main.cpp ast.cpp ${BISON_MonkeyParser_OUTPUTS} ${FLEX_MonkeyScanner_OUTPUTS})
target_include_directories(monkey PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})