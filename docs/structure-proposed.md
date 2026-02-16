# Maximus BBS — Proposed Directory Restructure

> **Status:** Draft proposal  
> **Author:** Kevin Morgan / Willow  
> **Date:** 2025-02-14  

---

## 1. Motivation

The current Maximus runtime layout is inherited from the DOS/OS2 era. Key pain points:

- **`etc/` is a junk drawer** — config files, display art, help screens, language files,
  user databases, access control, nodelists, tunes, and legacy `.ctl` files all live here.
- **`m/` is cryptic** — MEX source, headers, and compiled bytecode share a flat directory.
- **Per-node files (`lastusXX.bbs`, `bbstatXX.bbs`, `termcapXX.dat`) dump into `$PREFIX/` root**.
- **Area data (`.dat`/`.idx`), tracking files, and door drops also clutter `$PREFIX/` root**.
- **`man/` is unused** — no man pages are currently generated or distributed.
- **`runbbs.sh` lives in `bin/`** — should be at root level as the primary entry point.
- **`lib/` is a peer of `bin/`** — cleaner as a subdirectory of `bin/`.

The recent move to **TOML as the sole configuration format** (no compiled `.mnu`, no `.prm`,
no `.ltf` binaries) makes this the ideal time to restructure.

---

## 2. Proposed Directory Tree

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
│   │
│   ├── general/                      #   General BBS settings
│   │   ├── session.toml              #     Session / login behavior
│   │   ├── equipment.toml            #     Modem / port / comm settings
│   │   ├── display_files.toml        #     Display file path mappings
│   │   ├── colors.toml               #     Color scheme
│   │   ├── protocol.toml             #     Transfer protocol definitions
│   │   └── reader.toml               #     Offline reader settings
│   │
│   ├── lang/                         #   Language files (TOML only, no .ltf)
│   │   ├── english.toml              #     Primary language strings
│   │   ├── delta_english.toml        #     Delta overrides for customization
│   │   └── colors.lh                 #     Color name definitions (legacy header)
│   │
│   ├── menus/                        #   Menu definitions (TOML only, no .mnu)
│   │   ├── main.toml
│   │   ├── message.toml
│   │   ├── file.toml
│   │   ├── change.toml
│   │   ├── chat.toml
│   │   ├── edit.toml
│   │   ├── sysop.toml
│   │   ├── reader.toml
│   │   ├── msgread.toml
│   │   └── mex.toml
│   │
│   ├── areas/                        #   Area definitions
│   │   ├── msg/
│   │   │   └── areas.toml            #     Message area config
│   │   └── file/
│   │       └── areas.toml            #     File area config
│   │
│   ├── security/                     #   Access control
│   │   └── access_levels.toml        #     Privilege level definitions
│   │
│   └── legacy/                       #   Legacy config (read-only reference)
│       ├── max.ctl                   #     Original system control file
│       ├── areas.bbs                 #     Original area definitions
│       ├── access.ctl                #     Original access levels
│       ├── compress.cfg              #     Archiver definitions
│       ├── squish.cfg                #     Squish tosser config
│       ├── sqafix.cfg                #     SqaFix config
│       └── ...                       #     Other legacy .ctl files
│
├── display/                          # All user-facing display content
│   ├── screens/                      #   System screens (.bbs/.ans/.txt)
│   │   ├── logo.*                    #     Login logo (pre-ANSI detect)
│   │   ├── welcome.*                 #     Welcome screen (returning users)
│   │   ├── newuser1.*                #     New user — password prompt
│   │   ├── newuser2.*                #     New user — welcome
│   │   ├── notfound.*                #     User not found prompt
│   │   ├── applic.*                  #     New user questionnaire
│   │   ├── byebye.*                  #     Logoff screen
│   │   ├── badlogon.*                #     Failed login warning
│   │   ├── barricad.*                #     Barricade prompt
│   │   ├── bulletin.*                #     Bulletins
│   │   ├── yellreq.*                 #     Yell/page request
│   │   ├── daylimit.*                #     Daily time limit notice
│   │   ├── timewarn.*                #     Time warning
│   │   ├── tooslow.*                 #     Baud too slow
│   │   ├── nospace.*                 #     No disk space
│   │   ├── nomail.*                  #     No mail available
│   │   ├── shellbye.*                #     Shell-to-DOS goodbye
│   │   ├── shellhi.*                 #     Return from shell
│   │   ├── quotes.*                  #     Random quotes file
│   │   ├── hdrentry.*                #     Message header entry help
│   │   ├── msgentry.*                #     Message entry help
│   │   ├── xferbaud.*                #     Transfer baud warning
│   │   ├── fformat.*                 #     File listing format
│   │   ├── excbytes.*                #     Download byte limit exceeded
│   │   ├── excratio.*                #     Upload/download ratio exceeded
│   │   ├── exctime.*                 #     Time limit exceeded
│   │   ├── bad_upld.*                #     Bad upload warning
│   │   ├── file_ok.*                 #     File virus check passed
│   │   ├── file_bad.*                #     File virus check failed
│   │   ├── tag_file.*                #     Tag file help
│   │   ├── CHG_SENT.*                #     Message already sent warning
│   │   ├── CHG_NO.*                  #     Cannot change message
│   │   ├── WHY_PVT.*                 #     Why private message
│   │   ├── uedhelp.*                 #     Full-screen editor help
│   │   ├── userlist.*                #     User list display
│   │   ├── attrib.*                  #     Message attributes help
│   │   └── ...
│   │
│   ├── menus/                        #   Menu display art (referenced by menu_file)
│   │   ├── menu_main.*               #     Main menu art (.bbs/.ans)
│   │   ├── menu_files.*              #     File menu art
│   │   ├── menu_msgs.*               #     Message menu art
│   │   └── ...
│   │
│   ├── help/                         #   Context-sensitive help screens
│   │   ├── main.*                    #     Main help
│   │   ├── contents.*                #     Help table of contents
│   │   ├── fsed.*                    #     Full-screen editor help
│   │   ├── 1stedit.*                 #     Line editor help
│   │   ├── rep_edit.*                #     Replace editor help
│   │   ├── locate.*                  #     Locate command help
│   │   └── ...
│   │
│   └── tunes/                        #   Tune/music files
│       └── tunes.bbs
│
├── scripts/                          # MEX scripts (replaces m/)
│   ├── include/                      #   MEX headers (.mh, .lh)
│   │   ├── max.mh
│   │   ├── prm.mh
│   │   ├── language.mh
│   │   └── ...
│   ├── *.mex                         #   MEX source files
│   └── *.vm                          #   Compiled MEX bytecode
│
├── data/                             # Persistent mutable runtime data
│   ├── users/                        #   User database
│   │   ├── user.db                   #     SQLite user database
│   │   └── callers.bbs               #     Caller log
│   │
│   ├── msgbase/                      #   Message bases (Squish format)
│   │   ├── *.sqd / *.sqi            #     Per-area Squish data/index
│   │   └── ...
│   │
│   ├── filebase/                     #   File transfer area storage
│   │   ├── uploads/                  #     Unchecked uploads staging
│   │   └── <area>/                   #     Per-area file storage
│   │
│   ├── nodelist/                     #   FidoNet nodelists
│   │   └── nodelist.*
│   │
│   ├── mail/                         #   Network mail transit
│   │   ├── inbound/                  #     FTS-1 inbound packets
│   │   │   ├── incomplete/
│   │   │   └── unknown/
│   │   ├── outbound/                 #     FTS-1 outbound
│   │   ├── netmail/                  #     FidoNet netmail base
│   │   ├── attach/                   #     File attaches
│   │   ├── olr/                      #     Offline reader packets
│   │   ├── bad/                      #     Bad/corrupt packets
│   │   └── dupes/                    #     Duplicate messages
│   │
│   ├── tracking/                     #   Message tracking data
│   │   ├── trkarea.*
│   │   ├── trkmsg.*
│   │   └── trkown.*
│   │
│   ├── db/                           #   Database schemas and support
│   │   └── userdb_schema.sql
│   │
│   ├── colours.dat                   #   Color table (binary)
│   ├── oneliners.dat                 #   Oneliner database
│   └── shadow_save_*.json            #   User shadow saves
│
├── run/                              # Ephemeral runtime state
│   ├── tmp/                          #   General temp files (uploads, scratch)
│   │   └── msgtmpXX.$$$             #     Per-node message temp files
│   │
│   ├── node/                         #   Per-node workspace + IPC
│   │   ├── 01/                       #     Node 1
│   │   │   ├── ipc.bbs              #       IPC communication file
│   │   │   ├── lastus.bbs           #       Last user on this node
│   │   │   ├── bbstat.bbs           #       BBS statistics for this node
│   │   │   ├── termcap.dat          #       Terminal capabilities
│   │   │   ├── restar.bbs           #       Restart state
│   │   │   └── maxipc*              #       Unix domain socket + lock
│   │   ├── 02/
│   │   └── ...
│   │
│   └── stage/                        #   Staged file transfers
│
├── log/                              # Log files
│   ├── max.log                       #   Main system log
│   ├── maxtel.log                    #   Telnet supervisor log
│   ├── uploads.log                   #   Upload activity log
│   ├── echotoss.log                  #   Echomail toss trigger
│   └── debug.log                     #   Debug trace log
│
├── doors/                            # External door game programs
│   └── <doorname>/                   #   Per-door working directory
│       ├── Door.Sys                  #     Door drop files
│       └── dorinfo1.def
│
└── docs/                             # Documentation
    ├── wiki/                         #   Wiki-style documentation
    │   └── *.md
    ├── *.md                          #   General docs (BUILD.md, etc.)
    └── *.txt                         #   Legacy text documentation
