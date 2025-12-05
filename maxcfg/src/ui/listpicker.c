/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * listpicker.c - Scrollable list picker dialog for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "maxcfg.h"
#include "ui.h"

/* List picker state */
typedef struct {
    const char *title;
    ListItem *items;
    int item_count;
    int selected;
    int scroll_offset;
    int visible_rows;
} ListPickerState;

/* Forward declarations */
static void draw_list_picker(ListPickerState *state, int y, int x, int height, int width);

/*
 * listpicker_show - Display a list picker dialog
 *
 * Parameters:
 *   title - Dialog title
 *   items - Array of list items
 *   item_count - Number of items
 *   selected - Pointer to selected index (updated on return)
 *
 * Returns:
 *   ListPickResult indicating what action was taken
 */
ListPickResult listpicker_show(const char *title, ListItem *items, int item_count, int *selected)
{
    int max_rows, max_cols;
    getmaxyx(stdscr, max_rows, max_cols);
    
    /* Calculate dialog dimensions */
    int width = max_cols - 8;
    if (width > 76) width = 76;
    if (width < 50) width = 50;
    
    int height = max_rows - 6;
    if (height > 20) height = 20;
    if (height < 10) height = 10;
    
    int x = (max_cols - width) / 2;
    int y = (max_rows - height) / 2;
    
    /* Initialize state */
    ListPickerState state = {
        .title = title,
        .items = items,
        .item_count = item_count,
        .selected = *selected,
        .scroll_offset = 0,
        .visible_rows = height - 2  /* Minus top and bottom borders */
    };
    
    /* Adjust scroll if needed */
    if (state.selected >= state.visible_rows) {
        state.scroll_offset = state.selected - state.visible_rows + 1;
    }
    
    ListPickResult result = LISTPICK_NONE;
    bool done = false;
    
    /* Enable keypad for special keys */
    keypad(stdscr, TRUE);
    curs_set(0);
    
    while (!done) {
        draw_list_picker(&state, y, x, height, width);
        doupdate();
        
        int ch = getch();
        
        switch (ch) {
            case KEY_UP:
            case 'k':
                if (state.selected > 0) {
                    state.selected--;
                    if (state.selected < state.scroll_offset) {
                        state.scroll_offset = state.selected;
                    }
                }
                break;
                
            case KEY_DOWN:
            case 'j':
                if (state.selected < state.item_count - 1) {
                    state.selected++;
                    if (state.selected >= state.scroll_offset + state.visible_rows) {
                        state.scroll_offset = state.selected - state.visible_rows + 1;
                    }
                }
                break;
                
            case KEY_PPAGE:
                state.selected -= state.visible_rows;
                if (state.selected < 0) state.selected = 0;
                state.scroll_offset = state.selected;
                break;
                
            case KEY_NPAGE:
                state.selected += state.visible_rows;
                if (state.selected >= state.item_count) {
                    state.selected = state.item_count - 1;
                }
                if (state.selected >= state.scroll_offset + state.visible_rows) {
                    state.scroll_offset = state.selected - state.visible_rows + 1;
                }
                break;
                
            case KEY_HOME:
                state.selected = 0;
                state.scroll_offset = 0;
                break;
                
            case KEY_END:
                state.selected = state.item_count - 1;
                if (state.selected >= state.visible_rows) {
                    state.scroll_offset = state.selected - state.visible_rows + 1;
                }
                break;
                
            case '\n':
            case '\r':
            case KEY_ENTER:
                result = LISTPICK_EDIT;
                done = true;
                break;
                
            case KEY_IC:  /* Insert key */
            case 'i':
            case 'I':
                result = LISTPICK_INSERT;
                done = true;
                break;
                
            case KEY_DC:  /* Delete key */
            case 'd':
            case 'D':
                if (state.item_count > 0) {
                    /* Toggle enabled state */
                    items[state.selected].enabled = !items[state.selected].enabled;
                }
                break;
                
            case 27:  /* ESC */
                result = LISTPICK_EXIT;
                done = true;
                break;
                
            case ' ':
                /* Space bar - jump to last item */
                state.selected = state.item_count - 1;
                if (state.selected >= state.visible_rows) {
                    state.scroll_offset = state.selected - state.visible_rows + 1;
                }
                break;
                
            default:
                /* Number keys for quick jump */
                if (ch >= '0' && ch <= '9') {
                    int target = ch - '0';
                    if (target < state.item_count) {
                        state.selected = target;
                        if (state.selected < state.scroll_offset) {
                            state.scroll_offset = state.selected;
                        } else if (state.selected >= state.scroll_offset + state.visible_rows) {
                            state.scroll_offset = state.selected - state.visible_rows + 1;
                        }
                    }
                }
                break;
        }
    }
    
    *selected = state.selected;
    curs_set(1);
    
    return result;
}

