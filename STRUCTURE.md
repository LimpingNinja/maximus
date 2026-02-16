# Maximus BBS Directory Structure

This document describes the runtime directory layout for a Maximus BBS installation.
TOML is the sole configuration authority — no compiled `.prm`, `.mnu`, or `.ltf` binaries.

## Directory Tree

```
$PREFIX/                              # System root (e.g. /var/max or ~/max)
│
├── runbbs.sh                         # Primary BBS startup script (launches maxtel)
│
├── bin/                              # Executables + libraries
│   ├── max                           #   Main BBS executable
│   ├── maxtel                        #   Telnet supervisor
│   ├── maxcfg                        #   Configuration editor (TUI)
│   ├── mex                           #   MEX compiler
│   ├── mecca                         #   MECCA display file compiler
│   ├── squish                        #   Message tosser/scanner
│   ├── sqafix                        #   Echomail area manager
│   ├── init-userdb.sh                #   User database initialization
│   ├── recompile.sh                  #   Recompile display/MEX files
│   └── lib/                          #   Shared libraries
│       ├── libmax.so / .dylib
│       ├── libcompat.so / .dylib
│       └── ...
│
├── config/                           # All configuration (TOML is sole authority)
│   ├── maximus.toml                  #   Core system config (paths, system name, logging)
│   ├── matrix.toml                   #   FidoNet / echomail network config
│   ├── compress.cfg                  #   Archiver definitions
│   ├── squish.cfg                    #   Squish tosser config
│   ├── sqafix.cfg                    #   SqaFix config
│   ├── general/                      #   General BBS settings
│   │   ├── session.toml              #     Session / login behavior
│   │   ├── equipment.toml            #     Modem / port / comm settings
│   │   ├── display_files.toml        #     Display file path mappings
│   │   ├── colors.toml               #     Color scheme
│   │   ├── protocol.toml             #     Transfer protocol definitions
│   │   └── reader.toml               #     Offline reader settings
│   ├── lang/                         #   Language files (TOML only)
│   │   ├── english.toml
│   │   ├── delta_english.toml
│   │   └── colors.lh
│   ├── menus/                        #   Menu definitions (TOML only)
│   │   ├── main.toml
│   │   └── ...
│   ├── areas/                        #   Area definitions
│   │   ├── msg/areas.toml
│   │   └── file/areas.toml
│   ├── security/                     #   Access control
│   │   ├── access_levels.toml
│   │   ├── access.dat
│   │   └── baduser.bbs
│   └── legacy/                       #   Legacy config (read-only reference)
│       ├── max.ctl
│       ├── areas.bbs
│       ├── access.ctl
│       └── ...
│
├── display/                          # All user-facing display content
│   ├── screens/                      #   System screens (.bbs/.ans/.mec)
│   │   ├── logo.*                    #     Login logo
│   │   ├── welcome.*                 #     Welcome screen
│   │   ├── byebye.*                  #     Logoff screen
│   │   └── ...
│   ├── menus/                        #   Menu display art
│   │   ├── menu_main.*
│   │   └── ...
│   ├── help/                         #   Context-sensitive help screens
│   │   └── *.mec / *.bbs
│   ├── rip/                          #   RIP graphics files
│   │   └── *.rip
│   └── tunes/                        #   Tune/music files
│       └── tunes.bbs
│
├── scripts/                          # MEX scripts (replaces m/)
│   ├── include/                      #   MEX headers (.mh, .lh)
│   │   ├── max.mh
│   │   ├── prm.mh
│   │   └── ...
│   ├── *.mex                         #   MEX source files
│   └── *.vm                          #   Compiled MEX bytecode
│
├── data/                             # Persistent mutable runtime data
│   ├── users/                        #   User database
│   │   ├── user.db                   #     SQLite user database
│   │   └── callers.bbs               #     Caller log
│   ├── msgbase/                      #   Message bases (Squish format)
│   ├── filebase/                     #   File transfer area storage
│   ├── nodelist/                     #   FidoNet nodelists
│   ├── mail/                         #   Network mail transit
│   │   ├── inbound/
│   │   ├── outbound/
│   │   ├── netmail/
│   │   ├── attach/
│   │   ├── bad/
│   │   └── dupes/
│   ├── db/                           #   Database schemas
│   │   └── userdb_schema.sql
│   └── olr/                          #   Offline reader packets
│
├── run/                              # Ephemeral runtime state
│   ├── tmp/                          #   General temp files
│   ├── node/                         #   Per-node workspace + IPC
│   │   ├── 01/                       #     Node 1
│   │   │   ├── ipc.bbs
│   │   │   ├── lastus.bbs
│   │   │   ├── bbstat.bbs
│   │   │   ├── termcap.dat
│   │   │   ├── restar.bbs
│   │   │   └── maxipc* / maxipc.lck
│   │   └── ...
│   └── stage/                        #   Staged file transfers
│
├── log/                              # Log files
│   ├── max.log
│   └── echotoss.log
│
├── doors/                            # External door game programs
│
└── docs/                             # Documentation
```

