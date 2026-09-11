#pragma once

#include "compiler.hpp"

namespace eval {

// LLVM IR memory-to-register promotion scaffold. Run after llvmIRGen.
// Requires Compiler::module; throws if IR has not been generated.
// Currently leaves the module unchanged: no allocas are promoted.
void mem2reg(Compiler& c);

} // namespace eval
