# Security Policy

**Flux 0.1.1 "Hardened"**  
*All reported critical vulnerabilities have been mitigated in this release.*

---

## Supported Versions

| Version | Codename   | Supported          |
| ------- | ---------- | ------------------ |
| 0.1.1   | Hardened   | Full support    |
| 0.1.0   | Foundation | Upgrade advised |

---

## Reporting a Vulnerability

**Please do not open public issues for security vulnerabilities.**

Send details to **[your-email@example.com]** with:

- Clear description of the issue.
- Steps to reproduce (a minimal `.fx` script if applicable).
- Any relevant error output.

**Response expectations:**
- Acknowledgment within 48 hours.
- Status update within 7 days.
- Credit in release notes (unless you prefer anonymity).

---

## Security Hardening in v0.1.1

| Mitigation | Description |
|------------|-------------|
| **FFI Whitelist** | Only `sqrt`, `puts`, `sin`, `cos` can be called via `call`. |
| **Library Whitelist** | `load` only accepts `libm.so.6` and `libc.so.6`; path traversal blocked. |
| **Bounds Checking** | All lexer/parser string operations use safe, bounded copies. |
| **Recursion Guard** | Block recursion limited to 100 depth to prevent stack overflow. |
| **Memory Safety** | AST freed after execution; temporary values use a ring buffer. |

---

## Known Limitations (Design Choices)

- Variables are global (no scoping) – intentional for v0.x simplicity.
- Error messages do not yet include line numbers.
- `if` bodies support only one statement (multi‑statement coming in v0.2.0).

---

## Dependency Security

Flux has **zero external dependencies** beyond the standard C library and `libdl`. Vulnerabilities in those components should be reported to their respective maintainers.

---

*Thank you for helping keep Flux secure.*
