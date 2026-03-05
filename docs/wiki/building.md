---
layout: default
section: "getting-started"
title: "Building Maximus"
description: "Compiling Maximus BBS from source"
---

Most sysops grab a release package and skip straight to
[Quick Start]({{ site.baseurl }}{% link quick-start.md %}). But if you want to build from source
— whether you're hacking on the code, targeting an unusual platform, or just
prefer to compile everything yourself — this page has you covered.

Maximus builds on Linux, FreeBSD, and macOS (including Apple Silicon). The
build system is GNU Make with a `./configure` script that detects your platform
and sets up paths. A full build takes under a minute on modern hardware.

---

## Architecture

### Component Libraries (Build Order)

```
1. src/libs/unix/libcompat.so            - UNIX compatibility layer (DOS/OS2 API emulation)
2. src/libs/slib/libmax.so               - Core utility library
3. src/libs/msgapi/libmsgapi.so          - Message base API (Squish/SDM format)
4. src/libs/btree/libmaxbt.so            - B-tree indexing library (C++)
5. src/libs/btree/libmaxbtree.so         - Database/tracking library (C++)
6. src/libs/prot/libxfer.so              - File transfer protocols (X/Y/Zmodem)
7. mex/libmexvm.so                       - MEX scripting VM
8. src/libs/legacy/comdll/libcomm.so     - Communications drivers (IP/modem)
```

### Programs

- **squish** — Message tosser/scanner for FidoNet (`src/utils/squish/`)
- **max** — Main BBS executable (`src/max/`)
- **sqafix** — Squish area fix utility (`src/utils/sqafix/`)
- **util/** — Various utilities (mecca, etc.) (`src/utils/util/`)
- **maxcfg** — Configuration editor (`src/apps/maxcfg/`)
- **maxtel** — Telnet supervisor (`src/apps/maxtel/`)

---

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

---

## Build Instructions

### Prerequisites

The `resources/install_tree/` directory must exist with the base configuration
files. This contains the default control files, language files, MEX scripts,
help screens, and directory structure needed for a working BBS.

If `resources/install_tree/` is missing, extract it from the included archive:

```bash
mkdir -p resources
tar -xzvf install_tree.tar.gz -C resources
```

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

These scripts handle `./configure`, cleaning, building, and optionally
packaging.

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
3. Compiles display files (.mec → .bbs) and MEX scripts (.mex → .vm)
4. Creates initial user database

Language files are TOML — no compilation step needed.

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

---

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

---

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

---

## Upgrading from Legacy Maximus

Maximus NG uses TOML for all configuration and language files. The legacy
pipelines — SILT (CTL → PRM), MAID (MAD → LTF) — are retired. If you are
building from source to upgrade an existing Maximus 3.0 installation, you will
need to convert your CTL configuration files and MAD language files to TOML
before running.

Fresh installs ship with ready-to-use TOML files — no conversion needed.

See [Legacy Migration]({{ site.baseurl }}{% link legacy-migration.md %}) for the full guide
covering both [CTL → TOML]({{ site.baseurl }}{% link legacy-ctl-to-toml.md %}) configuration
export and [MAD → TOML]({{ site.baseurl }}{% link legacy-convert-mad.md %}) language conversion.

---

## Post-Install

After `make install`, you have a working BBS skeleton in your PREFIX directory.
See [Directory Structure]({{ site.baseurl }}{% link directory-structure.md %}) for the full
layout.

### Key Utilities

| Utility | Purpose |
|---------|---------|
| `maxcfg` | TOML configuration editor, legacy CTL → TOML and MAD → TOML conversion |
| `mecca` | Compiles display files (.mec → .bbs) |
| `mex` | Compiles MEX scripts (.mex → .vm) |
| `max` | Main BBS executable |
| `squish` | FidoNet message tosser/scanner |

### Manual Recompilation

If you edit display or MEX source files, recompile them:

```bash
cd $PREFIX

# Recompile display files
bin/mecca display/help/*.mec
bin/mecca display/screens/*.mec

# Recompile MEX scripts (bash/zsh)
export MEX_INCLUDE=$PREFIX/scripts/include
for f in scripts/*.mex; do bin/mex "$f"; done
```

Language files (`config/lang/english.toml`) are loaded directly at startup —
edit them in place and restart the BBS. No compilation needed.

### Environment Setup

Set your PREFIX path before running commands:

**Bash/Zsh:**

```bash
export PREFIX=/path/to/maximus/build
export MEX_INCLUDE=$PREFIX/scripts/include
export PATH="$PREFIX/bin:$PATH"
```

**Fish:**

```fish
set -x PREFIX /path/to/maximus/build
set -x MEX_INCLUDE $PREFIX/scripts/include
set -x PATH $PREFIX/bin $PATH
```

### Running the BBS

**Recommended: Use MAXTEL for telnet access**

MAXTEL is the multi-node telnet supervisor with a real-time ncurses dashboard:

```bash
cd $PREFIX
bin/maxtel -p 2323 -n 4         # Start 4 nodes on port 2323
```

See [MaxTel (Telnet)]({{ site.baseurl }}{% link maxtel.md %}) for full documentation including
headless and daemon modes.

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
rm -rf build/config build/scripts build/data
scripts/copy_install_tree.sh build
```

### Configuration Workflow

1. Edit TOML files in `config/` to customize your BBS
2. Edit `config/lang/english.toml` to customize language strings (no compile)
3. Run `bin/recompile.sh` only if you changed display (`.mec`) or MEX (`.mex`) source files
4. Test with `bin/runbbs.sh -c` (local console mode)
5. Start telnet server with `bin/maxtel -p 2323 -n 4`

---

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

---

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

The `ARCH` variable adds `-arch x86_64` flags to compiler and linker. Use
`archclean` when switching architectures to remove old libraries.

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

For Linux builds, use the build script which handles dependency checking:

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

For cross-compiling Linux binaries from macOS, use Docker or a Linux VM (e.g.,
OrbStack on macOS).

---

## Debugging

```bash
# Build with debug symbols
./configure --build=DEBUG

# Run with tracing
strace -f ./bin/max -w -pt1  # Linux
ktrace ./bin/max -w -pt1     # BSD
dtruss ./bin/max -w -pt1     # macOS
```

---

## See Also

- [Directory Structure]({{ site.baseurl }}{% link directory-structure.md %}) — runtime directory
  layout after installation
- [MaxTel (Telnet)]({{ site.baseurl }}{% link maxtel.md %}) — running your BBS with the telnet
  supervisor
- [maxcfg CLI]({{ site.baseurl }}{% link maxcfg-cli.md %}) — command-line configuration and
  legacy CTL → TOML conversion
- [Updates & Changelog]({{ site.baseurl }}{% link updates.md %}) — release notes and version
  history
