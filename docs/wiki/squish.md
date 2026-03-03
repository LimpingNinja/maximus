---
layout: default
title: "Squish"
section: "tools"
description: "Squish — the FTN mail tosser, scanner, and packer bundled with Maximus"
---

Squish is the mail processor that connects your Maximus BBS to
FidoNet-Technology Networks. It's the middle piece of the FTN stack —
sitting between your BBS (where callers read and post messages) and your
mailer (which transfers packets over the network).

In short, Squish does three things:

- **Toss** — takes incoming mail packets from your mailer's inbound
  directory and places messages into the correct local message areas
- **Scan** — finds outbound messages your callers have posted and builds
  packets for the network
- **Pack** — compresses outbound packets into archives for your mailer to
  send

A typical invocation runs all three at once:

```bash
squish in out squash
```

Squish is configured through `squish.cfg`, which defines your FTN
addresses, echomail areas, routing rules, and packing behavior.

---

## Where to Learn More

Squish is deeply tied to FTN networking — its configuration only makes sense
in that context. Rather than duplicate that material here, the FidoNet
documentation covers everything you need:

- **[FidoNet & FTN Networking]({% link fidonet.md %})** — what FTN is, how
  mail flows, and where Squish fits in the stack
- **[Joining a Network]({% link fidonet-joining-network.md %})** — the
  step-by-step tutorial that walks you through setting up Squish as part of
  your first network connection
- **[Squish Tosser]({% link fidonet-squish.md %})** — the full `squish.cfg`
  reference with all configuration directives
- **[Echomail]({% link fidonet-echomail.md %})** — dupe checking, SEEN-BY
  lines, passthru areas, and echomail security
- **[Netmail & Routing]({% link fidonet-netmail-routing.md %})** — private
  messaging and mail routing configuration

---

## See Also

- [SqaFix]({% link sqafix.md %}) — automatic echomail area management
- [MECCA Compiler]({% link mecca-compiler.md %}) — another tool in the
  Maximus suite
