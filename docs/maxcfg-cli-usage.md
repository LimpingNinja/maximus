# maxcfg Command-Line Reference

When you run `maxcfg` by itself it launches the interactive TUI—the full
configuration editor with menus, forms, and all the trimmings. But maxcfg also
has a set of CLI flags for headless work: converting files, exporting configs,
and applying delta overlays. That's what this page covers.

---

## General Usage

```
maxcfg [sys_path] [options]
```

If you don't pass a `sys_path`, maxcfg tries to figure it out from the binary
location or the first positional argument.

| Flag | Description |
|------|-------------|
| `-h`, `--help` | Show usage summary and exit |
| `-v`, `--version` | Show version info and exit |

---

## Config Export

These flags convert your legacy CTL configuration files to next-gen TOML
format. This is a one-way upgrade—CTL in, TOML out.

| Flag | Argument | Description |
|------|----------|-------------|
| `--export-nextgen` | `<path/to/max.ctl>` | Parse legacy CTL files and emit TOML. Exits after export. |
| `--export-dir` | `<path>` | Override output directory. Implies `--export-nextgen`. |

**Examples:**

```bash
# Export from a specific max.ctl to the default config directory
maxcfg --export-nextgen /opt/max/etc/max.ctl

# Export to a custom output directory
maxcfg --export-nextgen /opt/max/etc/max.ctl --export-dir /tmp/ng-config
```

---

## Language Conversion

Got a legacy `.mad` language file? These flags convert it to TOML.

| Flag | Argument | Description |
|------|----------|-------------|
| `--convert-lang` | `<file.mad>` | Convert a single `.mad` file to `.toml` and exit. |
| `--lang-out-dir` | `<path>` | Output directory for the `.toml` file (default: same as input). |
| `--convert-lang-all` | — | Convert all `.mad` files in `<sys_path>/config/lang/` and exit. |

Here's what the converter does under the hood:

1. Parses the `.mad` file (resolving `#include` / `#define` directives).
2. Converts AVATAR color/cursor sequences to MCI pipe codes.
3. Writes a `.toml` file with heap sections, flags, and a `[_legacy_map]`.
4. Applies the delta overlay (see below) according to the active delta mode.

**Examples:**

```bash
# Convert a single file, output alongside the .mad
maxcfg --convert-lang config/lang/legacy/english.mad

# Convert to a specific output directory
maxcfg --convert-lang config/lang/legacy/english.mad \
       --lang-out-dir config/lang

# Convert all .mad files under the sys_path lang directory
maxcfg /opt/max --convert-lang-all
```

---

## Delta Overlay

This is the interesting one. Every release ships a `delta_english.toml` that
contains improvements you can layer onto your language file without replacing
it. The delta has two tiers:

- **Tier 1 (`@merge`)** — parameter metadata (`params`, `desc`, `default`).
  Enriches strings with documentation without touching visible text.
- **Tier 2 (`[maximusng-*]`)** — theme color overrides. Swaps legacy numeric
  color codes for semantic theme codes.

| Flag | Argument | Description |
|------|----------|-------------|
| `--apply-delta` | `<file.toml>` | Apply delta overlay to an existing `.toml` file and exit. |
| `--delta` | `<delta.toml>` | Explicit path to the delta file. Default: auto-detect `delta_<name>.toml` in the same directory. |

### Delta Mode Flags

These control which tiers get applied. They work with `--apply-delta`,
`--convert-lang`, and `--convert-lang-all`.

| Flag | Tier 1 (params) | Tier 2 (theme) | When to use it |
|------|:---:|:---:|------|
| `--full` (default) | ✓ | ✓ | Stock builds, fresh installs |
| `--merge-only` | ✓ | ✗ | User migration—keeps your colors untouched |
| `--ng-only` | ✗ | ✓ | Add the NG theme to an already-enriched file |

**Examples:**

```bash
# Full apply (params + theme) — the default
maxcfg --apply-delta config/lang/english.toml

# Params only — safe for user-customized files
maxcfg --apply-delta config/lang/english.toml --merge-only

# Theme only — add NG theme colors to an enriched file
maxcfg --apply-delta config/lang/english.toml --ng-only

# Use an explicit delta file
maxcfg --apply-delta config/lang/english.toml \
       --delta /path/to/delta_english.toml --full

# Convert a legacy .mad and apply only param metadata
maxcfg --convert-lang config/lang/legacy/english.mad \
       --lang-out-dir config/lang --merge-only
```

### How Auto-Detection Works

When you don't pass `--delta` explicitly:

- **`--apply-delta`** looks for `delta_<name>.toml` in the same directory as
  the target `.toml` file.
- **`--convert-lang`** checks the output directory first, then falls back to
  the `.mad` input directory.

If no delta file is found, nothing happens—the operation succeeds silently.

---

## Workflow Recipes

Here are the common scenarios you'll actually run into.

### Fresh Install

```bash
# The shipped english.toml is already pre-merged.
# Nothing extra needed — just install and run.
```

### Build Pipeline

```bash
# Start from the pure .mad conversion, apply full delta
maxcfg --convert-lang resources/lang/legacy/english.mad \
       --lang-out-dir resources/lang --full
```

### Migrating from a Custom .mad

```bash
# Step 1: Convert your custom .mad, merge only param metadata
maxcfg --convert-lang config/lang/legacy/english.mad \
       --lang-out-dir config/lang --merge-only

# Step 2 (optional): Opt in to the NG theme later
maxcfg --apply-delta config/lang/english.toml --ng-only
```

### Upgrading to a New Release

```bash
# Drop in the new delta_english.toml from the release, then:
maxcfg --apply-delta config/lang/english.toml --merge-only

# Your colors stay put. New param metadata gets merged in.
```

---

For more on how the language file itself works, see
[Using Language TOML Files](using-lang-toml.md).
