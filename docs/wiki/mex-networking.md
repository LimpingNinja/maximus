---
layout: default
title: "MEX Networking"
section: "mex"
description: "Outgoing TCP, HTTP/HTTPS, and JSON processing from MEX scripts"
permalink: /mex-networking/
---

MEX scripts can reach out to the internet — fetching API data, posting
webhooks, talking custom protocols over raw TCP, and parsing or building
JSON payloads. Everything runs with timeout protection so a dead server
can't hang your caller's session.

---

## Capabilities

| Feature | What it does |
|---------|-------------|
| **HTTP/HTTPS requests** | One-shot GET/POST/PUT/DELETE with automatic DNS, TLS, and redirect handling |
| **Raw TCP sockets** | Persistent outgoing connections for custom protocols, with send/recv/status/avail |
| **JSON parsing** | Parse JSON strings into navigable trees, pull values by path or cursor |
| **JSON building** | Construct JSON objects and arrays from scratch, serialize to string |

All networking is **outgoing only** — your BBS is the client. Scripts
cannot listen for incoming connections.

---

## Getting Started

Include the headers you need:

```mex
#include <socket.mh>   // http_request, sock_open, sock_send, sock_recv, etc.
#include <json.mh>     // json_open, json_get_str, json_create, etc.
```

The simplest useful script is a one-shot API call:

```mex
#include <max.mh>
#include <json.mh>
#include <socket.mh>

void main()
{
    string: response;
    int:    status;
    int:    jh;

    status := http_request("https://catfact.ninja/fact",
                           "GET", "", "", response, 5000);
    if (status <> 200)
    {
        print("Request failed.\n");
        return;
    }

    jh := json_open(response);
    if (jh < 0)
    {
        print("Bad JSON.\n");
        return;
    }

    print("|14Cat Fact:|07 " + json_get_str(jh, "fact") + "\n");
    json_close(jh);
}
```

That's DNS resolution, TLS handshake, HTTP request, JSON parsing, and
formatted display — in about 20 lines.

---

## Choosing Your Approach

| I want to... | Use |
|--------------|-----|
| Hit a web API and get a response | [HTTP/HTTPS Requests]({% link mex-http-https.md %}) |
| Talk a custom protocol or keep a connection open | [Socket Programming]({% link mex-sockets.md %}) |
| Parse a JSON string or API response | [JSON Processing]({% link mex-json.md %}) |
| Build a JSON payload for a POST body | [JSON Processing]({% link mex-json.md %}) |
| Do all of the above in one script | Combine all three — they work together naturally |

Most scripts only need `http_request` + JSON. Raw sockets are for custom
protocols (game servers, IRC, QOTD, etc.) or when you need to control the
connection lifecycle yourself.

---

## Access Control

Sysops control which scripts can make outgoing connections. Socket access
is configured in the system TOML configuration:

- **Allowed hosts/ports** — restrict which destinations scripts can reach
- **Per-script permissions** — enable or disable networking per script
- **Global kill switch** — disable all outgoing connections system-wide

If a script tries to connect and access is denied, `sock_open` and
`http_request` return `-1`. No crash — the script just gets a connection
failure.

This protects the BBS from rogue scripts phoning home or making
unauthorized requests.

---

## Limits at a Glance

| Limit | Value |
|-------|-------|
| Concurrent sockets | 8 per MEX session |
| HTTP response size | 64 KB |
| JSON handles | 16 simultaneous |
| JSON nesting depth | 16 levels |
| Timeout hard cap | 30 seconds per I/O operation |
| Redirect hops | 5 maximum |
| Protocol | TCP only (no UDP) |

---

## Sub-Pages

- **[HTTP/HTTPS Requests]({% link mex-http-https.md %})** — the one-shot
  convenience function for web API calls
- **[Socket Programming]({% link mex-sockets.md %})** — raw TCP sockets
  for custom protocols and persistent connections
- **[JSON Processing]({% link mex-json.md %})** — parsing, navigating, and
  building JSON data

---

## See Also

- [MEX Getting Started]({% link mex-getting-started.md %}) — introduction
  to MEX scripting
- [Display Codes]({% link display-codes.md %}) — color codes for formatted
  output
- [UI Primitives]({% link mex-ui-primitives.md %}) — interactive screen
  controls for displaying fetched data
