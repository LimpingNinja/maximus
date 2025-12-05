/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * colorform.c - Color editing form for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "maxcfg.h"
#include "ui.h"

/* Color field definition */
typedef struct {
    const char *label;          /* Display label */
    const char *define_name;    /* colors.lh define name */
    const char *help;           /* Help text */
    int current_fg;             /* Current foreground (0-15) */
    int current_bg;             /* Current background (0-7) */
} ColorFieldDef;

/* Menu colors */
static ColorFieldDef menu_colors[] = {
    { "Menu name",        "COL_MNU_NAME",   "Color for menu item names", 14, 0 },
    { "Menu highlight",   "COL_MNU_HILITE", "Color for highlighted menu items", 14, 0 },
    { "Menu option",      "COL_MNU_OPTION", "Color for menu option text", 7, 0 },
};
#define NUM_MENU_COLORS (sizeof(menu_colors) / sizeof(menu_colors[0]))

/* File area colors */
static ColorFieldDef file_colors[] = {
    { "File name",        "COL_FILE_NAME",  "Color for file names in listings", 14, 0 },
    { "File size",        "COL_FILE_SIZE",  "Color for file sizes", 5, 0 },
    { "File date",        "COL_FILE_DATE",  "Color for file dates", 2, 0 },
    { "File description", "COL_FILE_DESC",  "Color for file descriptions", 3, 0 },
    { "File search match","COL_FILE_FIND",  "Color for search match highlights", 14, 0 },
    { "Offline file",     "COL_FILE_OFFLN", "Color for offline files", 4, 0 },
    { "New file",         "COL_FILE_NEW",   "Color for new files (with blink)", 3, 0 },
};
#define NUM_FILE_COLORS (sizeof(file_colors) / sizeof(file_colors[0]))

/* Message reader colors */
static ColorFieldDef msg_colors[] = {
    { "From label",       "COL_MSG_FROM",    "Color for 'From:' label", 3, 0 },
    { "From text",        "COL_MSG_FROMTXT", "Color for sender name", 14, 0 },
    { "To label",         "COL_MSG_TO",      "Color for 'To:' label", 3, 0 },
    { "To text",          "COL_MSG_TOTXT",   "Color for recipient name", 14, 0 },
    { "Subject label",    "COL_MSG_SUBJ",    "Color for 'Subject:' label", 3, 0 },
    { "Subject text",     "COL_MSG_SUBJTXT", "Color for subject text", 14, 0 },
    { "Attributes",       "COL_MSG_ATTR",    "Color for message attributes", 10, 0 },
    { "Date",             "COL_MSG_DATE",    "Color for message date", 10, 0 },
    { "Address",          "COL_MSG_ADDR",    "Color for network address", 3, 0 },
    { "Locus",            "COL_MSG_LOCUS",   "Color for message locus", 9, 0 },
    { "Message body",     "COL_MSG_BODY",    "Color for message body text", 3, 0 },
    { "Quoted text",      "COL_MSG_QUOTE",   "Color for quoted text", 7, 0 },
    { "Kludge lines",     "COL_MSG_KLUDGE",  "Color for kludge/control lines", 13, 0 },
};
#define NUM_MSG_COLORS (sizeof(msg_colors) / sizeof(msg_colors[0]))

/* Full screen reader colors (these have backgrounds!) */
static ColorFieldDef fsr_colors[] = {
    { "Message number",   "COL_FSR_MSGNUM",  "Color for message number display", 12, 1 },
    { "Links",            "COL_FSR_LINKS",   "Color for reply chain links", 14, 1 },
    { "Attributes",       "COL_FSR_ATTRIB",  "Color for message attributes", 14, 1 },
    { "Message info",     "COL_FSR_MSGINFO", "Color for message info line", 14, 1 },
    { "Date",             "COL_FSR_DATE",    "Color for date display", 15, 1 },
    { "Address",          "COL_FSR_ADDR",    "Color for network addresses", 14, 1 },
    { "Static text",      "COL_FSR_STATIC",  "Color for static labels", 15, 1 },
    { "Border",           "COL_FSR_BORDER",  "Color for window borders", 11, 1 },
    { "Locus",            "COL_FSR_LOCUS",   "Color for locus display", 15, 0 },
};
#define NUM_FSR_COLORS (sizeof(fsr_colors) / sizeof(fsr_colors[0]))

