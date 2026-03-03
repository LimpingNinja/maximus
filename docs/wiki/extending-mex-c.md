---
layout: default
section: "mex"
title: "Extending with MEX/C"
description: "Runtime language strings, TOML extensions, and the C language API"
permalink: /extending-mex-c/
---

Maximus ships with hundreds of language strings — every prompt, error
message, and status line the BBS displays. But your scripts and C modules
aren't limited to what ships in the box. You can register new strings at
runtime, ship extension TOML files with custom prompts, and call the
language API from both MEX and C.

This page covers the language extension system. For adding entirely new MEX
intrinsic *functions* (the `word EXPENTRY fn(void)` pattern), see
[Adding Intrinsics]({% link mex-adding-intrinsics.md %}).

---

## How Language Strings Work

Every displayable string in Maximus lives in the language system. At
startup, the BBS loads `english.toml` (or whichever language file is
configured), which maps **heap names** and **keys** to display strings:

```toml
[global]
press_enter = "|07Press |15ENTER|07 to continue..."
invalid_pwd = "|12Invalid password.|07"
```

Scripts and C code look up strings by a two-part identifier:
**heap** + **key**. The heap is the TOML section (`global`, `f_area`,
`m_area`, etc.) and the key is the name within that section.

The language system supports:

- **Pipe codes** (`|07`, `|15`, etc.) for color
- **Positional parameters** (`|!1`, `|!2s`, etc.) for runtime substitution
- **RIP alternates** for graphical terminal variants
- **Runtime registration** from MEX scripts and C modules

---

## MEX Language API

Include the standard header — the language functions are part of the core
intrinsics:

```mex
#include <max.mh>
```

### lang_get(heap, key)

