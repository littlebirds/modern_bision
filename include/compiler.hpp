#pragma once

#include "ast.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace eval {

// A pass is a named unit of work: reads Compiler state, produces/modifies it.
// Some passes walk the AST (SemanticAnalysis, IR lowering).
// Others walk a linear IR (InstructionSelection, RegisterAllocation).
// The Compiler orchestrates them in order; they communicate through its fields.
struct Pass {
    std::string name;
    std::function<void(class Compiler&)> run;
};

// Compiler IS the pass manager. It owns:
//   - the pipeline of passes
//   - the shared state that passes read and write
//
// Usage:
//   Compiler c;
//   c.ast = &program;
//   c.addPass("semantic",   semanticAnalysis);
//   c.addPass("llvm-ir-gen", llvmIRGen);       // prototype backend
//   // c.addPass("linear-ir", linearIRGen);     // future: your own IR
//   // c.addPass("inst-sel",  instSelection);   // future: your own backend
//   c.run();
//   c.module->print(llvm::outs(), nullptr);

class Compiler {
public:
    // --- Pipeline ---
    void addPass(std::string name, std::function<void(Compiler&)> fn) {
        passes_.push_back({std::move(name), std::move(fn)});
    }

    void run() {
        for (auto& pass : passes_)
            pass.run(*this);
    }

    // --- Shared state (inputs / outputs of passes) ---
    ast::StmtList* ast = nullptr;

    // LLVM-specific state (used by the prototype LLVM IR gen pass)
    std::unique_ptr<llvm::LLVMContext> llvmCtx;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    // Future: your own IR representation
    // std::unique_ptr<LinearIR> ir;

    // Future: target machine description
    // TargetMachine* target = nullptr;

private:
    std::vector<Pass> passes_;
};

} // namespace eval