/* Color pair base for preview */
#define CP_PREVIEW_BASE 50

/* Form geometry */
typedef struct {
    int win_x, win_y;
    int win_w, win_h;
    int help_y, help_h;
    int field_x, field_y;
    int label_w, value_w;
    int max_visible;        /* Max fields that fit in window */
} ColorFormGeometry;

#define MAX_VISIBLE_FIELDS 10  /* Max fields before scrolling */

static ColorFormGeometry calc_color_geometry(const char *title, int field_count)
{
    ColorFormGeometry g;
    
    g.label_w = 18;
    g.value_w = 22;  /* "Light Magenta on Blue" format */
    
    int content_w = g.label_w + 2 + g.value_w;
    int title_len = title ? strlen(title) : 0;
    if (title_len + 4 > content_w) content_w = title_len + 4;
    
    /* Cap visible fields */
    g.max_visible = field_count;
    if (g.max_visible > MAX_VISIBLE_FIELDS) {
        g.max_visible = MAX_VISIBLE_FIELDS;
    }
    
    g.win_w = content_w + 6;
    g.win_h = g.max_visible + 9;  /* visible fields + help + borders */
    
    if (g.win_w > COLS - 4) g.win_w = COLS - 4;
    if (g.win_h > LINES - 4) g.win_h = LINES - 4;
    
    g.win_x = (COLS - g.win_w) / 2;
    g.win_y = (LINES - g.win_h) / 2;
    
    g.field_x = g.win_x + 2;
    g.field_y = g.win_y + 2;
    g.help_y = g.win_y + g.win_h - 5;
    g.help_h = 3;
    
    return g;
}

static void draw_color_window(const ColorFormGeometry *g, const char *title)
{
    int x = g->win_x;
    int y = g->win_y;
    int w = g->win_w;
    int h = g->win_h;
    
    /* Draw border */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    
    mvaddch(y, x, ACS_ULCORNER);
    addch(ACS_HLINE);
    addch(' ');
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("%s", title);
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(' ');
    for (int i = strlen(title) + 4; i < w - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_URCORNER);
    
    for (int i = 1; i < h - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + w - 1, ACS_VLINE);
    }
    
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    for (int i = 1; i < w - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_LRCORNER);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Fill interior */
    attron(COLOR_PAIR(CP_FORM_BG));
    for (int i = 1; i < h - 1; i++) {
        mvhline(y + i, x + 1, ' ', w - 2);
    }
    attroff(COLOR_PAIR(CP_FORM_BG));
}

static void draw_color_help_separator(const ColorFormGeometry *g)
{
    int y = g->help_y;
    int x = g->win_x;
    int w = g->win_w;
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    mvaddch(y, x, ACS_LTEE);
    addch(ACS_HLINE);
    addch(' ');
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("Help");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    printw(" ");
    addch(ACS_HLINE);
    printw(" ");
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("F2");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=Pick Color");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    /* Fill rest with line */
    int cur_x = getcurx(stdscr);
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    printw(" ");
    for (int i = cur_x + 2; i < x + w - 1; i++) {
        addch(ACS_HLINE);
    }
    mvaddch(y, x + w - 1, ACS_RTEE);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
}

