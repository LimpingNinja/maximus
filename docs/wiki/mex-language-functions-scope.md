---
layout: default
title: "Functions & Scope"
section: "mex"
description: "Declaring functions, passing parameters by value and reference, prototypes, and how scope works in MEX"
---

Once your script outgrows a single `main()`, you start breaking it into
functions. A function that draws the header. A function that formats a
profile row. A function that validates input. This is how BBS scripts stay
readable at 2am — and how you reuse code across different scripts by
putting shared functions in their own `.mh` header files.

MEX functions work like C functions with one important addition borrowed
from Pascal: the `ref` keyword for pass-by-reference parameters. If you
need a function to modify a variable in the caller's scope — or if you're
passing a struct to an intrinsic that fills it in — `ref` is how you do it.

---

## On This Page

- [Quick Reference](#quick-reference)
- [Declaring Functions](#declaring)
- [Parameters](#parameters)
- [Pass by Reference (`ref`)](#pass-by-ref)
- [Return Values](#return-values)
- [Forward Declarations (Prototypes)](#prototypes)
- [The `main()` Function](#main)
- [Scope Rules](#scope)
- [Recursive Functions](#recursion)

---

## Quick Reference {#quick-reference}

| Concept | Syntax | Notes |
|---------|--------|-------|
| [Function definition](#declaring) | `void greet() { ... }` | Body in braces |
| [Typed parameters](#parameters) | `void greet(string: name)` | Type-colon-name, comma-separated |
| [Pass by reference](#pass-by-ref) | `void fill(ref int: x)` | Caller's variable is modified |
| [Return value](#return-values) | `int add(int: a, int: b) { return a + b; }` | Return type before name |
| [Prototype](#prototypes) | `int add(int: a, int: b);` | Semicolon instead of body |
| [Entry point](#main) | `void main()` | Required — execution starts here |

---

## Declaring Functions {#declaring}

A function has a return type, a name, a parameter list, and a body:

```mex
void draw_header()
{
  ui_fill_rect(1, 1, 80, 1, ' ', UI_CYAN);
  ui_write_padded(1, 1, 80, "[ My BBS ]", UI_CYAN);
}
```

If the function doesn't return anything, use `void`. If it does, specify the
return type:

```mex
int double_it(int: x)
{
  return x * 2;
}
```

Functions must be defined **before** they're called — the compiler reads
top-to-bottom. If you have mutual recursion or just prefer putting `main()`
at the top, use a [forward declaration](#prototypes).

---

## Parameters {#parameters}

Parameters use the same `type: name` syntax as variable declarations:

```mex
void greet(string: name, int: times_called)
{
  print("Welcome back, " + name + "!\n");
  print("This is call #" + itostr(times_called) + ".\n");
}
```

Call it with matching arguments:

```mex
greet(usr.name, usr.times);
```

**By default, parameters are passed by value.** The function gets a copy.
Modifying a parameter inside the function does not affect the caller's
variable:

```mex
void try_to_change(int: x)
{
  x := 99;   // only changes the local copy
}

void main()
{
  int: n;
  n := 42;
  try_to_change(n);
  print(itostr(n) + "\n");   // still prints 42
}
```

---

## Pass by Reference (`ref`) {#pass-by-ref}

Add `ref` before the parameter type to pass by reference. The function
receives the actual variable, not a copy — changes inside the function
are visible to the caller:

```mex
void swap(ref int: a, ref int: b)
{
  int: temp;
  temp := a;
  a := b;
  b := temp;
}

void main()
{
  int: x, y;
  x := 10;
  y := 20;
  swap(x, y);
  // x is now 20, y is now 10
}
```

### When You Need `ref`

- **Returning multiple values** — MEX functions can only return one value.
  Use `ref` parameters for additional outputs.
- **Filling in structs** — the UI style-default functions all take `ref`
  parameters:

  ```mex
  struct ui_lightbar_style: ls;
  ui_lightbar_style_default(ls);   // fills in ls with defaults
  ```

- **Intrinsics that modify their arguments** — functions like `timestamp()`,
  `userfindopen()`, and `filefindfirst()` all use `ref` to write results
  back through their parameters.

### Arrays Are Always `ref`

When you pass an array to a function, it's always by reference — there's no
way to pass an array by value. The function declaration uses `ref array`:

```mex
void fill_menu(ref array [1..] of string: items)
{
  items[1] := "Read Messages";
  items[2] := "File Areas";
  items[3] := "Goodbye";
}
```

The `[1..]` syntax means "array starting at index 1, size determined by the
caller." This lets you write functions that accept arrays of any size.

---

## Return Values {#return-values}

Use `return` to exit a function and (optionally) send a value back to the
caller:

```mex
int max(int: a, int: b)
{
  if (a > b)
    return a;
  return b;
}
```

A `void` function can use bare `return;` to exit early:

```mex
void maybe_greet(int: should_greet)
{
  if (not should_greet)
    return;
  print("Hello!\n");
}
```

If a non-void function falls off the end without a `return`, the return
value is undefined. Always make sure every code path returns a value.

---

## Forward Declarations (Prototypes) {#prototypes}

A prototype declares a function's signature without its body — just a
semicolon where the braces would be:

```mex
// Prototypes (at top of file)
void draw_header();
void draw_footer();
int show_menu();

// Now main() can call them even though they're defined below
void main()
{
  draw_header();
  int: choice;
  choice := show_menu();
  draw_footer();
}

// Definitions follow...
void draw_header()
{
  // ...
}
```

Prototypes are also how the `.mh` header files work — `max.mh` and
`maxui.mh` are full of prototypes that tell the compiler "this function
exists, here's its signature, the runtime will provide the implementation."

### Include Files

You can put your own prototypes (and shared struct definitions, constants,
and utility functions) in a `.mh` file and `#include` it:

```mex
// mylib.mh
int clamp(int: val, int: lo, int: hi);
```

```mex
// myscript.mex
#include <max.mh>
#include "mylib.mh"

void main()
{
  int: x;
  x := clamp(usr.times, 0, 100);
}
```

Use angle brackets (`<max.mh>`) for system headers and double quotes
(`"mylib.mh"`) for your own files in the same directory.

---

## The `main()` Function {#main}

Every MEX script must have a `main()` function. It's the entry point —
when Maximus runs your `.vm` file, execution begins at `main()`:

```mex
#include <max.mh>

void main()
{
  print("Hello, " + usr.name + "!\n");
}
```

`main()` is always `void` and takes no parameters. When `main()` returns
(or execution falls off the end), the script exits and control returns to
Maximus.

---

## Scope Rules {#scope}

### Global Scope

Variables declared outside any function are global — visible to every
function in the file, from the point of declaration onward:

```mex
int: total_score;   // global

void add_points(int: pts)
{
  total_score := total_score + pts;   // can access global
}
```

The globals exported by `max.mh` (`usr`, `marea`, `farea`, `msg`, `sys`,
`id`, `input`) are always in scope after `#include <max.mh>`.

### Local Scope

Variables declared inside a function (or inside a block) are local to that
function or block:

```mex
void example()
{
  int: x;          // local to example()
  x := 42;

  if (x > 10)
  {
    string: temp;  // local to this block
    temp := "big";
  }
  // temp does not exist here
}
```

Local variables shadow globals with the same name — the local wins inside
its scope:

```mex
int: count;   // global

void reset()
{
  int: count;     // local — shadows global
  count := 0;     // modifies local, not global
}
```

### Static Data

If you need data to persist across multiple calls to a script (or across
different scripts), MEX provides static data functions:

```mex
// Store a value that survives after the script exits
create_static_string("my_key");
set_static_string("my_key", "some value");

// Later (even in a different script invocation)
string: saved;
get_static_string("my_key", saved);
```

See [Display & I/O Intrinsics]({{ site.baseurl }}{% link mex-intrinsics-display-io.md %}) for
the full static data API.

---

## Recursive Functions {#recursion}

MEX supports recursion — a function can call itself. The classic example:

```mex
int factorial(int: n)
{
  if (n <= 1)
    return 1;
  return n * factorial(n - 1);
}
```

Be mindful of stack depth. The MEX VM has a fixed stack size (configurable
with the `-s` compiler flag, default is modest). Deep recursion will
overflow it. For most BBS scripting tasks, iteration (`while`/`for`) is
the better choice.

---

## See Also

- [Variables & Types]({{ site.baseurl }}{% link mex-language-vars-types.md %}) — the types
  your parameters and return values can have
- [Control Flow]({{ site.baseurl }}{% link mex-language-control-flow.md %}) — `if`, `while`,
  `for`, and `goto`
- [MEX Compiler]({{ site.baseurl }}{% link mex-compiler.md %}) — stack size flags and include
  path behavior
