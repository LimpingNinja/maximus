---
layout: default
title: "Talking to Your Callers: Are You Still There?"
section: "mex"
description: "Lesson 2 — Input, output, and the art of conversation at 2400 baud"
---

*Lesson 2 of Learning MEX*

---

> **What you'll build:** A script that asks the caller a question, waits
> for their answer, and responds like it actually heard them.

## 2:22 AM

Your greeter works. Callers see their name when they log in. Your board
has a personality now — or at least the beginning of one. A board that
talks.

But talking isn't a conversation. A conversation requires *listening*.

Right now your scripts are like that guy at the party who tells you about
his weekend and then walks away. Technically communication occurred. Nobody
felt communicated *with*.

This lesson fixes that. You're going to teach your board to ask a
question, wait for an answer, and do something with what it hears. It's
the difference between a bulletin board and a *place*.

## Two Ways to Listen

MEX gives you two main input functions, and they serve very different
purposes:

### input_str() — "Type something and press Enter"

This is your workhorse for getting text from callers. It displays a
prompt, waits for them to type, and hands you back whatever they entered
as a string.

```c
int input_str(ref string: s, int: type, char: ch, int: max, string: prompt);
```

That's a lot of parameters. Let's break them down:

| Parameter | What It Does |
|-----------|-------------|
| `s` | The string variable where the answer goes (passed by reference) |
| `type` | Input mode flags — how the input behaves |
| `ch` | A character to pre-fill (usually `0` for nothing) |
| `max` | Maximum number of characters to accept |
| `prompt` | The prompt string displayed to the caller |

The `type` parameter uses flags from `max.mh`. The most common ones:

| Flag | What It Does |
|------|-------------|
| `INPUT_NLB_LINE` | Read a full line (ignoring any stacked input) |
| `INPUT_WORD` | Read a single word |
| `INPUT_NOECHO` | Don't show what the caller types (for passwords) |

A simple call looks like this:

```c
string: answer;
input_str(answer, INPUT_NLB_LINE, 0, 60, "|14> |15");
```

That displays a yellow `>` followed by bright white (so their typing
shows up white), waits for up to 60 characters, and puts whatever they
typed into `answer`.

### input_ch() — "Press any key"

Sometimes you don't need a sentence. You need a single keypress — yes or
no, continue or quit, pick a door.

```c
int input_ch(int: type, string: options);
```

| Parameter | What It Does |
|-----------|-------------|
| `type` | Behavior flags |
| `options` | Acceptable characters, or a prompt string |

The return value is the character they pressed (as an `int`).

```c
int: choice;
choice := input_ch(CINPUT_DISPLAY, "");
```

That waits for any keypress, echoes it to the screen, and stores it.
With `CINPUT_ACCEPTABLE`, you can restrict which keys are valid:

```c
choice := input_ch(CINPUT_DISPLAY + CINPUT_ACCEPTABLE, "YyNn");
```

Now the caller *has* to press Y or N. Anything else gets ignored. No
validation loop needed — the function handles it.

## The Interviewer

Let's build something real. A script that asks the caller three quick
questions and assembles a little profile from the answers. Call it
`interview.mex`:

```c
#include <max.mh>

int main()
{
  string: fav_board;
  string: best_memory;
  int: choice;

  // The opening
  print("\n|11═══════════════════════════════════\n");
  print("|14  The Late-Night Interview\n");
  print("|11═══════════════════════════════════|07\n\n");

  print("|03Hey |15", usr.name, "|03.\n");
  print("|03Quick — three questions. No wrong answers.\n\n");

  // Question 1: text input
  print("|14What's your favorite BBS (besides this one)?\n");
  input_str(fav_board, INPUT_NLB_LINE, 0, 40, "|11> |15");

  if (fav_board = "")
    fav_board := "the silent type, huh?";

  // Question 2: text input
  print("\n|14Best memory from the BBS days?\n");
  input_str(best_memory, INPUT_NLB_LINE, 0, 60, "|11> |15");

  if (best_memory = "")
    best_memory := "(stares wistfully into the distance)";

  // Question 3: single keypress
  print("\n|14Would you run a BBS again if it were easy? |11[|15Y|11/|15N|11] ");
  choice := input_ch(CINPUT_DISPLAY + CINPUT_ACCEPTABLE, "YyNn");

  // The response
  print("\n\n|11═══════════════════════════════════\n");
  print("|14  Your Answers\n");
  print("|11═══════════════════════════════════|07\n\n");

  print("|03Favorite board:  |15", fav_board, "\n");
  print("|03Best memory:     |15", best_memory, "\n");
  print("|03Run one again?   |15");

  if (choice = 'Y' OR choice = 'y')
    print("Yes — and honestly, you already are.\n");
  else
    print("No — but you're calling one at 2 AM, so...\n");

  print("|07\n");
  return 0;
}
```

