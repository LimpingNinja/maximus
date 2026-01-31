# Maximus BBS - Modern Build Guide

## Overview

Maximus BBS is a classic bulletin board system originally written for DOS/OS2/Windows, 
ported to UNIX around 2003. This document covers building on modern systems including 
Linux, FreeBSD, and macOS (Darwin).

## Architecture

### Component Libraries (Build Order)

```
1. src/libs/unix/libcompat.so            - UNIX compatibility layer (DOS/OS2 API emulation)
2. src/libs/slib/libmax.so               - Core utility library
3. src/libs/msgapi/libmsgapi.so          - Message base API (Squish/SDM format)
4. src/libs/btree/libmaxbt.so            - B-tree indexing library (C++)
5. src/libs/btree/libmaxbtree.so         - Database/tracking library (C++)
6. src/libs/prot/libxfer.so              - File transfer protocols (X/Y/Zmodem)
7. mex/libmexvm.so                       - MEX scripting VM (compiler/runtime split happens in Phase 4)
8. src/libs/legacy/comdll/libcomm.so     - Communications drivers (IP/modem)
```

### Programs

- **squish** - Message tosser/scanner for FidoNet (`src/utils/squish/`)
- **max** - Main BBS executable (`src/max/`)
- **sqafix** - Squish area fix utility (`src/utils/sqafix/`)
- **util/** - Various utilities (maid, mecca, etc.) (`src/utils/util/`)
- **maxcfg** - Configuration editor (`src/apps/maxcfg/`)
- **maxtel** - Telnet supervisor (`src/apps/maxtel/`)

## Dependencies

### Required
- GCC or Clang (C/C++ compiler)
- GNU Make 3.79+
- Bison or Yacc
- ncurses development libraries

### Optional
- makedepend (for dependency tracking)

### Platform-Specific

**Linux:**
```bash
apt install build-essential libncurses-dev bison
```

**macOS (Darwin):**
```bash
xcode-select --install   # Provides clang, make, bison
# ncurses is included in macOS SDK
```

**WSL (Windows Subsystem for Linux with Ubuntu) /Untested/:**
```bash
sudo apt update
sudo apt install build-essential libncurses-dev bison
```

**FreeBSD /Untested/:**
```bash
pkg install gmake gcc ncurses bison
```
## Build Instructions

### Prerequisites

**Important:** The `resources/install_tree/` directory must exist with the base configuration
files. This contains the default control files, language files, MEX scripts, help
screens, and directory structure needed for a working BBS.

If `resources/install_tree/` is missing, extract it from the included archive:

```bash
mkdir -p resources
tar -xzvf install_tree.tar.gz -C resources
```

The archive contains:
- `etc/` - Configuration files (max.ctl, access.ctl, menus.ctl, etc.)
- `etc/lang/` - Language definition files
- `etc/help/` - Help screen templates (.mec)
- `etc/misc/` - Display file templates (.mec)
- `m/` - MEX scripts and headers (.mex, .mh)
- `etc/rip/` - RIP graphics files
- `spool/` - Default spool directories for mail/files
- `man/` - Documentation

### Quick Start (Recommended)

Use the platform-specific build scripts for the easiest experience:

**macOS:**
```bash
./scripts/build-macos.sh              # Build for native arch (auto-detect)
./scripts/build-macos.sh arm64        # Build for Apple Silicon
./scripts/build-macos.sh x86_64       # Build for Intel (via Rosetta on AS)
./scripts/build-macos.sh release      # Build + create release package
./scripts/build-macos.sh arm64 release # Specific arch + release
```

**Linux:**
```bash
./scripts/build-linux.sh              # Build for native arch
./scripts/build-linux.sh release      # Build + create release package
```

These scripts handle `./configure`, cleaning, building, and optionally packaging.

### Manual Build

```bash
# For local development build
./configure --prefix=$(pwd)/build
make build
make install

# For system-wide installation  
./configure --prefix=/var/max
make build
sudo make install
```

The `make install` target:
1. Copies `resources/install_tree/` to PREFIX (first install only)
2. Adjusts paths in config files for your PREFIX
3. Compiles language files, help screens, and MEX scripts
4. Creates initial user database

### Step-by-Step

```bash
# 1. Configure
./configure --prefix=/var/max

# 2. Build Squish only
make squish

# 3. Build everything
make build

# 4. Install
make install
```

### Build Targets

| Target | Description |
|--------|-------------|
| `build` | Build all components (squish, max, sqafix) |
| `squish` | Build Squish message tosser only |
| `max` | Build Maximus BBS only |
| `sqafix` | Build SqaFix only |
| `squish_install` | Build and install Squish |
| `max_install` | Build and install Maximus |
| `config_install` | Install configuration files |
| `clean` | Remove build artifacts |
| `archclean` | Clean + remove build/lib (for cross-arch builds) |
| `distclean` | Remove all generated files |

## Library Dependencies

```
libcompat.so  <- ncurses (standalone)
libmax.so     <- libcompat.so, ncurses
libmsgapi.so  <- libmax.so, libcompat.so
libmaxbt.so   <- libcompat.so (C++)
libmaxbtree.so<- libmaxbt.so, libcompat.so (C++)
libxfer.so    <- libmax.so, libcompat.so
libmexvm.so   <- libmax.so, libcompat.so
libcomm.so    <- libmax.so, libcompat.so
```

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| macOS arm64 | ✓ Tested | Apple Silicon native |
| macOS x86_64 | ✓ Tested | Intel or Rosetta on AS |
| Linux x86_64 | ✓ Tested | GCC, OrbStack, native |
| Linux ARM64 | ✓ Should work | Untested |
| FreeBSD | ✓ Supported | Use gmake |
| Solaris/SunOS | ✓ Supported | Big-endian issues with FidoNet packets |
| Windows | ✗ Broken | Would need significant work |

## Post-Install: Getting Started

After `make install`, you have a working BBS skeleton in your PREFIX directory.

### Directory Structure

```
$PREFIX/
├── bin/           # Executables (max, squish, mex, maid, mecca, maxcfg, etc.)
├── lib/           # Shared libraries
├── config/        # TOML configuration files
├── etc/           # Legacy CTL configuration inputs (optional; convertible to TOML)
│   ├── max.ctl    # Main legacy control file (includes other .ctl files)
│   ├── access.ctl # Legacy access levels/privileges
│   ├── msgarea.ctl# Legacy message area definitions
│   ├── filearea.ctl# Legacy file area definitions
│   ├── menus.ctl  # Legacy menu definitions
│   ├── lang/      # Language files
│   ├── help/      # Help screens (compiled from .mec)
│   └── misc/      # Misc display files (compiled from .mec)
└── m/             # MEX scripts (.vm compiled from .mex)
```

### Key Utilities

| Utility | Purpose |
|---------|--------|
| `maxcfg`| TOML configuration editor and legacy CTL → TOML conversion |
| `maid`  | Compiles language files (.mad) |
| `mecca` | Compiles display files (.mec → .bbs) |
| `mex`   | Compiles MEX scripts (.mex → .vm) |
| `max`   | Main BBS executable |
| `squish`| FidoNet message tosser/scanner |

### Manual Recompilation

If you edit language, display, or MEX source files, recompile with:

```bash
cd $PREFIX

# Recompile language files
bin/maid etc/lang/english -d -s -p$PREFIX/config/maximus

# Recompile display files
bin/mecca etc/help/*.mec
bin/mecca etc/misc/*.mec

# Recompile MEX scripts (bash/zsh)
export MEX_INCLUDE=$PREFIX/m
for f in m/*.mex; do bin/mex "$f"; done

# Recompile MEX scripts (fish)
# set -x MEX_INCLUDE $PREFIX/m
# for f in m/*.mex; bin/mex "$f"; end
```

### Migrating from Legacy CTL Configuration

If you have an older Maximus 3.0 installation with `.ctl` files and want to migrate to TOML:

```bash
cd $PREFIX
bin/maxcfg --export-nextgen "$PREFIX/etc/max.ctl"
```

This converts your legacy CTL configuration to TOML files in `$PREFIX/config/`.

To specify a different output directory:

```bash
bin/maxcfg --export-nextgen "$PREFIX/etc/max.ctl" --export-dir "$PREFIX/config"
```

### Environment Setup

Set your PREFIX path before running commands. The syntax varies by shell:

**Bash/Zsh:**
```bash
export PREFIX=/path/to/maximus/build
export MEX_INCLUDE=$PREFIX/m
export PATH="$PREFIX/bin:$PATH"
```

**Fish:**
```fish
set -x PREFIX /path/to/maximus/build
set -x MEX_INCLUDE $PREFIX/m
set -x PATH $PREFIX/bin $PATH
```

To make permanent, add to your shell config (`~/.bashrc`, `~/.zshrc`, or `~/.config/fish/config.fish`).

### Running the BBS

**Recommended: Use MAXTEL for telnet access**

MAXTEL is the multi-node telnet supervisor with a real-time ncurses dashboard:

```bash
cd $PREFIX
bin/maxtel -p 2323 -n 4         # Start 4 nodes on port 2323
```

See `docs/maxtel.md` for full documentation including headless and daemon modes.

**Alternative: Direct execution**

```bash
cd $PREFIX

# Local console mode (testing)
bin/max config/maximus -c

# Single node with wait-for-call
bin/max config/maximus -w -pt1
```

### Fresh Install / Reset

To reset to a fresh install state (removes all user data, message bases, etc.):

```bash
# Option 1: Remove and rebuild PREFIX directory (complete reset)
rm -rf build/
./configure --prefix=$(pwd)/build
make install

# Option 2: Force reinstall config files (backs up existing)
scripts/copy_install_tree.sh build --force
# Backups saved to build/backup-YYYYMMDD-HHMMSS/

# Option 3: Manual reset of specific directories
rm -rf build/etc build/m build/spool
scripts/copy_install_tree.sh build
```

### Configuration Workflow

1. Edit TOML files in `config/` to customize your BBS
2. Run `bin/recompile.sh` only if you changed language/display/MEX source files
3. Test with `bin/runbbs.sh -c` (local console mode)
4. Start telnet server with `bin/maxtel -p 2323 -n 4`

## Known Issues

### macOS/Darwin
- Uses clang instead of GCC (some flags differ)
- Shared library linking requires explicit dependencies  
- Config files contain ANSI codes; sed requires `LC_ALL=C` for processing
- ARM64 requires removing `packed` attribute from pointer-containing structs
- `malloc.h` is `stdlib.h` on macOS
- `pty.h` is `util.h` on macOS
- `/bin/true` is at `/usr/bin/true`

### General
- Big-endian platforms may have issues with FidoNet packet handling
- C++ code in btree/ generates warnings with modern compilers

## Cross-Compilation

### macOS Cross-Architecture (ARM64 ↔ x86_64)

On Apple Silicon Macs, you can build for both architectures:

```bash
# Using build script (recommended)
./scripts/build-macos.sh arm64       # Native ARM64
./scripts/build-macos.sh x86_64      # Intel via Rosetta

# Manual method
make clean
make install                          # Native ARM64

make archclean                        # Clean for arch switch
make ARCH=x86_64 install              # x86_64 via Rosetta
```

For x86_64 on Apple Silicon, Rosetta 2 is required:
```bash
softwareupdate --install-rosetta
```

The `ARCH` variable adds `-arch x86_64` flags to compiler and linker.
Use `archclean` when switching architectures to remove old libraries.

### Switching Between macOS and Linux

The `./configure` script generates `vars.mk` with platform-specific paths.
**You must re-run configure when switching between platforms** (e.g., after
building on Linux via OrbStack and returning to macOS):

```bash
# On macOS after building on Linux
./scripts/build-macos.sh

# Or manually
./configure --prefix=$(pwd)/build
make clean
make build
```

### Release Packaging

Use the release scripts to create distributable packages:

```bash
# Build + package in one step
./scripts/build-macos.sh release
./scripts/build-linux.sh release

# Or use release scripts directly
./scripts/make-release.sh arm64
./scripts/make-release.sh x86_64

# Build all available architectures
./scripts/make-all-releases.sh --all
```

Release packages are created in `release/maximus-VERSION-OS-ARCH.tar.gz`

### Linux Build

For Linux builds, use the build script which handles dependencies check:

```bash
./scripts/build-linux.sh              # Build
./scripts/build-linux.sh release      # Build + package
```

**Dependencies (Debian/Ubuntu):**
```bash
sudo apt-get install build-essential bison libncurses-dev
```

**Dependencies (Alpine):**
```bash
apk add build-base bison ncurses-dev
```

**Dependencies (RHEL/Fedora):**
```bash
sudo dnf install gcc make bison ncurses-devel
```

For cross-compiling Linux binaries from macOS, use Docker or a Linux VM
(e.g., OrbStack on macOS).

## File Locations

| Path | Purpose |
|------|---------|
| `/var/max/bin/` | Executables |
| `/var/max/lib/` | Shared libraries |
| `/var/max/etc/` | Configuration files |
| `/var/max/m/` | MEX scripts |

## Debugging

```bash
# Build with debug symbols
./configure --build=DEBUG

# Run with tracing
strace -f ./bin/max -w -pt1  # Linux
ktrace ./bin/max -w -pt1     # BSD
dtruss ./bin/max -w -pt1     # macOS
```

## Contributing

See `HACKING` for coding style and contribution guidelines.