Look up a language string by heap and key. Returns the raw string with pipe
codes intact (they're expanded when you `print` them).

```mex
string: msg;
msg := lang_get("global", "press_enter");
print(msg);
```

If the key doesn't exist, `lang_get` returns an empty string. No crash, no
error popup — just silence.

### lang_register(heap, key, value)

Register a new language string at runtime. This is how your scripts add
custom prompts without touching `english.toml`.

```mex
lang_register("trivia", "welcome", "|14Welcome to Trivia Night!|07\n");
lang_register("trivia", "correct", "|10Correct! |07+|151|07 point.\n");
lang_register("trivia", "wrong",   "|12Nope.|07 The answer was |15|!1|07.\n");
```

Registered strings:

- **Override** existing keys if the heap+key already exists
- **Persist** for the duration of the MEX session (cleared on script exit)
- **Support** pipe codes and positional parameters just like built-in strings
- **Are visible** to `lang_get` immediately after registration

### lang_unregister(heap, key)

Remove a previously registered string. The built-in value (if any) becomes
visible again.

```mex
lang_unregister("trivia", "welcome");
```

### lang_load_extension(filename)

Load a TOML file as a language extension. This is the batch version of
`lang_register` — useful when your script ships with a companion file of
custom prompts.

```mex
int: rc;
rc := lang_load_extension("trivia_lang.toml");
if (rc < 0)
  print("Warning: could not load trivia language file.\n");
```

The extension file uses the same format as `english.toml`:

```toml
# trivia_lang.toml — shipped alongside trivia.mex
[trivia]
welcome  = "|14Welcome to Trivia Night!|07\n"
correct  = "|10Correct! |07+|151|07 point.\n"
wrong    = "|12Nope.|07 The answer was |15|!1|07.\n"
question = "|11Q|!1: |15|!2|07\n"
score    = "|14Your score: |15|!1|07 / |15|!2|07\n"
```

The file is looked up relative to the scripts directory
(`$PREFIX/scripts/`). Returns `0` on success, `-1` on failure (file not
found, parse error).

Extension strings are merged into the live language table — they override
built-in strings with the same heap+key, and they persist for the session.

### Positional Parameters

Language strings support positional substitution with `|!N` codes:

```mex
lang_register("trivia", "score", "|14Score: |15|!1|07 out of |15|!2|07\n");

// Later, when printing:
string: msg;
msg := lang_get("trivia", "score");
// Use LangPrintf from C, or manual string replacement in MEX
```

The `|!1`, `|!2`, etc. markers are replaced at display time when the string
is passed through `LangPrintf` (C side) or when you do manual substitution
in MEX.

**Type suffixes** on positional codes (`|!1d`, `|!1l`, `|!1s`) tell the C
layer what type to pull from varargs. In MEX scripts, you typically
pre-format values as strings and use bare `|!1` codes.

### RIP Alternates

For boards supporting RIP graphics terminals, you can register alternate
versions of strings:

```mex
lang_register("trivia", "welcome",
              "|14Welcome to Trivia Night!|07\n");
lang_register("trivia", "welcome.rip",
              "!|1K@,0,0,639,349,0,0,10,Trivia Night\n");
```

The `.rip` suffix is a convention — the display layer checks for it
automatically when the caller's terminal supports RIP.

---

## C Language API

If you're writing C modules or modifying the Maximus source, the language
API lives in `maxlang.h` and `maxlang.c`.

### Looking Up Strings

```c
/**
 * @brief Look up a language string by heap and key.
 * @param heap  Section name (e.g., "global", "f_area")
 * @param key   Key within the section
 * @return Pointer to the string, or "" if not found
 */
const char *maxlang_get(const char *heap, const char *key);
```

The returned pointer is valid for the lifetime of the language table. Don't
free it.

### Formatted Output

```c
/**
 * @brief Print a language string with positional parameter substitution.
 * @param heap  Section name
 * @param key   Key within the section
 * @param ...   Values for |!1, |!2, etc. (type must match suffixes)
 *
 * Positional codes in the string are replaced with the provided arguments.
 * Type suffixes: |!1s = string, |!1d = int, |!1l = long, |!1c = char.
 * Bare |!1 = string (default).
 */
void LangPrintf(const char *heap, const char *key, ...);
```

**Important:** The type suffix in the TOML string must match what you pass
in varargs. If the TOML has `|!1l` (long), pass a `long`. If you
pre-format to a string, use bare `|!1` or `|!1s`.

### Registering Strings from C

```c
/**
 * @brief Register a runtime language string.
 * @param heap   Section name
 * @param key    Key within the section
 * @param value  The string value (copied internally)
 * @return 0 on success, -1 on error
 */
int maxlang_register(const char *heap, const char *key, const char *value);

/**
 * @brief Remove a runtime-registered string.
 * @param heap  Section name
 * @param key   Key within the section
 */
void maxlang_unregister(const char *heap, const char *key);
```

### Loading Extension Files from C

```c
/**
 * @brief Load a TOML extension file into the language table.
 * @param filename  Path relative to scripts directory
 * @return 0 on success, -1 on failure
 */
int maxlang_load_extension(const char *filename);
```

This is the same function the MEX `lang_load_extension` intrinsic calls
internally.

---

## Shipping Extension Files

If your script uses custom language strings, the cleanest approach is to
ship a companion TOML file:

```
scripts/
├── trivia.mex          # compiled script (.vm)
├── trivia_lang.toml    # language strings
└── include/
    └── max.mh
```

At the top of your script, load the extension:

```mex
void main()
{
  // Load our custom prompts
  lang_load_extension("trivia_lang.toml");

  // Now use them
  print(lang_get("trivia", "welcome"));
  // ...
}
```

**Benefits:**

- **Sysops can customize** your script's prompts by editing the TOML file
- **Translators can localize** by creating `trivia_lang_de.toml`, etc.
- **Your script code** stays clean — no hardcoded display strings
- **Pipe codes and MCI** work in extension strings just like built-in ones

---

## Best Practices

- **Use unique heap names.** Pick a heap name specific to your script
  (`trivia`, `weather`, `chatbot`) to avoid colliding with built-in heaps
  like `global`, `f_area`, or `m_area`.

- **Ship TOML extensions for anything user-visible.** Hardcoding strings in
  MEX source makes them impossible for sysops to customize. If a string
  will be seen by callers, put it in a TOML file.

- **Always check `lang_load_extension` return values.** If the file is
  missing, fall back to inline `lang_register` calls or hardcoded defaults
  so your script doesn't silently show blank prompts.

- **Keep pipe codes in the TOML, not in MEX.** Let the language file own
  all color formatting. This way sysops can restyle your script without
  touching code.

- **Use positional parameters for dynamic content.** Instead of string
  concatenation in MEX, use `|!1`, `|!2`, etc. in your TOML strings and
  substitute at display time. This keeps strings translatable.

---

## Complete Example: Trivia Door

A small but complete example showing the full pattern — extension file,
language lookups, and runtime registration:

**trivia_lang.toml:**

```toml
[trivia]
welcome  = "\n|14|17  Trivia Night!  |07|16\n\n"
question = "|11Question |!1:|07 |15|!2|07\n"
correct  = "|10Correct!|07 +1 point.\n"
wrong    = "|12Wrong.|07 The answer was |15|!1|07.\n"
score    = "\n|14Final score: |15|!1|07 out of |15|!2|07.\n"
goodbye  = "\n|07Thanks for playing!\n"
```

**trivia.mex:**

```mex
#include <max.mh>

void main()
{
  int:    rc;
  string: answer;

  rc := lang_load_extension("trivia_lang.toml");
  if (rc < 0)
  {
    // Fallback: register inline if TOML is missing
    lang_register("trivia", "welcome",  "\nTrivia Night!\n\n");
    lang_register("trivia", "correct",  "Correct! +1 point.\n");
    lang_register("trivia", "wrong",    "Wrong.\n");
    lang_register("trivia", "goodbye",  "\nThanks for playing!\n");
  }

  print(lang_get("trivia", "welcome"));

  // Ask a question
  print("|11Question 1:|07 |15What year was the IBM PC introduced?|07\n");
  input_str(answer, INPUT_WORD, 0, 10, "|11> |07");

  if (answer = "1981")
    print(lang_get("trivia", "correct"));
  else
    print(lang_get("trivia", "wrong"));

  print(lang_get("trivia", "goodbye"));
}
```

---

## Quick Reference

### MEX Functions

| Function | Returns | Purpose |
|----------|---------|---------|
| `lang_get(heap, key)` | string | Look up a language string |
| `lang_register(heap, key, value)` | — | Register a runtime string |
| `lang_unregister(heap, key)` | — | Remove a runtime string |
| `lang_load_extension(filename)` | 0 or -1 | Load a TOML extension file |

### C Functions

| Function | Returns | Purpose |
|----------|---------|---------|
| `maxlang_get(heap, key)` | `const char *` | Look up a language string |
| `maxlang_register(heap, key, value)` | `int` | Register a runtime string |
| `maxlang_unregister(heap, key)` | `void` | Remove a runtime string |
| `maxlang_load_extension(filename)` | `int` | Load a TOML extension file |
| `LangPrintf(heap, key, ...)` | `void` | Print with positional substitution |

---

## See Also

- [Adding Intrinsics]({% link mex-adding-intrinsics.md %}) — how to add
  new MEX functions backed by C code
- [Language Files (TOML)]({% link lang-toml.md %}) — the main language file
  format and built-in heaps
- [MEX Getting Started]({% link mex-getting-started.md %}) — introduction
  to MEX scripting
