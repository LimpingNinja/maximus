---
layout: default
title: "User & Session"
section: "mex"
description: "User record access, privilege checks, time management, class lookups, user file operations, and caller log"
---

The caller is the center of everything your BBS does. Who are they? What
privilege level do they have? How long have they been on? When does their
subscription expire? MEX gives you full read access to the user record
through the `usr` global, and a set of functions for managing time,
checking privileges, searching the user file, and reading the caller log.

The `usr` struct itself is documented on the
[Variables & Types]({% link mex-language-vars-types.md %}#user-record) page
with every field listed. This page covers the *functions* that operate on
users and sessions — the things that go beyond just reading fields.

---

## On This Page

- [Quick Reference](#quick-reference)
- [Time Management](#time)
- [Privilege & Class](#privilege)
- [User File Operations](#user-file)
- [Caller Log](#caller-log)
- [Session Info](#session-info)
- [Chat Status](#chat)
- [Modem & Carrier](#modem)

---

## Quick Reference {#quick-reference}

### Time

| Function | What It Does |
|----------|-------------|
| [`time()`](#time-func) | Current time as seconds since epoch |
| [`timeon()`](#timeon) | Seconds the caller has been online this session |
| [`timeleft()`](#timeleft) | Seconds remaining in the caller's session |
| [`timeadjust()`](#timeadjust) | Add or subtract time from the caller's session |
| [`timeadjustsoft()`](#timeadjustsoft) | Soft time adjustment (doesn't exceed daily limit) |
| [`time_check()`](#time-check) | Enable or disable time-limit checking |
| [`timestamp()`](#timestamp) | Get current date/time as a `_stamp` struct |
| [`stamp_string()`](#stamp-string) | Convert a `_stamp` to a human-readable string |
| [`stamp_to_long()`](#stamp-to-long) | Convert a `_stamp` to a `long` for comparison |
| [`long_to_stamp()`](#long-to-stamp) | Convert a `long` back to a `_stamp` |

### Privilege & Class

| Function | What It Does |
|----------|-------------|
| [`privok()`](#privok) | Check if caller meets a privilege string |
| [`class_info()`](#class-info) | Query class limits (time, downloads, etc.) |
| [`class_abbrev()`](#class-abbrev) | Get the short name for a privilege level |
| [`class_name()`](#class-name) | Get the full name for a privilege level |
| [`class_loginfile()`](#class-loginfile) | Get the login display file for a privilege level |
| [`class_to_priv()`](#class-to-priv) | Convert a class name string to a privilege number |

### User File

| Function | What It Does |
|----------|-------------|
| [`userfindopen()`](#userfindopen) | Start searching the user file |
| [`userfindnext()`](#userfindnext) | Get the next matching user |
| [`userfindprev()`](#userfindprev) | Get the previous matching user |
| [`userfindclose()`](#userfindclose) | Close the user file search |
| [`userfindseek()`](#userfindseek) | Seek to a specific user record number |
| [`userfilesize()`](#userfilesize) | Get total number of user records |
| [`userupdate()`](#userupdate) | Update a user record |
| [`usercreate()`](#usercreate) | Create a new user record |
| [`userremove()`](#userremove) | Delete a user record |

---

## Time Management {#time}

{: #time-func}
### time()

Returns the current time as an `unsigned long` — seconds since the Unix
epoch. Useful for seeding random numbers, computing elapsed time, and
comparing timestamps:

```mex
unsigned long time();
```

```mex
unsigned long: start;
start := time();
// ... do work ...
print("That took " + ultostr(time() - start) + " seconds.\n");
```

{: #timeon}
### timeon()

Seconds the caller has been online this session:

```mex
unsigned long timeon();
```

{: #timeleft}
### timeleft()

Seconds remaining before the caller's time limit expires:

```mex
unsigned long timeleft();
```

```mex
if (timeleft() < 300)
  print("You have less than 5 minutes left.\n");
```

{: #timeadjust}
### timeadjust(delta)

Add or subtract seconds from the caller's remaining time. Positive values
add time, negative values subtract. Returns the new time remaining:

```mex
long timeadjust(long: delta);
```

```mex
// Award 5 minutes for completing a survey
timeadjust(300);
print("Bonus! 5 minutes added to your session.\n");
```

{: #timeadjustsoft}
### timeadjustsoft(delta)

Like `timeadjust()`, but won't let the caller exceed their daily class
limit. Use this when you want to be generous without breaking the rules:

```mex
long timeadjustsoft(long: delta);
```

{: #time-check}
### time_check(state)

Enable or disable automatic time-limit enforcement. Pass `0` to disable,
`1` to enable. Returns the previous state:

```mex
int time_check(int: state);
```

```mex
// Disable time checking during a game
int: old;
old := time_check(0);
play_game();
time_check(old);
```

{: #timestamp}
### timestamp(stamp)

Fill a `_stamp` struct with the current date and time:

```mex
void timestamp(ref struct _stamp: stamp);
```

```mex
struct _stamp: now;
timestamp(now);
print("Current hour: " + itostr(now.time.hh) + "\n");
```

{: #stamp-string}
### stamp_string(stamp)

Convert a `_stamp` to a human-readable date/time string:

```mex
string stamp_string(ref struct _stamp: stamp);
```

```mex
print("Last call: " + stamp_string(usr.ludate) + "\n");
```

{: #stamp-to-long}
### stamp_to_long(stamp)

Convert a `_stamp` to an `unsigned long` for numeric comparison:

```mex
unsigned long stamp_to_long(ref struct _stamp: st);
```

```mex
// Check if cached data is stale (more than 3600 seconds old)
unsigned long: cached_time, now_time;
cached_time := stamp_to_long(cached_stamp);
struct _stamp: now;
timestamp(now);
now_time := stamp_to_long(now);
if (now_time - cached_time > 3600)
  print("Cache is stale.\n");
```

{: #long-to-stamp}
### long_to_stamp(time, stamp)

Convert a `long` back to a `_stamp`:

```mex
void long_to_stamp(long: time, ref struct _stamp: st);
```

---

## Privilege & Class {#privilege}

{: #privok}
### privok(privstr)

Check if the current caller meets a privilege requirement. The `privstr`
is a privilege expression (e.g., `"Sysop"`, `"Normal/K1"`):

```mex
int privok(string: privstr);
```

```mex
if (privok("Sysop"))
  print("You have sysop access.\n");
```

{: #class-info}
### class_info(priv, CIT)

Query the limits and settings for a privilege class:

```mex
long class_info(int: priv, int: CIT);
```

| Query Constant | Returns |
|----------------|---------|
| `CIT_NUMCLASSES` | Total number of defined classes (pass `-1` for `priv`) |
| `CIT_DAY_TIME` | Daily time limit (minutes) |
| `CIT_CALL_TIME` | Per-call time limit (minutes) |
| `CIT_DL_LIMIT` | Download limit (KB per day) |
| `CIT_RATIO` | Upload/download ratio |
| `CIT_MAX_CALLS` | Maximum calls per day |
| `CIT_LEVEL` | Numeric privilege level |
| `CIT_CLASSKEY` | Class key |
| `CIT_ACCESSFLAGS` | Access flags (`CFLAGA_*` constants) |
| `CIT_MAILFLAGS` | Mail flags (`CFLAGM_*` constants) |

```mex
long: daily_limit;
daily_limit := class_info(usr.priv, CIT_DAY_TIME);
print("Your daily time limit: " + ltostr(daily_limit) + " minutes.\n");
```

To query by class index instead of privilege level, OR the query type with
`CIT_BYINDEX`:

```mex
// Get name of class at index 0
long: num_classes;
num_classes := class_info(-1, CIT_NUMCLASSES);
```

{: #class-abbrev}
### class_abbrev(priv)

Returns the short abbreviation for a privilege level:

```mex
string class_abbrev(int: priv);
```

{: #class-name}
### class_name(priv)

Returns the full name for a privilege level:

```mex
string class_name(int: priv);
```

```mex
print("Your access level: " + class_name(usr.priv) + "\n");
```

{: #class-loginfile}
### class_loginfile(priv)

Returns the path to the login display file for a privilege level:

```mex
string class_loginfile(int: priv);
```

{: #class-to-priv}
### class_to_priv(s)

Convert a class name string (e.g., `"Sysop"`) to its numeric privilege
value:

```mex
unsigned int class_to_priv(string: s);
```

---

## User File Operations {#user-file}

These functions let you search, read, and modify the user database. They
work with a `_usr` struct that you provide — not the global `usr`.

{: #userfindopen}
### userfindopen(name, alias, u)

Start a user search. Pass the name and/or alias to search for (empty string
to match any). Returns `0` on success, non-zero if no match:

```mex
int userfindopen(string: name, string: alias, ref struct _usr: u);
```

```mex
struct _usr: found;
if (userfindopen("John Smith", "", found) = 0)
  print("Found: " + found.name + " from " + found.city + "\n");
userfindclose();
```

{: #userfindnext}
### userfindnext(u)

Get the next matching user. Returns `0` on success:

```mex
int userfindnext(ref struct _usr: u);
```

{: #userfindprev}
### userfindprev(u)

Get the previous matching user:

```mex
int userfindprev(ref struct _usr: u);
```

{: #userfindclose}
### userfindclose()

Close the user search. Always call this when done:

```mex
void userfindclose();
```

{: #userfindseek}
### userfindseek(rec, u)

Jump to a specific user record by record number:

```mex
int userfindseek(long: rec, ref struct _usr: u);
```

{: #userfilesize}
### userfilesize()

Returns the total number of records in the user file:

```mex
long userfilesize();
```

{: #userupdate}
### userupdate(u, origname, origalias)

Update an existing user record. Pass the original name and alias so
Maximus can find the correct record to update:

```mex
int userupdate(ref struct _usr: u, string: origname, string: origalias);
```

{: #usercreate}
### usercreate(u)

Create a new user record:

```mex
int usercreate(ref struct _usr: u);
```

{: #userremove}
### userremove(u)

Delete a user record:

```mex
int userremove(ref struct _usr: u);
```

### User List Example

```mex
struct _usr: u;
char: nonstop;

nonstop := False;
reset_more(nonstop);

if (userfindopen("", "", u) = 0)
{
  do
  {
    print(strpad(u.name, 25, ' ') + " " + u.city + "\n");
    if (do_more(nonstop, COL_CYAN) = 0)
      goto done;
  }
  while (userfindnext(u) = 0);
}

done:
userfindclose();
```

---

## Caller Log {#caller-log}

Read the historical caller log (who called, when, what they did):

```mex
int call_open();
void call_close();
long call_numrecs();
int call_read(long: recno, ref struct _callinfo: ci);
```

The `_callinfo` struct contains:

| Field | Type | Description |
|-------|------|-------------|
| `ci.name` | `string` | Caller's name |
| `ci.city` | `string` | Caller's city |
| `ci.login` | `struct _stamp` | Login time |
| `ci.logoff` | `struct _stamp` | Logoff time |
| `ci.task` | `int` | Node number |
| `ci.flags` | `unsigned int` | Call flags (`CALL_*` constants) |
| `ci.calls` | `unsigned int` | Total calls at time of login |
| `ci.read` | `unsigned int` | Messages read during call |
| `ci.posted` | `unsigned int` | Messages posted during call |

```mex
struct _callinfo: ci;
long: total, i;

if (call_open() = 0)
{
  total := call_numrecs();
  // Show last 5 callers
  for (i := total - 5; i < total; i := i + 1)
  {
    if (i >= 0 and call_read(i, ci) = 0)
      print(strpad(ci.name, 20, ' ') + " " + stamp_string(ci.login) + "\n");
  }
  call_close();
}
```

---

## Session Info {#session-info}

The `id` global (`struct _instancedata`) provides session-level information:

| Field | Type | Description |
|-------|------|-------------|
| `id.task_num` | `unsigned int` | Node/task number |
| `id.local` | `int` | Non-zero if caller is local |
| `id.port` | `int` | COM port number |
| `id.speed` | `unsigned long` | Connection speed (baud rate) |
| `id.alias_system` | `int` | Non-zero if this is an alias system |
| `id.ask_name` | `int` | Non-zero if system asks for real name |

```mex
if (id.local)
  print("You're logging in locally.\n");
else
  print("Connected at " + ultostr(id.speed) + " baud.\n");
```

---

## Chat Status {#chat}

### chat_querystatus(cstat)

Query the chat status of other nodes:

```mex
int chat_querystatus(ref struct _cstat: cstat);
```

The `_cstat` struct:

| Field | Type | Description |
|-------|------|-------------|
| `cstat.task_num` | `int` | Node number |
| `cstat.avail` | `int` | Available for chat? |
| `cstat.username` | `string` | User on that node |
| `cstat.status` | `string` | Status string |

### chatstart()

Initiate a chat session with the sysop:

```mex
void chatstart();
```

---

## Modem & Carrier {#modem}

| Function | What It Does |
|----------|-------------|
| `carrier()` | Returns non-zero if carrier detect is present |
| `dcd_check(int: state)` | Enable (1) or disable (0) carrier-detect checking |
| `mdm_command(string: cmd)` | Send a modem command string |
| `mdm_flow(int: state)` | Enable (1) or disable (0) modem flow control |
| `xfertime(int: proto, long: bytes)` | Estimate transfer time for a file |

```mex
if (not carrier())
{
  log("Caller dropped carrier during script.");
  return;
}
```

---

## See Also

- [Variables & Types — User Record]({% link mex-language-vars-types.md %}#user-record)
  — every field in the `usr` struct
- [Display & I/O]({% link mex-intrinsics-display-io.md %}) — output, input,
  and file functions
- [Message & File Areas]({% link mex-intrinsics-message-file.md %}) — area
  navigation and message access
