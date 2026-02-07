# MAXCFG Menu Editor Design Document

**SPDX-License-Identifier:** GPL-2.0-or-later  
**Author:** Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja  
**Copyright:** (C) 2025 Kevin Morgan  
**Date:** December 15, 2025

---

## Overview

This document outlines the design for the menu editing functionality in MAXCFG. The menu editor will allow SysOps to configure the BBS menu system defined in `menus.ctl` through an intuitive ncurses interface, following the same patterns established for message/file area editing.

## Menu System Architecture

### File Structure

- **Source:** `menus.ctl` - Human-readable menu definitions
- **Compiled:** `*.mnu` - Binary menu files compiled by SILT
- **Editing Target:** Work directly with `menus.ctl` (not compiled .mnu files)

### Data Structures (from max.h)

```c
struct _menu {
    word header;        // Header type (HEADER_MESSAGE, HEADER_FILE, etc.)
    word num_options;   // Total number of menu options
    word menu_length;   // Lines the custom MenuFile takes
    word opt_width;     // Option width override (default 20)
    sword hot_colour;   // Color for hotkey display (-1 = none)
    word title;         // Offset to title string
    word headfile;      // Offset to header file path
    word dspfile;       // Offset to custom menu display file
    word flag;          // MFLAG_* flags
};

struct _opt {
    option type;        // Command type (from option.h enum)
    zstr priv;          // Privilege level string
    dword rsvd;         // Reserved bit-field locks
    word flag;          // OFLAG_* flags
    zstr name;          // Menu option description
    zstr keypoke;       // Auto-keypoke string
    zstr arg;           // Command argument
    byte areatype;      // Area type restriction
    byte fill1;         // Reserved
    byte rsvd2[8];      // Reserved
};
```

### Menu Flags (MFLAG_*)

**MenuFile Display Flags:**
- `MFLAG_MF_NOVICE` (0x0001) - Show MenuFile to novice users
- `MFLAG_MF_REGULAR` (0x0002) - Show MenuFile to regular users
- `MFLAG_MF_EXPERT` (0x0004) - Show MenuFile to expert users
- `MFLAG_MF_RIP` (0x0400) - Show MenuFile to RIP users
- `MFLAG_MF_ALL` - Show to all help levels

**HeaderFile Display Flags:**
- `MFLAG_HF_NOVICE` (0x0001) - Show HeaderFile to novice users
- `MFLAG_HF_REGULAR` (0x0002) - Show HeaderFile to regular users
- `MFLAG_HF_EXPERT` (0x0004) - Show HeaderFile to expert users
- `MFLAG_HF_RIP` (0x0400) - Show HeaderFile to RIP users
- `MFLAG_HF_ALL` - Show to all help levels

**Other Flags:**
- `MFLAG_SILENT` - Silent menu header

### Option Flags (OFLAG_*)

- `OFLAG_NODSP` (0x0001) - Don't display on menu (hidden command)
- `OFLAG_CTL` (0x0002) - Produce .CTL file for external command
- `OFLAG_NOCLS` (0x0004) - Don't clear screen for Display_Menu
- `OFLAG_THEN` (0x0008) - Execute only if last IF was true
- `OFLAG_ELSE` (0x0010) - Execute only if last IF was false
- `OFLAG_ULOCAL` (0x0020) - Local users only
- `OFLAG_UREMOTE` (0x0040) - Remote users only
- `OFLAG_REREAD` (0x0080) - Re-read LASTUSER.BBS after external command
- `OFLAG_STAY` (0x0100) - Don't perform menu cleanup
- `OFLAG_RIP` (0x0200) - RIP callers only
- `OFLAG_NORIP` (0x0400) - Non-RIP callers only

### Menu Commands (from option.h)

Commands are organized into functional blocks:

**MISC_BLOCK (100-199):**
- `display_menu` - Chain to another menu
- `display_file` - Display a .BBS file
- `message`, `file`, `other` - Legacy area types
- `o_press_enter` - Wait for keypress
- `key_poke` - Auto-enter keys
- `clear_stacked` - Clear stacked input
- `o_if` - Conditional execution
- `o_menupath` - Change menu path
- `o_cls` - Clear screen
- `mex` - Execute MEX script
- `link_menu` - Link to menu
- `o_return` - Return from menu

**XTERN_BLOCK (200-299):**
- `xtern_erlvl` - External program via errorlevel
- `xtern_dos` - External DOS/shell command
- `xtern_run` - Run external program
- `xtern_chain` - Chain to external program
- `xtern_concur` - Concurrent external program

