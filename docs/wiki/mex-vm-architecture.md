---
layout: default
title: "VM Architecture"
section: "mex"
description: "How the MEX virtual machine works — bytecode format, stack machine, data segments, intrinsic dispatch, and the compiler pipeline"
---

> **Audience note:** This page is for developers working on Maximus itself
> or anyone curious about how MEX scripts actually execute under the hood.
> You don't need any of this to *write* MEX scripts — the
> [Language Guide]({{ site.baseurl }}{% link mex-language-guide.md %}) and
> [Standard Intrinsics]({{ site.baseurl }}{% link mex-standard-intrinsics.md %}) are what
> you want for that.

MEX is a compiled language. Your `.mex` source file goes through a compiler
(`mex`) that produces a `.vm` bytecode file. When Maximus needs to run a
script, it loads that `.vm` file into the MEX virtual machine — a
stack-based interpreter embedded in the BBS process. The VM executes
opcodes, manages a heap for dynamic strings, and dispatches calls to
*intrinsic functions* that bridge into the C runtime.

---

## On This Page

- [Pipeline Overview](#pipeline)
- [The .vm File Format](#vm-format)
- [Stack Machine](#stack-machine)
- [Data Segments](#data-segments)
- [String Heap](#string-heap)
- [Intrinsic Dispatch](#intrinsics)
- [Global Variable Binding](#globals)
- [Instance Stack](#instance-stack)
- [Compiler Flags](#compiler-flags)

---

## Pipeline Overview {#pipeline}

```
  .mex source
      │
      ▼
  ┌─────────┐    #include
  │   mex   │◄── .mh headers
  │ compiler│
  └────┬────┘
       │
       ▼
   .vm bytecode
       │
       ▼
  ┌──────────────────┐
  │  MEX VM runtime  │
  │  (inside max)    │
  │                  │
  │  ┌────────────┐  │
  │  │ MexExecute │  │
  │  │  bytecode  │  │
  │  │ interpreter│  │
  │  └──────┬─────┘  │
  │         │        │
  │  intrinsic calls │
  │         │        │
  │  ┌──────▼─────┐  │
  │  │  Maximus   │  │
  │  │  C runtime │  │
  │  └────────────┘  │
  └──────────────────┘
```

1. **Compile time:** The `mex` compiler reads `.mex` source and `.mh`
   headers, performs type checking, and emits a `.vm` bytecode file.
2. **Load time:** `MexExecute()` loads the `.vm` file, allocates the stack
   and heap, resolves intrinsic function references, and calls
   `intrin_setup()` to populate global variables.
3. **Run time:** The interpreter fetches and executes opcodes in a loop.
   Intrinsic calls dispatch to C functions registered in the
   `_intrinfunc[]` table.
4. **Teardown:** `intrin_term()` imports modified data back into the
   Maximus process (user record changes, area changes, etc.) and frees
   resources.

---

## The .vm File Format {#vm-format}

A `.vm` file contains:

| Section | Purpose |
|---------|---------|
| **Header** | Magic number, version, segment sizes, entry point offset |
| **Code segment (CSEG)** | Bytecode instructions |
| **Data segment (DSEG)** | Initial values for global variables and string literals |
| **Symbol table** | Names and addresses for global symbols (used for binding) |
| **Relocation table** | Fixups for absolute addresses in the code |

The compiler writes the segments sequentially. At load time, the VM
allocates memory for each segment and applies relocations. The code and
data segments are separate — MEX uses a Harvard-ish architecture where code
addresses and data addresses are in different namespaces.

---

## Stack Machine {#stack-machine}

MEX is a **stack-based** virtual machine. There are no general-purpose
registers. Operands are pushed onto the stack, operators consume them
and push results.

```
  Expression: a + b * c

  Bytecodes:
    PUSH a        stack: [a]
    PUSH b        stack: [a, b]
    PUSH c        stack: [a, b, c]
    MUL           stack: [a, b*c]
    ADD           stack: [a+b*c]
```

The stack is a contiguous block of memory with a configurable size
(default set at compile time, adjustable with the `-s` flag). Each stack
frame contains:

- **Return address** — where to resume after the function returns
- **Frame pointer** — base of the local variable area
- **Local variables** — allocated in the frame at known offsets
- **Temporaries** — intermediate expression values

Function calls push a new frame; returns pop it. The frame pointer chain
allows the VM to walk back through nested calls.

---

## Data Segments {#data-segments}

| Segment | Lifetime | Contents |
|---------|----------|----------|
| **CSEG** (code) | Entire execution | Bytecode instructions |
| **DSEG** (data) | Entire execution | Global variables, string literal pointers |
| **Stack** | Per-function | Local variables, temporaries, return addresses |
| **Heap** | Dynamic | String data, dynamically allocated blocks |

Global variables are allocated in DSEG at fixed offsets determined by the
compiler. The runtime can also inject globals at setup time — the
`EnterSymtabBlank()` function creates named entries in the symbol table
and allocates zeroed DSEG space for them.

---

## String Heap {#string-heap}

Strings in MEX are heap-allocated. A string variable is actually a
`IADDR` (indirect address) — a small descriptor containing:

- **Length** — current string length (as a `word`)
- **Data pointer** — offset into the heap where the character data lives

When you assign a string, the VM allocates new heap space, copies the
data, and updates the descriptor. When a string goes out of scope (local
variable in a returning function), the heap block is freed.

The heap has a configurable size (the `-h` compiler flag). String
concatenation (`+`) allocates a new block large enough for both operands,
copies both sides in, and returns the new descriptor. This is why
building strings in a tight loop can be expensive — each `+` allocates.

String literals from the source code are stored in the heap at load time
via `MexStoreHeapByteString()`. They persist for the life of the program.

---

## Intrinsic Dispatch {#intrinsics}

Intrinsic functions are the bridge between MEX bytecode and the C runtime.
They're registered in the `_intrinfunc[]` table in `mex.c`:

```c
static struct _usrfunc _intrinfunc[] = {
    {"strlen",        intrin_strlen,        0},
    {"print",         intrin_printstring,    0},
    {"ui_goto",       intrin_ui_goto,        0},
    // ... ~200 entries
    {NULL,            NULL,                  0}
};
```

Each entry maps a **MEX function name** (as declared in `.mh` headers) to
a **C function pointer**. When the VM encounters a call to an intrinsic,
it:

1. Looks up the function name in the table
2. Calls the C function, passing argument-access helpers
3. The C function uses `MexArgGetWord()`, `MexArgGetString()`, etc. to
   pull arguments from the VM stack
4. The C function calls `MexReturnString()` or pushes a return value
5. Control returns to the bytecode interpreter

The argument helpers handle type conversion between the VM's internal
representation and C types, including string descriptor → `char*`
translation.

### Print Is Special

`print()` in MEX is variadic — it accepts any number of arguments of any
type. The compiler resolves this by emitting a separate intrinsic call for
each argument based on its type:

| MEX type | Intrinsic called |
|----------|-----------------|
| `char` | `__printCHAR` |
| `int` | `__printINT` |
| `long` | `__printLONG` |
| `string` | `__printSTRING` |
| `unsigned int` | `__printUNSIGNED_INT` |
| `unsigned long` | `__printUNSIGNED_LONG` |

So `print("Hello", 42, "\n")` compiles to three separate intrinsic calls.

---

## Global Variable Binding {#globals}

When a MEX script starts, `intrin_setup()` creates the global variables
that scripts see after `#include <max.mh>`:

```c
pmis->vmaUser  = EnterSymtabBlank("usr",   sizeof(struct mex_usr));
pmis->vmaMarea = EnterSymtabBlank("marea", sizeof(struct mex_marea));
pmis->vmaFarea = EnterSymtabBlank("farea", sizeof(struct mex_farea));
pmis->vmaMsg   = EnterSymtabBlank("msg",   sizeof(struct mex_msg));
pmis->vmaSys   = EnterSymtabBlank("sys",   sizeof(struct mex_sys));
pmis->vmaID    = EnterSymtabBlank("id",    sizeof(struct mex_instancedata));
```

Each call allocates space in DSEG and registers the name in the symbol
table. The runtime then *exports* data from the Maximus C structs into
these MEX-side structs using `MexExportUser()`, `MexStoreMarea()`, etc.

When the script finishes, `intrin_term()` *imports* data back:

```c
MexImportUser(pmis->pmu, &usr);
```

This two-way sync means scripts can read *and* modify the user record,
change message/file areas, and update session state — all changes are
reflected back into the running BBS process.

### The `input` Global

The `input` variable (exposed as `linebuf` in C) is the stacked keyboard
input buffer. MEX scripts can read from it and it's synchronized on both
setup and teardown.

---

## Instance Stack {#instance-stack}

MEX scripts can invoke other MEX scripts (via `menu_cmd()`). Each
invocation gets its own `_mex_instance_stack` entry, which tracks:

| Field | Purpose |
|-------|---------|
| `vmaUser`, `vmaMarea`, etc. | DSEG addresses for this instance's globals |
| `fht[16]` | File handle table (up to 16 open files per instance) |
| `hafFile`, `hafMsg` | Area search handles |
| `huf`, `huff` | User file search handles |
| `fhCallers` | Caller log file handle |
| `next` | Pointer to parent instance |

The instance stack is a linked list. When a nested MEX invocation starts,
a new entry is pushed. When it finishes, the entry is popped and its
resources are cleaned up — open files are closed, search handles are
released, JSON and socket handles are freed.

This design prevents resource leaks even if a script crashes or exits
abnormally.

---

## Compiler Flags {#compiler-flags}

| Flag | Default | Purpose |
|------|---------|---------|
| `-h<size>` | (built-in) | Set heap size in bytes |
| `-s<size>` | (built-in) | Set stack size in bytes |
| `-q` | off | Emit quad (intermediate) output instead of `.vm` |
| `-u` | on | Disable UTF-8 → CP437 auto-conversion in string literals |

The `-h` and `-s` flags are baked into the `.vm` file header. The VM
allocates exactly that much memory at load time. If your script uses many
large strings or deep recursion, increase these values.

The `-u` flag controls whether the compiler's string literal scanner
converts UTF-8 multi-byte sequences to their CP437 single-byte
equivalents. This is on by default so you can write source in a modern
editor and have box-drawing characters (═, ║, etc.) automatically mapped
to their CP437 codepoints for terminal display.

---

## See Also

- [MEX Compiler]({{ site.baseurl }}{% link mex-compiler.md %}) — compiling scripts, include
  paths, and command-line usage
- [Language Guide]({{ site.baseurl }}{% link mex-language-guide.md %}) — the user-facing
  language reference
- [Standard Intrinsics]({{ site.baseurl }}{% link mex-standard-intrinsics.md %}) — every
  built-in function available to MEX scripts
