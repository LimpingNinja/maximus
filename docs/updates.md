# Maximus Updates and Migration Guide

## Modern Maximus (2025+)

This fork of Maximus has been modernized for current operating systems (macOS, Linux, FreeBSD) and includes significant architectural changes from the original DOS/OS2 versions.

### MaxUI (Display/UI Intrinsics)

- Struct-based field editing and prompt intrinsics (`ui_edit_field`, `ui_prompt_field`) are the preferred interface.
- New form runner (`ui_form_run`) provides multi-field forms with 2D navigation and required-field validation.
- UI key handling is centralized so common ANSI escape sequences behave consistently across widgets.
- Forward-delete is supported in the bounded field editor (Delete key / `ESC[3~`) including `format_mask` fields.
- MaxUI docs were updated to document behavior and to call out deferred/unsupported advanced ESC parsing.

## Key Changes from Legacy Maximus 3.0

### Configuration System

**Legacy (pre-2025):**
- Configuration stored in `.ctl` text files
- Compiled to binary `.prm` and `.dat` files via `silt` utility
- Required recompilation after every config change

**Modern (2025+):**
- Configuration stored in TOML files under `config/`
- No compilation step required
- Direct runtime parsing of TOML
- Legacy `.ctl` files can be converted via `maxcfg --export-nextgen`

### Removed Components

- **SILT** - Configuration compiler (replaced by TOML runtime parsing)
- **SM** - OS/2 session manager (obsolete)
- **PRM files** - Binary compiled config format (replaced by TOML)

### Migration from Legacy 3.0 Installations

If you have an existing Maximus 3.0 installation with `.ctl` configuration files:

```bash
# Convert CTL to TOML
bin/maxcfg --export-nextgen /path/to/etc/max.ctl

# Output will be written to config/ directory
```

### What Still Works

- All BBS functionality (telnet, local console)
- MEX scripting language
- MECCA display file compiler
- FidoNet message processing (Squish)
- File transfers
- Multi-node operation via MAXTEL

### Documentation

- **BUILD.md** - Modern build instructions
- **README.md** - Quick start guide
- **docs/maxtel.md** - MAXTEL telnet supervisor documentation
- **Legacy manuals** - Historic documentation (see banner notes in those files)

For detailed technical documentation on the TOML configuration schema, see `docs/maximus-ngconfig-docs.md`.
