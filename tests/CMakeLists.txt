# Test executables
add_executable(test_runner
    test_parser.cpp
    test_interpreter.cpp
    ${CMAKE_SOURCE_DIR}/src/ast.cpp
    ${CMAKE_SOURCE_DIR}/src/pretty_printer.cpp
    ${CMAKE_SOURCE_DIR}/src/interpreter.cpp
    ${CMAKE_BINARY_DIR}/Parser.cpp
    ${CMAKE_BINARY_DIR}/Scanner.cpp
)

target_include_directories(test_runner PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${WIN_FLEX_BISON_INCLUDE}
    ${CMAKE_BINARY_DIR}/_deps/catch2-src/src/catch2
    ${CMAKE_BINARY_DIR}/_deps/catch2-build/generated-includes
)

target_link_libraries(test_runner PRIVATE Catch2::Catch2)
add_test(NAME test_runner COMMAND test_runner)
