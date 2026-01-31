# Maximus BBS

A classic bulletin board system from the DOS/OS2 era, now running on modern operating systems based on UNIX (macOS, Linux, FreeBSD, etc.)

![Maximus running on macOS ARM64](docs/screenshot.png)

## What is This?

Maximus was one of the premier BBS packages of the early 90s, developed by Scott Dudley and Lanius Corporation. It supported FidoNet messaging, file transfers, MEX scripting, and all the features that made BBSing great. The source was released under GPL after Lanius ceased operations.

This fork focuses on getting Maximus compiling and running on modern systems so folks who want to relive the BBS days or run a retro board can actually do so without hunting down a DOS emulator or vintage hardware.

## Current Status

| Platform | Status |
|----------|--------|
| macOS arm64 (Apple Silicon) | Tested |
| macOS x86_64 (Intel) | Tested |
| Linux x86_64 | Tested |
| Linux arm64 | Should work |
| FreeBSD | Supported |

**What works:** Full BBS operation via telnet access (MAXTEL), MEX scripting, all utilities (maid, mecca, mex compiler). Most features should continue to work as expected and are assumed to work unless otherwise noted (e.g. FidoNet messaging, file transfers).

## Quick Start: Running Your Own BBS

Want to be a SysOp? Ready to command your own retro battle station? You don't need to compile anything - just grab a release and go. The release packages include everything you need to run a BBS. 

### 1. Download and Extract

Download the latest release for your platform from [GitHub Releases](https://github.com/LimpingNinja/maximus/releases):

```bash
# Example for macOS ARM64
tar -xzvf maximus-3.04a-r2-macos-arm64.tar.gz
cd maximus-3.04a-r2-macos-arm64
```

### 2. Run the Install Script

The interactive install script configures your BBS name, sysop name, and sets up all paths:

```bash
bin/install.sh
```

Follow the prompts to name your BBS and set your sysop name. The script sets up the TOML configuration under `config/`.

### 3. Create Your Sysop Account (Required First Time)

Log in locally to create your sysop account and the user database:

```bash
bin/runbbs.sh -c
```

Enter your sysop name (matching what you set during install), create a password, and exit with `G` (Goodbye). Remote connections will fail until this step is complete.

### 4. Launch with MAXTEL

MAXTEL is the telnet supervisor that manages multi-node access to your BBS:

```bash
# Start your BBS with 4 nodes on port 2323
bin/maxtel -p 2323 -n 4
```

That's it. Your BBS is now accepting callers. Connect with any telnet client:
```bash
telnet localhost 2323
```

### Upgrading Your BBS

When a new version is released, you only need to update the executables and libraries. Your configuration, message bases, and user data are preserved. If there are new configurations, scripts, display files, or help files these will be noted in the release notes and [CHANGES.md](CHANGES.md) file. 

```bash
# Download and extract the new release
tar -xzvf maximus-NEW-VERSION.tar.gz

# Copy only bin/ and lib/ to your existing installation
cp -r maximus-NEW-VERSION/bin/* /path/to/your/bbs/bin/
cp -r maximus-NEW-VERSION/lib/* /path/to/your/bbs/lib/

# Optionally update docs
cp -r maximus-NEW-VERSION/docs/* /path/to/your/bbs/docs/
```

**What to update:**
- `bin/` and `lib/` - Always safe, contains executables and libraries

**What to keep:**
- `config/` - Your BBS configuration (TOML)
- `etc/*.ctl` - Legacy CTL inputs (optional; only needed if you re-export to TOML)
- `spool/` - Message bases and file areas
- User database files

**Optional updates:**
- `m/*.mex` - MEX scripts (only if you haven't customized them)
- `etc/misc/`, `etc/help/` - Display files (only if you haven't customized them)

## Telnet: MAXTEL Supervisor

MAXTEL is the preferred way to run Maximus for telnet access. It provides a real-time ncurses dashboard showing node status, connected users, and caller history.

**Features:**
- Multi-node management (1-32 simultaneous nodes)
- Automatic node assignment for incoming callers
- Real-time status display with responsive layouts
- Headless mode (`-H`) for running under process supervisors
- Daemon mode (`-D`) for background operation

**Common usage:**
```bash
bin/maxtel -p 2323 -n 4         # Interactive mode with UI
bin/maxtel -p 2323 -n 4 -H      # Headless (no UI)
bin/maxtel -p 2323 -n 4 -D      # Daemon (background)
```

See `docs/maxtel.md` for complete documentation.

## Quick Start: Building from Source

If you want to hack on the code or build from source, we have platform-specific scripts that handle everything:

**macOS:**
```bash
./scripts/build-macos.sh              # Auto-detect architecture
./scripts/build-macos.sh arm64        # Apple Silicon
./scripts/build-macos.sh x86_64       # Intel (or Rosetta)
```

**Linux:**
```bash
./scripts/build-linux.sh              # Build for your system
```

Both scripts run configure, clean, build, and install to `build/`. Add `release` to create a distributable package.

See [BUILD.md](BUILD.md) for detailed instructions, manual build steps, dependencies, and troubleshooting.

## Why?

Sometimes you just want to make sure old code still runs. Picking up retro projects, getting them to compile on modern systems, maybe adding a feature or two, and leaving them around for others who share the interest - that's what this is about. Maximus deserved to keep running.

If you're into BBS history, FidoNet nostalgia, or just appreciate software archaeology, feel free to poke around. Pull requests welcome if you get something working that hasn't been tackled yet.

## History

- **1989-2004**: Developed by Lanius Corporation
- **~2003**: UNIX port by Wes Garland, Bo Simonsen, and R.F. Jones
- **2025**: macOS/Linux modernization and MAXTEL by Kevin Morgan (LimpingNinja)

## License

GNU General Public License (GPL). See [LICENSE](LICENSE) for details.

## Links

- [Original Maximus Site](http://maximus.sourceforge.net/) (historical)
- [BUILD.md](BUILD.md) - Build instructions and documentation
- [CHANGES.md](CHANGES.md) - Release notes and changelog
- [FidoNet](https://en.wikipedia.org/wiki/FidoNet) - If you're wondering what all this FREQ/netmail stuff is about
