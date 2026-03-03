---
layout: default
section: "mex"
title: "MEX JSON"
description: "JSON parsing and manipulation from MEX scripts"
---

Every interesting script eventually needs structured data. Need to
talk to a web API, read a config blob, or assemble a structured payload? You
have a full set of JSON intrinsics backed by cJSON under the hood. No external
tools, no string hacking, no prayer.

This guide walks through the two ways to work with JSON (quick path lookups and
full cursor navigation), how to build JSON from scratch, and the gotchas you
should know about before you ship a script.

---

## Getting Started

Include the JSON header at the top of your script:

```mex
#include <json.mh>
```

That gives you every JSON function and the type constants (`JSON_STRING`,
`JSON_OBJECT`, etc.). The header lives in `scripts/include/json.mh` and is
found automatically by the compiler.

The basic flow is always the same:

1. **Open** — parse a JSON string into a handle.
2. **Read** — pull values out with path accessors or cursor navigation.
3. **Close** — free the handle when you're done.

```mex
int: jh;

jh := json_open("{\"name\":\"Kevin\",\"node\":1}");
if (jh < 0)
{
  print("Parse error!\n");
  return;
}

print("Name: " + json_get_str(jh, "name") + "\n");
json_close(jh);
```

That's it for a simple case. One function to parse, one to read, one to clean
up.

---

## Handles

Every `json_open`, `json_create`, or `json_create_array` call returns an
integer **handle** — a number from 0 to 15 that identifies your JSON tree. You
can have up to 16 handles open at once (plenty for any script). When you're
done with a handle, call `json_close` to free it.

If you forget to close a handle, don't panic — the runtime cleans up
automatically when your script exits. But it's good practice to close them
explicitly, especially in loops.

---

## Two Approaches

JSON intrinsics give you two ways to pull data out of a parsed tree. Here's
the same JSON read both ways so you can see the difference up front.

The JSON we're working with:

```json
{"name": "Kevin", "age": 42, "active": true}
```

### With Path Accessors

If you know the key names, path accessors are a one-liner per value:

```mex
int:    jh;
string: name;
long:   age;
int:    active;

jh := json_open("{\"name\":\"Kevin\",\"age\":42,\"active\":true}");

name   := json_get_str(jh, "name");       // "Kevin"
age    := json_get_num(jh, "age");         // 42
active := json_get_bool(jh, "active");     // 1

json_close(jh);
```

Straight to the point. You ask for a key by name, you get the value.

### With Cursor Navigation

If you don't know the keys — or you want to walk every field — the cursor
lets you iterate:

```mex
int:    jh;
int:    t;
string: k;
string: v;

jh := json_open("{\"name\":\"Kevin\",\"age\":42,\"active\":true}");

json_enter(jh);                            // enter root object
t := json_next(jh);                        // advance to first field
while (t <> JSON_END)
{
  k := json_key(jh);                       // field name
  v := json_str(jh);                       // value as string
  print(k + " = " + v + "\n");
  t := json_next(jh);
}

json_close(jh);
```

Output:

```
name = Kevin
age = 42
active = true
```

### When to Use Which

| Situation | Use |
|-----------|-----|
| You know the key names | Path accessors |
| You need to iterate unknown keys or arrays | Cursor |
| One-shot value lookup | Path accessors |
| Walking a tree of unknown shape | Cursor |
| Building structured output | Mix both — they don't interfere |

You can use both on the same handle. Path accessors never move the cursor, so
they work fine alongside cursor navigation.

---

## Path Accessors — The Quick Way

You've seen the basics above — `json_get_str`, `json_get_num`, `json_get_bool`
on flat keys. Where path accessors really earn their keep is reaching into
nested structures without any navigation ceremony. They take a handle and a
dotted path, return the value, and never move the cursor.

### Nested Paths

Dots walk into nested objects. Brackets index into arrays:

```mex
// Given: {"prefs":{"theme":"dark","cols":80},"tags":["sysop","dev","retro"]}

json_get_str(jh, "prefs.theme");    // "dark"
json_get_num(jh, "prefs.cols");     // 80
json_get_str(jh, "tags[0]");        // "sysop"
json_get_str(jh, "tags[2]");        // "retro"
```

You can mix dots and brackets as deep as the JSON goes:
`"users[0].prefs.theme"` works exactly like you'd expect.

