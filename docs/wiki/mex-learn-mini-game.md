---
layout: default
title: "Your First Mini-Game: Game Night"
section: "mex"
description: "Lesson 10 — The capstone: build a real game your callers can play"
---

*Lesson 10 of Learning MEX*

---

> **What you'll build:** A text adventure called *The Dungeon of
> Forgotten Logins*. Three rooms, a locked door, a trivia boss fight, a
> high score file, and a lightbar menu driving the whole thing. Everything
> you've learned, in one script.

## 4:52 AM

Nine lessons. You've gone from `print("Hello")` to HTTP requests and
lightbar menus. You know variables, loops, decisions, files, structs,
arrays, menus, area iteration, and JSON parsing. That's not a tutorial
anymore. That's a toolkit.

This lesson doesn't teach new concepts. It teaches *assembly* — how to
take the pieces and bolt them together into something that feels like a
real program. The game is small (it has to fit in one lesson), but the
patterns inside it scale to anything you want to build.

The modem light is still blinking. Let's finish what we started.

## The Game Design

**The Dungeon of Forgotten Logins** — a three-room text adventure:

1. **The Entry Hall** — you arrive. There's a note on the wall and a
   door to the east.
2. **The Archive Room** — shelves of ancient floppies. There's a key
   on the floor. The boss door is to the north.
3. **The Server Room** — the boss: a trivia question. Answer correctly
   and you escape. Answer wrong and you're trapped forever (or until
   you try again).

**Game state:** current room, whether the key is picked up, whether the
boss is defeated, number of moves taken.

**Win condition:** defeat the boss. Your score is how few moves it took.

**Persistence:** high scores saved to a file.

## The Architecture

Before we code, let's think about structure. A game loop looks like this:

```
while (game is running)
{
  1. Display the current room
  2. Show available actions
  3. Read the player's choice
  4. Update game state based on the choice
}
```

That's a `while` loop from Lesson 5, a lightbar menu from Lesson 7,
`if`/`else` decisions from Lesson 4, and state tracking with variables
from Lessons 1 and 2. The high score file is Lesson 6. The trivia boss
is Lesson 4 again.

Every game ever made is some variation of this loop.

## The Full Script

Call it `dungeon.mex`. This is the longest script in the series, but every
piece of it uses something you've already learned:

