---
layout: default
title: "Converting Legacy MAD"
section: "configuration"
description: "Converting legacy .mad language files to TOML"
---

This page covers converting your legacy Maximus 3.0 `.mad` language files to
the TOML format used by Maximus NG.

---

## The Legacy Pipeline

In classic Maximus, language strings (every prompt, status message, error, and
menu label the BBS displays) lived in `.mad` source files. The MAID compiler
parsed these into binary `.ltf` and `.lth` blobs:

```
english.mad ──MAID──► english.ltf  (binary string blob)
                  ├──► english.lth  (C header with offsets)
                  └──► english.mh   (MEX header)
```

You edited `.mad` files (or the `.lh` include files for colors and macros),
then ran MAID to recompile before changes took effect.

## What Replaced It

Maximus NG loads language strings directly from a single `english.toml` file at
startup. There is no compile step — edit the file, restart the BBS, and your
changes are live.

MAID, the `.ltf`/`.lth` binary blobs, and the C/MEX headers are all retired.

---

## Quick Start

Convert a single `.mad` file:

```bash
bin/maxcfg --convert-lang /path/to/english.mad \
           --lang-out-dir config/lang
```

Convert all `.mad` files in the language directory:

```bash
bin/maxcfg --convert-lang-all
```

Or use the maxcfg TUI: **Tools → Convert Legacy Language (MAD)**.

---

## What the Converter Does

The converter replicates MAID's preprocessing logic, then emits clean TOML
instead of binary output:

| Step | What changes |
|------|-------------|
| **Preprocessing** | `#include` and `#define` macros are resolved |
| **AVATAR colors** | Raw `\x16\x01\xNN` bytes become MCI pipe codes (`\|00`–`\|23`) |
| **AVATAR cursor** | Binary cursor sequences become MCI cursor codes |
| **Printf specifiers** | `%s`, `%d`, `%ld` become positional parameters (`\|!1`, `\|!1d`, `\|!1l`) |
| **Flags** | `@MEX` becomes `flags = ["mex"]`; `@RIP` becomes a `rip = "..."` field |
| **Heap sections** | Each `:heapname` becomes a TOML table (`[heapname]`) |
| **Legacy map** | A `[_legacy_map]` table preserves old numeric IDs for backward compat |

If a [delta overlay]({{ site.baseurl }}{% link legacy-delta-overlays.md %}) file is found, it is
applied automatically during conversion.

---

## Custom Language Files

If you have a customized `.mad` file (modified prompts, custom colors), use the
`--merge-only` flag to preserve your colors while still picking up parameter
metadata from the delta:

```bash
bin/maxcfg --convert-lang /path/to/english.mad \
           --lang-out-dir config/lang --merge-only
```

This applies only Tier 1 (parameter metadata) from the delta, leaving your
color choices intact. See [Delta Overlays]({{ site.baseurl }}{% link legacy-delta-overlays.md %})
for details on the tier system.

---

## Prerequisites

The converter needs the `.lh` include files that ship with your `.mad` file:

- `colors.lh` — color macro definitions
- `rip.lh` — RIP graphics macros
- `user.lh` — user-level macros

These must be in the same directory as the `.mad` file. In the source tree,
legacy includes live in `resources/lang/legacy/`.

---

## After Conversion

1. **Review the output.** Open the `.toml` in any text editor — strings are
   human-readable with MCI pipe codes visible.
2. **Copy the delta file.** Place `delta_english.toml` from the release
   alongside your converted `.toml` in `config/lang/`.
3. **Restart the BBS.** Language strings load at startup — no compile step.

---

## Full Walkthrough

For the complete step-by-step guide — including batch conversion, delta
overlay details, type suffixes, width/padding operators, troubleshooting,
and upgrade workflows — see
[Language Conversion HOWTO]({{ site.baseurl }}{% link lang-conversion.md %}).

---

## See Also

- [Language Conversion HOWTO]({{ site.baseurl }}{% link lang-conversion.md %}) — detailed
  step-by-step conversion guide
- [Legacy Migration]({{ site.baseurl }}{% link legacy-migration.md %}) — migration overview and
  the CTL → TOML configuration path
- [CTL to TOML]({{ site.baseurl }}{% link legacy-ctl-to-toml.md %}) — converting configuration
  files
- [Delta Overlays]({{ site.baseurl }}{% link legacy-delta-overlays.md %}) — the delta overlay
  system for language files
- [maxcfg CLI]({{ site.baseurl }}{% link maxcfg-cli.md %}) — full command-line reference for
  `--convert-lang` and other flags
- [Language Files (TOML)]({{ site.baseurl }}{% link lang-toml.md %}) — how the TOML language
  system works after migration