### Type and Count

Two more path accessors round out the set:

```mex
int: t;
int: cnt;

t   := json_get_type(jh, "name");    // JSON_STRING
t   := json_get_type(jh, "tags");    // JSON_ARRAY
cnt := json_get_count(jh, "tags");   // 3  (array length)
cnt := json_get_count(jh, "prefs");  // 2  (object key count)
```

### When a Path Doesn't Exist

Path accessors fail silently — no crash, no error popup. You just get a safe
default:

| Function | Returns on miss |
|----------|----------------|
| `json_get_str` | `""` |
| `json_get_num` | `0` |
| `json_get_bool` | `0` |
| `json_get_type` | `JSON_INVALID` |
| `json_get_count` | `0` |

This makes it safe to probe for optional keys without checking first.

---

## Cursor Navigation — The Powerful Way

Path accessors are great when you know the key names up front. But what if you
need to iterate an array of unknown length, enumerate every key in an object,
or walk a tree you've never seen before? That's where the cursor comes in.

The cursor is a position marker inside the JSON tree. You move it with five
functions:

| Function | What it does |
|----------|-------------|
| `json_enter(jh)` | Descend into the current object or array |
| `json_next(jh)` | Advance to the next sibling; returns its type, or `JSON_END` |
| `json_exit(jh)` | Ascend back to the parent |
| `json_find(jh, key)` | Jump to a named child within the current object |
| `json_rewind(jh)` | Reset to before-first-child (iterate again) |

The key mental model: **enter puts you before the first child**. You call
`json_next` to land on each child in turn. When `json_next` returns
`JSON_END`, you've seen them all.

### Iterating an Array

```mex
// Given: {"items":[{"id":1,"label":"Mail"},{"id":2,"label":"Files"}]}

jh := json_open(json_text);

json_enter(jh);                      // enter root object
json_find(jh, "items");              // cursor now at "items" array
json_enter(jh);                      // enter the array

t := json_next(jh);                  // advance to first element
while (t <> JSON_END)
{
  json_enter(jh);                    // enter this item object
  json_find(jh, "id");
  id := json_num(jh);               // read value at cursor
  json_find(jh, "label");
  label := json_str(jh);
  json_exit(jh);                     // back to array level

  print("Item #" + ltostr(id) + ": " + label + "\n");
  t := json_next(jh);               // advance to next element
}

json_close(jh);
```

Notice the rhythm: **enter, next-in-a-loop, exit**. That pattern works for
arrays and objects alike.

### Enumerating Object Keys

Same pattern, but now `json_key` gives you each key name:

```mex
// Given: {"settings":{"theme":"dark","lang":"en","node":3}}

json_enter(jh);                      // root object
json_find(jh, "settings");
json_enter(jh);                      // settings object

t := json_next(jh);
while (t <> JSON_END)
{
  k := json_key(jh);
  v := json_str(jh);                 // auto-converts numbers too
  print(k + " = " + v + "\n");
  t := json_next(jh);
}
```

Output:

```
theme = dark
lang = en
node = 3
```

### Reading Values at the Cursor

Once the cursor is on a node, you can read it:

| Function | Returns |
|----------|---------|
| `json_type(jh)` | Type constant (`JSON_STRING`, `JSON_NUMBER`, etc.) |
| `json_key(jh)` | Key name (empty string if inside an array) |
| `json_str(jh)` | Value as string — auto-converts numbers and booleans |
| `json_num(jh)` | Value as `long` (0 if not a number) |
| `json_bool(jh)` | 0 or 1 (0 if not a boolean) |
| `json_count(jh)` | Child count if cursor is on a container |

`json_str` is the Swiss army knife — it converts anything to a printable
string. Numbers come out as `"42"`, booleans as `"true"` or `"false"`, nulls
as `"null"`.

### Rewind

Hit the end of an array and want to go again? `json_rewind` resets the cursor
to before-first-child without leaving the container:

```mex
json_enter(jh);           // enter array
// ... iterate to JSON_END ...
json_rewind(jh);          // back to start
t := json_next(jh);       // first element again
```

---

## Building JSON From Scratch

Need to construct a JSON payload — say, for a future API call or a config
blob? Start with `json_create` (for an object) or `json_create_array` (for an
array), then add fields:

