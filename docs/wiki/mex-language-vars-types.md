---
layout: default
title: "Variables & Types"
section: "mex"
description: "Every data type MEX supports — primitives, strings, arrays, structs, and how to convert between them"
---

Every piece of data in a MEX script lives in a variable, and every variable
has a type. MEX is statically typed — the compiler needs to know what kind of
data you're working with before it'll let you do anything with it. No
surprises at runtime because you accidentally added a string to an integer.

If you're coming from C, the type system will feel familiar with a few
Pascal-flavored twists: declarations use a colon instead of prefix notation,
arrays have explicit bounds, and strings are dynamic (no buffer sizing, no
null terminators, no tears). If you're not coming from C, don't worry — the
[lessons]({{ site.baseurl }}{% link mex-learning.md %}) already walked you through most of this
by building real things. This page is the complete reference.

---

## On This Page

- [Quick Reference](#quick-reference)
- [Primitive Types](#primitive-types)
- [Declaration Syntax](#declaration-syntax)
- [Characters & Escape Sequences](#characters)
- [Strings](#strings)
- [Type Conversions](#type-conversions)
- [Arrays](#arrays)
- [Structs](#structs)
- [The User Record (`usr`)](#user-record)
- [Global Data](#global-data)

---

## Quick Reference {#quick-reference}

### Primitive Types

| Type | Size | Range | Notes |
|------|------|-------|-------|
| `char` | 1 byte | 0–255 | Single character or small unsigned integer |
| `int` | 2 bytes | −32,768 to 32,767 | Signed 16-bit integer |
| `unsigned int` | 2 bytes | 0 to 65,535 | Unsigned 16-bit integer |
| `long` | 4 bytes | −2,147,483,648 to 2,147,483,647 | Signed 32-bit integer |
| `unsigned long` | 4 bytes | 0 to 4,294,967,295 | Unsigned 32-bit integer |
| `string` | dynamic | n/a | Dynamic-length string — grows and shrinks automatically |

### Conversion Functions

| Function | Direction | Example |
|----------|-----------|---------|
| [`itostr()`](#itostr) | `int` → `string` | `itostr(42)` → `"42"` |
| [`uitostr()`](#uitostr) | `unsigned int` → `string` | `uitostr(65000)` → `"65000"` |
| [`ltostr()`](#ltostr) | `long` → `string` | `ltostr(100000)` → `"100000"` |
| [`ultostr()`](#ultostr) | `unsigned long` → `string` | `ultostr(3000000)` → `"3000000"` |
| [`strtoi()`](#strtoi) | `string` → `int` | `strtoi("42")` → `42` |
| [`strtol()`](#strtol) | `string` → `long` | `strtol("100000")` → `100000` |

---

## Primitive Types {#primitive-types}

MEX has five primitive types. They're all integer-family — there are no
floating-point numbers. If you need decimal math, you'll have to fake it
with fixed-point arithmetic (multiply by 100, do your math, divide at
display time).

**`char`** — A single byte. Can hold a character (`'A'`) or a small number
(0–255). Characters are unsigned by default. Use `char` for flags, loop
counters over small ranges, and individual keypress values.

**`int`** — A signed 16-bit integer. This is the workhorse type for counters,
indices, return codes, and most numeric work. Range: −32,768 to 32,767.

**`unsigned int`** — The unsigned variant. Range: 0 to 65,535. Used in the
user record for fields like `usr.priv`, `usr.times`, and `usr.credit`.

**`long`** — A signed 32-bit integer. Use for message numbers, file sizes,
timestamps, and anything that might exceed 32,767.

**`unsigned long`** — The unsigned variant. Range: 0 to 4,294,967,295. Used
for `usr.up`, `usr.down`, `time()`, and byte counts.

**`string`** — A dynamic-length character string. No fixed buffer, no null
terminator to manage. Strings grow when you concatenate, shrink when you
reassign, and the runtime handles all the memory. See [Strings](#strings)
for the full story.

---

## Declaration Syntax {#declaration-syntax}

MEX declarations use a colon between the type and the variable name — a
Pascal convention that reads naturally once you get used to it:

```mex
int: count;
string: name;
long: file_size;
```

Multiple variables of the same type on one line:

```mex
int: i, j, k;
string: first, last;
```

All numeric variables are initialized to `0`. All strings are initialized to
`""` (the empty string). You don't have to set them before use, but it's
good practice.

Assignment uses `:=` (not `=` — that's comparison):

```mex
count := 10;
name := "Sysop";
file_size := 1024 * 1024;
```

---

## Characters & Escape Sequences {#characters}

Character literals use single quotes: `'A'`, `'7'`, `'\n'`. You can also
use hex values for CP437 characters: `0xCD` for the double horizontal line
(`═`).

### Escape Sequences

| Sequence | Value | Meaning |
|----------|-------|---------|
| `\n` | 10 | Newline (line feed) |
| `\r` | 13 | Carriage return |
| `\t` | 9 | Tab |
| `\b` | 8 | Backspace |
| `\f` | 12 | Formfeed (clears screen) |
| `\\` | 92 | Literal backslash |
| `\"` | 34 | Literal double quote |
| `\a` | 7 | Bell (beep) |

These work in both character literals (`'\n'`) and string literals
(`"Hello\n"`).

---

## Strings {#strings}

Strings are dynamic. You never declare a buffer size — just use them:

```mex
string: greeting;
greeting := "Welcome to " + prm_string(0) + "!";
```

**Concatenation** uses the `+` operator:

```mex
string: full_name;
full_name := usr.name + " (" + usr.alias + ")";
```

**Character indexing** uses `[]` with 1-based positions:

```mex
string: s;
char: ch;

s := "Hello";
ch := s[1];      // ch = 'H'
s[5] := '!';     // s = "Hell!"
```

**String length** with `strlen()`:

```mex
if (strlen(usr.city) = 0)
  print("You haven't set your city yet.\n");
```

The full set of string functions is covered in
[String Operations]({{ site.baseurl }}{% link mex-language-string-ops.md %}).

---

## Type Conversions {#type-conversions}

MEX does not implicitly convert between numeric types and strings. If you
pass an `int` where a `string` is expected, the compiler will reject it.
You need explicit conversion functions.

This is the gotcha from [Lesson 3]({{ site.baseurl }}{% link mex-learn-user-record.md %}) —
if your function takes a `string` parameter and you hand it `usr.times`
(an `unsigned int`), you need `uitostr()`:

```mex
print("Calls: " + uitostr(usr.times) + "\n");
```

{: #itostr}
### itostr(int) → string

Converts a signed `int` to its string representation.

```mex
string: s;
s := itostr(-42);   // s = "-42"
```

{: #uitostr}
### uitostr(unsigned int) → string

Converts an `unsigned int` to its string representation.

```mex
print("Priv level: " + uitostr(usr.priv) + "\n");
```

{: #ltostr}
### ltostr(long) → string

Converts a signed `long` to its string representation.

```mex
print("Messages posted: " + ltostr(usr.msgs_posted) + "\n");
```

{: #ultostr}
### ultostr(unsigned long) → string

Converts an `unsigned long` to its string representation.

```mex
print("Downloaded: " + ultostr(usr.down) + " KB\n");
```

{: #strtoi}
### strtoi(string) → int

Parses a string as a signed `int`. Returns 0 if the string can't be parsed.

```mex
int: num;
num := strtoi("42");
```

{: #strtol}
### strtol(string) → long

Parses a string as a signed `long`.

```mex
long: big_num;
big_num := strtol("1000000");
```

### Numeric Casts

For conversions between numeric types (e.g., `int` to `long`), use a C-style
cast:

```mex
long: big;
int: small;

small := 42;
big := (long)small;
```

---

## Arrays {#arrays}

Arrays use a Pascal-style declaration with explicit lower and upper bounds:

```mex
array [1..100] of int: scores;
array [0..9] of string: names;
```

The bounds are inclusive — `[1..100]` gives you 100 elements, indexed 1
through 100. The lower bound doesn't have to be 1 (or even 0), though
0-based arrays generate slightly faster code.

**Accessing elements:**

```mex
scores[1] := 95;
scores[2] := 87;
print("First score: " + itostr(scores[1]) + "\n");
```

**Looping over an array:**

```mex
int: i;
for (i := 1; i <= 100; i := i + 1)
  scores[i] := 0;
```

**Out-of-bounds access** is undefined behavior — no runtime check, no error
message, just garbage. Be careful with your indices.

**Arrays of structs** work too — this is how you build lightbar menus:

```mex
array [1..5] of string: items;
items[1] := "Read Messages";
items[2] := "File Areas";
items[3] := "Goodbye";
```

---

## Structs {#structs}

Structs group related data into a single variable. They're declared with
the `struct` keyword:

```mex
struct _stamp
{
  struct _date: date;
  struct _time: time;
};
```

Access fields with the dot operator:

```mex
struct _stamp: now;
timestamp(now);
print("Hour: " + itostr(now.time.hh) + "\n");
```

You typically won't define your own structs — the headers (`max.mh`,
`maxui.mh`) declare all the struct types you need. But you will *use* them
constantly, especially the user record and the style structs for UI widgets.

**Passing structs to functions** — structs are passed by value by default.
Use `ref` if the function needs to modify the struct or if the struct is
a style that gets filled in by a default-initializer:

```mex
struct ui_edit_field_style: es;
ui_edit_field_style_default(es);   // takes ref, fills in defaults
```

---

## The User Record (`usr`) {#user-record}

The `usr` global is the most-used struct in MEX. It's a `struct _usr` that
contains everything Maximus knows about the current caller. It's always
available — no function call needed.

### Identity & Contact

| Field | Type | Description |
|-------|------|-------------|
| `usr.name` | `string` | Caller's real name |
| `usr.alias` | `string` | Caller's alias/handle |
| `usr.city` | `string` | City and state/province |
| `usr.phone` | `string` | Voice phone number |
| `usr.dataphone` | `string` | Data phone number |
| `usr.pwd` | `string` | Password (encrypted if `usr.encrypted` is set) |
| `usr.dob` | `string` | Date of birth (`yyyy.mm.dd`) |
| `usr.sex` | `char` | `SEX_UNKNOWN` (0), `SEX_MALE` (1), `SEX_FEMALE` (2) |

### Activity & Stats

| Field | Type | Description |
|-------|------|-------------|
| `usr.times` | `unsigned int` | Total previous calls |
| `usr.call` | `unsigned int` | Calls today |
| `usr.msgs_posted` | `long` | Messages posted (lifetime) |
| `usr.msgs_read` | `long` | Messages read (lifetime) |
| `usr.up` | `unsigned long` | KB uploaded (lifetime) |
| `usr.down` | `unsigned long` | KB downloaded (lifetime) |
| `usr.downtoday` | `unsigned long` | KB downloaded today |
| `usr.nup` | `unsigned long` | Files uploaded (lifetime) |
| `usr.ndown` | `unsigned long` | Files downloaded (lifetime) |
| `usr.ndowntoday` | `unsigned long` | Files downloaded today |
| `usr.time` | `unsigned int` | Minutes online today |

### Preferences & Terminal

| Field | Type | Description |
|-------|------|-------------|
| `usr.priv` | `unsigned int` | Privilege level |
| `usr.help` | `char` | Help level: `HELP_NOVICE` (6), `HELP_REGULAR` (4), `HELP_EXPERT` (2) |
| `usr.video` | `char` | Terminal: `VIDEO_TTY` (0), `VIDEO_ANSI` (1), `VIDEO_AVATAR` (2) |
| `usr.width` | `char` | Screen width |
| `usr.len` | `char` | Screen height |
| `usr.hotkeys` | `char` | 1 = hotkeys enabled |
| `usr.fsr` | `char` | 1 = full-screen reader enabled |
| `usr.more` | `char` | 1 = "More?" prompts enabled |
| `usr.cls` | `char` | 1 = can handle clear-screen |
| `usr.ibmchars` | `char` | 1 = can handle IBM (high-bit) characters |
| `usr.rip` | `char` | 1 = RIP graphics enabled |
| `usr.tabs` | `char` | 1 = can handle tab characters |
| `usr.bored` | `char` | 1 = BORED editor, 0 = MaxEd |
| `usr.lang` | `char` | Current language index |
| `usr.def_proto` | `signed char` | Default protocol (see `PROTOCOL_*` constants) |
| `usr.compress` | `char` | Default compression program |

### Dates & Expiration

| Field | Type | Description |
|-------|------|-------------|
| `usr.ludate` | `struct _stamp` | Date/time of last call |
| `usr.date_1stcall` | `struct _stamp` | Date of first call |
| `usr.date_pwd_chg` | `struct _stamp` | Date of last password change |
| `usr.date_newfile` | `struct _stamp` | Date of last new-files search |
| `usr.xp_date` | `struct _stamp` | Subscription expiration date |
| `usr.xp_mins` | `unsigned long` | Minutes remaining before expiration |
| `usr.xp_priv` | `unsigned int` | Privilege to demote to on expiration |
| `usr.expdate` | `char` | 1 = expire based on date |
| `usr.expmins` | `char` | 1 = expire based on minutes |
| `usr.expdemote` | `char` | 1 = demote on expiration |
| `usr.expaxe` | `char` | 1 = hang up on expiration |

### Other

| Field | Type | Description |
|-------|------|-------------|
| `usr.lastread_ptr` | `unsigned int` | Offset in lastread database |
| `usr.xkeys` | `string` | Access keys (as a string) |
| `usr.credit` | `unsigned int` | NetMail credit (cents) |
| `usr.debit` | `unsigned int` | NetMail debit (cents) |
| `usr.point_credit` | `unsigned long` | Point credits |
| `usr.point_debit` | `unsigned long` | Point debits |
| `usr.time_added` | `unsigned int` | Bonus time credits for today |
| `usr.msg` | `string` | Current message area tag |
| `usr.files` | `string` | Current file area tag |
| `usr.deleted` | `char` | 1 = user record is deleted |
| `usr.permanent` | `char` | 1 = user is permanent (never expires) |
| `usr.notavail` | `char` | 1 = not available for chat |
| `usr.nerd` | `char` | 1 = cannot yell for sysop |
| `usr.noulist` | `char` | 1 = hidden from user list |
| `usr.badlogon` | `char` | 1 = last logon attempt was bad |
| `usr.configured` | `char` | 1 = registration fields filled in |
| `usr.nulls` | `char` | Number of NUL characters after CR |

---

## Global Data {#global-data}

Beyond `usr`, MEX exports several other global variables that are always
available:

| Variable | Type | Description |
|----------|------|-------------|
| `input` | `string` | Current stacked input string |
| `id` | `struct _instancedata` | Session info: task number, baud rate, local flag |
| `marea` | `struct _marea` | Current message area (name, path, tag, attributes) |
| `farea` | `struct _farea` | Current file area (name, paths, attributes) |
| `msg` | `struct _msg` | Current message (number, high, count, direction) |
| `sys` | `struct _sys` | System info (cursor position, more-line count) |

These are read-only snapshots — modifying `marea.name` doesn't change the
actual area. Use the area selection functions (`msgareaselect`,
`fileareaselect`) to actually change areas.

---

## See Also

- [Control Flow]({{ site.baseurl }}{% link mex-language-control-flow.md %}) — `if`, `while`,
  `for`, and friends
- [Functions & Scope]({{ site.baseurl }}{% link mex-language-functions-scope.md %}) — declaring
  functions, `ref` parameters, scope rules
- [String Operations]({{ site.baseurl }}{% link mex-language-string-ops.md %}) — every string
  function
- [User & Session Intrinsics]({{ site.baseurl }}{% link mex-intrinsics-user-session.md %}) —
  functions that operate on the user record and session
