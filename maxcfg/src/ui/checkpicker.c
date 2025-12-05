/* checkpicker.c - Multi-select checkbox picker dialog
 *
 * Shows a list of items with checkboxes that can be toggled.
 * Used for flag selections like System Flags, Mail Flags, etc.
 */

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "ui.h"
#include "maxcfg.h"

/* Internal state for the checkbox picker */
typedef struct {
    const char *title;
    CheckItem *items;
    int item_count;
    int selected;
    int scroll_offset;
    int visible_rows;
} CheckPickerState;

/* Forward declaration */
static void draw_check_picker(CheckPickerState *state, int y, int x, int height, int width);

/*
 * checkpicker_show - Display a multi-select checkbox picker dialog
 *
 * Parameters:
 *   title - Dialog title
 *   items - Array of check items (modified in place)
 *   item_count - Number of items
 *
 * Returns:
 *   true if user pressed ENTER to confirm, false if ESC to cancel
 */
bool checkpicker_show(const char *title, CheckItem *items, int item_count)
{
    int max_rows, max_cols;
    getmaxyx(stdscr, max_rows, max_cols);
    
    /* Calculate dialog dimensions */
    int width = 50;
    
    /* Find max item width */
    for (int i = 0; i < item_count; i++) {
        int len = strlen(items[i].name) + 6;  /* [X] name */
        if (len + 4 > width) width = len + 4;
    }
    if (width > max_cols - 8) width = max_cols - 8;
    
    int height = item_count + 4;  /* items + borders + status line */
    if (height > max_rows - 6) height = max_rows - 6;
    if (height < 8) height = 8;
    
    int x = (max_cols - width) / 2;
    int y = (max_rows - height) / 2;
    
    /* Initialize state */
    CheckPickerState state = {
        .title = title,
        .items = items,
        .item_count = item_count,
        .selected = 0,
        .scroll_offset = 0,
        .visible_rows = height - 4  /* Minus borders and status line */
    };
    
    bool confirmed = false;
    bool done = false;
    
    /* Enable keypad for special keys */
    keypad(stdscr, TRUE);
    curs_set(0);
    
    while (!done) {
        draw_check_picker(&state, y, x, height, width);
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
                
            case ' ':
            case 'x':
            case 'X':
                /* Toggle current item */
                if (state.item_count > 0) {
                    items[state.selected].checked = !items[state.selected].checked;
                }
                break;
                
            case 'a':
            case 'A':
                /* Select all */
                for (int i = 0; i < item_count; i++) {
                    items[i].checked = true;
                }
                break;
                
            case 'n':
            case 'N':
                /* Select none */
                for (int i = 0; i < item_count; i++) {
                    items[i].checked = false;
                }
                break;
                
            case '\n':
            case '\r':
            case KEY_ENTER:
                confirmed = true;
                done = true;
                break;
                
            case 27:  /* ESC */
                confirmed = false;
                done = true;
                break;
        }
    }
    
    curs_set(1);
    return confirmed;
}

