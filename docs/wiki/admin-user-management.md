---
layout: default
title: "User Management"
section: "administration"
description: "User accounts, access control, problem users, reserved names, password resets, and the flags that shape caller behavior"
---

Your users are the reason the modem light blinks. Every caller who creates
an account becomes a row in your database, a privilege level in your access
system, and — if things go well — a regular who posts messages, uploads
files, and tells other people about your board. Managing those accounts is
one of the most hands-on parts of being a sysop.

This page covers the administrative side of user management: how accounts
work, what happens when new callers show up, how to handle the ones who
cause trouble, and the flags and tools that give you fine-grained control
over individual users. For the MaxCFG editor interface itself — the screens,
fields, and key bindings — see the
[MaxCFG User Editor]({{ site.baseurl }}{% link maxcfg-user-editor.md %}) page.

---

## On This Page

- [The User Database](#user-database)
- [New User Registration](#new-user-registration)
- [Privilege Levels & Promotion](#privilege-levels)
- [Password Management](#passwords)
- [Problem Users & Bad Users](#problem-users)
- [Reserved Names](#reserved-names)
- [The Bad Logon Flag](#bad-logon)
- [Nerd Mode](#nerd-mode)
- [Other Per-User Flags](#other-flags)
- [Bulk Operations](#bulk-operations)

---

## The User Database {#user-database}

Maximus NG stores all user accounts in a **SQLite database** (`user.db`)
in your system path. Every account — from your sysop login to the newest
caller — lives in this single file. It's a big improvement over the legacy
fixed-record `user.bbs` format: it's faster, it doesn't fragment, and you
can inspect it with any SQLite tool if you ever need to.

For day-to-day work, you'll use the
[MaxCFG User Editor]({{ site.baseurl }}{% link maxcfg-user-editor.md %}) to browse, search,
and modify accounts. But it's worth knowing that the database is just a
file — which means backing it up is as simple as copying `user.db` to your
backup location. Do that regularly. Your user database is one of the few
things on your board that you genuinely can't recreate from scratch.

---

## New User Registration {#new-user-registration}

When someone connects to your BBS for the first time and their name isn't
in the database, Maximus walks them through new user registration. The
exact flow depends on your `session.toml` settings:

| Setting | What It Controls |
|---------|-----------------|
| `alias_system` | Whether the board uses handles (aliases) or real names as the primary identity |
| `ask_alias` | Whether new users are prompted for an alias during registration |
| `single_word_names` | Whether single-word names are allowed (suppresses "What is your LAST name?") |
| `no_real_name` | Whether to skip the real name requirement entirely |
| `ask_phone` | Whether to prompt for a phone number |
| `logon_priv` | The default privilege level assigned to new accounts |

Once registration completes, the new user lands at whatever `first_menu`,
`first_message_area`, and `first_file_area` you've configured. They start
at the privilege level set by `logon_priv`.

### Validation Workflow

Some sysops prefer to validate new users manually before granting full
access. The typical approach:

1. Set `logon_priv` to a restricted level (e.g., "New User" or
   "Unvalidated") that only has access to a welcome area and maybe a
   registration questionnaire.
2. When a new caller registers, they get that restricted level
   automatically.
3. You review the account — check the name, city, maybe their answers to
   a questionnaire MEX script — and if everything looks good, promote them
   to a regular access level using the
   [User Editor]({{ site.baseurl }}{% link maxcfg-user-editor.md %}).

This adds friction for new users, so use it judiciously. On a public board
with lots of traffic, manual validation can become a bottleneck fast. On a
smaller or more security-conscious board, it's a reasonable tradeoff.

For the full session configuration reference, see
[Session & Login]({{ site.baseurl }}{% link config-session-login.md %}).

---

## Privilege Levels & Promotion {#privilege-levels}

Every user has a privilege level that determines what they can access —
which menus, message areas, file areas, and commands are available to them.
Maximus uses a class-based system where each level has a name, a numeric
value, and a set of associated limits (time, download bytes, etc.).

Common setups look something like:

| Level | Typical Use |
|-------|------------|
| **Disgrace** | Restricted/probation — can barely do anything |
| **Limited** | Unvalidated new users waiting for sysop approval |
| **Normal** | Standard validated caller |
| **Privileged** | Trusted users with extra access |
| **Sysop** | Full access to everything |

You promote or demote users through the User Editor's **Security** category.
Select the privilege field, pick a level from the list, save. The change
takes effect on the user's next action (they don't need to log off and
back on).

A few tips:

- **Keep it simple.** Three or four levels is plenty for most boards.
  Every level you add is another thing to maintain and debug.
- **Use keys for fine-grained access.** Instead of creating a dozen
  privilege levels, use access keys (`/A`, `/B`, `/C`, etc.) to gate
  specific areas or commands. You can assign keys per-user without changing
  their overall level.
- **Document your levels.** Write down what each level can do. Future-you
  will thank present-you when a user asks "why can't I access the Amiga
  file area?"

For the full access control reference, see
[Security & Access]({{ site.baseurl }}{% link config-security-access.md %}) and
[Access Levels]({{ site.baseurl }}{% link config-access-levels.md %}).

---

## Password Management {#passwords}

Passwords are one of those things you'll deal with more often than you'd
like. Users forget them. Users mistype them. Users set them to "password"
and then wonder why bad things happen.

### Resetting a Password

Open the user in MaxCFG's User Editor → **Security** → **Password**. You'll
be prompted to enter and confirm a new password. If encryption is enabled
(`maximus.encrypt_pw = true`), you can't see the current password — only
replace it. This is by design.

After resetting, you'll need to communicate the new password to the user
through some out-of-band channel. If you're running a retro board with
actual phone callers, this might be a voice call. For a telnet board, it's
probably email or a message on another platform.

### Password Encryption

When encryption is enabled, passwords are hashed (MD5) before storage.
This means:

- You can't recover a forgotten password — only replace it
- The database doesn't contain plaintext passwords (good for security)
- If you're migrating from a legacy system with plaintext passwords,
  enabling encryption will require all users to set new passwords on their
  next login

If encryption is *off*, passwords are stored in plaintext. This is simpler
but not recommended for any board accessible over the internet. Enable
encryption unless you have a specific reason not to.

---

## Problem Users & Bad Users {#problem-users}

They show up eventually. Maybe it's someone posting garbage in your message
areas. Maybe it's someone trying to social-engineer access. Maybe it's just
someone who's rude to your other callers. You have tools for this.

### Graduated Response

Not every problem requires a ban. Here's a reasonable escalation ladder:

1. **Talk to them.** Send a message. Most people respond to a polite "hey,
   knock it off." You'd be surprised.
2. **Restrict access.** Demote them to a lower privilege level that limits
   what they can do. This is a good cooling-off measure.
3. **Enable the Bad Logon flag.** This marks the account as suspicious (see
   [below](#bad-logon)). It doesn't prevent login, but it alerts you.
4. **Lock the account.** Set their privilege to the lowest level (Disgrace)
   and set an expiration that axes the account.
5. **Add them to the Bad Users list.** This prevents login entirely.
6. **Delete the account.** The nuclear option. Use the User Editor to
   remove them from the database.

### The Bad Users List

MaxCFG provides a dedicated editor for this under **Users → Bad Users**.
It manages a text file (`etc/baduser.bbs`) containing usernames that are
rejected at login. If someone tries to log in with a name on this list,
Maximus turns them away before they even get to the password prompt.

The editor is simple — add names, remove names, save:

| Key | Action |
|-----|--------|
| **Enter** | Edit the selected name |
| **Insert** | Add a new name |
| **Delete** | Remove the selected name |
| **Esc** | Save and exit |

Names are matched case-insensitively. If you add "TroubleUser" to the
list, "troubleuser", "TROUBLEUSER", and "tRouBleUsEr" are all blocked.

Keep in mind that this only blocks the *name*. A determined troll can
create a new account with a different name. For persistent problems, you
may need to look at IP-based blocking at the network level (firewall rules,
MaxTel configuration, or your reverse proxy).

---

## Reserved Names {#reserved-names}

Separate from the Bad Users list, Maximus maintains a **Reserved Names**
list (`etc/reserved.bbs`). Names on this list can't be used for new user
registration — but unlike Bad Users, they don't block login for existing
accounts that already have those names.

This is for preventing impersonation and confusion. You'll want to reserve:

- **"Sysop"**, **"SysOp"**, **"System Operator"** — obvious ones
- **"Admin"**, **"Administrator"**, **"Root"** — people will try
- **"Guest"** — unless you actually have a guest account
- **Your board's name** — so nobody registers as "The Midnight BBS"
- **Your own name/alias** — protect your identity
- **"All"**, **"Everyone"** — prevents confusing message addressing

Access the editor from MaxCFG under **Users → Reserved Names**. Same
interface as Bad Users — add, edit, delete, escape to save.

A good practice is to populate this list when you first set up your board,
before anyone has a chance to claim these names. It's much easier to
prevent the problem than to clean it up after someone has been posting as
"SysOp" for a week.

---

## The Bad Logon Flag {#bad-logon}

The `BadLogon` flag is a per-user security indicator. When set, it means
the account has had suspicious login activity — typically failed password
attempts.

Maximus sets this flag automatically when a caller fails to provide the
correct password after the allowed number of attempts. The flag persists
until a sysop manually clears it.

**What it does:**
- The flag is visible in the User Editor under **Keys/Flags**
- It serves as an alert to the sysop that something happened
- It does *not* prevent the user from logging in on its own — it's
  informational
- You can combine it with MEX scripts that check `usr.badlogon` and take
  additional action (extra logging, notification to the sysop, temporary
  lockout, etc.)

**What to do when you see it:**
1. Check your logs for the failed login attempts
2. Determine if it looks like the actual user forgetting their password or
   someone else trying to break in
3. If it's the real user, reset their password and clear the flag
4. If it looks malicious, consider adding the name to the Bad Users list
   or investigating further

To clear the flag, open the user in MaxCFG → **Keys/Flags** → toggle
**BadLogon** off → save.

---

## Nerd Mode {#nerd-mode}

The `Nerd` flag is one of those classic BBS features with a name that
hasn't aged particularly well, but the functionality is straightforward:
when enabled, the user **cannot yell for the sysop**.

In the Maximus context, "yelling" means using the chat/page command to
get the sysop's attention. On a board where you're actively monitoring
(sitting at the console or watching MaxTel), callers can page you for a
live chat. The Nerd flag disables this for a specific user.

**When to use it:**
- Users who abuse the page function (paging repeatedly, paging at 3 AM,
  paging to complain about things that aren't emergencies)
- Automated or guest accounts that shouldn't be able to page
- Any situation where you want the user to have normal access but not the
  ability to interrupt you

The flag is available in the User Editor under **Keys/Flags** → **Nerd**.
You can also check and set it programmatically in MEX via `usr.nerd`.

It's a lighter touch than restricting access — the user can still do
everything else on the board. They just can't ring your doorbell.

---

## Other Per-User Flags {#other-flags}

Beyond BadLogon and Nerd, there are several other per-user flags worth
knowing about:

| Flag | What It Does | When You'd Use It |
|------|-------------|-------------------|
| **NotAvail** | Marks the user as unavailable for inter-node chat | User preference — they can set this themselves |
| **NoUList** | Hides the user from the public "who's online" list | Privacy-conscious users, or test/bot accounts you don't want visible |
| **Deleted** | Marks the account for deletion | Soft-delete — the record stays but the user can't log in |
| **Permanent** | Prevents the account from expiring | Your sysop account, co-sysops, anyone who should never age out |
| **Configured** | Indicates the user has completed initial setup | Set automatically after first-time configuration; clearing it forces re-setup |
| **Hotkeys** | Enables single-keypress commands | User preference, but you might disable it for users who struggle with accidental key presses |

Most of these are set by the users themselves or automatically by the
system. You'd only touch them manually to fix a problem or enforce a
policy.

---

## Bulk Operations {#bulk-operations}

Sometimes you need to do something to a lot of users at once — reset
everyone's daily download counters, purge accounts that haven't logged in
for a year, or grant a bonus to all active users. The MaxCFG user editor
works one account at a time, which is fine for individual cases but
painful for bulk work.

For bulk operations, you have two options:

### SQLite Direct Access

Since `user.db` is a standard SQLite database, you can run queries
directly:

```sql
-- Find users who haven't logged in for 180 days
SELECT name, city, last_on FROM users
WHERE last_on < date('now', '-180 days');

-- Reset daily download counters for everyone
UPDATE users SET downtoday = 0, ndowntoday = 0;

-- Delete users inactive for over a year (careful!)
DELETE FROM users
WHERE last_on < date('now', '-365 days')
  AND permanent = 0;
```

Use the `sqlite3` command-line tool, DB Browser for SQLite, or any
language with SQLite bindings. **Stop the BBS first** if you're doing
writes — MaxCFG and Maximus use proper locking, but external tools
might not.

### MEX Scripts

For operations that need to happen while the BBS is running, or that
need access to BBS logic (like recalculating ratios), you can write a
MEX script that iterates through users:

```mex
#include <max.mh>

void main()
{
  struct _usr: u;

  if (userfindopen("", "", u) = 0)
  {
    do
    {
      // Your logic here — check fields, update, etc.
      if (u.times = 0 and u.permanent = 0)
        log("Inactive: " + u.name);
    }
    while (userfindnext(u) = 0);
  }
  userfindclose();
}
```

See [Examples & Recipes]({{ site.baseurl }}{% link mex-examples-recipes.md %}) for more
patterns.

---

## See Also

- [General Administration]({{ site.baseurl }}{% link admin-general.md %}) — the daily
  rhythm of running a board
- [MaxCFG User Editor]({{ site.baseurl }}{% link maxcfg-user-editor.md %}) — the editor
  interface, fields, and key bindings
- [Session & Login]({{ site.baseurl }}{% link config-session-login.md %}) — registration
  flow and session policy
- [Security & Access]({{ site.baseurl }}{% link config-security-access.md %}) — the
  privilege and access control system
- [Access Levels]({{ site.baseurl }}{% link config-access-levels.md %}) — defining and
  configuring class levels
- [Backup & Recovery]({{ site.baseurl }}{% link admin-backup-recovery.md %}) — protecting
  your user database
