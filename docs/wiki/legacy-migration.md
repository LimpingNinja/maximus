---
layout: default
title: "Legacy Migration"
section: "configuration"
description: "Migrating from legacy Maximus 3.0 to Maximus NG"
---

If you're running a classic Maximus 3.0 board — with `.ctl` configuration
files, `.mad` language files, and the SILT/MAID compile pipeline — welcome to
Maximus NG. Your existing configuration isn't lost; it just needs to be
converted to TOML, which is the only format Maximus NG reads.

The good news: the conversion tools are built into `maxcfg`, the process is
mostly automated, and your original files are never modified. You run the
converter, review the output, and you're on TOML from that point forward.

---

## What Changed

Maximus NG replaces the two legacy compile toolchains with direct TOML loading:

| Legacy Pipeline | What It Did | Replaced By |
|-----------------|-------------|-------------|
| **SILT** | Compiled `.ctl` text files → binary `.prm` / `.dat` | Direct TOML loading at runtime |
| **MAID** | Compiled `.mad` language source → binary `.ltf` / `.lth` | Direct TOML loading at runtime |

Both SILT and MAID are retired. There is no compile step for configuration or
language files — you edit TOML directly and the BBS loads it at startup.

The conversion tools are built into `maxcfg`. You run them once to migrate your
files, then work in TOML from that point forward.

---

## Two Conversion Paths

Migration involves two independent operations. You can do one or both depending
on what you have.

### 1. Configuration: CTL → TOML

Your `.ctl` files (system settings, areas, menus, access levels, protocols,
colours, etc.) are converted to a set of TOML files in `config/`.

```bash
bin/maxcfg --export-nextgen /path/to/max.ctl
```

This is a one-way export — CTL in, TOML out. Your original files are not
modified.

**→ [CTL to TOML]({% link legacy-ctl-to-toml.md %})** — full walkthrough of
what gets converted, what to review, and edge cases.

### 2. Language: MAD → TOML

Your `.mad` language files (all the BBS prompts, messages, status strings) are
converted to a single `english.toml` that you can edit with any text editor.

```bash
bin/maxcfg --convert-lang /path/to/english.mad \
           --lang-out-dir config/lang
```

The converter handles AVATAR-to-MCI color translation, `#include`/`#define`
resolution, and printf-to-positional parameter mapping automatically.

**→ [Convert Legacy MAD]({% link legacy-convert-mad.md %})** — full walkthrough
including custom language files and the conversion pipeline.

---

## Delta Overlays

Every Maximus NG release ships a `delta_english.toml` that layers improvements
onto your converted language file — parameter metadata for editor help, lightbar
prompt overrides, and optionally the NG semantic theme colors.

The delta system has two tiers so you can pick up new metadata without losing
your color customizations.

**→ [Delta Overlays]({% link legacy-delta-overlays.md %})** — how the tier
system works, merge vs. replace, and upgrade workflows.

---

## Fresh Installs

If you don't have legacy files to migrate, none of this applies. The shipped
TOML configuration and `english.toml` language file are ready to use out of the
box. See [Building Maximus]({% link building.md %}) for build and install
instructions.

---

## Command Reference

All conversion operations use `maxcfg` on the command line. For the full flag
reference — export options, delta modes, output directories — see
[maxcfg CLI]({% link maxcfg-cli.md %}).

---

## See Also

- [CTL to TOML]({% link legacy-ctl-to-toml.md %}) — converting legacy
  configuration files
- [Convert Legacy MAD]({% link legacy-convert-mad.md %}) — converting legacy
  language files
- [Delta Overlays]({% link legacy-delta-overlays.md %}) — the delta overlay
  system for language files
- [maxcfg CLI]({% link maxcfg-cli.md %}) — command-line reference for all
  conversion flags
- [Language Files (TOML)]({% link lang-toml.md %}) — how the TOML language
  system works after migration
- [Building Maximus]({% link building.md %}) — building from source
