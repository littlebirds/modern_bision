# Monkey Programming Language Compiler in C++ /Bison/ Flex

## Goal
This repo is for setting up a starting project for compiler development. It leverages Flex/Bison to generate lexer/parser in C++ for 
the monkey programming language, enabling agile testing out new feature ideas.  

## Reference
[Monkey Programming Language](https://monkeylang.org/)

[Generating C++ programs with flex and bison](https://learnmoderncpp.com/2020/12/18/generating-c-programs-with-flex-and-bison-3/)

[Bison 3.8 Manual](https://www.gnu.org/software/bison/manual/bison.html)

## Build & Test

### Linux / WSL
```bash
sudo apt install flex bison cmake g++
cmake -DCMAKE_BUILD_TYPE=Debug -S. -Bbuild
cmake --build build
build/parser
```

### Windows
1. Install [current win_flex_bison](https://github.com/lexxmark/winflexbison) (or place in `C:\Users\<user>\bin\win_flex_bison-2.5.25`)
2. Install [Visual Studio](https://visualstudio.microsoft.com/) with "Desktop development with C++" workload
3. Open **x64 Native Tools Command Prompt for VS 2022** (not regular PowerShell)
4. Run:
```cmd
cmake -DCMAKE_BUILD_TYPE=Debug -S. -Bbuild
cmake --build build
build\Debug\parser.exe
```

Or use MinGW:
```cmd
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -S. -Bbuild
cmake --build build
build\parser.exe
```