static void draw_list_picker(ListPickerState *state, int y, int x, int height, int width)
{
    /* Draw border */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Top border with title */
    mvaddch(y, x, ACS_ULCORNER);
    for (int i = 1; i < width - 1; i++) {
        mvaddch(y, x + i, ACS_HLINE);
    }
    mvaddch(y, x + width - 1, ACS_URCORNER);
    
    /* Title centered */
    if (state->title) {
        int title_len = strlen(state->title);
        int title_x = x + (width - title_len - 2) / 2;
        mvaddch(y, title_x - 1, ' ');
        attron(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
        mvprintw(y, title_x, "%s", state->title);
        attroff(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvaddch(y, title_x + title_len, ' ');
    }
    
    /* Side borders and content area */
    for (int i = 1; i < height - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        
        /* Clear content area */
        attron(COLOR_PAIR(CP_DIALOG_TEXT));
        for (int j = 1; j < width - 1; j++) {
            mvaddch(y + i, x + j, ' ');
        }
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        
        mvaddch(y + i, x + width - 1, ACS_VLINE);
    }
    
    /* Bottom border with embedded status line */
    int bottom_y = y + height - 1;
    mvaddch(bottom_y, x, ACS_LLCORNER);
    addch(ACS_HLINE);
    addch(' ');
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Status items embedded in border - INS=(I)nsert */
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("INS");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=(");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("I");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw(")nsert");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(ACS_HLINE);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* DEL=(D)elete */
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("DEL");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=(");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("D");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw(")elete");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(ACS_HLINE);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* ESC=Exit */
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("ESC");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=Exit");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    /* Fill rest of bottom border */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    int cur_x = getcurx(stdscr);
    addch(' ');
    for (int i = cur_x + 1; i < x + width - 1; i++) {
        addch(ACS_HLINE);
    }
    mvaddch(bottom_y, x + width - 1, ACS_LRCORNER);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Draw list items */
    for (int i = 0; i < state->visible_rows && (i + state->scroll_offset) < state->item_count; i++) {
        int item_idx = i + state->scroll_offset;
        ListItem *item = &state->items[item_idx];
        int row = y + 1 + i;
        
        /* Build display string */
        char display[256];
        if (item->extra && item->extra[0]) {
            snprintf(display, sizeof(display), "%d: %s (%s)", item_idx, item->name, item->extra);
        } else {
            snprintf(display, sizeof(display), "%d: %s", item_idx, item->name);
        }
        
        /* Truncate if too long */
        int max_len = width - 4;
        if ((int)strlen(display) > max_len) {
            display[max_len] = '\0';
        }
        
        if (item_idx == state->selected) {
            /* Selected item - blue background */
            attron(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
            mvprintw(row, x + 2, "%-*s", width - 4, display);
            attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
        } else {
            /* Normal item */
            if (!item->enabled) {
                /* Disabled - dim */
                attron(COLOR_PAIR(CP_DIALOG_TEXT) | A_DIM);
            } else {
                attron(COLOR_PAIR(CP_DIALOG_TEXT));
            }
            mvprintw(row, x + 2, "%s", display);
            if (!item->enabled) {
                attroff(COLOR_PAIR(CP_DIALOG_TEXT) | A_DIM);
            } else {
                attroff(COLOR_PAIR(CP_DIALOG_TEXT));
            }
        }
    }
    
    /* Scroll indicator if needed */
    if (state->item_count > state->visible_rows) {
        if (state->scroll_offset > 0) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(y + 1, x + width - 2, ACS_UARROW);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        if (state->scroll_offset + state->visible_rows < state->item_count) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(y + height - 2, x + width - 2, ACS_DARROW);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
    }
    
    wnoutrefresh(stdscr);
}

/*
 * Helper functions for managing list items
 */

ListItem *listitem_create(const char *name, const char *extra, void *data)
{
    ListItem *item = malloc(sizeof(ListItem));
    if (!item) return NULL;
    
    item->name = name ? strdup(name) : strdup("");
    item->extra = extra ? strdup(extra) : NULL;
    item->enabled = true;
    item->data = data;
    
    return item;
}

void listitem_free(ListItem *item)
{
    if (item) {
        free(item->name);
        free(item->extra);
        free(item);
    }
}

void listitem_array_free(ListItem *items, int count)
{
    if (items) {
        for (int i = 0; i < count; i++) {
            free(items[i].name);
            free(items[i].extra);
        }
        free(items);
    }
}
