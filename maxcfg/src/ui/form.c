/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * form.c - Form editor for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "maxcfg.h"
#include "ui.h"
#include "fields.h"

/* Form state */
typedef struct {
    char **values;              /* Current field values */
    int field_count;            /* Number of fields */
    int selected;               /* Currently selected field */
    bool dirty;                 /* Has data been modified */
} FormState;

/* Window geometry */
typedef struct {
    int win_x, win_y;           /* Window position */
    int win_w, win_h;           /* Window size */
    int help_y;                 /* Y position of help separator */
    int help_h;                 /* Height of help area */
    int field_x, field_y;       /* Start of field area */
    int label_w;                /* Label column width */
    int value_w;                /* Value column width */
    int max_visible;            /* Max fields visible before scrolling */
} FormGeometry;

#define MIN_VALUE_WIDTH    30
#define HELP_LINES         4
#define PADDING            2
#define MAX_VISIBLE_FIELDS 16  /* Max rows before scrolling */
#define PAIR_LABEL_W       16  /* Label width for paired columns */
#define PAIR_VALUE_W       10  /* Value width for paired columns */

/* Check if form has any paired fields */
static bool has_paired_fields(const FieldDef *fields, int field_count)
{
    for (int i = 0; i < field_count; i++) {
        if (fields[i].pair_with_next) return true;
    }
    return false;
}

/* Count display rows (paired fields share a row, separators take a row) */
static int count_display_rows(const FieldDef *fields, int field_count)
{
    int rows = 0;
    for (int i = 0; i < field_count; i++) {
        rows++;
        if (fields[i].pair_with_next && i + 1 < field_count) {
            i++;  /* Skip the paired field, same row */
        }
    }
    return rows;
}

/* Calculate form geometry based on content */
static FormGeometry calc_geometry(const char *title, const FieldDef *fields, int field_count)
{
    FormGeometry g;
    bool has_pairs = has_paired_fields(fields, field_count);
    
    /* Calculate max label width */
    g.label_w = 0;
    for (int i = 0; i < field_count; i++) {
        if (fields[i].type == FIELD_SEPARATOR) continue;
        int len = strlen(fields[i].label);
        if (len > g.label_w) g.label_w = len;
    }
    
    /* Calculate max value width needed */
    int max_val_len = MIN_VALUE_WIDTH;
    for (int i = 0; i < field_count; i++) {
        if (fields[i].type == FIELD_SEPARATOR) continue;
        if (fields[i].max_length > max_val_len) {
            max_val_len = fields[i].max_length;
        }
    }
    g.value_w = max_val_len + 2;  /* Add padding for highlight */
    if (g.value_w > 50) g.value_w = 50;  /* Cap at reasonable width */
    
    /* Calculate window dimensions */
    int title_len = title ? strlen(title) : 0;
    int content_w = g.label_w + 2 + g.value_w;  /* label + ": " + value */
    
    /* If we have paired fields, make room for two columns */
    if (has_pairs) {
        int pair_w = (PAIR_LABEL_W + 2 + PAIR_VALUE_W) * 2 + 4;  /* Two columns + gap */
        if (pair_w > content_w) content_w = pair_w;
    }
    
    if (title_len + 4 > content_w) content_w = title_len + 4;
    
    /* Count actual display rows */
    int display_rows = count_display_rows(fields, field_count);
    
    /* Initial cap on visible rows */
    g.max_visible = display_rows;
    if (g.max_visible > MAX_VISIBLE_FIELDS) {
        g.max_visible = MAX_VISIBLE_FIELDS;
    }
    
    /* Window layout:
     * - 1 line: top border with title
     * - 1 line: blank space above fields
     * - max_visible lines: field content area
     * - 1 line: blank space below fields
     * - 1 line: help separator
     * - HELP_LINES: help text
     * - 1 line: bottom border
     * Total: max_visible + HELP_LINES + 5
     */
    g.win_w = content_w + PADDING * 2 + 2;  /* Add padding and borders */
    g.win_h = g.max_visible + HELP_LINES + 5;
    
    /* Cap to screen size */
    if (g.win_w > COLS - 4) g.win_w = COLS - 4;
    if (g.win_h > LINES - 4) g.win_h = LINES - 4;
    
    /* Recalculate max_visible based on actual window height */
    /* Available space = win_h - 5 (borders/spacing) - HELP_LINES */
    int available_rows = g.win_h - HELP_LINES - 5;
    if (available_rows < 1) available_rows = 1;
    if (g.max_visible > available_rows) {
        g.max_visible = available_rows;
    }
    
    /* Center window */
    g.win_x = (COLS - g.win_w) / 2;
    g.win_y = (LINES - g.win_h) / 2;
    
    /* Calculate internal positions */
    g.field_x = g.win_x + PADDING;
    g.field_y = g.win_y + 2;  /* 1 for border + 1 space above fields */
    g.help_y = g.win_y + g.win_h - HELP_LINES - 2;  /* 1 for border + 1 for separator line */
    g.help_h = HELP_LINES;
    
    return g;
}

