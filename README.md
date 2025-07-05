# Monkey Programming Language Compiler in C++ /Bison/ Flex

## Goal
This repo is for setting up a starting project for compiler development. It leverages Flex/Bison to generate lexer/parser in C++ for 
the monkey programming language, enabling agile testing out new feature ideas.  

## Reference
[Monkey Programming Language](https://monkeylang.org/)

[Generating C++ programs with flex and bison](https://learnmoderncpp.com/2020/12/18/generating-c-programs-with-flex-and-bison-3/)

[Bison 3.8 Manual](https://www.gnu.org/software/bison/manual/bison.html)

## Build & Test
sudo apt install flex bison cmake
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -S. -Bbuild
cmake --build build
build/parser 
```
