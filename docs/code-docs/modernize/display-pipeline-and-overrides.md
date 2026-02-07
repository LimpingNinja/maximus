# Maximus Display Pipeline and Per-File Override Strategy

This document describes how Maximus chooses and renders display files (`.BBS`, `.RBS`, etc), how those files become ANSI/AVATAR/RIP output at runtime, and proposes a per-file override strategy using `.ANS`, `.AVT`, and `.TXT` files.

## Terminology

- **Display-script file**: A file that is parsed by Maximus’s `Display_File()` engine (`max/display.c`). These files contain Maximus/FBBS display control bytes (generally values `0x00..0x1A`) and text bytes. Examples: `.BBS`, `.RBS`, and historically `.GBS`.
- **Raw terminal file**: A file that should be streamed “as-is” to the caller’s terminal (e.g. raw ANSI escape sequences). These files should not be parsed by `Display_File()` because bytes `<= 26` would be interpreted as Maximus display controls.

## Current Runtime Pipeline (What Happens Today)

### 1) File selection by extension

File selection occurs in `DisplayOpenFile()` (`max/display.c`). The choice is based on the caller’s negotiated capabilities:

- **RIP** is represented by `usr.bits & BITS_RIP` and `hasRIP()` is defined in `max/max.h`.
- **ANSI/AVATAR/TTY** is represented by `usr.video` (`GRAPH_ANSI`, `GRAPH_AVATAR`, `GRAPH_TTY`).

`DisplayOpenFile()` currently probes in this order:

- If `hasRIP()`:
  - Try `basename + ".rbs"` first.
- If `usr.video == GRAPH_ANSI`:
  - Try `basename + ".gbs"` next (if it exists).
- Otherwise:
  - Try `basename + ".bbs"`.
- Finally:
  - Try the bare filename without adding an extension.

Even if your tree doesn’t use `.gbs`, the code path still exists.

### 2) Display-script parsing (`.BBS` engine)

Once a file is opened by `DisplayOpenFile()`, the extension no longer matters: the file is processed by the same display engine.

Entry point:

- `Display_File()` → `DisplayOneFile()` → `DisplayNormal()` (or `DisplayFilesBbs()` for `FILES.BBS`).

In `DisplayNormal()` (`max/display.c`), each byte is read and handled as follows:

- If `ch <= 26`, it is treated as a **display control code** and dispatched via `dispfn[]` (defined in `max/displayp.h`).
- Otherwise it is treated as printable data and handled by `DCNormal()`, which ultimately calls `Putc(ch)`.

This is why `.BBS` files can contain more than “text”: they contain a bytecode-like stream of display operations.

### 3) Output translation (where `.BBS` becomes ANSI/AVATAR/RIP)

This is the critical detail:

- The `.BBS` engine does **not** emit ANSI directly.
- It emits a stream of characters and control bytes via `Putc()` / `Puts()` / `Printf()`.

The final terminal-specific translation happens at the output layer:

- `Putc()` (`max/max_out.c`) sends remote output to `Mdm_putc()`.
- `Mdm_putc()` is implemented in `max/max_outr.c`.

`Mdm_putc()` is a state machine that interprets AVATAR/Maximus control bytes and converts them depending on the caller’s mode:

- **ANSI callers (`usr.video == GRAPH_ANSI`)**:
  - Emits ANSI escape sequences (`ansi_cls`, `ansi_goto`, etc.) and converts attribute changes using `avt2ansi(...)`.
- **AVATAR callers (`usr.video == GRAPH_AVATAR`)**:
  - Sends AVATAR control sequences directly.
- **TTY callers (`usr.video == GRAPH_TTY`)**:
  - Degrades/suppresses cursor/attribute operations.
- **RIP callers (`hasRIP()`)**:
  - Enables RIP-aware parsing logic so that RIP sequences don’t break internal line/column accounting.

In other words:

- `.BBS` is a **display-script** format.
- ANSI/AVATAR/RIP are **output encodings** decided at runtime by `usr.video` and `BITS_RIP`.

## How `.BBS` Gets Its Color/Control Codes (MECCA)

MECCA is the compiler that turns `.MEC` into `.BBS` display-script files.

Relevant implementation:

- `util/mecca.c`

Key points:

- Default input extension:
  - `.mec` (normal)
  - `.mer` (RIP variant)
- Default output extension:
  - `.bbs` (normal)
  - `.rbs` (RIP variant)
- The `-r` flag toggles RIP output:
  - Help text: `MECCA <infile> [outfile] [-t] [-r]`
  - `-r` “changes the default output extension to .RBS.”

MECCA compiles tokens like `[lightgreen]`, `[white]`, etc. into the display-script control bytes used by the runtime engine.

## Utility: ANSI to `.BBS` conversion already exists

If the input is already a raw ANSI file (`.ANS`), there is an existing converter:

- `util/ansi2bbs.c` (builds into `ansi2bbs`)

This tool parses ANSI escape sequences and writes a `.BBS`-compatible display-script stream (AVATAR-ish control bytes, RLE blocks, etc).

Important implication:

- You do not need MECCA to “understand ANSI” to get a `.BBS` out of `.ANS`. `ansi2bbs` already does it.

## Problem: Per-file overrides using raw `.ANS` / `.AVT` / `.TXT`

Because `Display_File()` parses bytes `<= 26` as Maximus display controls, raw files need special handling:

- A raw `.ANS` file might contain `0x0C` (form feed / clear) or other control bytes that would be interpreted by `Display_File()`.
- A raw `.AVT` file (true AVATAR stream) contains control bytes by definition and will be misinterpreted by `Display_File()`.

