---
layout: default
title: "Reaching the Outside World: Phone Home"
section: "mex"
description: "Lesson 9 — HTTP requests, JSON, and pulling live data onto a text-mode screen"
---

*Lesson 9 of Learning MEX*

---

> **What you'll build:** A script that fetches a random joke from a public
> API and displays it on your board. Live data, from the internet, on a
> text-mode BBS. The future is absurd and we're here for it.

## 4:31 AM

Your board knows everything about itself — its message areas, its callers,
its guestbook, its high scores. But the world outside the modem line?
Invisible. A BBS is an island, and until now, MEX had no way to build a
bridge.

That changes in this lesson. MEX has HTTP support and a JSON parser.
Your scripts can make web requests, parse the responses, and bring live
data onto the terminal. Weather forecasts. Random jokes. Quotes of the
day. Anything with a public API.

A text-mode BBS that checks the internet. Your 1994 self would not
believe this.

## The HTTP Function

Everything starts with `http_request()` from `socket.mh`:

```c
#include <socket.mh>

int: status;
string: body;

status := http_request(
  "https://api.example.com/data",  // URL
  "GET",                           // method
  "",                              // extra headers
  "",                              // request body
  body,                            // response goes here
  5000                             // timeout (milliseconds)
);
```

| Parameter | What It Does |
|-----------|-------------|
| `url` | Full URL — `http://` or `https://` |
| `method` | `"GET"`, `"POST"`, `"PUT"`, `"DELETE"` |
| `headers` | Extra HTTP headers, `\r\n` separated, or `""` |
| `body` | Request body for POST/PUT, or `""` for GET |
| `response` | Passed by reference — response body lands here |
| `timeout_ms` | Timeout per I/O operation in milliseconds |

Returns the HTTP status code (`200`, `404`, `500`, etc.) or `-1` if the
connection failed entirely.

HTTPS works transparently — MEX uses mbedTLS under the hood. If the URL
starts with `https://`, the connection is encrypted automatically. No
extra setup needed.

## Parsing JSON

Most APIs return JSON. MEX's `json.mh` library parses it into a handle
you can query:

```c
#include <json.mh>

int: jh;
jh := json_open(body);

if (jh = -1)
{
  print("JSON parse error.\n");
  return 0;
}
```

### Path-Based Access

The easiest way to read values — use a dot-separated path:

```c
// Given: {"joke": {"setup": "Why?", "punchline": "Because."}}

string: setup;
string: punch;

setup := json_get_str(jh, "joke.setup");
punch := json_get_str(jh, "joke.punchline");
```

Array elements use bracket syntax:

```c
// Given: {"items": ["alpha", "bravo", "charlie"]}

string: second;
second := json_get_str(jh, "items[1]");  // "bravo" (0-indexed)
```

### Path Accessors

| Function | Returns |
|----------|---------|
| `json_get_str(jh, path)` | String value (or `""`) |
| `json_get_num(jh, path)` | Long value (or `0`) |
| `json_get_bool(jh, path)` | Int `0` or `1` |
| `json_get_type(jh, path)` | Type constant (`JSON_STRING`, etc.) |
| `json_get_count(jh, path)` | Array length or object key count |

### Always Close

```c
json_close(jh);
```

Just like files and sockets — clean up when you're done.

## Error Handling

Network code fails. Servers go down. Connections time out. DNS breaks.
Your script has to handle all of this gracefully:

```c
status := http_request(url, "GET", "", "", body, 5000);

if (status = -1)
{
  print("|12Connection failed.|07 The internet is being the internet.\n");
  return 0;
}

if (status <> 200)
{
  print("|12API returned status ", status, ".|07\n");
  return 0;
}

jh := json_open(body);

if (jh = -1)
{
  print("|12Couldn't parse the response.|07\n");
  return 0;
}
```

Three checks:

1. **`status = -1`** — connection failed entirely (timeout, DNS, refused)
2. **`status <> 200`** — server responded but with an error
3. **`jh = -1`** — response arrived but isn't valid JSON

Never assume the network will cooperate. Always check.

## The Joke Fetcher

Here's the full script. It fetches a random joke from a public API and
displays it with a dramatic pause. Call it `joke.mex`:

```c
#include <max.mh>
#include <socket.mh>
#include <json.mh>

int main()
{
  int: status;
  int: jh;
  string: body;
  string: setup;
  string: punch;

  print("\n|11═══════════════════════════════════════\n");
  print("|14  Joke of the Day\n");
  print("|11═══════════════════════════════════════|07\n\n");

  print("|03Reaching out to the internet...\n");

  status := http_request(
    "https://official-joke-api.appspot.com/random_joke",
    "GET", "", "", body, 5000
  );

  if (status = -1)
  {
    print("|12Couldn't connect.|07 The tubes are clogged.\n\n");
    return 0;
  }

  if (status <> 200)
  {
    print("|12API returned ", status, ".|07 Try again later.\n\n");
    return 0;
  }

  jh := json_open(body);

  if (jh = -1)
  {
    print("|12Couldn't parse response.|07 Blame the API.\n\n");
    return 0;
  }

  // The API returns: {"type":"...", "setup":"...", "punchline":"..."}
  setup := json_get_str(jh, "setup");
  punch := json_get_str(jh, "punchline");

  json_close(jh);

  if (setup = "")
  {
    print("|12No joke found.|07 The irony.\n\n");
    return 0;
  }

  // Display with dramatic pause
  print("\n|15", setup, "|07\n\n");
  print("|08(press any key for the punchline)|07");
  getch();
  print("\r                                    \r");
  print("|10", punch, "|07\n\n");

  return 0;
}
```

