---
layout: default
title: "Network Configuration"
section: "administration"
description: "matrix.toml, message area types, nodelists, echotoss, and the Maximus side of FTN networking"
---

This page covers the **Maximus side** of FidoNet-Technology Networking —
the settings that tell your BBS how to interact with the FTN world.
Squish handles the actual mail processing (see
[Squish Tosser]({% link fidonet-squish.md %})), but Maximus needs to
know about your network identity, how to signal outbound mail, where
nodelists live, and what privileges callers need for network-aware
message features.

If you haven't set up FTN yet, start with the
[Joining a Network]({% link fidonet-joining-network.md %}) tutorial —
it walks through all of this in context. This page is the reference for
when you need to tune things later.

---

## On This Page

- [matrix.toml — Overview](#matrix-overview)
- [Network Identity & Paths](#identity)
- [Echotoss — The Maximus-to-Squish Bridge](#echotoss)
- [Nodelist Settings](#nodelist)
- [Privilege Levels for Network Features](#privileges)
- [Message Edit Attributes](#message-edit)
- [Exit Codes (Legacy Integration)](#exit-codes)
- [Message Area Types](#area-types)
- [FTN Paths in maximus.toml](#maximus-toml)

---

## matrix.toml — Overview {#matrix-overview}

**File:** `config/matrix.toml`

This is Maximus's FTN configuration file. It controls how the BBS
interacts with network mail — what callers are allowed to do, how
outbound echomail is signaled to Squish, where nodelists are found,
and what message attributes are available during message editing.

Here's the full default file with annotations:

```toml
# --- Privilege levels for network features ---
ctla_priv = 65535          # Priv to see ^A kludge lines (hidden control info)
seenby_priv = 100          # Priv to see SEEN-BY / PATH lines
private_priv = 100         # Priv to send private netmail
fromfile_priv = 100        # Priv to use file attaches
unlisted_priv = 100        # Priv to send netmail to unlisted nodes
unlisted_cost = 50         # Credit cost for netmail to unlisted nodes

# --- Echotoss & logging ---
log_echomail = true        # Log echomail activity
echotoss_name = "log/echotoss.log"  # Where Maximus writes the echotoss log

# --- Exit codes ---
after_edit_exit = 11       # Errorlevel after netmail is entered
after_echomail_exit = 12   # Errorlevel after echomail is entered
after_local_exit = 0       # Errorlevel after local message is entered

# --- Nodelist ---
nodelist_version = "7"     # Nodelist format version
fidouser = "data/nodelist/fidouser.lst"  # FidoUser name lookup file

# --- Message edit attribute controls ---
[message_edit]

[message_edit.ask]         # Minimum priv to be ASKED about these attributes
private = 65535
crash = 100
fileattach = 100
killsent = 65535
hold = 100
filerequest = 65535
updaterequest = 65535
localattach = 30

[message_edit.assume]      # Minimum priv where attribute is AUTO-SET
private = 65535
crash = 65535
fileattach = 65535
killsent = 65535
hold = 65535
filerequest = 65535
updaterequest = 65535
```

The following sections explain each group in detail.

---

## Network Identity & Paths {#identity}

Your actual FTN addresses (`Address` lines) are configured in
`squish.cfg`, not in `matrix.toml`. Maximus doesn't need to know your
node number directly — it gets that information from the message bases
and Squish configuration.

However, several paths in `maximus.toml` are FTN-relevant:

```toml
# In maximus.toml
net_info_path = "data/nodelist"
outbound_path = "data/mail/outbound"
inbound_path  = "data/mail/inbound"
```

These must match the corresponding paths in your `squish.cfg` and
`binkd.cfg`. See [FTN Paths in maximus.toml](#maximus-toml) below.

---

## Echotoss — The Maximus-to-Squish Bridge {#echotoss}

```toml
echotoss_name = "log/echotoss.log"
log_echomail = true
```

The **echotoss log** is the key integration point between Maximus and
Squish. Here's how it works:

1. A caller posts a message in an EchoMail area.
2. Maximus writes the area's tag (e.g., `FSX_GEN`) to the echotoss log
   file.
3. When you run `squish out`, Squish reads this file to know which
   areas have new outbound messages to scan.
4. After scanning, Squish deletes the echotoss log (or truncates it).

Without this file, Squish wouldn't know which areas have new mail to
export — it would have to scan every area every time, which is slow.

The path is relative to `sys_path`. Make sure this matches the echotoss
path Squish expects. In `squish.cfg`, Squish reads the echotoss log
from the path specified by the `-f` command-line flag or its own
internal default.

**`log_echomail`** controls whether Maximus logs echomail activity to
the main system log. This is useful for debugging ("did the caller
actually post?") but can be turned off on busy systems to reduce log
noise.

---

## Nodelist Settings {#nodelist}

```toml
nodelist_version = "7"
fidouser = "data/nodelist/fidouser.lst"
```

### Nodelist version

The `nodelist_version` setting tells Maximus which version of the
nodelist format to expect. For modern FTN use, `"7"` is the standard
St. Louis format used by FidoNet, fsxNet, and most other networks.

### FidoUser list

The `fidouser` file is a compiled name-to-address lookup table. When a
caller composes netmail, Maximus can look up a sysop's name and
auto-fill the destination address. This file is generated by external
tools that compile nodelist data into a fast-lookup format.

For fsxNet, you typically won't need this — callers address netmail by
node number rather than name lookup. For FidoNet with its large
nodelist, it's more useful.

### Nodelist files

Place your network's nodelist files in `data/nodelist/` (or wherever
`net_info_path` points). For fsxNet, the current nodelist ships in the
weekly infopack as `FSXNET.xxx` (where `xxx` is the day number).

BinkD also uses nodelists for address lookups when connecting, so this
directory is typically shared between Maximus and BinkD.

---

## Privilege Levels for Network Features {#privileges}

```toml
ctla_priv = 65535
seenby_priv = 100
private_priv = 100
fromfile_priv = 100
unlisted_priv = 100
unlisted_cost = 50
```

These settings control which callers can access network-specific message
features. Each value is a Maximus privilege level — callers must have at
least that privilege to use the feature.

| Setting | What It Controls | Default | Recommendation |
|---------|-----------------|---------|----------------|
| `ctla_priv` | See `^A` kludge lines (hidden FTN control info like MSGID, REPLY, PID) | 65535 (Sysop) | Keep high — kludges are ugly and confuse normal callers |
| `seenby_priv` | See SEEN-BY and PATH lines in echomail | 100 | Moderate — some callers are curious, most don't care |
| `private_priv` | Send private netmail | 100 | Moderate — depends on your trust level |
| `fromfile_priv` | Use file attaches in netmail | 100 | Moderate to high — file attaches consume bandwidth |
| `unlisted_priv` | Send netmail to nodes not in your nodelist | 100 | Moderate — useful for inter-network mail |
| `unlisted_cost` | Credit cost for sending to unlisted nodes | 50 | Adjust based on how you manage caller credits |

A privilege of `65535` effectively disables a feature for all normal
callers (only Sysop access). A privilege of `0` makes it available to
everyone.

---

## Message Edit Attributes {#message-edit}

```toml
[message_edit.ask]
private = 65535
crash = 100
# ...

[message_edit.assume]
private = 65535
crash = 65535
# ...
```

When a caller writes a netmail message, Maximus can set various FTN
message attributes. These are controlled at two levels:

### ask — "Would you like to mark this message as...?"

The `[message_edit.ask]` section sets the **minimum privilege** at which
Maximus will *ask* the caller whether to set each attribute. Below this
privilege, the caller isn't even offered the choice.

### assume — "This attribute is set automatically"

The `[message_edit.assume]` section sets the privilege at which Maximus
**automatically sets** the attribute without asking. This is for
convenience — high-privilege callers (or the sysop) can have certain
attributes applied by default.

### Available attributes

| Attribute | FTN Meaning |
|-----------|-------------|
| `private` | Message is private (only sender and recipient can read) |
| `crash` | Send immediately — high priority, triggers immediate delivery |
| `fileattach` | Message has an attached file |
| `killsent` | Delete the message after it's been successfully sent |
| `hold` | Hold for pickup — don't actively deliver, wait for recipient to poll |
| `filerequest` | Request a file from the destination system |
| `updaterequest` | Request a file update from the destination system |
| `localattach` | File attach using a local file path |

For most sysops, the defaults are fine. The main ones you might adjust:

- Lower `crash` ask privilege if you want trusted callers to send
  urgent netmail.
- Lower `private` ask privilege if you want callers to be able to mark
  netmail as private (though netmail is inherently private-ish).
- Keep `fileattach` and `filerequest` relatively high — these features
  consume real bandwidth and disk space.

---

## Exit Codes (Legacy Integration) {#exit-codes}

```toml
after_edit_exit = 11
after_echomail_exit = 12
after_local_exit = 0
```

These are **errorlevel exit codes** — a legacy mechanism from the DOS
batch-file era. When a caller posts a message, Maximus can exit with a
specific errorlevel, which the controlling batch file or wrapper script
uses to trigger Squish.

| Setting | Errorlevel | Meaning |
|---------|-----------|---------|
| `after_edit_exit` | 11 | Caller posted a netmail message |
| `after_echomail_exit` | 12 | Caller posted an echomail message |
| `after_local_exit` | 0 | Caller posted a local message (no FTN action needed) |

### Modern usage

On modern Linux/Unix systems running Maximus under MaxTel, these exit
codes are less relevant — you're not running inside a batch loop. The
typical modern approach is:

- **Cron-based:** Run `squish in out squash` on a schedule (every
  15–30 minutes). The echotoss log ensures Squish only scans areas with
  new mail. Exit codes aren't needed.
- **Event-driven:** Use MaxTel's event system or an inotify watcher on
  the echotoss log to trigger Squish immediately when new mail is
  posted.

If you're not using a batch loop, you can leave these at their defaults
— they won't cause harm, they just won't trigger anything.

---

## Message Area Types {#area-types}

When defining message areas in Maximus (through MaxCFG or
`areas.toml`), the **area type** field determines how Maximus treats
messages in that area:

| Type | Behavior |
|------|----------|
| `local` | Local messages only. No network interaction. Messages stay on your BBS. |
| `echo` | EchoMail. Messages are shared across the network. Maximus writes to the echotoss log when callers post. Origin lines are added. |
| `netmail` | NetMail. Private point-to-point messages. Maximus prompts for a destination FTN address when composing. |
| `conference` | Conference-style area. Similar to echo but may have different display/access behavior depending on your configuration. |

Each area also needs a **path** that matches the corresponding area
definition in `squish.cfg`. For example, if your Squish config says:

```
EchoArea  FSX_GEN  /var/max/data/msgbase/fsx_gen  -$  21:1/100
```

Then your Maximus area definition needs to point to the same path:

```toml
[[area]]
name = "fsxNet General"
tag = "FSX_GEN"
type = "echo"
path = "data/msgbase/fsx_gen"
```

The path in `areas.toml` is relative to `sys_path`; the path in
`squish.cfg` is typically absolute. They must resolve to the same
directory.

For full area definition details, see
[MaxCFG Areas]({% link maxcfg-areas.md %}).

---

## FTN Paths in maximus.toml {#maximus-toml}

**File:** `config/maximus.toml`

Three paths in the core config are FTN-related:

```toml
net_info_path = "data/nodelist"
outbound_path = "data/mail/outbound"
inbound_path  = "data/mail/inbound"
```

| Setting | Purpose | Must Match |
|---------|---------|------------|
| `net_info_path` | Where nodelist files live | BinkD's nodelist path |
| `outbound_path` | Where outbound mail archives go | `Outbound` in `squish.cfg` and BinkD's `outbound` |
| `inbound_path` | Where incoming mail arrives | `NetFile` in `squish.cfg` and BinkD's `inbound` |

All three paths are resolved relative to `sys_path`. The golden rule:
**Maximus, Squish, and BinkD must all agree on where files go.** If
your mail isn't flowing, mismatched paths are the first thing to check.

See [Directory Structure]({% link directory-structure.md %}) for the
full layout and [Core Settings]({% link config-core-settings.md %}) for
details on `sys_path` and path resolution.