Therefore:

- `.BBS` / `.RBS` / `.GBS` should continue to use `Display_File()`.
- `.ANS` / `.AVT` / `.TXT` should be displayed using a **raw streaming display path** (read bytes and call `Putc()`), not the display-script parser.

## Proposed Solution (Option 1): Per-file override resolution order

Goal: allow per-file overrides without changing the compiled `.BBS` format.

### Capability model

- **RIP**: `hasRIP()` (i.e., remote + `usr.bits & BITS_RIP`)
- **ANSI**: `usr.video == GRAPH_ANSI`
- **AVATAR**: `usr.video == GRAPH_AVATAR`
- **TTY**: `usr.video == GRAPH_TTY`

### Proposed extension probe order

When the caller requests `Display_File(..., "misc/byebye")` (no extension), probe like this:

1) **RIP callers**

- Try `basename + .RBS` (display-script)
- If not found, continue into ANSI/AVATAR/TTY logic below

2) **ANSI callers** (`usr.video == GRAPH_ANSI`)

- Try `basename + .ANS` (raw ANSI override)
- Then try `basename + .GBS` (display-script, if your install uses it)
- Then try `basename + .BBS` (display-script)

3) **AVATAR callers** (`usr.video == GRAPH_AVATAR`)

- Try `basename + .AVT` (raw AVATAR override)
- Then try `basename + .BBS` (display-script)

4) **TTY callers** (`usr.video == GRAPH_TTY`)

- Try `basename + .TXT` (raw text override)
- Then try `basename + .BBS` (display-script)

5) **Fallback**

- Try the bare filename as passed (current behavior)

### Implementation note (important)

To make this correct:

- `.ANS`, `.AVT`, `.TXT` should be sent via a raw streaming routine (e.g. `Display_Raw_File()`), not via `Display_File()`.
- `.BBS/.GBS/.RBS` should continue through `Display_File()`.

This preserves existing behavior (MECCA compiled `.BBS` continues to be authoritative), while enabling per-file overrides where desired.

## Proposed Solution (Option 2): Add an ANSI compile mode ("-a")

You asked: “check if MECCA will compile a `.ANS` to a `.GBS`—if not, propose a solution.”

### What exists today

- MECCA (`util/mecca.c`) compiles `.mec` → `.bbs` (and `.mer` → `.rbs`)
- MECCA does not accept `.ans` as an input format.
- A separate tool exists that *does* accept `.ans` and emits `.bbs` display-script output:
  - `ansi2bbs` (`util/ansi2bbs.c`)

So: MECCA does **not** compile `.ANS` today.

### Option 2A: Keep MECCA as-is; introduce an ANSI-to-GBS build step

Because `.GBS` is treated as a display-script file (same pipeline as `.BBS`), the simplest approach is:

- Take `ansi2bbs` and create an `ansi2gbs` variant that writes the same display-script bytes but defaults its output extension to `.GBS`.

This can be as simple as building the same source with a different `EXT`.

Pros:

- Minimal impact on MECCA.
- Reuses the already working ANSI parser.

Cons:

- Adds another build-time tool / step.

### Option 2B: Add `-a` mode to MECCA (output extension `.GBS`)

If you want MECCA itself to produce `.GBS` outputs as a distinct “ANSI-flavor display-script”, the most direct MECCA change is:

- Add a new command-line flag (e.g. `-a`) that selects output extension `.gbs`.

This would *not* change the fundamental compiled bytecode format. It would only:

- Cause MECCA to emit `basename.gbs` instead of `basename.bbs`.

Pros:

- Integrates cleanly into existing MECCA usage patterns (`-r` already exists).
- Provides a first-class “ANSI-specific display-script output” file that Maximus can preferentially choose for ANSI users.

Cons:

- Still does not make MECCA parse raw `.ANS`; it’s about output targeting.

### Option 2C: Teach MECCA to accept `.ANS` as an input format

This is the most invasive option:

- Extend MECCA to recognize `.ans` inputs and compile them into the Maximus display-script format.

However, since `ansi2bbs` already implements ANSI parsing, the pragmatic approach would be:

- Reuse or refactor the `ansi2bbs` parser logic into a shared library/module, and call it from MECCA when input extension is `.ans`.

Pros:

- One tool pipeline (`mecca`) can compile both `.mec` and `.ans`.

Cons:

- Larger refactor and risk.

### Note about “convert .ANS codes to .AVT”

At runtime, Maximus can output either ANSI or AVATAR depending on `usr.video`.

The display-script format is already “AVATAR-like” in the sense that `Mdm_putc()` understands its control bytes and can:

- translate them to ANSI for ANSI callers (`avt2ansi`)
- or output AVATAR controls for AVATAR callers

So the existing `ansi2bbs` behavior (ANSI → display-script controls) is effectively “ANSI → a format that Maximus can deliver as ANSI or AVATAR”, which is the desired direction.

## Summary

- `.BBS` files are display-script bytecode.
- The conversion to ANSI happens at output time in `Mdm_putc()` (`max/max_outr.c`).
- `.GBS` (if used) is also a display-script file and goes through the same parsing + output translation.
- For per-file overrides:
  - Add `.ANS` / `.AVT` / `.TXT` probes per capability.
  - Ensure those extensions are displayed via a raw streaming path, not `Display_File()`.
- For ANSI compilation:
  - `ansi2bbs` already converts `.ans` → `.bbs` display-script.
  - If you want `.gbs`, either add an `ansi2gbs` tool (recommended) or add a `-a` flag to MECCA to target `.gbs` output (or do both).
