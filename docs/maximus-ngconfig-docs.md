# Maximus Next-Generation TOML Configuration

**Version:** 3.00+  
**Date:** January 2026  
**Author:** Kevin Morgan (Limping Ninja)

## Overview

Maximus 3.00+ introduces a TOML-based configuration system replacing legacy CTL/PRM files. This document describes the TOML structure, integration, and field documentation.

This document is the authoritative location for **field meaning, intent, and operational notes**. TOML configuration files are rewritten by tooling and do not preserve comments, so sysops should rely on this document for descriptions.

## TOML File Structure

```
config/
|--- maximus.toml              # System settings
|--- general/
|    |--- session.toml          # Session/user settings
|    |--- language.toml         # Language configuration
|    `--- display_files.toml    # Display file paths
|--- matrix.toml               # EchoMail/NetMail/FidoNet
|--- areas/
|    |--- msg/areas.toml        # Message areas
|    `--- file/areas.toml       # File areas
|--- menus/*.toml              # Menu definitions
`--- security/access_levels.toml
```

## Migration from CTL

### Command Line

```bash
maxcfg --export-nextgen /path/to/max.ctl
```

### UI

**Tools -> Export Next-Gen Config** (hotkey: E)

### What Gets Converted

- All system, session, display settings
- EchoMail/Matrix configuration
- Message and file areas
- Menus and access levels
- Runtime-computed values are written as 0 (or omitted) and computed at runtime

## Configuration Reference (By TOML File)

This section is organized by TOML file. Each file section:

- Lists the keys present in that file.
- Describes the meaning and intent of each key.
- Notes any legacy CTL keyword(s) the value historically came from.

### `config/maximus.toml` (System / Global)

**Purpose**: global system identity, key paths, logging, and core runtime toggles.

**Legacy sources**: `etc/max.ctl` (primarily the System section).

#### Keys

##### `config_version` (int)

Schema/version marker for the TOML configuration.

This is primarily intended for tooling and migrations. If you are hand-editing configuration, you normally do not need to change this.

##### `system_name` (string)

**Legacy**: `Name`

The name of your BBS/system. This is used as a default for EchoMail origin lines unless overridden per-area.

##### `sysop` (string)

**Legacy**: `SysOp`

Display name for the sysop. This does not grant privileges by itself; privileges come from the user's configured access level.

##### `snoop` (bool)

**Legacy**: `Snoop`

Controls whether "snoop" mode is enabled by default for the local console (the sysop screen mirrors the caller's screen).

##### `video` (string)

**Legacy**: `Video`

Selects the console video output mode. This is historically relevant for DOS builds; on modern platforms it is generally ignored.

##### `sys_path` (string)

**Legacy**: `Path System`

The base directory for the Maximus installation. Relative paths in TOML are resolved from this root unless a key states otherwise.

##### `misc_path` (string)

**Legacy**: `Path Misc`

Directory containing miscellaneous display files (and other system text files shown to callers).

##### `lang_path` (string)

**Legacy**: `Path Language`

Directory containing language translation files.

##### `temp_path` (string)

**Legacy**: `Path Temp`

Temporary working directory used for uploads and other operations. This directory is treated as scratch space; do not store important files here.

##### `ipc_path` (string)

**Legacy**: `Path IPC`

Inter-process communications directory. This is typically set to a fast local filesystem.

##### `file_password` (string)

**Legacy**: `File Password`

Location of the user database.

##### `no_password_encryption` (bool)

**Legacy**: `No Password Encryption`

Disables automatic password encryption. This is only recommended for compatibility with tooling that cannot read encrypted passwords.

##### `file_access` (string)

**Legacy**: `File Access`

Location of the access/privilege-level database.

##### `file_callers` (string)

**Legacy**: `File Callers`

Location of the callers log (call-overview log). Maximus records session summaries here for use by third-party utilities.

##### `log_file` (string)

**Legacy**: `Log File`

Primary system log file used to record notable system events. If no log file is configured, no log will be produced.

##### `log_mode` (string)

**Legacy**: `Log Mode`

Controls the verbosity of logging. Historically supported values include `Terse`, `Verbose`, and `Trace`.

##### `task_num` (int)

**Legacy**: `Task`

Node/task number for multi-node configurations. Used in logging and in separating per-node state.

##### `mcp_pipe` (string)

**Legacy**: `MCP Pipe`

Inter-process communications pipe/path used by MCP integrations (historically OS/2-focused).

##### `mcp_sessions` (int)

**Legacy**: `MCP Sessions`

Maximum number of online nodes/sessions.

##### `multitasker` (string)

**Legacy**: `Multitasker`

Identifies the multitasking environment. Historically used for DOS/OS2 behavior tuning.

##### `menu_path` (string)

**Legacy**: `Menu Path`

Directory containing menu files used at the start of a session (the default `*.mnu` / menu assets). This value can be changed later by certain menu commands, but this is the initial location Maximus uses.

If left blank in legacy configs, Maximus would fall back to `sys_path`.

##### `rip_path` (string)

**Legacy**: `RIP Path`

Directory containing RIP icon and scene files (used by MECCA RIP-related tokens).

##### `stage_path` (string)

**Legacy**: `Stage Path`

Temporary staging directory used for CD-ROM or other slow storage media. When enabled, files from areas marked as `Staged` (or `CD`) are copied into this directory before being transferred to the caller.

##### `swap` (bool)

**Legacy**: `Swap`

When enabled (legacy DOS behavior), Maximus swaps itself out of memory when running external programs.

##### `reboot` (bool)

**Legacy**: `Reboot`

Legacy DOS watchdog behavior. Historically used to reboot the machine if a caller drops carrier while inside an external program.

##### `dos_close` (bool)

**Legacy**: `Dos Close Standard Files`

Closes internal files while launching external programs. Historically required if external programs must append to the system log.

##### `no_share` (bool)

**Legacy**: `No Share.Exe`

Legacy DOS/NetWare-era setting related to file sharing behavior. When using Squish-format message areas in a multitasking/network environment, the legacy documentation warns that proper sharing support is required to prevent message base corruption.

##### `status_line` (bool)

**Legacy**: `StatusLine`

When enabled, Maximus displays a status line at the bottom of the local screen while a remote caller is online. Historically this only worked in the `Video IBM` mode.

##### `local_input_timeout` (bool)

**Legacy**: `Local Input Timeout`

Safety feature for local logins. When enabled, Maximus will automatically log off a local user after five minutes of inactivity. This safety behavior is always active for remote callers.

##### `message_data` (string)

**Legacy**: `MessageData`

Directory (or path root) where message area data files are stored.

##### `file_data` (string)

**Legacy**: `FileData`

Directory (or path root) where file area data files are stored.

##### `net_info_path` (string)

**Legacy**: `Path NetInfo`

Directory containing nodelist / net-info files (used for network addressing and related lookups).

##### `outbound_path` (string)

**Legacy**: `Path Outbound`

Outbound directory for certain legacy mailer/control-file formats. In the legacy documentation this is described as an Opus compatibility path and is not used by Maximus itself in many configurations.

##### `inbound_path` (string)

Inbound directory for inbound mail bundles.

Depending on your mailer/packer pipeline, this may be used by external tools more than Maximus itself. Keep it on a reliable local filesystem.

##### `protocol_ctl` (string)

**Legacy**: `Include .../protocol.ctl`

Path to the external protocol definitions control file (historically `etc/protocol.ctl`). This TOML key exists so tools can locate protocol definitions without relying on CTL include behavior.

##### `has_snow` (bool)

**Legacy**: `Video IBM/snow`

When true, enables the legacy CGA "snow" avoidance behavior for direct screen writes. This is typically only relevant on very old hardware.

### `config/general/session.toml` (Session / Policy)

This file controls the day-to-day "personality" of your BBS session: how callers log on, what the system considers a valid name, how graphics are negotiated, where new users start, and how long Maximus will wait before timing out.

**Legacy sources**: primarily the `Session Section` in `etc/max.ctl`, plus supporting notes in the Maximus manual (`max_mast`).

#### Overview

Many of these settings are "policy" knobs. They do not create content (areas, menus, file lists), but they decide how Maximus behaves while a caller is online.

If you are migrating from CTL, this is the section that tends to answer questions like: "Why does it keep asking for a last name?", "Why are ANSI screens disabled?", or "Why does the system log off idle users?"

#### Keys

##### `alias_system` (bool)

**Legacy**: `Alias System`

Enables system-wide alias (handle) usage by default.

In plain terms, this controls whether Maximus treats a user's alias as their "primary" name for most displays and message authoring.

In legacy Maximus terminology:

- When enabled, Maximus prefers the user's alias for display and message authoring unless an area explicitly forces real names.
- This is commonly paired with `ask_alias` so new users are prompted to choose an alias.

If you want a handle-based system (where callers appear as handles on "Who is On" and in message headers), you almost always want this enabled.

##### `ask_alias` (bool)

**Legacy**: `Ask Alias`

When enabled, Maximus prompts users (especially new users) for an alias.

Practical notes:

- Even if aliases are not used system-wide, collecting an alias can still be useful for specific areas, doors, or sysop policy.
- In legacy behavior, a user with an alias may be able to log in using either their real name or alias at the name prompt regardless of these two toggles.

Common setup:

- Enable both `alias_system` and `ask_alias` if you want handles.
- Enable only `ask_alias` if you want to collect handles but keep real names as the default.

##### `single_word_names` (bool)

**Legacy**: `Single Word Names`

Allows usernames containing only a single word.

This is useful for handle-centric systems and it suppresses the default "What is your LAST name:" prompt.

##### `check_ansi` (bool)

**Legacy**: `Check ANSI`

Enables ANSI capability verification at logon.

If auto-detection cannot confirm ANSI support, Maximus prompts the user to confirm before enabling ANSI.

##### `check_rip` (bool)

**Legacy**: `Check RIP`

Enables RIP capability verification at logon.

If auto-detection cannot confirm RIP support, Maximus prompts the user to confirm before enabling RIP.

##### `ask_phone` (bool)

**Legacy**: `Ask Phone`

When enabled, Maximus prompts new users for their telephone number during logon.

##### `no_real_name` (bool)

**Legacy**: `No Real Name`

When enabled, Maximus does not require a user's real name during the logon/new-user process.

This is a policy choice: some sysops want real names for accountability, while others prefer a handle-based environment. If you run a handle system (aliases on, single word names allowed), this is often enabled.

##### `disable_userlist` (bool)

**Legacy**: `Disable Userlist` / `Edit Disable Userlist`

Disables the interactive "user list" feature when addressing private messages.

In classic Maximus behavior, users could press `?` at the "To:" prompt to get a list of users. Some systems disable this to discourage random private messaging or to keep the user base less discoverable.

##### `disable_magnet` (bool)

**Legacy**: `Disable Magnet`

Disables the BORED/MAGNET-style full-screen editor for callers.

If you want to force the line editor (or an external editor policy) for certain classes of users, this is one of the switches that affects editor selection.

##### `edit_menu` (string)

**Legacy**: `Edit Menu`

Name of the menu used for editor-related functions.

Maximus historically requires an "EDIT" menu name for certain flows (notably the BORED editor). If you leave this blank, Maximus typically falls back to `EDIT`.

##### `min_logon_baud` (int)

**Legacy**: `Min Logon Baud`

Minimum connection speed required to log on.

Notes:

- In the legacy control file, this is described as an additional gate on top of other session limits.
- If omitted in CTL (commented out), legacy Maximus used its defaults.

##### `min_graphics_baud` (int)

**Legacy**: `Min NonTTY Baud`

Minimum connection speed required for ANSI/AVATAR graphics (non-TTY features).

##### `min_rip_baud` (int)

**Legacy**: `Min RIP Baud`

Minimum connection speed required to enable RIP graphics.

To disable RIP graphics support in the legacy configuration, the documentation recommends setting this value to `65535`.

##### `logon_priv` (int)

**Legacy**: `Logon Level`

Default privilege level assigned to new users.

Legacy note:

- The CTL file also supports a closed-system mode (`Logon Preregistered`) that shows the application file and then hangs up.

##### `logon_timelimit` (int)

**Legacy**: `Logon Timelimit`

Maximum time (in minutes) allowed for a caller to complete the login process.

This includes displaying the logo, prompting for name/password, and displaying the application file (if applicable).

##### `first_menu` (string)

**Legacy**: `First Menu`

Name of the menu displayed after the logon sequence.

##### `first_message_area` (string)

**Legacy**: `First Message Area`

Default starting message area for new users.

##### `first_file_area` (string)

**Legacy**: `First File Area`

Default starting file area for new users.

##### `area_change_keys` (string)

**Legacy**: `Area Change Keys`

Controls the key bindings shown on the area-change menu.

Legacy behavior:

- Character 1: change to prior area
- Character 2: change to next area
- Character 3: list areas

##### `input_timeout` (int)

**Legacy**: `Input Timeout`

Controls how long Maximus waits for input before timing out.

Legacy behavior:

- Maximus waits the configured number of minutes.
- It then sends a warning.
- It then waits one additional minute.
- If still no input occurs, the session is terminated.

The documented valid range is 1 to 127 minutes.

##### `max_msgsize` (uint)

**Legacy**: `MaxMsgSize`

Sets the maximum size of a message that can be uploaded using message upload features.

In day-to-day sysop terms: if you have users who upload messages or large prepared text, this is the "hard ceiling" Maximus will accept.

##### `min_free_kb` (uint)

**Legacy**: `Upload Space Free`

Minimum free disk space required (in KB) to allow an upload.

If the upload drive has less free space than this, Maximus aborts the upload and shows the configured "no space" message.

##### `upload_log` (string)

**Legacy**: `Upload Log`

Optional dedicated upload log. The legacy documentation describes this as plain ASCII containing uploader, filename, size, and date/time.

##### `virus_check` (string)

**Legacy**: `Virus Check` (legacy CTL comments may refer to this as an upload virus check hook)

Optional external virus checker hook.

In the legacy configuration, this was a command or batch file invoked once per uploaded file. If you enable this, make sure the command you run is safe, reliable, and returns the expected result codes for your setup.

##### `upload_check_dupe` (bool)

**Legacy**: `Upload Check Dupe`

Enables duplicate upload detection.

When enabled, Maximus checks for duplicates when the user selects the upload command and refuses duplicate files.

Operational note (legacy): this feature depends on up-to-date file area metadata; in classic setups this meant recompiling file areas (for example with FB) after changing file area definitions.

##### `upload_check_dupe_extension` (bool)

**Legacy**: `Upload Check Dupe Extension`

When enabled, duplicate checking considers the filename extension as well as the base name.

##### `exit_after_call` (int)

**Legacy**: `After Call Exit`

Controls the process exit code Maximus returns after a caller disconnects.

This is one of the classic "batch file" integration points. If you run Maximus under a wrapper script, you can use this exit code to decide what to do next (pack mail, run maintenance, rotate logs, etc.). The legacy documentation notes the usable range is 5 to 255.

##### `chat_program` (string)

**Legacy**: `Chat Program` (historically documented as `Chat External`)

If set, this runs an external program (or a MEX program) for the C)hat key instead of the internal chat module.

Legacy tip: to run a MEX program instead of an external executable, the command name starts with a `:`.

##### `local_editor` (string)

**Legacy**: `Local Editor`

Configures an external message editor command used instead of the internal MaxEd editor.

The legacy manual describes several useful behaviors:

- If the message is a reply, Maximus creates `msgtmp##.$$$` (where `##` is the task number in hex) containing the quoted text, then invokes the editor.
- After the editor exits, Maximus expects the final message text to be present in the same `msgtmp##.$$$` file.
- If you include `%s` in the editor command string, Maximus replaces it with the filename of the temporary message file.
- If the command starts with `@`, certain remote users may also be allowed to use the "local" editor (controlled via `MailFlags Editor` / `MailFlags LocalEditor` access settings).