**MAIN_BLOCK (300-399):**
- `goodbye` - Log off
- `statistics` - Show statistics
- `o_yell` - Page sysop
- `userlist` - Show user list
- `o_version` - Show version
- `user_editor` - Edit users
- `leave_comment` - Leave comment
- `climax` - CLIMAX door

**MSG_BLOCK (400-499):**
- `msg_area` - Change message area
- `read_next`, `read_previous` - Navigate messages
- `enter_message` - Post message
- `msg_reply` - Reply to message
- `msg_list` - List messages
- `msg_browse` - Browse messages
- `msg_kill` - Delete message
- `msg_hurl` - Move message
- `msg_forward` - Forward message
- `msg_upload` - Upload message
- `msg_tag` - Tag areas
- `msg_change` - Change current message
- And more...

**FILE_BLOCK (500-599):**
- `file_area` - Change file area
- `locate` - Locate files
- `file_titles` - Show file titles
- `file_type` - View text file
- `upload`, `download` - Transfer files
- `file_kill` - Delete file
- `file_hurl` - Move file
- `contents` - View archive contents
- `file_tag` - Tag files
- `newfiles` - New files scan
- And more...

**CHANGE_BLOCK (600-699):**
- `chg_city`, `chg_password`, `chg_help` - User settings
- `chg_video`, `chg_editor`, `chg_protocol` - Preferences
- And more...

**EDIT_BLOCK (700-799):**
- `edit_save`, `edit_abort` - Editor commands
- `edit_list`, `edit_edit`, `edit_insert` - Edit operations
- And more...

**CHAT_BLOCK (800-899):**
- `who_is_on` - Show online users
- `o_page` - Page user
- `o_chat_cb` - CB chat
- `chat_toggle` - Toggle chat status
- `o_chat_pvt` - Private chat

---

## menus.ctl Format

### Menu Definition Block

```
Menu <name>
    [Global Options]
    [Menu Options]
End Menu
```

### Global Menu Options

```
Title <display_title>
    - Menu title shown to users (can include tokens like %t for time)

HeaderFile <filespec> [type...]
    - File/MEX to display when entering menu
    - Optional types: Novice, Regular, Expert, RIP
    - MEX scripts start with ":"

MenuFile <filespec> [type...]
    - Custom .BBS file instead of auto-generated menu
    - Optional types: Novice, Regular, Expert, RIP

MenuLength <lines>
    - Number of lines the MenuFile occupies

MenuColor <attr>
    - AVATAR color code for hotkey display

OptionWidth <width>
    - Width for each menu option (default 20)
```

### Menu Option Format

```
[Modifiers] <Command> [Arguments] <PrivLevel> "<Description>" ["<Hotkey>"]
```

**Modifiers (optional, space-separated):**
- `NoDsp` - Hidden command
- `NoCLS` - Don't clear screen (Display_Menu only)
- `NoRIP` - Non-RIP users only
- `RIP` - RIP users only
- `Local` - Local users only (Style Local areas)
- `Matrix` - Network users only (Style Net areas)
- `Echo` - Echo areas only (Style Echo)
- `Conf` - Conference areas only (Style Conf)
- `UsrLocal` - Local connection only
- `UsrRemote` - Remote connection only
- `ReRead` - Re-read user file after external command

**Arguments:**
- Spaces replaced with underscores
- Will be converted back to spaces at runtime

**Privilege Level:**
- Standard: `Demoted`, `Transient`, `Normal`, `Sysop`, `Hidden`
- Custom: `<level>/<keys>` (e.g., `Normal/1C` = Normal with keys 1 and C)

**Description:**
- Quoted string
- First character becomes hotkey unless explicit hotkey provided

**Hotkey (optional):**
- Second quoted string
- Can be special keys like "`46" for Alt-C

### Example Menu

```
Menu MAIN
    Title           MAIN (%t mins)
    MenuFile        etc/rip/max_main RIP
    
    Display_Menu    MESSAGE                 Demoted "Message areas"
    Display_Menu    FILE                    Demoted "File areas"
    Goodbye                               Transient "Goodbye (log off)"
    MEX             m/stats                 Demoted "Statistics"
  NoDsp Press_Enter                         Demoted "S"
    Userlist                                Demoted "UserList"
    Display_File    etc/help/main           Demoted "?help"
End Menu
```

---

## UI Design

### Menu Hierarchy