```mex
int:    jh;
string: out;

jh := json_create();

// Add top-level fields
json_set_str(jh, "username", "Kevin");
json_set_num(jh, "node", 1);
json_set_bool(jh, "active", 1);

// Add a nested object
json_add_object(jh, "prefs");
json_enter(jh);
json_find(jh, "prefs");
json_enter(jh);
json_set_str(jh, "theme", "dark");
json_set_num(jh, "width", 80);
json_exit(jh);
json_exit(jh);

// Add an array
json_add_array(jh, "tags");
json_enter(jh);
json_find(jh, "tags");
json_enter(jh);
json_array_push_str(jh, "sysop");
json_array_push_str(jh, "dev");
json_array_push_num(jh, 42);
json_exit(jh);
json_exit(jh);

// Serialize to a string
out := json_serialize(jh);
print(out + "\n");

json_close(jh);
```

The `json_set_*` functions work on the current object context. For nested
objects, you navigate in with `json_enter` / `json_find`, add your fields,
then `json_exit` back. For arrays, `json_array_push_*` appends to whichever
array the cursor is inside.

---

## Error Handling

The JSON intrinsics are designed to fail gracefully rather than crash your
script.

**Parse errors:** `json_open` returns `-1` if the string isn't valid JSON.
Always check.

```mex
jh := json_open(user_input);
if (jh < 0)
{
  print("That's not valid JSON.\n");
  return;
}
```

**Bad handles:** Passing an invalid or already-closed handle to any function
returns a safe default (`""`, `0`, `JSON_INVALID`, or `-1` depending on the
function). No crash.

**Missing paths:** Path accessors return empty/zero on miss (see the table
above). You don't need to check `json_get_type` before every `json_get_str`
unless you specifically care about the difference between "key exists but is
empty" and "key doesn't exist."

**Cursor errors:** `json_enter` on a non-container returns `-1`. `json_exit`
at root depth returns `-1`. `json_find` on a missing key returns `-1`. Check
the return value if you need to branch on structure.

---

## Type Constants

These are defined in `json.mh` and returned by `json_type`, `json_next`, and
`json_get_type`:

| Constant | Value | Meaning |
|----------|-------|---------|
| `JSON_NULL` | 0 | Null value |
| `JSON_BOOL` | 1 | Boolean (`true` / `false`) |
| `JSON_NUMBER` | 2 | Numeric value (integers only — MEX has no floats) |
| `JSON_STRING` | 3 | String value |
| `JSON_ARRAY` | 4 | Array container |
| `JSON_OBJECT` | 5 | Object container |
| `JSON_END` | -1 | No more siblings (iteration complete) |
| `JSON_INVALID` | -2 | Invalid handle or path |

---

## Limits and Gotchas

A few things to keep in mind:

- **16 handles max.** You can have up to 16 JSON trees open simultaneously.
  That's more than enough for any reasonable script. If you somehow exhaust
  them, `json_open` and `json_create` return `-1`.

- **16 levels of nesting.** The cursor stack supports 16 levels of
  `json_enter` depth. Real-world JSON rarely goes deeper than 4 or 5, so this
  shouldn't bite you.

- **No floats.** MEX doesn't have a floating-point type. `json_num` and
  `json_get_num` return `long` — fractional values are truncated. If you need
  the original string representation of a number, use `json_str` or
  `json_get_str` instead.

- **String length.** MEX strings have a practical limit (around 256 bytes).
  Long JSON values or deeply nested `json_serialize` output may be truncated.
  For typical BBS use — config blobs, short API payloads — this is fine.

- **Variables go at the top.** MEX requires all variable declarations at the
  beginning of each function. You can't declare a variable after a statement.
  Plan your variables up front.

---

## Choosing Your Approach

| Situation | Use |
|-----------|-----|
| You know the key names | Path accessors (`json_get_str`, etc.) |
| You need to iterate an array | Cursor (`json_enter` / `json_next` / `json_exit`) |
| You need to enumerate unknown keys | Cursor with `json_key` |
| You want a one-shot value lookup | Path accessors |
| You're walking a tree of unknown shape | Cursor (recursive `dump_node` pattern) |
| Building a request body | `json_create` + `json_set_*` + `json_serialize` |

You can mix both styles on the same handle. Path accessors don't move the
cursor, so you can use them for quick lookups while navigating with the cursor
elsewhere in the tree.

