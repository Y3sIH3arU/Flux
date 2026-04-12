# Flux

**A minimalist, extensible scripting language with blocking I/O, user‑defined blocks, and a secure FFI.**

> UPDATES: Regular commits most nights (Central European Time).  
> Some days may have no updates—check the commit history for activity.

---

## Flux 0.1.1 "Hardened"

[![Version](https://img.shields.io/badge/version-0.1.1-blue)](https://github.com/Y3sIH3arU/Flux/releases)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Security](https://img.shields.io/badge/security-hardened-success)](SECURITY.md)

### ✨ Features

- **Core I/O:** `write` for output, `wait for user` for blocking input.
- **Conditionals:** `if user said "..." then reply "..."` with exact string matching.
- **Variables:** `set name to "value"`, copy from other variables, or assign numbers.
- **User‑defined Blocks:** `define name as ... end` – call reusable code by name.
- **Foreign Function Interface:** `load "libm.so.6"` and `call sqrt 16.8` (whitelisted for security).
- **Comments:** `#` to end of line.
- **Command‑line Help:** `flux --help` prints usage reference.
- **Memory Management:** AST is freed after execution.

### 🔒 Security Hardening (v0.1.1)

| Mitigation | Description |
|------------|-------------|
| **FFI Whitelist** | Only `sqrt`, `puts`, `sin`, `cos` allowed via `call`. |
| **Library Whitelist** | `load` restricted to `libm.so.6` and `libc.so.6`; path traversal blocked. |
| **Bounds Checking** | All string operations use safe, bounded copies. |
| **Recursion Guard** | Block recursion limited to 100 depth. |
| **Memory Safety** | Ring buffer prevents expression memory leaks. |

### 📝 Syntax at a Glance
