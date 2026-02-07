# Runtime TOML Wiring Report (Maximus)

This document tracks which next-gen TOML files are loaded by the Maximus runtime, which keys are actively consumed (TOML-first with PRM fallback), and what has been verified via smoke testing.

## Status Summary

- Maximus runtime now has a global next-gen TOML tree (`ng_cfg`) and helper APIs (`ngcfg_get_*`) for TOML-first lookup with legacy PRM fallback.
- A targeted subset of TOML files is currently loaded at startup.
- The currently-wired keys have been smoke-tested and appear to be working (except where explicitly called out below).

## Runtime TOML Load (Current)

Loaded once at startup from `Read_Prm()` (when `ng_cfg == NULL`):

The config root directory is discovered at runtime (preferring `MAX_INSTALL_PATH`, then `MAXIMUS`, then `./config/`).

Once `maximus.toml` is loaded, Maximus prefers `maximus.config_path` (TOML-first with PRM fallback) as the authoritative config root.

Loaded files (relative to the resolved config root):

- `maximus.toml` as `maximus`
- `general/session.toml` as `general.session`
- `general/display_files.toml` as `general.display_files`
- `general/equipment.toml` as `general.equipment`
- `general/colors.toml` as `general.colors`
- `general/reader.toml` as `general.reader`
- `general/protocol.toml` as `general.protocol`
- `general/language.toml` as `general.language`
- `security/access_levels.toml` as `security.access_levels`
- `areas/msg/areas.toml` as `areas.msg`
- `areas/file/areas.toml` as `areas.file`
- `matrix.toml` as `matrix`
- `menus/*.toml` as `menus.*`

## Runtime TOML Consumers (Current)

The following runtime modules now prefer TOML values via `ngcfg_get_*`, with PRM fallback:

- `src/max/max_init.c`
  - Loads TOMLs (see above)
  - Config root discovery + `maximus.config_path` support:
    - Avoids hard-coded `config/...` loads and avoids relying on working directory
  - Uses TOML keys for `first_menu`, `mcp_pipe`, and `input_timeout`
  - Applies TOML overrides into PRM scalars for:
    - `general.reader.max_pack`
    - `general.language.max_lang`, `general.language.max_ptrs`, `general.language.max_heap`, `general.language.max_glh_ptrs`, `general.language.max_glh_len`, `general.language.max_syh_ptrs`, `general.language.max_syh_len`
    - `general.protocol.protoexit`
    - `matrix.*` (selected scalar fields)
  - Access levels are TOML-first by generating a temporary `access.dat` from `security.access_levels` at startup (with fallback)
  - Archiver control file is TOML-first via `general.reader.archivers_ctl` (with fallback)
  - Colors: applies `general.colors.*` to runtime language color strings (menu/file/message/FSR `*_col` AVATAR sequences)
- `src/max/max_log.c`
  - Prefers `general.display_files.*` display-file paths
  - Prefers `maximus.misc_path` for misc/display prompt content
  - Prefers `maximus.sysop`
  - Prefers `maximus.file_password` for the user file path (legacy `PRM(user_file)` use)
  - Prefers `general.session.first_message_area` / `general.session.first_file_area`

- Protocols / transfers
  - `src/max/f_up.c`, `src/max/f_down.c`, `src/max/m_attach.c`
    - Prefers `general.display_files.xfer_baud` for xfer-baud warning display file
  - `src/max/f_xfer.c`
    - Prefers `general.display_files.protocol_dump` when showing available protocols
  - `src/max/max_prot.c`
    - Prefers `general.protocol.protocol_max_path` for `protocol.max`
  - `src/max/f_logup.c`
    - Prefers `general.session.upload_log` for upload log path
- `src/max/max_rmen.c`
  - Menus are TOML-first (reads `menus.*` TOML and builds runtime menu heap)
- `src/max/max_init.c` (areas)
  - Message/file areas are TOML-first (generates temporary `marea.*` / `farea.*` from `areas.msg` + `areas.file`, with PRM fallback)

- Reader (QWK/OLR)
  - `src/max/mb_qwk.c`
    - Prefers `general.reader.work_directory`, `general.reader.packet_name`, `general.reader.archivers_ctl`, `general.reader.phone`
  - `src/max/mb_qwkup.c`
    - Prefers `general.reader.packet_name` when validating/handling .REP uploads
  - `src/max/m_util.c`
    - Prefers `general.reader.packet_name` when skipping cleanup of in-progress QWK packet
- `src/max/tagapi.c`
  - Prefers `maximus.sys_path` for MTAG.* file locations
- `src/max/events.c`
  - Prefers `maximus.sys_path` for `eventsxx.dat`/`eventsxx.bbs`
- `src/max/max_chat.c`
  - Prefers `maximus.ipc_path` for IPCXX.BBS access and chat IPC
- `src/max/max_fini.c`
  - Prefers `general.equipment.busy` for modem busy command string
  - Prefers `maximus.ipc_path` for IPC cleanup
- `src/max/m_upload.c`
  - Prefers `maximus.temp_path` for upload temp directory
- `src/max/max_main.c` and `src/max/disp_max.c`
  - Prefers `general.display_files.tune` for tune file path
- `src/max/max_args.c` (UNIX)
  - Prefers `maximus.ipc_path` for dynamic task-number semaphore path

## Smoke Test Coverage (Completed)

The following runtime behaviors were manually smoke-tested and appear to be working:

- Session/menu selection
  - `general.session.first_menu`
  - `general.session.input_timeout`
- Display files
  - `general.display_files.*`
- Path base semantics
  - `maximus.sys_path` (including relative-path prefixing)
- IPC path
  - `maximus.ipc_path`
- Temp path
  - `maximus.temp_path`
- Tunes
  - `general.display_files.tune`
- Equipment (busy string)
  - `general.equipment.busy`
- Menus
  - `menus.*`
- Areas
  - `areas.msg`
  - `areas.file`

- Protocols/transfers
  - `general.display_files.xfer_baud`
  - `general.display_files.protocol_dump`
  - `general.protocol.protocol_max_path`
  - `general.session.upload_log`

- Reader (QWK/OLR)
  - `general.reader.work_directory`
  - `general.reader.packet_name`
  - `general.reader.archivers_ctl`
  - `general.reader.phone`

- Access levels
  - `security.access_levels` -> generated runtime `access.dat` (with fallback)

- Matrix scalars
  - `matrix.*` selected scalars (applied as PRM overrides)

## Not Yet Wired (Runtime)

These TOMLs are loaded, but do not yet have substantial runtime consumers beyond basic bootstrapping and a small number of TOML-first call sites:

- None currently tracked in this category (as of the latest wiring pass).

## Next Steps

- Finish PRM hotwiring sweeps (TOML-first, PRM fallback OK for now):
  - Logging + paths
  - Protocols + transfers

- Smoke test `general.colors` runtime overlay (menus/files/messages/FSR)

- Decide long-term fate of legacy status/popup/WFC palette colors (`_maxcol col` / `colours.dat`) for MAXTEL-only future builds.

## Notes

- `mexint.c` must include `libmaxcfg.h` before language headers to avoid macro collisions with generated language symbols (e.g. `file_offline`).
