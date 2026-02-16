# Using Language TOML Files

Good news—every piece of text your callers see lives in a single, plain-text
TOML file. Prompts, menus, error messages, log entries—all of it. You can
customize anything with your favorite editor. No compiler, no binary blobs,
no restart-and-pray.

This guide walks you through how the language file works, how to customize it,
and how to keep your changes safe when upgrades come around.

---

## Where Language Files Live

```
<prefix>/config/lang/english.toml         ← your active language file
<prefix>/config/lang/delta_english.toml   ← delta overlay (ships with releases)
```

Maximus loads the language file at startup. Change a string, restart, and it's
live. That's it.

---

## File Structure

The language file is organized into **heaps**—TOML tables that group related
strings by functional area:

```toml
[meta]
name = "English"
version = 1

[global]
press_enter = "Press ENTER to continue"
wrong_pwd = { text = "\aWrong! (Try #|!1d)\n", params = [{name = "attempt_number", type = "int"}] }

[max_init]
logo1 = "|15|12Maximus BBS\n"

[m_area]
no_msg = "No messages in this area.\n"
```

The heaps you'll see most often:

| Heap | What it covers |
|------|---------------|
| `global` | Prompts and strings used everywhere |
| `sysop` | Log messages and sysop-only text |
| `max_init` | Login, new user registration, welcome screens |
| `m_area` | Message area browsing and reading |
| `f_area` | File area browsing, uploads, downloads |
| `max_main` | Main menu and general navigation |
| `max_chng` | User settings (change city, password, etc.) |
| `max_chat` | Chat and paging |
| `protocols` | File transfer protocol messages |

---

## String Formats

Strings come in two flavors.

**Simple strings**—just a key and a quoted value:

```toml
press_enter = "Press ENTER to continue"
```

**Inline table strings**—when a string carries extra metadata like flags,
RIP alternates, or parameter descriptions:

```toml
left_x = { text = "\x16\x19\x02[D01|!1c", flags = ["mex"] }
located = { text = "|15\nLocated |!1d match|!2.\n", params = [{name = "count", type = "int"}, {name = "plural", type = "string"}] }
```

The `text` field holds the actual string. Everything else is metadata:

- **`flags`** — `["mex"]` means the string is accessible from MEX scripts.
- **`rip`** — an alternate string sent to RIP graphics terminals.
- **`params`** — documents what each `|!N` positional parameter means
  (see below).

---

## Escape Sequences

Language strings support the standard set of escapes:

| Escape | Meaning |
|--------|---------|
| `\n` | Newline |
| `\r` | Carriage return |
| `\t` | Tab |
| `\a` | Bell (beep) |
| `\\` | Literal backslash |
| `\"` | Literal double quote |
| `\xHH` | Raw hex byte |

---

## Display Codes in Language Strings

Language strings support the same display codes documented in
[Using Display Codes](using-display-codes.md). Here are the ones you'll run
into most:

**Colors** — `|00` through `|15` for foreground, `|16` through `|23` for
background.

**Formatting** — `$R20` to right-pad, `$L20` to left-pad, `$T10` to truncate.

**Terminal control** — `|CL` to clear screen, `|CR` for newline, `|BS` for
backspace.

**Information codes** — `|UN` for user name, `|BN` for BBS name, `|TL` for
time left, and dozens more.

For example:

```toml
welcome_back = "|14Welcome back, |15|UN|14! You have |15|TL|14 minutes today.\n"
```

This produces yellow text with the user's name and time remaining in white.

---

## Positional Parameters

Many strings contain **positional parameters**—placeholders that the BBS fills
in at runtime. They use the `|!` prefix:

| Code | Slot |
|------|------|
| `\|!1` through `\|!9` | Parameters 1–9 |
| `\|!A` through `\|!F` | Parameters 10–15 |

Parameters can carry a **type suffix** that tells the runtime what kind of
value to expect:

| Suffix | Type | Example |
|--------|------|---------|
| *(none)* | string | `\|!1` |
| `d` | int | `\|!1d` |
| `l` | long | `\|!1l` |
| `u` | unsigned int | `\|!1u` |
| `c` | char | `\|!1c` |

You usually don't need to think about suffixes when editing strings—just leave
them as they are. But if you're rearranging parameters, make sure the suffix
stays with the right slot number. The `params` metadata on the string tells you
what each slot means:

```toml
located = { text = "|15\nLocated |!1d match|!2.\n", params = [{name = "count", type = "int"}, {name = "plural", type = "string"}] }
```

Here, `|!1d` is the match count (an integer) and `|!2` is the plural suffix
(a string like `"es"` or `""`).

Positional parameters work with formatting operators too—`$R06|!1d` right-pads
the integer value to 6 columns.

---

## Customizing Strings

### Direct Editing

The simplest approach—open `english.toml` in your favorite editor, find the
string, change the text, save, restart. Done.

A few things to watch for:

- **Keep `|!N` parameters intact.** If the original has `|!1` and `|!2`, your
  replacement should use the same slots. You can reorder them (that's the whole
  point of positional parameters) but don't drop any the code expects.
- **Keep type suffixes.** If a parameter is `|!1d`, keep the `d`. Changing it
  to `|!1` will cause the runtime to interpret an integer as a string pointer,
  which is bad news.
- **Preserve flags.** If the original is an inline table with
  `flags = ["mex"]`, keep the flags. MEX scripts rely on them.
- **Test your changes.** Restart the BBS and exercise the area that uses the
  string. Typos in escape sequences can produce garbled output.

### Using the maxcfg Editor

If you'd rather have a guided experience, maxcfg includes a built-in language
string browser and editor. See
[Using the maxcfg Language Editor](using-maxcfg-langed.md) for details.

