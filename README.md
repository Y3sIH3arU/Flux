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
write "Hello, world!"
write variable_name
wait for user
if user said "hi" then reply "Hey there!"
set name to "Flux"
define greet as
write "Hello from inside a block!"
end
greet
load "libm.so.6"
call sqrt 16.8
This is a comment


### ⚠️ Limitations (by design for v0.1.x)

- `if` body supports only a single statement (multi‑statement coming in v0.2.0).
- FFI restricted to hardcoded safe functions.
- No loops or `else` clause.
- Error messages do not yet include line numbers.

### 🔧 Recent Updates

**v0.1.1 "Hardened"**
- Security whitelists for libraries and FFI functions.
- Path traversal prevention.
- Full bounds checking in lexer and parser.
- Recursion depth guard.
- Memory leak fixes with ring buffer.

**v0.1.0 "Foundation"**
- Initial release with core I/O, variables, blocks, and basic FFI.

### 🚀 Quick Start

```bash
git clone https://github.com/Y3sIH3arU/Flux.git
cd Flux
make
./flux examples/hello.fx

 #USAGE#

./flux --help          # Show command reference
./flux script.fx       # Run a Flux program

🗺️ Roadmap
Version	Planned Features
0.2.0	Multi‑statement if bodies, else clause
0.3.0	Loops (repeat, while)
0.4.0	Expanded FFI with more safe functions
1.0.0	Stable standard library and full documentation
🤝 Contributing

Pull requests and security reports are welcome!
Please see SECURITY.md for responsible disclosure guidelines.
📄 License

MIT – see LICENSE for details.
text


---

Now both `README.md` and `SECURITY.md` are consistent and reflect the **0.1.1 "Hardened"** release. Push them together with the tag:

```bash
git add README.md SECURITY.md
git commit -m "Docs: update README and SECURITY for v0.1.1 Hardened"
git push
git push origin v0.1.1

