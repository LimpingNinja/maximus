---
layout: default
title: "Script Structure"
section: "mex"
description: "MEX script file structure"
---

Every MEX script follows the same basic shape. Once you've seen it once,
you'll recognize it everywhere — in the shipped scripts, in other sysops'
code, in anything you write yourself. It's a small language with a
predictable skeleton, and that's a feature.

Here's the whole thing, top to bottom:

```c
// 1. Preprocessor directives
#include <max.mh>
#define GREETING "Welcome aboard"

// 2. Global variables (optional)
int: call_count;

// 3. Helper functions (optional)
void say_hello(string: name)
{
  print(GREETING, ", ", name, "!\n");
}

// 4. The main function (required)
int main()
{
  say_hello(usr.name);
  return 0;
}
```

That's it. Four zones, and only the last one is mandatory. Let's walk
through each.

## 1. Preprocessor Directives

The top of the file is where you tell the compiler what to pull in and
what constants to define. These lines start with `#` and are handled
before the real compilation begins.

### `#include`

Every MEX script needs at least this one:

```c
#include <max.mh>
```

This imports the standard Maximus header — all the built-in function
declarations, type definitions, and constants. Without it, the compiler
won't recognize `print()`, `usr`, `input()`, or anything else
Maximus-specific.

You can include additional headers for specialized features:

```c
#include <prm.mh>      // System path access
#include <input.mh>    // Extended input functions
#include <rand.mh>     // Random numbers
#include <intpad.mh>   // Number formatting
```

### `#define`

Defines create named constants or simple macros:

```c
#define MAX_ENTRIES  10
#define DATA_FILE    "guestbook.dat"
```

Anywhere the compiler sees `MAX_ENTRIES` after this, it substitutes `10`.
No magic, no runtime cost — it's a straight text replacement before
compilation.

You can also use `#ifdef` / `#ifndef` / `#endif` for conditional
compilation — handy for toggling features without deleting code:

```c
#ifdef DEBUG
  print("Debug: we got here\n");
#endif
```

## 2. Global Variables

Variables declared outside any function are global — visible everywhere
in the script. Use them sparingly; most variables belong inside functions.

**Globals must be declared at the top of the file**, after your `#include`
and `#define` directives but before any function definitions. You can't
scatter them between functions or tuck them in at the bottom — the
compiler expects them up front.

```c
#include <max.mh>

// Global variables go here, before any functions
int: total_score;
string: last_visitor;

// Functions follow
int main()
{
  ...
}
```

MEX supports these basic types:

| Type | What It Holds |
|------|--------------|
| `int` | A whole number (32-bit signed integer) |
| `char` | A single character |
| `string` | A dynamic text string (no fixed length limit) |
| `long` | A large integer |
| `void` | Nothing (used for functions that don't return a value) |

You can also declare arrays:

```c
array [1..10] of string: names;
```

Note the MEX variable syntax — the colon before the variable name is
required: `int: x`, not `int x`. It looks a little different from C, but
you'll get used to it fast.

## 3. Helper Functions

Functions are reusable blocks of code. They have a return type, a name,
and zero or more parameters:

```c
void show_banner()
{
  print("|12===========================\n");
  print("|14  Welcome to the Board!   \n");
  print("|12===========================\n");
}
```

```c
int add(int: a, int: b)
{
  return a + b;
}
```

Functions must be declared *before* they're called — if `main()` calls
`show_banner()`, then `show_banner()` needs to appear above `main()` in
the file.

### Pass by Reference

By default, arguments are passed by value — the function gets a copy. If
you want a function to modify the caller's variable, use a reference
parameter (prefix with `ref`):

```c
void increment(ref int: counter)
{
  counter := counter + 1;
}
```

## 4. The `main()` Function

Every MEX script must have a `main()` function. This is where execution
starts when Maximus runs your script. It returns an `int` — conventionally
`0` for success:

```c
int main()
{
  print("Hello from MEX!\n");
  return 0;
}
```

When `main()` returns (or when execution falls off the end), control goes
back to Maximus and the caller sees whatever menu they were on before.

### Variable Declarations Go Up Top

This applies to `main()` and every other function: **all local variable
declarations must appear at the top of the function body**, immediately
after the opening `{`. You can't declare a variable mid-function the way
you can in modern C — the compiler needs all declarations before the first
executable statement.

```c
int main()
{
  // Declarations first — all of them, right here
  int: count;
  string: name;
  char: choice;

  // Then your code
  name := usr.name;
  count := 0;
  ...
}
```

This is a hard rule, not a style preference. If you try to declare a
variable after a `print()` or an `if`, the compiler will complain.

## Statements and Syntax

Inside functions, the code is made up of statements — each one ending
with a semicolon. The basics:

- **Assignment** uses `:=` (not `=`): `x := 42;`
- **Comparison** uses `=` for equality (not `==`): `if (x = 42)`
- **String concatenation** uses `+`: `name + " was here"`
- **Comments** use `//` for single-line: `// this is a comment`
- **Braces** `{ }` group multiple statements into a block
- **Semicolons** end every statement — the compiler relies on them, not
  line breaks

If you've written C, most of this will feel familiar — with the `:=`
assignment and `=` equality being the main surprises. If you've written
Pascal, those will feel like home instead.

## A Real Example

Here's `logo.mex` from the shipped scripts — a script that displays a
random logo file on login. It touches all four zones:

```c
#include <max.mh>
#include <prm.mh>
#include <rand.mh>
#include <intpad.mh>

#define RANDOM 6

int main()
{
  char: nonstop;
  string: logo;
  int: which;

  srand(time());

  logo := prm_string(PRM_MISCPATH) + "Logo";

  which := (rand() % RANDOM) + 1;
  logo := logo + intpadleft(which, 2, '0');

  reset_more(nonstop);
  display_file(logo, nonstop);

  return 0;
}
```

Preprocessor at the top, variables declared inside `main()`, built-in
functions called in sequence, and a `return 0` at the end. That's the
whole pattern.

## Next Steps

You've seen the shape. Now let's fill it in — the next page walks you
through writing, compiling, and running your first script from scratch.

**[Hello World →]({% link mex-hello-world.md %})**