static void draw_check_picker(CheckPickerState *state, int y, int x, int height, int width)
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
    for (int i = 1; i < height - 2; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        
        /* Clear content area */
        attron(COLOR_PAIR(CP_DIALOG_TEXT));
        for (int j = 1; j < width - 1; j++) {
            mvaddch(y + i, x + j, ' ');
        }
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        
        mvaddch(y + i, x + width - 1, ACS_VLINE);
    }
    
    /* Separator before status line */
    int sep_y = y + height - 2;
    mvaddch(sep_y, x, ACS_LTEE);
    for (int i = 1; i < width - 1; i++) {
        mvaddch(sep_y, x + i, ACS_HLINE);
    }
    mvaddch(sep_y, x + width - 1, ACS_RTEE);
    
    /* Status line area */
    int status_y = y + height - 1;
    mvaddch(status_y, x, ACS_VLINE);
    attron(COLOR_PAIR(CP_DIALOG_TEXT));
    for (int j = 1; j < width - 1; j++) {
        mvaddch(status_y, x + j, ' ');
    }
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    mvaddch(status_y, x + width - 1, ACS_VLINE);
    
    /* Bottom border */
    int bottom_y = y + height;
    mvaddch(bottom_y, x, ACS_LLCORNER);
    for (int i = 1; i < width - 1; i++) {
        mvaddch(bottom_y, x + i, ACS_HLINE);
    }
    mvaddch(bottom_y, x + width - 1, ACS_LRCORNER);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Draw status line content */
    move(status_y, x + 2);
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("SPACE");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=Toggle ");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("A");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("ll ");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("N");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("one ");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("ENTER");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=OK ");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("ESC");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=Cancel");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    /* Draw checkbox items */
    for (int i = 0; i < state->visible_rows && (i + state->scroll_offset) < state->item_count; i++) {
        int item_idx = i + state->scroll_offset;
        CheckItem *item = &state->items[item_idx];
        int row = y + 1 + i;
        
        bool is_selected = (item_idx == state->selected);
        
        /* Move to item position */
        move(row, x + 2);
        
        /* Draw checkbox */
        if (is_selected) {
            attron(COLOR_PAIR(CP_FORM_HIGHLIGHT) | A_BOLD);
        } else {
            attron(COLOR_PAIR(CP_DIALOG_TEXT));
        }
        
        if (item->checked) {
            printw("[X] ");
        } else {
            printw("[ ] ");
        }
        
        /* Draw item name */
        printw("%s", item->name);
        
        /* Clear rest of line */
        int cur_x = getcurx(stdscr);
        for (int j = cur_x; j < x + width - 2; j++) {
            addch(' ');
        }
        
        if (is_selected) {
            attroff(COLOR_PAIR(CP_FORM_HIGHLIGHT) | A_BOLD);
        } else {
            attroff(COLOR_PAIR(CP_DIALOG_TEXT));
        }
    }
    
    /* Draw scroll indicators if needed */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    if (state->scroll_offset > 0) {
        mvaddch(y + 1, x + width - 1, ACS_UARROW);
    }
    if (state->scroll_offset + state->visible_rows < state->item_count) {
        mvaddch(y + height - 3, x + width - 1, ACS_DARROW);
    }
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
}

/*
 * checkpicker_build_string - Build space-separated string from checked items
 *
 * Returns: Newly allocated string (caller must free)
 */
char *checkpicker_build_string(CheckItem *items, int item_count)
{
    /* Calculate required length */
    int len = 1;  /* For null terminator */
    for (int i = 0; i < item_count; i++) {
        if (items[i].checked) {
            const char *val = items[i].value ? items[i].value : items[i].name;
            len += strlen(val) + 1;  /* +1 for space */
        }
    }
    
    char *result = malloc(len);
    if (!result) return NULL;
    
    result[0] = '\0';
    
    bool first = true;
    for (int i = 0; i < item_count; i++) {
        if (items[i].checked) {
            const char *val = items[i].value ? items[i].value : items[i].name;
            if (!first) strcat(result, " ");
            strcat(result, val);
            first = false;
        }
    }
    
    return result;
}

/*
 * checkpicker_parse_string - Set checks from space-separated string
 */
void checkpicker_parse_string(CheckItem *items, int item_count, const char *str)
{
    /* Clear all checks first */
    for (int i = 0; i < item_count; i++) {
        items[i].checked = false;
    }
    
    if (!str || !*str) return;
    
    /* Parse space-separated tokens */
    char *copy = strdup(str);
    if (!copy) return;
    
    char *token = strtok(copy, " \t");
    while (token) {
        /* Find matching item */
        for (int i = 0; i < item_count; i++) {
            const char *val = items[i].value ? items[i].value : items[i].name;
            if (strcasecmp(token, val) == 0 || strcasecmp(token, items[i].name) == 0) {
                items[i].checked = true;
                break;
            }
        }
        token = strtok(NULL, " \t");
    }
    
    free(copy);
}