### What's New Here

**`http_request()`** does the entire HTTP transaction in one call — DNS
resolution, TCP connect, TLS handshake (for HTTPS), sending the request,
reading the response. The response body lands in the `body` string.

**`json_open()` / `json_get_str()` / `json_close()`** — parse, extract,
clean up. The path accessor `json_get_str(jh, "setup")` reaches into the
JSON object and pulls out the `"setup"` field as a string. No cursor
navigation needed for simple cases.

**`getch()`** reads a single keypress without echoing or requiring Enter.
We use it for the dramatic pause — "press any key for the punchline."

**`\r` (carriage return)** moves the cursor back to the start of the line
without advancing. We use it to overwrite the "(press any key...)" prompt
with spaces, then overwrite the spaces with the punchline. Cheap animation.

**Defensive error handling.** Four checks before we trust the data: HTTP
connection, status code, JSON parse, and the actual field existing. Any
failure gets a human-readable message instead of a crash.

## Caching with Files

If you're fetching from the same API often, don't hammer it on every call.
Cache the result to a file and only re-fetch periodically:

```c
#define CACHE_FILE "joke_cache.txt"

int is_cache_fresh()
{
  struct _stamp: fdate;
  struct _stamp: now;
  long: file_time;
  long: cur_time;

  if (fileexists(CACHE_FILE) = 0)
    return 0;

  filedate(CACHE_FILE, fdate);
  file_time := stamp_to_long(fdate);
  cur_time := time();

  // Cache for 1 hour (3600 seconds)
  if (cur_time - file_time > 3600)
    return 0;

  return 1;
}
```

Combine this with `readln()` / `writeln()` from Lesson 6 to save and load
the cached response. Your callers get instant results, and the API
provider doesn't block your IP.

## Building POST Requests

Some APIs need you to send data. Use `json.mh` to build the request body:

```c
int: req;

req := json_create();
json_set_str(req, "name", usr.name);
json_set_str(req, "board", "My Awesome BBS");
json_set_num(req, "calls", (long)usr.times);

string: req_body;
req_body := json_serialize(req);
json_close(req);

// req_body is now: {"name":"Kevin","board":"My Awesome BBS","calls":42}

status := http_request(
  "https://api.example.com/checkin",
  "POST",
  "Content-Type: application/json\r\n",
  req_body,
  body,
  5000
);
```

**`json_create()`** makes an empty JSON object. **`json_set_str()`** and
**`json_set_num()`** add fields. **`json_serialize()`** converts the whole
tree to a compact JSON string. Clean and type-safe — no manual string
concatenation of curly braces.

## Cursor Navigation (Advanced)

For complex or nested JSON, the path accessors might not be enough. The
cursor API lets you walk the tree:

```c
// Given: {"users": [{"name": "Alice"}, {"name": "Bob"}]}

jh := json_open(body);

json_enter(jh);           // enter root object
json_find(jh, "users");   // move to "users" key
json_enter(jh);           // enter the array

while (json_next(jh) <> JSON_END)
{
  json_enter(jh);         // enter each object
  json_find(jh, "name");
  print(json_str(jh), "\n");
  json_exit(jh);          // back to array level
}

json_exit(jh);            // back to root
json_close(jh);
```

**`json_enter()`** descends into an object or array. **`json_next()`**
advances to the next sibling (returns `JSON_END` when done).
**`json_find()`** jumps to a key within the current object.
**`json_exit()`** goes back up one level. It's like navigating a
directory tree.

For most scripts, the path accessors are all you need. The cursor is
there for when you need to iterate arrays of unknown length or walk
deeply nested structures.

## What You Learned

- **`http_request()`** — one-call HTTP/HTTPS. Include `socket.mh`.
- **`json_open()` / `json_close()`** — parse and release JSON. Include
  `json.mh`.
- **Path accessors** — `json_get_str()`, `json_get_num()`,
  `json_get_bool()`. Dot-and-bracket paths like `"obj.arr[0].field"`.
- **JSON building** — `json_create()`, `json_set_str()`,
  `json_serialize()` for constructing POST bodies.
- **Cursor navigation** — `json_enter()`, `json_next()`, `json_find()`,
  `json_exit()` for complex traversal.
- **Network error handling** — check connection, status code, parse
  result, and field existence. Four layers of defense.
- **Caching** — use file I/O from Lesson 6 to avoid hammering APIs.
- **`getch()`** — single keypress, no echo. Good for dramatic pauses.

## Next

You've learned everything: variables, I/O, decisions, loops, files, menus,
message bases, and live internet data. Every tool in the MEX toolbox.

There's only one thing left to do: build something that uses *all of it*.

Next lesson: a capstone mini-game. Everything you've learned, in one
script. The grand finale of your 4 AM sysop session.

**[Lesson 10: "Game Night" →]({% link mex-learn-mini-game.md %})**