##### `compat_local_baud_9600` (bool)

**Legacy**: `Local Baud 9600`

Compatibility switch for older setups that treated the local console as a fixed high-speed connection for feature gating.

On modern systems this is usually left disabled, but it can help preserve legacy behavior in edge cases where local/remote capability checks were based on a baud-rate assumption.

##### `chat_capture` (bool)

**Legacy**: `Chat Capture On`

Controls whether Maximus automatically begins logging sysop/caller chat when chat mode is entered.

If enabled, the legacy behavior is to create chat logs in the system root using a CHATLOG.* filename with the node number as the extension.

##### `strict_xfer` (bool)

**Legacy**: `Strict Transfer` (historically documented as `Strict Time Limit`)

Enforces time limits during internal file transfers.

In classic Maximus terms, enabling this causes Maximus to terminate (and log) a file transfer if the caller runs out of time while a download is in progress. This is primarily relevant to internal protocols.

##### `track_base` (string)

**Legacy**: `Track Base`

Enables and configures the Message Tracking System (MTS) database.

This value is the path and root name of the tracking database. In classic Maximus style, you only specify the root because multiple database files are created using that name.

If you are not using MTS, leave this unset/empty.

##### `track_privview` (string)

**Legacy**: `Track PrivView`

Privilege required to view tracking information for messages owned by other users.

