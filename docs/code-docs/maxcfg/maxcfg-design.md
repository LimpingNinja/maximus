# MAXCFG Design Document

## Overview

MAXCFG is an ncurses-based configuration editor for Maximus BBS, inspired by classic
BBS configuration utilities like KBBS Config and RemoteAccess Setup. It provides a
safe, user-friendly interface for editing CTL files, language files, and other
configuration without requiring knowledge of file formats or CP437 encoding.

**Author:** Kevin Morgan (Limping Ninja)  
**License:** GPL-2.0-or-later  
**Target Platform:** Unix/Linux/macOS (ncurses)

---

## Configuration Files Inventory

| File | Lines | Purpose | Editable Sections |
|------|-------|---------|-------------------|
| `max.ctl` | 1755 | Main config | System, Equipment, Matrix, Session |
| `access.ctl` | 436 | Security levels | Access definitions |
| `colors.ctl` | 68 | Display colors | Status, popup, WFC colors |
| `compress.cfg` | 176 | Archivers | ZIP, ARC, LHA, etc. |
| `menus.ctl` | 450 | Menu system | Menu definitions |
| `msgarea.ctl` | 263 | Message areas | Area configs |
| `filearea.ctl` | 131 | File areas | Area configs |
| `language.ctl` | 20 | Languages | Language list |
| `protocol.ctl` | 155 | Transfer protocols | External protocols |
| `reader.ctl` | 53 | QWK settings | Offline reader |
| `events00.bbs` | 71 | Events/schedules | Timed events |
| `baduser.bbs` | - | Banned users | Name list |
| `names.max` | - | Reserved names | Protected names |
| `tunes.bbs` | - | Page tunes | Tune definitions |
| `bulletin.mec` | - | Bulletins | MECCA bulletin list |

### max.ctl Sections

The main configuration file contains these major sections:

1. **System Section** - BBS name, sysop, paths, logging, user file, password encryption
2. **Equipment Section** - Video mode, COM ports, modem settings (DOS legacy)
3. **Matrix and EchoMail Section** - FidoNet addresses, netmail config, origins
4. **Session Section** - Login settings, time limits, display files, file/message options

---

## Menu Hierarchy

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ MAXCFG v1.0              Maximus Configuration Editor              F1=Help  │
├──────────────────────────────────────────────────────────────────────────────┤
│  Setup   Content   Conferences   Files   Users   Tools                       │
└──────────────────────────────────────────────────────────────────────────────┘

Setup
├── Global
│   ├── BBS and Sysop Information    [max.ctl: Name, Sysop]
│   ├── System Paths                  [max.ctl: Path System/Misc/Lang/Temp/IPC]
│   ├── Logging Options               [max.ctl: Log File, Log Level]
│   ├── Global Toggles                [max.ctl: Snoop, Video, Password Encryption]
│   ├── Login Settings                [max.ctl: Min Baud, TTY Baud]
│   ├── New User Defaults             [max.ctl: Session Section new user items]
│   ├── Display Files                 [max.ctl: Uses Logo/Welcome/Goodbye/etc]
│   └── Default Colors                [colors.ctl]

