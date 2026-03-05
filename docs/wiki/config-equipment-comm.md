---
layout: default
title: "Equipment & Comm"
section: "configuration"
description: "equipment.toml — modem, serial port, and connection settings"
---

This page covers `config/general/equipment.toml` — the file that defines how
Maximus handles the connection layer. If you're running Maximus behind MaxTel
(which handles all the TCP/telnet work), most of these settings are vestigial
and safe to leave at their defaults. They exist because Maximus was originally
designed to answer phone calls directly through a serial modem.

If you're running in local console mode (`output = "local"`) or behind MaxTel,
the modem strings (`busy`, `init`, `ring`, `answer`, `connect`) are never
sent. The only setting that matters for local operation is `output`.

---

## Quick Reference

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `output` | string | `"com"` | I/O mode: `"com"` for serial port, `"local"` for console |
| `com_port` | int | `1` | COM port number when `output = "com"` |
| `baud_maximum` | int | `38400` | Highest baud rate the system supports |
| `busy` | string | *(modem string)* | Command sent to the modem when a user logs off |
| `init` | string | *(modem string)* | Command sent when the WFC subsystem starts |
| `ring` | string | `"Ring"` | Substring Maximus watches for to detect an incoming call |
| `answer` | string | `"ATA\|"` | Command sent to answer an incoming call |
| `connect` | string | `"Connect"` | Substring returned by the modem on successful connection |
| `carrier_mask` | int | `128` | Carrier-detect bitmask — leave at default unless you know otherwise |
| `handshaking` | array | `["xon", "cts"]` | Flow control: `xon` (software), `cts` (hardware), `dsr` |
| `send_break` | bool | `false` | Send BREAK to clear the modem buffer (rare; most modems don't need this) |
| `no_critical` | bool | `false` | Disable Maximus' internal critical error handler |

---

## Deep Dives

### Output Mode

The `output` key is the most important setting here:

- **`"com"`** — Maximus talks to a serial port. This was the standard mode
  for dial-up BBSes. On modern systems, this is only relevant if you're
  connecting through a real or emulated serial device.
- **`"local"`** — Maximus runs on the local console with no serial I/O. This
  is what `bin/runbbs.sh -c` uses for local console login.

When running behind MaxTel, the telnet supervisor handles all TCP/IP
communication and passes data to Maximus through a PTY. Maximus doesn't
interact with the modem strings at all in this mode.

### Modem Strings

The `busy`, `init`, `answer`, and `connect` strings use a simple escape
language inherited from the DOS era:

| Escape | Meaning |
|--------|---------|
| `v` | Set DTR low |
| `^` | Set DTR high |
| `~` | Pause 1 second |
| `` ` `` | Pause 1/20th of a second |
| `\|` | End of command sequence |

These strings are only meaningful if `output = "com"` and you're driving a
real modem. For MaxTel or local operation, they're ignored.

### Handshaking

The `handshaking` array controls flow control. For high-speed modem
operation, `cts` (hardware flow control) is essential. `xon` enables
software flow control (Ctrl-S/Ctrl-Q), which lets callers pause screen
output. The defaults (`["xon", "cts"]`) are appropriate for most setups.

---

## See Also

- [Core Settings]({{ site.baseurl }}{% link config-core-settings.md %}) — system identity,
  paths, and logging (`maximus.toml`)
- [Session & Login]({{ site.baseurl }}{% link config-session-login.md %}) — user experience and
  policy settings (`session.toml`)
- [MaxTel]({{ site.baseurl }}{% link maxtel.md %}) — the telnet supervisor that replaces
  direct modem handling
- [Running MaxTel]({{ site.baseurl }}{% link maxtel-running.md %}) — starting MaxTel in
  various modes