/* Draw the form window border and title */
static void draw_form_window(const FormGeometry *g, const char *title)
{
    int x = g->win_x;
    int y = g->win_y;
    int w = g->win_w;
    int h = g->win_h;
    
    /* Draw border */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Top border with title */
    mvaddch(y, x, ACS_ULCORNER);
    addch(ACS_HLINE);
    addch(' ');
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Title in grey (white on black) */
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("%s", title);
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(' ');
    for (int i = strlen(title) + 4; i < w - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_URCORNER);
    
    /* Sides */
    for (int i = 1; i < h - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + w - 1, ACS_VLINE);
    }
    
    /* Bottom border */
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    for (int i = 1; i < w - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_LRCORNER);
    
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Fill interior with form background */
    attron(COLOR_PAIR(CP_FORM_BG));
    for (int i = 1; i < h - 1; i++) {
        mvhline(y + i, x + 1, ' ', w - 2);
    }
    attroff(COLOR_PAIR(CP_FORM_BG));
}

/* Draw the help separator line */
static void draw_help_separator(const FormGeometry *g, const FieldDef *field, bool is_disabled, bool is_mex)
{
    int y = g->help_y;
    int x = g->win_x;
    int w = g->win_w;
    
    /* Draw horizontal line separator */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    mvaddch(y, x, ACS_LTEE);
    addch(ACS_HLINE);
    addch(' ');
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* "Help" in grey */
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("Help");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    printw(" ");
    addch(ACS_HLINE);
    printw(" ");
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* "F2" in yellow */
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("F2");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    
    /* Show context-specific text based on field type */
    if (field->type == FIELD_FILE) {
        /* "=Picker" in grey */
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("=Picker");
        attroff(COLOR_PAIR(CP_MENU_BAR));
        
        /* Add F3=Enable/Disable for fields that can be disabled */
        if (field->can_disable) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            printw(" ");
            addch(ACS_HLINE);
            printw(" ");
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
            attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
            printw("F3");
            attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
            attron(COLOR_PAIR(CP_MENU_BAR));
            printw(is_disabled ? "=On" : "=Off");
            attroff(COLOR_PAIR(CP_MENU_BAR));
        }
        
        /* Add Space=Edit */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        printw(" ");
        addch(ACS_HLINE);
        printw(" ");
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        printw("Space");
        attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("=Edit");
        attroff(COLOR_PAIR(CP_MENU_BAR));
        
        /* Add F4=MEX/BBS for fields that support MEX */
        if (field->supports_mex) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            printw(" ");
            addch(ACS_HLINE);
            printw(" ");
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
            attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
            printw("F4");
            attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
            attron(COLOR_PAIR(CP_MENU_BAR));
            printw(is_mex ? "=Use a BBS file" : "=Use a MEX program");
            attroff(COLOR_PAIR(CP_MENU_BAR));
        }
    } else {
        /* "=Modify/PickList" in grey */
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("=Modify/PickList");
        attroff(COLOR_PAIR(CP_MENU_BAR));
    }
    
    /* Add "Space to Toggle" for toggle fields */
    if (field->type == FIELD_TOGGLE) {
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        printw(" ");
        addch(ACS_HLINE);
        printw(" ");
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        printw("Space");
        attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("=Toggle");
        attroff(COLOR_PAIR(CP_MENU_BAR));
    }
    
    /* Get current cursor position and fill rest with line */
    int cur_x = getcurx(stdscr);
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(' ');
    
    /* Rest of line */
    for (int i = cur_x + 2; i < x + w - 1; i++) {
        addch(ACS_HLINE);
    }
    
    mvaddch(y, x + w - 1, ACS_RTEE);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
}