```
Messages > Setup Menus
    ├─ Menu List (Picklist)
    │   ├─ MAIN
    │   ├─ MESSAGE
    │   ├─ FILE
    │   ├─ CHANGE
    │   ├─ EDIT
    │   ├─ CHAT
    │   ├─ READER
    │   ├─ SYSOP
    │   └─ MEX
    │
    └─ Menu Editor (for selected menu)
        ├─ Menu Properties Form
        │   ├─ Menu Name (read-only, shown in title)
        │   ├─ Title
        │   ├─ HeaderFile
        │   ├─ HeaderFile Types (checkboxes)
        │   ├─ MenuFile
        │   ├─ MenuFile Types (checkboxes)
        │   ├─ MenuLength
        │   ├─ MenuColor
        │   └─ OptionWidth
        │
        └─ Options List (Picklist/Table)
            ├─ [Edit Option]
            ├─ [Insert Option]
            ├─ [Delete Option]
            └─ [Move Up/Down]
```

### Menu List View

**Picklist showing all menus:**
- Name (e.g., "MAIN", "MESSAGE")
- Title (e.g., "MAIN (%t mins)")
- Option count (e.g., "12 options")

**Actions:**
- ENTER/F5 - Edit menu
- INS - Add new menu
- DEL - Delete menu
- F10 - Save and exit

### Menu Properties Form

**Fields:**
1. **Title** - Text input (e.g., "MAIN (%t mins)")
2. **HeaderFile** - File picker or text input
3. **HeaderFile Types** - Checkboxes: [ ] Novice [ ] Regular [ ] Expert [ ] RIP
4. **MenuFile** - File picker or text input
5. **MenuFile Types** - Checkboxes: [ ] Novice [ ] Regular [ ] Expert [ ] RIP
6. **MenuLength** - Numeric input (lines)
7. **MenuColor** - Numeric input (AVATAR color code) or color picker
8. **OptionWidth** - Numeric input (6-80, default 20)

### Option Editor Form

**Fields:**
1. **Command** - Dropdown with all available commands (grouped by block)
   - When `Display_Menu` selected: Show menu picker popup
2. **Arguments** - Text input (spaces shown as underscores)
3. **Privilege Level** - Dropdown or text input
   - Standard: Demoted, Transient, Normal, Sysop, Hidden
   - Custom: Allow manual entry (e.g., "Normal/1C")
4. **Description** - Text input (first char becomes hotkey)
5. **Explicit Hotkey** - Text input (optional, overrides first char)
6. **Modifiers** - Checkboxes:
   - [ ] NoDsp (Hidden)
   - [ ] NoCLS (No clear screen)
   - [ ] NoRIP (Non-RIP only)
   - [ ] RIP (RIP only)
   - [ ] Local (Local areas)
   - [ ] Matrix (Network areas)
   - [ ] Echo (Echo areas)
   - [ ] Conf (Conference areas)
   - [ ] UsrLocal (Local connection)
   - [ ] UsrRemote (Remote connection)
   - [ ] ReRead (Re-read user file)

---

## Implementation Plan

### Phase 1: Data Structures

**Create menu data structures in maxcfg:**

```c
typedef struct {
    char *name;              // Menu name (e.g., "MAIN")
    char *title;             // Display title
    char *header_file;       // HeaderFile path
    word header_flags;       // MFLAG_HF_* flags
    char *menu_file;         // MenuFile path
    word menu_flags;         // MFLAG_MF_* flags
    int menu_length;         // MenuFile line count
    int menu_color;          // AVATAR color (-1 = none)
    int opt_width;           // Option width (default 20)
    
    MenuOption *options;     // Array of menu options
    int option_count;        // Number of options
} MenuDefinition;

typedef struct {
    char *command;           // Command name (e.g., "Display_Menu")
    char *arguments;         // Command arguments
    char *priv_level;        // Privilege level string
    char *description;       // Menu option description
    char *hotkey;            // Explicit hotkey (optional)
    word flags;              // OFLAG_* flags
} MenuOption;
```

### Phase 2: Parser

**Reuse existing CTL parser infrastructure:**

1. Extend `ctl_parse.c` with menu-specific parsing
2. Parse `Menu <name>` blocks
3. Parse global menu options (Title, MenuFile, etc.)
4. Parse menu option lines with modifiers
5. Build in-memory MenuDefinition structures

**Key parsing functions:**
```c
MenuDefinition **parse_menus_ctl(const char *sys_path, int *menu_count, char *err, size_t err_len);
bool save_menus_ctl(const char *sys_path, MenuDefinition **menus, int menu_count, char *err, size_t err_len);
void free_menu_definitions(MenuDefinition **menus, int menu_count);
```

### Phase 3: UI Components

**Reuse existing UI components:**

1. **Menu List** - Use `listpicker.c` (like area picklists)
2. **Menu Properties Form** - Use `form.c` with field definitions
3. **Option List** - Use `listpicker.c` with custom formatting
4. **Option Editor Form** - Use `form.c` with field definitions
5. **Command Picker** - Use `listpicker.c` with grouped commands
6. **Menu Picker** - Use `listpicker.c` for Display_Menu target selection

