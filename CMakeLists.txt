cmake_minimum_required(VERSION 3.25)

project(Monkey LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# --- LLVM ---
# On macOS with Homebrew, LLVM is keg-only; add its prefix to the search path.
if(APPLE AND EXISTS "/opt/homebrew/opt/llvm")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/llvm")
endif()
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
llvm_map_components_to_libnames(LLVM_LIBS core support native)

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

add_executable(monkey
    src/main.cpp src/ast.cpp src/pretty_printer.cpp
    src/interpreter.cpp
    src/passes/semantic_pass.cpp
    src/passes/llvm_ir_pass.cpp
    ${BISON_MonkeyParser_OUTPUTS} ${FLEX_MonkeyScanner_OUTPUTS})

target_include_directories(monkey PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/passes
    ${WIN_FLEX_BISON_INCLUDE}
    ${LLVM_INCLUDE_DIRS})

target_compile_definitions(monkey PRIVATE ${LLVM_DEFINITIONS})

target_link_libraries(monkey PRIVATE ${LLVM_LIBS})

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
