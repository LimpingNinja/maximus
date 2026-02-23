# Using Sockets in MEX Scripts

Your BBS can talk to the internet now. MEX scripts can open outgoing TCP
connections, send and receive raw data, and make full HTTP/HTTPS requests —
all with timeout control so a dead server can't hang your caller's session.

Want to fetch a weather API, ping a game server, or POST a webhook when
someone logs in? This is the guide. We'll start with the one-liner
convenience function, then dig into raw sockets for when you need full
control.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [The Easy Way: http_request](#the-easy-way-http_request)
   - [A Simple GET](#a-simple-get)
   - [HTTPS — It Just Works](#https--it-just-works)
   - [Redirects](#redirects)
   - [POST with Headers](#post-with-headers)
   - [Pairing with JSON](#pairing-with-json)
3. [Raw Sockets](#raw-sockets)
   - [Open, Send, Receive, Close](#open-send-receive-close)
   - [Checking Connection State](#checking-connection-state)
   - [Non-Blocking Reads with sock_avail](#non-blocking-reads-with-sock_avail)
   - [A Simple TCP Client](#a-simple-tcp-client)
4. [Error Handling](#error-handling)
5. [Limits and Gotchas](#limits-and-gotchas)
6. [Complete Example: Weather Report](#complete-example-weather-report)
7. [Quick Reference](#quick-reference)
8. [See Also](#see-also)

---

## Getting Started

Include the socket header at the top of your script:

```mex
#include <socket.mh>
```

That gives you every socket function and the status constants (`SOCK_CONNECTED`,
`SOCK_CLOSED`, `SOCK_ERROR`). The header lives in `scripts/include/socket.mh`
and is found automatically by the compiler.

Most scripts only need one function — `http_request`. If that covers your use
case, you can skip the raw socket section entirely.

---

## The Easy Way: http_request

If all you need is to hit a web API and get a response back, `http_request`
does everything in one call: DNS resolution, TCP connect, TLS handshake (for
HTTPS), sending the request, reading the response, and even following
redirects. You don't manage any sockets yourself.

### A Simple GET

```mex
#include <max.mh>
#include <socket.mh>

void main()
{
    string: body;
    int:    status;

    status := http_request("http://example.com/hello.txt",
                           "GET", "", "", body, 5000);

    if (status = 200)
        print("Got: " + body + "\n");
    else
        print("Request failed (status " + itostr(status) + ")\n");
}
```

That's it. One function call, one status check.

The arguments, in order:

| Argument | What it does |
|----------|-------------|
| `url` | Full URL including scheme (`http://` or `https://`) |
| `method` | HTTP method — `"GET"`, `"POST"`, `"PUT"`, `"DELETE"`, etc. |
| `headers` | Extra headers, each ending with `\r\n`, or `""` for none |
| `body` | Request body for POST/PUT, or `""` for none |
| `response` | (ref) The response body is stored here |
| `timeout_ms` | Timeout per I/O operation in milliseconds |

The function returns the HTTP status code (200, 404, 500, etc.) or `-1` if
the connection failed or timed out.

### HTTPS — It Just Works

Change `http://` to `https://` in your URL. That's the whole migration:

```mex
status := http_request("https://api.example.com/data",
                       "GET", "", "", body, 10000);
```

TLS is handled automatically under the hood via OpenSSL (`libssl`/`libcrypto`).
Your callers won't notice any difference except that the request is encrypted
in transit.

### Redirects

If the server sends a `301`, `302`, `307`, or `308` redirect, `http_request`
follows it automatically — up to 5 hops. You don't have to do anything.

This matters more than you might think. Many APIs and web services redirect
from HTTP to HTTPS, from `www.` to a bare domain, or from an old endpoint to
a new one. Your script handles all of that transparently.

If the redirect chain exceeds 5 hops, `http_request` gives up and returns
`-1`. In practice, you'll never hit this unless something is misconfigured on
the remote end.

### POST with Headers

Need to send data? Set the method to `"POST"`, add a `Content-Type` header,
and put the payload in the body argument:

```mex
string: response;
int:    status;
string: payload;

payload := "username=sysop&action=checkin";

status := http_request("https://example.com/api/checkin",
                       "POST",
                       "Content-Type: application/x-www-form-urlencoded\r\n",
                       payload,
                       response, 10000);

if (status = 200)
    print("Check-in OK!\n");
else
    print("Check-in failed: " + itostr(status) + "\n");
```

Multiple headers are separated with `\r\n`:

```mex
string: hdrs;

hdrs := "Content-Type: application/json\r\n"
      + "Authorization: Bearer abc123\r\n";

status := http_request(url, "POST", hdrs, json_body, response, 10000);
```

### Pairing with JSON

`http_request` and the JSON intrinsics are natural partners. Fetch a JSON API,
parse the response, pull out the fields you need:

```mex
#include <max.mh>
#include <json.mh>
#include <socket.mh>

void main()
{
    string: response;
    int:    status;
    int:    jh;
    string: fact;

    status := http_request("https://catfact.ninja/fact",
                           "GET", "", "", response, 5000);

    if (status <> 200)
    {
        print("Could not fetch cat fact.\n");
        return;
    }

    jh := json_open(response);
    if (jh < 0)
    {
        print("Bad JSON in response.\n");
        return;
    }

    fact := json_get_str(jh, "fact");
    print("|14Cat Fact:|07 " + fact + "\n");

    json_close(jh);
}
```

This pattern — fetch, parse, extract, display — covers the vast majority of
what you'll want to do with sockets in a BBS script. The
[Using JSON guide](mex-using-json.md) has the full rundown on the JSON side.

---

## Raw Sockets

Sometimes `http_request` isn't enough. Maybe you need to talk a custom
protocol, keep a connection open across multiple exchanges, or read data in
chunks. That's where raw sockets come in.

Raw sockets give you a handle to a TCP connection. You open it, send bytes,
receive bytes, and close it when you're done. Everything uses timeouts so your
caller's session won't freeze if the remote server goes dark.

### Open, Send, Receive, Close

The basic lifecycle:

```mex
int:    sh;
string: buf;
int:    got;

// Connect to a server (5-second timeout)
sh := sock_open("example.com", 80, 5000);
if (sh < 0)
{
    print("Connection failed.\n");
    return;
}

// Send a request
sock_send(sh, "HELLO\r\n", 0);

// Read the response (3-second timeout)
got := sock_recv(sh, buf, 1024, 3000);
if (got > 0)
    print("Server said: " + buf + "\n");

// Done
sock_close(sh);
```

`sock_open` returns a handle — a small integer from 0 to 7. You pass that
handle to every other socket function. When you're finished, `sock_close`
releases the handle for reuse.

The third argument to `sock_send` is the number of bytes to send. Pass `0` to
send the entire string (which is almost always what you want).

`sock_recv` fills the buffer by reference and returns the number of bytes
received. It returns `0` on timeout (no data arrived within the deadline) and
`-1` on error or disconnect.

### Checking Connection State

If you need to test whether a socket is still alive without reading:

```mex
int: state;

state := sock_status(sh);

if (state = SOCK_CONNECTED)
    print("Still connected.\n");
else if (state = SOCK_CLOSED)
    print("Connection closed.\n");
else
    print("Socket error.\n");
```

The three constants from `socket.mh`:

| Constant | Value | Meaning |
|----------|-------|---------|
| `SOCK_CONNECTED` | 1 | Connection is alive |
| `SOCK_CLOSED` | 0 | Socket is closed or was never opened |
| `SOCK_ERROR` | -1 | Connection error (reset, broken pipe, etc.) |

### Non-Blocking Reads with sock_avail

Want to check if data is waiting before you commit to a `sock_recv` call?
`sock_avail` tells you how many bytes are in the receive buffer without
blocking:

```mex
int: pending;

pending := sock_avail(sh);
if (pending > 0)
{
    got := sock_recv(sh, buf, pending, 1000);
    // process buf...
}
else
{
    print("Nothing yet — doing other work.\n");
}
```

This is useful when you want to interleave socket I/O with user interaction —
poll for data, handle a keystroke, poll again.

### A Simple TCP Client

Here's a complete example that connects to a "quote of the day" service
(RFC 865) — a server that sends one line of text and disconnects:

```mex
#include <max.mh>
#include <socket.mh>

void main()
{
    int:    sh;
    string: quote;
    int:    got;

    print("|14Connecting to Quote of the Day...|07\n");

    sh := sock_open("djxmmx.net", 17, 5000);
    if (sh < 0)
    {
        print("|12Connection failed.|07\n");
        return;
    }

    // QOTD servers send their quote immediately — just read
    got := sock_recv(sh, quote, 1024, 5000);
    sock_close(sh);

    if (got > 0)
        print("|15" + quote + "|07\n");
    else
        print("|12No response from server.|07\n");
}
```

No send step needed — QOTD servers speak first. This shows the minimal raw
socket pattern: open, recv, close.

---

## Error Handling

Socket operations fail gracefully. No crashes, no script aborts — you just
get predictable return values.

**Connection failures:** `sock_open` returns `-1` if DNS resolution fails, the
host is unreachable, or the connection times out. Always check before using the
handle.

```mex
sh := sock_open("bad.example.com", 80, 3000);
if (sh < 0)
{
    print("Can't connect.\n");
    return;
}
```

**Send failures:** `sock_send` returns `-1` if the connection dropped. The
socket status flips to `SOCK_ERROR`.

**Receive failures:** `sock_recv` returns `0` on timeout (nothing arrived) and
`-1` on error/disconnect. Both are safe — your buffer is unchanged on failure.

**HTTP errors:** `http_request` returns `-1` for connection/timeout failures
and the HTTP status code for server-side errors (404, 500, etc.). Any non-200
code means something went wrong on the remote end, not in your script.

**Leaked sockets:** If your script exits without calling `sock_close`, the
runtime cleans up automatically. Don't rely on this — close your sockets
explicitly — but it won't leak file descriptors if you forget.

---

## Limits and Gotchas

A few things to keep in mind:

- **8 sockets max.** You can have up to 8 concurrent socket connections per
  MEX session. That's plenty — most scripts use one or two. If you exhaust the
  pool, `sock_open` returns `-1`.

- **Outgoing only.** These are client sockets. You can connect to remote
  servers, but you cannot listen for incoming connections. The BBS is the
  client, not the server.

- **Timeouts are per-operation.** The `timeout_ms` on `sock_recv` and
  `http_request` applies to each individual I/O operation, not to the overall
  request. A slow server that trickles data will keep the connection alive as
  long as each chunk arrives within the timeout.

- **Timeout limits.** If you pass `0` for timeout, the runtime uses a 5-second
  default. The hard cap is 30 seconds — anything larger gets clamped down. This
  protects your callers from scripts that accidentally hang.

- **HTTP response size.** `http_request` caps the response body at 64 KB. If
  the server sends more, it gets truncated. For typical API payloads — JSON,
  XML, plain text — this is generous. For bulk downloads, you'd need raw
  sockets and chunked reads.

- **String length.** MEX strings have a practical limit. Very large HTTP
  responses may bump against string heap limits. If you get "Out of string
  space" errors, consider requesting smaller payloads (API query parameters,
  pagination, etc.).

- **No UDP.** Only TCP connections are supported. This covers HTTP, HTTPS, and
  the vast majority of API protocols.

- **No certificate verification.** HTTPS connections do not verify the server's
  TLS certificate. This is fine for a BBS making outgoing API calls — you're
  not running a bank. The traffic is still encrypted in transit.

- **Variables go at the top.** MEX requires all variable declarations at the
  beginning of each function. You can't declare a variable after a statement.
  Plan your variables up front.

---

## Complete Example: Weather Report

This is the full `socktest.mex` that ships with Maximus. It prompts for a
city, fetches weather data from wttr.in over HTTPS, parses the JSON response,
and displays formatted conditions. It shows how sockets, JSON, display codes,
and user input all work together in a real script.

```mex
#include <max.mh>
#include <json.mh>
#include <socket.mh>

// Replace spaces with '+' for URL-safe city names.
string url_safe_city(string: s)
{
    string: result;
    string: tok;
    int:    pos;
    int:    first;

    result := "";
    pos    := 0;
    first  := 1;

    tok := strtok(s, " ", pos);
    while (strlen(tok) > 0)
    {
        if (first = 0)
            result := result + "+";
        result := result + tok;
        first  := 0;
        tok    := strtok(s, " ", pos);
    }

    return result;
}

// Prompt for an optional city override.
string get_city()
{
    string: city;
    string: entered;
    int:    rc;

    city := usr.city;

    if (strlen(city) > 0)
        print("|14Your profile city: |15" + city + "\n");
    else
        print("|14No city in your profile.|07\n");

    if (strlen(city) > 0)
        print("|07Press |15ENTER|07 to use " + city
              + ", or type a city name:\n");
    else
        print("|07Press |15ENTER|07 to use Vancouver"
              + ", or type a city name:\n");

    entered := "";
    rc := input_str(entered, INPUT_WORD, 0, 40, "|11City: |07");

    if (strlen(entered) > 0)
        return entered;
    if (strlen(city) > 0)
        return city;
    return "Vancouver";
}

void sep()
{
    print("|08" + strpad("", 50, '-') + "|07\n");
}

void main()
{
    string: city;
    string: safe_city;
    string: url;
    string: response;
    int:    status;
    int:    jh;

    string: area_name;
    string: country;
    string: temp_c;
    string: temp_f;
    string: feels_c;
    string: feels_f;
    string: weather_desc;
    string: humidity;
    string: wind_kmph;
    string: wind_dir;
    string: max_c;
    string: max_f;

    print(AVATAR_CLS);
    print("\n|15|17  Weather Report  |07|16\n\n");

    city := get_city();

    print("\n|11Fetching weather for |15" + city + "|11...\n\n");

    safe_city := url_safe_city(city);
    url := "https://wttr.in/" + safe_city + "?format=j1";

    // One call does DNS, TCP, TLS, send, receive, and redirect following
    status := http_request(url, "GET", "", "", response, 10000);

    if (status <> 200)
    {
        print("|12Error: |07Could not get weather data");
        if (status > 0)
            print(" (HTTP " + itostr(status) + ")");
        else
            print(" (connection failed)");
        print("\n\n");
        return;
    }

    // Parse the JSON response
    jh := json_open(response);
    if (jh = -1)
    {
        print("|12Error: |07Failed to parse weather JSON\n\n");
        return;
    }

    // Pull fields with path accessors — no cursor needed
    area_name    := json_get_str(jh, "nearest_area[0].areaName[0].value");
    country      := json_get_str(jh, "nearest_area[0].country[0].value");
    temp_c       := json_get_str(jh, "current_condition[0].temp_C");
    temp_f       := json_get_str(jh, "current_condition[0].temp_F");
    feels_c      := json_get_str(jh, "current_condition[0].FeelsLikeC");
    feels_f      := json_get_str(jh, "current_condition[0].FeelsLikeF");
    weather_desc := json_get_str(jh,
                        "current_condition[0].weatherDesc[0].value");
    humidity     := json_get_str(jh, "current_condition[0].humidity");
    wind_kmph    := json_get_str(jh, "current_condition[0].windspeedKmph");
    wind_dir     := json_get_str(jh, "current_condition[0].winddir16Point");
    max_c        := json_get_str(jh, "weather[0].maxtempC");
    max_f        := json_get_str(jh, "weather[0].maxtempF");

    json_close(jh);

    // Display the results
    sep();
    print("|14  Location:      |15" + area_name + "|07, " + country + "\n");
    print("|14  Conditions:    |15" + weather_desc + "\n");
    sep();
    print("\n");
    print("|14  Temperature:   |15" + temp_c + " C  |08/  |15"
          + temp_f + " F\n");
    print("|14  Feels Like:    |15" + feels_c + " C  |08/  |15"
          + feels_f + " F\n");
    print("|14  Today's High:  |15" + max_c + " C  |08/  |15"
          + max_f + " F\n");
    print("\n");
    print("|14  Humidity:      |15" + humidity + "%%\n");
    print("|14  Wind:          |15" + wind_kmph + " km/h "
          + wind_dir + "\n");
    sep();
    print("\n|07");
}
```

Run it from the MEX Scripts menu (option **F**) or compile it yourself:

```bash
./scripts/compile-mex.sh socktest --deploy
```

---

## Quick Reference

### http_request

| Function | Returns | Purpose |
|----------|---------|---------|
| `http_request(url, method, headers, body, ref response, timeout_ms)` | HTTP status or -1 | One-shot HTTP/HTTPS request with redirect following |

### Raw Socket Functions

| Function | Returns | Purpose |
|----------|---------|---------|
| `sock_open(host, port, timeout_ms)` | handle (0–7) or -1 | Open a TCP connection |
| `sock_close(sh)` | 0 or -1 | Close a socket handle |
| `sock_send(sh, data, len)` | bytes sent or -1 | Send raw bytes (len=0 sends entire string) |
| `sock_recv(sh, ref buf, max_len, timeout_ms)` | bytes received, 0=timeout, -1=error | Receive with timeout |
| `sock_status(sh)` | `SOCK_CONNECTED`, `SOCK_CLOSED`, or `SOCK_ERROR` | Check connection state |
| `sock_avail(sh)` | byte count | Bytes available to read without blocking |

### Constants (from socket.mh)

| Constant | Value | Meaning |
|----------|-------|---------|
| `SOCK_CONNECTED` | 1 | Connection is alive |
| `SOCK_CLOSED` | 0 | Closed or unused |
| `SOCK_ERROR` | -1 | Connection error |
| `SOCK_FLAG_NONE` | 0 | Reserved for future flags |

---

## See Also

- [Using JSON in MEX Scripts](mex-using-json.md) — the JSON intrinsics that
  pair naturally with HTTP responses
- [Using Display Codes](using-display-codes.md) — color codes for making your
  script output look sharp
- [MEX Socket & JSON Enhancement Design](code-docs/mex-enhancements-socket-json.md) —
  the full design document with C-side implementation details
