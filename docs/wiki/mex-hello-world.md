---
layout: default
title: "Hello World"
section: "mex"
description: "Your first MEX script"
---

Time to write something, compile it, and watch it run on your board. This
is the shortest path from "nothing" to "a MEX script that a caller can
actually see." It won't be fancy. It will work. And once it works, you'll
know the whole pipeline is solid — editor to compiler to menu to terminal.

## Write It

Open your text editor and create a file called `hello.mex` in your
`resources/m/` directory:

```c
#include <max.mh>

int main()
{
  print("|14Hello, world!|07\n");
  print("This is MEX. It works. You're a sysop now.\n");
  return 0;
}
```

That's the entire script. Let's break it down:

- **`#include <max.mh>`** — pulls in the Maximus header so the compiler
  knows about `print()` and everything else.
- **`int main()`** — the entry point. Maximus starts here when it runs
  your script.
- **`print("|14Hello, world!|07\n")`** — prints "Hello, world!" in
  bright yellow (`|14`), then resets to gray (`|07`). The `\n` moves
  the cursor to the next line.
- **`return 0;`** — tells Maximus the script finished successfully.

The pipe codes (`|14`, `|07`) are the same color codes you use in ANSI
display files and menu descriptions. They work inside MEX strings too.

## Compile It

From your project root:

```bash
./scripts/compile-mex.sh hello
```

Or manually:

```
cd resources/m
../../build/bin/mex hello.mex
```

If it compiles without errors, you'll have `hello.vm` sitting next to
your source file. If you get an error, the compiler will tell you the
line number — double-check your semicolons and braces.

## Wire It to a Menu

To make the script accessible to callers, add a menu option. Open your
main menu TOML (e.g., `resources/config/menus/main.toml`) and add an
option:

```toml
[[option]]
command = "MEX"
arguments = "hello"
priv_level = "Demoted"
description = "%Hello World"
```

The `%` in the description marks `H` as the hotkey — callers press `H`
to run the script. The `arguments` field is the name of your `.vm` file
without the extension. Maximus will look for it in the scripts directory.

> **No restart needed.** Maximus picks up menu changes on the fly — just
> reload the menu (or have a caller navigate to it) and the new option
> appears.

## Run It

Log in to your board (or use a local test session), navigate to the menu
where you added the option, and press the hotkey. You should see:

```
Hello, world!
This is MEX. It works. You're a sysop now.
```

The yellow "Hello, world!" followed by the gray second line. That's your
code, running inside Maximus, displayed to the terminal in real time.

If you don't see it, check:

- **Did it compile?** Look for `hello.vm` in the scripts directory.
- **Is the path right?** The `arguments` field should match the `.vm`
  filename (without extension) relative to the scripts directory.
- **Is the menu reloaded?** Try navigating away from the menu and back.

## Make It Yours

Try changing the script. Replace the text, add more `print()` lines,
experiment with color codes:

```c
#include <max.mh>

int main()
{
  print("|11╔══════════════════════════════╗\n");
  print("|11║ |15Welcome to My Board!       |11║\n");
  print("|11║ |07Built with MEX, powered by  |11║\n");
  print("|11║ |07caffeine and stubbornness.  |11║\n");
  print("|11╚══════════════════════════════╝|07\n");
  return 0;
}
```

Recompile, reload the menu, and see the result. The edit-compile-test
loop is fast — there's no linking step, no deployment pipeline, just
save, compile, and go.

## What's Next

You've got the pipeline working: write → compile → wire → run. That's
the foundation everything else builds on.

From here you have two paths:

- **[Learning MEX]({{ site.baseurl }}{% link mex-learning.md %})** — a hands-on,
  lesson-by-lesson journey that builds real, useful things. Start here
  if you want the guided tour.
- **[Language Guide]({{ site.baseurl }}{% link mex-language-guide.md %})** — the full
  reference for MEX syntax, types, and semantics. Start here if you
  already know how to program and just need the details.

Either way, the hard part is done. You have a working script on a
running board. Everything from here is just making it do more interesting
things.

**[Start Learning MEX →]({{ site.baseurl }}{% link mex-learning.md %})**
