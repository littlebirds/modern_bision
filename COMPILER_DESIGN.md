# Monkey Compiler — Design & Implementation Notes

> Generated from the implementation session. Covers the thinking process,
> architecture decisions, and all code needed to add a single-pass LLVM
> compiler to the Monkey language.

---

## 1. Concept Overview

### Why "Symphony Pass" (Single-Pass Semantic Analysis)

Instead of three independent tree walks (scope analysis, symbol resolution,
type checking), the compiler does everything in **one visit** over the AST:

```
visit(LetExpr)
  ├── TypeTable::isKnownTypeName()   ← type checking (reject `let int = 3`)
  ├── compileExpr(node.value)        ← IR generation (recurse)
  ├── createEntryAlloca()            ← allocate stack slot
  ├── builder_->CreateStore()        ← emit LLVM store
  └── currentScope_->define()        ← scope analysis

visit(IdentExpr)
  ├── resolve(name)                  ← symbol resolution (walk scope chain)
  └── builder_->CreateLoad()         ← IR generation

visit(IfStmt)
  ├── compileExpr(cond)              ← IR for condition
  ├── CreateCondBr → then/else/merge ← basic blocks
  └── each branch compiles independently
```

The tree-walking interpreter (`interpreter.cpp`) uses the same `ASTVisitor`
interface, so the compiler is a **parallel implementation** — same visitor
pattern, different `visit()` bodies that emit LLVM IR instead of computing
`Value` results.

### Active Record / Call Frame

The interpreter gets "active records" for free from C++'s call stack — each
`visit(CallExpr)` is a recursive call that saves/restores `ctx_`. The compiler
mirrors this: each `CallExpr` emits an LLVM `call` instruction; the LLVM JIT
(or compiled native code) manages the actual stack frames at runtime via the
architecture's calling convention.

---

## 2. Architecture

```
                    ┌──────────────────────────────────────┐
                    │               main.cpp                │
                    │  -i: REPL (interpreter)               │
                    │  -f: file → interpret                 │
                    │  -c: file → compile → dump LLVM IR    │
                    └──────────┬──────────┬─────────────────┘
                               │          │
                    ┌──────────┘          └──────────┐
                    ▼                                ▼
            ┌───────────────┐              ┌─────────────────┐
            │  Interpreter  │              │    Compiler      │
            │  (existing)   │              │  (pass manager)  │
            │  Value eval   │              │  LLVM IR gen     │
            └───────────────┘              └────────┬────────┘
                                                    │
                                            ┌───────┴───────┐
                                            │   SymTab       │
                                            │  (new — 54 loc)│
                                            │  Scope<T>      │
                                            └───────────────┘
```

### New Files

| File | Lines | Purpose |
|---|---|---|
| `include/symtab.hpp` | 54 | Generic `Scope<T>` template for scope chains |
| `include/compiler.hpp` | 67 | `Compiler` pass manager class with shared state |
| `include/passes/semantic_pass.hpp` | 11 | Semantic analysis pass declaration |
| `include/passes/llvm_ir_pass.hpp` | 12 | LLVM IR generation pass declaration |
| `src/passes/semantic_pass.cpp` | ~155 | Scope analysis + type checking visitor |
| `src/passes/llvm_ir_pass.cpp` | ~320 | Full LLVM IR generation for all AST nodes |

### Modified Files

| File | Change |
|---|---|
| `CMakeLists.txt` | `find_package(LLVM)`, link `core support native`, add pass sources |
| `include/ast.hpp` | Added `SemanticInfo` annotation struct, `std::any annotation` on `Node` |
| `include/interpreter.hpp` | Replaced `Context` with `Scope<Value>` |
| `src/interpreter.cpp` | Updated to use `Scope` API (`define`/`resolve`/`resolveMut`) |
| `src/main.cpp` | `-c <file>` mode: parse → compile → dump IR |

### How Calls / Recursion Work

Two-phase compilation of function literals (`FnLitExpr`):

```
Phase 1: declareFunction()
  Creates llvm::Function with type signature, param names
  Returns llvm::Function* pointer

Phase 2: compile body
  beginFunction(fn)        → entry block, allocas for params
  enterScope()             → new lexical scope
  node.body->accept(*this) → compile the body
  exitScope()              → pop scope
  emit implicit return     → store value_ to retval, ret
```

For `let` bindings, `LetExpr::visit` splits the phases so the symbol is
**registered before** the body compiles — enabling recursion:

