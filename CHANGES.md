# Maximus-CBCS Change Log

---

## Sat Feb 7 2026 - Maximus/UNIX 3.04a-r2 [alpha]

**MaxUI Field/Input + Forms Improvements**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

### New: MaxUI Form Runner

- New struct-based form system with spatial (2D) navigation and field editing
- Cursor behavior aligned with other UI widgets:
  - Cursor hidden during form navigation
  - Cursor shown while actively editing a field
- Required-field validation with a simple splash message on save

### Improved: Field Editing + Key Handling

- Centralized UI key decoding via shared helper (`ui_read_key`) so ESC-sequences behave consistently across UI widgets
- Forward-delete support (Delete key / `ESC[3~`) in `ui_edit_field`:
  - Works for both plain fields and `format_mask` fields
- Documentation updated for these behaviors, including deferred “missing functionality” notes for more advanced ESC parsing

### MEX + Documentation

- All new/updated UI features are exposed to MEX (including `ui_form_run`)
- `uitest.mex` expanded with interactive coverage for prompt start modes and format-mask cancel/edit flows

## Sat Nov 30 2025 - Maximus/UNIX 3.04a-r2 [alpha]

**MAXTEL + Linux Support Release**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

### New: Linux x86_64 Support

- **Full Linux build support** alongside macOS (arm64/x86_64)
- GCC compatibility fixes for stricter type checking
- POSIX `spawnlp()` implementation for process spawning
- Tested on Debian/Ubuntu x86_64 (OrbStack, native)

**Linux-Specific Changes:**
- Added missing function declarations for GCC strictness
- Explicit pointer casts for `byte*`/`char*` type mismatches
- Added `<ctype.h>` includes where needed
- Fixed linker dependencies for shared libraries

**Platform Support:**
- macOS arm64 (Apple Silicon) - native
- macOS x86_64 (Intel/Rosetta) - native
- Linux x86_64 - native
- Linux arm64 - untested but should work

### New: MAXTEL Telnet Supervisor

- **Multi-node telnet supervisor** with real-time ncurses UI
- Manages 1-32 simultaneous BBS nodes
- Automatic node assignment for incoming callers
- Telnet protocol negotiation (terminal type, ANSI detection)

**UI Features:**
- Responsive layouts: Compact (80x25), Medium (100x30), Full (132x60+)
- Real-time node status, user info, and caller history panels
- Keyboard controls for node management (kick, restart)
- Dynamic terminal resize handling (SIGWINCH)

**Operation Modes:**
- Interactive mode with full ncurses interface
- Headless mode (`-H`) for scripts and process supervisors
- Daemon mode (`-D`) for background/startup operation

**Documentation:**
- Complete user manual: `docs/maxtel.txt` and `docs/maxtel.md`

### Build System Updates

- MAXTEL included in release packages
- Linux release packaging support in make-release.sh
- Release README updated with MAXTEL quick-start instructions
- Documentation (.txt and .md) included in release archives
- See `code-docs/LINUX_PORT_CHANGES.md` for detailed porting notes

---

## Thu Nov 27 2025 - Maximus/UNIX 3.04a [alpha]

**Modern macOS/Darwin Revival Release**  
Maintainer: Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja  
Current Status: Operational on macOS arm64, builds on x86_64

### Major Changes

- Full macOS (Darwin) support for arm64 and x86_64 architectures
- MEX VM fixed for 64-bit systems (struct alignment, pointer handling)
- Removed vestigial anti-tampering checks (VER_CHECKSUM, KEY system)
- Added MAX_INSTALL_PATH environment variable for portable installs
- Version now includes VER_SUFFIX for consistent release tagging
- Platform identification (/OSX, /LINUX) in version strings

### Build System

- Cross-architecture build support (arm64/x86_64 on Apple Silicon)
- Release packaging scripts (make-release.sh, make-all-releases.sh)
- Version extracted from max/max_vr.h for releases

### MEX Compiler/VM Fixes

- Fixed 64-bit pointer alignment in _usrfunc struct
- Added NULL terminator to intrinsic function table
- Fixed IADDR struct packing for proper string handling
- Documented parameter scoping bug in MEX compiler

### Tested Functionality

- Local console mode (max -c) login and operation
- MEX scripts: card.mex, dorinfo.mex, stats.mex, userlist.mex
- Tools: maid, silt, mecca, mex compiler
- User data structures readable and writable

### Known Issues

- Stability not fully validated across all features
- Some MEX scripts may need parameter renaming (scoping bug)

---

## Wed Jun 11 2003 - Maximus/UNIX 3.03b [pre-release]

- More changes to allow support for Sparc and other BIG_ENDIAN platforms
- Data files are still not binary compatible with other systems
- Massive work done on configure script and Makefile rules for cleaner builds
- Now detect platform endianness rather than reading from C header files
- Changed struct _stamp to union _stampu for proper DOS date/time handling
- Fixed lack-of-responsiveness in keyboard/video driver in max -c and max -k modes
- Integrated patches from Andrew Clarke for FreeBSD support
- Many other changes; see CVS for details

---

## Fri Jun 6 2003 - Maximus/UNIX 3.03b [pre-release]

- Incorporated 3.03a-patch1: 7-bit NVT (telnet capability) with multi-user support
- Incorporated FreeBSD & Linux build fixes from Andrew Clarke and Bo Simonsen
- Incorporated msgapi changes from Bo Simonsen (Husky Project origins)
- Squish bases now binary-compatible with other software
- Modified comdll API to COMMAPI_VER=2 (Nagle algorithm support)
- Added BINARY-TRANSFER NVT option for 8-bit clear sessions
- Major work to configuration engine
- Support for GNU Make 3.76 deprecated; 3.79+ recommended
- Added -fpermissive to g++ CXXFLAGS
- Now builds under Solaris 8/SPARC (data structs still wrong endian)
- Many other changes; see CVS for complete details

---

## Sat May 24 2003 - Maximus/UNIX 3.03a

**Initial Public Release**  
Bumped version number to 3.03 to avoid confusion.

### Major Problem Areas

- No Installer
- Local console output buggy (always one keystroke behind)
- fdcomm.so comm driver buggy, does not speak telnet
- fdcomm.so only allows one concurrent session
- MexVM crashes regularly on 64-bit arch

### Added Features

- `max -p2000` makes Maximus listen on TCP port 2000
- Added more options to master makefile
- Added simple ./configure script

---

## Tue May 13 2003 - Maximus/UNIX 3.02

**Initial Release**  
Released so Tony Summerfelt can play with Squish.  
Current Status: Squish "works for me"