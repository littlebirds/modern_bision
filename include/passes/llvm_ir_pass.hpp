#pragma once

#include "compiler.hpp"

namespace eval {

// Pass: AST → LLVM IR lowering  (prototype backend).
// Reads  Compiler::ast + node annotations (SemanticInfo).
// Writes Compiler::llvmCtx, Compiler::module, Compiler::builder.
void llvmIRGen(Compiler& c);

} // namespace eval