```cpp
// LetExpr::visit (pseudocode)
if (value is FnLitExpr) {
    fn = declareFunction(fnLit);         // Phase 1
    currentScope_->define(Symbol(..., fn)); // register NOW
    // ... compile fnLit->body ...       // Phase 2 (can find "fact" in scope)
}
```

For direct calls, `CallExpr::visit` resolves the identifier, reads `sym->func`,
and emits `builder_->CreateCall(calleeFn, args)`.

---

## 3. Code Walkthrough

### 3.1 Symbol Table (`include/symtab.hpp`)

```cpp
struct Symbol {
    std::string name;
    TypeId type;           // type of the value
    llvm::Value* slot;     // alloca (stack slot) holding the value
    llvm::Function* func;  // non-null if symbol is a compiled function
    bool isMutable;
};

class Scope {
    std::unordered_map<std::string, Symbol> symbols_;
    std::shared_ptr<Scope> parent_;  // linked list for scope chain
public:
    void define(Symbol sym);           // add to THIS scope (shadows parent)
    const Symbol* resolve(name);       // walk up scope chain
    std::shared_ptr<Scope> parent();
};
```

**Why `slot` not `alloca`?** macOS `<alloca.h>` defines `alloca` as a macro
(`#define alloca(size) __builtin_alloca(size)`), which silently corrupts any
field named `alloca`. Renamed to `slot`.

**Why default constructor?** `std::unordered_map::operator[]` default-constructs
the value if the key doesn't exist. Without a default constructor, this fails
at compile time. Added `Symbol()` with zero-initialized fields.

### 3.2 Compiler (`include/compiler.hpp` + `src/passes/llvm_ir_pass.cpp`)

```
Compiler (pass manager)
  ├── Pipeline: vector of {name, function} passes
  ├── Shared state: ast, llvmCtx, module, builder
  └── Passes: semanticAnalysis → llvmIRGen

LLVMGen (internal to llvm_ir_pass.cpp)
  ├── LLVM infra:  ctx_, mod_, bld_
  ├── Scope chain: scope_  (enterScope/exitScope)
  ├── Function state: curFn_, retAlloca_, retBlock_
  └── Result register: val_  (like interpreter's result_)
```

**Key design decisions:**

1. **`value_` as result register** — same pattern as interpreter's `result_`.
   Each `visit(Expr)` sets `value_` to the generated LLVM `Value*`. Callers
   read `value_` after visiting an expression.

2. **`compileExpr()` helper** — calls `expr.accept(*this)`, returns `value_`.
   Convenience for composing expressions in binary ops, calls, etc.

3. **`createEntryAlloca()`** — all allocas go in the **entry block** of the
   current function (required by LLVM for mem2reg/promotion to work correctly).
   Uses a temporary `IRBuilder` positioned at the start of the entry block.

4. **`beginFunction()`** — sets up entry block, creates allocas for params
   (with store of the incoming argument), and sets up `returnAlloca_`/`returnBlock_`
   for explicit returns.

5. **`toLLVMTypeStatic()`** — made public static so `declareFunction()` (a free
   function) can use it. Instance method `toLLVMType()` delegates to it.

### 3.3 Expression Compilation Highlights

**Literals** — trivial LLVM constants:
```
IntLitExpr  → ConstantInt::get(i64, value)
FloatLitExpr → ConstantFP::get(double, value)
BoolLitExpr  → ConstantInt::get(i1, value)
StringLitExpr → CreateGlobalString(text, ".str")
```

**Short-circuit `and`/`or`** — generates basic blocks with phi nodes:
```
     and:                        or:
     [lhs]                       [lhs]
     /    \                      /    \
  rhs    merge                 merge  rhs
     \    /                      \    /
    phi(0, rhs)                 phi(1, rhs)
```

**Binary operators** — detect int vs float from LLVM types, promote int→float
if either operand is float, then emit the appropriate instruction
(`CreateAdd`/`CreateFAdd`, etc.).

### 3.4 Statement Compilation Highlights

**IfStmt** — three basic blocks (then, else, merge), conditional branch.
Currently `value_ = nullptr` at merge (if-expression support is a TODO).

**WhileStmt** — three blocks (cond, body, exit). Loop body branches back to cond.

**ReturnStmt** — stores value to `returnAlloca_`, branches to `returnBlock_`.
If no return handling (void function), emits `CreateRet(val)` directly.

**Implicit return** — after compiling a function body, if the block has no
terminator, checks `value_` (result of last expression), stores it to
`returnAlloca_`, loads it back, emits `ret`.