In practice, this is what separates "regular users can see their own ticket status" from "staff can inspect and triage everyone".

##### `track_privmod` (string)

**Legacy**: `Track PrivMod`

Privilege required to modify tracking information for messages not owned by the current user.

Legacy notes:

- This privilege is also required to delete entries from the tracking database.
- It is also required to access tracking administration menus.

##### `track_exclude` (string)

**Legacy**: `Track Exclude`

Name/path of the tracking exclusion list.

This is a simple text file (one user name per line) used to prevent automatic tracking assignment for specific users. Messages entered by excluded users can still be placed into the tracking system manually (via track insert workflows), but automatic insertion is disabled.

##### `charset` (string)

**Legacy**: `Charset <name>`

Selects an internal character set mode.

The legacy manual describes at least these values:

- `swedish`: Swedish 7-bit character set support
- `chinese`: Chinese BIG5 support

Practical notes:

- This setting interacts with `global_high_bit` in some configurations.
- Charset support may require corresponding language-file adjustments (for example, bracket definitions for Swedish).

##### `yell_enabled` (bool)

**Legacy**: `Yell Off` (inverted)

Controls whether the Y)ell command makes noise on the local console by default.

In legacy configs this was a "noise maker" default toggle (it could still be changed locally while the system was running). If you want Yell to be available but silent by default, disable this.

##### `global_high_bit` (bool)

**Legacy**: `Global High Bit`

Enables high-bit characters throughout most of the BBS (names, file descriptions, etc.).

This is a powerful feature, but it comes with a tradeoff: callers whose terminal programs are configured for 7-bit operation may not be able to log on successfully.

##### `filelist_margin` (int)

**Legacy**: `FileList Margin`

Controls the indentation applied when wrapping long file descriptions in file listings.

##### `autodate` (bool)

**Legacy**: `File Date Automatic` / `File Date Manual`

This controls where Maximus gets the date and size information it shows in file listings.

If `autodate` is enabled (legacy "Automatic" mode), Maximus reads the file's directory entry and displays the size/date from the filesystem. This is the most common setup on modern storage.

If `autodate` is disabled (legacy "Manual" mode), Maximus assumes the file's date/size are stored in FILES.BBS as text. In classic setups this was used for speed on slow media (CD-ROMs, WORMs, and other cases where directory access was expensive).

Practical guidance:

- Use automatic unless you have a specific reason to maintain FILES.BBS dates manually.
- If you use manual mode, remember that existing catalog entries may need dates added by hand.

##### `date_style` (int)

**Legacy**: `File Date ... <format>`

Controls the date format used when Maximus displays dates in file listings and when it prompts a user for a date (for example, during new-files scanning).

Legacy formats:

- `mm-dd-yy` (USA)
- `dd-mm-yy` (Canada/England)
- `yy-mm-dd` (Japanese)
- `yymmdd` (scientific)

In legacy behavior, the chosen format applies slightly differently depending on `autodate`:

- In automatic mode, Maximus formats the date at display time.
- In manual mode, Maximus expects dates in FILES.BBS to already be written in the selected format.

##### `attach_base` (string)

**Legacy**: `Attach Base`

Root name of the file attach database. Only the root is specified because multiple database files are created using that name.

##### `attach_path` (string)

**Legacy**: `Attach Path`

Default holding directory for local file attaches. The legacy documentation notes this can be overridden per message area via the message area definition.

##### `attach_archiver` (string)

**Legacy**: `Attach Archiver`

