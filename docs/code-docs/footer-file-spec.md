# Menu Footer File Feature Specification

## Overview

Add a `footer_file` field to menus, mirroring the existing `header_file`.
The footer is displayed **after** the menu body (`.ans` background + title) and
**before** the lightbar interaction loop, giving MEX scripts and display files a
clean canvas where positioned output (e.g. area info on rows 22–23) will not be
overwritten by subsequent rendering.

### Rendering Order (per Display_Options iteration)

```
1. CLS              (lightbar re-entry only — already implemented)
2. ShowMenuHeader() → runs header_file (.ans / MEX)
3. ShowMenuBody()   → draws menu_file .ans, title at title_location
4. ShowMenuFooter() → NEW: runs footer_file (.ans / MEX)
5. Lightbar loop    → user interaction
```

## TOML Configuration

New keys in each menu `.toml` file, placed immediately after `header_types`:

```toml
footer_file = ":scripts/footerfile"
footer_types = []
```

Semantics match `header_file` / `header_types` exactly:
- Path to a display file, or `:path` prefix for MEX scripts.
- `footer_types` restricts which user help levels see the footer
  (`["Novice"]`, `["Regular"]`, `["Expert"]`, `["RIP"]`).
  Empty array = all levels.

## File-by-File Change List

### 1. `src/max/core/max.h`

**`struct _menu` (packed on-disk struct):**

Add after `word dspfile`:

```c
word footfile;    /* Footer filename, offset into menuheap. */
```

**New flag constants** (after `MFLAG_HF_*` block):

```c
#define MFLAG_FF_NOVICE   0x1000u /* FooterFile for novice users */
#define MFLAG_FF_REGULAR  0x2000u /* FooterFile for regular users */
#define MFLAG_FF_EXPERT   0x4000u /* FooterFile for expert users */
#define MFLAG_FF_ALL      (MFLAG_FF_NOVICE | MFLAG_FF_REGULAR | MFLAG_FF_EXPERT)
```

> Note: No RIP flag for footer — the original flag word has limited bits.
> RIP support can be added later if needed by extending to a dword.

### 2. `src/libs/libmaxcfg/include/libmaxcfg.h`

**`MaxCfgNgMenu` struct** — add after `menu_type_count`:

```c
char  *footer_file;
char **footer_types;
size_t footer_type_count;
```

### 3. `src/libs/libmaxcfg/src/libmaxcfg.c`

Three touch points:

| Function | Change |
|---|---|
| `maxcfg_ng_get_menu()` | Read `footer_file` and `footer_types` from TOML (same pattern as header) |
| `maxcfg_ng_menu_free()` | Free `footer_file` and `footer_types` |
| `maxcfg_ng_write_menu_toml()` | Write `footer_file` and `footer_types` (same pattern as header) |

### 4. `src/apps/maxcfg/include/menu_data.h`

**`MenuDefinition` struct** — add after `header_flags`:

```c
char *footer_file;       /* FooterFile path (NULL if none) */
word  footer_flags;      /* MFLAG_FF_* flags */
```

**New flag defines** — mirror the `MFLAG_FF_*` from `max.h`, or reference them
if the header is shared.

### 5. `src/apps/maxcfg/src/config/menu_parse.c`

| Location | Change |
|---|---|
| `menu_types_from_flags()` / `menu_flags_from_types()` | Extend `is_header` to a tri-state or add `is_footer` parameter, mapping `MFLAG_FF_*`. |
| TOML → `MenuDefinition` load | Read `footer_file` and convert `footer_types` via flags. |
| `MenuDefinition` → TOML save | Write `footer_file` and `footer_types`. |
| Legacy `.ctl` parser (`kw_value` block) | Add `FooterFile` keyword parsing (same pattern as `HeaderFile`). |
| `free_menu_definition()` | Free `footer_file`. |

### 6. `src/apps/maxcfg/src/config/nextgen_export.c`

Mirror the `header_file` / `header_types` export for `footer_file` /
`footer_types` in the `MenuDefinition → MaxCfgNgMenu` conversion.

### 7. `src/apps/maxcfg/src/config/fields.c`

Add two new field definitions after the `HeaderFileTypes` entry:

