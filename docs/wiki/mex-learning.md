---
layout: default
title: "Learning MEX"
section: "mex"
description: "A hands-on, story-driven guide to MEX scripting"
---

## It's 2 AM. Your Board Is Running. Let's Make It Weird.

You know how to compile a script. You've seen "Hello, world" land on a
terminal. Congratulations — you've done the thing every programming guide
makes you do first, and it was exactly as thrilling as printing two words
to a screen ever is.

Now let's do something *real*.

This is a ten-lesson journey from "I sort of know what MEX is" to "I have
a working game on my board and my callers think I'm a wizard." Each lesson
builds on the one before it. Each one makes something you can actually use.
And each one happens the way the best BBS hacking always happens — late at
night, one idea at a time, fueled by curiosity and the faint glow of a
terminal.

You don't need prior programming experience. You don't need to read the
[Language Guide]({% link mex-language-guide.md %}) first (though it's
there when you want the fine print). You just need a board, a text editor,
and the willingness to try things and see what breaks.

---

## The Roadmap

Here's where we're going. Each lesson is self-contained enough to stop
after, but they're better together — by the end, the skills stack up into
something genuinely cool.

### The Basics (Lessons 1–3)

You'll go from printing text to having a real conversation with your
callers — and knowing exactly who you're talking to.

- **[Your First Script]({% link mex-learn-first-script.md %})** —
  "Hello, Stranger." We're past hello-world. This time the board knows
  your caller's name, and it's not afraid to use it.
- **[Talking to Your Callers]({% link mex-learn-input-output.md %})** —
  "Are You Still There?" Asking questions, hearing answers, and making
  your board feel like it's actually listening.
- **[Know Your Caller]({% link mex-learn-user-record.md %})** —
  "Who Goes There?" Peeking behind the curtain at the user record.
  Name, access level, time left, how many times they've called — it's
  all there, and it's all yours.

### Making Things Interesting (Lessons 4–6)

Your scripts learn to think, repeat, and remember.

- **[Making Decisions]({% link mex-learn-decisions.md %})** —
  "Choose Your Own Adventure." Your board becomes smarter than a light
  switch. If/else, conditions, and a trivia question that actually works.
- **[Going in Rounds]({% link mex-learn-loops.md %})** —
  "One More Time, With Feeling." Loops — the hamster wheel of programming,
  except this hamster builds a quote-of-the-day rotator.
- **[Remembering Things]({% link mex-learn-file-io.md %})** —
  "Dear Diary." Your board gets a memory. Read files, write files, build
  a guestbook that survives a reboot.

### Building Real Things (Lessons 7–9)

Now you're building features your callers will actually notice.

- **[Building Menus]({% link mex-learn-menus.md %})** —
  "Press Any Key to Be Amazing." Lightbar menus, selection prompts, and
  making your board feel like a place, not a command line.
- **[The Message Base]({% link mex-learn-messages.md %})** —
  "You've Got Mail (And We Can Read It)." Talking to the message base
  from MEX — reading posts, counting messages, maybe even posting one.
- **[Reaching the Outside World]({% link mex-learn-networking.md %})** —
  "Phone Home." Your BBS learns to talk to the internet. HTTP requests,
  JSON parsing, and pulling live data onto a text-mode screen like it's
  the most normal thing in the world.

### The Grand Finale (Lesson 10)

Everything comes together.

- **[Your First Mini-Game]({% link mex-learn-mini-game.md %})** —
  "Game Night." Variables, loops, files, menus, decisions — all of it,
  woven into a real game your callers can play. This is why you stayed
  up until 2 AM.

---

## How to Use This Guide

**Go in order** if you're new. Each lesson assumes you've done the ones
before it.

**Skip around** if you already know some programming. The lessons are
self-contained enough that you can jump to the topic you need — but if
something doesn't make sense, the earlier lesson probably covers it.

**Type the code yourself.** Seriously. Copy-paste teaches your clipboard,
not your brain. The examples are short on purpose.

**Break things on purpose.** Change a variable name. Remove a semicolon.
See what the compiler says. The error messages are pretty good, and
understanding them is half the skill.

When you want the deep reference — every function signature, every type
rule, every edge case — the [Language Guide]({% link mex-language-guide.md %})
and [Standard Intrinsics]({% link mex-standard-intrinsics.md %}) are
waiting. But you don't need them yet.

You just need Lesson 1 and a late night.

**[Let's go →]({% link mex-learn-first-script.md %})**
