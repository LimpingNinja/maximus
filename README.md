# Maximus BBS

A classic bulletin board system from the DOS/OS2 era, now running on modern UNIX systems.

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

**What works:** Full BBS operation, telnet access via MAXTEL, MEX scripting, FidoNet messaging, file transfers, all utilities (maid, silt, mecca, mex compiler).

## Quick Start: Running Your Own BBS

Want to be a SysOp? Ready to command your own retro battle station? You don't need to compile anything - just grab a release and go.

### 1. Download and Extract

Download the latest release for your platform from [GitHub Releases](https://github.com/LimpingNinja/maximus/releases):

```bash
# Example for macOS ARM64
tar -xzvf maximus-3.04a-r2-macos-arm64.tar.gz
cd maximus-3.04a-r2-macos-arm64
```

### 2. Customize Your BBS

Edit `etc/max.ctl` to give your BBS a name and identity:

```bash
# Open in your favorite editor
nano etc/max.ctl
# or: vim etc/max.ctl
```

Find and change these lines near the top:
```
Name            Your Battle Station [EXAMPLE]
SysOp           Your Name [EXAMPLE]
```

After editing, recompile the configuration:
```bash
bin/silt etc/max -x
```

### 3. Launch with MAXTEL

MAXTEL is the telnet supervisor that manages multi-node access to your BBS:

```bash
# Start your BBS with 4 nodes on port 2323
bin/maxtel -p 2323 -n 4
```

That's it. Your BBS is now accepting callers. Connect with any telnet client:
```bash
telnet localhost 2323
```

### 4. Test Locally (Optional)

To test without telnet, run in local console mode:
```bash
bin/max etc/max -c
```

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