Archiver type used for local attach storage (must be defined in the system's archiver configuration).

##### `msg_localattach_priv` (int)

**Legacy**: `Message Edit Ask LocalAttach`

Privilege level required to create a local file attach.

If you offer file attaches to regular users, keep this reasonably low. If you want attaches to be a sysop-only feature (common on some systems), raise it accordingly.

##### `kill_attach` (string: `never|ask|always`)

**Legacy**: `Kill Attach`

Controls how Maximus deletes received file attaches.

Legacy behaviors:

- `never`: never delete received attaches unless the associated message is deleted
- `ask`: prompt user to delete the attach upon receipt
- `always`: always delete received attaches

##### `kill_attach_priv` (int)

**Legacy**: `Kill Attach Ask <acs>`

Privilege/ACS threshold used when `kill_attach` is set to `ask`. If the user's privilege is below this threshold, the attach is deleted without prompting.

##### `kill_private` (string: `never|ask|always`)

**Legacy**: `Kill Private`

Controls what happens to private messages after a user reads them.

Legacy behavior:

- `ask` was historically the default
- `always` deletes automatically
- `never` keeps private messages after reading

##### `mailchecker_kill_priv` (int)

**Legacy**: `Mailchecker Kill`

Privilege threshold used by the internal mailchecker for whether "kill" actions are available.

Legacy note: Maximus also checks the access levels of the relevant menu commands (Msg_Kill, etc.) in a menu named MESSAGE.

##### `mailchecker_reply_priv` (int)

**Legacy**: `Mailchecker Reply`

Privilege threshold used by the internal mailchecker for whether reply actions are available.

Legacy note: as with kill privileges, menu command access levels also apply.

##### `use_umsgids` (bool)

**Legacy**: `Use UMSGIDs`

When enabled (Squish bases only), messages receive unique identifiers that are never reused even after deletions. This provides "constant message number" behavior.

##### `comment_area` (string)

**Legacy**: `Comment Area`

Area where log-off comments are stored.

If you change this, make sure the referenced area exists and is accessible to the users who are expected to leave comments.

##### `highest_file_area` (string)

**Legacy**: `Highest FileArea`

Limits how high Maximus will search when performing certain file-area operations (such as locate) and also affects how "next" and "previous" area navigation behaves.

If you use this as a protection mechanism, remember it is not a true access control system by itself; it is a navigation/search limit.

##### `highest_message_area` (string)

**Legacy**: `Highest MsgArea`

Message-area counterpart to `highest_file_area`. In legacy configs this limited scan behavior.

##### `save_directories` (array of strings)

**Legacy**: `Save Directories`

Controls which drive letters Maximus saves/restores the current directory for when running external programs.

This is a legacy DOS-era safety feature. Historically, sysops were warned not to include removable drives (floppies, CD-ROM) because Maximus would try to access them each time it executed an external program.

To disable directory saving entirely, leave this empty.

##### `gate_netmail` (bool)

**Legacy**: `Gate NetMail`

Gate-routes interzone NetMail through the FidoNet zone gate.

The Maximus manual notes this is handled by the mail processor and is normally left disabled unless specifically required.

#### Examples

Example: handle-centric system with safe defaults:

```toml
alias_system = true
ask_alias = true
single_word_names = true

check_ansi = true
check_rip = true
min_graphics_baud = 1200
min_rip_baud = 65535

logon_timelimit = 15
input_timeout = 15
```

### `config/matrix.toml` (Matrix / NetMail / EchoMail)

This file contains FidoNet addressing, echomail behavior, and message attribute privilege policies.

**Legacy sources**: the `Matrix and EchoMail Section` in `etc/max.ctl`, plus the Matrix/EchoMail keyword listing in the Maximus manual (`max_mast`).

#### Overview

If you run a FidoNet-style networked system, this file is where you tell Maximus:

- What addresses you own (your "matrix" addresses)
- Where to find nodelist information (so names and node numbers resolve cleanly)
- Who is allowed to see or edit technical message control lines (kludges, SEEN-BYs)
- What NetMail attributes users are allowed to request or automatically apply
- When Maximus should exit with specific errorlevels so an external mailer/tosser can run

This file does not define message areas themselves. Instead, it defines the network behavior and policy that applies to NetMail/EchoMail features across the system.

#### Privilege values

Many keys in this file are "privilege threshold" values.

In legacy CTL, these are commonly written using access level names (for example `Sysop`, `AsstSysOp`, `Hidden`). In TOML, these are written as integers.

Practical notes:

- During conversion/export, access level names are resolved using `etc/access.ctl`.
- `Hidden` is treated as a special case and exported as `65535`.
- A value of `0` generally means "no special restriction" or "not configured" depending on the key.

#### Keys

##### `[message_edit.ask]` (table)

See `message_edit.ask` below.

##### `[message_edit.assume]` (table)

See `message_edit.assume` below.

##### `ctla_priv` (int)

**Legacy**: `Message Show Ctl_A to <priv>`

Controls the privilege required to see ^A control ("kludge") lines when reading messages.

If you are aiming for a clean reader experience for normal callers, keep this high (or set to hidden). If you need moderators to debug message routing, set it to a staff level.

##### `seenby_priv` (int)

**Legacy**: `Message Show Seenby to <priv>`

Controls the privilege required to see SEEN-BY and related echomail routing lines.

SEEN-BYs are useful for diagnosing echomail distribution problems, but they are mostly noise for regular users.

##### `private_priv` (int)

**Legacy**: `Message Show Private to <priv>`

Controls the privilege required to read private messages that are not addressed to the current user.

Operational guidance (legacy): setting this below AsstSysOp is not recommended.

##### `fromfile_priv` (int)

**Legacy**: `Message Edit Ask FromFile <priv>`

Controls the privilege required for "from file" message entry (for example, the FB "bombing run" workflow).

This is one of the easiest ways for a user to inject large amounts of text or mass-mail, so it is typically restricted to sysop/staff.

##### `unlisted_priv` (int)

**Legacy**: `Message Send Unlisted <priv> <cost>`

Privilege required to send NetMail to nodes that are not present in your configured nodelist.

##### `unlisted_cost` (int)

**Legacy**: `Message Send Unlisted <priv> <cost>`

Cost (in cents) charged to the user's matrix/netmail account when sending to an unlisted address.

If you want to allow unlisted sends but not charge, set this to `0`.

##### `log_echomail` (bool)

**Legacy**: `Log EchoMail <filespec>`

Enables writing an "echotoss" list file as users enter EchoMail.

This file is intended as input to your external mail exporter/tosser (for example, SquishMail). In classic setups, this significantly reduces the time required to scan and pack outgoing EchoMail.

##### `echotoss_name` (string)

**Legacy**: `Log EchoMail <filespec>` (tosslog filename)

Filename used for the EchoMail tosslog.

Practical notes:

- If `log_echomail` is enabled, you should set this to a valid writable path (relative to `sys_path` is typical).
- Some legacy systems allowed variable substitution in this path; if you see `%` sequences in older configs, keep them.

##### `after_edit_exit` (int)

**Legacy**: `After Edit Exit <errorlevel>`

Exit code Maximus uses when a user enters a NetMail message.

This is a batch integration hook: your wrapper script can detect this exit code and immediately run a mail packer/exporter.

##### `after_echomail_exit` (int)

**Legacy**: `After EchoMail Exit <errorlevel>`

Exit code Maximus uses when a user enters an EchoMail message.

Legacy behavior note: this supersedes the `After Edit` errorlevel if both NetMail and EchoMail were entered.

##### `after_local_exit` (int)

**Legacy**: `After Local Exit <errorlevel>`

Exit code Maximus uses when a user enters a local (non-network) message.

##### `nodelist_version` (string: `5|6|7|fd`)

**Legacy**: `Nodelist Version <n>`

Selects the nodelist database format Maximus uses.

Legacy formats:

- `5`, `6`, `7`: versioned nodelist formats
- `fd`: FrontDoor format

Practical notes:

- The nodelist files are expected under `net_info_path` (in `config/maximus.toml`), which corresponds to `Path NetInfo` in legacy CTL.
- If you do not specify a nodelist version in legacy CTL, version 6 was considered the default.

##### `fidouser` (string)

**Legacy**: `FidoUser <filespec>`

Path to a `fidouser.lst` style name/address list used for automatic address lookup when composing NetMail.

Legacy note: if you use nodelist version 7 or FrontDoor formats, name lookup is often provided by the nodelist format and this becomes redundant.

##### `message_edit.ask` (table)

**Legacy**: `Message Edit Ask <attr> <priv>`

Controls which NetMail attributes are available for a user to request.

In classic Maximus, "Ask" means Maximus will query the user, or (in full-screen editor mode) allow them to choose the attribute in the header attributes field.

Known attribute keys (as exported):

- `private`
- `crash`
- `fileattach`
- `killsent`
- `hold`
- `filerequest`
- `updaterequest`
- `localattach`

##### `message_edit.assume` (table)

**Legacy**: `Message Edit Assume <attr> <priv>`

Controls which NetMail attributes are automatically applied.

In classic Maximus, "Assume" means Maximus forces the attribute on without prompting.

The same attribute keys are used as in `[message_edit.ask]`.

##### `[[address]]` (array of tables)

**Legacy**: `Address <zone:net/node.point>`

Defines the FidoNet-style addresses owned by your system.

##### `zone` (int)
##### `net` (int)
##### `node` (int)
##### `point` (int)

Fields of an `[[address]]` entry.

Legacy behavior:

- You may specify up to 16 addresses.
- The first address is the PRIMARY address used on outgoing mail.
- Additional addresses are secondary and are not normally used for outgoing mail.

Point systems (legacy guidance):

- In a point configuration, the first address should include your point number.
- Some point setups use a second "fakenet" address provided by a bossnode.

#### Examples

Example: typical node with one address and conservative visibility:

```toml
ctla_priv = 65535
seenby_priv = 100
private_priv = 100

fromfile_priv = 100
unlisted_priv = 100
unlisted_cost = 50

log_echomail = true
echotoss_name = "log/echotoss.log"

after_edit_exit = 11
after_echomail_exit = 12
after_local_exit = 0

nodelist_version = "7"
fidouser = "etc/fidouser.lst"

[message_edit.ask]
crash = 100
hold = 100
fileattach = 100
localattach = 10

[message_edit.assume]
# (empty means no forced attributes)

[[address]]
zone = 1
net = 249
node = 106
point = 0
```

Example: point setup with "real" address and bossnode/fakenet address:

```toml
[[address]]
zone = 1
net = 249
node = 106
point = 2

[[address]]
zone = 1
net = 31623
node = 2
point = 0
```

### `config/general/display_files.toml`

This file contains display file paths used throughout the system.

**Legacy sources**: primarily the `Uses ...` statements in `etc/max.ctl`, plus supporting descriptions in the Maximus manual (`max_mast`).

#### Overview

Maximus has two big classes of "display files":

- Informational screens shown during logon/logoff and common workflows (welcome, application, time warnings, etc.)
- Help and format resources used by built-in menus and editors

In the legacy world, these were defined with `Uses <Thing> <path>` statements.

Practical notes:

- These paths are typically relative to `sys_path`.
- Many systems store these under `etc/misc/` or `etc/help/`.
- These files are usually plain text or MECCA (`.mec`) scripts. Maximus also supports other display formats depending on your setup.

#### Keys

##### `logo` (string)

**Legacy**: `Uses Logo`

Displayed at the very start of the logon sequence.

##### `not_found` (string)

**Legacy**: `Uses NotFound`

Displayed during the new-user flow after the caller enters a name, before Maximus confirms the name.

##### `application` (string)

**Legacy**: `Uses Application`

Displayed to new callers after they enter a name and password. This is commonly where you put rules and expectations.

##### `welcome` (string)

**Legacy**: `Uses Welcome`

Main post-logon welcome screen for established users.

##### `new_user1` (string)

**Legacy**: `Uses NewUser1`

Displayed to new users before Maximus prompts for a password.

##### `new_user2` (string)

**Legacy**: `Uses NewUser2`

Displayed after a new user logs in but before the standard new-user configuration questions.

##### `rookie` (string)

**Legacy**: `Uses Rookie`

Optional "between two and seven calls" screen. If not configured, Maximus typically falls back to `welcome`.

##### `not_configured` (string)

**Legacy**: `Uses NotConfigured`

Displayed when Maximus needs to show a "feature is not configured" message (for example, missing resources).

##### `quote` (string)

**Legacy**: `Uses Quote`

Quote file used by the `[quote]` MECCA token. Legacy guidance: plain ASCII text with a blank line between quotes.

##### `day_limit` (string)

**Legacy**: `Uses DayLimit`

Displayed when a caller has exhausted their daily time limit.

##### `time_warn` (string)

**Legacy**: `Uses TimeWarn`

Displayed right after `welcome` if the caller has already been on earlier that day, usually to warn about remaining time.

##### `too_slow` (string)

**Legacy**: `Uses TooSlow`

Displayed when a caller attempts to log on at a connection speed below your configured minimum.

##### `bye_bye` (string)

**Legacy**: `Uses ByeBye`

Displayed at logoff after the caller selects G)oodbye.

