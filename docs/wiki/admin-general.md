---
layout: default
title: "General Administration"
section: "administration"
description: "The care and feeding of your Maximus BBS — daily upkeep, user management, backups, and keeping things humming"
permalink: /admin-general/
---

Running a BBS is a little like tending a garden. You plant the seeds —
configure the menus, set up your message areas, pick out the ANSI art — and
then the callers show up and things start *growing*. New users register.
Messages pile up. File areas fill. Logs accumulate. And somewhere in there,
you realize that the fun part (building it) has quietly handed off to the
important part (keeping it alive).

This page is about that second part. Not the glamorous stuff — the
*necessary* stuff. The habits that keep your board healthy, your users happy,
and your 3 AM self from panicking because something went sideways and you
can't figure out what.

If you've just finished setting up Maximus and you're looking at a running
system wondering "okay, now what?" — you're in the right place.

---

## On This Page

- [The Sysop's Daily Rhythm](#daily-rhythm)
- [Users: Your Board's Lifeblood](#users)
- [Backups: Because Things Happen](#backups)
- [Logs: Your Silent Witness](#logs)
- [Housekeeping Tasks](#housekeeping)
- [When Something Goes Wrong](#troubleshooting)

---

## The Sysop's Daily Rhythm {#daily-rhythm}

Most BBS administration isn't dramatic. It's a quick check-in: skim the
logs, glance at who called, make sure nothing's on fire. A healthy board
can run unattended for days or weeks. But a *well-run* board has a sysop
who pokes their head in regularly.

Here's a rough cadence that works well:

**Every day (or every login):**
- Skim the system log for errors or unusual activity
- Check for new user registrations that need validation
- Glance at disk space — especially if you host file areas

**Weekly:**
- Review and prune inactive or suspicious accounts
- Rotate or archive old logs
- Verify backups are completing successfully

**Monthly:**
- Audit access levels and privileges
- Clean up orphaned files in upload directories
- Test a backup restore (yes, really — untested backups aren't backups)

You don't need a calendar reminder for all of this. Once you get into the
rhythm, it takes five minutes most days. The key is *consistency* — small
regular checks prevent big surprise disasters.

---

## Users: Your Board's Lifeblood {#users}

Users are why you're doing this. Managing them well is the single most
important admin task.

**New user validation** — Depending on your security settings, new callers
may need sysop approval before they get full access. Check your pending
queue regularly. Nothing kills a new user's enthusiasm faster than signing
up and then waiting three days to actually use the board.

**Privilege levels** — Maximus uses a class-based access system. New users
start at whatever privilege level you've configured, and you promote (or
demote) from there. Keep your class structure simple — three or four levels
is plenty for most boards. Overcomplicating access control is a classic
sysop trap.

**Problem users** — They happen. Maximus gives you tools: you can lock
accounts, adjust privileges, set expiration dates, or remove users
entirely. Handle it quickly, handle it fairly, and document what you did
in case it comes up again.

**The user database** — Lives in your data directory. It's a binary file,
so you manage it through Maximus tools (MaxCFG's user editor, or MEX
scripts), not by hand-editing. Back it up. Seriously.

For the full details on user operations, see
[User Management]({% link admin-user-management.md %}).

---

## Backups: Because Things Happen {#backups}

Your BBS is a collection of files — configuration, user database, message
bases, file areas, logs. All of it lives on disk. Disks fail. Configs get
fat-fingered. Sometimes you just want to undo something you did at 2 AM
that seemed like a great idea at the time.

**What to back up:**

| What | Where | Priority |
|------|-------|----------|
| Configuration | `config/` (TOML files) | **Critical** — this IS your board |
| User database | `data/` directory | **Critical** — your users' accounts |
| Message bases | Squish/JAM data files | **High** — conversation history |
| File areas | Your download directories | **Medium** — large, but replaceable if you have sources |
| Display files | `.ans`, `.bbs`, `.mec` files | **Medium** — your board's personality |
| MEX scripts | `.mex` source + `.vm` compiled | **Medium** — your custom code |
| Logs | `log/` directory | **Low** — useful but regenerated |

**How often:**
- Config + user DB: daily (they're small, there's no excuse)
- Message bases: daily or weekly depending on volume
- Everything else: weekly is fine

**Where:**
- Somewhere that isn't the same disk. A different machine, a cloud bucket,
  a USB drive you rotate — anything that survives if the primary disk dies.

**Test your restores.** A backup you've never restored from is a hope, not
a plan. Once a month, pick a backup and verify you can actually bring a
system back from it.

For backup procedures and scripts, see
[Backup & Recovery]({% link admin-backup-recovery.md %}).

---

## Logs: Your Silent Witness {#logs}

Maximus logs system events, caller activity, errors, and security-relevant
actions. When something goes wrong — or when you just want to understand
what's happening on your board — logs are where you start.

**What gets logged:**
- Caller connections and disconnections
- Login successes and failures
- Area changes and message operations
- MEX script execution
- Errors and warnings
- Sysop actions (user edits, config changes)

**Reading logs effectively:**
- Don't try to read every line. Scan for `ERROR`, `WARNING`, and anything
  that looks unusual.
- Pay attention to patterns — the same error repeating is more important
  than a one-off hiccup.
- Timestamps are your friend when correlating "something broke" with "what
  happened right before."

**Log rotation:**
- Logs grow forever if you let them. Set up rotation (daily or weekly) so
  old logs get compressed or archived. A simple cron job or scheduled task
  handles this.
- Keep at least two weeks of logs accessible. Archive older ones if you
  have the space.

For log locations, formats, and troubleshooting techniques, see
[Logging & Troubleshooting]({% link admin-logging-troubleshooting.md %}).

---

## Housekeeping Tasks {#housekeeping}

These are the little things that add up over time:

**Orphaned files** — Users upload files, file areas get reorganized, and
sometimes files end up on disk without a corresponding database entry (or
vice versa). Periodically scan your file areas for orphans and either
catalog them or clean them out.

**Message base maintenance** — Squish message bases can accumulate slack
space over time. The Squish utilities (`sqpack`) can repack them. If you
notice message areas getting sluggish, a repack usually helps.

**Disk space** — BBS file areas can grow surprisingly fast, especially if
you're running a file distribution network. Monitor disk usage and set
reasonable limits. Running out of disk space mid-session is not a good
look.

**Event scheduling** — Maximus supports timed events (defined in
`events00.bbs`). Use them for automated maintenance: log rotation, message
base packing, nightly backups. The system handles the scheduling; you just
define what runs and when.

---

## When Something Goes Wrong {#troubleshooting}

It will. Here's the short version of the troubleshooting playbook:

1. **Check the logs.** Almost every problem leaves a trace.
2. **Check recent changes.** If it worked yesterday and doesn't today,
   what changed? A config edit? A new MEX script? An OS update?
3. **Reproduce it.** Can you make it happen again? Consistent bugs are
   easy to fix. Intermittent ones are where the real fun begins.
4. **Isolate it.** Is it one user, one node, one area, or everything?
   Narrow the scope before you start changing things.
5. **Change one thing at a time.** The fastest way to make a problem worse
   is to change three things at once and not know which one helped (or
   didn't).

For detailed troubleshooting procedures and common issues, see
[Logging & Troubleshooting]({% link admin-logging-troubleshooting.md %}).

---

## See Also

- [User Management]({% link admin-user-management.md %}) — creating,
  editing, and removing user accounts
- [Backup & Recovery]({% link admin-backup-recovery.md %}) — backup
  strategies, scripts, and restore procedures
- [Logging & Troubleshooting]({% link admin-logging-troubleshooting.md %})
  — log formats, rotation, and debugging techniques
- [MaxTel]({% link maxtel.md %}) — the telnet server and node management
- [Security & Access]({% link config-security-access.md %}) — access
  levels, privileges, and flags
