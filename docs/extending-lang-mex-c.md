# Extending Languages via MEX and C

Maximus NG lets you go beyond editing the built-in language strings. If you are
writing a MEX door, building a custom module, or just want to keep your own
strings organized and translatable, the language system has you covered.

This guide shows how to register custom strings at runtime, ship your own
language TOML files, and use the language API from both MEX and C.

---

## Why Use the Language System?

You could hard-code strings directly in your MEX script or C module. But using
the language system gives you:

- **Translation support** — sysops running non-English boards can translate
  your strings without touching your code.
- **Central management** — all strings live in TOML files that can be browsed
  and edited in maxcfg.
- **Consistency** — your door or module uses the same display code and
  parameter system as the rest of the BBS.

---

## MEX: Registering Strings at Runtime

The simplest approach for a MEX door is to register your strings when the door
starts, use them during execution, and clean up when done.

### Registering a Single String

```c
lang_register("my_door", "welcome", "|15Welcome to My Door, |!1!\n");
lang_register("my_door", "goodbye", "|14Thanks for playing!\n");
lang_register("my_door", "score",   "|07Your score: $R06|!1d\n");
```

Each call registers one string under a **namespace** (`"my_door"` in this
case). The namespace is created automatically on first use.

Once registered, strings are accessible as `"namespace.key"`:

```c
string name = "Kevin";
string s = lang_get("my_door.welcome");
print(s);
```

### Retrieving Strings

```c
string s = lang_get("my_door.score");
```

Returns the string value, or an empty string if the key is not found. You can
also retrieve strings from the built-in language file:

```c
string prompt = lang_get("global.press_enter");
print(prompt);
```

### RIP Alternates

If your door supports RIP graphics terminals, you can retrieve the RIP
alternate for any string:

```c
string rip_ver = lang_get_rip("global.press_enter_s");
if (rip_ver <> "")
    print(rip_ver);
```

Returns an empty string if no RIP alternate is defined.

### Cleaning Up

When your door exits, unregister the namespace to free memory:

```c
lang_unregister("my_door");
```

This removes all strings registered under that namespace. It is good practice
but not strictly required — the runtime cleans up when the session ends.

### Complete MEX Example

```c
#include <max.mh>

void main()
{
    // Register our strings
    lang_register("trivia", "welcome",  "|11=== Trivia Challenge ===|07\n\n");
    lang_register("trivia", "question", "|15Question |!1d of |!2d:|07 |!3\n");
    lang_register("trivia", "correct",  "|10Correct!|07 +|!1d points.\n");
    lang_register("trivia", "wrong",    "|12Wrong!|07 The answer was: |!1\n");
    lang_register("trivia", "final",    "\n|14Final score: |15|!1d|14 points.\n");

    // Use them
    print(lang_get("trivia.welcome"));

    // ... game logic ...

    // Clean up
    lang_unregister("trivia");
}
```

---

## MEX: Loading Extension TOML Files

For larger projects, you probably do not want to hard-code every string in your
MEX source. Instead, ship a TOML file and load it at runtime.

### Creating an Extension File

Create a `.toml` file in `config/lang/ext/` (or wherever you like):

```toml
# config/lang/ext/trivia.toml

[meta]
name = "trivia"
version = 1

[trivia]
welcome = "|11=== Trivia Challenge ===|07\n\n"
question = { text = "|15Question |!1d of |!2d:|07 |!3\n", params = [{name = "current", type = "int"}, {name = "total", type = "int"}, {name = "text", type = "string"}] }
correct = { text = "|10Correct!|07 +|!1d points.\n", params = [{name = "points", type = "int"}] }
wrong = { text = "|12Wrong!|07 The answer was: |!1\n", params = [{name = "answer", type = "string"}] }
final = { text = "\n|14Final score: |15|!1d|14 points.\n", params = [{name = "score", type = "int"}] }
```

The file format is identical to the main language file. Each table becomes an
accessible namespace.

### Loading It from MEX

```c
if (lang_load_extension("ext/trivia.toml") = 0)
    print("Warning: could not load trivia language file\n");
```

If the path is relative (does not start with `/`), it is resolved against the
configured language directory (`config/lang/`). After loading, all heaps in the
file are accessible via `lang_get()`:

```c
string s = lang_get("trivia.welcome");
print(s);
```

### Why Extension Files?

- **Sysops can translate them** without modifying your MEX code.
- **Sysops can customize them** — change wording, colors, formatting.
- **They show up in maxcfg** once loaded, so the sysop can browse them
  alongside the built-in strings.
- **Multiple languages** — ship `ext/trivia.toml` for English and
  `ext/trivia_de.toml` for German, load the right one based on the user's
  language preference.

---

## MEX API Reference

