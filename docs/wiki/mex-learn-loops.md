---
layout: default
title: "Going in Rounds: One More Time, With Feeling"
section: "mex"
description: "Lesson 5 — Loops, iteration, and a number guessing game"
---

*Lesson 5 of Learning MEX*

---

> **What you'll build:** A number guessing game — the board picks a secret
> number, the caller guesses, and the script says "higher" or "lower"
> until they get it right. Then it asks if they want to play again.

## 3:04 AM

Your trivia game works, but it has a problem: it asks exactly three
questions and stops. What if you wanted ten questions? A hundred?
Copy-pasting the same block of code a hundred times is not programming.
It's suffering.

Loops fix this. A loop says "do this thing, then check if we should do it
again, and keep going until we shouldn't." Three lines of loop code can
replace a hundred lines of copy-paste. That's not laziness — that's the
entire point of programming.

This lesson teaches you all three kinds of loops MEX offers, then puts
them together into something that actually needs them: a guessing game
that runs until the caller wins, and asks if they want another round.

## The for Loop

The `for` loop is for when you know how many times you want to go around.
It packs the setup, the test, and the increment into one tidy line:

```c
for (i := 1; i <= 10; i := i + 1)
  print("Line ", i, "\n");
```

That prints "Line 1" through "Line 10." The three parts inside the
parentheses:

1. **`i := 1`** — runs once before the loop starts (initialization)
2. **`i <= 10`** — checked before each iteration (continue if true)
3. **`i := i + 1`** — runs after each iteration (increment)

With a compound body:

```c
for (i := 1; i <= 5; i := i + 1)
{
  print("|14Star #", i, ": |11*|07\n");
}
```

The `for` loop is perfect for counted things — printing numbered lists,
iterating through arrays, drawing repeated characters. Anytime you think
"do this N times," reach for `for`.

## The while Loop

The `while` loop is for when you *don't* know how many iterations you
need. You just have a condition, and the loop keeps going as long as it's
true:

```c
while (condition)
  statement;
```

The condition is checked *before* each iteration, including the first. If
the condition is false from the start, the body never runs.

```c
int: guess;
guess := 0;

while (guess <> 42)
{
  print("Guess the number: ");
  input_str(answer, INPUT_NLB_LINE, 0, 10, "|11> |15");
  guess := strtoi(answer);
}

print("You got it!\n");
```

This loop has no idea how many times it'll run. Could be one guess, could
be fifty. It just keeps going until `guess` equals `42`.

Note **`strtoi()`** — it converts a string to an integer. `input_str()`
gives you a string; if you need to do math or number comparisons with it,
you convert it first.

## The do..while Loop

The `do..while` is the loop that always runs at least once. It checks the
condition *after* the body, not before:

```c
do
{
  print("Press [Q] to quit: ");
  choice := input_ch(CINPUT_DISPLAY + CINPUT_ACCEPTABLE, "Qq");
}
while (choice <> 'Q' and choice <> 'q');
```

This is the "play again?" loop. You always want to show the prompt at
least once — there's no situation where you'd skip it entirely. The
`do..while` guarantees that.

The syntax: `do { ... } while (condition);` — note the semicolon after
the `while` condition. Easy to forget. The compiler will remind you.

## Putting It Together: The Guessing Game

Here's a real script that uses all three loop types. Call it `guess.mex`:

