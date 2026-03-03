---
layout: default
title: "Making Decisions: Choose Your Own Adventure"
section: "mex"
description: "Lesson 4 — If/else, conditions, and a BBS trivia game that keeps score"
---

*Lesson 4 of Learning MEX*

---

> **What you'll build:** A three-question BBS trivia game that checks
> answers, keeps score, and responds differently depending on how the
> caller does.

## 2:51 AM

Every script you've written so far runs in a straight line. Top to bottom.
Same path, same output, same experience for everyone. The greeter greets.
The profile card profiles. Nobody's making any *choices*.

But every interesting thing that's ever happened on a BBS involved a
choice. Press Y to continue. Pick a door. Answer the question. Challenge
your fate.

This lesson is about the fork in the road — the `if` statement and its
friends. By the end, you'll have a trivia game that asks questions, checks
answers, keeps score, and gives the caller a different sign-off depending
on how they did. Same script, different experience every time.

That's when scripting stops being output and starts being *interactive*.

## The if Statement

You've already used `if` a few times — checking for empty strings, testing
`usr.times`. Here's the full picture.

### Basic Form

```c
if (condition)
  statement;
```

If the condition is true (non-zero), the statement runs. If not, it's
skipped. Simple.

### if / else

```c
if (condition)
  statement1;
else
  statement2;
```

One path or the other. Never both. Never neither.

### Compound Statements

When you need more than one line in a branch, wrap them in braces:

```c
if (score >= 3)
{
  print("|10Perfect score!\n");
  print("|10You clearly know your stuff.\n");
}
else
{
  print("|14Not bad. Room to grow.\n");
}
```

Without the braces, only the *first* statement after `if` is conditional.
Everything after it runs unconditionally. This is the most common beginner
bug in any C-like language, and MEX is no exception.

### else if Chains

MEX doesn't have a `switch` statement. When you need multiple branches,
chain `else if`:

```c
if (score = 3)
  print("Perfect!");
else if (score = 2)
  print("Pretty good.");
else if (score = 1)
  print("Could be worse.");
else
  print("Ooof.");
```

It's not as elegant as a switch, but it reads clearly and it works.

## Comparison Operators

Here's what you have to work with:

| Operator | Meaning | Example |
|----------|---------|---------|
| `=` | Equal to | `if (x = 5)` |
| `<>` | Not equal to | `if (x <> 5)` |
| `<` | Less than | `if (x < 10)` |
| `>` | Greater than | `if (x > 0)` |
| `<=` | Less than or equal | `if (x <= 100)` |
| `>=` | Greater than or equal | `if (x >= 1)` |

And the logical operators to combine conditions:

| Operator | Meaning | Example |
|----------|---------|---------|
| `and` | Both must be true | `if (x > 0 and x < 10)` |
| `or` | Either can be true | `if (x = 1 or x = 2)` |

Remember: `=` is comparison, `:=` is assignment. Every other language on
earth uses `==` for comparison. MEX doesn't. You will get this wrong at
least once. Probably at 3 AM. Probably tonight.

## The Case Problem

String comparison in MEX is **case-sensitive**. The string `"Maximus"` is
not equal to `"maximus"` or `"MAXIMUS"`. They're three different strings
as far as the `=` operator is concerned.

For a trivia game, that's a problem. You don't want to mark someone wrong
because they typed "fidonet" instead of "FidoNet."

The fix: `strupper()` or `strlower()`. These built-in functions return a
new string with all letters converted to uppercase or lowercase:

```c
string: answer;
input_str(answer, INPUT_NLB_LINE, 0, 40, "|11> |15");

if (strupper(answer) = "MAXIMUS")
  print("Correct!\n");
```

Now it doesn't matter whether they typed "Maximus", "maximus", "MAXIMUS",
or "mAxImUs". `strupper()` normalizes everything to uppercase before the
comparison. Problem solved.

## The Trivia Game

Here's the whole thing. Call it `trivia.mex`:

