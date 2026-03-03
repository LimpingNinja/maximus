---
layout: default
title: "HTTP/HTTPS Requests"
section: "mex"
description: "One-shot HTTP and HTTPS requests from MEX scripts"
permalink: /mex-http-https/
---

Most scripts that need to talk to the internet just want to hit a URL and
get a response back. The `http_request` intrinsic handles everything in one
call — DNS, TCP connect, TLS handshake (for HTTPS), sending the request,
reading the response, and following redirects. You don't touch sockets
at all.

If you need persistent connections, custom protocols, or chunked streaming,
see [Socket Programming]({% link mex-sockets.md %}). For parsing the JSON
that most APIs return, see [JSON Processing]({% link mex-json.md %}).

```mex
#include <socket.mh>
```

---

## http_request

```mex
int http_request(string: url, string: method, string: headers,
                 string: body, ref string: response, int: timeout_ms);
```

| Argument | What it does |
|----------|-------------|
| `url` | Full URL including scheme — `http://` or `https://` |
| `method` | HTTP method: `"GET"`, `"POST"`, `"PUT"`, `"DELETE"`, etc. |
| `headers` | Extra headers, each ending with `\r\n`, or `""` for none |
| `body` | Request body for POST/PUT, or `""` for none |
| `response` | **(ref)** The response body is stored here |
| `timeout_ms` | Timeout per I/O operation in milliseconds |

**Returns** the HTTP status code (`200`, `404`, `500`, etc.) or `-1` if the
connection failed, timed out, or the redirect chain was exhausted.

---

## GET Requests

The simplest case — fetch a URL and check the status:

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

No socket handles, no cleanup — it's a single function call.

---

## HTTPS — It Just Works

Change `http://` to `https://` in the URL. That's the whole migration:

```mex
status := http_request("https://api.example.com/data",
                       "GET", "", "", body, 10000);
```

TLS is handled automatically under the hood via **mbedTLS**. The handshake,
cipher negotiation, and encrypted transport are all invisible to your
script.

**Certificate verification** is not enforced. This is fine for a BBS making
outgoing API calls — you're not running a bank. The traffic is still
encrypted in transit; the server's identity just isn't validated against a
CA bundle. This keeps the runtime simple and avoids certificate management
headaches on embedded-style systems.

---

## POST Requests

Set the method to `"POST"`, add a `Content-Type` header, and pass the
payload:

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

### JSON POST

For APIs that expect JSON, set the content type and build your payload
with the [JSON intrinsics]({% link mex-json.md %}):

```mex
#include <max.mh>
#include <json.mh>
#include <socket.mh>

void main()
{
    int:    jh;
    string: payload;
    string: response;
    int:    status;

    // Build the request body
    jh := json_create();
    json_set_str(jh, "username", "sysop");
    json_set_str(jh, "event", "login");
    json_set_num(jh, "node", 1);
    payload := json_serialize(jh);
    json_close(jh);

    // Send it
    status := http_request("https://hooks.example.com/bbs",
                           "POST",
                           "Content-Type: application/json\r\n",
                           payload,
                           response, 10000);

    if (status = 200)
        print("|10Webhook sent.|07\n");
    else
        print("|12Webhook failed.|07\n");
}
```

---

## Custom Headers

Pass extra headers as a single string. Each header must end with `\r\n`:

```mex
string: hdrs;

hdrs := "Authorization: Bearer abc123\r\n"
      + "Accept: application/json\r\n"
      + "X-BBS-Node: 1\r\n";

status := http_request(url, "GET", hdrs, "", response, 10000);
```

The runtime adds `Host`, `Content-Length`, and `Connection: close`
automatically. You only need to add headers that aren't part of the
standard boilerplate.

---

## Automatic Redirect Following

If the server responds with a `301`, `302`, `307`, or `308` redirect,
`http_request` follows it automatically — up to **5 hops**. You don't have
to do anything.

This matters more than you'd think. Many APIs and web services redirect:

- **HTTP → HTTPS** (e.g., `http://api.example.com` → `https://...`)
- **www → bare domain** (or vice versa)
- **Old endpoint → new endpoint** (API versioning)