/* Draw help text with word wrapping */
static void draw_help_text(const FormGeometry *g, const char *help_text)
{
    if (!help_text) return;
    
    int start_y = g->help_y + 1;
    int start_x = g->win_x + 2;
    int max_x = g->win_x + g->win_w - 3;
    int max_y = start_y + g->help_h - 1;  /* 4 lines: start_y to start_y+3 */
    
    attron(COLOR_PAIR(CP_MENU_BAR));
    
    int y = start_y;
    int x = start_x;
    const char *p = help_text;
    char word[80];
    int word_len;
    
    while (*p && y <= max_y) {
        /* Collect a word */
        word_len = 0;
        while (*p && !isspace(*p) && word_len < 79) {
            word[word_len++] = *p++;
        }
        word[word_len] = '\0';
        
        /* Check if word fits on current line */
        if (x + word_len >= max_x && x > start_x) {
            y++;
            x = start_x;
        }
        
        /* Print word */
        if (y <= max_y && word_len > 0) {
            mvprintw(y, x, "%s", word);
            x += word_len;
        }
        
        /* Handle whitespace */
        while (*p && isspace(*p)) {
            if (*p == '\n') {
                y++;
                x = start_x;
            } else if (x < max_x) {
                x++;
            }
            p++;
        }
    }
    
    attroff(COLOR_PAIR(CP_MENU_BAR));
}

/* Draw a single field - can be in normal or paired (two-column) mode */
static void draw_field_at(const FormGeometry *g, int y, int label_x, int label_w, int value_w,
                          const FieldDef *field, const char *value, bool selected, 
                          bool is_disabled, bool is_mex)
{
    (void)g;  /* Used in caller for positioning */
    int value_x = label_x + label_w + 2;  /* label + ": " */
    
    /* Handle separator - just a blank line */
    if (field->type == FIELD_SEPARATOR) {
        return;  /* Nothing to draw */
    }
    
    /* Draw label - grey normally, bold white when selected, right-justified */
    if (is_disabled) {
        attron(COLOR_PAIR(CP_DIALOG_BORDER));  /* Dim grey for disabled */
    } else if (selected) {
        attron(COLOR_PAIR(CP_MENU_BAR) | A_BOLD);
    } else {
        attron(COLOR_PAIR(CP_MENU_BAR));
    }
    mvprintw(y, label_x, "%*s", label_w, field->label);
    attroff(COLOR_PAIR(CP_MENU_BAR) | A_BOLD);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Draw colon */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    printw(": ");
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Draw value */
    int disp_width = value_w;
    
    if (is_disabled) {
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        if (selected) attron(A_REVERSE);
        mvprintw(y, value_x, "%-*s", disp_width, "(disabled)");
        if (selected) attroff(A_REVERSE);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    } else if (field->type == FIELD_TOGGLE || field->type == FIELD_SELECT) {
        const char *opt_text = value ? value : field->default_value;
        if (selected) {
            attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        } else {
            attron(COLOR_PAIR(CP_FORM_VALUE));
        }
        mvprintw(y, value_x, "%-*.*s", disp_width, disp_width, opt_text ? opt_text : "");
        if (selected) {
            attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        } else {
            attroff(COLOR_PAIR(CP_FORM_VALUE));
        }
    } else {
        if (selected) {
            attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        } else {
            attron(COLOR_PAIR(CP_FORM_VALUE));
        }
        
        if (is_mex && field->type == FIELD_FILE) {
            const char *disp_val = value ? value : "";
            if (disp_val[0] == ':') disp_val++;
            mvprintw(y, value_x, ":%-*.*s", disp_width - 1, disp_width - 1, disp_val);
        } else {
            mvprintw(y, value_x, "%-*.*s", disp_width, disp_width, value ? value : "");
        }
        
        if (selected) {
            attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        } else {
            attroff(COLOR_PAIR(CP_FORM_VALUE));
        }
    }
}