```

---

## 3. Key Design Decisions

| Decision | Rationale |
|---|---|
| **`runbbs.sh` at root** | Primary entry point; should launch `maxtel` instead of raw `max` |
| **`lib/` under `bin/`** | Single location for executables + their deps; cleaner `$PREFIX/` root |
| **No `man/`** | No man pages exist; docs go in `docs/` and `docs/wiki/` |
| **`config/lang/`** not top-level | Language TOML files are config; sysop edits them like other config |
| **`data/mail/`** not top-level | Mail transit is persistent mutable state, same domain as msgbase |
| **`display/` not under `config/`** | Art/display files are content, not configuration |
| **`scripts/` replaces `m/`** | Self-documenting name for MEX scripts |
| **`scripts/include/`** | Separates headers from source; `MEX_INCLUDE` points here |
| **`run/node/XX/`** | Per-node isolation for IPC, lastuser, bbstat, termcap, restart |
| **`config/legacy/`** | Legacy `.ctl` files preserved for reference/upgrade path only |
| **No compiled menus** | TOML menus in `config/menus/` are authoritative; display art in `display/menus/` |
| **No `.ltf` language files** | TOML language files in `config/lang/` are authoritative |
| **`data/db/`** | Schema files for user database initialization |

---

## 4. TOML Config Key Schema (`maximus.toml`)

The current `maximus.toml` defines path keys that map directly to the legacy `etc/`
layout. With the restructure, some keys stay, some get renamed, some get removed,
and several **new keys** are needed to properly expose the new directory hierarchy.

### 4.1 Keys to KEEP (with updated defaults)

These keys remain with the same name. Only their default values change.

| Key | Current Default | Proposed Default | Code Consumers | Notes |
|---|---|---|---|---|
| `sys_path` | `$PREFIX` | `$PREFIX` | everywhere | Base path anchor — unchanged |
| `config_path` | `config` | `config` | `maxcfg`, `maxtel`, `libmaxcfg` | Unchanged |
| `lang_path` | `etc/lang` | `config/lang` | `language.c`, `mexlang.c`, `colorform.c`, `lang_browse.c`, `menubar.c` | Now under `config/` |
| `temp_path` | `tmp` | `run/tmp` | `dropfile.c`, `max_ocmd.c`, `max_bor.c`, `max_init.c`, `f_misc.c`, `m_upload.c`, `frecv.c` | Heavily used for per-node temp, uploads, msg scratch |
| `net_info_path` | `etc/nodelist` | `data/nodelist` | `node.c` (4 callsites) | Nodelist lookups |
| `stage_path` | `spool/staged_xfer` | `run/stage` | `f_down.c` (2 callsites) | File transfer staging |
| `log_file` | `log/max.log` | `log/max.log` | logging subsystem | Unchanged |
| `file_callers` | `etc/callers` | `data/users/callers` | `callinfo.c` (2 callsites), `maxtel.c` | Caller log |
| `file_password` | `etc/user` | `data/users/user` | `mexuser.c`, `mci_preview.c`, `init_userdb.c` | User DB path |
| `file_access` | `etc/access` | `config/security/access` | `menubar.c`, `ctl_to_ng.c`, `nextgen_export.c` | Access level definitions |
| `message_data` | `marea` | `data/msgbase/marea` | `max_init.c` | Message area index |
| `file_data` | `farea` | `data/msgbase/farea` | `max_init.c` | File area index |
| `outbound_path` | `""` | `data/mail/outbound` | echomail/netmail subsystem | FTS-1 outbound |
| `inbound_path` | `""` | `data/mail/inbound` | echomail/netmail subsystem | FTS-1 inbound |

### 4.2 Keys to RENAME

These keys change name to better reflect their role in the new layout.

| Current Key | Proposed Key | Current Default | Proposed Default | Rationale |
|---|---|---|---|---|
| `misc_path` | `display_path` | `etc/misc` | `display/screens` | `misc` is meaningless — this is the display/screens base path. **All** ~30+ callsites in the runtime (`max_main.c`, `max_cho.c`, `max_fini.c`, `max_log.c`, `max_locl.c`, `f_up.c`, `f_down.c`, `f_logup.c`, `f_area.c`, `f_tag.c`, `m_area.c`, `m_browse.c`, `m_change.c`, `ued_cmds.c`, `mh_tty.c`, `mh_graph.c`, `disp_max.c`, `max_rip.c`) read `maximus.misc_path` — the rename means updating the config key string in every `ngcfg_get_*()` call. |
| `ipc_path` | `node_path` | `ipc` | `run/node` | IPC files are just one of several per-node files. The directory now holds IPC sockets, lastuser, bbstat, termcap, and restart files. `node_path` better describes the per-node workspace concept. |

### 4.3 Keys to REMOVE

| Key | Current Default | Reason for Removal | Migration |
|---|---|---|---|
| `menu_path` | `etc/menus` | No compiled menus exist. Menu **definitions** are TOML files in `config/menus/`. Menu **display art** is referenced via `menu_file` in each menu TOML and resolved against `display_path`. | Code in `max_init.c:1131` reads this into the global `menupath` — remove that code path. |
| `rip_path` | `etc/rip` | RIP graphics are display content. They should resolve against `display_path` (or a `display/rip/` subdirectory). Only 2 callsites: `max_init.c:1138` and `max_rip.c:158`. | Replace with `display_path`-relative resolution, e.g. `<display_path>/../rip/<file>` or add a dedicated `rip_path` under display if needed. |
| `protocol_ctl` | `""` | Legacy CTL reference. Protocol config is in TOML. | Remove; `max_prot.c` already falls back to `protocol.max` in sys_path. |
| `mcp_pipe` | `\pipe\maximus\mcp` | Windows named pipe path — dead code on Unix. | Remove or gate behind platform ifdef. |

### 4.4 Keys to ADD

These are **new keys** needed to properly expose the restructured directories.

| New Key | Default Value | Purpose | Code Impact |
|---|---|---|---|
| `display_path` | `display` | **Base path for all display content.** Replaces `misc_path`. Resolves to `$PREFIX/display/`. Sub-paths (`screens/`, `menus/`, `help/`, `tunes/`) are derived from this base. All existing `ngcfg_get_*("maximus.misc_path")` calls become `ngcfg_get_*("maximus.display_path")`. | ~30 callsites across runtime + maxcfg. See §4.2 rename. |
| `screens_path` | `display/screens` | **Direct path to system display screens.** This is what most of the ~30 callsites actually need (the old `misc_path` behavior). If `display_path` alone is too coarse, this gives explicit control. | Optional — can be derived as `<display_path>/screens` if we prefer fewer keys. |
| `help_path` | `display/help` | **Path to help display files.** Currently help files are under `etc/help/` and referenced by display file config. Gives sysops explicit control over help file location. | `display_files.toml` entries that point to help files. |
| `mex_path` | `scripts` | **Path to MEX scripts directory.** Replaces the hardcoded `m/` in maxtel (`maxtel.c:545`) and the `MEX_INCLUDE` env var construction. Also used by the file picker in `form.c:1224`. | `maxtel.c` (spawn env setup), `form.c` (file picker), `Makefile` (reconfig target), env var construction. |
| `node_path` | `run/node` | **Base path for per-node runtime directories.** Each node gets `<node_path>/XX/` containing IPC, lastuser, bbstat, termcap, restart files. Replaces `ipc_path` semantics. | See §4.2 rename of `ipc_path`. All per-node file construction in `max_v.h`, `max_fman.c`, `max_rest.c`, `max_init.c`, `max_xtrn.c`, `max_log.c`, `max_wfc.c`, `max_chat.c`, `maxtel.c`. |
| `data_path` | `data` | **Base path for persistent mutable data.** Anchor for `users/`, `msgbase/`, `filebase/`, `mail/`, `nodelist/`, `tracking/`. Not strictly required if sub-paths are individually configurable, but provides a clean default root. | New — used as default prefix for `file_password`, `file_callers`, `message_data`, `file_data`, `net_info_path`. |
| `doors_path` | `doors` | **Base path for external door programs.** Door drop files (`Door.Sys`, `dorinfo1.def`) and per-door working directories. Currently these land in `$PREFIX/` root or temp. | `max_xtrn.c`, `dropfile.c` — door execution paths. |
| `run_path` | `run` | **Base path for ephemeral runtime state.** Parent of `tmp/`, `node/`, `stage/`. Allows sysop to point all runtime state to a tmpfs or separate partition. | New — `temp_path`, `node_path`, `stage_path` can default to subdirs of this. |

### 4.5 Proposed `maximus.toml` — Complete Key Layout

This is what the restructured `maximus.toml` would look like with all changes applied:

```toml
config_version = 2
system_name = "Battle Station BBS"
sysop = "Limping Ninja"
task_num = 1
video = "IBM"
has_snow = false
multitasker = "UNIX"

