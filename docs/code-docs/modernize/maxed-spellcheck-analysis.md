# MaxEd Spell Checking Feature Analysis

## Overview

Analysis of adding real-time spell checking to MaxEd, the Maximus full-screen message editor. Feature would highlight misspelled words in red and show suggestions when cursor overlaps them.

## Current Architecture

### Key Files

| File | Purpose |
|------|---------|
| `max/maxed.c` | Main editor loop (`MagnEt()` function) |
| `max/maxed.h` | Editor state: `cursor_x`, `cursor_y`, `offset`, `mmsg` |
| `max/maxedp.h` | Function prototypes for editor operations |
| `max/med_scrn.c` | Screen display functions including `Update_Line()` |
| `max/med_add.c` | Character/line addition functions |
| `max/med_del.c` | Deletion functions (backspace, delete char/word/line) |
| `max/max_edit.h` | Shared editor definitions: `screen[]`, `num_lines`, `max_lines` |

### Data Structures

```c
// Text storage (max_edit.h)
byte *screen[MAX_LINES+2];  // Array of line pointers
word num_lines;              // Current number of lines
word max_lines;              // Maximum allowed lines

// Cursor state (maxed.h)
word cursor_x;               // Current row (relative to offset)
word cursor_y;               // Current column
word offset;                 // Scroll offset (first visible line)
byte usrwidth;               // User's screen width
```

### Current Display Logic

The `Update_Line()` function in `med_scrn.c` (lines 93-129) renders text:

```c
void Update_Line(word cx, word cy, word inc, word update_cursor)
{
  // Only update lines which are on-screen
  if (cx > offset && cx < offset+usrlen)
  {
    if (!Mdm_keyp() || cx==cursor_x+offset)
    {
      if (cx-offset != cursor_x || cy != cursor_y)   
        Goto(cx-offset,cy);

      if (cx != max_lines)
      {
        // Simple direct print - no formatting
        Printf("%0.*s", usrwidth-cy, screen[cx]+cy);
      }
      else Printf(end_widget, msg_text_col);

      if (cx >= max_lines || screen[cx]==NULL ||
          (cy+strlen(screen[cx]+cy) <= usrwidth))
        Puts(CLEOL);

      if (update_cursor)
        Goto(cx-offset, cy+inc);
    }
  }
}
```

### Main Editor Loop

Located in `maxed.c` `MagnEt()` function (line 86+):
- Processes keystrokes via `Mdm_getcwcc()`
- Handles scan codes, control keys, regular characters
- Calls `Do_Update()` to refresh screen
- Status line on line 23 (`PROMPT_LINE`)

## Proposed Implementation

### 1. Dictionary System

**New files:**
- `max/spell.c` - Dictionary loading and spell checking
- `max/spell.h` - Spell check API

**Dictionary format:**
- One word per line, sorted alphabetically
- Location: `etc/lang/dict_<language>.txt` (e.g., `dict_english.txt`)
- Binary search for O(log n) lookup, or hash table for O(1)

**API:**
```c
// spell.h
int  spell_init(char *dict_path);     // Load dictionary
void spell_close(void);               // Free dictionary
int  spell_check(char *word);         // Returns 1 if valid, 0 if misspelled
int  spell_suggest(char *word, char **suggestions, int max_suggestions);
```

### 2. Line Spell State

Track misspelled words per line for efficient redraw:

```c
#define MAX_MISSPELL_PER_LINE 10

struct spell_mark {
    word start_col;
    word end_col;
};

struct line_spell_state {
    struct spell_mark marks[MAX_MISSPELL_PER_LINE];
    byte count;
    byte dirty;  // Needs re-check
};

// Global array parallel to screen[]
struct line_spell_state *spell_state[MAX_LINES+2];
```

### 3. Modified Update_Line()

