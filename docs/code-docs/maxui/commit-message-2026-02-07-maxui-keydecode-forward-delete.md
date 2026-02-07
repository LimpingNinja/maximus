# Commit Message

## ui: MaxUI forms + key handling, MAXTEL term caps, and repo cleanup

### MaxUI (C display layer)

- Add MaxUI primitives and widgets:
  - Bounded field editor (`ui_edit_field`) + prompt wrapper (`ui_prompt_field`) using style structs
  - Lightbar menus (classic + positioned/multi-column) and `ui_select_prompt`
  - Struct-based form runner (`ui_form_run`) with 2D navigation, editing, and required-field validation
- Centralize UI key decoding via shared `ui_read_key()` helper
  - Used by field editor, form runner, and lightbar/select prompt loops
- Add forward-delete support (Delete / `ESC[3~`) in `ui_edit_field`
  - Works for both plain and `format_mask` fields
- Align cursor behavior across widgets
  - Cursor hidden during navigation
  - Cursor shown while actively editing a field

### MEX integration + tests

- Expose MaxUI to MEX (intrinsics + ABI structs):
  - Register UI intrinsics in the MEX runtime
  - Add MEX ABI structs for MaxUI styles and form fields
  - Add MEX headers for UI (`resources/m/maxui.mh`)
- Expand interactive MEX tests (`resources/m/uitest.mex`)
  - Prompt start modes
  - `format_mask` edit/cancel flows
  - Form runner demo

### Documentation

- Update MaxUI canonical API doc (`docs/code-docs/maxui/new_ui_mex_functions.md`)
  - Document Delete behavior and centralized key decoding
  - Call out deferred “missing functionality” (robust ESC parsing/timeouts)
- Add release notes entries:
  - `CHANGES.md`
  - `docs/updates.md`

### MAXTEL terminal sizing + Maximus integration

- Improve MAXTEL terminal capability detection:
  - Negotiate and parse NAWS (width/height) and TTYPE
  - Fallback ANSI dimension probing when NAWS is absent
  - Write detected Width/Height into the node term-caps file
- Apply terminal caps in Maximus core for remote sessions
  - Introduce/export `Apply_Term_Caps()` and call it during login/session setup
  - Preserve negotiated `usr.width`/`usr.len` across guest/account resets

### Runtime fixes / misc

- Fix RLE display decoding path (expand repeats instead of emitting raw marker)
- Add debugging/trace improvements for `ansi2bbs` repeat compression edge cases
- Minor whitespace/format cleanups in MEX headers/scripts

### Repo hygiene

- Stop tracking built executables and generated parser outputs in the source tree
  - Update `.gitignore` accordingly
  - Remove tracked binaries under `src/**` and generated `mex_tab.c/h`