# === Core Paths ===
sys_path = "/var/max"                    # System root (anchor for all relative paths)
config_path = "config"                   # TOML configuration files

# === Display ===
display_path = "display"                 # Base for all display content
# screens_path = "display/screens"       # (optional — derived from display_path)
# help_path = "display/help"             # (optional — derived from display_path)

# === Scripts ===
mex_path = "scripts"                     # MEX scripts (.mex source + .vm bytecode)

# === Language ===
lang_path = "config/lang"                # TOML language files

# === Data ===
data_path = "data"                       # Persistent mutable data root
file_password = "data/users/user"        # User database
file_callers = "data/users/callers"      # Caller log
file_access = "config/security/access"   # Access level definitions
message_data = "data/msgbase/marea"      # Message area index
file_data = "data/msgbase/farea"         # File area index
net_info_path = "data/nodelist"          # FidoNet nodelists
outbound_path = "data/mail/outbound"     # FTS-1 outbound
inbound_path = "data/mail/inbound"       # FTS-1 inbound

# === Runtime ===
run_path = "run"                         # Ephemeral runtime state root
node_path = "run/node"                   # Per-node directories (IPC, lastuser, bbstat, etc.)
temp_path = "run/tmp"                    # Temp files (uploads, msg scratch)
stage_path = "run/stage"                 # Staged file transfers
doors_path = "doors"                     # External door programs

