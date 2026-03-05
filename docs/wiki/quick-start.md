---
layout: default
title: "Quick Start"
section: "getting-started"
description: "Get Maximus BBS up and running quickly"
---

This page gets you from zero to a running BBS in four steps. No compiling
required — just grab a release package.

If you want to build from source instead, see
[Building Maximus]({{ site.baseurl }}{% link building.md %}).

---

## 1. Download and Extract

Download the latest release for your platform from
[GitHub Releases](https://github.com/LimpingNinja/maximus/releases):

```bash
tar -xzvf maximus-<version>-<platform>.tar.gz
cd maximus-<version>-<platform>
```

The archive contains a complete BBS installation tree:

| Directory | Contents |
|-----------|----------|
| `bin/` | Executables (`max`, `maxtel`, `maxcfg`, `mecca`, `mex`, `squish`) |
| `lib/` | Shared libraries |
| `config/` | TOML configuration files |
| `config/lang/` | Language strings (`english.toml`) and delta overlay |
| `display/` | Compiled display files (help screens, menus) |
| `scripts/` | Compiled MEX scripts and includes |
| `data/` | User database, message bases, file areas |

See [Directory Structure]({{ site.baseurl }}{% link directory-structure.md %}) for the full
layout.

---

## 2. Run the Install Script

The interactive install script configures your BBS name, sysop name, and
updates all paths in the TOML configuration:

```bash
bin/install.sh
```

Follow the prompts. The script writes your settings into `config/maximus.toml`
and updates path references across the configuration tree.

---

## 3. Create Your Sysop Account

Log in locally to create your sysop account and initialize the user database:

```bash
bin/runbbs.sh -c
```

Enter your sysop name (matching what you set during install), create a
password, and exit with **G** (Goodbye). Remote connections will fail until
this step is complete.

See [First Login & Sysop Account]({{ site.baseurl }}{% link first-login.md %}) for details on
privilege setup and what happens during first login.

---

## 4. Launch with MAXTEL

MAXTEL is the telnet supervisor that manages multi-node access:

```bash
bin/maxtel -p 2323 -n 4
```

Your BBS is now accepting callers on port 2323 with up to 4 simultaneous
nodes. Connect with any telnet client:

```bash
telnet localhost 2323
```

See [MaxTel]({{ site.baseurl }}{% link maxtel.md %}) for headless mode, daemon mode, and the
full ncurses dashboard.

---

## What's Next

- **Customize your BBS** — edit TOML files in `config/` directly. No compile
  step; changes take effect on restart. See
  [Configuration Overview]({{ site.baseurl }}{% link configuration.md %}).
- **Edit language strings** — modify `config/lang/english.toml` or use the
  interactive editor via `bin/maxcfg`. See
  [Language Files (TOML)]({{ site.baseurl }}{% link lang-toml.md %}).
- **Recompile display files** — if you edit `.mec` source files, run
  `bin/recompile.sh` or see [Building Maximus]({{ site.baseurl }}{% link building.md %}).
- **Upgrade later** — when a new release drops, see
  [Upgrading]({{ site.baseurl }}{% link upgrading.md %}).
- **Migrate from Maximus 3.0** — if you have legacy CTL/MAD files, see
  [Legacy Migration]({{ site.baseurl }}{% link legacy-migration.md %}).

---

## See Also

- [First Login & Sysop Account]({{ site.baseurl }}{% link first-login.md %}) — sysop account
  setup and first-run details
- [Upgrading]({{ site.baseurl }}{% link upgrading.md %}) — how to update to a new release
- [Directory Structure]({{ site.baseurl }}{% link directory-structure.md %}) — where files live
- [Building Maximus]({{ site.baseurl }}{% link building.md %}) — building from source
- [MaxTel]({{ site.baseurl }}{% link maxtel.md %}) — telnet supervisor documentation
- [Legacy Migration]({{ site.baseurl }}{% link legacy-migration.md %}) — upgrading from
  Maximus 3.0