##### `bad_logon` (string)

**Legacy**: `Uses BadLogon`

Displayed when Maximus believes something suspicious has happened during logon (for example, repeated failures).

##### `barricade` (string)

**Legacy**: `Uses Barricade`

Displayed when a caller is prompted for barricade (password/name) access to an area.

##### `no_space` (string)

**Legacy**: `Uses NoSpace`

Displayed when there is not enough disk space to allow an upload.

##### `no_mail` (string)

**Legacy**: `Uses NoMail`

Displayed by the mailchecker when the caller has no waiting mail.

##### `area_not_exist` (string)

**Legacy**: `Uses Cant_Enter_Area`

Optional message shown when a user selects an area that does not exist or they cannot access.

##### `chat_begin` (string)

**Legacy**: `Uses BeginChat`

Displayed when the sysop enters chat with a caller.

##### `chat_end` (string)

**Legacy**: `Uses EndChat`

Displayed when the sysop ends chat.

##### `out_leaving` (string)

**Legacy**: `Uses Leaving`

Displayed when the sysop shells out.

##### `out_return` (string)

**Legacy**: `Uses Returning`

Displayed when the sysop returns from a shell-out.

##### `shell_to_dos` (string)

**Legacy**: `Uses Shell_Leaving`

Displayed when a user is about to be dropped to the external shell.

##### `back_from_dos` (string)

**Legacy**: `Uses Shell_Returning`

Displayed when a user returns from the external shell.

##### `locate` (string)

**Legacy**: `Uses LocateHelp`

Help file shown when a user asks for help during the L)ocate command.

##### `contents` (string)

**Legacy**: `Uses ContentsHelp`

Help file shown when a user asks for help during the C)ontents command.

##### `oped_help` (string)

**Legacy**: `Uses OpedHelp`

Help file shown when a user asks for help in the message editor.

##### `line_ed_help` (string)