## TOML Config Keys (`maximus.toml`)

All paths except `sys_path` are **relative** — resolved against `sys_path` by `ngcfg_get_path()`.

| Key | Default | Description |
|-----|---------|-------------|
| `sys_path` | `$PREFIX` | **Absolute** system root — anchor for all relative paths |
| `config_path` | `config` | TOML configuration files |
| `display_path` | `display/screens` | System display screens |
| `mex_path` | `scripts` | MEX scripts directory |
| `lang_path` | `config/lang` | TOML language files |
| `data_path` | `data` | Persistent mutable data root |
| `file_password` | `data/users/user` | User database |
| `file_callers` | `data/users/callers` | Caller log |
| `file_access` | `config/security/access` | Access level definitions |
| `message_data` | `data/msgbase/marea` | Message area index |
| `file_data` | `data/msgbase/farea` | File area index |
| `net_info_path` | `data/nodelist` | FidoNet nodelists |
| `outbound_path` | `data/mail/outbound` | FTS-1 outbound |
| `inbound_path` | `data/mail/inbound` | FTS-1 inbound |
| `run_path` | `run` | Ephemeral runtime state root |
| `node_path` | `run/node` | Per-node directories |
| `temp_path` | `run/tmp` | Temp files |
| `stage_path` | `run/stage` | Staged file transfers |
| `doors_path` | `doors` | External door programs |
| `log_file` | `log/max.log` | System log |

## Compilation Workflow

```
Source              Compiler    Output              Location
────────────────────────────────────────────────────────────────
*.mec               mecca       *.bbs               display/screens/, display/help/
*.mex               mex         *.vm                scripts/
```

Language files are **TOML only** — no compilation step.

## Environment Variables

| Variable | Value | Set by |
|----------|-------|--------|
| `MEX_INCLUDE` | `$PREFIX/scripts/include` | `runbbs.sh` |
| `LD_LIBRARY_PATH` | `$PREFIX/bin/lib` | `runbbs.sh` |
| `DYLD_LIBRARY_PATH` | `$PREFIX/bin/lib` | `runbbs.sh` |
| `MAX_INSTALL_PATH` | `$PREFIX` | `runbbs.sh` |

## Notes

1. **`sys_path` is CWD** — `runbbs.sh` does `cd $PREFIX` before running maxtel, so relative paths resolve correctly.

2. **Display files** — Can be `.bbs` (compiled MECCA) or plain text. Maximus tries both extensions.

3. **Menu references to MEX** — Use `scripts/scriptname` (without `.vm` extension). Example: `MEX scripts/oneliner`

4. **Per-node isolation** — Each node gets its own directory under `run/node/XX/` containing IPC, lastuser, bbstat, termcap, and restart files.