```c
#include <max.mh>
#include <rand.mh>

int play_round()
{
  int: secret;
  int: guess;
  int: attempts;
  string: answer;

  secret := (rand() % 50) + 1;
  attempts := 0;

  print("\n|14I'm thinking of a number between 1 and 50.\n");
  print("|03Good luck, |15", usr.name, "|03.\n\n");

  // while loop — keep guessing until they get it
  guess := 0;

  while (guess <> secret)
  {
    print("|14Your guess? ");
    input_str(answer, INPUT_NLB_LINE, 0, 5, "|11> |15");

    if (answer = "")
    {
      print("|12Come on, guess something.\n");
    }
    else
    {
      guess := strtoi(answer);
      attempts := attempts + 1;

      if (guess < secret)
        print("|06Too low.\n");
      else if (guess > secret)
        print("|06Too high.\n");
    }
  }

  print("\n|10Got it!|07 The number was |15", secret, "|07.\n");
  print("|03You got it in |14", attempts, "|03 guess");

  if (attempts <> 1)
    print("es");

  print(".|07\n");

  return attempts;
}

int main()
{
  int: best;
  int: rounds;
  int: result;
  int: choice;

  srand(time());

  best := 999;
  rounds := 0;

  print("\n|11═══════════════════════════════════════\n");
  print("|14  The Number Game\n");
  print("|11═══════════════════════════════════════|07\n");

  // do..while — always play at least one round
  do
  {
    result := play_round();
    rounds := rounds + 1;

    if (result < best)
      best := result;

    print("\n|14Play again? |11[|15Y|11/|15N|11] ");
    choice := input_ch(CINPUT_DISPLAY + CINPUT_ACCEPTABLE, "YyNn");
    print("\n");
  }
  while (choice = 'Y' or choice = 'y');

  // for loop — draw a results banner
  print("\n|11");
  for (result := 1; result <= 39; result := result + 1)
    print("═");
  print("|07\n");

  print("|14  Game Over\n");
  print("|03  Rounds played: |15", rounds, "\n");
  print("|03  Best score:    |15", best, " guesses\n");

  print("|11");
  for (result := 1; result <= 39; result := result + 1)
    print("═");
  print("|07\n\n");

  return 0;
}
```

### What's New Here

**`#include <rand.mh>`** — this header provides `srand()` and `rand()`.
Call `srand(time())` once at the start to seed the random number generator
with the current time, then call `rand()` to get random numbers. The
expression `(rand() % 50) + 1` gives you a number between 1 and 50.

**`strtoi()`** converts a string to an integer. `input_str()` always
returns a string — even if the caller types `"42"`, that's a string of
two characters, not the number forty-two. `strtoi()` does the conversion.
There's also `strtol()` for longs.

**Three loop types in one script:**

- **`while`** — the guessing loop. Runs until `guess = secret`. Could be
  one iteration, could be fifty.
- **`do..while`** — the "play again?" loop. Always runs at least once
  (you always play at least one round).
- **`for`** — drawing the banner lines. We know exactly how many
  characters we want (39), so `for` is the right tool.

**A function that returns a value.** `play_round()` returns the number
of attempts. `main()` uses this to track the best score across rounds.
Functions aren't just for avoiding repetition — they're for organizing
logic into coherent units. `play_round()` handles one round; `main()`
handles the session.

**Score tracking across rounds.** `best` starts at `999` (impossibly
high) and gets updated whenever a round beats it. `rounds` counts how
many times the caller played. Simple variables, accumulating across
loop iterations.

## When to Use Which Loop

| Loop | Use When... |
|------|------------|
| `for` | You know how many iterations you need |
| `while` | You don't know — you have a condition to check |
| `do..while` | Same as `while`, but the body must run at least once |

In practice, `while` and `for` handle most situations. `do..while` is
specifically for "do this, then ask if we should do it again" patterns —
menus, "play again?" prompts, retry loops.

## The goto Escape Hatch

MEX also has `goto` for jumping to a labeled point in the code:

```c
for (i := 1; i <= 100; i := i + 1)
{
  if (some_error_condition)
    goto bail_out;
}

bail_out:
print("Exited the loop.\n");
```

Use it sparingly — it's there for breaking out of deeply nested loops or
error-handling situations where `while` conditions get awkward. For
everyday loop control, stick with the structured loops.

## What You Learned

- **`for` loops** — init, test, increment. Perfect for counted iteration.
- **`while` loops** — test first, then run. Perfect for unknown-length
  iteration.
- **`do..while` loops** — run first, then test. Perfect for "at least
  once" patterns.
- **`strtoi()` / `strtol()`** — convert strings to numbers for math and
  comparison.
- **`rand()` and `srand()`** — random numbers, seeded from `time()`.
  Include `rand.mh`.
- **`goto`** — exists, works, use sparingly.
- **Functions as organizational units** — `play_round()` encapsulates one
  round of logic and returns a result to the caller.

## Next

Your scripts can loop, branch, and keep state. But everything lives in
memory — the moment the script exits, all your variables vanish. What if
you want something to *persist*? A guestbook. A high score table. A
quote-of-the-day file.

Next lesson: files. Reading them, writing them, and finally giving your
board a memory that survives between calls.

**[Lesson 6: "Dear Diary" →]({{ site.baseurl }}{% link mex-learn-file-io.md %})**