```c
#include <max.mh>

int main()
{
  string: answer;
  int: score;

  score := 0;

  print("\n|11═══════════════════════════════════════\n");
  print("|14  BBS Trivia: How Well Do You Know\n");
  print("|14  Your History, |15", usr.name, "|14?\n");
  print("|11═══════════════════════════════════════|07\n\n");
  print("|03Three questions. No lifelines. Let's go.\n\n");

  // Question 1
  print("|14Q1: |07What BBS software shares its name with\n");
  print("    a Roman general? (Hint: you're using it.)\n");
  input_str(answer, INPUT_NLB_LINE, 0, 40, "|11> |15");

  if (strupper(answer) = "MAXIMUS")
  {
    print("|10Correct!|07 You're on it right now.\n\n");
    score := score + 1;
  }
  else
    print("|12Nope.|07 It's Maximus. You're literally using it.\n\n");

  // Question 2
  print("|14Q2: |07What network connected BBSes worldwide\n");
  print("    using store-and-forward message routing?\n");
  input_str(answer, INPUT_NLB_LINE, 0, 40, "|11> |15");

  if (strupper(answer) = "FIDONET" or strupper(answer) = "FIDO NET"
      or strupper(answer) = "FIDO")
  {
    print("|10Correct!|07 FidoNet — the original social network.\n\n");
    score := score + 1;
  }
  else
    print("|12Nope.|07 FidoNet. Est. 1984. Still running.\n\n");

  // Question 3
  print("|14Q3: |07What does BBS stand for?\n");
  input_str(answer, INPUT_NLB_LINE, 0, 60, "|11> |15");

  if (strfind(strupper(answer), "BULLETIN") <> -1
      and strfind(strupper(answer), "BOARD") <> -1
      and strfind(strupper(answer), "SYSTEM") <> -1)
  {
    print("|10Correct!|07 Bulletin Board System.\n\n");
    score := score + 1;
  }
  else
    print("|12Nope.|07 Bulletin Board System.\n\n");

  // Results
  print("|11═══════════════════════════════════════\n");
  print("|14  Score: |15", score, "|14 out of 3\n");
  print("|11═══════════════════════════════════════|07\n\n");

  if (score = 3)
    print("|10Perfect score! |07You are a true sysop.\n");
  else if (score = 2)
    print("|14Two out of three.|07 Respectable showing.\n");
  else if (score = 1)
    print("|06One out of three.|07 The spirit is willing.\n");
  else
    print("|12Zero.|07 ...are you sure you meant to call a BBS?\n");

  print("\n");
  return 0;
}
```

### What's New Here

**Score tracking.** `score` starts at `0` and gets incremented with
`score := score + 1` each time the caller answers correctly. It's just
an integer — nothing fancy. But it accumulates across the whole script,
and the final `if` / `else if` chain uses it to pick a response.

**`strupper()` for case-insensitive matching.** Every answer gets
uppercased before comparison. The caller can type in whatever case they
want; we compare against an all-caps version of the expected answer.

**`or` for multiple accepted answers.** Question 2 accepts "FidoNet",
"Fido Net", or just "Fido." Each variation is its own comparison,
connected with `or`. If *any* of them match, you get the point.

**`strfind()` for partial matching.** Question 3 is trickier — "Bulletin
Board System" could be typed as "bulletin board system", "A Bulletin Board
System", or "it's a Bulletin Board System, obviously." Instead of trying
to match every possible phrasing, we use `strfind()` to check whether the
answer *contains* each key word.

`strfind()` returns the position of the substring if found, or `-1` if
not. So `strfind(str, "BULLETIN") <> -1` means "the word BULLETIN
appears somewhere in this string."

**Compound conditions with `and`.** The Question 3 check requires *all
three* keywords to be present: BULLETIN `and` BOARD `and` SYSTEM. If the
caller just types "bulletin" without the other words, they don't get
credit.

## Compile and Run

```
mex trivia.mex
```

Test it yourself. Try correct answers, wrong answers, weird capitalization,
partial matches. Press Enter without typing anything on a question and
watch what happens. (It'll count as wrong — the empty string doesn't
contain "BULLETIN".)

## Nesting

Conditions can go inside other conditions. You've actually seen this
already in the `else if` chains, but here's an explicit example:

```c
if (usr.times > 10)
{
  if (usr.video = VIDEO_ANSI)
    print("|10Welcome back, veteran ANSI user!\n");
  else
    print("Welcome back, veteran caller!\n");
}
else
  print("Welcome, newcomer!\n");
```

The outer `if` checks call count. The inner `if` checks video mode. The
result: regulars with ANSI get a colorful greeting, regulars without ANSI
get a plain one, and newcomers get a different message entirely.

You can nest as deep as you want, but two or three levels is usually the
practical limit before readability suffers. If your nesting starts looking
like a staircase, consider breaking the inner logic into a helper function.

## Boolean Values in MEX

MEX doesn't have a dedicated `true` / `false` type. Instead:

- **Zero is false.** Any integer, char, or long with value `0` is treated
  as false in a condition.
- **Non-zero is true.** Anything else — `1`, `-1`, `42`, `'A'` — is true.

`max.mh` defines `TRUE` as `1` and `FALSE` as `0` for readability, but
they're just integers. A condition like `if (usr.hotkeys)` works because
`usr.hotkeys` is `1` when enabled and `0` when disabled — it's already a
boolean by convention.

## What You Learned

- **`if` / `else` / `else if`** — the core branching mechanism. One path
  runs, the others are skipped.
- **Comparison operators** — `=`, `<>`, `<`, `>`, `<=`, `>=`. Remember:
  `=` is comparison, `:=` is assignment.
- **Logical operators** — `and`, `or` for combining conditions.
- **`strupper()` / `strlower()`** — convert strings for case-insensitive
  comparison.
- **`strfind()`** — search for a substring. Returns position or `-1`.
- **Score tracking** — just an integer that accumulates. Simple and
  effective.
- **No switch statement** — use `else if` chains instead.
- **Booleans** — zero is false, non-zero is true.

## Next

Your scripts can make decisions now. But they still only do each thing
*once*. The trivia game asks three questions — what if you wanted thirty?
Copy-paste? No.

Next lesson: loops. Doing things more than once, without writing the same
code more than once.

**[Lesson 5: "One More Time, With Feeling" →]({% link mex-learn-loops.md %})**
