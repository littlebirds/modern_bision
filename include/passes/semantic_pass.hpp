#pragma once

#include "compiler.hpp"

namespace eval {

// Pass: scope analysis + type checking.
// Reads Compiler::ast, annotates AST nodes with SemanticInfo.
void semanticAnalysis(Compiler& c);

} // namespace eval