| Function | Description |
|----------|-------------|
| `string lang_get(string key)` | Get a string by dotted key (e.g. `"global.located"`). Returns `""` if not found. |
| `string lang_get_rip(string key)` | Get the RIP alternate for a string. Returns `""` if none. |
| `int lang_register(string ns, string key, string value)` | Register a single string under a namespace. Returns 1 on success. |
| `void lang_unregister(string ns)` | Remove all strings in a namespace. |
| `int lang_load_extension(string path)` | Load an extension TOML file. Returns 1 on success. |

### Legacy API (Still Works)

The old `lstr()` and `hstr()` functions continue to work during the migration
period. They resolve through the `[_legacy_map]` in the TOML file:

```c
string s = lstr(42);   // resolves to the TOML key mapped from numeric ID 42
```

New code should use `lang_get()` with dotted keys instead.

---

## C: Using the Language API

If you are writing a C module or modifying the BBS core, the language system is
available through the `maxlang` API in libmaxcfg.

### Getting Strings

```c
#include "maxlang.h"

/* g_current_lang is the global language handle, set at startup */
const char *msg = maxlang_get(g_current_lang, "global.press_enter");
```

Returns a pointer to the string (valid until the language handle is closed), or
`""` if the key is not found.

### Printing with Parameters

Use `LangPrintf()` to print a language string with positional parameter
substitution:

```c
LangPrintf(s_ret(LOCATED), match_count, plural_suffix);
```

Or with the new string-keyed API:

```c
const char *fmt = maxlang_get(g_current_lang, "global.located");
LangPrintf(fmt, match_count, plural_suffix);
```

`LangPrintf()` detects `|!N` positional parameters and binds your varargs to
the correct slots, respecting type suffixes. If no `|!N` codes are found, it
falls back to classic `printf()` behavior for backward compatibility.

### Registering Runtime Strings from C

```c
const char *keys[]   = { "welcome", "goodbye" };
const char *values[] = { "|15Welcome!\n", "|14Goodbye!\n" };

maxlang_register(g_current_lang, "my_module", keys, values, 2);

/* Later... */
const char *s = maxlang_get(g_current_lang, "my_module.welcome");
```

To remove the namespace:

```c
maxlang_unregister(g_current_lang, "my_module");
```

### Loading Extension Files from C

```c
MaxCfgStatus st = maxlang_load_extension(g_current_lang, "/path/to/ext/my_module.toml");
if (st != MAXCFG_OK) {
    logit("Failed to load my_module language file");
}
```

### C API Reference

```c
/* Lifecycle */
MaxCfgStatus maxlang_open(MaxCfg *cfg, const char *lang_name, MaxLang **out_lang);
void         maxlang_close(MaxLang *lang);

/* Retrieval */
const char  *maxlang_get(MaxLang *lang, const char *key);
const char  *maxlang_get_rip(MaxLang *lang, const char *key);
const char  *maxlang_get_by_id(MaxLang *lang, int strn);
bool         maxlang_has_flag(MaxLang *lang, const char *key, const char *flag);

/* RIP mode */
void         maxlang_set_use_rip(MaxLang *lang, bool use_rip);

/* Runtime registration */
MaxCfgStatus maxlang_register(MaxLang *lang, const char *ns,
                              const char **keys, const char **values, int count);
void         maxlang_unregister(MaxLang *lang, const char *ns);

/* Extension loading */
MaxCfgStatus maxlang_load_extension(MaxLang *lang, const char *path);

/* Display with parameter substitution */
int          LangPrintf(char *format, ...);
int          LangVsprintf(char *buf, size_t bufsz, const char *format, va_list ap);
```

---

## Best Practices

**Use descriptive key names.** `trivia.question_prompt` is better than
`trivia.q1`. Sysops reading the TOML file (or browsing in maxcfg) should be
able to tell what a string does from its name.

**Include params metadata.** If your extension TOML has parameterized strings,
add `params` arrays so the maxcfg editor can show sysops what each `|!N` slot
means.

**Pick a unique namespace.** Use your door or module name. Avoid generic names
like `custom` or `strings` that might collide with other extensions.

**Ship a delta file if appropriate.** If your extension evolves across versions,
you can ship a `delta_trivia.toml` that patches the base without requiring a
full replacement.

**Clean up in MEX.** Call `lang_unregister()` when your door exits. It is not
strictly required (the runtime handles cleanup at session end), but it is
polite — especially in a multi-door session where memory adds up.

---

## See Also

- [Using Language TOML Files](using-lang-toml.md) — format reference and
  customization guide
- [Using the maxcfg Language Editor](using-maxcfg-langed.md) — browsing and
  editing strings in the TUI
- [Using Display Codes](using-display-codes.md) — color codes, formatting
  operators, and information codes
- [Language Conversion HOWTO](language-conversion-howto.md) — converting legacy
  `.mad` files
