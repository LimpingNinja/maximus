---
layout: default
title: "String Operations"
section: "mex"
description: "Every string function MEX provides — concatenation, slicing, searching, padding, case conversion, and tokenizing"
---

Strings are everywhere in BBS scripting. You're building prompts, formatting
profile cards, parsing filenames, tokenizing user input, padding columns for
neat display. MEX gives you dynamic strings that handle their own memory and
a solid set of functions to work with them.

This page covers every string operation available in MEX — from the basics
(concatenation, length) to the functions you'll reach for when you need to
slice, search, pad, or recase. If you're looking for the numeric-to-string
conversion functions (`itostr`, `ltostr`, etc.), those live on the
[Variables & Types]({{ site.baseurl }}{% link mex-language-vars-types.md %}#type-conversions)
page since they're about type conversion rather than string manipulation.

---

## On This Page

- [Quick Reference](#quick-reference)
- [Concatenation](#concat)
- [Length](#strlen)
- [Substrings](#substr)
- [Searching](#searching)
- [Tokenizing](#tokenizing)
- [Padding & Alignment](#padding)
- [Case Conversion](#case)
- [Trimming](#trimming)
- [Character Access](#char-access)

---

## Quick Reference {#quick-reference}

| Function | Signature | What It Does |
|----------|-----------|-------------|
| [`strlen`](#strlen) | `unsigned int strlen(string: s)` | Returns the length of `s` |
| [`substr`](#substr) | `string substr(string: s, int: pos, int: length)` | Extracts a substring (1-based) |
| [`strfind`](#strfind) | `int strfind(string: str, string: substring)` | Finds first occurrence, returns position or 0 |
| [`stridx`](#stridx) | `int stridx(string: str, int: startpos, int: ch)` | Finds character forward from position |
| [`strridx`](#strridx) | `int strridx(string: str, int: startpos, int: ch)` | Finds character backward from position |
| [`strtok`](#strtok) | `string strtok(string: src, string: toks, ref int: pos)` | Extracts next token |
| [`strpad`](#strpad) | `string strpad(string: str, int: length, char: pad)` | Right-pads to length |
| [`strpadleft`](#strpadleft) | `string strpadleft(string: str, int: length, char: pad)` | Left-pads to length |
| [`strupper`](#strupper) | `string strupper(string: s)` | Converts to uppercase |
| [`strlower`](#strlower) | `string strlower(string: s)` | Converts to lowercase |
| [`strtrim`](#strtrim) | `string strtrim(string: s, string: x)` | Trims characters from both ends |

---

## Concatenation {#concat}

The `+` operator joins strings together. It's the same `+` used for
arithmetic on numbers — the compiler knows which one you mean based on the
types:

```mex
string: greeting;
greeting := "Hello, " + usr.name + "! Welcome to " + prm_string(0) + ".\n";
print(greeting);
```

You can chain as many `+` operations as you like. Each one produces a new
string — MEX handles all the memory automatically.

To concatenate a number with a string, convert it first:

```mex
print("You have " + uitostr(usr.times) + " calls on record.\n");
```

---

## Length {#strlen}

```mex
unsigned int strlen(string: s);
```

Returns the number of characters in `s`. An empty string has length 0.

```mex
if (strlen(usr.city) = 0)
  print("You haven't set your city yet.\n");
```

```mex
// Truncate a string to fit a column
string: display_name;
display_name := usr.name;
if (strlen(display_name) > 20)
  display_name := substr(display_name, 1, 20);
```

---

## Substrings {#substr}

```mex
string substr(string: s, int: pos, int: length);
```

Extracts `length` characters from `s`, starting at position `pos`.
Positions are **1-based** — the first character is position 1.

```mex
string: s;
s := "Hello, World!";

substr(s, 1, 5)     // "Hello"
substr(s, 8, 5)     // "World"
substr(s, 1, 1)     // "H"
```

If `pos + length` exceeds the string, you get whatever characters are
available — no error, no crash.

**Common pattern — strip a prefix:**

```mex
// Remove "Re: " from a subject line
string: subject;
subject := "Re: Testing 1 2 3";
if (strlen(subject) > 4)
{
  if (substr(subject, 1, 4) = "Re: ")
    subject := substr(subject, 5, strlen(subject) - 4);
}
```

---

## Searching {#searching}

{: #strfind}
### strfind — Find a Substring

```mex
int strfind(string: str, string: substring);
```

Returns the **1-based** position of the first occurrence of `substring` in
`str`, or `0` if not found.

```mex
int: pos;
pos := strfind(usr.city, "TX");
if (pos > 0)
  print("Howdy, fellow Texan!\n");
```

{: #stridx}
### stridx — Find a Character (Forward)

```mex
int stridx(string: str, int: startpos, int: ch);
```

Searches forward from `startpos` for character `ch`. Returns the 1-based
position, or `0` if not found. `startpos` is 1-based.

```mex
// Find the first space in a string
int: space_pos;
space_pos := stridx(usr.name, 1, ' ');
if (space_pos > 0)
  print("First name: " + substr(usr.name, 1, space_pos - 1) + "\n");
```

{: #strridx}
### strridx — Find a Character (Backward)

```mex
int strridx(string: str, int: startpos, int: ch);
```

Searches backward from `startpos` for character `ch`. If `startpos` is 0,
the search begins from the end of the string.

```mex
// Find the last dot in a filename
string: filename;
int: dot_pos;
filename := "archive.tar.gz";
dot_pos := strridx(filename, 0, '.');
if (dot_pos > 0)
  print("Extension: " + substr(filename, dot_pos + 1, strlen(filename) - dot_pos) + "\n");
```

---

## Tokenizing {#tokenizing}

{: #strtok}
### strtok — Split on Delimiters

```mex
string strtok(string: src, string: toks, ref int: pos);
```

Extracts the next token from `src`, splitting on any character in `toks`.
The `pos` variable tracks the current position — initialize it to `0`
before the first call. Returns an empty string when there are no more
tokens.

```mex
string: line, token;
int: pos;

line := "one,two,three,four";
pos := 0;

token := strtok(line, ",", pos);
while (strlen(token) > 0)
{
  print("Token: " + token + "\n");
  token := strtok(line, ",", pos);
}
```

Output:
```
Token: one
Token: two
Token: three
Token: four
```

You can split on multiple delimiter characters:

```mex
// Split on comma, space, or semicolon
token := strtok(input, ", ;", pos);
```

---

## Padding & Alignment {#padding}

{: #strpad}
### strpad — Right-Pad

```mex
string strpad(string: str, int: length, char: pad);
```

Returns `str` padded on the right with `pad` characters until it reaches
`length`. If `str` is already at least `length` characters, it's returned
unchanged.

```mex
// Right-pad a label to 20 characters
print(strpad("Name:", 20, '.') + " " + usr.name + "\n");
print(strpad("City:", 20, '.') + " " + usr.city + "\n");
```

Output:
```
Name:............... Kevin
City:............... Austin, TX
```

{: #strpadleft}
### strpadleft — Left-Pad

```mex
string strpadleft(string: str, int: length, char: pad);
```

Pads on the left. Great for right-aligning numbers:

```mex
// Right-align a number in a 6-character column
print(strpadleft(uitostr(usr.times), 6, ' ') + " calls\n");
```

### Utility Headers: intpad.mh

The `intpad.mh` header provides convenience wrappers that combine conversion
and padding in one call:

```mex
#include <intpad.mh>

// intpad(int, width)        — right-pad an int with spaces
// uintpad(uint, width)      — right-pad an unsigned int
// intpadleft(int, width, ch)  — left-pad an int with a character
// uintpadleft(uint, width, ch) — left-pad an unsigned int

print(intpadleft(42, 5, '0'));    // "00042"
print(intpad(42, 8));             // "42      "
```

---

## Case Conversion {#case}

{: #strupper}
### strupper — To Uppercase

```mex
string strupper(string: s);
```

Returns a copy of `s` with all lowercase letters converted to uppercase.
Non-letter characters are unchanged.

```mex
string: answer;
input_str(answer, INPUT_NLB_LINE, 0, 1, "Continue? [Y/N] ");
if (strupper(answer) = "Y")
  print("Continuing...\n");
```

{: #strlower}
### strlower — To Lowercase

```mex
string strlower(string: s);
```

Returns a copy of `s` with all uppercase letters converted to lowercase.

```mex
// Case-insensitive comparison
if (strlower(usr.city) = "austin, tx")
  print("Local caller!\n");
```

---

## Trimming {#trimming}

{: #strtrim}
### strtrim — Trim Characters

```mex
string strtrim(string: s, string: x);
```

Removes all characters in `x` from both the beginning and end of `s`.
Characters in the middle are not affected.

```mex
string: padded;
padded := "   Hello   ";
print("[" + strtrim(padded, " ") + "]");   // "[Hello]"
```

You can trim multiple character types at once:

```mex
// Strip leading/trailing whitespace and dots
string: clean;
clean := strtrim(raw_input, " .\t");
```

---

## Character Access {#char-access}

Individual characters in a string are accessed with `[]` using **1-based**
indexing:

```mex
string: s;
char: ch;

s := "Hello";
ch := s[1];      // 'H'
ch := s[5];      // 'o'
```

You can also assign to individual positions:

```mex
s[1] := 'h';     // s is now "hello"
```

**Building strings character by character:**

```mex
string: result;
int: i;
result := "";
for (i := 1; i <= strlen(source); i := i + 1)
{
  if (source[i] <> ' ')
    result := result + source[i];
}
```

Note that `s[i]` returns a `char`, and concatenating a `char` to a `string`
with `+` works — the char is promoted to a single-character string
automatically.

---

## See Also

- [Variables & Types]({{ site.baseurl }}{% link mex-language-vars-types.md %}) — type system,
  including numeric-to-string conversions
- [Display & I/O Intrinsics]({{ site.baseurl }}{% link mex-intrinsics-display-io.md %}) —
  `print()`, `input_str()`, and other I/O functions
- [Control Flow]({{ site.baseurl }}{% link mex-language-control-flow.md %}) — loops and
  conditionals for string processing
