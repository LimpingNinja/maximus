---
layout: default
title: "Getting Started with MEX"
section: "mex"
description: "Introduction to MEX scripting"
---

So you've got a board running. Callers are logging in, messages are flowing,
files are moving. Everything works. But now you want to make it *yours* —
a custom welcome screen that knows who's calling, a trivia game for the
regulars, a voting booth, maybe something nobody's ever seen on a BBS before.

That's what MEX is for.

## What Is MEX?

MEX — the **Maximus Extension Language** — is a real programming language
built right into your board. It looks a lot like C, feels a lot like BASIC
in spirit, and runs inside the Maximus VM so your callers experience it
live, in real time, right at their terminal.

MEX isn't a toy. It has variables, functions, loops, string handling, file
I/O, direct access to the user record and message base, lightbar menus,
and even HTTP networking. Parts of the standard Maximus distribution are
*written* in MEX — the file and message area headers, pieces of the Change
Menu, and more.

But it's also not scary. If you can edit a TOML file, you can write MEX.
The compiler catches your mistakes before callers ever see them, and the
worst thing a buggy script can do is print something weird.

## What You'll Need

- **A text editor** — anything that saves plain ASCII. Your system editor,
  `nano`, `vim`, VS Code, whatever you like.
- **The MEX compiler** (`mex`) — ships with Maximus. It reads your `.mex`
  source files and compiles them into `.vm` bytecode that the board can run.
- **A running board** — so you can test your scripts with a real caller
  session (even if that caller is you, logged in from another terminal).

## The Three-Step Path

This section covers the essentials — just enough to get something running:

1. **[MEX Compiler]({% link mex-compiler.md %})** — What the compiler does,
   how to invoke it, and what the output looks like.
2. **[Script Structure]({% link mex-script-structure.md %})** — The anatomy
   of a `.mex` file: includes, functions, `main()`, and how it all fits
   together.
3. **[Hello World]({% link mex-hello-world.md %})** — Write it, compile it,
   wire it to a menu option, and watch it run.

After that, you'll know enough to be dangerous. Which is exactly where
**[Learning MEX]({% link mex-learning.md %})** picks up — a hands-on,
lesson-by-lesson journey from "hello" to a working mini-game on your board.

## Where Things Live

MEX source files (`.mex`) and compiled bytecode (`.vm`) typically live in
your Maximus `m/` directory. Header files (`.mh`) — especially the
all-important `max.mh` — live there too.

To run a compiled MEX script from a menu, you add a `MEX` command option
pointing to the `.vm` file. That's it. One menu entry, one script, one
happy caller. (Details are in the [Menu Options]({% link config-menu-options.md %})
reference under the `MEX` command.)

## Ready?

Pick up at the [MEX Compiler]({% link mex-compiler.md %}) if you want the
methodical walk-through, or jump straight to
[Learning MEX]({% link mex-learning.md %}) if you learn by doing and
want the fun version.
