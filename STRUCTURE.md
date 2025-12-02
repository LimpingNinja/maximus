# Maximus BBS Directory Structure

This document describes the directory layout for a Maximus BBS installation.
Cross-referenced against source code (`max/prm.h`, `m/prm.mh`) and `max.ctl`.

## Directory Tree

```
$PREFIX/                    # Base install directory (Path System)
├── bin/                    # Executables
│   ├── max                 # Main BBS executable
│   ├── maxtel              # Telnet supervisor
│   ├── mex                 # MEX compiler
│   ├── mecca               # MECCA compiler
│   ├── maid                # Language file compiler
│   ├── silt                # Control file compiler
│   ├── squish              # Message tosser/scanner
│   ├── sqafix              # Echomail processor
│   ├── install.sh          # Installation script
│   ├── recompile.sh        # Recompile all config files
│   └── runbbs.sh           # BBS startup script
│
├── lib/                    # Shared libraries
│   └── *.so / *.dylib
│
├── etc/                    # Configuration files
│   ├── max.ctl             # Main configuration (compiled → max.prm)
│   ├── max.prm             # Compiled configuration (binary)
│   ├── menus.ctl           # Menu definitions (compiled → *.mnu)
│   ├── msgarea.ctl         # Message area definitions
│   ├── filearea.ctl        # File area definitions
│   ├── areas.bbs           # Echomail areas
│   ├── compress.cfg        # Archiver definitions
│   ├── squish.cfg          # Squish tosser config
│   ├── sqafix.cfg          # SqaFix config
│   │
│   ├── help/               # Help display files (.mec → .bbs)
│   │   ├── *.mec           # MECCA source files
│   │   └── *.bbs           # Compiled display files
│   │
│   ├── lang/               # Language files (Path Language → PRM_LANGPATH)
│   │   ├── english.mad     # Language source
│   │   └── english.ltf     # Compiled language translation
│   │
│   ├── menus/              # Menu files (Menu Path → PRM_MENUPATH)
│   │   └── *.mnu           # Compiled menu files
│   │
│   ├── misc/               # Display files (Path Misc → PRM_MISCPATH)
│   │   ├── *.mec           # MECCA source files
│   │   ├── *.bbs           # Compiled display files
│   │   ├── logo.*          # Login logo (PRM_LOGO)
│   │   ├── welcome.*       # Welcome file (PRM_WELCOME)
│   │   ├── newuser1.*      # New user password prompt (PRM_NEWUSER1)
│   │   ├── newuser2.*      # New user welcome (PRM_NEWUSER2)
│   │   ├── applic.*        # New user questionnaire (PRM_APPLIC)
│   │   ├── byebye.*        # Logoff file (PRM_BYEBYE)
│   │   └── ...             # Other display files
│   │
│   ├── nodelist/           # FidoNet nodelists (Path NetInfo → PRM_NETPATH)
│   │   └── nodelist.*
│   │
│   ├── rip/                # RIP graphics files (PRM_RIPPATH)
│   │   └── *.rip
│   │
│   └── ansi/               # Source ANSI art files (for reference/editing)
│       └── *.ans           # Raw ANSI files (convert with ansi2bbs → etc/misc/)
│
├── m/                      # MEX scripts and headers
│   ├── *.mh                # MEX header files (MEX_INCLUDE points here)
│   │   ├── max.mh          # Main MEX header
│   │   ├── prm.mh          # PRM constants
│   │   ├── input.mh        # Input functions
│   │   └── ...
│   ├── *.mex               # MEX source files
│   └── *.vm                # Compiled MEX bytecode
│
├── ipc/                    # Inter-process communication (Path IPC → PRM_IPCPATH)
│   └── *.ipc               # IPC files for multi-node
│
├── log/                    # Log files (Log File → PRM_LOGNAME)
│   └── max.log
│
├── tmp/                    # Temporary files (Path Temp → PRM_TEMPPATH)
│
├── spool/                  # Mail and file spools
│   ├── attach/             # Local file attaches (PRM_ATTCHPATH)
│   ├── bad/                # Bad/corrupt messages
│   ├── dupes/              # Duplicate messages
│   ├── file/               # File areas
│   │   ├── max/            # Default file area
│   │   ├── newup/          # New uploads (unchecked)
│   │   ├── uncheck/        # Unchecked uploads
│   │   └── util/           # Utilities area
│   ├── inbound/            # FTS-1 inbound (PRM_INBOUND)
│   │   ├── incomplete/     # Partial transfers
│   │   └── unknown/        # Unknown files
│   ├── msgbase/            # Message bases (Squish format)
│   ├── netmail/            # FidoNet netmail
│   ├── olr/                # Offline reader packets (PRM_OLRPATH)
│   ├── outbound/           # FTS-1 outbound
│   ├── outbound.sq/        # Squish outbound
│   └── staged_xfer/        # CD-ROM staged transfers (PRM_STAGEPATH)
│
├── docs/                   # Documentation
│   └── *.md, *.txt
│
└── man/                    # Man pages
    └── *.1
```