**Legacy**: `Uses LineEdHelp`

Help file shown for the line editor.

##### `replace_help` (string)

**Legacy**: `Uses ReplaceHelp`

Help file shown for replace/edit workflows.

##### `inquire_help` (string)

**Legacy**: `Uses InquireHelp`

Help file shown for inquire/lookup workflows.

##### `scan_help` (string)

**Legacy**: `Uses ScanHelp`

Help file shown for scan workflows.

##### `list_help` (string)

**Legacy**: `Uses ListHelp`

Help file shown for list workflows.

##### `header_help` (string)

**Legacy**: `Uses HeaderHelp`

Help file shown while entering the message header (attributes, alias policy, etc.).

##### `entry_help` (string)

**Legacy**: `Uses EntryHelp`

Displayed when entering the message editor, regardless of editor type.

##### `xfer_baud` (string)

**Legacy**: `Uses XferBaud`

Displayed when a user attempts a transfer at a too-slow baud rate.

##### `file_area_list` (string)

**Legacy**: `Uses FileAreas`

Optional flat file containing a file area list, shown when a user presses `?` on the file area change prompt.

##### `file_header` (string)

**Legacy**: `Format FileHeader`

Format string used for the header of the built-in file area list.

##### `file_format` (string)

**Legacy**: `Format FileFormat`

Format string used for each entry line of the built-in file area list.

##### `file_footer` (string)

**Legacy**: `Format FileFooter`

Optional footer format string shown at the end of the file area list.

##### `msg_area_list` (string)

**Legacy**: `Uses MsgAreas`

Optional flat file containing a message area list, shown when a user presses `?` on the message area change prompt.

##### `msg_header` (string)

**Legacy**: `Format MsgHeader`

Format string used for the header of the built-in message area list.

##### `msg_format` (string)

**Legacy**: `Format MsgFormat`

Format string used for each entry line of the built-in message area list.

##### `msg_footer` (string)

**Legacy**: `Format MsgFooter`

Optional footer format string shown at the end of the message area list.

##### `protocol_dump` (string)

**Legacy**: `Uses ProtocolDump`

Optional custom display for the protocol selection screen.

##### `fname_format` (string)

**Legacy**: `Uses Filename_Format`

Mini-essay shown when a user enters an invalid upload filename.

##### `time_format` (string)

**Legacy**: `Format Time`

Controls how Maximus formats times in message headers and other displays.

##### `date_format` (string)

**Legacy**: `Format Date`

Controls how Maximus formats dates in message headers and other displays.

##### `tune` (string)

**Legacy**: `Uses Tunes`

Path to the tune file used for sysop "yell" and related console sounds.

### `config/general/equipment.toml`

This file contains equipment/modem-related settings.

**Legacy sources**: the `Equipment Section` in `etc/max.ctl`, plus modem/port notes in the Maximus manual (`max_mast`).

#### Overview

This file defines how Maximus handles the connection layer: local vs modem output, modem control strings, and hardware/software handshaking.

If you are running locally (for development or maintenance), these settings may not be used much. If you are answering calls, these are critical.

#### Keys

##### `output` (string: `com|local`)

**Legacy**: `Output`

Selects where Maximus sends I/O.

- `com`: use a serial port
- `local`: run on the local console

##### `com_port` (int)

**Legacy**: `Output Com<n>`

Which COM port number to use when `output = "com"`.

##### `baud_maximum` (int)

**Legacy**: `Baud Maximum`

Highest baud rate Maximus should assume the system can support.

##### `busy` (string)

**Legacy**: `Busy`

Command string sent to the modem when a user logs off.

Legacy string escape notes:

