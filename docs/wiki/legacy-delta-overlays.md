---
layout: default
title: "Delta Overlays"
section: "configuration"
description: "The delta overlay system for language file upgrades"
---

Delta files let you layer changes onto a converted `.toml` language file
without editing it by hand. Every Maximus NG release ships a
`delta_english.toml` that adds new metadata, fixes conversion artifacts, and
optionally applies the NG semantic theme colors.

---

## Why Deltas Exist

The language converter does a mechanical translation of your `.mad` file — it
gets the structure and content right, but it cannot:

- Add **parameter metadata** (names, types, descriptions) that the language
  editor uses for inline help.
- Generate **lightbar prompt strings** — the rich `[Y]es,[n]o` format the
  lightbar UI needs.
- Apply **semantic theme colors** — replacing hardcoded numeric color codes with
  theme slot references like `|pr` (prompt) or `|er` (error).
- Fix **edge-case conversion artifacts** — occasional strings where AVATAR
  binary sequences inside printf specifiers get mangled.

The delta file handles all of these cleanly, without requiring you to hand-edit
the base `.toml`.

---

## Two Tiers

The delta has two independent tiers. You can apply one, the other, or both.

### Tier 1: Parameter Metadata (`@merge`)

Entries marked with `# @merge` add structured metadata to existing strings
without touching visible text:

```toml
# @merge
wrong_pwd = { params = [{name = "attempt_number", type = "int", desc = "Login attempt counter"}] }
```

This merges into the base string's line, preserving its `text` and `flags`
while adding the `params` field. The result:

```toml
wrong_pwd = { text = "\aWrong! (Try #|!1d)\n", params = [{name = "attempt_number", type = "int", desc = "Login attempt counter"}] }
```

Tier 1 is **safe for all files** — stock or user-customized. It only adds
documentation metadata; it never changes what the user sees on screen.

### Tier 2: Theme Colors (`[maximusng-*]`)

Sections prefixed with `maximusng-` contain theme color overrides per heap:

```toml
[maximusng-global]
wrong_pwd = { text = "|er\aWrong! (Try #|!1d)|cd\n" }
```

These replace hardcoded numeric color codes (`|12`) with semantic theme codes
(`|er` for error, `|cd` for default reset). Tier 2 is applied **only to stock
`english.toml`** — if you have a customized language file with your own color
choices, use `--merge-only` to skip this tier.

---

## Delta Mode Flags

These flags control which tiers get applied. They work with `--apply-delta`,
`--convert-lang`, and `--convert-lang-all`:

| Flag | Tier 1 (params) | Tier 2 (theme) | When to use it |
|------|:---:|:---:|------|
| `--full` (default) | ✓ | ✓ | Stock builds, fresh installs |
| `--merge-only` | ✓ | ✗ | User migration — keeps your colors untouched |
| `--ng-only` | ✗ | ✓ | Add the NG theme to an already-enriched file |

See [maxcfg CLI]({% link maxcfg-cli.md %}) for the full flag reference and
examples.

---

## File Location

The delta file must be named `delta_<langname>.toml` and placed alongside the
target `.toml`:

```
config/lang/english.toml         ← base language file
config/lang/delta_english.toml   ← delta overlay
```

### Auto-Detection

When you don't specify `--delta` explicitly:

- **`--apply-delta`** looks for `delta_<name>.toml` in the same directory as
  the target `.toml`.
- **`--convert-lang`** checks the output directory first, then falls back to
  the `.mad` input directory.

If no delta file is found, the operation succeeds silently — no delta is
applied.

---

## Replacement vs. Merge

### Plain Entries (Replace)

A delta entry without `# @merge` fully replaces the matching base key:

```toml
CYn = { text = "[Y]es,[n]o", flags = ["mex"] }
```

If the key exists in the base, the entire line is replaced. If the key is new,
it is inserted before the `[_legacy_map]` section.

### Merge Entries

A `# @merge` comment before an entry tells the converter to merge delta fields
into the existing line rather than replacing it:

```toml
# @merge
savedmsg2 = { params = [{name = "target_area", type = "string", desc = "Area message was saved to"}] }
```

This is a **shallow merge** — each field name is treated independently. The
base `text` field is preserved; the delta's `params` field is appended.

---

## Applying Deltas Manually

If you already have a converted `.toml` and want to apply (or re-apply) the
delta:

```bash
# Full apply (params + theme) — the default
bin/maxcfg --apply-delta config/lang/english.toml

# Params only — safe for user-customized files
bin/maxcfg --apply-delta config/lang/english.toml --merge-only

# Theme only — add NG theme colors to an enriched file
bin/maxcfg --apply-delta config/lang/english.toml --ng-only

# Use an explicit delta file
bin/maxcfg --apply-delta config/lang/english.toml \
           --delta /path/to/delta_english.toml
```

---

## Upgrade Workflow

When you upgrade to a new Maximus NG release:

1. **Keep your customized `english.toml`** — your edits are preserved.
2. **Replace `delta_english.toml`** with the one from the new release.
3. **Re-apply the delta:**

   ```bash
   bin/maxcfg --apply-delta config/lang/english.toml --merge-only
   ```

   Your colors stay put. New parameter metadata and fixes get merged in.

If you have not customized any strings, simply replace both `english.toml` and
`delta_english.toml` from the release.

---

## See Also

- [Legacy Migration]({% link legacy-migration.md %}) — migration overview
- [Convert Legacy MAD]({% link legacy-convert-mad.md %}) — the MAD → TOML
  conversion that produces the base file deltas apply to
- [CTL to TOML]({% link legacy-ctl-to-toml.md %}) — converting configuration
  files (separate from language deltas)
- [maxcfg CLI]({% link maxcfg-cli.md %}) — full command-line reference for
  `--apply-delta`, `--delta`, and mode flags
- [Language Files (TOML)]({% link lang-toml.md %}) — how the TOML language
  system works after migration
- [Language Conversion HOWTO]({% link lang-conversion.md %}) — detailed
  conversion walkthrough including delta application
