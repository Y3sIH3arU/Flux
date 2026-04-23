# Flux Language Reverse Engineering

## Overview

Flux 0.1.0 "Foundation" is a minimalist interactive scripting language implemented in C. It features basic I/O, conditionals, variables, user-defined blocks, and a limited Foreign Function Interface (FFI) for calling C library functions.

## Architecture

Flux follows a classic three-stage pipeline:

```
Source File (.fx) → Lexer → Parser → Interpreter → Output
```

### File Structure

| File | Purpose |
|------|---------|
| `src/main.c` | Entry point, file I/O |
| `src/lexer.c` | Tokenization |
| `src/lexer.h` | Token type definitions |
| `src/parser.c` | AST construction |
| `src/parser.h` | AST node definitions |
| `src/interpreter.c` | AST execution |
| `src/interpreter.h` | Interpreter interface |

---

## Lexer (src/lexer.c)

The lexer converts source code into a stream of tokens with a maximum of 2048 tokens.

### Token Types

```
TOKEN_WRITE       - "write"
TOKEN_WAIT        - "wait"
TOKEN_FOR         - "for"
TOKEN_USER        - "user"
TOKEN_IF          - "if"
TOKEN_SAID        - "said"
TOKEN_THEN        - "then"
TOKEN_REPLY       - "reply"
TOKEN_SET         - "set"
TOKEN_TO          - "to"
TOKEN_DEFINE      - "define"
TOKEN_AS          - "as"
TOKEN_END         - "end"
TOKEN_LOAD        - "load"
TOKEN_CALL        - "call"
TOKEN_STRING      - "..."
TOKEN_NUMBER      - 42, 3.14
TOKEN_IDENTIFIER  - variable/block names
TOKEN_NEWLINE     - \n (filtered out)
TOKEN_EOF         - end of input
TOKEN_UNKNOWN     - error
```

### Lexer Rules

1. **Comments**: `#` to end of line
2. **Strings**: Double-quoted, max 255 characters with overflow detection
3. **Numbers**: Digits and optional decimal point (e.g., `3.14`, `.5`)
4. **Identifiers**: Alphanumeric + underscore, max 255 characters
5. **Keywords**: Matched case-insensitively via string comparison

---

## Parser (src/parser.c)

The parser builds an Abstract Syntax Tree (AST) from tokens.

### AST Node Types

```
NODE_WRITE    - write "text" / var
NODE_WAIT     - wait for user
NODE_IF_SAID  - if user said "..." then reply "..."
NODE_REPLY    - reply "text" / var
NODE_SET      - set name to value
NODE_DEFINE   - define name as ... end
NODE_CALL     - block invocation
NODE_LOAD     - load "lib.so"
NODE_FCALL    - foreign function call
NODE_STRING   - string literal or identifier reference
NODE_NUMBER   - numeric literal
```

### Node Structure (parser.h:20-27)

```c
typedef struct Node {
    NodeType type;
    char var_name[64];    // variable/block name
    char value[256];      // string value or condition
    int is_identifier;    // true if value is variable reference
    struct Node *body;    // child nodes (block body, if body, args)
    struct Node *next;    // next statement in sequence
} Node;
```

### Grammar

```
program      → statement*

statement    → write_stmt
             | wait_stmt
             | if_stmt
             | reply_stmt
             | set_stmt
             | define_stmt
             | call_stmt
             | load_stmt
             | fcall_stmt

write_stmt   → TOKEN_WRITE (STRING | IDENTIFIER)
wait_stmt    → TOKEN_WAIT TOKEN_FOR TOKEN_USER
if_stmt      → TOKEN_IF TOKEN_USER TOKEN_SAID (STRING | IDENTIFIER) TOKEN_THEN statement
reply_stmt   → TOKEN_REPLY (STRING | IDENTIFIER)
set_stmt     → TOKEN_SET IDENTIFIER TOKEN_TO (STRING | NUMBER | IDENTIFIER)
define_stmt  → TOKEN_DEFINE IDENTIFIER TOKEN_AS statement* TOKEN_END
call_stmt    → IDENTIFIER
load_stmt    → TOKEN_LOAD STRING
fcall_stmt   → TOKEN_CALL IDENTIFIER expression
expression   → STRING | NUMBER | IDENTIFIER
```

---

## Interpreter (src/interpreter.c)

The interpreter executes the AST with runtime state management.

### Runtime Data Structures

**Variables** (`vars` array):
- Max 256 variables
- Name: 64 chars, Value: 256 chars

**Blocks** (`blocks` array):
- Max 256 user-defined blocks
- Stored as AST node pointers

**Libraries** (`lib_handles` array):
- Max 16 loaded shared libraries

### Execution Model

Each `NODE_*` type has a corresponding case in `execute()`:

| Node Type | Behavior |
|-----------|----------|
| `NODE_WRITE` | Print string or variable value |
| `NODE_WAIT` | Read line from stdin into `last_input` |
| `NODE_IF_SAID` | Compare `last_input` to condition, execute body if match |
| `NODE_REPLY` | Print (used in if body) |
| `NODE_SET` | Create/update variable |
| `NODE_DEFINE` | Store block AST in `blocks` table |
| `NODE_CALL` | Execute block body (with recursion guard) |
| `NODE_LOAD` | `dlopen()` shared library |
| `NODE_FCALL` | Call foreign function via `dlsym()` |

### FFI Implementation

**Allowed Libraries** (security whitelist):
- `libm.so.6`
- `libc.so.6`

**Allowed Functions**:
- `sqrt(double) -> double`
- `puts(const char*) -> int`
- `sin(double) -> double`
- `cos(double) -> double`

**Security Measures**:
- Path traversal rejection (`/`, `..`, `.` prefix)
- Function name whitelist
- Recursion depth limit (100)

---

## Syntax Reference

```flux
# Output
write "Hello, World!"
write variable_name

# Input
wait for user

# Conditionals (exact match only)
if user said "hello" then reply "Hi there!"

# Variables
set name to "Flux"
set num to 42
set other to name

# User-defined blocks
define greet as
    write "Hello!"
    write "Welcome"
end
greet

# Foreign Function Interface
load "libm.so.6"
call sqrt 16.8

# Comments
# This is ignored
```

---

## Limitations (by design v0.1.0)

1. `if` body supports only one statement
2. FFI limited to hardcoded non-variadic functions
3. No loops
4. No `else` clause
5. No line numbers in error messages
6. Exact string match only for `if user said`
7. One argument per function call

---

## Build System

```makefile
# Makefile
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -ldl -lm

flux: src/main.o src/lexer.o src/parser.o src/interpreter.o
```

---

## Execution Flow Example

For input:
```flux
set name to "Flux"
write name
```

1. **Lex**: `SET`, `IDENTIFIER("name")`, `TO`, `STRING("Flux")`, `WRITE`, `IDENTIFIER("name")`
2. **Parse**: `NODE_SET(var_name="name", value="Flux", is_identifier=0)` → `NODE_WRITE(value="name", is_identifier=1)`
3. **Execute**:
   - `NODE_SET`: Store `{name: "Flux"}` in vars
   - `NODE_WRITE`: Lookup "name" → "Flux" → print "Flux"

---

## Memory Management

- All AST nodes allocated via `malloc()`
- `free_ast()` performs recursive cleanup after execution
- Ring buffer (`tmp_buf[4][256]`) for expression evaluation to prevent leaks
- Safe string copy via bounded `strncpy()` + null terminator

---

## Error Handling

- Lexer: Truncation warnings for oversized tokens
- Parser: Error messages with expected token hints, error recovery via token skipping
- Runtime: Security policy violations printed to stderr