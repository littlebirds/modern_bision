#include "passes/mem2reg_pass.hpp"
#include <stdexcept>

namespace eval {
namespace {

void promoteFunction(llvm::Function& function) {
    // TODO: Identify eligible entry-block allocas whose uses permit promotion.
    // TODO: Replace their loads/stores with SSA values and insert phi nodes
    // at control-flow merges (including loop headers), then remove the allocas.
    // LLVM's promotion utilities can supply this transformation; no custom
    // SSA construction or optimization is implemented by this scaffold.
    (void)function;
}

} // namespace

void mem2reg(Compiler& c) {
    if (!c.module)
        throw std::runtime_error("mem2reg requires LLVM IR; run llvmIRGen first");

    for (auto& function : *c.module) {
        if (function.isDeclaration()) continue;
        promoteFunction(function);
    }
}

} // namespace eval