```c
#include <max.mh>
#include <maxui.mh>
#include <rand.mh>

#define SCORE_FILE "dungeon_scores.txt"

// Room constants
#define ROOM_ENTRY   1
#define ROOM_ARCHIVE 2
#define ROOM_SERVER  3

// Game state
int: current_room;
int: has_key;
int: boss_defeated;
int: moves;
int: game_over;

void show_scores()
{
  int: fd;
  int: count;
  string: line;

  fd := open(SCORE_FILE, IOPEN_READ);

  if (fd = -1)
    return;

  print("|11─── High Scores ───|07\n");
  count := 0;

  while (readln(fd, line) = 0 and count < 5)
  {
    print("|03  ", line, "\n");
    count := count + 1;
  }

  close(fd);
  print("|11───────────────────|07\n\n");
}

void save_score()
{
  int: fd;
  string: entry;

  entry := usr.name + " — " + itostr(moves) + " moves";

  fd := open(SCORE_FILE, IOPEN_APPEND + IOPEN_CREATE);

  if (fd <> -1)
  {
    writeln(fd, entry);
    close(fd);
  }
}

void describe_room()
{
  print("\f");
  print("|11═══════════════════════════════════════\n");

  if (current_room = ROOM_ENTRY)
  {
    print("|14  The Entry Hall\n");
    print("|11═══════════════════════════════════════|07\n\n");
    print("|03You stand in a dim hallway. The walls are lined\n");
    print("with faded printouts of login screens from boards\n");
    print("that no longer exist.\n\n");
    print("|07A |15note|07 on the wall reads:\n");
    print("|06  \"The server room holds the way out.\n");
    print("   But you'll need the key.\"\n\n");
    print("|03There is a passage |14east|03 to the Archive Room.\n");
  }
  else if (current_room = ROOM_ARCHIVE)
  {
    print("|14  The Archive Room\n");
    print("|11═══════════════════════════════════════|07\n\n");
    print("|03Shelves sag under the weight of a thousand\n");
    print("floppy disks. The labels are all handwritten.\n");
    print("Someone organized these by |15vibe|03.\n\n");

    if (has_key = 0)
    {
      print("|07A |15brass key|07 glints on the floor between the\n");
      print("shelves.\n\n");
    }
    else
      print("|08(The key is gone. You have it.)|07\n\n");

    print("|03Passages lead |14west|03 (Entry Hall) and\n");
    print("|14north|03 (Server Room).\n");
  }
  else if (current_room = ROOM_SERVER)
  {
    print("|14  The Server Room\n");
    print("|11═══════════════════════════════════════|07\n\n");

    if (boss_defeated)
    {
      print("|10The terminal is dark. The door is open.\n");
      print("You can see daylight beyond it.|07\n\n");
    }
    else
    {
      print("|03A massive CRT terminal hums in the center of\n");
      print("the room. Green text scrolls across its face.\n\n");
      print("|15It's waiting for you to answer its question.|07\n\n");
    }

    print("|03A passage leads |14south|03 (Archive Room).\n");
  }

  print("\n|08Moves: ", moves, "|07\n");
}

int room_menu()
{
  struct ui_lightbar_style: style;
  int: choice;

  ui_lightbar_style_default(style);
  style.normal_attr   := ui_make_attr(UI_CYAN, UI_BLACK);
  style.selected_attr := ui_make_attr(UI_WHITE, UI_BLUE);
  style.wrap := 1;
  style.show_brackets := UI_BRACKET_SQUARE;

  if (current_room = ROOM_ENTRY)
  {
    array [1..2] of string: items;
    items[1] := "Go East (Archive Room)";
    items[2] := "Look around";
    choice := ui_lightbar(items, 2, 3, 18, 28, style);
    return choice;
  }
  else if (current_room = ROOM_ARCHIVE)
  {
    if (has_key = 0)
    {
      array [1..4] of string: items;
      items[1] := "Go West (Entry Hall)";
      items[2] := "Go North (Server Room)";
      items[3] := "Pick up the key";
      items[4] := "Look around";
      choice := ui_lightbar(items, 4, 3, 18, 28, style);
      return choice + 10;
    }
    else
    {
      array [1..3] of string: items;
      items[1] := "Go West (Entry Hall)";
      items[2] := "Go North (Server Room)";
      items[3] := "Look around";
      choice := ui_lightbar(items, 3, 3, 18, 28, style);
      // Remap: 1=11, 2=12, 3=14
      if (choice = 3)
        return 14;
      return choice + 10;
    }
  }
  else if (current_room = ROOM_SERVER)
  {
    if (boss_defeated)
    {
      array [1..2] of string: items;
      items[1] := "Go South (Archive Room)";
      items[2] := "Walk through the exit";
      choice := ui_lightbar(items, 2, 3, 18, 28, style);
      return choice + 20;
    }
    else
    {
      array [1..2] of string: items;
      items[1] := "Go South (Archive Room)";
      items[2] := "Approach the terminal";
      choice := ui_lightbar(items, 2, 3, 18, 28, style);
      return choice + 20;
    }
  }

  return 0;
}

void boss_fight()
{
  array [1..3] of string: questions;
  array [1..3] of string: answers;
  int: q;
  string: reply;

  questions[1] := "What does BBS stand for?";
  answers[1]   := "BULLETIN BOARD SYSTEM";

  questions[2] := "What year was FidoNet created?";
  answers[2]   := "1984";

  questions[3] := "What BBS software are you running right now?";
  answers[3]   := "MAXIMUS";

  q := (rand() % 3) + 1;

  print("\n|15The terminal flickers. Text appears:\n\n");
  print("|14  \"ANSWER MY QUESTION TO PASS.\"\n\n");
  print("|07  ", questions[q], "\n\n");
  input_str(reply, INPUT_NLB_LINE, 0, 60, "|11> |15");

  if (strupper(reply) = answers[q])
  {
    print("\n|10The terminal goes dark. A door slides open behind it.\n");
    print("A draft of fresh air hits your face.|07\n\n");
    boss_defeated := 1;
  }
  else
  {
    print("\n|12\"INCORRECT.\"\n");
    print("|03The terminal buzzes angrily. You step back.|07\n\n");
  }

  print("|08(press any key)|07");
  getch();
}

int main()
{
  int: action;

  srand(time());

  current_room := ROOM_ENTRY;
  has_key := 0;
  boss_defeated := 0;
  moves := 0;
  game_over := 0;

  print("\f");
  print("|11═══════════════════════════════════════\n");
  print("|14  The Dungeon of Forgotten Logins\n");
  print("|11═══════════════════════════════════════|07\n\n");
  print("|03Welcome, |15", usr.name, "|03.\n\n");
  print("|07You wake up in a hallway that smells like\n");
  print("warm electronics and forgotten passwords.\n");
  print("There is no going back. Only forward.\n\n");

  show_scores();

  print("|08(press any key to begin)|07");
  getch();

  // === Main game loop ===
  while (game_over = 0)
  {
    describe_room();
    action := room_menu();

    if (action = 0)
    {
      // Escape pressed — confirm quit
      print("\n|14Quit the game? |11[|15Y|11/|15N|11] ");

      if (input_ch(CINPUT_DISPLAY + CINPUT_ACCEPTABLE, "YyNn") = 'Y')
      {
        game_over := 1;
      }
      else
        print("\n");
    }
    // Entry Hall actions
    else if (action = 1)
    {
      current_room := ROOM_ARCHIVE;
      moves := moves + 1;
    }
    else if (action = 2)
    {
      print("\n|03The printouts are mostly login prompts.\n");
      print("One says: |06\"SysOp: CALL BACK AFTER 6PM\"|07\n");
      print("|08(press any key)|07");
      getch();
    }
    // Archive Room actions
    else if (action = 11)
    {
      current_room := ROOM_ENTRY;
      moves := moves + 1;
    }
    else if (action = 12)
    {
      if (has_key = 0)
      {
        print("\n|12The door to the Server Room won't budge.\n");
        print("|03Maybe you need something...|07\n");
        print("|08(press any key)|07");
        getch();
      }
      else
      {
        print("\n|10The brass key turns. The door opens.|07\n");
        print("|08(press any key)|07");
        getch();
        current_room := ROOM_SERVER;
        moves := moves + 1;
      }
    }
    else if (action = 13)
    {
      print("\n|10You pick up the brass key.|07\n");
      print("|03It's warm. Engraved on it: |06\"ENTER\"|07\n");
      print("|08(press any key)|07");
      getch();
      has_key := 1;
      moves := moves + 1;
    }
    else if (action = 14)
    {
      print("\n|03The floppy labels read things like:\n");
      print("|06  \"DOOM (disk 7 of 12)\"");
      print("  \"TAX STUFF??\"");
      print("  \"DO NOT FORMAT\"|07\n");
      print("|08(press any key)|07");
      getch();
    }
    // Server Room actions
    else if (action = 21)
    {
      current_room := ROOM_ARCHIVE;
      moves := moves + 1;
    }
    else if (action = 22)
    {
      if (boss_defeated)
      {
        // Walk through the exit — you win!
        game_over := 1;
        moves := moves + 1;

        print("\f");
        print("|11═══════════════════════════════════════\n");
        print("|10  YOU ESCAPED!\n");
        print("|11═══════════════════════════════════════|07\n\n");
        print("|03Congratulations, |15", usr.name, "|03.\n\n");
        print("|07You made it out of the Dungeon of Forgotten\n");
        print("Logins in |15", moves, "|07 moves.\n\n");

        save_score();
        print("|10Score saved!|07\n\n");
        show_scores();
      }
      else
      {
        moves := moves + 1;
        boss_fight();
      }
    }
  }

  if (boss_defeated = 0)
  {
    print("\n|06You leave the dungeon unfinished.\n");
    print("|03The terminal will remember you, |15",
          usr.name, "|03.\n\n|07");
  }

  return 0;
}
```

