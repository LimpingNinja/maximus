---
layout: default
---

<img src="{{ '/assets/logo.png' | relative_url }}" alt="Maximus BBS ANSI Logo" class="bbs-logo">

**Maximus BBS** was originally written by Scott J. Dudley and released in 1990. It became one of the most popular BBS packages of the early 1990s, known for its powerful MEX scripting language, flexible area management, and deep FidoNet integration.

**Maximus NG** (Next Generation) is a modernized fork that preserves the soul of the original while bringing it into the present — modern C17, TOML-based configuration, semantic theme colors, TLS support, and a local terminal backend with proper UTF-8/CP437 rendering.

---

## Quick Links

| | |
|---|---|
| **[Quick Start](quick-start/)** | From zero to a running BBS in four steps |
| **[Building Maximus](building/)** | Compile from source on Linux or macOS |
| **[Configuration](configuration/)** | System configuration with TOML files |
| **[Display Codes](display-codes/)** | Pipe codes, colors, info codes, and formatting |
| **[MEX Scripting](mex-getting-started/)** | Extend your BBS with the MEX language |
| **[Theme Colors](theme-colors/)** | Semantic color theming system |

---

## What's Different in NG?

- **TOML configuration** — human-readable config files replace the legacy PRM/CTL compile pipeline
- **Semantic theme colors** — 25 named color slots (`|tx`, `|pr`, `|er`, etc.) defined in `colors.toml`
- **Language TOML** — all user-facing strings in editable TOML with parameter metadata
- **MCI enhancements** — format operators (`$L`, `$R`, `$T`, `$C`, `$D`), deferred params (`|#N`)
- **MEX sockets + JSON** — HTTP/HTTPS requests and JSON parsing from MEX scripts
- **TLS support** — mbedTLS for secure outbound connections
- **Local terminal** — pluggable backend with proper CP437→UTF-8 translation
- **Multi-node safe** — no shared temp files; areas and access loaded directly from TOML into memory