### What's Happening Here

**String variables** are declared at the top of `main()`, right after the
opening brace — remember, MEX requires all declarations before any
executable statements. We declare `fav_board` and `best_memory` as
strings, and `choice` as an int (because `input_ch()` returns an int).

**The empty-string check** is important. When a caller just hits Enter
without typing anything, `input_str()` gives you an empty string (`""`).
If you don't handle that, your output will have awkward blank spots. The
`if (fav_board = "")` check catches that and substitutes something with
personality.

Remember: `=` is equality in MEX, not assignment. Assignment is `:=`. If
you accidentally write `fav_board := ""` inside an `if`, you'll blank out
the variable instead of checking it. The compiler won't warn you. Ask me
how I know.

**`input_ch()` with `CINPUT_ACCEPTABLE`** is the clean way to do yes/no
prompts. You pass `"YyNn"` as the options string, and the function
refuses to return until the caller presses one of those keys. No
validation loop. No "invalid input, try again." It just waits.

**The `OR` keyword** in `if (choice = 'Y' OR choice = 'y')` is MEX's
logical OR. It works like `||` in C. There's also `AND` (like `&&`) and
`NOT` (like `!`).

## Compile and Run

```
mex interview.mex
```

Wire it to a menu and test it. Log in, run the script, answer the
questions. Then log in as a *different* user and run it again — notice how
`usr.name` changes automatically. Your script doesn't need to do anything
special. The user record is always current.

Try pressing Enter without typing anything on the text questions. Watch
the fallback messages kick in. Try pressing `Q` at the yes/no prompt and
notice how nothing happens — it's waiting for Y or N and it will wait all
night if it has to.

## The Art of the Prompt

A few things that make input prompts feel polished:

**Color your prompt.** The `"|11> |15"` prompt string uses cyan for the
`>` character and switches to bright white so the caller's typing stands
out against the prompt text. Small detail, big difference.

**Set a reasonable max length.** The `max` parameter in `input_str()`
caps how many characters the caller can type. 40 for a board name, 60 for
a memory — generous enough to be useful, short enough to fit on screen
without wrapping weirdly.

**Always handle empty input.** Not everyone wants to answer every
question. Some people just mash Enter. Your script should survive that
gracefully — either with a fallback value, a "never mind" message, or by
skipping the question entirely.

## What You Learned

- **`input_str()`** reads a line of text into a string variable. The
  caller types, presses Enter, and you get their answer.
- **`input_ch()`** reads a single keypress. With `CINPUT_ACCEPTABLE`, it
  restricts which keys are valid — no validation loop needed.
- **String variables** are declared at the top of the function:
  `string: name;`
- **Empty strings** happen when callers press Enter without typing.
  Always check for `""`.
- **`=` is equality**, `:=` is assignment. Getting these backwards is the
  #1 MEX debugging mystery.
- **`OR`**, **`AND`**, **`NOT`** are the logical operators.

## Next

Your board can talk. It can listen. But it doesn't really *know*
anything about who it's talking to — beyond what's already in the user
record. In the next lesson, we'll dig deeper into that record and build
something that shows the caller everything the board knows about them.

It's a little bit creepy. It's a lot useful.

**[Lesson 3: "Who Goes There?" →]({{ site.baseurl }}{% link mex-learn-user-record.md %})**