### What You Can Safely Change

| Safe to change | Be careful |
|---------------|-----------|
| Wording and phrasing | `\|!N` parameter slots and suffixes |
| Color codes | Inline table structure (`{ text = ... }`) |
| Formatting operators (`$R`, `$L`, `$T`) | Escape sequences (`\n`, `\a`, `\x`) |
| Adding/removing newlines | `flags` and `rip` fields |

---

## The Legacy Map

At the bottom of the language file you'll find a `[_legacy_map]` section:

```toml
[_legacy_map]
0x00 = "global.left_x"
0x01 = "global.located"
0x02 = "global.pl_match"
```

This maps old numeric string IDs (used by `s_ret()` in C and `lstr()` in MEX)
to the new dotted key names. **Don't edit this section**—it's auto-generated
by the converter and used for backward compatibility. It'll be removed in a
future version once all legacy numeric references are migrated.

---

## The Delta Overlay System

Every release ships a `delta_english.toml` alongside the stock `english.toml`.
The delta file contains two tiers of changes:

- **Tier 1 (`@merge`)** — parameter metadata (`params`, `desc`, `default`
  fields). These enrich strings with documentation about their positional
  parameters without touching any visible text or colors.
- **Tier 2 (`[maximusng-*]`)** — MaximusNG theme color overrides. These swap
  legacy numeric color codes (`|07`, `|14`, `|12`, …) for semantic theme codes
  (`|tx`, `|pr`, `|er`, …) that resolve through `colors.toml`.

You control which tiers get applied using delta mode flags:

| Flag | Tier 1 (params) | Tier 2 (theme) | Use case |
|------|:---:|:---:|------|
| `--full` (default) | ✓ | ✓ | Stock builds, fresh installs |
| `--merge-only` | ✓ | ✗ | User migration (keep your colors) |
| `--ng-only` | ✗ | ✓ | Add theme to an already-enriched file |

### Applying the Delta to an Existing File

Use `--apply-delta` to overlay the delta onto an existing `.toml` file:

```bash
# Full apply (params + theme) — the default
maxcfg --apply-delta config/lang/english.toml

# Params only — safe for user-customized files
maxcfg --apply-delta config/lang/english.toml --merge-only

# Theme only — add NG colors to an already-enriched file
maxcfg --apply-delta config/lang/english.toml --ng-only

# Explicit delta file path (instead of auto-detect)
maxcfg --apply-delta config/lang/english.toml --delta /path/to/delta_english.toml
```

By default, maxcfg looks for `delta_<name>.toml` in the same directory as the
target file. Use `--delta` to point somewhere else.

### Delta Modes During Conversion

The same mode flags work with `--convert-lang` when converting a `.mad` file:

```bash
# Convert .mad and apply full delta (default)
maxcfg --convert-lang config/lang/legacy/english.mad \
       --lang-out-dir config/lang

# Convert .mad but only merge param metadata (keep legacy colors)
maxcfg --convert-lang config/lang/legacy/english.mad \
       --lang-out-dir config/lang --merge-only
```

For the full CLI reference, see [maxcfg Command-Line Reference](maxcfg-cli-usage.md).

---

## Keeping Your Changes Across Upgrades

When a new release ships, it may include an updated `english.toml` with new
strings, fixes, or metadata. Here's how to upgrade without losing your work:

**If you've only changed a few strings:**

1. Note which keys you customized.
2. Replace `english.toml` with the one from the release.
3. Re-apply your changes to the new file.

**If you've heavily customized your language file:**

1. Keep your `english.toml` as-is.
2. Replace `delta_english.toml` with the one from the release.
3. Re-apply the delta in merge-only mode to pick up new param metadata without
   touching your colors:

   ```bash
   maxcfg --apply-delta config/lang/english.toml --merge-only
   ```

4. If you later want the MaximusNG theme, opt in:

   ```bash
   maxcfg --apply-delta config/lang/english.toml --ng-only
   ```

**If you migrated from a custom `.mad` file:**

1. Re-run the converter with `--merge-only` to get param enrichments without
   overwriting your color choices:

   ```bash
   maxcfg --convert-lang config/lang/legacy/english.mad \
          --lang-out-dir config/lang --merge-only
   ```

2. Then re-apply your customizations on top.

**If you haven't customized anything:** just replace both files from the
release and restart. Easy.

The delta system is designed so that official fixes and metadata enrichments
ship separately from the base language file. Your custom `english.toml` is
always yours—Maximus won't overwrite it during a normal install.

---

## Creating a New Language

Want to run your board in another language? Here's how:

1. Copy `english.toml` to `german.toml` (or whatever you like).
2. Translate the strings.
3. Update the metadata:

   ```toml
   [meta]
   name = "German"
   version = 1
   ```

4. Reference it in your configuration:

   ```toml
   # In config/general/language.toml
   lang_file = ["german"]
   ```

5. Optionally create a `delta_german.toml` for your own overrides.

The `[_legacy_map]` section should be identical across languages—it maps
numeric IDs to key names, not to translated text.

---

## Quick Reference

| Task | How |
|------|-----|
| Change a prompt | Edit `english.toml`, restart BBS |
| Add a color | Insert `\|NN` color code (see [display codes](using-display-codes.md)) |
| Reorder parameters | Swap `\|!1` and `\|!2` (keep suffixes) |
| Edit via maxcfg | Content → Language Strings (see [editor guide](using-maxcfg-langed.md)) |
| Add custom strings | See [Extending Languages](extending-lang-mex-c.md) |
| Convert from legacy | See [Conversion HOWTO](language-conversion-howto.md) |
| CLI reference | See [maxcfg Command-Line Reference](maxcfg-cli-usage.md) |