/* Draw a single field (normal full-width mode) */
static void draw_field(const FormGeometry *g, int row_idx, const FieldDef *field, 
                       const char *value, bool selected, bool is_disabled, bool is_mex)
{
    int y = g->field_y + row_idx;
    draw_field_at(g, y, g->field_x, g->label_w, g->value_w - 2, field, value, selected, is_disabled, is_mex);
}

/* Draw a paired row (two fields side by side) */
static void draw_paired_row(const FormGeometry *g, int row_idx, 
                            const FieldDef *field1, const char *value1, bool sel1, bool dis1, bool mex1,
                            const FieldDef *field2, const char *value2, bool sel2, bool dis2, bool mex2)
{
    int y = g->field_y + row_idx;
    int col_width = (g->win_w - 2 * PADDING - 6) / 2;  /* Two columns with gap */
    int label_w = PAIR_LABEL_W;
    int value_w = col_width - label_w - 2;
    if (value_w < 3) value_w = 3;
    
    /* Left column */
    draw_field_at(g, y, g->field_x, label_w, value_w, field1, value1, sel1, dis1, mex1);
    
    /* Right column */
    if (field2) {
        int right_x = g->field_x + col_width + 3;
        draw_field_at(g, y, right_x, label_w, value_w, field2, value2, sel2, dis2, mex2);
    }
}

/* Edit a text field inline
 * paired_col: -1 for non-paired, 0 for left column, 1 for right column */
static bool edit_text_field(const FormGeometry *g, int field_idx, char *buffer, int max_len, int paired_col)
{
    int y = g->field_y + field_idx;
    int x, display_width;
    
    if (paired_col >= 0) {
        /* Paired field - calculate column positions */
        int col_width = (g->win_w - 2 * PADDING - 6) / 2;
        int label_w = PAIR_LABEL_W;
        int value_w = col_width - label_w - 2;
        if (value_w < 3) value_w = 3;
        
        if (paired_col == 0) {
            x = g->field_x + label_w + 2;
        } else {
            int right_x = g->field_x + col_width + 3;
            x = right_x + label_w + 2;
        }
        display_width = value_w;
    } else {
        /* Normal single-column field */
        x = g->field_x + g->label_w + 2;  /* label + ": " */
        display_width = g->value_w - 2;
    }
    
    int cursor = strlen(buffer);
    int offset = 0;
    int ch;
    bool done = false;
    bool saved = false;
    
    curs_set(1);
    
    while (!done) {
        /* Adjust offset */
        if (cursor < offset) {
            offset = cursor;
        } else if (cursor >= offset + display_width) {
            offset = cursor - display_width + 1;
        }
        
        /* Draw field */
        attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        mvprintw(y, x, "%-*.*s", display_width, display_width, buffer + offset);
        move(y, x + cursor - offset);
        attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        
        refresh();
        
        ch = getch();
        
        switch (ch) {
            case '\n':
            case '\r':
                saved = true;
                done = true;
                break;
                
            case 27:
                done = true;
                break;
                
            case KEY_LEFT:
                if (cursor > 0) cursor--;
                break;
                
            case KEY_RIGHT:
                if (cursor < (int)strlen(buffer)) cursor++;
                break;
                
            case KEY_HOME:
                cursor = 0;
                break;
                
            case KEY_END:
                cursor = strlen(buffer);
                break;
                
            case KEY_BACKSPACE:
            case 127:
            case 8:
                if (cursor > 0) {
                    memmove(buffer + cursor - 1, buffer + cursor, strlen(buffer) - cursor + 1);
                    cursor--;
                }
                break;
                
            case KEY_DC:
                if (cursor < (int)strlen(buffer)) {
                    memmove(buffer + cursor, buffer + cursor + 1, strlen(buffer) - cursor);
                }
                break;
                
            default:
                if (ch >= 32 && ch < 127 && (int)strlen(buffer) < max_len - 1) {
                    memmove(buffer + cursor + 1, buffer + cursor, strlen(buffer) - cursor + 1);
                    buffer[cursor] = ch;
                    cursor++;
                }
                break;
        }
    }
    
    curs_set(0);
    return saved;
}