- `v` sets DTR low
- `^` sets DTR high
- `~` pauses 1 second
- `` ` `` pauses 1/20th of a second
- `|` ends the command sequence

##### `init` (string)

**Legacy**: `Init`

Command string sent when the internal WFC (Waiting For Caller) subsystem starts.

Legacy tip: for locked-port setups, the init string often includes `&B1`.

##### `ring` (string)

**Legacy**: `Ring`

Substring Maximus expects to see from the modem to detect an incoming call.

##### `answer` (string)

**Legacy**: `Answer`

Command string Maximus sends to answer the call.

##### `connect` (string)

**Legacy**: `Connect`

Substring returned by the modem when a connection is established (link rate typically follows).

##### `carrier_mask` (int)

**Legacy**: `Mask Carrier`

Carrier-detect mask value. If you do not know you need this, leave it at the exported/default value.

##### `handshaking` (array of strings: `xon|cts|dsr`)

**Legacy**: `Mask Handshaking`

Flow control / handshaking settings.

Practical guidance:

- Enable `cts` for high-speed modem operation.
- Enable `xon` if you want callers to be able to pause output with ^S/^Q.

##### `send_break` (bool)

**Legacy**: `Send Break to Clear Buffer`

Legacy feature intended for specific modem models: sends a BREAK to clear the modem buffer.

Do not enable this unless you know your modem supports it.

##### `no_critical` (bool)

**Legacy**: `No Critical Handler`

Disables Maximus' internal critical error handler.

### `config/general/language.toml`

This file contains language selection and language-heap sizing values.

**Legacy sources**: `etc/language.ctl` (selection list) and MAID/manual notes in `max_mast`.

#### Overview

This file defines which language packs are available and which one is the default.

In classic Maximus, language packs came from compiled MAID outputs, and the "heap sizing" values were part of the low-level language system. In NextGen TOML, those sizing values are typically left at `0` unless you have a specific legacy reason to tune them.

#### Keys

##### `lang_file` (array of strings)

**Legacy**: `Language <name>`

List of language roots.

Legacy behavior:

- The first entry is the default language.
- Up to 8 languages were traditionally offered on the selection menu.

##### `max_lang` (int)

Runtime/computed language count.

##### `max_ptrs` (int)
##### `max_heap` (int)
##### `max_glh_ptrs` (int)
##### `max_glh_len` (int)
##### `max_syh_ptrs` (int)
##### `max_syh_len` (int)

Language heap sizing/tuning values.

Practical guidance: if you are not doing advanced language pack work, leave these set to `0`.

### `config/general/reader.toml`

Offline reader settings.

**Legacy sources**: `etc/reader.ctl` and off-line reader sections in `max_mast`.

#### Overview

These settings control QWK/off-line reader packaging: where to find archive definitions, what packet name is used, where temporary work files go, and how many messages can be packed at once.

#### Keys

##### `archivers_ctl` (string)

**Legacy**: `Archivers`

Path to `compress.cfg`, which defines the archiving/unarchiving programs used for QWK bundles.

Legacy note: Maximus and Squish used compatible `compress.cfg` formats, so sysops often shared one copy.

##### `packet_name` (string)

**Legacy**: `Packet Name`

Base filename used for QWK packets.

Legacy guidance: keep it to 8 characters, no spaces, and DOS-safe characters.

##### `work_directory` (string)

**Legacy**: `Work Directory`

Blank work directory for off-line reader operations.

Legacy warning: Maximus creates subdirectories under this path. Do not manually modify its contents while it is in use.

##### `phone` (string)

**Legacy**: `Phone Number`

Phone number embedded into downloaded packets.

Legacy note: some readers expected the format `(xxx) yyy-zzzz`.

##### `max_pack` (int)

**Legacy**: `Max Messages`

Maximum number of messages that can be downloaded in one browse/download session.

### `config/general/colors.toml`

Color theme values.

**Legacy sources**: `etc/colors.ctl` plus additional color constants in language headers (for example `LANG/COLORS.LH`).

#### Overview

This file controls the color scheme used by Maximus in various screens.

Colors are stored as inline tables:

`{ fg = <int>, bg = <int>, blink = <bool> }`

In classic 16-color themes, `fg` and `bg` are typically in the range 0-15.

#### Tables

##### `[menu]`

##### `name` (inline table)

Menu title/name color.

##### `highlight` (inline table)

Menu highlight color.

##### `option` (inline table)

Menu option color.

##### `[file]`

##### `name` (inline table)

File name color.

##### `size` (inline table)

File size color.

##### `date` (inline table)

File date color.

##### `description` (inline table)

File description color.

##### `search_match` (inline table)

Color used to highlight search matches in file listings.

##### `offline` (inline table)

Color used for offline/temporary indicators.

##### `new` (inline table)

Color used to mark new files.

##### `[msg]`

##### `from_label` (inline table)

"From" label color.

##### `from_text` (inline table)

"From" value color.

##### `to_label` (inline table)

"To" label color.

##### `to_text` (inline table)

"To" value color.

##### `subject_label` (inline table)

"Subject" label color.

##### `subject_text` (inline table)

"Subject" value color.

##### `attributes` (inline table)

Message attribute field color.

##### `date` (inline table)

Message date color.

##### `address` (inline table)

Network address color (NetMail/EchoMail headers).

##### `locus` (inline table)

Locus/location field color.

##### `body` (inline table)

Message body text color.

##### `quote` (inline table)

Quoted text color.

##### `kludge` (inline table)

Control/kludge line color.

##### `[fsr]`

##### `msgnum` (inline table)

Message number color.

##### `links` (inline table)

Link/navigation color.

##### `attrib` (inline table)

Attribute display color.

##### `msginfo` (inline table)

Message info panel color.

##### `date` (inline table)

Date/time color.

##### `addr` (inline table)

Address display color.

##### `static` (inline table)

Static UI text color.

##### `border` (inline table)

Border color.

##### `locus` (inline table)

Locus/location color.

### `config/general/protocol.toml`

This file contains external protocol definitions.

**Legacy sources**: `etc/protocol.ctl`.

#### Overview

This file defines the external transfer protocols Maximus offers for uploads and downloads.

Each protocol entry describes:

- What program to run (and whether it is Opus-compatible)
- How Maximus should invoke it for send/receive
- How to interpret success/failure (either via errorlevel or by parsing protocol output)

Legacy operational notes (from `protocol.ctl`):

- If you run a locked baud rate, you may need to replace `%W` tokens in protocol command lines with your locked baud rate.
- If you use an alternate shell (for example 4DOS in legacy setups), you may need to replace `command.com` in protocol command lines.

#### Keys

##### `protoexit` (int)

**Legacy**: `External Protocol Errorlevel`

Errorlevel Maximus uses for `Type Errorlevel` external protocols.

##### `protocol_max_path` (string)
##### `protocol_max_exists` (bool)
##### `protocol_ctl_path` (string)
##### `protocol_ctl_exists` (bool)

Diagnostic metadata describing which protocol sources were found and used.

#### Protocol entries

##### `[[protocol]]` (table)

One protocol definition.

Each `[[protocol]]` table defines one protocol.

##### `index` (int)

Protocol index/slot number.

##### `name` (string)

Display name.

##### `program` (string)

Program name/path (if applicable).

##### `batch` (bool)

Whether this protocol is invoked via a batch/shell wrapper.

##### `exitlevel` (bool)

Whether success/failure is communicated via the program's exit code.

##### `opus` (bool)

Whether this protocol is Opus-compatible.

##### `bi` (bool)

Whether the protocol supports bi-directional transfers.

##### `log_file` (string)

Log file name used by the protocol.

##### `control_file` (string)

Control file name written for the transfer.

##### `download_cmd` (string)
##### `upload_cmd` (string)

Command lines used for sending and receiving.

Legacy note: these often include Maximus substitution tokens (port, baud, paths, node number, etc.). Those tokens are expanded at runtime.

##### `download_string` (string)
##### `upload_string` (string)

Human-readable status strings shown to the user (for example "Send %s" / "Get %s").

##### `download_keyword` (string)
##### `upload_keyword` (string)

Keywords used when parsing the protocol's output/logs.

##### `filename_word` (int)
##### `descript_word` (int)

Field offsets used when parsing protocol output lines.

### `config/areas/msg/areas.toml`

This file contains message areas and message divisions.

**Legacy sources**: `etc/msgarea.ctl` plus message-area documentation in the Maximus manual (`max_mast`).

#### Overview

This file defines your message area tree.

- `[[division]]` entries define optional grouping headers.
- `[[area]]` entries define the actual message bases.

When migrating from CTL, most sysops think of `MsgArea` blocks as `[[area]]` entries.

#### Divisions (`[[division]]`)

##### `[[division]]` (array of tables)

##### `name` (string)

Division name shown to the user.

##### `key` (string)

Stable identifier used by areas to refer to a division.

##### `description` (string)

Longer description used in lists.

##### `acs` (string)

Privilege/ACS required to access the division.

##### `display_file` (string)

Optional display file used for the division.

##### `level` (int)

Legacy ordering / hierarchy value.

#### Areas (`[[area]]`)

##### `[[area]]` (array of tables)

##### `name` (string)

Short area identifier.

##### `description` (string)

Verbose description shown to the caller.

##### `acs` (string)

Privilege/ACS required to access the area. If the caller does not meet it, Maximus behaves as if the area does not exist.

##### `menu` (string)

Optional per-area menu override (legacy `MenuName` behavior).

##### `division` (string)

Division key this area belongs to.

##### `tag` (string)

Echo tag (usually for EchoMail areas). This should match the tag used by your mail processor.

##### `path` (string)

Message base path.

Legacy note: for Squish areas this is a base filename; for `*.MSG` areas this is a directory.

##### `owner` (string)

Owner/moderator name.

##### `origin` (string)

Optional custom origin line override (EchoMail).

##### `attach_path` (string)

Optional per-area override for local attach storage.

##### `barricade` (string)

Barricade configuration (password/name controlled access).

##### `style` (array of strings)

Area style flags.

Common legacy styles include:

- Storage: `Squish`, `MSG`
- Distribution: `Local`, `Net`, `Echo`, `Conf`
- Visibility: `Pub`, `Pvt`
- Other: `Anon`, `ReadOnly`, `Hidden`, `Audit`, `HighBit`, `NoNameKludge`, `RealName`, `Alias`

##### `renum_max` (int)
##### `renum_days` (int)

Optional automated purge/renumber limits.

### `config/areas/file/areas.toml`

This file contains file areas and file divisions.

**Legacy sources**: `etc/filearea.ctl` plus file-area documentation in the Maximus manual (`max_mast`).

#### Overview

This file defines your file area tree.

File areas control:

- Where files are downloaded from
- Where uploads are placed
- Optional FILES.BBS catalog location overrides
- Special handling for slow/CD media

#### Divisions (`[[division]]`)

Same schema and intent as message divisions.

#### Areas (`[[area]]`)

##### `name` (string)
##### `description` (string)
##### `acs` (string)
##### `menu` (string)
##### `division` (string)

Same general meaning as message areas.

##### `download` (string)

Download directory for this area. This directory typically contains both the files and the `FILES.BBS` catalog.

##### `upload` (string)

Upload directory for this area.

Legacy guidance:

- Validation required: point uploads at a sysop-only staging directory.
- No validation required: point uploads at the same directory as `download`.

##### `filelist` (string)

Optional override path for the file listing (FILES.BBS-like) when it does not live in the same directory as the files.

##### `barricade` (string)

Barricade configuration (password/name controlled access).

##### `types` (array of strings)

File-area type flags.

- `Slow`: on slow media
- `Staged`: stage files to `stage_path` before transfer
- `NoNew`: exclude from new-files scanning
- `CD`: shorthand equivalent to `Slow`, `Staged`, and `NoNew`

### `config/menus/*.toml`

These files contain menu definitions.

**Legacy sources**: `etc/menus.ctl`.

#### Overview

Each menu is stored as its own TOML file.

Legacy note: Maximus traditionally requires at least two menu names: `MAIN` and `EDIT`.

#### Menu keys

##### `name` (string)

Menu identifier.

##### `title` (string)

Title line displayed to the user.

##### `header_file` (string)

Optional header display resource.

##### `header_types` (array of strings)

Visibility types applied to the header.

##### `menu_file` (string)

Optional custom menu display file shown instead of a generated menu.

##### `menu_types` (array of strings)

Visibility types applied to the menu file.

##### `menu_length` (int)

When using a custom menu file, this is the number of lines it occupies.

##### `menu_color` (int)

AVATAR color number written before printing the key the user pressed (helps avoid background bleed when hotkey-aborting a menu file).

##### `option_width` (int)

Width used when formatting generated menus.

#### Menu options (`[[option]]`)

##### `[[option]]` (array of tables)

##### `command` (string)

The command to execute (for example `Display_Menu`, `Display_File`, `MEX`, `Msg_Browse`, `Goodbye`).

##### `arguments` (string)

Optional argument string.

Legacy note: CTL commonly represented spaces as underscores, which were converted back to spaces at runtime.

##### `priv_level` (string)

Privilege required to access the option.

Legacy note: may include `/<keys>` gating (for example `Normal/1C`).

##### `description` (string)

User-facing description.

##### `key_poke` (string)

Optional explicit selection key.

##### `modifiers` (array of strings)

Behavior and visibility modifiers.

### `config/security/access_levels.toml`

Access level definitions.

**Legacy sources**: `etc/access.ctl`.

#### Overview

This file defines your privilege classes (Normal, Sysop, etc.) and the policy knobs attached to them (time limits, baud restrictions, ratios, and flags).

These values are used throughout configuration:

- Area access (ACS)
- Menu option access
- Many policy thresholds in `session.toml` and `matrix.toml`

#### Access levels (`[[access_level]]`)

##### `[[access_level]]` (array of tables)

##### `name` (string)

Symbolic name of the class.

##### `level` (int)

Numeric privilege level.

##### `description` (string)

User-visible description.

##### `alias` (string)

Alternate label (legacy compatibility).

##### `key` (string)

Single-character key used by legacy MECCA tokens.

##### `time` (int)

Per-session time limit in minutes.

##### `cume` (int)

Per-day cumulative time limit in minutes.

##### `calls` (int)

Maximum logons per day. Legacy note: `-1` often meant unlimited.

##### `logon_baud` (int)
##### `xfer_baud` (int)

Minimum baud rates for logon and file transfers.

##### `file_limit` (int)

Maximum kilobytes downloadable per day.

##### `file_ratio` (int)

Download-to-upload ratio requirement.

##### `ratio_free` (int)

Amount (in KB) the user may download before ratio enforcement begins.

##### `upload_reward` (int)

Percent time credit returned for uploads.

##### `login_file` (string)

Optional file displayed immediately after logon for users of this class.

##### `flags` (array of strings)

General behavior flags (hide in userlist, no limits, etc.).

##### `mail_flags` (array of strings)

Mail/editor related flags (ShowPvt, Editor, LocalEditor, NetFree, MsgAttrAny, etc.).

##### `user_flags` (int)

Sysop-defined bitfield for MEX scripts.

##### `oldpriv` (int)

Legacy Maximus 2.x compatibility field.

## Runtime-Computed Fields

These are set to 0 in TOML and computed at runtime:

- `max_lang` - Language count
- `max_ptrs`, `max_heap` - Language heap sizes
- `max_glh_ptrs`, `max_glh_len` - Global language heap
- `max_syh_ptrs`, `max_syh_len` - System help heap

## Appendix A: CTL to TOML Mapping (Legacy)

This appendix exists to help sysops migrating from legacy CTL configurations.

### System (maximus.toml)

| CTL Keyword | TOML Field | Type | Default |
|-------------|------------|------|---------|
| `Name` | `system_name` | string | "" |
| `SysOp` | `sysop` | string | "" |
| `Task` | `task_num` | int | 0 |
| `Video` | `video` | string | "IBM" |
| `Path System` | `sys_path` | string | "" |
| `Menu Path` | `menu_path` | string | "etc/menus" |
| `Log File` | `log_file` | string | "log/max.log" |
| `Log Mode` | `log_mode` | string | "Verbose" |
| `MessageData` | `message_data` | string | "marea" |
| `FileData` | `file_data` | string | "farea" |

### Session (general/session.toml)

| CTL Keyword | TOML Field | Type | Default |
|-------------|------------|------|---------|
| `File Date Automatic` | `autodate` | bool | false |
| `File Date <fmt>` | `date_style` | int | 0 |
| `Yell Off` | `yell_enabled` | bool | true |
| `MaxMsgSize` | `max_msgsize` | int | 0 |
| `First Message Area` | `first_message_area` | string | "" |
| `Kill Attach` | `kill_attach` | string | "" |
| `Kill Attach Ask <priv>` | `kill_attach_priv` | int | 0 |
| `Message Edit Ask LocalAttach` | `msg_localattach_priv` | int | 0 |
| `Logon Level` | `logon_priv` | int | 0 |

### Matrix (matrix.toml)

| CTL Keyword | TOML Field | Type | Default |
|-------------|------------|------|---------|
| `Log EchoMail` | `log_echomail` | bool | false |
| `After Edit Exit` | `after_edit_exit` | int | 0 |
| `After EchoMail Exit` | `after_echomail_exit` | int | 0 |
| `Message Show Ctl_A to` | `ctla_priv` | int | 0 |
| `Address` | `addresses[]` | array | [] |
| `Message Edit Ask <attr>` | `message_edit.ask.<attr>` | int | 0 |

## Integration Notes

- **No compilation required** - TOML read directly
- **No PRM files** - Binary formats eliminated
- **Immediate effect** - Changes active on restart
- **TOML is authoritative** - CTL ignored at runtime
- **Portable** - Relative paths work across systems