```c
void Update_Line(word cx, word cy, word inc, word update_cursor)
{
  if (cx > offset && cx < offset+usrlen)
  {
    // ... existing checks ...
    
    if (spell_enabled && spell_state[cx])
    {
      // Render with highlighting
      byte *line = screen[cx] + cy;
      word col = cy;
      
      for (int i = 0; i < spell_state[cx]->count; i++)
      {
        struct spell_mark *m = &spell_state[cx]->marks[i];
        
        // Print text before misspelled word
        if (col < m->start_col)
          Printf("%.*s", m->start_col - col, line + (col - cy));
        
        // Print misspelled word in red
        Printf("\x1b[31m%.*s\x1b[0m", 
               m->end_col - m->start_col,
               line + (m->start_col - cy));
        
        col = m->end_col;
      }
      
      // Print remainder
      Printf("%s", line + (col - cy));
    }
    else
    {
      // Original behavior
      Printf("%0.*s", usrwidth-cy, screen[cx]+cy);
    }
    
    // ... rest of function ...
  }
}
```

### 4. Cursor Overlap Detection

In main loop after keystroke processing:

```c
// Check if cursor is on a misspelled word
int check_cursor_on_misspell(void)
{
  struct line_spell_state *ls = spell_state[offset + cursor_x];
  if (!ls) return -1;
  
  for (int i = 0; i < ls->count; i++)
  {
    if (cursor_y >= ls->marks[i].start_col && 
        cursor_y < ls->marks[i].end_col)
      return i;  // Return mark index
  }
  return -1;
}

// Show suggestions on status line
void show_spell_suggestions(int mark_idx)
{
  char *suggestions[5];
  char word[64];
  
  // Extract word from screen
  struct spell_mark *m = &spell_state[offset+cursor_x]->marks[mark_idx];
  strncpy(word, screen[offset+cursor_x] + m->start_col, 
          m->end_col - m->start_col);
  
  int n = spell_suggest(word, suggestions, 5);
  
  // Display on status line
  Goto(PROMPT_LINE, 1);
  Printf("Suggestions: ");
  for (int i = 0; i < n; i++)
    Printf("%s ", suggestions[i]);
  Puts(CLEOL);
}
```

### 5. Configuration

Add to `max.ctl`:
```
Spell Check         On
Spell Dictionary    etc/lang/dict_english.txt
```

Or user preference in `usr.bits2`:
```c
#define BITS2_SPELLCHECK  0x8000
```

## Complexity Assessment

| Component | Lines of Code | Difficulty | Notes |
|-----------|---------------|------------|-------|
| Dictionary loader | ~80 | Low | File I/O, malloc array |
| Binary search lookup | ~30 | Low | Standard algorithm |
| Word parser | ~40 | Low | isalpha() state machine |
| Spell state tracking | ~60 | Low | Parallel array to screen[] |
| Update_Line() mods | ~50 | Medium | ANSI codes, cursor math |
| Cursor detection | ~30 | Low | Range check |
| Suggestion display | ~40 | Low | Status line printf |
| Config/toggle | ~30 | Low | PRM flag or user bit |
| **Total** | **~360** | **Medium** | |

## Risk Assessment

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| ANSI codes break cursor positioning | Medium | Track display vs logical position |
| Performance on long messages | Low | Only check visible lines |
| Memory usage (dictionary) | Low | ~100KB typical, acceptable |
| Non-ASCII/international chars | Medium | Skip non-alpha or use locale |
| Breaking existing functionality | Low | Feature toggle, isolated changes |

## Files to Modify

1. **New files:**
   - `max/spell.c` - Core spell checking
   - `max/spell.h` - API header

2. **Modifications:**
   - `max/med_scrn.c` - `Update_Line()` for highlighting
   - `max/maxed.c` - Main loop for cursor detection, init/cleanup
   - `max/maxed.h` - Add spell state globals
   - `max/Makefile` - Add spell.o

3. **Optional:**
   - `silt/s_system.c` - Config parsing for spell dictionary path
   - `resources/install_tree/etc/lang/dict_english.txt` - Default dictionary

## Conclusion

**Feasibility: HIGH**

The MaxEd architecture is clean and well-suited for this enhancement:
- Text is stored line-by-line in `screen[]` array
- Display is centralized in `Update_Line()`
- Status line mechanism already exists
- Similar pattern to existing language heap system

**Estimated effort: 1-2 days**

The feature can be implemented incrementally:
1. Dictionary loading and spell_check() function
2. Line parsing and spell state tracking
3. Update_Line() highlighting
4. Cursor detection and suggestions

Each step can be tested independently before moving to the next.