# === Logging ===
log_file = "log/max.log"
log_mode = "Trace"

# === System Settings ===
msg_reader_menu = "MSGREAD"
mcp_sessions = 16
snoop = true
no_password_encryption = false
no_share = false
reboot = false
swap = false
dos_close = true
local_input_timeout = false
status_line = false
```

### 4.6 Key Removal Impact Summary

| Removed Key | Callsites to Update |
|---|---|
| `menu_path` | `max_init.c:1131` — remove `menupath` global init from this key |
| `rip_path` | `max_init.c:1138` — derive from `display_path`; `max_rip.c:158` — same |
| `protocol_ctl` | `max_prot.c:55` — already has fallback; remove key read |
| `mcp_pipe` | Windows-only dead code — remove or ifdef |

### 4.7 Key Rename Impact Summary

| Rename | Callsites (grep `ngcfg_get.*"maximus.<old_key>"`) |
|---|---|
| `misc_path` → `display_path` | **~30+ sites** across `max_main.c`, `max_cho.c`, `max_fini.c`, `max_log.c`, `max_locl.c`, `f_up.c`, `f_down.c`, `f_logup.c`, `f_area.c`, `f_tag.c`, `m_area.c`, `m_browse.c`, `m_change.c`, `ued_cmds.c`, `mh_tty.c`, `mh_graph.c`, `disp_max.c`, `max_rip.c`, `menubar.c`, `fields.c`, `form.c` |
| `ipc_path` → `node_path` | `max_cho.c` (6 sites), `max_chat.c` (4 sites), `max_args.c` (1 site) |

---

## 5. Path Mapping — Current → Proposed

### 5.1 TOML Config Keys (`maximus.toml`) — Value Changes

| Key (proposed name) | Current Value | Proposed Value | Notes |
|---|---|---|---|
| `sys_path` | `$PREFIX` | `$PREFIX` | Unchanged |
| `config_path` | `config` | `config` | Unchanged |
| `display_path` *(was misc_path)* | `etc/misc` | `display/screens` | **Rename + move** |
| `lang_path` | `etc/lang` | `config/lang` | **Move** |
| `temp_path` | `tmp` | `run/tmp` | **Move** |
| `node_path` *(was ipc_path)* | `ipc` | `run/node` | **Rename + restructure** |
| `mex_path` *(new)* | *(none)* | `scripts` | **Add** |
| `data_path` *(new)* | *(none)* | `data` | **Add** |
| `run_path` *(new)* | *(none)* | `run` | **Add** |
| `doors_path` *(new)* | *(none)* | `doors` | **Add** |
| `net_info_path` | `etc/nodelist` | `data/nodelist` | **Move** |
| `stage_path` | `spool/staged_xfer` | `run/stage` | **Move** |
| `log_file` | `log/max.log` | `log/max.log` | Unchanged |
| `file_callers` | `etc/callers` | `data/users/callers` | **Move** |
| `file_password` | `etc/user` | `data/users/user` | **Move** |
| `file_access` | `etc/access` | `config/security/access` | **Move** |
| `message_data` | `marea` | `data/msgbase/marea` | **Move** |
| `file_data` | `farea` | `data/msgbase/farea` | **Move** |
| `outbound_path` | `""` | `data/mail/outbound` | **Move** |
| `inbound_path` | `""` | `data/mail/inbound` | **Move** |

### 5.2 Display Files Config (`general/display_files.toml`)

All display file paths change from `etc/misc/*` and `etc/help/*` prefixes:

| Current | Proposed |
|---|---|
| `etc/misc/logo` | `display/screens/logo` |
| `etc/misc/welcome` | `display/screens/welcome` |
| `etc/misc/newuser1` | `display/screens/newuser1` |
| `etc/misc/newuser2` | `display/screens/newuser2` |
| `etc/misc/notfound` | `display/screens/notfound` |
| `etc/misc/applic` | `display/screens/applic` |
| `etc/misc/byebye` | `display/screens/byebye` |
| `etc/misc/badlogon` | `display/screens/badlogon` |
| `etc/misc/barricad` | `display/screens/barricad` |
| `etc/misc/quotes` | `display/screens/quotes` |
| `etc/misc/daylimit` | `display/screens/daylimit` |
| `etc/misc/timewarn` | `display/screens/timewarn` |
| `etc/misc/tooslow` | `display/screens/tooslow` |
| `etc/misc/nospace` | `display/screens/nospace` |
| `etc/misc/nomail` | `display/screens/nomail` |
| `etc/misc/shellbye` | `display/screens/shellbye` |
| `etc/misc/shellhi` | `display/screens/shellhi` |
| `etc/misc/hdrentry` | `display/screens/hdrentry` |
| `etc/misc/msgentry` | `display/screens/msgentry` |
| `etc/misc/xferbaud` | `display/screens/xferbaud` |
| `etc/misc/fformat` | `display/screens/fformat` |
| `etc/help/locate` | `display/help/locate` |
| `etc/help/contents` | `display/help/contents` |
| `etc/help/fsed` | `display/help/fsed` |
| `etc/help/1stedit` | `display/help/1stedit` |
| `etc/help/rep_edit` | `display/help/rep_edit` |
| `etc/tunes` | `display/tunes/tunes` |

### 5.3 Menu TOML `menu_file` References

| Current | Proposed |
|---|---|
| `etc/display/menu_main` | `display/menus/menu_main` |
| `etc/display/menu_files` | `display/menus/menu_files` |
| *(other menu art)* | `display/menus/*` |

### 5.4 MEX Script References (in menu `arguments`)

| Current | Proposed |
|---|---|
| `m/stats` | `scripts/stats` |
| `m/oneliner` | `scripts/oneliner` |
| `m/callers` | `scripts/callers` |
| *(all `m/*`)* | `scripts/*` |

### 5.5 Per-Node Files

| Current (in `$PREFIX/` root) | Proposed |
|---|---|
| `lastusXX.bbs` | `run/node/XX/lastus.bbs` |
| `bbstatXX.bbs` | `run/node/XX/bbstat.bbs` |
| `termcapXX.dat` | `run/node/XX/termcap.dat` |
| `restarXX.bbs` | `run/node/XX/restar.bbs` |
| `msgtmpXX.$$$` | `run/tmp/msgtmpXX.$$$` |
| `maxipcX` / `maxipcX.lck` | `run/node/XX/maxipc` / `maxipc.lck` |

### 5.6 Loose Root Files → `data/`

| Current (in `$PREFIX/` root) | Proposed |
|---|---|
| `marea.dat` / `marea.idx` | `data/msgbase/marea.dat` / `.idx` |
| `farea.dat` / `farea.idx` | `data/msgbase/farea.dat` / `.idx` |
| `colours.dat` | `data/colours.dat` |
| `oneliners.dat` | `data/oneliners.dat` |
| `shadow_save_*.json` | `data/shadow_save_*.json` |
| `trkarea.*` / `trkmsg.*` / `trkown.*` | `data/tracking/*` |
| `mtag.*` | `data/mtag.*` |
| `Door.Sys` / `dorinfo1.def` | `doors/<door>/` |

---

## 6. Required Code Changes

### 6.1 Language Loading — `src/max/core/language.c`

**Current hardcoded path:** `etc/lang/%s.toml`

```c
// language.c:143 — open_toml_lang()
snprintf(rel, sizeof(rel), "etc/lang/%s.toml", lang_name);

// language.c:257 — Get_Language()
snprintf(rel, sizeof(rel), "etc/lang/%s.toml", lname);
```

**Change:** Read `lang_path` from TOML config (`maximus.lang_path`) and use it instead
of hardcoded `etc/lang`. If `lang_path` is set in `maximus.toml`, use
`<lang_path>/<name>.toml`. Fallback to `config/lang/<name>.toml`.

### 6.2 MEX Language Extension — `src/max/mex_runtime/mexlang.c`

**Current hardcoded path:** `etc/lang`

```c
// mexlang.c:236
snprintf(full_path, sizeof(full_path), "%s/etc/lang/%s",
         ngcfg_get_string_raw("maximus.system_path"), path);
```

**Change:** Use `ngcfg_get_path("maximus.lang_path")` instead of hardcoding `etc/lang`.

### 6.3 Per-Node File Patterns — `src/max/core/max_v.h`

**Current format strings:**

```c
// max_v.h:214-215
extrn char lastuser_bbs[] IS("%slastuser.bbs");
extrn char lastusxx_bbs[] IS("%slastus%02x.bbs");

// max_v.h:217
extrn char restarxx_bbs[] IS("%srestar%02x.bbs");

// max_v.h:237
extrn char msgtemp_name[] IS("%smsgtmp%02x.$$$");

// max_v.h:239
extrn char bbs_stats[] IS("%sbbstat%02x.bbs");
```

**Change:** These format strings currently use `original_path` (which is `$PREFIX/`).
They need to be updated to write into `run/node/XX/` with the node number as a
subdirectory, not a filename suffix. The format strings and every callsite
(`max_fman.c`, `max_rest.c`, `max_init.c`, `max_xtrn.c`, `max_log.c`, `max_wfc.c`)
must be updated.

**New approach:** Create a helper function `node_path(task_num, filename, buf, bufsize)`
that produces `<prefix>/run/node/<NN>/<filename>`. All callsites use this instead
of `sprintf(temp, lastusxx_bbs, original_path, task_num)`.

**Affected files:**
- `src/max/core/max_v.h` — format strings
- `src/max/core/max_fman.c` — `Write_LastUser()`
- `src/max/core/max_rest.c` — restart file reading
- `src/max/core/max_init.c` — lastuser/restart reading at startup
- `src/max/core/max_xtrn.c` — door execution, lastuser write
- `src/max/core/max_log.c` — `Write_LastUser()`, multi-node check
- `src/max/core/max_wfc.c` — `Update_Lastuser()`, bbstat reading
- `src/max/core/max_chat.c` — `ChatOpenIPC()`, IPC path construction

### 6.4 IPC Path Construction — `src/max/core/max_chat.c`

**Current:** Uses `ngcfg_get_path("maximus.ipc_path")` and constructs
`<ipc_path>/ipcXX.bbs`.

**Change:** IPC files move into `run/node/XX/`. The `ipc_path` config key semantics
change to point to `run/node`. IPC file creation uses node subdirectories.
`ChatOpenIPC()` and all IPC callers in `max_cho.c` must be updated.

### 6.5 Maxtel Node Spawn — `src/apps/maxtel/maxtel.c`

**Current hardcoded paths:**

```c
// maxtel.c:154
static char max_path[512] = "./bin/max";

// maxtel.c:544
snprintf(lib_path, sizeof(lib_path), "%s/lib", full_base);

// maxtel.c:545
snprintf(mex_path, sizeof(mex_path), "%s/m", full_base);

// maxtel.c:553
setenv("MEX_INCLUDE", mex_path, 1);

// maxtel.c:504-507 — socket/lock paths in base_path root
snprintf(node->socket_path, ..., "%s/%s%d", base_path, SOCKET_PREFIX, node_num + 1);

// maxtel.c:1292 — termcap in base_path root
snprintf(path, ..., "%s/termcap%02d.dat", base_path, node_num + 1);

// maxtel.c:1584 — lastuser in base_path root
snprintf(lastus_path, ..., "%s/%s%02d.bbs", base_path, LASTUS_PREFIX, i + 1);

// maxtel.c:1643-1646 — bbstat in base_path root
snprintf(path, ..., "%s/%s00.bbs", base_path, STATUS_PREFIX);

// maxtel.c:1668 — lastuser for node status
snprintf(path, ..., "%s/%s%02d.bbs", base_path, LASTUS_PREFIX, node_num + 1);

// maxtel.c:2427 — maxcfg launch
snprintf(maxcfg_path, ..., "%s/bin/maxcfg", base_path);
```

**Changes needed:**
- `lib_path` → `%s/bin/lib` (lib is now under bin)
- `mex_path` → `%s/scripts` (m/ renamed to scripts/)
- `MEX_INCLUDE` → `%s/scripts/include`
- Socket/lock paths → `%s/run/node/%d/maxipc*`
- Termcap → `%s/run/node/%02d/termcap.dat`
- Lastuser → `%s/run/node/%02d/lastus.bbs`
- Bbstat → `%s/run/node/%02d/bbstat.bbs`

### 6.6 Display File Defaults — `src/apps/maxcfg/src/config/fields.c`

**Current defaults hardcode `etc/misc` and `etc/help`:**

```c
// fields.c:122 — Path Miscellaneous default
.default_value = "/var/max/etc/misc"

// fields.c:132 — Path Language default
.default_value = "/var/max/etc/lang"

// fields.c:451 — Logo default
.default_value = "etc/misc/logo",
.file_base_path = "etc/misc",

// (repeated for ~25+ display file fields)
```

**Change:** Update all `.default_value` and `.file_base_path` references:
- `"etc/misc"` → `"display/screens"`
- `"etc/misc/<file>"` → `"display/screens/<file>"`
- `"etc/help/<file>"` → `"display/help/<file>"`
- `"/var/max/etc/misc"` → `"/var/max/display/screens"`
- `"/var/max/etc/lang"` → `"/var/max/config/lang"`

### 6.7 File Picker MEX Base Path — `src/apps/maxcfg/src/ui/form.c`

**Current fallback:**

```c
// form.c:1224-1225
const char *base_path = mex_mode[state.selected] ? "m" :
    (field->file_base_path ? field->file_base_path : "etc/misc");
```

**Change:** `"m"` → `"scripts"`, `"etc/misc"` → `"display/screens"`.

### 6.8 Color Form Paths — `src/apps/maxcfg/src/ui/colorform.c`

**Current hardcoded paths:**

```c
// colorform.c:200
snprintf(out, out_sz, "%s/etc/lang/colors.lh", root);

// colorform.c:214
snprintf(out, out_sz, "etc/lang/colors.lh");

// colorform.c:225
snprintf(out, out_sz, "%s/etc/lang", root);

// colorform.c:239
snprintf(out, out_sz, "etc/lang");
```

**Change:** All `etc/lang` references → `config/lang`.

### 6.9 Language Directory References — Multiple Files

**Affected files:**

| File | Line(s) | Current | Proposed |
|---|---|---|---|
| `maxcfg/src/main.c` | 228, 471 | `etc/lang` | `config/lang` |
| `maxcfg/src/config/ctl_to_ng.c` | 808 | `etc/language.ctl` | `config/legacy/language.ctl` |
| `maxcfg/src/config/lang_browse.c` | 1312 | `etc/lang` | `config/lang` |
| `maxcfg/src/ui/menubar.c` | 1961, 5926 | `etc/lang` | `config/lang` |
| `maxcfg/src/config/nextgen_export.c` | 1638 | `etc/lang/colors.lh` | `config/lang/colors.lh` |

### 6.10 Legacy CTL References — `maxcfg/src/config/ctl_to_ng.c`

**Current:** References `etc/access.ctl`, `etc/language.ctl` via `sys_path`.

**Change:** These are legacy import paths. Move legacy `.ctl` files to `config/legacy/`
and update the parser paths. Since CTL parsing is a one-way upgrade path, these
paths only matter during migration.

### 6.11 Access Control References

**Affected:**
- `maxcfg/src/ui/menubar.c:691` — `%s/etc/access.ctl`
- `maxcfg/src/config/ctl_to_ng.c:56` — `%s/etc/access.ctl`
- `maxcfg/src/config/nextgen_export.c:592, 1542` — `%s/etc/access.ctl`
- `maxcfg/src/config/fields.c:170` — `etc/access`

**Change:** Legacy `.ctl` references → `config/legacy/access.ctl`.
Runtime `file_access` default → `config/security/access`.

### 6.12 User Database — `src/apps/maxcfg/src/ui/mci_preview.c`, `src/utils/util/init_userdb.c`

**Current:**

```c
// mci_preview.c:106
snprintf(db_path, sizeof(db_path), "%s/etc/user.db", sys_path);

// init_userdb.c:243
snprintf(default_db, sizeof(default_db), "etc/user.db");
```

**Change:** `etc/user.db` → `data/users/user.db`.

### 6.13 Menu Legacy CTL — `src/apps/maxcfg/src/config/menu_parse.c`

**Current:**

```c
// menu_parse.c:945, 1200
snprintf(path, sizeof(path), "%s/etc/menus.ctl", sys_path);
```

**Change:** `etc/menus.ctl` → `config/legacy/menus.ctl` (legacy import only).

---

## 7. Build System Changes

### 7.1 `vars.mk.configure`

```makefile
# Current
BIN  = $(PREFIX)/bin
ETC  = $(PREFIX)/etc
LIB  = $(PREFIX)/lib

# Proposed
BIN  = $(PREFIX)/bin
LIB  = $(PREFIX)/bin/lib          # lib under bin
```

Remove `ETC` variable (no longer used as a single directory).

### 7.2 `Makefile` — Install Targets

**Current `config_install` target** (lines 161-180):

```makefile
config_install:
    @scripts/copy_install_tree.sh "$(PREFIX)"
    @[ -d ${PREFIX}/etc/lang ] || mkdir -p ${PREFIX}/etc/lang
    @cp -f build/etc/lang/english.toml ${PREFIX}/etc/lang/english.toml
    @cp -f resources/lang/delta_english.toml ${PREFIX}/etc/lang/delta_english.toml
    @[ -d ${PREFIX}/etc/db ] || mkdir -p ${PREFIX}/etc/db
    @cp -f scripts/db/userdb_schema.sql ${PREFIX}/etc/db/userdb_schema.sql
    @cp -f scripts/db/init-userdb.sh ${PREFIX}/bin/init-userdb.sh
    @[ ! -f ${PREFIX}/etc/user.db ] || ...
    @[ -f ${PREFIX}/etc/user.db ] || (cd ${PREFIX} && bin/init-userdb.sh ${PREFIX} || true)
```

**Proposed:**

```makefile
config_install:
    @scripts/copy_install_tree.sh "$(PREFIX)"
    @[ -d ${PREFIX}/config/lang ] || mkdir -p ${PREFIX}/config/lang
    @cp -f build/config/lang/english.toml ${PREFIX}/config/lang/english.toml
    @cp -f resources/lang/delta_english.toml ${PREFIX}/config/lang/delta_english.toml
    @[ -d ${PREFIX}/data/db ] || mkdir -p ${PREFIX}/data/db
    @cp -f scripts/db/userdb_schema.sql ${PREFIX}/data/db/userdb_schema.sql
    @cp -f scripts/db/init-userdb.sh ${PREFIX}/bin/init-userdb.sh
    @[ ! -f ${PREFIX}/data/users/user.db ] || ...
    @[ -f ${PREFIX}/data/users/user.db ] || (cd ${PREFIX} && bin/init-userdb.sh ${PREFIX} || true)
```

**Current `reconfig` target** (lines 182-193):

```makefile
reconfig:
    @(cd $(PREFIX)/etc/help && for f in *.mec; do ../../bin/mecca "$$f"; done)
    @(cd $(PREFIX)/etc/misc && for f in *.mec; do ../../bin/mecca "$$f"; done)
    @(cd $(PREFIX)/m && export MEX_INCLUDE=$(PREFIX)/m && for f in *.mex; do ../bin/mex "$$f"; done)
```

**Proposed:**

```makefile
reconfig:
    @(cd $(PREFIX)/display/help && for f in *.mec; do ../../bin/mecca "$$f"; done)
    @(cd $(PREFIX)/display/screens && for f in *.mec; do ../../bin/mecca "$$f"; done)
    @(cd $(PREFIX)/scripts && export MEX_INCLUDE=$(PREFIX)/scripts/include && \
      for f in *.mex; do ../bin/mex "$$f"; done)
```

**Current `buildclean` target** (line 95):

```makefile
-rm -rf $(PREFIX)/bin $(PREFIX)/lib $(PREFIX)/libexec
-rm -rf $(PREFIX)/etc $(PREFIX)/m $(PREFIX)/docs
```

**Proposed:**

```makefile
-rm -rf $(PREFIX)/bin $(PREFIX)/libexec
-rm -rf $(PREFIX)/config $(PREFIX)/display $(PREFIX)/scripts $(PREFIX)/docs
```

### 7.3 `scripts/copy_install_tree.sh`

The entire install tree must be restructured. The script copies
`resources/install_tree/*` into `$PREFIX`. The install tree itself needs
to be reorganized to match the proposed layout.

**Key changes:**
- `cp resources/scripts/runbbs.sh "${PREFIX}/bin/runbbs.sh"` → `"${PREFIX}/runbbs.sh"`
- Path substitution for legacy `.ctl` references
- Backup logic needs to cover `config/`, `display/`, `scripts/` instead of `etc/`, `m/`

### 7.4 `scripts/make-release.sh`

**Path references to update:**
- Line 155: `etc/max.ctl` → `config/legacy/max.ctl` (or remove — TOML is authoritative)
- Line 168: `etc/db` → `data/db`
- Line 196: `etc/misc/*.bbs` → `display/screens/*.bbs`
- Line 197: `etc/help/*.bbs` → `display/help/*.bbs`
- Line 201: `etc/lang/*.ltf` → *(remove — no .ltf files)*
- Line 205: `m/*.vm` → `scripts/*.vm`
- Line 217-228: `runbbs.sh` moves to root level, updates `lib` → `bin/lib`, `m` → `scripts`
- Line 231-268: `recompile.sh` updates all path references
- README directory structure description (lines 302-314) must match new layout

### 7.5 `resources/install_tree/` Reorganization

The install tree directory must be physically reorganized to match the proposed layout.
This is the source of truth for fresh installations.

**Current structure:**
```
resources/install_tree/
├── bin/
├── docs/
├── etc/           ← everything in here
│   ├── access.ctl
│   ├── display/
│   ├── help/
│   ├── lang/
│   ├── max.ctl
│   ├── menus/
│   ├── misc/
│   ├── nodelist/
│   ├── rip/
│   └── ...
├── ipc/
├── lib/
├── log/
├── m/
├── man/
├── spool/
└── tmp/
```

**Proposed structure:**
```
resources/install_tree/
├── runbbs.sh
├── bin/
│   └── lib/
├── config/
│   ├── general/
│   ├── lang/
│   ├── menus/
│   ├── areas/
│   │   ├── msg/
│   │   └── file/
│   ├── security/
│   └── legacy/     ← all .ctl files move here
├── display/
│   ├── screens/    ← was etc/misc
│   ├── menus/      ← was etc/display
│   ├── help/       ← was etc/help
│   └── tunes/
├── scripts/        ← was m/
│   └── include/
├── data/
│   ├── users/
│   ├── msgbase/
│   ├── filebase/
│   ├── nodelist/   ← was etc/nodelist
│   ├── mail/
│   ├── tracking/
│   └── db/
├── run/
│   ├── tmp/
│   ├── node/
│   └── stage/
├── log/
├── doors/
└── docs/
    └── wiki/
```

---

## 8. `runbbs.sh` — Root-Level Launcher

The new `runbbs.sh` sits at `$PREFIX/runbbs.sh` and launches `maxtel` as the
primary supervisor:

```bash
#!/bin/bash
# Maximus BBS launcher — starts maxtel supervisor
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="${SCRIPT_DIR}/bin/lib:$LD_LIBRARY_PATH"
export DYLD_LIBRARY_PATH="${SCRIPT_DIR}/bin/lib:$DYLD_LIBRARY_PATH"
export MEX_INCLUDE="${SCRIPT_DIR}/scripts/include"
export MAX_INSTALL_PATH="${SCRIPT_DIR}"
cd "$SCRIPT_DIR"
exec bin/maxtel -d "$SCRIPT_DIR" -p 2323 -n 4 "$@"
```

---

## 9. Environment Variable Changes

| Variable | Current Value | Proposed Value |
|---|---|---|
| `LD_LIBRARY_PATH` | `$PREFIX/lib` | `$PREFIX/bin/lib` |
| `DYLD_LIBRARY_PATH` | `$PREFIX/lib` | `$PREFIX/bin/lib` |
| `MEX_INCLUDE` | `$PREFIX/m` | `$PREFIX/scripts/include` |
| `MAX_INSTALL_PATH` | `$PREFIX` | `$PREFIX` (unchanged) |
| `MAXIMUS` | `$PREFIX` | `$PREFIX` (unchanged) |

---

## 10. Migration Checklist

### Phase 1: Foundation
- [ ] Reorganize `resources/install_tree/` to proposed layout
- [ ] Update `vars.mk.configure` — `LIB` under `BIN`
- [ ] Update `Makefile` — all install/reconfig/clean targets
- [ ] Update `scripts/copy_install_tree.sh`
- [ ] Create new `runbbs.sh` at root level

### Phase 2: Runtime Path Resolution
- [ ] Add `node_path()` helper for per-node file resolution
- [ ] Update `max_v.h` format strings or remove in favor of helper
- [ ] Update all per-node file callsites (5.3 above)
- [ ] Update IPC path construction in `max_chat.c` / `max_cho.c`
- [ ] Update `maximus.toml` default path values

### Phase 3: Display / Language / MEX
- [ ] Update language loading in `language.c` — use `lang_path` config
- [ ] Update `mexlang.c` — use `lang_path` config
- [ ] Update `fields.c` — all default paths and `file_base_path` values
- [ ] Update `colorform.c` — `colors.lh` path
- [ ] Update `form.c` — MEX file picker base path
- [ ] Update all `etc/lang` references in maxcfg

### Phase 4: Maxtel
- [ ] Update `maxtel.c` — `lib_path`, `mex_path`, socket/lock paths
- [ ] Update `maxtel.c` — per-node file paths (termcap, lastuser, bbstat)
- [ ] Update `maxtel.c` — default `max_path` if needed

### Phase 5: Build / Release
- [ ] Update `scripts/make-release.sh` — all path references
- [ ] Update `scripts/build-linux.sh` / `build-macos.sh` if needed
- [ ] Update `STRUCTURE.md` to reflect new layout
- [ ] Test full `make build && make install` cycle
- [ ] Test `make-release.sh` produces correct package
- [ ] Test maxtel supervisor spawns nodes correctly

### Phase 6: Legacy Cleanup
- [ ] Move legacy `.ctl` files to `config/legacy/`
- [ ] Update CTL parser paths in `ctl_to_ng.c` for migration path
- [ ] Update `nextgen_export.c` legacy references
- [ ] Remove `man/` from install tree
- [ ] Verify no stray `etc/` references remain in codebase
