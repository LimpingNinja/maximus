---
layout: default
title: "Core Settings"
section: "configuration"
description: "maximus.toml — system identity, paths, logging, and runtime toggles"
---

This page covers `config/maximus.toml` — the central configuration file for
your Maximus installation. It defines who your BBS is (system name, sysop),
where everything lives on disk (paths), how logging works, and a handful of
runtime toggles. Most sysops set these once during installation and rarely
touch them again.

This page also covers four smaller config files that live under
`config/general/`:

- [Offline Reader](#reader) — `reader.toml` (QWK packet settings)
- [External Protocols](#protocol) — `protocol.toml` (protocol definitions)
- [Display & Lightbar](#display) — `display.toml` (lightbar area display)
- [MEX Runtime](#mex) — `mex.toml` (socket and HTTP limits for scripts)

---

## maximus.toml — System & Global

**File:** `config/maximus.toml`

Every path in this file (except `sys_path` itself) is resolved relative to
`sys_path`. This means you can relocate your entire installation by changing
one value — everything else follows automatically.

### Quick Reference

#### Identity

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `system_name` | string | `"MaximusNG BBS"` | Your BBS name — used in origin lines, banners, and greeting screens |
| `sysop` | string | `"Sysop"` | Sysop display name (does not grant privileges — those come from access levels) |

#### Paths

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `sys_path` | string | *(install dir)* | Absolute path to your Maximus root — the one absolute path everything else is relative to |
| `config_path` | string | `"config"` | TOML configuration directory |
| `display_path` | string | `"display/screens"` | Display and screen files |
| `rip_path` | string | `"display/rip"` | RIP graphics icon and scene files |
| `lang_path` | string | `"config/lang"` | Language TOML files |
| `data_path` | string | `"data"` | Persistent data root (user DB, message bases, file listings) |
| `mex_path` | string | *(not set)* | MEX script directory (compiled `.vm` files) |
| `temp_path` | string | `"run/tmp"` | Scratch space for uploads and temporary operations |
| `stage_path` | string | `"run/stage"` | Staging area for slow-media file transfers |
| `run_path` | string | *(not set)* | Ephemeral runtime state root |
| `node_path` | string | `"run/node"` | Per-node runtime state (lock files, IPC sockets) |
| `doors_path` | string | *(not set)* | External door program directory |
| `net_info_path` | string | `"data/nodelist"` | Nodelist and net-info files |
| `outbound_path` | string | `"data/mail/outbound"` | Outbound mail bundles |
| `inbound_path` | string | `"data/mail/inbound"` | Inbound mail bundles |

#### Data Files

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `file_password` | string | `"data/users/user"` | User database location (root name — extensions added automatically) |
| `file_callers` | string | `"data/users/callers"` | Callers log location (root name) |
| `file_access` | string | `"config/security/access"` | Access level database (root name) |
| `message_data` | string | `"data/msgbase/marea"` | Message area data root |
| `file_data` | string | `"data/msgbase/farea"` | File area data root |

#### Logging

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `log_file` | string | `"log/max.log"` | Primary system log file (leave empty to disable logging) |
| `log_mode` | string | `"Trace"` | Log verbosity: `Terse`, `Verbose`, or `Trace` |

#### Multi-Node & IPC

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `task_num` | int | `1` | Node/task number — each node needs a unique value |
| `mcp_sessions` | int | `16` | Maximum concurrent nodes/sessions |
| `snoop` | bool | `true` | Mirror the caller's screen on the local console |
| `msg_reader_menu` | string | `"MSGREAD"` | Menu name used for the message reader |

#### Security

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `no_password_encryption` | bool | `false` | Disable password hashing — **not recommended** |

#### Legacy & Compatibility

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `config_version` | int | `2` | Schema version for tooling — don't change by hand |
| `video` | string | `"IBM"` | Console video mode (legacy DOS; ignored on UNIX) |
| `has_snow` | bool | `false` | CGA snow avoidance (legacy; always `false`) |
| `multitasker` | string | `"UNIX"` | Multitasking environment identifier |
| `status_line` | bool | `false` | Show a sysop status bar on the local console |
| `local_input_timeout` | bool | `false` | Auto-logoff local sessions after 5 min idle |
| `dos_close` | bool | `true` | Close internal files when launching external programs |
| `no_share` | bool | `false` | Disable file-sharing support (legacy DOS) |
| `reboot` | bool | `false` | Reboot on carrier drop in external programs (legacy DOS) |
| `swap` | bool | `false` | Swap out of memory for external programs (legacy DOS) |

### Paths — How They Work

All paths except `sys_path` are relative to `sys_path`. If your `sys_path` is
`/opt/maximus` and your `data_path` is `"data"`, the actual data directory is
`/opt/maximus/data`.

A few paths deserve extra context:

- **`data_path`** — the root for persistent mutable data: user databases,
  message bases, file listings, and nodelist files. This is the directory you
  want in your backup scripts.
- **`run_path`** and **`node_path`** — ephemeral runtime state. Per-node lock
  files, IPC sockets, and temporary session data live here. These are
  recreated each time a node starts — no need to back them up.
- **`stage_path`** — used when file areas are marked `Staged` or `CD`. Files
  from slow media are copied here before transfer. Most modern setups won't
  use this.
- **`mex_path`** — where compiled MEX scripts (`.vm` files) live. Only
  present in configs exported from CTL; set it if you store scripts outside
  the default locations.
- **`doors_path`** — home directory for external door programs. Only relevant
  if you run doors.

> **Renamed keys:** In older documentation you may see `misc_path` and
> `ipc_path`. These have been renamed to `display_path` and `node_path`
> respectively.

### Logging

`log_file` sets the path for the main system log. Leave it empty to disable
logging entirely (not recommended). `log_mode` controls how much detail goes
into the log:

| Mode | What Gets Logged |
|------|-----------------|
| `Terse` | Critical events only — logins, logoffs, errors |
| `Verbose` | Terse plus area changes, file transfers, menu navigation |
| `Trace` | Everything — great for debugging, but produces large log files |

### Multi-Node & IPC

- **`task_num`** — the node number for this instance. In a multi-node setup,
  each node needs a unique task number. MaxTel assigns these automatically
  when spawning nodes; if you run nodes manually, set this per-instance.
- **`mcp_sessions`** — the maximum number of simultaneous sessions. Match
  this to your MaxTel `-n` flag or your expected peak concurrency.
- **`snoop`** — mirrors the caller's screen on the local console. Useful for
  monitoring live sessions. Disable it for headless or daemon operation.
- **`msg_reader_menu`** — which menu file is loaded when a caller enters the
  message reader. The default `"MSGREAD"` corresponds to
  `config/menus/msgread.toml`.

### Security

`no_password_encryption` defaults to `false`, which means passwords are hashed
before storage. Only set this to `true` if you have external tooling that
requires plaintext passwords — and even then, consider fixing the tooling
instead.

### Legacy Compatibility

Several keys exist for compatibility with DOS/OS2 behavior. On modern UNIX
systems, you can safely leave these at their defaults:

- **`video`**, **`has_snow`**, **`multitasker`** — console/platform detection
  from the DOS era. Set `multitasker` to `"UNIX"` and ignore the rest.
- **`swap`**, **`reboot`** — DOS-era memory management and watchdog behavior.
  Both should be `false`.
- **`no_share`** — disabled DOS SHARE.EXE file locking. Leave `false`.
- **`dos_close`** — closes internal files before launching external programs.
  Leave `true` to avoid file handle conflicts with doors.
- **`status_line`** — shows a sysop info bar at the bottom of the local
  console. Only worked with `Video IBM` on DOS; limited utility on modern
  terminals.
- **`local_input_timeout`** — auto-logoff for local console sessions after
  five minutes of inactivity. Remote callers always have timeouts; this
  extends the same behavior to local sessions.

---

## reader.toml — Offline Reader {#reader}

**File:** `config/general/reader.toml`

If your BBS supports QWK offline mail packets, these settings control the
packing behavior — packet size limits, the working directory, and the archiver
used for compression. Most sysops running a modern telnet-only board won't
need to touch this file.

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `max_pack` | int | `1200` | Maximum messages per QWK download packet |
| `archivers_ctl` | string | `"config/legacy/compress.cfg"` | Path to archiver definitions |
| `packet_name` | string | `"TVK"` | Three-letter QWK packet identifier |
| `work_directory` | string | `"data/olr"` | Working directory for packet assembly |
| `phone` | string | `"(123) 456-7890"` | BBS phone number included in QWK control files |

---

## protocol.toml — External Protocols {#protocol}

**File:** `config/general/protocol.toml`

This file tells Maximus where to find external file transfer protocol
definitions (Zmodem, etc.). The actual protocol entries are defined in the
legacy `.ctl` or `.max` files referenced here. You can also configure protocols
through MaxCFG's [Setup & Tools]({{ site.baseurl }}{% link maxcfg-setup-tools.md %}) screen.

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `protoexit` | int | `9` | Errorlevel returned after protocol execution |
| `protocol_max_path` | string | `"config/legacy/protocol.max"` | Path to `protocol.max` definitions |
| `protocol_max_exists` | bool | `false` | Whether `protocol.max` is present |
| `protocol_ctl_path` | string | `"config/legacy/protocol.ctl"` | Path to `protocol.ctl` definitions |
| `protocol_ctl_exists` | bool | `true` | Whether `protocol.ctl` is present |

---

## display.toml — Display & Lightbar {#display}

**File:** `config/general/display.toml`

This file controls lightbar display behavior for file area lists, message area
lists, and the message reader — things like highlight style, selected-row
colors, list boundaries, and header/footer anchoring.

For a full walkthrough of what each setting does and how to set up lightbar
navigation, see
[Lightbar Customization]({{ site.baseurl }}{% link theming-lightbar.md %}).

### General

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `general.lightbar_prompts` | bool | `false` | Use lightbar-style prompts system-wide |

### Per-Section Settings

Each section (`[file_areas]`, `[msg_areas]`, `[msg_reader]`) supports the
same set of keys:

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `lightbar_area` | bool | `true` | Enable lightbar navigation for this section |
| `reduce_area` | int | `5` | Rows reserved for header/footer/status lines |
| `lightbar_what` | string | `"row"` | Highlight style: `row`, `full`, or `name`/`number` |
| `lightbar_fore` | string | `""` | Selected-row foreground color override |
| `lightbar_back` | string | `""` | Selected-row background color override |
| `custom_screen` | string | `""` | Optional display file shown before the list |

Optional boundary and anchor keys (commented out by default):

| Key | Type | Description |
|-----|------|-------------|
| `top_boundary` | `[row, col]` | Upper-left corner of the list area (1-based) |
| `bottom_boundary` | `[row, col]` | Lower-right corner of the list area |
| `header_location` | `[row, col]` | Fixed position for the header |
| `footer_location` | `[row, col]` | Fixed position for the footer |

---

## mex.toml — MEX Runtime {#mex}

**File:** `config/general/mex.toml`

This file controls the runtime behavior of MEX script socket and HTTP
intrinsics — connection limits, timeouts, and access control. If your BBS runs
MEX scripts that make network calls (weather lookups, API integrations,
inter-BBS messaging), these settings let you tune performance and security.

For full details on MEX networking, see
[MEX Networking]({{ site.baseurl }}{% link mex-networking.md %}). For socket programming
specifics, see [Socket Programming]({{ site.baseurl }}{% link mex-sockets.md %}).

### `[sockets]` Section

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `enabled` | bool | `true` | Master switch — `false` disables all outgoing socket/HTTP calls |
| `max_connections` | int | `8` | Maximum concurrent socket connections per MEX session |
| `connect_timeout_ms` | int | `500` | Default connect timeout (ms) when the script passes 0 |
| `tls_handshake_timeout_ms` | int | `500` | Hard cap on TLS handshake duration (ms) |
| `max_recv_size` | int | `131072` | Maximum bytes per `sock_recv` call (128 KB) |

### Access Control Rules

Access rules are defined as `[[sockets.rules]]` entries, evaluated
top-to-bottom (first match wins). Each rule has:

| Key | Type | Description |
|-----|------|-------------|
| `action` | string | `"allow"` or `"deny"` |
| `host` | string | Host pattern (supports `*` wildcards) |
| `ports` | array | *(optional)* Port whitelist for allow rules |

If no rules are defined, all outgoing connections are allowed.

---

## See Also

- [Session & Login]({{ site.baseurl }}{% link config-session-login.md %}) — user experience and
  policy settings (`session.toml`)
- [Equipment & Comm]({{ site.baseurl }}{% link config-equipment-comm.md %}) — modem and
  connection settings (`equipment.toml`)
- [Directory Structure]({{ site.baseurl }}{% link directory-structure.md %}) — full runtime
  layout reference
- [Configuration Overview]({{ site.baseurl }}{% link configuration.md %}) — top-level config
  guide