```c
{
    .keyword = "FooterFile",
    .label = "Footer file",
    .help = "File or MEX script to display after menu body is drawn. "
            "MEX scripts start with ':'. Leave blank for none.",
    .type = FIELD_TEXT,
    .max_length = 120,
    .default_value = "",
    .toggle_options = NULL
},
{
    .keyword = "FooterFileTypes",
    .label = "Footer types",
    .help = "User types that see the FooterFile. Press ENTER or F2 to select. "
            "If none selected, all users see it.",
    .type = FIELD_MULTISELECT,
    .max_length = 80,
    .default_value = "",
    .toggle_options = footerfile_type_options
},
```

Also add the `footerfile_type_options` array (identical to
`headerfile_type_options`).

### 8. `src/apps/maxcfg/src/ui/menu_edit.c`

Extend the properties form from 8 fields to 10:

| Index | Field |
|---|---|
| 0 | Title |
| 1 | HeaderFile |
| 2 | HeaderFileTypes |
| 3 | **FooterFile** *(new)* |
| 4 | **FooterFileTypes** *(new)* |
| 5 | MenuFile |
| 6 | MenuFileTypes |
| 7 | MenuLength |
| 8 | MenuColor |
| 9 | OptionWidth |

Update `menu_load_properties_form()` and `menu_save_properties_form()`
accordingly.

### 9. `src/max/core/max_rmen.c` (runtime loader)

- Extract `footer_file` from `MaxCfgNgMenu` (same as `header_file`).
- Add `footer_file` length to `heap_cap`.
- Call `mnu_heap_add()` for `footer_file` → `menu->m.footfile`.
- Build `MFLAG_FF_*` from `footer_types` and OR into `menu->m.flag`.

### 10. `src/max/core/max_menu.c` (display engine)

**New function `DoFtrFile()`** — mirrors `DoHdrFile()`:

```c
static int near DoFtrFile(byte help, word flag)
{
  return ((help==NOVICE   && (flag & MFLAG_FF_NOVICE))  ||
          (help==REGULAR  && (flag & MFLAG_FF_REGULAR)) ||
          (help==EXPERT   && (flag & MFLAG_FF_EXPERT)));
}
```

**New function `ShowMenuFooter()`** — mirrors `ShowMenuHeader()`:

```c
static void near ShowMenuFooter(PAMENU pam, byte help, int first_time)
{
  char *filename = MNU(*pam, m.footfile);

  if (!*filename || !DoFtrFile(help, pam->m.flag))
    return;

  if (*filename == ':')
  {
    char temp[PATHLEN];
    sprintf(temp, "%s %d", filename + 1, first_time);
    Mex(temp);
  }
  else if (Display_File(DISPLAY_HOTMENU | DISPLAY_MENUHELP, NULL, filename) == -1)
  {
    logit(cantfind, filename);
  }
}
```

**`Display_Options()` call site** — insert after `ShowMenuBody()`:

```c
ShowMenuHeader(&menu, help, first_time);
ShowMenuBody(&menu, help, title, menu_name);
ShowMenuFooter(&menu, help, first_time);   /* NEW */
```

## Migration: headfile.mex → footerfile.mex

The existing `resources/m/headfile.mex` for the FILE menu prints area info at
positioned rows. This becomes the footer:

1. Rename `resources/m/headfile.mex` → `resources/m/footerfile.mex`.
2. Update `resources/m/footerfile.mex`: keep the positioned prints, remove
   any `|CL` since the header/body already handle that.
3. Compile: `cd resources/m && ../../build/bin/mex footerfile.mex`
4. Deploy `.vm` to `build/scripts/` and `resources/install_tree/scripts/`.

## Menu TOML Updates

### `file.toml`

```toml
header_file = ""
footer_file = ":scripts/footerfile"
footer_types = []
menu_file = "display/screens/menu_files"
```

### `main.toml`

```toml
menu_file = "display/screens/menu_main"
```

## ANSI File Deployment

| Source | Destination |
|---|---|
| `resources/ansi/menu_main.ans` | `resources/install_tree/display/screens/menu_main.ans` |
| `resources/ansi/menu_files.ans` | `resources/install_tree/display/screens/menu_files.ans` |
| *(delete)* | `resources/install_tree/display/screens/max_main.ans` |

## Testing

1. Enter FILE menu → header draws `.ans`, body draws lightbar, **footer**
   prints area info at rows 22–23 without overlap.
2. Execute an action (e.g. Area change) → CLS → header → body → footer →
   lightbar. Footer content is clean.
3. Enter MAIN menu → `.ans` background displays, no footer (none configured).
4. `maxcfg` editor → Footer file and Footer types fields appear and
   round-trip correctly on save/load.
