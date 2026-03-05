---
layout: default
title: "Language Guide"
section: "mex"
description: "MEX language reference — types, operators, control flow, functions, and everything that makes the language tick"
---

You've built things in the [lessons]({{ site.baseurl }}{% link mex-learning.md %}). You've wired
up lightbar menus and fetched jokes from the internet at 2am. Now you need to
look something up — what operators does MEX actually have? How do arrays work
again? Can you pass a struct by reference?

This is the language reference. It covers the MEX language itself — the syntax,
the type system, the control flow constructs, the way functions and scope work.
It doesn't cover the built-in functions that talk to Maximus (that's
[Standard Intrinsics]({{ site.baseurl }}{% link mex-standard-intrinsics.md %})). Think of this
section as "the language" and intrinsics as "the library."

MEX looks like C, borrows from Pascal, and has a few tricks of its own.
If you've written C, most of this will feel familiar. If you haven't, the
lessons got you this far — this section fills in the gaps.

---

## At a Glance

| Topic | Page | What You'll Find |
|-------|------|-----------------|
| [Variables & Types](#vars) | [Variables & Types]({{ site.baseurl }}{% link mex-language-vars-types.md %}) | Primitive types, arrays, structs, type conversions, the `usr` record |
| [Control Flow](#flow) | [Control Flow]({{ site.baseurl }}{% link mex-language-control-flow.md %}) | `if`/`else`, `while`, `do`/`while`, `for`, `goto`, `break` |
| [Functions & Scope](#funcs) | [Functions & Scope]({{ site.baseurl }}{% link mex-language-functions-scope.md %}) | Declaring functions, parameters, `ref` pass-by-reference, scope rules |
| [String Operations](#strings) | [String Operations]({{ site.baseurl }}{% link mex-language-string-ops.md %}) | Concatenation, slicing, searching, conversion, and every string function |

{: #vars}
### Variables & Types

Every variable in MEX has a type — `int`, `long`, `char`, `string`, or a
`struct`. Arrays use a Pascal-style declaration with explicit bounds. Type
conversions between numeric types and strings require explicit function calls
(`itostr`, `strtoi`, etc.). The full tour is in
[Variables & Types]({{ site.baseurl }}{% link mex-language-vars-types.md %}).

{: #flow}
### Control Flow

MEX has the usual suspects: `if`/`else` for branching, `while` and `for` for
loops, and `goto` for when you just need to bail out of a deeply nested
situation. The `=` vs `:=` distinction (comparison vs assignment) trips up
C programmers — it's covered in detail in
[Control Flow]({{ site.baseurl }}{% link mex-language-control-flow.md %}).

{: #funcs}
### Functions & Scope

Functions are how you organize code. MEX supports pass-by-value and
pass-by-reference (`ref`) parameters, forward declarations (prototypes), and
both global and local scope. The `main()` function is your entry point.
See [Functions & Scope]({{ site.baseurl }}{% link mex-language-functions-scope.md %}).

{: #strings}
### String Operations

Strings in MEX are dynamic — they grow and shrink as needed, no buffer
management required. You can concatenate with `+`, index individual characters
with `[]`, and there's a full set of functions for searching, trimming,
padding, and case conversion. The complete list is in
[String Operations]({{ site.baseurl }}{% link mex-language-string-ops.md %}).

---

## See Also

- [Standard Intrinsics]({{ site.baseurl }}{% link mex-standard-intrinsics.md %}) — the built-in
  functions for I/O, user data, areas, and system access
- [MEX Compiler]({{ site.baseurl }}{% link mex-compiler.md %}) — compiling and running scripts
- [Learning MEX]({{ site.baseurl }}{% link mex-learning.md %}) — the 10-lesson tutorial series
