cmake_minimum_required(VERSION 3.25)

project(Monkey LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Windows: Find win_flex_bison
if(WIN32)
    set(WIN_FLEX_BISON_PATH "$ENV{USERPROFILE}/bin/win_flex_bison-2.5.25")
    if(EXISTS "${WIN_FLEX_BISON_PATH}/win_flex.exe" AND EXISTS "${WIN_FLEX_BISON_PATH}/FlexLexer.h")
        set(FLEX_EXECUTABLE "${WIN_FLEX_BISON_PATH}/win_flex.exe" CACHE FILEPATH "Flex executable" FORCE)
        set(BISON_EXECUTABLE "${WIN_FLEX_BISON_PATH}/win_bison.exe" CACHE FILEPATH "Bison executable" FORCE)
        set(WIN_FLEX_BISON_INCLUDE "${WIN_FLEX_BISON_PATH}")
        message(STATUS "Found win_flex_bison at: ${WIN_FLEX_BISON_PATH}")
    endif()
endif()

find_package(BISON)
find_package(FLEX)
bison_target(MonkeyParser grammar.y Parser.cpp DEFINES_FILE Parser.hpp)
flex_target(MonkeyScanner lexer.l Scanner.cpp)
add_flex_bison_dependency(MonkeyScanner MonkeyParser)

add_executable(monkey src/main.cpp src/ast.cpp ${BISON_MonkeyParser_OUTPUTS} ${FLEX_MonkeyScanner_OUTPUTS})

target_include_directories(monkey PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/include ${WIN_FLEX_BISON_INCLUDE})

# Catch2 tests
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)

enable_testing()
add_subdirectory(tests)