static void draw_color_field(const ColorFormGeometry *g, int idx, ColorFieldDef *field, bool selected)
{
    int y = g->field_y + idx;
    int label_x = g->field_x;
    int value_x = g->field_x + g->label_w + 2;
    
    /* Draw label */
    if (selected) {
        attron(COLOR_PAIR(CP_MENU_BAR) | A_BOLD);
    } else {
        attron(COLOR_PAIR(CP_MENU_BAR));
    }
    mvprintw(y, label_x, "%*s", g->label_w, field->label);
    attroff(COLOR_PAIR(CP_MENU_BAR) | A_BOLD);
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    printw(": ");
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Create preview color pair */
    int pair_num = CP_PREVIEW_BASE + idx;
    int fg_ncurses[] = { COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
                         COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE,
                         COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
                         COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE };
    int bg_ncurses[] = { COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
                         COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE };
    
    init_pair(pair_num, fg_ncurses[field->current_fg], bg_ncurses[field->current_bg]);
    
    /* Draw value with preview - show fg on bg if background is non-black */
    char value_str[40];
    if (field->current_bg > 0) {
        snprintf(value_str, sizeof(value_str), "%s on %s", 
                 color_get_name(field->current_fg),
                 color_get_name(field->current_bg));
    } else {
        snprintf(value_str, sizeof(value_str), "%s", 
                 color_get_name(field->current_fg));
    }
    
    if (selected) {
        attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        mvprintw(y, value_x, " %-16s", value_str);
        attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
    } else {
        /* Show color in its actual color */
        attron(COLOR_PAIR(pair_num));
        if (field->current_fg >= 8) attron(A_BOLD);
        mvprintw(y, value_x, " %-16s", value_str);
        if (field->current_fg >= 8) attroff(A_BOLD);
        attroff(COLOR_PAIR(pair_num));
    }
}