### Phase 4: Field Definitions

**Create field definitions in `fields.c`:**

```c
extern FieldDef menu_properties_fields[];
extern int menu_properties_field_count;

extern FieldDef menu_option_fields[];
extern int menu_option_field_count;
```

### Phase 5: Menu Actions

**Add to menubar.c:**

```c
static void action_menus_list(void);      // Show menu list
static void action_menu_edit(MenuDefinition *menu);  // Edit menu properties
static void action_menu_options(MenuDefinition *menu);  // Edit menu options
```

---

## Reusable Components

### From Existing Code

1. **CTL Parser** - `ctl_parse.c` for basic parsing
2. **Form System** - `form.c` for property editing
3. **Listpicker** - `listpicker.c` for menu/option lists
4. **File Picker** - `filepicker.c` for HeaderFile/MenuFile selection
5. **Dialog System** - `dialog.c` for confirmations
6. **PRM Access** - `prm_data.c` for system path access

### New Components Needed

1. **Menu Parser** - Extend CTL parser for menu-specific syntax
2. **Command Validator** - Validate command names against option.h enum
3. **Menu Reference Validator** - Validate Display_Menu targets exist
4. **Privilege Parser** - Parse "Normal/1C" style privilege strings
5. **Modifier Parser** - Parse NoDsp, RIP, etc. modifiers

---

## Special Handling

### Display_Menu Command

When user selects `Display_Menu` as the command:
1. Show popup with list of all available menus
2. Allow selection or manual entry
3. Validate menu exists when saving

### MEX Command

When user selects `MEX` as the command:
1. Show file picker for .mex files in m/ directory
2. Strip .mex extension in saved value
3. Validate MEX file exists

### External Commands (Xtern_*)

When user selects external command:
1. Allow manual entry of command line
2. Show ReRead modifier checkbox
3. Warn about security implications

### Privilege Levels

Support both standard and custom formats:
- Standard: Dropdown with Demoted, Transient, Normal, Sysop, Hidden
- Custom: Allow manual entry for key-based restrictions (e.g., "Normal/1C")

### Hotkeys

- Auto-generate from first character of description
- Allow explicit override in second field
- Support special keys like "`46" for Alt-C

---

## Validation Rules

### Menu Level

1. Menu name must be unique
2. Menu name must not be empty
3. MAIN and EDIT menus are required
4. Title should not be empty
5. MenuFile path should exist if specified
6. HeaderFile path should exist if specified
7. MenuLength must be positive if MenuFile specified
8. MenuColor must be valid AVATAR code (0-255) or -1
9. OptionWidth must be 6-80

### Option Level

1. Command must be valid (from option.h enum)
2. Display_Menu must have valid menu name as argument
3. MEX must have valid .mex file as argument
4. Description must not be empty
5. First character of description must be unique within menu (unless NoDsp)
6. Explicit hotkey must be unique within menu
7. Privilege level must be valid format
8. Conflicting modifiers (RIP + NoRIP) should warn

---

## Testing Strategy

1. **Parse existing menus.ctl** - Ensure parser handles all syntax
2. **Round-trip test** - Parse → Save → Parse should be identical
3. **Menu navigation** - Test Display_Menu references
4. **Privilege validation** - Test various privilege formats
5. **Modifier combinations** - Test all modifier flags
6. **Edge cases** - Empty menus, special characters, long descriptions
7. **Integration** - Test with actual SILT compilation

---

## Future Enhancements

1. **Visual menu preview** - Show how menu will appear to users
2. **Command help** - Context-sensitive help for each command
3. **Drag-and-drop** - Reorder options visually
4. **Menu templates** - Pre-built menu configurations
5. **Duplicate detection** - Warn about duplicate hotkeys
6. **Menu flow diagram** - Visualize menu navigation
7. **Privilege calculator** - Helper for key-based restrictions
8. **Import/Export** - Share menu configurations

---

## Questions Answered

1. **Work directly with CTL?** YES - Edit menus.ctl directly, not compiled .mnu files
2. **Reuse existing components?** YES - Use form.c, listpicker.c, ctl_parse.c, etc.
3. **Validation approach?** Parse command names against option.h, validate menu references
4. **Display_Menu handling?** Show popup menu picker when command selected, allow manual entry

---

## References

- `menus.ctl` - Example menu definitions
- `src/max/option.h` - Command enum and SILT table
- `src/max/max.h` - Menu and option structures
- `src/utils/util/s_menu.c` - SILT menu parser
- `build/docs/max_mast.txt` - Menu control file reference (Section 18.8)