Note: maxcfg currently still exposes the TOML key `maximus.swap` in its Global Toggles UI. SWAP support has been removed from the runtime and related tools, so this toggle currently has no effect.
├── Security Levels                   [access.ctl - list with CRUD]
├── Archivers                         [compress.cfg - list with CRUD]
├── Protocols                         [protocol.ctl - list with CRUD]
├── Events                            [events00.bbs - schedule editor]
├── Languages                         [language.ctl + lang/*.mad strings]
└── Matrix/Echomail
    ├── Network Addresses             [max.ctl: Address statements]
    ├── Netmail Settings              [max.ctl: Matrix Section]
    └── Origin Lines                  [max.ctl: Origin]

Content
├── Menus                             [menus.ctl - visual editor]
├── Display Files                     [etc/misc/*.mec - file browser]
├── Help Files                        [etc/help/*.mec - file browser]
├── Bulletins                         [etc/misc/bulletin.mec - editor]
└── Reader Settings                   [reader.ctl - QWK config]

Conferences                           [msgarea.ctl]
└── [List of message areas with Add/Edit/Delete]
    ├── Name, Description
    ├── ACS (Access Level)
    ├── Style (Squish/MSG, Local/Echo/Net, Pub/Pvt)
    ├── Path
    └── Origin (for echo areas)

Files                                 [filearea.ctl]
└── [List of file areas with Add/Edit/Delete]
    ├── Name, Description
    ├── ACS (Access Level)
    ├── Download Path
    ├── Upload Path
    └── FileList Path

Users
├── User Editor                       [etc/user.bbs - binary]
├── Bad Users                         [baduser.bbs]
└── Reserved Names                    [names.max]

Tools
├── Recompile All (silt, maid, mecca)
├── Compile Config (silt max.ctl)
├── Compile Language (maid)
├── View Log
└── System Information
```

---

## UI Components

### 1. Top Menu Bar
- Horizontal menu bar at line 2 (below title)
- Left/Right arrow navigation between top-level items
- Enter or Down arrow opens dropdown
- Hotkey support (first letter highlighted)

### 2. Dropdown Menus
- Vertical list appearing below menu bar item
- Up/Down arrow navigation
- Enter selects item
- ESC closes menu
- Right arrow on item with submenu opens submenu

### 3. Submenus (Cascading)
- Appear to the right of parent menu item
- Same navigation as dropdown
- Can nest multiple levels deep

### 4. Pop-up Windows
- Centered modal windows for forms/editors
- Title bar with window name
- Form fields with labels and editable values
- Bottom status bar with key hints

### 5. Pop-up Dialogs
- Small confirmation/selection dialogs
- Used for:
  - Save confirmation on ESC
  - Delete confirmation
  - File selection lists
  - Multi-select option lists

### 6. List Views
- Scrollable list of items (areas, levels, etc.)
- Insert/Delete/Edit operations
- Search/filter capability

---

## Interaction Model

### Field Types

| Type | Behavior | Trigger |
|------|----------|---------|
| Text | Direct edit in field | Enter or start typing |
| Number | Direct edit, validate numeric | Enter or start typing |
| Toggle | Cycle through Yes/No | Enter or Space |
| Select | Pop-up list of options | Enter or F2 |
| File Select | Pop-up file browser | Enter or F2 |
| Path | Text edit + validation | Enter |
| Color | Color picker popup | Enter or F2 |

### File Select Behavior
When a field is marked as "file select" (e.g., `*.bbs` type):
- Enter or F2 opens a popup listing all matching files in the configured directory
- Files shown without extension
- Highlight bar selection
- Enter confirms, ESC cancels

### Save/Exit Dialog
When ESC is pressed with unsaved changes:

```
┌─[■]─────────────────────────┐
│  Save and Exit              │  <- Highlighted
│  Abort without saving       │
│  Return to edit screen      │
├─────────────────────────────┤
│       ENTER=Select          │
└─────────────────────────────┘
```

---

## Color Scheme

Based on KBBS/RemoteAccess style:

### Primary Colors (ncurses color pairs)

| Element | Foreground | Background | Pair # |
|---------|------------|------------|--------|
| Screen background | Cyan | Cyan | 1 |
| Title bar | White | Blue | 2 |
| Menu bar | Yellow | Blue | 3 |
| Menu bar highlight | Black | Cyan | 4 |
| Dropdown menu | Yellow | Blue | 5 |
| Dropdown highlight | White | Blue (reverse) | 6 |
| Form background | - | Black | 7 |
| Form labels | Cyan | Black | 8 |
| Form values | Yellow | Black | 9 |
| Form highlight | Black | White | 10 |
| Status bar | Yellow | Cyan | 11 |
| Dialog border | Cyan | Black | 12 |
| Dialog text | Yellow | Black | 13 |
| Error text | Red | Black | 14 |
| Help text | White | Blue | 15 |

### Box Drawing Characters

Use ncurses ACS characters for portability:
- `ACS_ULCORNER`, `ACS_URCORNER` - corners
- `ACS_LLCORNER`, `ACS_LRCORNER`
- `ACS_HLINE`, `ACS_VLINE` - lines
- `ACS_TTEE`, `ACS_BTEE`, `ACS_LTEE`, `ACS_RTEE` - tees

---

## Key Bindings

### Global Keys
| Key | Action |
|-----|--------|
| F1 | Context help |
| F10 | Save and exit current form |
| ESC | Back/Cancel (with save prompt if dirty) |
| Tab | Next field |
| Shift+Tab | Previous field |
| Ctrl+Q | Quit program |

### Menu Navigation
| Key | Action |
|-----|--------|
| Left/Right | Move between top menu items |
| Up/Down | Move within dropdown |
| Enter | Select/Open |
| ESC | Close menu |
| First letter | Jump to item |

### List Views
| Key | Action |
|-----|--------|
| Insert/+ | Add new item |
| Delete/- | Delete selected item |
| Enter | Edit selected item |
| / | Search |
| Home/End | First/Last item |
| PgUp/PgDn | Page scroll |

### Form Editing
| Key | Action |
|-----|--------|
| Enter | Edit field / Open selector |
| F2 | Open picker (for select/file fields) |
| Space | Toggle (for boolean fields) |
| Ctrl+Y | Clear field |

---

## Technical Requirements

### CP437 Handling
- Read files as binary, interpret as CP437
- Display using Unicode equivalents or raw bytes
- Write back preserving original encoding
- Special handling for box-drawing characters in .mad files

### Parser Requirements
- CTL files: keyword-value with section markers
- CFG files: similar but with `;` comments
- BBS files: semicolon comments, event syntax
- MAD files: language string definitions

### Backup System
- Create `.bak` before first modification
- Timestamped backups in `~/.maxcfg/backups/`
- Option to restore from backup

### Validation
- Path existence checks
- Syntax validation before save
- Required field checks
- Privilege level references

### Compile Integration
- Detect when recompile is needed
- Option to auto-compile on save
- Show compilation errors in popup

---

## File Structure

```
maxcfg/
├── Makefile
├── src/
│   ├── main.c              # Entry point, argument parsing
│   ├── ui/
│   │   ├── ui.h            # UI component declarations
│   │   ├── screen.c        # Screen setup, colors, refresh
│   │   ├── menubar.c       # Top menu bar
│   │   ├── dropdown.c      # Dropdown/submenu handling
│   │   ├── dialog.c        # Pop-up dialogs
│   │   ├── form.c          # Form editor
│   │   ├── listview.c      # Scrollable list widget
│   │   └── widgets.c       # Text fields, toggles, etc.
│   ├── config/
│   │   ├── config.h        # Config structures
│   │   ├── parser.c        # CTL/CFG file parser
│   │   ├── writer.c        # CTL/CFG file writer
│   │   ├── max_ctl.c       # max.ctl specific handling
│   │   ├── access_ctl.c    # access.ctl handling
│   │   ├── areas.c         # msgarea.ctl, filearea.ctl
│   │   └── language.c      # language.ctl, .mad files
│   ├── editors/
│   │   ├── setup.c         # Setup menu editors
│   │   ├── content.c       # Content menu editors
│   │   ├── conferences.c   # Message area editor
│   │   ├── files.c         # File area editor
│   │   ├── users.c         # User management
│   │   └── tools.c         # Tools menu
│   └── util/
│       ├── cp437.c         # CP437 encoding helpers
│       ├── backup.c        # Backup management
│       └── compile.c       # Compiler invocation
└── include/
    └── maxcfg.h            # Global definitions
```

---

## Implementation Phases

### Phase 1: Core UI Framework
**Goal:** Establish the visual foundation matching KBBS/RemoteAccess style

1. **Screen initialization**
   - ncurses setup with color pairs
   - Screen layout (title, menu bar, work area, status)
   - Box-drawing character support

2. **Menu bar component**
   - Horizontal menu rendering
   - Left/right navigation
   - Hotkey highlighting

3. **Dropdown menu component**
   - Vertical menu rendering
   - Up/down navigation
   - Enter/ESC handling
   - Submenu cascade support

4. **Basic dialog component**
   - Centered popup window
   - Save/Abort/Return dialog
   - Simple message boxes

**Deliverable:** Working menu shell with Setup menu visible, no actual config loading

---

### Phase 2: Form System & Widgets
**Goal:** Create reusable form editing components

1. **Form container**
   - Field layout engine
   - Tab navigation between fields
   - Dirty state tracking
   - Save prompt on exit

2. **Widget types**
   - Text input field
   - Number input field
   - Boolean toggle
   - Static label
   - Selection field (triggers popup)

3. **Pop-up selector**
   - List selection dialog
   - File browser dialog
   - Multi-value picklist

4. **Validation framework**
   - Required field check
   - Type validation
   - Custom validators

**Deliverable:** Form editor with all widget types working (hardcoded test data)

---

### Phase 3: CTL File Parser
**Goal:** Read and write max.ctl and related files

1. **Tokenizer**
   - Handle `%` and `;` comments
   - Keyword-value extraction
   - Section boundary detection
   - Preserve formatting/comments on write

2. **max.ctl parser**
   - System Section fields
   - Session Section fields
   - Equipment Section (basic)
   - Matrix Section (basic)

3. **Supporting file parsers**
   - colors.ctl
   - language.ctl
   - access.ctl structure

4. **Writer with preservation**
   - Maintain original file structure
   - Only modify changed values
   - Preserve comments and whitespace

**Deliverable:** Can load, edit, and save max.ctl System Section

---

### Phase 4: Setup Menu Implementation
**Goal:** Full Setup menu functionality

1. **Global submenu**
   - BBS and Sysop Information form
   - System Paths form
   - Logging Options form
   - Global Toggles form
   - Login Settings form
   - New User Defaults form
   - Display Files form
   - Default Colors form (colors.ctl)

2. **Security Levels (access.ctl)**
   - List view of levels
   - Add/Edit/Delete operations
   - Level editor form

3. **Archivers (compress.cfg)**
   - List view of archivers
   - Archiver editor form

4. **Events (events00.bbs)**
   - Event list view
   - Event editor

5. **Languages**
   - Language list
   - String editor for .mad files

6. **Matrix/Echomail**
   - Network addresses
   - Basic netmail config

**Deliverable:** Complete Setup menu with all submenus functional

---

### Phase 5: Content Menu
**Goal:** Display file and menu management

1. **Menus editor (menus.ctl)**
   - Menu list view
   - Menu item editor
   - Command/ACS configuration

2. **Display Files browser**
   - File list from etc/misc/
   - Preview capability
   - Launch external editor

3. **Help Files browser**
   - File list from etc/help/

4. **Bulletins editor**
   - Bulletin list parsing
   - Bulletin entry editor

5. **Reader Settings (reader.ctl)**
   - QWK configuration form

**Deliverable:** Complete Content menu

---

### Phase 6: Area Editors
**Goal:** Message and file area management

1. **Conferences (msgarea.ctl)**
   - Area list view with all fields
   - Area editor form
   - Style selector (Squish/MSG, Local/Echo/Net)
   - Path validation

2. **Files (filearea.ctl)**
   - Area list view
   - Area editor form
   - Download/Upload path handling

**Deliverable:** Full area management

---

### Phase 7: Users & Tools
**Goal:** User management and utility functions

1. **User Editor**
   - Binary user.bbs parsing
   - User list view
   - User detail editor
   - Search functionality

2. **Bad Users / Reserved Names**
   - Simple text list editors

3. **Tools menu**
   - Recompile All (invoke silt, maid, mecca)
   - Individual compile options
   - Log viewer
   - System info display

**Deliverable:** Complete application

---

### Phase 8: Polish & Testing
**Goal:** Production readiness

1. **Help system**
   - Context-sensitive F1 help
   - Help text for each form

2. **Error handling**
   - Graceful error recovery
   - User-friendly error messages

3. **Configuration**
   - Command-line options (config path, etc.)
   - Preferences file

4. **Testing**
   - Test with real Maximus installations
   - Edge case handling
   - Performance optimization

5. **Documentation**
   - Man page
   - User guide

**Deliverable:** Release-ready maxcfg v1.0

---

## Dependencies

- **ncurses** - Terminal UI library
- **Standard C library** - File I/O, string handling

No external dependencies beyond ncurses to keep the tool lightweight and portable.

---

## Build Integration

Add to main Maximus Makefile:

```makefile
maxcfg:
	$(MAKE) -C maxcfg

maxcfg_install: maxcfg
	$(MAKE) -C maxcfg install

clean::
	$(MAKE) -C maxcfg clean
```

Install to `$PREFIX/bin/maxcfg`

---

## Notes

- All file operations should preserve CP437 encoding
- Backup original files before any modification
- Support both relative and absolute paths in config
- Consider read-only mode for viewing without risk
- Future: Support for remote editing via SSH

---

## Implementation Status

This section tracks actual implementation progress, showing which features are
complete, what files they read/write, and the current source files.

### Current Source Files

```
maxcfg/
├── Makefile
├── include/
│   ├── ui.h                 # UI component declarations, color pairs
│   └── fields.h             # Form field definitions, toggle types
├── src/
│   ├── main.c               # Entry, SIGWINCH handler, main loop
│   ├── config/
│   │   └── fields.c         # Field definitions for all forms
│   └── ui/
│       ├── screen.c         # Screen init, colors, work area
│       ├── menubar.c        # Menu bar, menu definitions, actions
│       ├── dropdown.c       # Dropdown/submenu navigation
│       ├── dialog.c         # Save/Abort dialog, confirm dialogs
│       ├── form.c           # Form editor with text/toggle/select/file/multiselect fields
│       ├── filepicker.c     # File picker (F2/Enter on FIELD_FILE)
│       ├── listpicker.c     # Scrollable single-select picker (INS/DEL/Enter)
│       ├── treeview.c       # Tree view editor (message/file areas)
│       ├── checkpicker.c    # Multi-select checkbox picker (FIELD_MULTISELECT)
│       ├── colorpicker.c    # Color grid picker (16x8 fg/bg)
│       └── colorform.c      # Color editing forms, category picker
```

### Menu Implementation Status

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ MAXCFG v1.0              Maximus Configuration Editor              F1=Help  │
├──────────────────────────────────────────────────────────────────────────────┤
│  Setup   Content   Conferences   Files   Users   Tools                       │
└──────────────────────────────────────────────────────────────────────────────┘

Setup
├── Global
│   ├── BBS and Sysop Information    ✓ CODED + WIRED
│   │   ├── BBS Name                 [max.ctl: Name] TEXT
│   │   ├── SysOp Name               [max.ctl: SysOp] TEXT
│   │   ├── Alias System             [max.ctl: Alias System] TOGGLE Yes/No
│   │   ├── Ask for Alias            [max.ctl: Ask Alias] TOGGLE Yes/No
│   │   ├── Single Word Names        [max.ctl: Single Word Names] TOGGLE Yes/No
│   │   ├── Check ANSI               [max.ctl: Check ANSI] TOGGLE Yes/No
│   │   └── Check RIP                [max.ctl: Check RIP] TOGGLE Yes/No
│   │
│   ├── System Paths                  ✓ CODED + WIRED
│   │   ├── System Path              [max.ctl: Path System] PATH
│   │   ├── Misc Path                [max.ctl: Path Misc] PATH
│   │   ├── Language Path            [max.ctl: Path Language] PATH
│   │   ├── Temp Path                [max.ctl: Path Temp] PATH
│   │   ├── IPC Path                 [max.ctl: Path IPC] PATH
│   │   ├── User File                [max.ctl: File Password] PATH
│   │   ├── Access File              [max.ctl: File Access] PATH
│   │   └── Log File                 [max.ctl: Log File] PATH
│   │
│   ├── Logging Options               ✓ CODED + WIRED
│   │   ├── Log File                  [max.ctl: Log File] PATH
│   │   ├── Log Level                 [max.ctl: Log Mode] TOGGLE Terse/Verbose/Trace
│   │   └── Callers Log               [max.ctl: Uses Callers] PATH
│   │
│   ├── Global Toggles                ✓ CODED + WIRED
│   │   ├── Snoop                     [max.ctl: Snoop] TOGGLE Yes/No
│   │   ├── Password Encryption       [max.ctl: No Password] TOGGLE Yes/No (inverted)
│   │   ├── Watchdog/Reboot           [max.ctl: Reboot] TOGGLE Yes/No
│   │   ├── Swap to Disk              [max.ctl: Swap] TOGGLE Yes/No
│   │   ├── Local Keyboard Timeout    [max.ctl: Local Input] TOGGLE Yes/No
│   │   └── Status Line               [max.ctl: StatusLine] TOGGLE Yes/No
│   │
│   ├── Login Settings                ✓ CODED + WIRED
│   │   ├── New User Access Level     [max.ctl: Logon Level] TEXT
│   │   ├── Logon Time Limit          [max.ctl: Logon TimeLimit] TEXT (minutes)
│   │   ├── Minimum Logon Baud        [max.ctl: Min Logon Baud] TEXT
│   │   ├── Minimum Graphics Baud     [max.ctl: Min NonTTY Baud] TEXT
│   │   ├── Minimum RIP Baud          [max.ctl: Min RIP Baud] TEXT
│   │   ├── Input Timeout             [max.ctl: Input Timeout] TEXT (minutes)
│   │   ├── Check ANSI on Login       [max.ctl: Check ANSI] TOGGLE Yes/No
│   │   └── Check RIP on Login        [max.ctl: Check RIP] TOGGLE Yes/No
│   │
│   ├── New User Defaults             ✓ CODED + WIRED
│   │   ├── Ask for Phone Number      [max.ctl: Ask Phone] TOGGLE Yes/No
│   │   ├── Ask for Alias/Real Name   [max.ctl: Ask Alias] TOGGLE Yes/No
│   │   ├── Alias System              [max.ctl: Alias System] TOGGLE Yes/No
│   │   ├── Single Word Names         [max.ctl: Single] TOGGLE Yes/No
│   │   ├── No Real Name Required     [max.ctl: No RealName] TOGGLE Yes/No
│   │   ├── First Menu                [max.ctl: First Menu] TEXT
│   │   ├── First File Area           [max.ctl: First File Area] TEXT
│   │   └── First Message Area        [max.ctl: First Message Area] TEXT
│   │
│   └── Default Colors                ✓ CODED + WIRED
│       ├── [Category Picker]         Dialog with 4 options
│       ├── Menu Colors               ✓ [colors.lh defines]
│       │   ├── Menu Name            [MENU_NAME] FG only
│       │   ├── Menu Highlight       [MENU_HIGHLIGHT] FG only
│       │   └── Menu Option          [MENU_OPTION] FG only
│       ├── File Colors               ✓ [colors.lh defines]
│       │   ├── File Name            [FILE_NAME] FG only
│       │   ├── File Size            [FILE_SIZE] FG only
│       │   ├── File Date            [FILE_DATE] FG only
│       │   ├── File Desc            [FILE_DESC] FG only
│       │   ├── File Offline         [FILE_OFFLINE] FG only
│       │   ├── File New             [FILE_NEW] FG only
│       │   └── Area Tag             [AREA_TAG] FG only
│       ├── Message Colors            ✓ [colors.lh defines] - Scrollable (13 items)
│       │   ├── Msg From             [MSG_FROM] FG only
│       │   ├── Msg To               [MSG_TO] FG only
│       │   ├── Msg Subject          [MSG_SUBJECT] FG only
│       │   ├── Msg Date             [MSG_DATE] FG only
│       │   ├── Msg Body             [MSG_BODY] FG only
│       │   ├── Msg Quote            [MSG_QUOTE] FG only
│       │   ├── Msg Kludge           [MSG_KLUDGE] FG only
│       │   ├── Msg Origin           [MSG_ORIGIN] FG only
│       │   ├── Msg Tearline         [MSG_TEARLINE] FG only
│       │   ├── Msg Seenby           [MSG_SEENBY] FG only
│       │   ├── Msg Header           [MSG_HEADER] FG only
│       │   ├── Msg Area Tag         [MSG_AREA_TAG] FG only
│       │   └── Msg Highlight        [MSG_HIGHLIGHT] FG only
│       └── Reader Colors             ✓ [colors.lh defines] - FG+BG
│           ├── FSR Normal           [FSR_NORM] FG+BG
│           ├── FSR High             [FSR_HIGH] FG+BG
│           ├── FSR Quote            [FSR_QUOTE] FG+BG
│           ├── FSR Kludge           [FSR_KLUDGE] FG+BG
│           ├── FSR Status           [FSR_STATFORE/FSR_STATBACK] FG+BG
│           ├── FSR Warning          [FSR_WARNFORE/FSR_WARNBACK] FG+BG
│           ├── FSR Msgn             [FSR_MSGNFORE/FSR_MSGNBACK] FG+BG
│           ├── FSR From             [FSR_FROMFORE/FSR_FROMBACK] FG+BG
│           └── FSR To               [FSR_TOFORE/FSR_TOBACK] FG+BG
│
├── Security Levels                   ✓ CODED + SAMPLE [access.ctl]
├── Archivers                         ○ PLACEHOLDER [compress.cfg]
├── Protocols                         ○ PLACEHOLDER [protocol.ctl]
├── Events                            ○ PLACEHOLDER [events00.bbs]
├── Languages                         ○ PLACEHOLDER [language.ctl]
└── Matrix/Echomail
    ├── Network Addresses             ○ PLACEHOLDER [max.ctl: Address]
    ├── Netmail Settings              ○ PLACEHOLDER [max.ctl: Matrix Section]
    └── Origin Lines                  ○ PLACEHOLDER [max.ctl: Origin]

Content
├── Menus                             ○ PLACEHOLDER [menus.ctl]
├── Display Files                     ✓ CODED + WIRED [max.ctl: Uses *]
│   ├── 40 display file paths with MEX toggle (F4)
│   ├── File picker with *.bbs/*.vm filter
│   └── Space for manual path editing
├── Help Files                        ○ PLACEHOLDER [etc/help/*.mec]
├── Bulletins                         ○ PLACEHOLDER [etc/misc/bulletin.mec]
└── Reader Settings                   ○ PLACEHOLDER [reader.ctl]

Conferences                           ✓ CODED + SAMPLE [msgarea.ctl]
└── [Tree view of message areas/divisions]

Files                                 ✓ CODED + SAMPLE [filearea.ctl]
└── [Tree view of file areas/divisions]

Users                                 ○ ALL PLACEHOLDER
├── User Editor                       [etc/user.bbs - binary]
├── Bad Users                         [baduser.bbs]
└── Reserved Names                    [names.max]

Tools                                 ○ ALL PLACEHOLDER
├── Recompile All
├── Compile Config
├── Compile Language
├── View Log
└── System Information

### Legend

- ✓ **CODED** - Feature is implemented and functional in the UI
- ✓ **SCAFFOLDED** - Menu wiring/UI skeleton exists but behavior is incomplete
- ✓ **DESIGNED** - Intended behavior/fields documented but not implemented
- ○ **PLACEHOLDER** - Menu item exists but shows "Not implemented" dialog

- **NO DATA** - No data model hooked up (often defaults only)
- **SAMPLE** - Uses hardcoded/sample data (no CTL/PRM parsing for that feature yet)
- **WIRED** - Reads/writes real data (e.g., PRM heap) and survives re-entry
- **TEXT** - Free-form text input field
- **PATH** - File/directory path field
- **TOGGLE** - Yes/No or multi-option toggle with ↑↓ indicator
- **SELECT** - Single-select field (Enter opens picker; Space allows manual override)
- **FILE** - File picker field (F2/Enter opens browser; Space allows manual edit)
- **MULTISELECT** - Multi-select checkbox picker field
- **FG only** - Color picker for foreground color (0-15)
- **FG+BG** - Color picker grid for foreground and background

### Picker Behavior Notes

- **Security level / access selection (ACS-style fields)**
  - **Enter**: open the access level picker
  - **F2**: open the access level picker
  - **Space**: manually type an override value (inline editor overlays the field)
- **Flags fields (System/Mail flags)**
  - Multi-select checkbox picker with **Space** to toggle items
- **Menu name fields (Custom menu / Replace menu)**
  - File picker for `*.mnu` under `m/` (menu loading/parsing is future work)

### Data Files Referenced

| Config File | Read | Write | Status |
|-------------|------|-------|--------|
| `max.ctl` | ○ | ○ | Forms defined, no parser yet |
| `colors.ctl` | ○ | ○ | Forms defined, no parser yet |
| `lang/colors.lh` | ✓ | ○ | Color defines extracted for form |
| `access.ctl` | ○ | ○ | Security Levels UI implemented (sample data; no parser yet) |
| `compress.cfg` | ○ | ○ | Not started |
| `protocol.ctl` | ○ | ○ | Not started |
| `menus.ctl` | ○ | ○ | Not started |
| `msgarea.ctl` | ○ | ○ | Message Areas UI implemented (sample data; no parser yet) |
| `filearea.ctl` | ○ | ○ | File Areas UI implemented (sample data; no parser yet) |

### UI Components Implemented

| Component | File | Status |
|-----------|------|--------|
| Screen/Colors | `screen.c` | ✓ Complete |
| Menu Bar | `menubar.c` | ✓ Complete |
| Dropdown Menus | `dropdown.c` | ✓ Complete (2 levels) |
| Submenus | `dropdown.c` | ✓ Works for 1 nested level |
| Form Editor | `form.c` | ✓ Text + Toggle + Select + File + Multi-select |
| Save/Abort Dialog | `dialog.c` | ✓ Complete |
| Color Picker Grid | `colorpicker.c` | ✓ 16x8 FG/BG grid |
| Color Form | `colorform.c` | ✓ With scrolling |
| List Picker | `listpicker.c` | ✓ Single-select picker with INS/DEL/Enter |
| Tree View | `treeview.c` | ✓ Message/File area editor (sample data) |
| File Picker | `filepicker.c` | ✓ Browse files (used for display files + menus) |
| Checkbox Picker | `checkpicker.c` | ✓ Multi-select (used for access flags) |

### Next Implementation Priorities

1. **CTL File Parser** - Read max.ctl into memory, populate form values
2. **CTL File Writer** - Save form values back to max.ctl preserving comments
3. **Logging Options Form** - Add fields for log settings
4. **Login Settings Form** - Min baud, TTY settings
5. **List View Component** - For Security Levels, Archivers, Areas
