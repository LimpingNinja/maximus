---
layout: default
title: "Control Flow"
section: "mex"
description: "Branching, looping, and the operators that drive decisions in MEX"
---

Every interesting script makes decisions. Should the caller see the sysop
menu? Keep looping until they pick a valid option? Bail out early if they
hit Escape? This page covers the constructs that control the flow of
execution — and the operators and expressions that feed them.

If you've written C, most of this is second nature. The big gotcha:
**`=` is comparison, `:=` is assignment.** MEX borrowed its assignment
operator from Pascal. Once that's in muscle memory, everything else follows.

---

## On This Page

- [Quick Reference](#quick-reference)
- [Operators](#operators)
- [if / else](#if-else)
- [while](#while)
- [do / while](#do-while)
- [for](#for)
- [goto](#goto)
- [Blocks and Braces](#blocks)

---

## Quick Reference {#quick-reference}

### Statements

| Statement | Purpose | Example |
|-----------|---------|---------|
| [`if / else`](#if-else) | Conditional branch | `if (x = 1) ... else ...` |
| [`while`](#while) | Loop while condition is true | `while (running) { ... }` |
| [`do / while`](#do-while) | Loop at least once, then test | `do { ... } while (again);` |
| [`for`](#for) | Counted loop | `for (i := 1; i <= 10; i := i + 1)` |
| [`goto`](#goto) | Jump to a label | `goto done;` |

### Operators

| Category | Operators |
|----------|----------|
| Arithmetic | `+` `-` `*` `/` `%` (modulo) |
| Comparison | `=` `<>` `<` `>` `<=` `>=` |
| Logical | `and` `or` `not` |
| Bitwise | `&` `\|` `^` `~` `shl` `shr` |
| Assignment | `:=` |
| String | `+` (concatenation) |

---

## Operators {#operators}

### Arithmetic

The usual five. All operate on integer types (`int`, `long`, `char`,
and their unsigned variants). You cannot do arithmetic on strings — `+`
between strings is concatenation, not addition.

```mex
int: a, b, result;
a := 17;
b := 5;

result := a + b;    // 22
result := a - b;    // 12
result := a * b;    // 85
result := a / b;    // 3  (integer division, truncates)
result := a % b;    // 2  (modulo — remainder)
```

Division by zero is undefined — don't do it.

### Comparison

All comparisons return an integer: `1` for true, `0` for false. Note that
equality is `=` (single equals), **not** `==`:

```mex
if (choice = 3)          // equal
if (choice <> 3)         // not equal
if (usr.priv >= 100)     // greater than or equal
if (count < max)         // less than
```

String comparisons work with `=` and `<>`:

```mex
if (usr.name = "Sysop")
  print("Welcome back, boss.\n");
```

### Logical

`and`, `or`, and `not` — spelled out, not symbolic. They work on integer
values: zero is false, anything non-zero is true.

```mex
if (usr.priv >= 100 and usr.hotkeys = 1)
  print("Power user detected.\n");

if (not carrier())
  return;

if (choice = 'q' or choice = 'Q')
  return;
```

MEX does **not** short-circuit — both sides of `and`/`or` are always
evaluated. Keep that in mind if one side has side effects.

### Bitwise

For flag manipulation and low-level work:

| Operator | Operation |
|----------|-----------|
| `&` | Bitwise AND |
| `\|` | Bitwise OR |
| `^` | Bitwise XOR |
| `~` | Bitwise NOT (complement) |
| `shl` | Shift left |
| `shr` | Shift right |

```mex
// Check if a message area allows private messages
if (marea.attribs & MA_PVT)
  print("Private messages are allowed here.\n");

// Set a flag
int: flags;
flags := flags | UI_SCROLL_REGION_SHOW_SCROLLBAR;
```

### Assignment

Assignment is `:=`, not `=`. This is the number one source of confusion
for C programmers:

```mex
count := count + 1;     // correct: assignment
if (count = 10)         // correct: comparison
```

There is no `+=`, `-=`, `++`, or `--`. Increment is always
`i := i + 1`.

### Operator Precedence

From highest to lowest:

1. `not`, `~` (unary)
2. `*`, `/`, `%`
3. `+`, `-`
4. `shl`, `shr`
5. `<`, `>`, `<=`, `>=`
6. `=`, `<>`
7. `&`
8. `^`
9. `|`
10. `and`
11. `or`

When in doubt, use parentheses. They cost nothing and make intent clear.

---

## if / else {#if-else}

The basic branch. The condition must be in parentheses:

```mex
if (usr.video = VIDEO_ANSI)
  print("ANSI detected.\n");
```

With an `else` clause:

```mex
if (usr.hotkeys)
  print("Hotkeys are on.\n");
else
  print("Hotkeys are off.\n");
```

**Chaining** — MEX doesn't have `else if` as a single token, but you can
chain them naturally:

```mex
if (choice = 1)
  run_messages();
else if (choice = 2)
  run_files();
else if (choice = 3)
  run_goodbye();
else
  print("Invalid choice.\n");
```

**Blocks** — for multiple statements, use braces:

```mex
if (usr.priv >= 100)
{
  print("Sysop menu available.\n");
  show_sysop_menu();
}
```

---

## while {#while}

Loops while the condition is true. Tests **before** each iteration:

```mex
int: tries;
string: answer;

tries := 0;
while (tries < 3)
{
  print("Password: ");
  input_str(answer, INPUT_NLB_LINE, 0, 20, "");
  if (answer = secret)
    goto ok;
  tries := tries + 1;
}
print("Too many attempts.\n");
return;

ok:
print("Access granted.\n");
```

**Infinite loop** — `while (1)` is the standard pattern for menu loops.
Break out with `return` or `goto`:

```mex
while (1)
{
  choice := show_menu();
  if (choice = 'Q')
    return;
  handle_choice(choice);
}
```

---

## do / while {#do-while}

Like `while`, but the body executes **at least once** — the test is at the
bottom:

```mex
int: guess;
string: buf;

do
{
  print("Pick a number (1-10): ");
  input_str(buf, INPUT_NLB_LINE, 0, 2, "");
  guess := strtoi(buf);
}
while (guess < 1 or guess > 10);
```

This is handy when you need to get at least one input before you can test
whether it's valid.

---

## for {#for}

A counted loop with initialization, condition, and increment — all in one
line:

```mex
int: i;
for (i := 1; i <= 10; i := i + 1)
  print(itostr(i) + "\n");
```

The three parts are separated by semicolons:
1. **Init:** `i := 1` — runs once before the loop starts
2. **Condition:** `i <= 10` — tested before each iteration
3. **Increment:** `i := i + 1` — runs after each iteration

You can use `for` with any increment:

```mex
// Count down
for (i := 10; i >= 1; i := i - 1)
  print(itostr(i) + "... ");

// Skip by twos
for (i := 0; i <= 20; i := i + 2)
  print(itostr(i) + " ");
```

---

## goto {#goto}

Jumps to a labeled statement. Labels are identifiers followed by a colon:

```mex
if (not carrier())
  goto cleanup;

// ... do work ...

cleanup:
  close(fd);
```

`goto` gets a bad reputation, but in MEX it's genuinely useful for two
patterns:

1. **Error cleanup** — when you have resources to release (open files,
   created regions) and multiple bail-out points
2. **Breaking out of nested loops** — MEX has no `break` or `continue`
   statement, so `goto` is the only way to exit a `while` inside a `while`

```mex
int: i, j;
for (i := 1; i <= 10; i := i + 1)
{
  for (j := 1; j <= 10; j := j + 1)
  {
    if (grid[i][j] = target)
      goto found;
  }
}
print("Not found.\n");
return;

found:
print("Found at " + itostr(i) + "," + itostr(j) + "\n");
```

Keep `goto` jumps forward and local. Don't jump into the middle of a loop
or between functions — the compiler won't stop you, but the results won't
be pretty.

---

## Blocks and Braces {#blocks}

Anywhere a single statement is expected, you can use a block — a pair of
braces containing multiple statements:

```mex
if (condition)
{
  statement1;
  statement2;
  statement3;
}
```

Variables declared inside a block are **local to that block** — they're
created on entry and destroyed on exit:

```mex
if (need_temp)
{
  string: temp;
  temp := "scratch";
  // temp exists here
}
// temp does not exist here
```

This is useful for keeping temporary variables from cluttering the enclosing
scope.

---

## See Also

- [Variables & Types]({% link mex-language-vars-types.md %}) — what you're
  comparing and computing with
- [Functions & Scope]({% link mex-language-functions-scope.md %}) — organizing
  code into callable pieces
- [String Operations]({% link mex-language-string-ops.md %}) — string
  comparisons and manipulation
