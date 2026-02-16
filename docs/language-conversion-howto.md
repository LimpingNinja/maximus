# Language Conversion HOWTO

## Converting Legacy Language Files to TOML

**Version 1.0**

Copyright © 2025 Kevin Morgan (Limping Ninja)
https://github.com/LimpingNinja

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Before You Begin](#2-before-you-begin)
3. [Running the Converter](#3-running-the-converter)
   - [Single File Conversion](#31-single-file-conversion)
   - [Batch Conversion](#32-batch-conversion)
   - [TUI Conversion](#33-tui-conversion)
4. [What the Converter Does](#4-what-the-converter-does)
5. [Delta Overlay Files](#5-delta-overlay-files)
   - [What Are Deltas?](#51-what-are-deltas)
   - [File Location](#52-file-location)
   - [Replacement Entries](#53-replacement-entries)
   - [Merge Entries](#54-merge-entries)
6. [After Conversion](#6-after-conversion)
7. [Upgrading from a Previous Version](#7-upgrading-from-a-previous-version)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. Introduction

If you are running Maximus with legacy `.mad` language files, this guide walks
you through converting them to the new TOML format. The good news — you only
need to do this once, and the converter handles all the heavy lifting.

The old language pipeline looked like this:

```
english.mad  ──MAID──►  english.ltf  (binary blob)
                    ├──► english.lth  (C header)
                    └──► english.mh   (MEX header)
```

The new system is simpler:

```
config/lang/english.toml   (human-readable, edit directly, no compile step)
```

Edit the TOML file, restart your BBS, and the change is live. No MAID, no
SILT, no binary blobs.

---

## 2. Before You Begin

You will need:

- A working Maximus NG installation with `maxcfg` in your `bin/` directory.
- Your legacy `.mad` language file(s) and their associated `.lh` include files
  (`colors.lh`, `rip.lh`, `user.lh`). These must be in the same directory as
  the `.mad` file.
- Write access to the output directory where the `.toml` file will be created.

If you are starting from a fresh install, Maximus ships with a pre-converted
`english.toml` — you can skip straight to
[Using Language TOML Files](using-lang-toml.md).

---

## 3. Running the Converter

### 3.1. Single File Conversion

Convert one `.mad` file to TOML:

```bash
maxcfg --convert-lang /path/to/english.mad
```

The converter writes `english.toml` to the same directory as the `.mad` file.

To write the output to a different directory, use `--lang-out-dir`:

```bash
maxcfg --convert-lang /path/to/legacy/english.mad \
       --lang-out-dir /path/to/maximus/config/lang
```

This is useful when your `.mad` files live in a legacy archive separate from
your active configuration.

### 3.2. Batch Conversion

Convert all `.mad` files in the language directory at once:

```bash
maxcfg --convert-lang-all
```

This looks for `.mad` files in `<sys_path>/config/lang/` and converts each one.
The `sys_path` is determined from the first positional argument or from
`maxcfg`'s own location.

### 3.3. TUI Conversion

If you prefer the graphical interface, open maxcfg and navigate to:

**Tools → Convert Legacy Language (MAD)**

This prompts for the language directory and converts all `.mad` files found
there. A summary is displayed when complete.

---

## 4. What the Converter Does

The converter replicates MAID's preprocessing logic, then emits clean TOML
instead of binary output. Here is what happens to your strings:

| Step | What changes |
|------|-------------|
| **Preprocessing** | `#include` and `#define` macros are resolved, just like MAID. |
| **AVATAR colors** | Raw `\x16\x01\xNN` bytes become MCI pipe color codes (`\|00`–`\|23`). |
| **AVATAR cursor** | Binary cursor sequences become MCI cursor codes (`[A##`, `[X##`, etc.). |
| **Printf specifiers** | `%s`, `%d`, `%ld`, etc. become positional parameters (`\|!1`, `\|!1d`, `\|!1l`). |
| **Flags** | `@MEX` becomes `flags = ["mex"]`; `@RIP` becomes a `rip = "..."` field. |
| **Heap sections** | Each `:heapname` becomes a TOML table (`[heapname]`). |
| **Legacy map** | A `[_legacy_map]` table maps old numeric IDs for backward compatibility. |

### Type Suffixes

Positional parameters carry type information from the original printf
specifiers. This helps the runtime handle typed arguments correctly:

| Printf | Positional | Type |
|--------|-----------|------|
| `%s` | `\|!1` | string (default, no suffix) |
| `%d`, `%i` | `\|!1d` | int |
| `%ld` | `\|!1l` | long |
| `%u` | `\|!1u` | unsigned int |
| `%c` | `\|!1c` | char |

You generally do not need to worry about these suffixes — the converter handles
them automatically, and the runtime knows what to do with them.

### Width and Padding

Printf width specifiers are converted to MCI formatting operators:

| Printf | MCI equivalent |
|--------|---------------|
| `%-20s` | `$R20\|!1` (right-pad to 20) |
| `%20s` | `$L20\|!1` (left-pad to 20) |
| `%.10s` | `$T10\|!1` (truncate to 10) |

---

## 5. Delta Overlay Files

### 5.1. What Are Deltas?

Delta files let you layer changes on top of a converted `.toml` without
modifying it by hand. This is especially useful for:

- **Lightbar prompt strings** — the converter cannot generate the rich
  `[Y]es,[n]o` format that the lightbar UI needs, so these are provided as
  delta overrides.
- **Parameter metadata** — structured `params` arrays that document what each
  `|!N` slot means (name, type) for editor help and validation.
- **Fixes** — if the mechanical conversion gets a string wrong, a delta entry
  patches it cleanly.

### 5.2. File Location

The delta file must be named `delta_<langname>.toml` and placed in the same
directory as the output `.toml`. For example:

```
config/lang/english.toml         ← base converted output
config/lang/delta_english.toml   ← delta overlay
```

The converter checks the output directory first, then the `.mad` input
directory, so you can keep them wherever makes sense for your setup.

### 5.3. Replacement Entries

A plain delta entry fully replaces the matching base key:

```toml
# Override the Yes/No prompt with rich lightbar format
CYn = { text = "[Y]es,[n]o", flags = ["mex"] }
```

If the key exists in the base, the entire line is replaced. If the key is new,
it is inserted before the `[_legacy_map]` section.

### 5.4. Merge Entries

A `# @merge` comment before an entry tells the converter to **merge** the delta
fields into the existing line rather than replacing it. This is how parameter
metadata gets added without overwriting the `text` or `flags` fields:

```toml
# @merge
wrong_pwd = { params = [{name = "attempt_number", type = "int"}] }
```

The result in the final `.toml` is a combined line:

```toml
wrong_pwd = { text = "\aWrong! (Try #|!1d)\n", params = [{name = "attempt_number", type = "int"}] }
```

The base `text` field is preserved; the delta's `params` field is appended.
This is a shallow merge — each field name is treated independently.

---

## 6. After Conversion

Once your `.toml` file is generated:

1. **Review it.** Open the file in any text editor. Strings are human-readable
   now — MCI color codes, cursor codes, and positional parameters are all
   visible.

2. **Copy it to your config directory.** If you used `--lang-out-dir`, the
   file is already there. Otherwise, copy it:

   ```bash
   cp english.toml /path/to/maximus/config/lang/
   ```

3. **Copy the delta file too.** The delta ships with Maximus releases and
   contains lightbar prompt overrides and parameter metadata:

   ```bash
   cp delta_english.toml /path/to/maximus/config/lang/
   ```

4. **Restart your BBS.** Language strings are loaded at startup. No compile step
   needed — just restart and the new strings are live.

---

## 7. Upgrading from a Previous Version

When you upgrade Maximus, the release includes an updated `delta_english.toml`
with new metadata and fixes. The upgrade workflow is:

1. **Keep your customized `english.toml`** — your edits are preserved.

2. **Replace `delta_english.toml`** with the one from the release.

3. **Re-run the converter** if you want the delta applied to a fresh base:

   ```bash
   maxcfg --convert-lang config/lang/legacy/english.mad \
          --lang-out-dir config/lang
   ```

4. Or **apply delta changes by hand** — the delta file is plain TOML, so you
   can copy specific entries into your customized file.

If you have not customized any strings, simply replace `english.toml` with the
one from the release.

---

## 8. Troubleshooting

### "The converter says it cannot find include files"

The `.lh` include files (`colors.lh`, `rip.lh`, `user.lh`) must be in the
same directory as the `.mad` file. These ship with the legacy Maximus
distribution. In the source tree, they live in `resources/lang/legacy/`.

### "My delta is not being applied"

- The delta file must be named exactly `delta_<langname>.toml` (e.g.,
  `delta_english.toml`).
- It must be in the output directory or the `.mad` input directory.
- Deltas are applied during conversion only, not at runtime. If you have
  already converted, edit the `.toml` directly or re-run the converter.

### "A string looks garbled after conversion"

Some strings with printf specifiers embedded inside AVATAR binary sequences
get mangled during the conversion pass. This is a known edge case. Fix the
string directly in the `.toml` file or via the maxcfg language editor.

### "Changes are not showing up"

- Make sure you edited the correct `.toml` file in your active
  `config/lang/` directory.
- Restart the BBS — language strings are loaded at startup.
- If testing from a build tree: `make build && make install` (both steps are
  required).
