# MEX Nested Execution — Architectural Implementation Guide

**Date:** 2025-07-15
**Status:** **Proposed — Ready for Review**
**Prerequisite:** `plans/mex-nested-execution-design.md` (high-level design)
**Scope:** Concrete architectural details, codebase evidence, and step-by-step implementation plan for both nested MEX execution models

---

## Table of Contents

1. [MEX VM Architecture Deep Dive](#1-mex-vm-architecture-deep-dive)
2. [MexExecute Lifecycle Trace](#2-mexexecute-lifecycle-trace)
3. [Complete State Inventory](#3-complete-state-inventory)
4. [Intrinsic Registration Guide](#4-intrinsic-registration-guide)
5. [Design Questions Answered](#5-design-questions-answered)
6. [Model 1: Spawned MEX Execution (`mex_spawn`)](#6-model-1-spawned-mex-execution)
7. [Model 2: In-VM MEX Execution (`call_mex`)](#7-model-2-in-vm-mex-execution)
8. [Implementation Plan](#8-implementation-plan)
9. [Risk Analysis](#9-risk-analysis)

---

## 1. MEX VM Architecture Deep Dive

### 1.1 Data Segment Memory Layout

The VM allocates a single contiguous block for all runtime data in `VmReadFileHdr()` (`src/libs/mexvm/vm_read.c:66`):

```c
pbDs = malloc(vmh.lGlobSize + vmh.lStackSize + vmh.lHeapSize);
```

The layout within this block:

```
pbDs -->  [  Globals (lGlobSize)  |  Stack (lStackSize)  |  Heap (lHeapSize)  ]
          ^                       ^                       ^
          pbDs+0                  pbBp/pbSp (init)        pdshDheap
```

- **Globals region** (`0` to `lGlobSize`): Contains all named global variables from the .vm file plus runtime-injected variables (usr, marea, farea, msg, id, sys, input). An extra 2048 bytes is added at load time for Maximus-injected globals (`vm_read.c:61`).
- **Stack region** (`lGlobSize` to `lGlobSize+lStackSize`): Grows downward from the top. `pbSp` and `pbBp` are initialized to `pbDs + lGlobSize + lStackSize` (`vm_run.c:645`).
- **Heap region** (`lGlobSize+lStackSize` to end): Managed by a linked-list allocator (`vm_heap.c`). Used for dynamic string storage.

### 1.2 VM File Format (.vm)

Defined by `struct _vmh` in `src/libs/mexvm/vm.h:234-244`:

```c
struct _vmh {
  char vmtext[20];      // Copyright/identification text
  VMADDR n_inst;        // Number of instructions (quadruples)
  VMADDR n_imp;         // Number of global data references (imports)
  VMADDR n_fdef;        // Number of function definitions (exports)
  VMADDR n_fcall;       // Number of function call sites
  VMADDR lHeapSize;     // Size of heap
  VMADDR lStackSize;    // Size of stack
  VMADDR lGlobSize;     // Size of DATA/BSS globals
};
```

On-disk structure:
```
[_vmh header]
[_vm_quad instructions × n_inst]
[_imp global data refs × n_imp, each followed by _ipat patches]
[_dfuncdef function defs × n_fdef]
[_dfcall function calls × n_fcall, each followed by VMADDR patch arrays]
```

### 1.3 Code Segment

The code segment is an array of `struct _vm_quad` (instruction quadruples) allocated in `VmReadFileHdr()` (`vm_read.c:95`):

```c
pinCs = malloc(sizeof(INST) * high_cs);
```

Each quad contains: opcode, form (byte/word/dword/string), flags, two arguments, and a result/destination. The VM executes instructions sequentially via `vaIp` (instruction pointer), with jumps modifying `vaIp` directly.

### 1.4 Virtual Registers

Four register banks of 100 registers each, defined in `vm_ifc.h:42-45`:

```c
byte  regs_1[100];   // byte registers
word  regs_2[100];   // word registers — regs_2[0] is the primary return value
dword regs_4[100];   // dword registers — regs_4[0] is dword return
IADDR regs_6[100];   // IADDR/string registers — regs_6[0] is string return
```

Intrinsic functions communicate return values via `regs_2[0]` (word), `regs_4[0]` (dword), or `regs_6[0]` (string IADDR). The `REGS_ADDR` macro in `mexp.h:25` aliases `regs_6`.

### 1.5 String Representation

Strings in MEX are **not** null-terminated C strings. They are heap-allocated with a 2-byte length prefix:

```
[word: length][char[length]: data]
```

String variables hold an `IADDR` descriptor pointing to the heap location. The `IADDR` structure:

```c
typedef struct { VMADDR offset; byte segment; byte indirect; } IADDR;
```

Segments: `SEG_GLOBAL` (data segment), `SEG_AR` (activation record / stack frame), `SEG_TEMP` (registers).

### 1.6 Runtime Symbol Table

The runtime symbol table (`rtsym`) maps global variable names to data segment offsets. Initialized in `init_symtab()` (`vm_symt.c:38`) with capacity `256 + vmh.n_imp`. `MexEnterSymtab()` assigns sequential offsets from the globals region and returns the offset.

### 1.7 Function Definition List

`fdlist` is a linked list of `struct _funcdef`, containing function names and their starting quad numbers. Both user-defined functions (from the .vm file) and intrinsic functions are registered here. Intrinsic functions receive negative quad numbers starting at `-2` (decremented per function).

### 1.8 Heap Allocator

The heap (`vm_heap.c`) is a simple linked-list allocator using `struct _dsheap` headers embedded in the heap region. Each block has a size, next pointer, and free flag. `hpalloc()` walks free blocks and coalesces adjacent free blocks. `hpfree()` marks blocks as free (no coalescing on free).

### 1.9 Stack Frame Convention

Function calls use this stack protocol (`vm_opfun.c`):

1. **STARTCALL**: No-op marker
2. **ARG_VAL/ARG_REF**: Push arguments onto stack
3. **FUNCJUMP**: Push return address (`vaIp`), set `vaIp` to function start
4. **FUNCSTART**: Push old `pbBp`, set `pbBp = pbSp`, allocate local vars by decrementing `pbSp`
5. **FUNCRET**: Deallocate locals, pop `pbBp`, pop return `vaIp`, pop caller's arguments

For intrinsic calls, the pattern is different (`vm_run.c:675-696`):
1. Push `pbBp` onto stack
2. Set `pbBp = pbSp`
3. Call the C function (returns pop_size)
4. Pop `pbBp`, pop `vaIp`
5. Advance `pbSp` by pop_size (discards arguments)

---

## 2. MexExecute Lifecycle Trace

### 2.1 Entry: `Mex(char *file)` — `mex.c:908`

1. Duplicates filename string, converts underscores to spaces
2. Splits into filename and arguments at first space
3. Calls `MexStartupIntrin(filename, args, 0)`

### 2.2 Guard: `MexStartupIntrin()` — `mex.c:871`

```c
static int cMexInvoked = 0;  // static local — persists across calls
if (cMexInvoked)
    logit(log_mex_no_reentrancy);  // blocks and returns -1
else {
    cMexInvoked++;
    rc = MexExecute(...);
    cMexInvoked--;
}
```

**Key finding:** `cMexInvoked` is a static local integer in `MexStartupIntrin`. It is the sole re-entrancy guard. For spawned execution, this guard must be relaxed to allow nesting. For `call_mex()`, the guard must allow controlled re-entrance with a depth limit.

### 2.3 Core: `MexExecute()` — `vm_run.c:716`

```
MexExecute(pszFile, pszArgs, fFlag, uscIntrinsic, puf, pfnSetup, pfnTerm, pfnLog, pfnHkBefore, pfnHkAfter)
```

Sequence:
1. Set logger and hooks
2. `setjmp(jbError)` — error recovery point
3. `VmInit()` — **zeros ALL global VM state**
4. `add_intrinsic_functions()` — registers intrinsics into `fdlist`
5. `VmRead(pszFile)` — reads .vm file, allocates `pbDs`, `pinCs`, `rtsym`; initializes heap
6. `pfnSetup()` → `intrin_setup()` — creates instance stack, exports BBS state into VM globals
7. `VmRun(pszArgs)` — executes bytecode until `main()` returns
8. `pfnTerm(&ret)` → `intrin_term()` — imports data back to BBS, cleans up instance
9. `vm_cleanup()` — frees `pinCs`, `fdlist`, `rtsym`, `pbDs`

**Critical observation:** `VmInit()` zeroes ALL global VM variables (registers, pointers, counters). `vm_cleanup()` frees ALL allocated memory. There is **no concept of suspending** a VM — it is completely created and destroyed per execution.

### 2.4 Setup: `intrin_setup()` — `mex.c:344`

Creates a `_mex_instance_stack` and:
- Allocates global structures in VM data segment: `input` (linebuf), `id`, `usr`, `marea`, `farea`, `msg`, `sys`
- Exports current BBS user/area/message state into the VM
- Initializes file handle table (16 handles), search handles, caller info handle
- Pushes instance onto the `pmisThis` linked list

### 2.5 Execution: `VmRun()` — `vm_run.c:620`

1. Finds `main()` in `fdlist`
2. Initializes `pbBp = pbSp = pbDs + lGlobSize + lStackSize`
3. Pushes `pszArgs` as a string argument
4. Pushes sentinel return address `(VMADDR)-1`
5. Executes instruction loop until `vaIp == (VMADDR)-1`
6. Returns `regs_2[0]`

### 2.6 Teardown: `intrin_term()` — `mex.c:747`

1. Pops instance from `pmisThis` linked list
2. Closes any open file/search/user handles
3. Calls `MexJsonCleanup()` and `MexSockCleanup()`
4. Imports linebuf, user record, message state, file area back to BBS
5. Frees the instance stack entry

### 2.7 Cleanup: `vm_cleanup()` — `vm_run.c:236`

Frees: `pinCs`, `fdlist` (linked list + names), `rtsym`, `pbDs`. Sets all to NULL.

---

## 3. Complete State Inventory

### 3.1 VM-Layer Global State (in `src/libs/mexvm/`)

These are C globals that constitute the entire interpreter state. **All must be saved/restored for spawned execution.**

| Variable | Type | Location | Description |
|----------|------|----------|-------------|
| `pinCs` | `INST *` | `vm.h:298` | Code segment (instruction array) |
| `pbDs` | `byte *` | `vm.h:300` | Data segment (globals+stack+heap) |
| `pbSp` | `byte *` | `vm.h:301` | Stack pointer |
| `pbBp` | `byte *` | `vm.h:302` | Base pointer (frame pointer) |
| `pdshDheap` | `struct _dsheap *` | `vm.h:304` | Heap head pointer |
| `rtsym` | `struct _rtsym *` | `vm.h:306` | Runtime symbol table |
| `vmh` | `struct _vmh` | `vm.h:307` | VM file header (sizes) |
| `vaIp` | `VMADDR` | `vm.h:309` | Instruction pointer |
| `high_cs` | `VMADDR` | `vm.h:309` | Code segment size |
| `n_rtsym` | `VMADDR` | `vm.h:310` | Max symbol table entries |
| `n_entry` | `VMADDR` | `vm.h:311` | Current symbol table entries |
| `fdlist` | `struct _funcdef *` | `vm.h:313` | Function definition linked list |
| `usrfn` | `struct _usrfunc *` | `vm.h:170` | Intrinsic function table pointer |
| `regs_1[100]` | `byte[]` | `vm_ifc.h:42` | Byte registers |
| `regs_2[100]` | `word[]` | `vm_ifc.h:43` | Word registers |
| `regs_4[100]` | `dword[]` | `vm_ifc.h:44` | Dword registers |
| `regs_6[100]` | `IADDR[]` | `vm_ifc.h:45` | IADDR/string registers |
| `deb` | `int` | `vm.h:315` | Debug flag |
| `debheap` | `int` | `vm.h:316` | Heap debug flag |
| `pfnLogger` | `fn ptr` | `vm_run.c:43` (static) | Logger callback |
| `pfnHookBefore` | `fn ptr` | `vm_run.c:47` (static) | Pre-intrinsic hook |
| `pfnHookAfter` | `fn ptr` | `vm_run.c:48` (static) | Post-intrinsic hook |
| `jbError` | `jmp_buf` | `vm_run.c:52` (static) | Error recovery point |
| `vaLastAssigned` | `VMADDR` | `vm_symt.c:33` (static) | Symbol table allocation cursor |

### 3.2 Host-Layer State (in `src/max/mex_runtime/`)

| Variable | Type | Location | Description |
|----------|------|----------|-------------|
| `pmisThis` | `struct _mex_instance_stack *` | `mex.c:55` | Current instance stack head |
| `cMexInvoked` | `int` (static local) | `mex.c:873` | Re-entrancy guard counter |
| `_intrinfunc[]` | `struct _usrfunc[]` (static) | `mex.c:59` | Intrinsic function table |
| `sdList` | `PSTATDATA` (static) | `mexstat.c:33` | Static data persistence list |
| `sdStringList` | `PSTATDATA` (static) | `mexstat.c:34` | Static string persistence list |

### 3.3 BBS-Layer State (imported/exported by MEX)

This state lives in BBS globals and is synchronized with the VM at setup/teardown/hooks:

- `usr` — Current user record
- `linebuf` — Input buffer
- `mah` / `fah` — Current message/file area handles
- `sq` — Current message area squish handle
- `last_msg` — Last read message number
- `direction` — Message reading direction
- `current_line` / `current_col` / `display_line` — Screen position
- `baud` — Modem speed
- `inchat` — Chat status flag

### 3.4 State That Persists Across MEX Invocations

The static data mechanism (`mexstat.c`) uses C-side linked lists (`sdList`, `sdStringList`) that survive across separate `MexExecute()` calls within a node session. This is **already** a cross-invocation state sharing mechanism.

---

## 4. Intrinsic Registration Guide

### 4.1 How to Add a New Intrinsic Function

**Step 1: Declare the prototype** in `src/max/mex_runtime/mexint.h`:
```c
word EXPENTRY intrin_call_mex(void);
```

**Step 2: Add to the intrinsic table** in `src/max/mex_runtime/mex.c`, in the `_intrinfunc[]` array. **Must be in alphabetical order** (the VM resolves function calls by name matching during .vm file load):
```c
{"call_mex",              intrin_call_mex,                0},
```

**Step 3: Implement the function** (in a new or existing `.c` file in `src/max/mex_runtime/`). The calling convention:

```c
word EXPENTRY intrin_call_mex(void)
{
    MA ma;
    char *pszFile;
    char *pszArgs;

    MexArgBegin(&ma);
    pszFile = MexArgGetString(&ma, FALSE);   // First argument: filename
    pszArgs = MexArgGetString(&ma, FALSE);   // Second argument: args string

    // ... implementation ...

    regs_2[0] = result;  // Return value to MEX

    free(pszFile);
    free(pszArgs);

    return MexArgEnd(&ma);
}
```

**Step 4: Declare in the MEX header** `resources/m/max.mh` so MEX scripts can call it:
```c
int call_mex(string: ref filename, string: ref args);
```

**Step 5: Update the Makefile** if a new `.c` file was created.

### 4.2 Argument Passing Convention

- `MexArgBegin(&ma)` — Initialize argument parser (reads from stack)
- `MexArgGetWord(&ma)` — Pop a word argument
- `MexArgGetDword(&ma)` — Pop a dword argument
- `MexArgGetByte(&ma)` — Pop a byte argument
- `MexArgGetString(&ma, FALSE)` — Pop a string argument (returns malloc'd C string; caller must free)
- `MexArgGetRef(&ma)` — Pop a pass-by-reference argument (returns pointer into VM data segment)
- `MexArgGetRefString(&ma, &where, &wLen)` — Pop a reference string
- `MexArgGetNonRefString(&ma, &where, &wLen)` — Pop a value string (returns pointer into VM data; no free needed)
- `MexArgEnd(&ma)` — Finalize; returns the total bytes consumed from stack

### 4.3 Return Value Convention

- **Word return**: Set `regs_2[0]`
- **Dword return**: Set `regs_4[0]`
- **String return**: Call `MexReturnString(char *s)` or `MexReturnStringBytes(char *s, int len)` which stores into `regs_6[0]`

---

## 5. Design Questions Answered

### Q1: Are MEX globals program-global, interpreter-global, or node-global?

**Answer: Program-global.** Each `MexExecute()` call creates a fresh data segment with its own globals region. The runtime symbol table (`rtsym`) is per-execution. Globals from one MEX program are invisible to another unless explicitly communicated via the static data mechanism (`mexstat.c`) or BBS-layer state (user record, areas, etc.).

**Evidence:** `VmInit()` zeros all state (`vm_run.c:213`). `VmRead()` allocates a new `pbDs` (`vm_read.c:66`). `vm_cleanup()` frees it (`vm_run.c:267`).

**Exception:** The static data lists (`sdList`, `sdStringList` in `mexstat.c:33-34`) are C-side statics that persist across MEX invocations within the same node session.

### Q2: Can two compiled MEX programs safely coexist in one VM?

**Answer: No, not currently.** The VM is architected around a single loaded program. There is one code segment (`pinCs`), one data segment (`pbDs`), one symbol table (`rtsym`), and one function list (`fdlist`). Loading a second .vm file would overwrite all of these.

**Evidence:** `VmRead()` allocates fresh `pinCs` and `pbDs` and populates them from the file. There is no concept of "appending" a second program.

### Q3: What return types are practical for the first version?

**Answer: A word (int) return code.** This is the natural return of `VmRun()` (`vm_run.c:710`: `return regs_2[0]`). The child MEX's `main()` return value becomes the parent's return from the intrinsic call.

### Q4: Should the child inherit display context?

**Answer: Yes, implicitly.** Display context (cursor position, colors, ANSI state) is terminal-side state managed by the BBS I/O layer. Since both parent and child write to the same terminal, the child inherits whatever display state exists. The `intrin_hook_after` already syncs `current_row`/`current_col`/`more_lines` back after each intrinsic call.

### Q5: How should runtime errors in the child affect the parent?

**Answer: The child should return an error code.** `MexExecute()` already returns 1 on error (via `longjmp` to `setjmp` at `vm_run.c:732`). The spawned execution intrinsic can check this return and set `regs_2[0]` to an error indicator (e.g., -1). The parent continues executing.

### Q6: What is the minimum set of state to save/restore for spawned execution?

**Answer:** All 23+ VM-layer globals listed in Section 3.1. However, because `MexExecute()` creates and destroys all state internally (`VmInit` → `vm_cleanup`), **the simplest approach is to save/restore only the variables that survive across the call boundary.** Since `VmInit()` zeros everything and `vm_cleanup()` frees everything, the child execution is self-contained — **but the parent's state is destroyed by `VmInit()` and must be saved before and restored after.**

### Q7: Can the static data mechanism serve as inter-script communication?

**Answer: Yes.** `create_static_data`/`get_static_data`/`set_static_data` already provide named, typed, persistent storage across MEX invocations. For spawned execution, the child can read/write static data created by the parent. This is the simplest inter-script data sharing mechanism available today.

### Q8: What is the recursion depth concern?

**Answer:** Each nested MEX execution allocates `lGlobSize + lStackSize + lHeapSize` bytes (typically 10-100KB total), plus code segment memory, plus the instance stack. At a depth limit of 8-16, this is manageable. The primary risk is stack depth on the C side (each `MexExecute` → `VmRun` is a C stack frame), but modern Linux stacks handle this easily.

---

## 6. Model 1: Spawned MEX Execution

### 6.1 Concept

A new intrinsic `mex_spawn(string filename, string args)` suspends the parent MEX, runs a child MEX as a completely separate interpreter instance, and resumes the parent with the child's return code.

### 6.2 Implementation Strategy: Save/Restore VM Globals

The key insight: `MexExecute()` already handles the complete lifecycle (init → load → run → cleanup). The problem is that it **destroys** the current VM state when called. The solution is to:

1. Save all VM-layer globals into a struct before calling `MexExecute()`
2. Let the child `MexExecute()` run normally (it creates fresh state)
3. After child returns, restore the parent's saved state

### 6.3 State Save Structure

```c
struct _mex_vm_state {
    INST *pinCs;
    byte *pbDs;
    byte *pbSp;
    byte *pbBp;
    struct _dsheap *pdshDheap;
    struct _rtsym *rtsym;
    struct _vmh vmh;
    VMADDR vaIp;
    VMADDR high_cs;
    VMADDR n_rtsym;
    VMADDR n_entry;
    struct _funcdef *fdlist;
    struct _usrfunc *usrfn;
    byte regs_1[100];
    word regs_2[100];
    dword regs_4[100];
    IADDR regs_6[100];
    int deb;
    int debheap;
    /* Note: pfnLogger, pfnHookBefore, pfnHookAfter, jbError are static
       locals in vm_run.c and need special handling */
};
```

### 6.4 Access Problem: Static Variables in `vm_run.c`

Four critical variables are `static` in `vm_run.c` and **not accessible** from `mex.c`:
- `pfnHookBefore` (line 47)
- `pfnHookAfter` (line 48)
- `jbError` (line 52)
- `vaLastAssigned` in `vm_symt.c` (line 33)

**Solutions (choose one):**

**Option A — Expose save/restore API from the VM library:**
Add functions to `vm_run.c`:
```c
void EXPENTRY MexSaveState(struct _mex_vm_state *state);
void EXPENTRY MexRestoreState(struct _mex_vm_state *state);
```
These would be the cleanest approach — the VM library manages its own internals.

**Option B — Make static variables extern:**
Change `static` to `vm_extern` for the needed variables. Less clean but simpler.

**Recommendation:** Option A. Add `MexSaveState()`/`MexRestoreState()` to the VM library's public interface (`vm_ifc.h`).

### 6.5 Intrinsic Implementation Sketch

```c
/* In a new file: src/max/mex_runtime/mexspawn.c */

word EXPENTRY intrin_mex_spawn(void)
{
    MA ma;
    char *pszFile;
    char *pszArgs;
    struct _mex_vm_state saved;
    int rc;

    MexArgBegin(&ma);
    pszFile = MexArgGetString(&ma, FALSE);
    pszArgs = MexArgGetString(&ma, FALSE);

    if (!pszFile) {
        regs_2[0] = -1;
        goto done;
    }

    /* Import BBS state from current VM before suspending */
    MexImportData(pmisThis);

    /* Save the entire parent VM state */
    MexSaveState(&saved);

    /* Execute child MEX (creates fresh VM, runs, cleans up) */
    rc = MexExecute(pszFile, pszArgs ? pszArgs : "", 0,
                    N_INTRINFUNC, _intrinfunc,
                    intrin_setup, intrin_term, MexLog,
                    intrin_hook_before, intrin_hook_after);

    /* Restore parent VM state */
    MexRestoreState(&saved);

    /* Re-export BBS state into parent VM (child may have changed usr, areas) */
    MexExportData(pmisThis);

    regs_2[0] = (word)rc;

done:
    if (pszFile) free(pszFile);
    if (pszArgs) free(pszArgs);

    return MexArgEnd(&ma);
}
```

### 6.6 Recursion Guard Changes

The `cMexInvoked` guard in `MexStartupIntrin()` must be changed from a boolean block to a depth counter:

```c
#define MEX_MAX_NEST_DEPTH 8

static int cMexInvoked = 0;

int MexStartupIntrin(char *pszFile, char *pszArgs, dword fFlag)
{
    int rc = -1;

    if (cMexInvoked >= MEX_MAX_NEST_DEPTH) {
        logit("!MEX: maximum nesting depth (%d) exceeded", MEX_MAX_NEST_DEPTH);
        return -1;
    }

    cMexInvoked++;
    // ... existing MexExecute call ...
    cMexInvoked--;

    return rc;
}
```

### 6.7 The `_intrinfunc` Visibility Problem

`_intrinfunc[]` is `static` in `mex.c`. The child `MexExecute()` call inside `intrin_mex_spawn()` needs access to the same intrinsic table.

**Solutions:**
- **Option A:** Make `_intrinfunc` non-static and declare it `extern` in the header. Simple, minimal change.
- **Option B:** Store a pointer to `_intrinfunc` and `N_INTRINFUNC` in a module-level variable accessible to the spawn intrinsic.
- **Option C:** The spawn intrinsic lives in `mex.c` itself, where `_intrinfunc` is visible.

**Recommendation:** Option C for simplicity (keep `intrin_mex_spawn` in `mex.c`), or Option A for better modularity.

### 6.8 BBS State Synchronization

When the child MEX runs, it creates its own `_mex_instance_stack` via `intrin_setup()`, which exports the current BBS state (user, areas, etc.) into the child VM. When the child terminates, `intrin_term()` imports the child's changes back into BBS globals.

After restoring the parent VM state, the parent's VM data segment still contains the **pre-child** user/area data. We need to re-export the current BBS state into the parent VM:

```c
MexExportData(pmisThis);   // Sync current BBS state back to parent VM
MexExportUser(pmisThis->pmu, &usr);
```

This means the parent "sees" any user/area changes the child made — which is the desired behavior.

### 6.9 MEX Script API

In `resources/m/max.mh`:
```c
int mex_spawn(string: ref filename, string: ref args);
```

Usage from MEX:
```c
int rc;
rc = mex_spawn("learn/learn-file-io", "");
if (rc != 0)
    print("Sub-script failed with code: ", itostr(rc));
```

---

## 7. Model 2: In-VM MEX Execution (`call_mex`)

### 7.1 Concept

`call_mex(string filename, string args)` loads and executes another .vm file **within the current interpreter context**, sharing the same globals, heap, and BBS state. This is the "hybrid" model the user requested.

### 7.2 Why This Is Harder

The current VM architecture assumes:
- One code segment per execution
- One data segment per execution
- One function namespace per execution
- Global offsets are fixed at load time

Loading a second .vm file requires:
- A second code segment (or appending to the first)
- Merging or extending the data segment for the child's globals
- Resolving the child's function calls against both child and parent function lists
- Running the child's `main()` and returning to the parent's execution point

### 7.3 Proposed Architecture: Overlay Execution

Rather than merging code/data segments (extremely invasive), use an **overlay** approach:

1. **Save** the parent's code segment pointer and instruction pointer
2. **Load** the child's .vm file into a separate code segment
3. **Extend** the parent's data segment to accommodate the child's globals (or allocate child globals in the heap)
4. **Patch** the child's global references to point to offsets in the extended region
5. **Run** the child's `main()` using the parent's stack and heap
6. **Restore** the parent's code segment and resume

### 7.4 Shared State Model

The "hybrid" model means:
- **Shared**: heap (strings), BBS-exported globals (usr, marea, etc.), stack
- **Separate**: child's own local globals (from its .vm file)
- **Shared on request**: via static data mechanism or agreed-upon global names

The key insight is that the **BBS-injected globals** (usr, marea, farea, msg, id, sys, input) are injected by `intrin_setup()` into the data segment at **runtime**, not at compile time. The child's .vm file would reference its own `usr`, `marea`, etc. — these need to be patched to point to the **same** data segment locations as the parent's.

### 7.5 Implementation Strategy

**Phase 1: Minimal overlay**
1. Inside the intrinsic, save `pinCs`, `high_cs`, `vaIp`
2. Read child .vm file into a new code segment (using a modified `VmRead` that doesn't re-allocate `pbDs`)
3. The child's globals are allocated in the **existing** data segment's heap (not a new data segment)
4. Run `VmRun()` for the child
5. Restore parent's `pinCs`, `high_cs`, `vaIp`
6. Free child's code segment

**Phase 2: Shared BBS globals**
- When the child references globals like `usr`, `marea`, etc., patch them to the parent's existing offsets using the parent's runtime symbol table
- For the child's own globals (not matching any parent name), allocate new space

### 7.6 Required VM Library Changes

A new function is needed in the VM library:

```c
int EXPENTRY MexExecuteOverlay(char *pszFile, char *pszArgs);
```

This would:
1. Save `pinCs`, `high_cs`, `vaIp`, `fdlist`
2. Call a modified `VmRead` that:
   - Reads the child's .vm header
   - Allocates only `pinCs` (code segment) for the child
   - Uses the **existing** `pbDs` (globals go into available space or heap)
   - Resolves global references against the **existing** `rtsym` first, falling back to new allocations
   - Adds child functions to `fdlist` (prefixed or namespaced to avoid collision)
3. Run `VmRun(pszArgs)` using the child's code but the shared data segment
4. Remove child functions from `fdlist`
5. Restore `pinCs`, `high_cs`, `vaIp`

### 7.7 Symbol Collision Handling

If parent and child both define a global named `counter`, the current `MexEnterSymtab()` would return the **parent's** existing offset (because it searches by name first — `vm_symt.c:62-63`). This means the child would share the parent's `counter`. This is the "shared globals" behavior.

For isolation, child globals could be name-prefixed (e.g., `child$counter`), but this breaks source compatibility. The simpler model: **same-named globals are shared, unique globals are private.** This is consistent with the user's preference for shared globals.

### 7.8 MEX Script API

In `resources/m/max.mh`:
```c
int call_mex(string: ref filename, string: ref args);
```

Usage from MEX:
```c
int rc;
// The called script shares our usr, marea, farea, etc.
rc = call_mex("lib/inventory", "show");
```

### 7.9 Limitations of First Version

- No explicit export/import of individual variables
- No namespace isolation (same-named globals are shared)
- Child's `main()` runs to completion — no cross-file function calls
- No persistent loading (child code is loaded and discarded each call)

---

## 8. Implementation Plan

### Phase 1: Spawned MEX Execution (`mex_spawn`) — **Priority: HIGH**

#### Step 1.1: Add VM state save/restore API
- **Files:** `src/libs/mexvm/vm_run.c`, `src/libs/mexvm/vm_symt.c`, `src/libs/mexvm/vm_ifc.h`
- Define `struct _mex_vm_state` in `vm_ifc.h`
- Implement `MexSaveState()` and `MexRestoreState()` in `vm_run.c`
- Handle static locals (`pfnHookBefore`, `pfnHookAfter`, `jbError`, `vaLastAssigned`)

#### Step 1.2: Change recursion guard to depth counter
- **File:** `src/max/mex_runtime/mex.c`
- Replace boolean `cMexInvoked` check with `MEX_MAX_NEST_DEPTH` limit
- Add depth to log messages for debugging

#### Step 1.3: Implement `intrin_mex_spawn()`
- **File:** `src/max/mex_runtime/mex.c` (or new `mexspawn.c`)
- Save VM state, call `MexExecute()`, restore VM state
- Handle BBS state synchronization
- Set return code

#### Step 1.4: Register the intrinsic
- **Files:** `mex.c` (table), `mexint.h` (prototype)
- Add `{"mex_spawn", intrin_mex_spawn, 0}` to `_intrinfunc[]`

#### Step 1.5: Add MEX-side declaration
- **File:** `resources/m/max.mh`
- Add `int mex_spawn(string: ref filename, string: ref args);`

#### Step 1.6: Write test MEX scripts
- Parent script that calls `mex_spawn()` on a child
- Child script that modifies user state and returns a code
- Verify parent resumes correctly, sees child's user changes

#### Step 1.7: Update learn-menus.mex
- Replace `menu_cmd(MNU_MEX, ...)` calls with `mex_spawn()` calls
- Test the tutorial flow end-to-end

### Phase 2: In-VM MEX Execution (`call_mex`) — **Priority: MEDIUM**

#### Step 2.1: Add overlay execution support to VM library
- **Files:** `src/libs/mexvm/vm_run.c`, `src/libs/mexvm/vm_read.c`
- Implement `MexExecuteOverlay()` or split `VmRead` into reusable parts
- Handle code segment swap and data segment sharing

#### Step 2.2: Implement `intrin_call_mex()`
- **File:** `src/max/mex_runtime/mex.c` (or new file)
- Call overlay execution
- Manage instance stack correctly (same instance, new code)

#### Step 2.3: Register and declare
- Same pattern as Phase 1 steps 1.4 and 1.5

#### Step 2.4: Test shared globals behavior
- Parent sets a global, child reads/modifies it
- Verify BBS globals (usr, etc.) are shared
- Test symbol collision behavior

### Phase 3: Refinement

- Add `mex_spawn` and `call_mex` documentation to wiki
- Add error messages to language files
- Performance testing with nested depth > 2
- Edge case testing (missing .vm files, heap exhaustion, stack overflow)

---

## 9. Risk Analysis

### 9.1 Spawned Execution Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| Incomplete state save (missing a global) | **High** | Audit checklist in Section 3.1; compile-time static_assert on struct size |
| Static locals in vm_run.c inaccessible | **Medium** | Option A: expose via save/restore API |
| jbError corruption on nested longjmp | **High** | Each MexExecute sets its own jbError; save/restore handles this |
| Memory leak if child crashes mid-execution | **Medium** | vm_cleanup() always runs; parent restore deallocates nothing |
| BBS state desync after child runs | **Low** | MexExportData() after restore ensures sync |
| C stack depth with deep nesting | **Low** | MEX_MAX_NEST_DEPTH=8 keeps C stack usage bounded |

### 9.2 In-VM Execution Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| Data segment too small for child globals | **High** | Allocate child globals from heap or extend data segment |
| Symbol collision causes silent bugs | **Medium** | Document sharing semantics clearly; consider warning log |
| Child function names collide with parent | **Medium** | Remove child functions from fdlist after return |
| Heap fragmentation with repeated calls | **Medium** | Profile heap usage; consider heap compaction |
| Stack overflow from overlay execution | **Low** | Use same stack; child stack usage is bounded by its own lStackSize |
| VmRead refactoring complexity | **High** | Phase carefully; keep existing VmRead working for non-overlay path |

### 9.3 Overall Risk Assessment

**Spawned execution** is low-to-moderate risk. The save/restore pattern is straightforward and the execution model is well-isolated. The main risk is missing a piece of state.

**In-VM execution** is moderate-to-high risk. It requires significant VM library changes and introduces shared-state semantics that can cause subtle bugs. However, the "same-named globals are shared" model via `MexEnterSymtab` is a natural fit for the existing architecture.

**Recommendation:** Implement spawned execution first (Phase 1). This is achievable with minimal VM library changes and immediately solves the tutorial/menu-chaining problem. In-VM execution (Phase 2) can follow once the save/restore infrastructure is proven.

---

## Appendix A: Key File Reference

| File | Role |
|------|------|
| `src/libs/mexvm/vm_run.c` | VM execution engine, MexExecute, VmRun, VmInit, vm_cleanup |
| `src/libs/mexvm/vm_read.c` | .vm file loader |
| `src/libs/mexvm/vm_heap.c` | Heap allocator |
| `src/libs/mexvm/vm_symt.c` | Symbol table and string storage |
| `src/libs/mexvm/vm_opfun.c` | Function call/return opcodes |
| `src/libs/mexvm/vm.h` | VM structures, global variable declarations |
| `src/libs/mexvm/vm_ifc.h` | VM public API (exported functions) |
| `src/max/mex_runtime/mex.c` | Host integration, intrinsic table, setup/teardown, MexStartupIntrin |
| `src/max/mex_runtime/mexp.h` | Instance stack structure, constants |
| `src/max/mex_runtime/mex_max.h` | MEX-visible struct definitions |
| `src/max/mex_runtime/mexint.h` | Intrinsic prototypes, argument helpers |
| `src/max/mex_runtime/mexint.c` | Miscellaneous intrinsic implementations |
| `src/max/mex_runtime/mexrtl.c` | Runtime helpers (hooks, string import/export) |
| `src/max/mex_runtime/mexstat.c` | Static data persistence (cross-invocation storage) |
| `src/max/mex_runtime/mexxtrn.c` | shell() and menu_cmd() intrinsics |
| `resources/m/max.mh` | MEX header file for script-side declarations |

## Appendix B: Glossary

| Term | Meaning |
|------|---------|
| VMADDR | 32-bit unsigned address in the VM address space |
| IADDR | Indirect address: {offset, segment, indirect flag} |
| INST | Instruction quadruple: {opcode, form, flags, arg1, arg2, result} |
| pbDs | Pointer to base of data segment |
| pbSp | Stack pointer (grows downward) |
| pbBp | Base pointer (frame pointer for current function) |
| pinCs | Pointer to code segment (instruction array) |
| vaIp | VM instruction pointer (index into pinCs) |
| fdlist | Linked list of function definitions |
| rtsym | Runtime symbol table (global name → offset mapping) |
| pmisThis | Current MEX instance stack pointer |
| FORM | Data type enum: FormByte, FormWord, FormDword, FormAddr, FormString |
