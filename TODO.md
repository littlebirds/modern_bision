# Compiler TODOs

This is the working backlog for unfinished language and compiler behavior.
A task is complete when its acceptance criterion is covered by a passing test.

## Semantic Analysis

| ID | Priority | Task | Status | Acceptance criterion |
|---|---|---|---|---|
| SEM-001 | High | Validate function argument count | Todo | A call with too few or too many arguments fails during compilation. |
| SEM-002 | High | Validate function argument types | Todo | A call whose argument type differs from the parameter type fails during compilation. |
| SEM-003 | High | Validate declared return types | Todo | A function returning a value with a different type from its declaration fails during compilation. |
| SEM-004 | High | Validate callable targets | Todo | Calling a non-function value fails during compilation. |
| SEM-005 | Medium | Require boolean conditions | Todo | `if` and `while` conditions must have type `bool`. |
| SEM-006 | Medium | Validate return placement | Todo | A `return` outside a function fails during compilation. |
| SEM-007 | Medium | Validate all return paths | Todo | A function with a declared return type cannot finish without a compatible return value. |
| SEM-008 | Medium | Improve semantic diagnostics | Todo | Semantic errors include source location, construct, and expected/actual types where applicable. |
| SEM-009 | Low | Support function types in the type checker | Todo | Function values can be declared as parameters or returned from functions. |

## LLVM Backend

| ID | Priority | Task | Status | Acceptance criterion |
|---|---|---|---|---|
| BACK-001 | High | Support arrays in compiled mode | Todo | A valid array literal compiles to LLVM IR without the current unsupported error. |
| BACK-002 | High | Support array indexing | Todo | Valid array indexing compiles and invalid indexes produce a clear error. |
| BACK-003 | Medium | Support higher-order calls | Todo | A function value can be passed to and called by another compiled function. |
| BACK-004 | Medium | Implement `if` expressions | Todo | Both branches contribute a value through an LLVM phi node. |
| BACK-005 | Low | Remove dead return blocks | Todo | Generated functions do not contain unused return blocks. |
| BACK-006 | Low | Add JIT execution mode | Todo | A command-line mode executes compiled LLVM code instead of only printing IR. |

## Workflow

- Keep source comments for local implementation notes only; track cross-cutting work here.
- Add or update a focused test before changing a task from `Todo` to `In Progress`.
- Change `In Progress` to `Done` only after the acceptance test passes.
- Use `Blocked` when a task depends on a language-design decision or another backlog item.
- Update [COMPILER_DESIGN.md](COMPILER_DESIGN.md) when an architectural limitation or decision changes.

## Status Values

`Todo` | `In Progress` | `Blocked` | `Done`