## Walkthrough: How It All Fits Together

### Game State as Global Variables

```c
int: current_room;
int: has_key;
int: boss_defeated;
int: moves;
int: game_over;
```

Five variables describe the entire world. `current_room` is a number
(1, 2, or 3). `has_key` and `boss_defeated` are boolean flags (0 or 1).
`moves` is a counter. `game_over` controls the main loop.

This is intentionally simple. For a bigger game you'd use a struct —
but for three rooms and a key, bare variables are clearer.

### The Game Loop (Lesson 5)

```c
while (game_over = 0)
{
  describe_room();
  action := room_menu();
  // ... handle action ...
}
```

Display, input, update. Every game loop ever written. The `while` loop
runs until `game_over` is set to `1` — either the player wins or quits.

### Room Descriptions (Lesson 4)

`describe_room()` is a big `if`/`else if` chain. Each room has its own
text, and the text changes based on state — the Archive Room's description
changes depending on whether the key has been picked up. Decisions inside
decisions.

### Lightbar Menus (Lesson 7)

`room_menu()` builds a different lightbar for each room. The Entry Hall
has two options. The Archive Room has three or four (depending on the
key). The Server Room has two (but the second option changes after the
boss is defeated).

The return values are encoded with offsets (`+10` for Archive, `+20` for
Server) so the main loop can distinguish "Go West from the Archive" from
"Go East from the Entry." This is a common pattern — encode both the
room and the action into one integer.

