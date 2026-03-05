---
layout: default
title: "SqaFix"
section: "tools"
description: "SqaFix — automatic echomail area management for FTN networks"
---

SqaFix is an area-fix processor for FidoNet-Technology Networks. It
automates the process of creating, linking, and unlinking echomail areas
on your system in response to remote requests — typically netmail commands
sent by your hub or downlinks.

When a downlink sends an areafix request to your system (a netmail to
"SqaFix" or "AreaFix" with area tags in the body), SqaFix processes
that request: creating new areas if needed, updating Squish's
configuration, and sending a confirmation back. It's the tool that lets
your FTN connections manage their own echo subscriptions without you
having to manually edit config files every time.

SqaFix can also handle:

- **Auto-creating areas** when your hub sends you a new echo you haven't
  seen before
- **Forwarding requests** upstream to your hub on behalf of your
  downlinks
- **Listing available areas** in response to `%LIST` requests

---

## Where to Learn More

SqaFix is part of the FTN networking stack and its configuration lives
alongside Squish's. The FidoNet documentation covers it in context:

- **[FidoNet & FTN Networking]({{ site.baseurl }}{% link fidonet.md %})** — the big picture
  of how FTN works and where area management fits in
- **[Joining a Network]({{ site.baseurl }}{% link fidonet-joining-network.md %})** —
  setting up your first network connection, including area subscriptions
- **[Echomail]({{ site.baseurl }}{% link fidonet-echomail.md %})** — echomail areas,
  subscriptions, and how SqaFix automates area management
- **[Squish Tosser]({{ site.baseurl }}{% link fidonet-squish.md %})** — Squish
  configuration, which SqaFix modifies when processing area requests

---

## See Also

- [Squish]({{ site.baseurl }}{% link squish.md %}) — the mail tosser that SqaFix works
  alongside
- [Netmail & Routing]({{ site.baseurl }}{% link fidonet-netmail-routing.md %}) — how
  areafix requests travel as netmail