Your script handles all of that transparently. If the redirect chain
exceeds 5 hops, `http_request` gives up and returns `-1`. In practice,
you'll never hit this unless something is misconfigured on the remote end.

**Method preservation:** `307` and `308` redirects preserve the original
method (POST stays POST). `301` and `302` redirects switch to GET, per
the HTTP spec.

---

## Parsing JSON Responses

The most common pattern is fetch + parse. `http_request` and the JSON
intrinsics are natural partners:

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

This pattern — **fetch, parse, extract, display** — covers the vast
majority of what you'll want to do with HTTP in a BBS script.

---

## Timeouts

The `timeout_ms` argument applies to **each individual I/O operation**, not
to the overall request. A slow server that trickles data keeps the
connection alive as long as each chunk arrives within the deadline.

| Value | Behavior |
|-------|----------|
| `0` | Uses the 5-second default |
| `1`–`30000` | Used as-is |
| `> 30000` | Clamped to 30 seconds |

The hard cap protects your callers from scripts that accidentally hang on
an unresponsive server. For most API calls, 5000–10000 ms is plenty.

---

## Error Handling

`http_request` fails gracefully — no crashes, no script aborts.

| Return value | Meaning |
|-------------|---------|
| `200`–`299` | Success |
| `3xx` | Redirect was followed (you see the final status, not the redirect) |
| `4xx` | Client error (bad URL, auth failure, not found, etc.) |
| `5xx` | Server error |
| `-1` | Connection failed, DNS error, timeout, or redirect chain exhausted |

Check both the status code and the response body:

```mex
status := http_request(url, "GET", "", "", response, 5000);

if (status = -1)
    print("Connection failed — check the URL or try later.\n");
else if (status = 200)
    print("Success: " + response + "\n");
else
    print("Server returned HTTP " + itostr(status) + "\n");
```

---

## Limits

A few things to keep in mind:

- **Response size cap: 64 KB.** If the server sends more, the response is
  truncated. For typical API payloads — JSON, XML, plain text — this is
  generous. For bulk downloads, you'd need raw sockets and chunked reads.

- **String length.** MEX strings have a practical limit (~256 bytes for
  variables, larger for the heap). Very large HTTP responses may bump
  against string heap limits. If you get "Out of string space" errors,
  request smaller payloads (pagination, query filters, etc.).

- **Outgoing only.** `http_request` makes outgoing calls. Your BBS cannot
  serve HTTP — it's a client, not a server.

- **No streaming.** The entire response is buffered before returning. You
  can't read headers separately or process chunks as they arrive. For
  that, use [raw sockets]({% link mex-sockets.md %}).

- **Sysop access control.** Socket access (including `http_request`) can
  be restricted via TOML configuration. The sysop controls which scripts
  are allowed to make outgoing connections and to which hosts/ports.

---

## Quick Reference

| Function | Returns | Purpose |
|----------|---------|---------|
| `http_request(url, method, headers, body, ref response, timeout_ms)` | HTTP status or `-1` | One-shot HTTP/HTTPS request with automatic redirect following |

### Supported Methods

`GET`, `POST`, `PUT`, `DELETE`, `PATCH`, `HEAD`, `OPTIONS`

### Automatic Behaviors

- **DNS resolution** — hostname in URL is resolved automatically
- **TLS handshake** — `https://` URLs use mbedTLS transparently
- **Redirect following** — up to 5 hops for 301/302/307/308
- **Content-Length** — computed and sent automatically for POST/PUT bodies
- **Connection: close** — added automatically (no keep-alive)

---

## See Also

- [Socket Programming]({% link mex-sockets.md %}) — raw TCP sockets for
  custom protocols and persistent connections
- [JSON Processing]({% link mex-json.md %}) — parsing and building JSON
  for API payloads
- [Display Codes]({% link display-codes.md %}) — color codes for making
  your script output look sharp
- [MEX Getting Started]({% link mex-getting-started.md %}) — introduction
  to MEX scripting