### The Boss Fight (Lessons 4 + 5)

`boss_fight()` picks a random trivia question from a small array, reads
the player's answer, and compares it (case-insensitive with `strupper()`).
This is Lesson 4's trivia game in miniature, wrapped in a function.

The random selection uses `rand() % 3` from `rand.mh` (Lesson 5). The
arrays use syntax from Lesson 7.

### High Scores (Lesson 6)

`save_score()` appends one line to `dungeon_scores.txt` using
`IOPEN_APPEND + IOPEN_CREATE`. `show_scores()` reads the first five
lines and displays them. Both functions are straight from Lesson 6's
guestbook pattern.

### Personalization (Lesson 3)

`usr.name` appears in the intro, the victory screen, and the quit
message. Small touch, big effect. The caller feels like the game knows
them.

### The `getch()` Pause (Lesson 9)

After every narrative beat — picking up the key, failing the boss,
looking around — the game pauses with `getch()` so the text doesn't
scroll away. This is pacing. Without it, the game would feel like a
wall of text.

## Ideas for Extending It

This is a *framework*, not just a game. Some things you could add:

- **More rooms.** Add a room constant, a new block in `describe_room()`,
  a new block in `room_menu()`, and new action handlers. The pattern
  scales.
- **An inventory system.** Replace `has_key` with an array of items.
  Check the array when the player tries to use something.
- **Multiple boss questions.** The boss already picks randomly from
  three — add ten more. Or fetch them from an API (Lesson 9).
- **A timer.** Use `time()` to track how long the game took. Display
  elapsed time on the victory screen.
- **A leaderboard.** Read the score file, sort by moves, display a
  ranked table. The two-pass read from Lesson 6 handles this.
- **Sound effects.** Use `display_file()` to show an ANSI file with
  embedded beep characters for dramatic moments.

## What You've Learned (All of It)

Over ten lessons, you've gone from zero to a full game. Here's the
complete MEX toolkit:

| Lesson | Concept | Key Functions |
|--------|---------|--------------|
| 1 | Output, variables, pipe colors | `print()`, `usr.name` |
| 2 | Input, strings, conversation | `input_str()`, `input_ch()` |
| 3 | User record, helper functions | `usr.*`, custom functions |
| 4 | Decisions, comparison, logic | `if`/`else`, `strupper()`, `strfind()` |
| 5 | Loops, random numbers | `for`, `while`, `do..while`, `rand()` |
| 6 | File I/O, persistence | `open()`, `readln()`, `writeln()`, `close()` |
| 7 | UI, lightbar menus, arrays | `ui_lightbar()`, `ui_select_prompt()` |
| 8 | Message base, built-in commands | `msgareafindfirst()`, `menu_cmd()` |
| 9 | HTTP, JSON, live data | `http_request()`, `json_open()`, `json_get_str()` |
| 10 | Assembly: putting it all together | Everything above, in one script |

## 5:17 AM

The sky outside your window is turning that particular shade of blue that
means you've been at this all night. The modem light is still blinking.
Your board has a greeting, a guestbook, a trivia game, a guessing game,
a dashboard, a joke fetcher, a lightbar toolbox, and now a dungeon crawler.

You built all of that in one session. In a language that runs inside a
BBS. On a system that most people think died in 1995.

They're wrong, obviously.

Go to bed. Your callers will find the dungeon in the morning. And when
they do — when someone posts in the General echo that they escaped in
twelve moves — you'll know exactly how it works. Because you wrote it.
At 5 AM. With a cup of cold coffee and a text editor.

Welcome to MEX. Welcome to sysop life.

---

*This concludes the Learning MEX tutorial series. For detailed reference
on every function mentioned in these lessons, see the
[MEX Reference]({{ site.baseurl }}{% link mex-getting-started.md %}).*