/* Edit a color category */
bool colorform_edit(const char *title, ColorFieldDef *fields, int field_count)
{
    int selected = 0;
    int scroll_offset = 0;
    bool dirty = false;
    bool done = false;
    bool saved = false;
    int ch;
    
    ColorFormGeometry g = calc_color_geometry(title, field_count);
    
    while (!done) {
        /* Adjust scroll offset to keep selection visible */
        if (selected < scroll_offset) {
            scroll_offset = selected;
        } else if (selected >= scroll_offset + g.max_visible) {
            scroll_offset = selected - g.max_visible + 1;
        }
        
        draw_work_area();
        draw_color_window(&g, title);
        draw_color_help_separator(&g);
        
        /* Draw visible fields */
        for (int i = 0; i < g.max_visible && (scroll_offset + i) < field_count; i++) {
            int field_idx = scroll_offset + i;
            draw_color_field(&g, i, &fields[field_idx], field_idx == selected);
        }
        
        /* Draw scroll indicators if needed */
        if (field_count > g.max_visible) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            if (scroll_offset > 0) {
                mvprintw(g.field_y - 1, g.win_x + g.win_w - 4, "^^^");
            }
            if (scroll_offset + g.max_visible < field_count) {
                mvprintw(g.field_y + g.max_visible, g.win_x + g.win_w - 4, "vvv");
            }
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        
        /* Draw help text */
        attron(COLOR_PAIR(CP_MENU_BAR));
        mvprintw(g.help_y + 1, g.win_x + 2, "%-*.*s", g.win_w - 4, g.win_w - 4, fields[selected].help);
        attroff(COLOR_PAIR(CP_MENU_BAR));
        
        draw_status_bar("ESC=Abort  F10=Save/Exit  F2/Enter=Pick Color");
        
        refresh();
        
        ch = getch();
        
        switch (ch) {
            case KEY_UP:
                if (selected > 0) selected--;
                break;
                
            case KEY_DOWN:
                if (selected < field_count - 1) selected++;
                break;
                
            case KEY_HOME:
                selected = 0;
                break;
                
            case KEY_END:
                selected = field_count - 1;
                break;
                
            case KEY_PPAGE:  /* Page Up */
                selected -= g.max_visible;
                if (selected < 0) selected = 0;
                break;
                
            case KEY_NPAGE:  /* Page Down */
                selected += g.max_visible;
                if (selected >= field_count) selected = field_count - 1;
                break;
                
            case '\n':
            case '\r':
            case KEY_F(2):
                {
                    /* Open full color grid picker */
                    int new_fg, new_bg;
                    if (colorpicker_select_full(fields[selected].current_fg,
                                                fields[selected].current_bg,
                                                &new_fg, &new_bg)) {
                        fields[selected].current_fg = new_fg;
                        fields[selected].current_bg = new_bg;
                        dirty = true;
                    }
                }
                break;
                
            case KEY_F(10):
                saved = true;
                done = true;
                break;
                
            case 27:
                if (dirty) {
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
    
    if (saved) {
        g_state.dirty = true;
    }
    
    return saved;
}

/* Action for Default Colors menu item - shows category picker */
void action_default_colors(void)
{
    const char *categories[] = {
        "Menu Colors",
        "File Colors", 
        "Message Colors",
        "Reader Colors"
    };
    int selected = 0;
    int ch;
    bool done = false;
    
    int width = 22;
    int height = 8;
    int x = (COLS - width) / 2;
    int y = (LINES - height) / 2;
    
    while (!done) {
        /* Draw border */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvaddch(y, x, ACS_ULCORNER);
        addch(ACS_HLINE);
        addch(' ');
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("Default Colors");
        attroff(COLOR_PAIR(CP_MENU_BAR));
        
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        addch(' ');
        for (int i = 18; i < width - 1; i++) addch(ACS_HLINE);
        addch(ACS_URCORNER);
        
        for (int i = 1; i < height - 1; i++) {
            mvaddch(y + i, x, ACS_VLINE);
            attron(COLOR_PAIR(CP_FORM_BG));
            for (int j = 1; j < width - 1; j++) addch(' ');
            attroff(COLOR_PAIR(CP_FORM_BG));
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(y + i, x + width - 1, ACS_VLINE);
        }
        
        mvaddch(y + height - 1, x, ACS_LLCORNER);
        for (int i = 1; i < width - 1; i++) addch(ACS_HLINE);
        addch(ACS_LRCORNER);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        /* Draw options */
        for (int i = 0; i < 4; i++) {
            if (i == selected) {
                attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
            } else {
                attron(COLOR_PAIR(CP_MENU_BAR));
            }
            mvprintw(y + 2 + i, x + 2, " %-16s ", categories[i]);
            if (i == selected) {
                attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
            } else {
                attroff(COLOR_PAIR(CP_MENU_BAR));
            }
        }
        
        refresh();
        ch = getch();
        
        switch (ch) {
            case KEY_UP:
                if (selected > 0) selected--;
                break;
            case KEY_DOWN:
                if (selected < 3) selected++;
                break;
            case '\n':
            case '\r':
                done = true;
                break;
            case 27:
                return;  /* Cancel */
        }
    }
    
    /* Open selected category */
    switch (selected) {
        case 0: colorform_edit("Menu Colors", menu_colors, NUM_MENU_COLORS); break;
        case 1: colorform_edit("File Area Colors", file_colors, NUM_FILE_COLORS); break;
        case 2: colorform_edit("Message Colors", msg_colors, NUM_MSG_COLORS); break;
        case 3: colorform_edit("Full Screen Reader Colors", fsr_colors, NUM_FSR_COLORS); break;
    }
}

/* Action for File Colors */
void action_file_colors(void)
{
    colorform_edit("File Area Colors", file_colors, NUM_FILE_COLORS);
}

/* Action for Message Colors */
void action_msg_colors(void)
{
    colorform_edit("Message Colors", msg_colors, NUM_MSG_COLORS);
}

/* Action for Full Screen Reader Colors */
void action_fsr_colors(void)
{
    colorform_edit("Full Screen Reader Colors", fsr_colors, NUM_FSR_COLORS);
}