---

## Complete Example: Parse and Display a User Record

Here's a realistic script that parses a JSON user record and displays it in a
formatted way:

```mex
#include <max.mh>
#include <json.mh>

void main()
{
  string: json_text;
  int:    jh;
  int:    t;
  int:    i;
  long:   n;
  string: name;
  string: city;
  long:   calls;
  string: tag;

  json_text := "{\"name\":\"Kevin\",\"city\":\"Portland\",\"calls\":1337,\"tags\":[\"sysop\",\"dev\"]}";

  jh := json_open(json_text);
  if (jh < 0)
  {
    print("Parse error.\n");
    return;
  }

  // Quick lookups with path accessors
  name  := json_get_str(jh, "name");
  city  := json_get_str(jh, "city");
  calls := json_get_num(jh, "calls");

  print("User:  " + name + "\n");
  print("City:  " + city + "\n");
  print("Calls: " + ltostr(calls) + "\n");

  // Iterate the tags array with cursor
  print("Tags:  ");
  json_enter(jh);
  json_find(jh, "tags");
  json_enter(jh);

  i := 0;
  t := json_next(jh);
  while (t <> JSON_END)
  {
    if (i > 0)
      print(", ");
    print(json_str(jh));
    i := i + 1;
    t := json_next(jh);
  }
  print("\n");

  json_close(jh);
}
```

Output:

```
User:  Kevin
City:  Portland
Calls: 1337
Tags:  sysop, dev
```

---

## The Test Suite

A menu-driven test script ships with Maximus at `scripts/jsontest.mex` (source
in `resources/m/jsontest.mex`). It exercises every intrinsic — path accessors,
cursor iteration, key enumeration, rewind, building, serialization, error
handling, and auto-conversion. There's also a live parser mode where you can
type in JSON and see the parsed tree.

Run it from the MEX Scripts menu (option **E**) or compile it yourself:

```bash
./scripts/compile-mex.sh jsontest --deploy
```

---

## Quick Reference

| Function | Returns | Purpose |
|----------|---------|---------|
| `json_open(text)` | handle or -1 | Parse a JSON string |
| `json_create()` | handle or -1 | Create an empty object |
| `json_create_array()` | handle or -1 | Create an empty array |
| `json_close(jh)` | — | Free a handle |
| `json_enter(jh)` | 0 or -1 | Descend into container |
| `json_next(jh)` | type or `JSON_END` | Advance to next sibling |
| `json_exit(jh)` | 0 or -1 | Ascend to parent |
| `json_find(jh, key)` | 0 or -1 | Jump to named child |
| `json_rewind(jh)` | — | Reset to before-first-child |
| `json_type(jh)` | type constant | Type at cursor |
| `json_key(jh)` | string | Key name at cursor |
| `json_str(jh)` | string | Value as string (auto-converts) |
| `json_num(jh)` | long | Value as number |
| `json_bool(jh)` | int | Value as boolean |
| `json_count(jh)` | int | Child count at cursor |
| `json_get_str(jh, path)` | string | String by path |
| `json_get_num(jh, path)` | long | Number by path |
| `json_get_bool(jh, path)` | int | Boolean by path |
| `json_get_type(jh, path)` | type constant | Type by path |
| `json_get_count(jh, path)` | int | Child count by path |
| `json_set_str(jh, key, val)` | 0 or -1 | Set string on object |
| `json_set_num(jh, key, val)` | 0 or -1 | Set number on object |
| `json_set_bool(jh, key, val)` | 0 or -1 | Set boolean on object |
| `json_add_object(jh, key)` | 0 or -1 | Add child object |
| `json_add_array(jh, key)` | 0 or -1 | Add child array |
| `json_array_push_str(jh, val)` | 0 or -1 | Append string to array |
| `json_array_push_num(jh, val)` | 0 or -1 | Append number to array |
| `json_serialize(jh)` | string | Serialize tree to string |

---

## See Also

- [MEX Sockets]({% link mex-sockets.md %}) — HTTP/HTTPS requests and raw TCP
  sockets that pair naturally with JSON
- [Display Codes]({% link display-codes.md %}) — color codes for making your
  script output look sharp
- [MEX Getting Started]({% link mex-getting-started.md %}) — introduction to
  MEX scripting
