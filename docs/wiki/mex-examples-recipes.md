---
layout: default
title: "Examples & Recipes"
section: "mex"
description: "Copy-paste MEX patterns for common BBS tasks — bulletins, guestbooks, menus, user lists, and more"
---

This page is a cookbook. Each recipe is a self-contained MEX script (or
pattern) that you can drop into your BBS, tweak the colors and labels, and
run. They're organized by what you're trying to do, not by which language
feature they demonstrate.

If you're looking for a guided walkthrough that builds up concepts one at a
time, start with the [Learning MEX]({{ site.baseurl }}{% link mex-getting-started.md %})
tutorial series instead. If you need the full reference for a specific
function, the [Standard Intrinsics]({{ site.baseurl }}{% link mex-standard-intrinsics.md %})
hub will point you to the right page.

---

## On This Page

- [Bulletin of the Day](#bulletin)
- [Guestbook](#guestbook)
- [Who's Online](#whos-online)
- [User List with Paging](#user-list)
- [Lightbar Main Menu](#lightbar-menu)
- [Registration Form](#registration-form)
- [Last Callers](#last-callers)
- [Quote of the Day (Random)](#qotd)
- [Timed Idle Kick](#idle-kick)
- [File Search Across All Areas](#file-search)

---

## Bulletin of the Day {#bulletin}

Display a different `.ans` file based on the day of the week. Put your
bulletins in `bull1.ans` through `bull7.ans`.

```mex
#include <max.mh>

void main()
{
  struct _stamp: now;
  int: dow;
  string: fname;

  timestamp(now);
  dow := now.date.dw;   // 0=Sunday, 1=Monday, ... 6=Saturday

  fname := "bull" + itostr(dow + 1) + ".ans";

  if (fileexists(fname))
  {
    char: nonstop;
    nonstop := False;
    display_file(fname, nonstop);
  }
  else
    print("No bulletin for today.\n");
}
```

---

## Guestbook {#guestbook}

Let callers leave a one-line message. Display the last 20 entries, then
prompt for a new one.

```mex
#include <max.mh>

#define GUESTBOOK "guestbook.txt"
#define MAX_SHOW  20

void show_entries()
{
  int: fd;
  string: line;
  string: lines[MAX_SHOW];
  int: count, i;

  count := 0;
  fd := open(GUESTBOOK, IOPEN_READ);
  if (fd = -1)
    return;

  // Read all lines, keep the last MAX_SHOW
  while (readln(fd, line) = 0)
  {
    count := count + 1;
    lines[((count - 1) % MAX_SHOW) + 1] := line;
  }
  close(fd);

  if (count = 0)
  {
    print("No entries yet — be the first!\n\n");
    return;
  }

  // Display them in order
  int: start, total;
  if (count > MAX_SHOW)
    total := MAX_SHOW;
  else
    total := count;

  start := ((count - total) % MAX_SHOW) + 1;
  for (i := 0; i < total; i := i + 1)
  {
    int: idx;
    idx := ((start - 1 + i) % MAX_SHOW) + 1;
    print(COL_CYAN, lines[idx], "\n");
  }
  print("\n");
}

void main()
{
  string: entry;

  print(AVATAR_CLS);
  print(COL_YELLOW, "=== Guestbook ===\n\n", COL_GRAY);

  show_entries();

  input_str(entry, INPUT_NLB_LINE, 0, 70, "Leave a message (Enter to skip): ");

  if (strlen(strtrim(entry, " ")) > 0)
  {
    int: fd;
    fd := open(GUESTBOOK, IOPEN_CREATE | IOPEN_APPEND);
    if (fd <> -1)
    {
      struct _stamp: now;
      timestamp(now);
      writeln(fd, stamp_string(now) + " " + usr.name + ": " + entry);
      close(fd);
      print(COL_LGREEN, "Thanks for signing!\n", COL_GRAY);
    }
  }
}
```

---

## Who's Online {#whos-online}

Check all nodes and display who's logged in:

```mex
#include <max.mh>

void main()
{
  struct _cstat: cs;
  int: node;

  print(COL_YELLOW, "Who's Online\n");
  print(COL_GRAY, strpad("Node", 6, ' ') + strpad("User", 26, ' ') + "Status\n");
  print(strpad("", 50, '-') + "\n");

  for (node := 0; node < 10; node := node + 1)
  {
    cs.task_num := node;
    if (chat_querystatus(cs) = 0)
    {
      if (strlen(cs.username) > 0)
      {
        print(strpadleft(itostr(cs.task_num), 4, ' ') + "  ");
        print(strpad(cs.username, 26, ' '));
        print(cs.status + "\n");
      }
    }
  }
}
```

---

## User List with Paging {#user-list}

Paginated display of all users with "More?" prompting:

```mex
#include <max.mh>

void main()
{
  struct _usr: u;
  char: nonstop;
  int: count;

  nonstop := False;
  count := 0;

  print(AVATAR_CLS);
  print(COL_YELLOW, strpad("Name", 26, ' ') + strpad("City", 26, ' '));
  print("Calls\n", COL_GRAY);
  print(strpad("", 60, '-') + "\n");

  reset_more(nonstop);

  if (userfindopen("", "", u) = 0)
  {
    do
    {
      print(strpad(u.name, 26, ' '));
      print(strpad(u.city, 26, ' '));
      print(uitostr(u.times) + "\n");
      count := count + 1;

      if (do_more(nonstop, COL_CYAN) = 0)
        goto done;
    }
    while (userfindnext(u) = 0);
  }

done:
  userfindclose();
  print("\n" + COL_LGREEN + itostr(count) + " users listed.\n" + COL_GRAY);
}
```

---

## Lightbar Main Menu {#lightbar-menu}

A full-screen lightbar menu that dispatches to built-in Maximus commands:

```mex
#include <max.mh>
#include <maxui.mh>
#include <max_menu.mh>

void main()
{
  string: items[6];
  struct ui_lightbar_style: ls;
  int: choice;

  items[1] := "Read Messages";
  items[2] := "File Areas";
  items[3] := "Who's Online";
  items[4] := "User List";
  items[5] := "Change Settings";
  items[6] := "Goodbye";

  ui_lightbar_style_default(ls);
  ls.normal_attr   := UI_GRAY;
  ls.selected_attr := UI_YELLOWONBLUE;
  ls.wrap          := 1;

  while (1)
  {
    print(AVATAR_CLS);
    ui_fill_rect(1, 1, 80, 1, ' ', UI_CYAN);
    ui_write_padded(1, 1, 80, "[ Main Menu ]", UI_CYAN);

    ui_goto(4, 5);
    ui_set_attr(UI_YELLOW);
    print("Choose an option:");

    choice := ui_lightbar(items, 6, 6, 10, 30, ls);

    if (choice = 1)
      menu_cmd(MNU_SAME_DIRECTION, "");
    else if (choice = 2)
      menu_cmd(MNU_FILE_AREA, "");
    else if (choice = 3)
      menu_cmd(MNU_CHAT_WHO_IS_ON, "");
    else if (choice = 4)
      menu_cmd(MNU_USERLIST, "");
    else if (choice = 5)
      menu_cmd(MNU_USER_EDITOR, "");
    else if (choice = 6 or choice = -1)
    {
      menu_cmd(MNU_GOODBYE, "");
      return;
    }
  }
}
```

---

## Registration Form {#registration-form}

Multi-field form using `ui_form_run`:

```mex
#include <max.mh>
#include <maxui.mh>

void main()
{
  struct ui_form_field: fields[4];
  struct ui_form_style: fs;
  int: result;

  // Set up form style
  ui_form_style_default(fs);
  fs.label_attr  := UI_YELLOW;
  fs.normal_attr := UI_GRAY;
  fs.focus_attr  := UI_YELLOWONBLUE;
  fs.wrap        := 1;

  // Define fields
  fields[1].name      := "name";
  fields[1].label     := "Full Name:";
  fields[1].x         := 15;
  fields[1].y         := 5;
  fields[1].width     := 30;
  fields[1].max_len   := 50;
  fields[1].field_type := UI_FIELD_TEXT;
  fields[1].required  := 1;
  fields[1].value     := usr.name;

  fields[2].name      := "city";
  fields[2].label     := "City:";
  fields[2].x         := 15;
  fields[2].y         := 7;
  fields[2].width     := 30;
  fields[2].max_len   := 40;
  fields[2].field_type := UI_FIELD_TEXT;
  fields[2].required  := 1;
  fields[2].value     := usr.city;

  fields[3].name      := "phone";
  fields[3].label     := "Phone:";
  fields[3].x         := 15;
  fields[3].y         := 9;
  fields[3].width     := 14;
  fields[3].max_len   := 14;
  fields[3].field_type := UI_FIELD_FORMAT;
  fields[3].format_mask := "(###) ###-####";
  fields[3].required  := 0;
  fields[3].value     := "";

  fields[4].name      := "email";
  fields[4].label     := "Email:";
  fields[4].x         := 15;
  fields[4].y         := 11;
  fields[4].width     := 40;
  fields[4].max_len   := 60;
  fields[4].field_type := UI_FIELD_TEXT;
  fields[4].required  := 1;
  fields[4].value     := "";

  // Draw the screen
  print(AVATAR_CLS);
  ui_fill_rect(1, 1, 80, 1, ' ', UI_CYAN);
  ui_write_padded(1, 1, 80, "[ New User Registration ]", UI_CYAN);

  ui_goto(3, 5);
  ui_set_attr(UI_GRAY);
  print("Fill in your details. Ctrl+S to save, Escape to cancel.");

  // Draw labels
  ui_write_padded(5, 3, 11, "Full Name:", UI_YELLOW);
  ui_write_padded(7, 3, 11, "City:", UI_YELLOW);
  ui_write_padded(9, 3, 11, "Phone:", UI_YELLOW);
  ui_write_padded(11, 3, 11, "Email:", UI_YELLOW);

  // Run the form
  result := ui_form_run(fields, 4, fs);

  if (result = 1)
  {
    ui_goto(14, 3);
    print(COL_LGREEN, "Saved! Welcome, " + fields[1].value + ".\n");
  }
  else
  {
    ui_goto(14, 3);
    print(COL_LRED, "Registration cancelled.\n");
  }
}
```

---

## Last Callers {#last-callers}

Display the most recent callers from the caller log:

```mex
#include <max.mh>

#define SHOW_COUNT 10

void main()
{
  struct _callinfo: ci;
  long: total, i, start;

  if (call_open() <> 0)
  {
    print("Cannot open caller log.\n");
    return;
  }

  total := call_numrecs();
  if (total > SHOW_COUNT)
    start := total - SHOW_COUNT;
  else
    start := 0;

  print(COL_YELLOW);
  print(strpad("Caller", 22, ' ') + strpad("City", 20, ' '));
  print(strpad("Login", 18, ' ') + "Node\n");
  print(COL_GRAY, strpad("", 65, '-') + "\n");

  for (i := start; i < total; i := i + 1)
  {
    if (call_read(i, ci) = 0)
    {
      print(strpad(ci.name, 22, ' '));
      print(strpad(ci.city, 20, ' '));
      print(strpad(stamp_string(ci.login), 18, ' '));
      print(itostr(ci.task) + "\n");
    }
  }

  call_close();
}
```

---

## Quote of the Day (Random) {#qotd}

Read quotes from a file, pick one at random:

```mex
#include <max.mh>
#include <rand.mh>

#define QUOTEFILE "quotes.txt"

void main()
{
  int: fd, count;
  string: line;
  string: quotes[100];

  fd := open(QUOTEFILE, IOPEN_READ);
  if (fd = -1)
  {
    print("No quotes file found.\n");
    return;
  }

  count := 0;
  while (readln(fd, line) = 0 and count < 100)
  {
    if (strlen(strtrim(line, " ")) > 0)
    {
      count := count + 1;
      quotes[count] := line;
    }
  }
  close(fd);

  if (count = 0)
  {
    print("Quote file is empty.\n");
    return;
  }

  srand(time());
  int: pick;
  pick := (rand() % count) + 1;

  print("\n");
  print(COL_LCYAN, "  \"" + quotes[pick] + "\"\n", COL_GRAY);
  print("\n");
}
```

---

## Timed Idle Kick {#idle-kick}

Run a loop that checks for input. If the caller idles too long, log them
off:

```mex
#include <max.mh>

#define IDLE_LIMIT 120   // seconds

void main()
{
  unsigned long: last_input;
  char: ch;

  last_input := time();
  print("You're in the lounge. Press Q to leave.\n");

  while (1)
  {
    if (kbhit())
    {
      ch := getch();
      last_input := time();

      if (ch = 'q' or ch = 'Q')
      {
        print("See you later!\n");
        return;
      }

      print("You pressed: " + ch + "\n");
    }

    // Check idle time
    if (time() - last_input > IDLE_LIMIT)
    {
      print("\nYou've been idle too long. Goodbye!\n");
      log(usr.name + " idle-kicked from lounge.");
      menu_cmd(301, "");   // MNU_GOODBYE
      return;
    }

    sleep(1);
  }
}
```

---

## File Search Across All Areas {#file-search}

Search every file area for files matching a pattern:

```mex
#include <max.mh>

void main()
{
  string: pattern;
  struct _farea: fa;
  struct _ffind: ff;
  int: found;

  input_str(pattern, INPUT_NLB_LINE, 0, 40, "Search for filename: ");
  if (strlen(pattern) = 0)
    return;

  found := 0;

  if (fileareafindfirst(fa, "", AFFO_NODIV) = 0)
  {
    do
    {
      string: searchpath;
      searchpath := fa.downpath + pattern;

      if (filefindfirst(ff, searchpath, 0) = 0)
      {
        do
        {
          if (found = 0)
          {
            print(COL_YELLOW);
            print(strpad("Area", 16, ' ') + strpad("Filename", 16, ' '));
            print("Size\n", COL_GRAY);
            print(strpad("", 45, '-') + "\n");
          }

          print(strpad(fa.tag, 16, ' '));
          print(strpad(ff.filename, 16, ' '));
          print(ultostr(ff.filesize) + "\n");
          found := found + 1;
        }
        while (filefindnext(ff) = 0);
        filefindclose(ff);
      }
    }
    while (fileareafindnext(fa) = 0);
  }
  fileareafindclose();

  if (found = 0)
    print("No files matching '" + pattern + "' found.\n");
  else
    print("\n" + itostr(found) + " file(s) found.\n");
}
```

---

## See Also

- [Learning MEX]({{ site.baseurl }}{% link mex-getting-started.md %}) — guided tutorial series
- [Language Guide]({{ site.baseurl }}{% link mex-language-guide.md %}) — variables, control
  flow, functions, strings
- [Standard Intrinsics]({{ site.baseurl }}{% link mex-standard-intrinsics.md %}) — all built-in
  functions by category
- [UI Primitives]({{ site.baseurl }}{% link mex-ui-primitives.md %}) — screen control, fields,
  lightbars, forms
