---
layout: default
title: "Backup & Recovery"
section: "administration"
description: "What to back up, how often, where to put it, and how to get your board back when things go sideways"
---

In the [General Administration]({% link admin-general.md %}) overview, we
said "a backup you've never restored from is a hope, not a plan." This page
is about turning hope into a plan. We'll cover exactly what files matter,
how to back them up safely, and — the part nobody likes to think about —
how to bring your board back from a backup when you need to.

Running a BBS without backups is fine right up until the moment it isn't.
Disks fail. Configs get mangled at 2 AM. Power outages hit mid-write.
Someone (maybe you, maybe a user, maybe a MEX script with a bug) deletes
something that shouldn't have been deleted. The question isn't *if* you'll
need a backup — it's *when*.

---

## On This Page

- [What to Back Up](#what-to-back-up)
- [Backup Priorities](#priorities)
- [The User Database](#user-database)
- [Configuration Files](#config-files)
- [Language Files & Deltas](#lang-files)
- [Message Bases](#message-bases)
- [Display Files & MEX Scripts](#display-mex)
- [A Complete Backup Script](#backup-script)
- [Where to Store Backups](#storage)
- [Backup Schedule](#schedule)
- [Restoring from Backup](#restoring)
- [Testing Your Backups](#testing)

---

## What to Back Up {#what-to-back-up}

Your BBS is a collection of directories under `$PREFIX`. Not all of them
are equally important. Some are critical (lose them and you're rebuilding
from scratch), some are replaceable (annoying to lose but recoverable),
and some are ephemeral (regenerated every time the BBS starts).

Here's the full picture:

| Directory | Contents | Backup? |
|-----------|----------|---------|
| `config/` | All TOML configuration | **Critical** |
| `data/users/` | `user.db` (SQLite), `callers.bbs` | **Critical** |
| `data/msgbase/` | Squish message bases | **High** |
| `data/filebase/` | File area storage | **Medium** |
| `display/` | `.ans`, `.bbs`, `.mec` display files | **Medium** |
| `scripts/` | `.mex` source + `.vm` compiled | **Medium** |
| `data/nodelist/` | FidoNet nodelists | **Low** (re-downloadable) |
| `data/mail/` | Network mail transit | **Low** (transient) |
| `log/` | System logs | **Low** (useful but regenerated) |
| `run/` | Per-node state, temp files | **Skip** (ephemeral) |
| `bin/` | Executables and libraries | **Skip** (reinstallable) |

The `run/` directory is entirely ephemeral — it's recreated every time
MaxTel starts. Don't waste backup space on it. The `bin/` directory is
your installed software — if you lose it, you reinstall from your build
or release package.

---

## Backup Priorities {#priorities}

If you had to pick just two things to back up, it would be:

1. **`config/`** — This *is* your board. Every menu, every area definition,
   every access level, every session setting. Rebuilding this from memory
   is painful and error-prone.
2. **`data/users/user.db`** — Your user accounts. Names, passwords,
   privilege levels, statistics. You can't get this back once it's gone.

Everything else is either recoverable (display files can be recreated,
message bases can be seeded fresh, file areas can be re-uploaded) or
transient (logs, nodelists, temp files). Important? Yes. But not
*existential* the way config and user data are.

---

## The User Database {#user-database}

`user.db` is a SQLite database. SQLite has specific considerations for
safe backups:

### The Safe Way: `.backup` Command

SQLite has a built-in backup command that creates a consistent snapshot
even while the database is in use:

```bash
sqlite3 /var/max/data/users/user.db ".backup /backups/max/user.db"
```

This uses SQLite's backup API internally — it handles locking, ensures
consistency, and won't give you a half-written file. This is the
**recommended approach** for automated backups.

### The Quick Way: File Copy (With Caveats)

You *can* just copy `user.db` with `cp`, but only if no process is
writing to it at the time. If the BBS is running and a user happens to
be mid-login when you copy, you could get a corrupt backup.

Safe options:
- Stop MaxTel, copy, restart MaxTel (brief downtime)
- Use `sqlite3 .backup` instead (no downtime)
- Use filesystem snapshots (ZFS, LVM, Btrfs) that provide atomic
  point-in-time copies

### Don't Forget `callers.bbs`

The caller log (`data/users/callers.bbs`) isn't critical — it's a
running log of who called and when — but it's nice to have for historical
purposes. Include it in your backup if size isn't a concern.

---

## Configuration Files {#config-files}

The `config/` directory contains everything that defines your board's
personality and behavior:

```
config/
├── maximus.toml              # Core system config
├── matrix.toml               # FidoNet network config
├── compress.cfg              # Archiver definitions
├── squish.cfg                # Squish tosser config
├── general/                  # Session, equipment, display, colors
│   ├── session.toml
│   ├── equipment.toml
│   ├── display_files.toml
│   ├── colors.toml
│   ├── protocol.toml
│   └── reader.toml
├── lang/                     # Language files
│   ├── english.toml
│   ├── delta_english.toml
│   └── colors.lh
├── menus/                    # Menu definitions
│   ├── main.toml
│   └── ...
├── areas/                    # Area definitions
│   ├── msg/areas.toml
│   └── file/areas.toml
└── security/                 # Access control
    ├── access_levels.toml
    ├── access.dat
    └── baduser.bbs
```

These are all text files (TOML, cfg, or simple lists). They're small,
they compress well, and they're the single most important thing to
protect. A full backup of `config/` is rarely more than a few hundred
kilobytes.

### Version Control

Because these are text files, they're perfect candidates for version
control. Consider keeping a Git repository of your `config/` directory:

```bash
cd /var/max/config
git init
git add -A
git commit -m "Initial board configuration"
```

Then after every significant change:

```bash
cd /var/max/config
git add -A
git commit -m "Added Amiga file area, tweaked session timeout"
```

This gives you:
- A full history of every config change
- The ability to diff any two points in time
- Easy rollback: `git checkout <commit> -- menus/main.toml`
- A remote backup if you push to GitHub/GitLab/etc.

This is arguably the single best thing you can do for your board's
long-term health. When something breaks after a config change, `git diff`
tells you exactly what changed.

---

## Language Files & Deltas {#lang-files}

Language files live in `config/lang/` and control every string the BBS
displays to callers. The system uses a layered approach:

| File | Role | Back Up? |
|------|------|----------|
| `english.toml` | Base language strings (stock conversion from legacy `.mad`) | **Yes** |
| `delta_english.toml` | Overlay with theme colors and metadata | **Yes** |
| `colors.lh` | Color header mappings | **Yes** |

The base `english.toml` is a direct conversion from the legacy language
file and should never be hand-edited — all customizations go in the delta.
But you should still back up both, because:

- Your `english.toml` may have been generated from your own custom legacy
  `.mad` file (not the stock one)
- The delta contains your theme color choices and any string overrides
- Regenerating either from scratch means re-running the conversion pipeline

If you're using version control on `config/`, the lang files are already
covered. If not, include the entire `config/lang/` directory in your
backup.

---

## Message Bases {#message-bases}

Squish message bases live in `data/msgbase/` and consist of multiple files
per area (`.sqd`, `.sqi`, `.sql`). They can grow large on active boards.

### Backup Strategy

- **Active boards:** Back up message bases daily. Message loss is
  noticeable and annoying to your users.
- **Low-traffic boards:** Weekly is probably fine.
- **FidoNet echomail areas:** These can be re-seeded from your uplink if
  lost, so they're lower priority than local areas.

### Squish-Specific Notes

Squish bases can accumulate slack space over time. If your backups are
getting large, run `sqpack` before backing up to compact them:

```bash
# Pack all Squish bases (reduces backup size)
squish pack *
```

Back up *after* packing — the packed files are smaller and in a cleaner
state.

### Consistency

Squish uses file locking for multi-node safety, but a backup taken while
a message is being posted could theoretically capture a half-written
state. For critical backups:

1. Stop MaxTel (brief downtime)
2. Run `sqpack` on all bases
3. Back up `data/msgbase/`
4. Restart MaxTel

For daily automated backups, the risk of corruption from a hot copy is
low — Squish writes are atomic at the record level — but the
stop-pack-copy-start approach is safest for your "golden" weekly backup.

---

## Display Files & MEX Scripts {#display-mex}

| What | Where | Notes |
|------|-------|-------|
| Display screens | `display/screens/`, `display/menus/`, `display/help/` | `.ans`, `.bbs`, `.mec` files — your board's visual identity |
| MEX source | `scripts/*.mex` | Your custom scripts |
| MEX compiled | `scripts/*.vm` | Can be recompiled from source, but nice to have |
| MEX headers | `scripts/include/` | Standard `.mh` files (reinstallable, but custom ones need backup) |

The display files are your board's personality — the login screen, the
menu art, the help screens. Losing them means recreating (or re-drawing)
every ANSI screen. If you've spent hours in PabloDraw or TheDraw making
your board look just right, *back these up*.

MEX source files are the important ones — compiled `.vm` files can be
regenerated with the MEX compiler. But including both in the backup means
you can restore and be running immediately without a recompilation step.

---

## A Complete Backup Script {#backup-script}

Here's a script that covers everything, suitable for a daily cron job:

```bash
#!/bin/bash
# max-backup.sh — Daily backup for Maximus BBS
#
# Usage: max-backup.sh [backup_dir]
# Default backup dir: /backups/max

set -euo pipefail

PREFIX="${MAX_INSTALL_PATH:-/var/max}"
BACKUP_DIR="${1:-/backups/max}"
DATE=$(date +%Y-%m-%d)
DEST="${BACKUP_DIR}/${DATE}"

mkdir -p "${DEST}"

echo "[$(date)] Starting Maximus backup to ${DEST}"

# --- Critical: User database (safe SQLite backup) ---
echo "  Backing up user database..."
sqlite3 "${PREFIX}/data/users/user.db" \
  ".backup '${DEST}/user.db'"

# --- Critical: Configuration ---
echo "  Backing up configuration..."
tar czf "${DEST}/config.tar.gz" \
  -C "${PREFIX}" config/

# --- High: Message bases ---
echo "  Backing up message bases..."
tar czf "${DEST}/msgbase.tar.gz" \
  -C "${PREFIX}" data/msgbase/

# --- Medium: Display files ---
echo "  Backing up display files..."
tar czf "${DEST}/display.tar.gz" \
  -C "${PREFIX}" display/

# --- Medium: MEX scripts ---
echo "  Backing up MEX scripts..."
tar czf "${DEST}/scripts.tar.gz" \
  -C "${PREFIX}" scripts/

# --- Medium: File area data ---
if [ -d "${PREFIX}/data/filebase" ]; then
  echo "  Backing up file areas..."
  tar czf "${DEST}/filebase.tar.gz" \
    -C "${PREFIX}" data/filebase/
fi

# --- Low: Caller log ---
if [ -f "${PREFIX}/data/users/callers.bbs" ]; then
  cp "${PREFIX}/data/users/callers.bbs" "${DEST}/"
fi

# --- Prune old backups (keep 14 days) ---
echo "  Pruning backups older than 14 days..."
find "${BACKUP_DIR}" -maxdepth 1 -type d \
  -name '20[0-9][0-9]-[0-9][0-9]-[0-9][0-9]' \
  -mtime +14 -exec rm -rf {} +

echo "[$(date)] Backup complete: ${DEST}"
ls -lh "${DEST}"
```

### Setting It Up

```bash
# Make it executable
chmod +x /usr/local/bin/max-backup.sh

# Add to crontab — run daily at 4:00 AM
crontab -e
# Add this line:
0 4 * * * /usr/local/bin/max-backup.sh >> /var/log/max-backup.log 2>&1
```

### macOS (launchd)

If you're running on macOS, use a LaunchDaemon instead of cron:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key>
  <string>com.maximus.backup</string>
  <key>ProgramArguments</key>
  <array>
    <string>/usr/local/bin/max-backup.sh</string>
  </array>
  <key>StartCalendarInterval</key>
  <dict>
    <key>Hour</key>
    <integer>4</integer>
    <key>Minute</key>
    <integer>0</integer>
  </dict>
  <key>StandardOutPath</key>
  <string>/var/log/max-backup.log</string>
  <key>StandardErrorPath</key>
  <string>/var/log/max-backup.log</string>
</dict>
</plist>
```

Save as `/Library/LaunchDaemons/com.maximus.backup.plist` and load:

```bash
sudo launchctl load /Library/LaunchDaemons/com.maximus.backup.plist
```

---

## Where to Store Backups {#storage}

The golden rule: **backups go somewhere that isn't the same disk.**

| Option | Pros | Cons |
|--------|------|------|
| **Second local disk** | Fast, simple | Doesn't survive a machine failure |
| **NFS/SMB share** | Off-machine, easy to script | Network dependency |
| **rsync to remote host** | Off-site, efficient incremental | Needs SSH setup |
| **Cloud storage** (S3, B2, etc.) | Off-site, durable, cheap | Needs tooling, latency |
| **USB drive rotation** | Air-gapped, survives everything | Manual effort, easy to forget |

For most sysops, `rsync` to a second machine is the sweet spot. Add this
to the end of your backup script:

```bash
# Sync to remote backup host
rsync -az --delete "${BACKUP_DIR}/" backup@remote:/backups/max/
```

For cloud storage, tools like `rclone` make it easy:

```bash
# Sync to Backblaze B2 (or S3, Google Cloud, etc.)
rclone sync "${BACKUP_DIR}" b2:my-bbs-backups/max/
```

The best backup strategy is one you'll actually maintain. Pick something
that fits your setup and automate it. Manual backups are the ones that
don't happen.

---

## Backup Schedule {#schedule}

Here's a practical schedule aligned with the
[General Administration]({% link admin-general.md %}) rhythm:

| What | Frequency | Method |
|------|-----------|--------|
| Config + user DB | **Daily** | Automated script |
| Message bases | **Daily** | Automated script |
| Display files + MEX | **Weekly** | Automated script (or on change) |
| File areas | **Weekly** | Automated script |
| Full golden backup | **Weekly** | Stop BBS, pack, full backup, restart |
| Off-site sync | **Daily** | rsync / rclone after backup completes |
| Restore test | **Monthly** | Manual — verify a backup actually works |

The daily automated backup covers the things that change constantly
(user data, messages). The weekly golden backup is the one you'd use for
a full disaster recovery — taken with the BBS stopped so everything is
in a guaranteed-clean state.

---

## Restoring from Backup {#restoring}

When the moment comes, here's the procedure:

### Full Restore (Disaster Recovery)

If you've lost everything and you're starting from a fresh install:

1. **Install Maximus** — get the binaries in place (`bin/`)
2. **Stop MaxTel** if it's running
3. **Restore configuration:**
   ```bash
   cd /var/max
   tar xzf /backups/max/2025-01-15/config.tar.gz
   ```
4. **Restore user database:**
   ```bash
   cp /backups/max/2025-01-15/user.db /var/max/data/users/user.db
   ```
5. **Restore message bases:**
   ```bash
   tar xzf /backups/max/2025-01-15/msgbase.tar.gz
   ```
6. **Restore display files and scripts:**
   ```bash
   tar xzf /backups/max/2025-01-15/display.tar.gz
   tar xzf /backups/max/2025-01-15/scripts.tar.gz
   ```
7. **Verify permissions** — make sure the BBS user can read everything
8. **Start MaxTel** and test

### Partial Restore (Fixing a Mistake)

More commonly, you just need to undo a specific change:

**Restore a single config file:**
```bash
# Extract just session.toml from yesterday's backup
tar xzf /backups/max/2025-01-15/config.tar.gz \
  config/general/session.toml
cp config/general/session.toml /var/max/config/general/session.toml
```

**Restore the user database:**
```bash
# Stop the BBS first!
sudo systemctl stop maxtel  # or kill MaxTel manually
cp /backups/max/2025-01-15/user.db /var/max/data/users/user.db
sudo systemctl start maxtel
```

**Restore a single message area:**
```bash
# Extract specific Squish files
tar xzf /backups/max/2025-01-15/msgbase.tar.gz \
  data/msgbase/local.sqd data/msgbase/local.sqi data/msgbase/local.sql
cp data/msgbase/local.* /var/max/data/msgbase/
```

**Roll back config with Git** (if you followed the version control advice):
```bash
cd /var/max/config
git log --oneline -10          # find the commit to roll back to
git checkout abc1234 -- menus/main.toml   # restore one file
# or
git revert HEAD                # undo the last commit entirely
```

---

## Testing Your Backups {#testing}

This is the part everyone skips and then regrets. A backup is only as
good as your ability to restore from it. Once a month, do this:

1. **Pick a recent backup** — ideally not today's
2. **Create a temporary directory:**
   ```bash
   mkdir /tmp/max-restore-test
   cd /tmp/max-restore-test
   ```
3. **Restore everything into it:**
   ```bash
   tar xzf /backups/max/2025-01-10/config.tar.gz
   tar xzf /backups/max/2025-01-10/display.tar.gz
   tar xzf /backups/max/2025-01-10/scripts.tar.gz
   cp /backups/max/2025-01-10/user.db .
   ```
4. **Verify the files are intact:**
   ```bash
   # Check SQLite integrity
   sqlite3 user.db "PRAGMA integrity_check;"
   # Should print: ok

   # Check config files parse
   cat config/maximus.toml | head -20

   # Check display files exist
   ls -la display/screens/
   ```
5. **Clean up:**
   ```bash
   rm -rf /tmp/max-restore-test
   ```

If step 4 fails — if the SQLite integrity check reports errors, or
config files are truncated, or tar reports corruption — you have a
backup problem and you need to fix it *now*, while your live system is
still healthy.

The five minutes this takes each month is cheap insurance against the
alternative: discovering your backups are broken at the exact moment you
desperately need them.

---

## See Also

- [General Administration]({% link admin-general.md %}) — the daily rhythm
  and best practices overview
- [User Management]({% link admin-user-management.md %}) — protecting and
  managing user accounts
- [Directory Structure]({% link directory-structure.md %}) — the full
  layout of files and paths
- [MaxTel]({% link maxtel.md %}) — stopping and starting the BBS for
  maintenance windows