## Path Keywords in max.ctl

| Keyword | PRM Constant | Default | Description |
|---------|--------------|---------|-------------|
| `Path System` | `PRM_SYSPATH` | `/var/max` | Base system path, CWD for Maximus |
| `Path Misc` | `PRM_MISCPATH` | `etc/misc` | Display files, F-key files |
| `Path Language` | `PRM_LANGPATH` | `etc/lang` | Language translation files |
| `Path Temp` | `PRM_TEMPPATH` | `tmp` | Temporary files |
| `Path IPC` | `PRM_IPCPATH` | `ipc` | Inter-process communication |
| `Path NetInfo` | `PRM_NETPATH` | `etc/nodelist` | FidoNet nodelists |
| `Menu Path` | `PRM_MENUPATH` | `etc/menus` | Compiled menu files |

## MEX Path Resolution

MEX scripts use `prm_string(PRM_*)` to get configured paths:

```mex
// Write to misc path (recommended)
fd := open(prm_string(PRM_MISCPATH) + "myfile.dat", IOPEN_RW | IOPEN_CREATE);

// Write to system path
fd := open(prm_string(PRM_SYSPATH) + "data/myfile.dat", IOPEN_RW | IOPEN_CREATE);
```

**Relative paths** in MEX resolve to `Path System` (the CWD when Maximus runs).

## Compilation Workflow

```
Source              Compiler    Output              Location
───────────────────────────────────────────────────────────────
*.ctl               silt        *.prm, *.mnu        etc/
*.mec               mecca       *.bbs               etc/misc/, etc/help/
*.mad               maid        *.ltf               etc/lang/
*.mex               mex         *.vm                m/
```

## Files Referenced by PRM Constants

| Constant | Field in prm.h | Configured via | Typical Value |
|----------|----------------|----------------|---------------|
| `PRM_USERBBS` | `user_file` | `User File` | `etc/user.bbs` |
| `PRM_LOGNAME` | `log_name` | `Log File` | `log/max.log` |
| `PRM_LOGO` | `logo` | `Uses Logo` | `etc/misc/logo` |
| `PRM_WELCOME` | `welcome` | `Uses Welcome` | `etc/misc/welcome` |
| `PRM_NEWUSER1` | `newuser1` | `Uses NewUser1` | `etc/misc/newuser1` |
| `PRM_NEWUSER2` | `newuser2` | `Uses NewUser2` | `etc/misc/newuser2` |
| `PRM_ROOKIE` | `rookie` | `Uses Rookie` | `etc/misc/rookie` |
| `PRM_APPLIC` | `application` | `Uses Application` | `etc/misc/applic` |
| `PRM_BYEBYE` | `byebye` | `Uses Goodbye` | `etc/misc/byebye` |
| `PRM_NOMAIL` | `nomail` | `Uses NoMail` | `etc/misc/nomail` |

## Environment Variables

| Variable | Purpose | Set by |
|----------|---------|--------|
| `MEX_INCLUDE` | MEX header search path | `recompile.sh`, should point to `$PREFIX/m` |
| `LD_LIBRARY_PATH` | Shared library path | `runbbs.sh`, includes `$PREFIX/lib` |
| `DYLD_LIBRARY_PATH` | macOS library path | `runbbs.sh`, includes `$PREFIX/lib` |

## Notes

1. **No `etc/m/` directory** - MEX files live only in `m/`. The `.vm` files are compiled and run from there.

2. **Path System is CWD** - `runbbs.sh` does `cd $PREFIX` before running `max`, so relative paths resolve there.

3. **Display files** - Can be `.bbs` (compiled MECCA) or plain text. Maximus tries both extensions.

4. **Menu references to MEX** - Use `m/scriptname` (without `.vm` extension). Example: `MEX m/oneliner`