/* Show the form and edit fields */
bool form_edit(const char *title, const FieldDef *fields, int field_count, char **values)
{
    FormState state = {
        .values = values,
        .field_count = field_count,
        .selected = 0,
        .dirty = false
    };
    
    FormGeometry g = calc_geometry(title, fields, field_count);
    
    /* Track disabled state for each field */
    /* A field is disabled if it has no value and can_disable is true */
    bool *disabled = calloc(field_count, sizeof(bool));
    if (!disabled) return false;
    
    /* Track MEX mode for each field (value starts with :) */
    bool *mex_mode = calloc(field_count, sizeof(bool));
    if (!mex_mode) { free(disabled); return false; }
    
    for (int i = 0; i < field_count; i++) {
        if (fields[i].can_disable) {
            /* Disabled if value is NULL or empty */
            disabled[i] = (!values[i] || values[i][0] == '\0');
        }
        if (fields[i].supports_mex && values[i] && values[i][0] == ':') {
            mex_mode[i] = true;
        }
    }
    
    int ch;
    int scroll_offset = 0;  /* Row-based scroll offset */
    bool done = false;
    bool saved = false;
    
    /* Build a map from field index to display row */
    int *field_to_row = calloc(field_count, sizeof(int));
    if (!field_to_row) { free(mex_mode); free(disabled); return false; }
    
    int row = 0;
    for (int i = 0; i < field_count; i++) {
        field_to_row[i] = row;
        if (!fields[i].pair_with_next || i + 1 >= field_count) {
            row++;
        }
    }
    int total_rows = row;
    
    while (!done) {
        /* Get current field's row */
        int sel_row = field_to_row[state.selected];
        
        /* Adjust scroll offset to keep selection visible */
        if (sel_row < scroll_offset) {
            scroll_offset = sel_row;
        } else if (sel_row >= scroll_offset + g.max_visible) {
            scroll_offset = sel_row - g.max_visible + 1;
        }
        
        /* Draw background (shaded) */
        draw_work_area();
        
        /* Draw form window */
        draw_form_window(&g, title);
        
        /* Draw help separator - skip for separators */
        if (fields[state.selected].type != FIELD_SEPARATOR) {
            draw_help_separator(&g, &fields[state.selected], disabled[state.selected], mex_mode[state.selected]);
        } else {
            /* For separators, show generic help */
            draw_help_separator(&g, &fields[0], false, false);  /* Use first field as fallback */
        }
        
        /* Draw visible fields, handling pairs and separators */
        int draw_row = 0;
        for (int i = 0; i < field_count && draw_row < scroll_offset + g.max_visible; i++) {
            int field_row = field_to_row[i];
            
            /* Skip rows before scroll offset */
            if (field_row < scroll_offset) {
                if (fields[i].pair_with_next && i + 1 < field_count) i++;
                continue;
            }
            
            int screen_row = field_row - scroll_offset;
            if (screen_row >= g.max_visible) break;
            
            if (fields[i].pair_with_next && i + 1 < field_count) {
                /* Draw paired row */
                draw_paired_row(&g, screen_row,
                    &fields[i], values[i], i == state.selected, disabled[i], mex_mode[i],
                    &fields[i+1], values[i+1], i+1 == state.selected, disabled[i+1], mex_mode[i+1]);
                i++;  /* Skip the second field of the pair */
            } else {
                /* Draw single field (or separator) */
                draw_field(&g, screen_row, &fields[i], values[i], 
                           i == state.selected, disabled[i], mex_mode[i]);
            }
            
            draw_row = field_row + 1;
        }
        
        /* Draw scroll indicators if needed */
        if (total_rows > g.max_visible) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            if (scroll_offset > 0) {
                mvprintw(g.field_y - 1, g.win_x + g.win_w - 4, "^^^");
            }
            if (scroll_offset + g.max_visible < total_rows) {
                mvprintw(g.field_y + g.max_visible, g.win_x + g.win_w - 4, "vvv");
            }
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        
        /* Draw help text - show help for current field unless it's a separator */
        if (fields[state.selected].type != FIELD_SEPARATOR) {
            draw_help_text(&g, fields[state.selected].help);
        }
        
        /* Draw status bar */
        draw_status_bar("ESC=Abort  F10=Save/Exit  Enter=Edit");
        
        refresh();
        
        ch = getch();
        
        switch (ch) {
            case KEY_UP:
                if (state.selected > 0) {
                    state.selected--;
                    /* Skip separators */
                    while (state.selected > 0 && fields[state.selected].type == FIELD_SEPARATOR)
                        state.selected--;
                }
                break;
                
            case KEY_DOWN:
                if (state.selected < field_count - 1) {
                    state.selected++;
                    /* Skip separators */
                    while (state.selected < field_count - 1 && fields[state.selected].type == FIELD_SEPARATOR)
                        state.selected++;
                }
                break;
                
            case KEY_PPAGE:  /* Page Up */
                state.selected -= g.max_visible;
                if (state.selected < 0) state.selected = 0;
                break;
                
            case KEY_NPAGE:  /* Page Down */
                state.selected += g.max_visible;
                if (state.selected >= field_count) state.selected = field_count - 1;
                break;
                
            case KEY_HOME:
                state.selected = 0;
                break;
                
            case KEY_END:
                state.selected = field_count - 1;
                break;
                
            case ' ':  /* Space for manual edit or toggle */
                {
                    const FieldDef *field = &fields[state.selected];
                    
                    /* Compute paired column: -1=single, 0=left, 1=right */
                    int paired_col = -1;
                    if (field->pair_with_next) {
                        paired_col = 0;  /* Left column */
                    } else if (state.selected > 0 && fields[state.selected - 1].pair_with_next) {
                        paired_col = 1;  /* Right column */
                    }
                    int visual_row = field_to_row[state.selected] - scroll_offset;
                    
                    /* For file fields, space opens manual edit */
                    if (field->type == FIELD_FILE && !disabled[state.selected]) {
                        char buffer[256];
                        strncpy(buffer, values[state.selected] ? values[state.selected] : "", 255);
                        buffer[255] = '\0';
                        
                        if (edit_text_field(&g, visual_row, buffer, field->max_length, paired_col)) {
                            if (values[state.selected]) {
                                free(values[state.selected]);
                            }
                            values[state.selected] = strdup(buffer);
                            /* Update mex_mode based on new value */
                            mex_mode[state.selected] = (buffer[0] == ':');
                            state.dirty = true;
                        }
                    } else if (field->type == FIELD_SELECT && field->toggle_options) {
                        /* FIELD_SELECT: Space opens manual text edit for custom override */
                        char buffer[256];
                        strncpy(buffer, values[state.selected] ? values[state.selected] : "", 255);
                        buffer[255] = '\0';
                        
                        if (edit_text_field(&g, visual_row, buffer, field->max_length, paired_col)) {
                            if (values[state.selected]) {
                                free(values[state.selected]);
                            }
                            values[state.selected] = strdup(buffer);
                            state.dirty = true;
                        }
                    } else if (field->type == FIELD_TOGGLE && field->toggle_options) {
                        /* FIELD_TOGGLE: Space cycles through options */
                        int cur_idx = 0;
                        const char *cur_val = values[state.selected] ? values[state.selected] : field->default_value;
                        
                        for (int i = 0; field->toggle_options[i]; i++) {
                            if (cur_val && strcasecmp(cur_val, field->toggle_options[i]) == 0) {
                                cur_idx = i;
                                break;
                            }
                        }
                        
                        /* Move to next option (wrap around) */
                        cur_idx++;
                        if (!field->toggle_options[cur_idx]) {
                            cur_idx = 0;  /* Wrap to first option */
                        }
                        
                        /* Update value */
                        if (values[state.selected]) {
                            free(values[state.selected]);
                        }
                        values[state.selected] = strdup(field->toggle_options[cur_idx]);
                        state.dirty = true;
                    }
                }
                break;
                
            case '\n':
            case '\r':
            case KEY_F(2):
                {
                    const FieldDef *field = &fields[state.selected];
                    
                    /* FIELD_SELECT with F2: open picker dialog */
                    if (field->type == FIELD_SELECT && field->toggle_options && ch == KEY_F(2)) {
                        /* Find current option index */
                        int cur_idx = 0;
                        const char *cur_val = values[state.selected] ? values[state.selected] : field->default_value;
                        
                        for (int i = 0; field->toggle_options[i]; i++) {
                            if (cur_val && strcasecmp(cur_val, field->toggle_options[i]) == 0) {
                                cur_idx = i;
                                break;
                            }
                        }
                        
                        /* Show picker dialog */
                        int new_idx = dialog_option_picker(field->label, field->toggle_options, cur_idx);
                        if (new_idx >= 0) {
                            if (values[state.selected]) {
                                free(values[state.selected]);
                            }
                            values[state.selected] = strdup(field->toggle_options[new_idx]);
                            state.dirty = true;
                        }
                        /* Redraw form after dialog */
                        draw_form_window(&g, title);
                    }
                    /* FIELD_SELECT with Enter: open picker dialog (same as F2) */
                    else if (field->type == FIELD_SELECT && field->toggle_options && (ch == '\n' || ch == '\r')) {
                        /* Find current option index */
                        int cur_idx = 0;
                        const char *cur_val = values[state.selected] ? values[state.selected] : field->default_value;
                        
                        for (int i = 0; field->toggle_options[i]; i++) {
                            if (cur_val && strcasecmp(cur_val, field->toggle_options[i]) == 0) {
                                cur_idx = i;
                                break;
                            }
                        }
                        
                        /* Show picker dialog */
                        int new_idx = dialog_option_picker(field->label, field->toggle_options, cur_idx);
                        if (new_idx >= 0) {
                            if (values[state.selected]) {
                                free(values[state.selected]);
                            }
                            values[state.selected] = strdup(field->toggle_options[new_idx]);
                            state.dirty = true;
                        }
                        /* Redraw form after dialog */
                        draw_form_window(&g, title);
                    }
                    /* FIELD_TOGGLE with Enter: cycle options */
                    else if (field->type == FIELD_TOGGLE && field->toggle_options) {
                        /* Find current option index */
                        int cur_idx = 0;
                        const char *cur_val = values[state.selected] ? values[state.selected] : field->default_value;
                        
                        for (int i = 0; field->toggle_options[i]; i++) {
                            if (cur_val && strcasecmp(cur_val, field->toggle_options[i]) == 0) {
                                cur_idx = i;
                                break;
                            }
                        }
                        
                        /* Move to next option (wrap around) */
                        cur_idx++;
                        if (!field->toggle_options[cur_idx]) {
                            cur_idx = 0;  /* Wrap to first option */
                        }
                        
                        /* Update value */
                        if (values[state.selected]) {
                            free(values[state.selected]);
                        }
                        values[state.selected] = strdup(field->toggle_options[cur_idx]);
                        state.dirty = true;
                    } else if (field->type == FIELD_FILE && (ch == KEY_F(2) || ch == '\n' || ch == '\r')) {
                        /* Only open file picker if field is not disabled */
                        if (!disabled[state.selected]) {
                            /* Use *.vm filter and m/ path for MEX mode */
                            const char *filter = mex_mode[state.selected] ? "*.vm" : 
                                (field->file_filter ? field->file_filter : "*.bbs");
                            const char *base_path = mex_mode[state.selected] ? "m" :
                                (field->file_base_path ? field->file_base_path : "etc/misc");
                            char *selected = filepicker_select(
                                base_path,
                                filter,
                                values[state.selected]
                            );
                            if (selected) {
                                if (values[state.selected]) {
                                    free(values[state.selected]);
                                }
                                /* Add : prefix for MEX mode */
                                if (mex_mode[state.selected]) {
                                    char *mex_val = malloc(strlen(selected) + 2);
                                    if (mex_val) {
                                        sprintf(mex_val, ":%s", selected);
                                        free(selected);
                                        values[state.selected] = mex_val;
                                    } else {
                                        values[state.selected] = selected;
                                    }
                                } else {
                                    values[state.selected] = selected;
                                }
                                state.dirty = true;
                            }
                        }
                    } else if (field->type == FIELD_MULTISELECT && field->toggle_options && 
                               (ch == KEY_F(2) || ch == '\n' || ch == '\r')) {
                        /* Count options */
                        int opt_count = 0;
                        while (field->toggle_options[opt_count]) opt_count++;
                        
                        /* Build CheckItem array */
                        CheckItem *check_items = calloc(opt_count, sizeof(CheckItem));
                        if (check_items) {
                            for (int i = 0; i < opt_count; i++) {
                                check_items[i].name = field->toggle_options[i];
                                check_items[i].value = field->toggle_options[i];
                                check_items[i].checked = false;
                            }
                            
                            /* Parse current value to set checked states */
                            checkpicker_parse_string(check_items, opt_count, 
                                values[state.selected] ? values[state.selected] : "");
                            
                            /* Show picker */
                            if (checkpicker_show(field->label, check_items, opt_count)) {
                                /* Build new value string */
                                char *new_val = checkpicker_build_string(check_items, opt_count);
                                if (new_val) {
                                    if (values[state.selected]) free(values[state.selected]);
                                    values[state.selected] = new_val;
                                    state.dirty = true;
                                }
                            }
                            
                            free(check_items);
                        }
                        /* Redraw form after dialog */
                        draw_form_window(&g, title);
                    } else if (field->type != FIELD_TOGGLE && field->type != FIELD_FILE && 
                               field->type != FIELD_MULTISELECT) {
                        char buffer[256];
                        strncpy(buffer, values[state.selected] ? values[state.selected] : "", 255);
                        buffer[255] = '\0';
                        
                        /* Compute paired column: -1=single, 0=left, 1=right */
                        int paired_col = -1;
                        if (field->pair_with_next) {
                            paired_col = 0;
                        } else if (state.selected > 0 && fields[state.selected - 1].pair_with_next) {
                            paired_col = 1;
                        }
                        int visual_row = field_to_row[state.selected] - scroll_offset;
                        
                        if (edit_text_field(&g, visual_row, buffer, field->max_length, paired_col)) {
                            if (values[state.selected]) {
                                free(values[state.selected]);
                            }
                            values[state.selected] = strdup(buffer);
                            state.dirty = true;
                        }
                    }
                }
                break;
                
            case KEY_F(3):
                /* Toggle enable/disable for fields that support it */
                {
                    const FieldDef *field = &fields[state.selected];
                    if (field->can_disable) {
                        disabled[state.selected] = !disabled[state.selected];
                        if (disabled[state.selected]) {
                            /* Clear value when disabling */
                            if (values[state.selected]) {
                                free(values[state.selected]);
                                values[state.selected] = NULL;
                            }
                            /* Also clear MEX mode */
                            mex_mode[state.selected] = false;
                        }
                        state.dirty = true;
                    }
                }
                break;
                
            case KEY_F(4):
                /* Toggle MEX mode for fields that support it */
                {
                    const FieldDef *field = &fields[state.selected];
                    if (field->type == FIELD_FILE && field->supports_mex && !disabled[state.selected]) {
                        mex_mode[state.selected] = !mex_mode[state.selected];
                        /* Update the value prefix */
                        if (values[state.selected]) {
                            if (mex_mode[state.selected] && values[state.selected][0] != ':') {
                                /* Add : prefix */
                                char *new_val = malloc(strlen(values[state.selected]) + 2);
                                if (new_val) {
                                    sprintf(new_val, ":%s", values[state.selected]);
                                    free(values[state.selected]);
                                    values[state.selected] = new_val;
                                }
                            } else if (!mex_mode[state.selected] && values[state.selected][0] == ':') {
                                /* Remove : prefix */
                                char *new_val = strdup(values[state.selected] + 1);
                                if (new_val) {
                                    free(values[state.selected]);
                                    values[state.selected] = new_val;
                                }
                            }
                        }
                        state.dirty = true;
                    }
                }
                break;
                
            case KEY_F(10):
                saved = true;
                done = true;
                break;
                
            case 27:
                if (state.dirty) {
                    DialogResult result = dialog_save_prompt();
                    if (result == DIALOG_SAVE_EXIT) {
                        saved = true;
                        done = true;
                    } else if (result == DIALOG_ABORT) {
                        done = true;
                    }
                } else {
                    done = true;
                }
                break;
        }
    }
    
    free(disabled);
    free(mex_mode);
    free(field_to_row);
    
    if (saved) {
        g_state.dirty = true;
    }
    
    return saved;
}