### 3.5 `main.cpp` — Compiler Mode (`-c`)

```cpp
if (strncmp(mode, "-c", 2) == 0) {
    // parse file into pAST
    auto* program = static_cast<ast::StmtList*>(pAST.get());
    eval::Compiler compiler;
    auto module = compiler.compile(*program);
    module->print(llvm::outs(), nullptr);
}
```

The `compile()` method wraps the program in an implicit `main() → i64` function,
so `-c` output is a valid LLVM module with a callable `main`.

---

## 4. Build Configuration

### CMakeLists.txt changes

```cmake
# macOS Homebrew: LLVM is keg-only, add /opt/homebrew/opt/llvm to search path
if(APPLE AND EXISTS "/opt/homebrew/opt/llvm")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/llvm")
endif()
find_package(LLVM REQUIRED CONFIG)
llvm_map_components_to_libnames(LLVM_LIBS core support native)

# Link monkey binary against LLVM
target_include_directories(monkey PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(monkey PRIVATE ${LLVM_DEFINITIONS})
target_link_libraries(monkey PRIVATE ${LLVM_LIBS})
```

**Why `LANGUAGES C CXX`?** LLVM's cmake module (`FindLibEdit.cmake`) runs
`check_include_file(histedit.h)` which requires a C compiler. Setting both
languages avoids configuration errors on macOS.

### macOS `alloca` macro conflict

The `<alloca.h>` header (included transitively through standard library headers)
defines `#define alloca(size) __builtin_alloca(size)`. Any field named `alloca`
in a struct gets silently expanded. Fix: rename to `slot`.

---

## 5. Sample Output

Input (`test.txt`):
```monkey
let add = fn(a int, b int) int { a + b; };
add(41, 1);
```

Output (`monkey -c test.txt`):
```llvm
define i64 @main() {
entry:
  %add = alloca ptr, align 8
  store ptr @fn.0, ptr %add, align 8
  %calltmp = call i64 @fn.0(i64 41, i64 1)
  ret i64 %calltmp
}

define internal i64 @fn.0(i64 %a, i64 %b) {
entry:
  %b2 = alloca i64, align 8
  %a1 = alloca i64, align 8
  store i64 %a, ptr %a1, align 4
  store i64 %b, ptr %b2, align 4
  %a3 = load i64, ptr %a1, align 4
  %b4 = load i64, ptr %b2, align 4
  %addtmp = add i64 %a3, %b4
  store i64 %addtmp, ptr %retval, align 4
  %retload = load i64, ptr %retval, align 4
  ret i64 %retload
}
```

Recursive factorial (`fact(5)`):
```llvm
define internal i64 @fn.0(i64 %n) {
entry:
  %n1 = alloca i64
  store i64 %n, ptr %n1
  %letmp = icmp sle i64 %n, 1
  br i1 %letmp, label %then, label %else

then:
  br label %ifmerge

else:
  %subtmp = sub i64 %n, 1
  %calltmp = call i64 @fn.0(i64 %subtmp)   ; recursive call
  %multmp = mul i64 %n, %calltmp
  br label %ifmerge

ifmerge:
  ; ... implicit return ...
}
```

---

## 6. Known Limitations & Next Steps

| Limitation | Status |
|---|---|
| If-as-expression (branches don't store to retval) | Needs phi node at merge block |
| Indirect/higher-order function calls | `sym->func` only; needs function pointer loading |
| Arrays, array indexing | Throws "not yet supported" |
| Dead `return:` block in output | Harmless; LLVM optimizer removes it |
| Int/float promotion is type-inferring from LLVM types | Could be more precise with TypeId tracking |
| No JIT execution (`-c` only dumps IR) | Add `llvm::ExecutionEngine` for `-j` mode |
| Empty return blocks when no explicit return | `beginFunction` always creates one |

---

## 7. Files Changed Summary

```
modified:   CMakeLists.txt
modified:   include/ast.hpp
modified:   include/interpreter.hpp
modified:   src/interpreter.cpp
modified:   src/main.cpp
deleted:    include/context.hpp
new file:   include/symtab.hpp
new file:   include/compiler.hpp
new file:   include/passes/semantic_pass.hpp
new file:   include/passes/llvm_ir_pass.hpp
new file:   src/passes/semantic_pass.cpp
new file:   src/passes/llvm_ir_pass.cpp
```

All existing tests still pass (`ctest --test-dir build` → 1/1 passed).
