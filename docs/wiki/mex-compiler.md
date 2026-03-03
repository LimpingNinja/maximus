---
layout: default
title: "MEX Compiler"
section: "mex"
description: "Using the MEX compiler"
---

The MEX compiler (`mex`) is the tool that turns your `.mex` source files
into `.vm` bytecode — the format Maximus actually loads and runs. You write
human-readable code, the compiler translates it into instructions for the
Maximus VM, and your callers never see the difference. They just see
whatever cool thing you built.

If the compiler finds a problem — a missing semicolon, an undeclared
variable, a function that doesn't exist — it'll tell you exactly where the
problem is. Fix the line, recompile, try again. The cycle is fast.

## Where It Lives

The compiler binary lands in `build/bin/mex` after you build Maximus. It's
not something you install separately — if you've built the project, you've
got it.

MEX source files (`.mex`) and their compiled output (`.vm`) typically live
in `resources/m/`. The all-important header file `max.mh` — which every
MEX script needs to include — lives in `resources/m/include/`.

## Compiling a Script

The simplest invocation is just:

```
cd resources/m
../../build/bin/mex myscript.mex
```

If everything compiles cleanly, you'll get `myscript.vm` in the same
directory. No news is good news — the compiler is quiet on success.

If something's wrong, you'll get an error message with a line number:

```
myscript.mex 12: Undeclared identifier: 'naem'
```

The line number points you to the approximate location. Fix the typo,
recompile, move on. The error messages are straightforward — they tell you
what went wrong and where.

## For Developers: compile-mex.sh

> **Note:** This section is for people building Maximus from source — not
> sysops running a release. If you installed a release package, the `mex`
> compiler is already in your path and you compile scripts directly as
> shown above.

If you're working in the source tree and want a quick way to compile a
script, test it against a local build, and optionally stage it for a
release package, there's a convenience wrapper:

```bash
./scripts/compile-mex.sh myscript
```

This compiles `resources/m/myscript.mex` and copies the resulting `.vm`
to `build/scripts/` — handy for testing against a local build without
manually shuffling files around.

Add `--deploy` to also push the files into the install tree for
packaging:

```bash
./scripts/compile-mex.sh myscript --deploy
```

That copies both the `.mex` source and the `.vm` bytecode to
`resources/install_tree/scripts/`, which is what ends up in a release
tarball.

## What the Compiler Actually Does

Under the hood, the compiler:

1. **Preprocesses** — handles `#include` and `#define` directives, expands
   macros, strips comments.
2. **Parses** — reads your source into an abstract syntax tree, checking
   syntax and types along the way.
3. **Generates bytecode** — emits `.vm` instructions that the Maximus VM
   can execute at runtime.

The `.vm` file is a compact binary. You don't need to read it or edit it —
it's purely machine food. If you want to change what the script does, you
edit the `.mex` source and recompile.

## UTF-8 and CP437

Maximus displays in CP437 — the classic IBM PC character set that gives
you box-drawing characters, block elements, and all the symbols that make
a BBS look like a BBS. If you save your `.mex` file as UTF-8 (which most
modern editors do by default), those fancy characters won't match what
the caller's terminal expects, and you'll get garbage on screen.

The MEX compiler handles this automatically. During compilation, any
UTF-8 multi-byte sequences in your source — in strings, char literals,
anywhere — are converted to their CP437 equivalents. You can type `═` in
your editor and the compiled `.vm` will contain the correct CP437 byte
(`0xCD`). No manual hex codes required.

If you're saving your `.mex` file as CP437 already (the way you'd edit
an `.ans` file), the conversion isn't needed. Use the `-u` flag to
disable it:

```
mex -u myscript.mex
```

## Compiler Flags

| Flag | What It Does |
|------|-------------|
| `-d` | Debug output — show internal compiler state |
| `-o <file>` | Write output to a specific filename |
| `-h <size>` | Set heap size in bytes |
| `-s <size>` | Set stack size in bytes |
| `-q` | Quad output (dump intermediate representation, no `.vm`) |
| `-u` | Disable UTF-8 to CP437 conversion |

## Include Files

Most MEX scripts start with at least one `#include`:

```c
#include <max.mh>
```

This pulls in the standard Maximus header — type definitions, constants,
and declarations for all the built-in intrinsic functions (`print()`,
`input()`, `usr.*`, etc.). Without it, the compiler won't know about any
of the Maximus-specific functions your script wants to call.

There are additional headers for specialized functionality:

| Header | What It Provides |
|--------|-----------------|
| `max.mh` | Core types, constants, and intrinsics (always needed) |
| `maxui.mh` | UI library — lightbar menus, cursor positioning, color attributes |
| `max_menu.mh` | Menu command dispatch (`menu_cmd()`) |
| `socket.mh` | HTTP requests (`http_request()`) |
| `json.mh` | JSON parsing and building (`json_open()`, `json_get_str()`, etc.) |
| `prm.mh` | System path and configuration access (`prm_string()`) |
| `input.mh` | Extended input functions and constants |
| `rand.mh` | Random number generation (`srand()`, `rand()`) |
| `intpad.mh` | Number-to-string formatting helpers |

Headers are searched in the `include/` subdirectory relative to your
`.mex` file, which is why the standard layout puts `max.mh` in
`resources/m/include/`.

## Next Steps

Now that you know how to compile, the next page explains what goes *inside*
a `.mex` file — the structure of a script, from includes to `main()` to
everything in between.

**[Script Structure →]({% link mex-script-structure.md %})**
