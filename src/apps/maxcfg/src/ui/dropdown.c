/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * dropdown.c - Dropdown and submenu handling for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <string.h>
#include <ctype.h>
#include "maxcfg.h"
#include "ui.h"

/* External access to menu data */
extern TopMenu* menubar_get_menu(int index);
extern int menubar_get_position(int index);

/* Dropdown state */
static struct {
    bool open;
    int menu_index;         /* Which top-level menu */
    int selected_item;      /* Currently selected item */
    int submenu_selected;   /* Selected item in submenu (-1 if no submenu open) */
    bool submenu_open;      /* Is a submenu currently open? */
} dropdown_state = {
    .open = false,
    .menu_index = 0,
    .selected_item = 0,
    .submenu_selected = -1,
    .submenu_open = false
};

/* Calculate the width needed for a menu */
static int calc_menu_width(MenuItem *items, int count)
{
    int max_width = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(items[i].label);
        if (items[i].submenu) {
            len += 2;  /* Arrow indicator */
        }
        if (len > max_width) {
            max_width = len;
        }
    }
    return max_width + 4;  /* Padding */
}

/* Draw a single dropdown menu */
static void draw_menu(int y, int x, MenuItem *items, int count, int selected)
{
    int width = calc_menu_width(items, count);
    int height = count + 2;
    
    /* Draw box background - black */
    attron(COLOR_PAIR(CP_DROPDOWN));
    for (int row = 0; row < height; row++) {
        move(y + row, x);
        for (int col = 0; col < width; col++) {
            addch(' ');
        }
    }
    
    /* Draw border - cyan on black */
    draw_box(y, x, height, width, CP_DIALOG_BORDER);
    
    /* Draw menu items */
    for (int i = 0; i < count; i++) {
        const char *label = items[i].label;
        
        if (i == selected) {
            /* Highlighted: bold yellow on blue for entire line */
            attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
            mvprintw(y + 1 + i, x + 1, " %-*s", width - 3, label);
            attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        } else {
            /* Normal: bold yellow hotkey, grey rest */
            move(y + 1 + i, x + 1);
            addch(' ');
            
            /* First char is hotkey - bold yellow */
            attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
            addch(label[0]);
            attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
            
            /* Rest of label - grey */
            attron(COLOR_PAIR(CP_DROPDOWN));
            printw("%-*s", width - 4, label + 1);
            attroff(COLOR_PAIR(CP_DROPDOWN));
        }
        
        /* Draw submenu arrow if applicable */
        if (items[i].submenu) {
            if (i == selected) {
                attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
            } else {
                attron(COLOR_PAIR(CP_DROPDOWN));
            }
            mvaddch(y + 1 + i, x + width - 2, ACS_RARROW);
            if (i == selected) {
                attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
            } else {
                attroff(COLOR_PAIR(CP_DROPDOWN));
            }
        }
    }
}

void dropdown_open(int menu_index)
{
    dropdown_state.open = true;
    dropdown_state.menu_index = menu_index;
    dropdown_state.selected_item = 0;
    dropdown_state.submenu_open = false;
    dropdown_state.submenu_selected = -1;
    g_state.menu_open = true;
}

void dropdown_close(void)
{
    dropdown_state.open = false;
    dropdown_state.submenu_open = false;
    dropdown_state.submenu_selected = -1;
    g_state.menu_open = false;
}

void draw_dropdown(void)
{
    if (!dropdown_state.open) {
        return;
    }
    
    TopMenu *menu = menubar_get_menu(dropdown_state.menu_index);
    if (!menu) {
        return;
    }
    
    int x = menubar_get_position(dropdown_state.menu_index);
    int y = MENUBAR_ROW + 1;
    
    /* Draw main dropdown */
    draw_menu(y, x, menu->items, menu->item_count, dropdown_state.selected_item);
    
    /* Draw submenu if open */
    if (dropdown_state.submenu_open) {
        MenuItem *current = &menu->items[dropdown_state.selected_item];
        if (current->submenu) {
            int sub_x = x + calc_menu_width(menu->items, menu->item_count);
            int sub_y = y + dropdown_state.selected_item;
            
            /* Adjust if would go off screen */
            int sub_width = calc_menu_width(current->submenu, current->submenu_count);
            if (sub_x + sub_width > COLS) {
                sub_x = x - sub_width;
            }
            
            draw_menu(sub_y, sub_x, current->submenu, current->submenu_count,
                     dropdown_state.submenu_selected);
        }
    }
    
    wnoutrefresh(stdscr);
}

bool dropdown_is_open(void)
{
    return dropdown_state.open;
}

bool dropdown_handle_key(int ch)
{
    if (!dropdown_state.open) {
        return false;
    }
    
    TopMenu *menu = menubar_get_menu(dropdown_state.menu_index);
    if (!menu) {
        return false;
    }
    
    /* If submenu is open, handle submenu navigation */
    if (dropdown_state.submenu_open) {
        MenuItem *current = &menu->items[dropdown_state.selected_item];
        
        switch (ch) {
            case KEY_UP:
                if (dropdown_state.submenu_selected > 0) {
                    dropdown_state.submenu_selected--;
                }
                return true;
                
            case KEY_DOWN:
                if (dropdown_state.submenu_selected < current->submenu_count - 1) {
                    dropdown_state.submenu_selected++;
                }
                return true;
                
            case KEY_LEFT:
            case 27:  /* ESC */
                dropdown_state.submenu_open = false;
                dropdown_state.submenu_selected = -1;
                return true;
                
            case '\n':
            case '\r':
                /* Execute submenu action - don't close menu, stay in place */
                if (current->submenu[dropdown_state.submenu_selected].action) {
                    current->submenu[dropdown_state.submenu_selected].action();
                    /* Action completed, stay in submenu */
                }
                return true;
                
            default:
                /* Check for hotkey in submenu */
                for (int i = 0; i < current->submenu_count; i++) {
                    const char *hotkey = current->submenu[i].hotkey;
                    if (hotkey && toupper(ch) == toupper(hotkey[0])) {
                        dropdown_state.submenu_selected = i;
                        if (current->submenu[i].action) {
                            current->submenu[i].action();
                            /* Stay in submenu after action */
                        }
                        return true;
                    }
                }
                break;
        }
        return true;
    }
    
    /* Main dropdown navigation */
    switch (ch) {
        case KEY_UP:
            if (dropdown_state.selected_item > 0) {
                dropdown_state.selected_item--;
            } else {
                /* At top of dropdown, close it and return to menu bar */
                dropdown_close();
            }
            return true;
            
        case KEY_DOWN:
            if (dropdown_state.selected_item < menu->item_count - 1) {
                dropdown_state.selected_item++;
            }
            return true;
            
        case KEY_LEFT:
            /* Close dropdown and return to menu bar */
            dropdown_close();
            return true;
            
        case KEY_RIGHT:
            /* If current item has submenu, open it */
            if (menu->items[dropdown_state.selected_item].submenu) {
                dropdown_state.submenu_open = true;
                dropdown_state.submenu_selected = 0;
            } else {
                /* Close dropdown and move to next top menu */
                dropdown_close();
                if (g_state.current_menu < 5) {  /* NUM_TOP_MENUS - 1 */
                    g_state.current_menu++;
                }
            }
            return true;
            
        case 27:  /* ESC */
            dropdown_close();
            return true;
            
        case '\n':
        case '\r':
            {
                MenuItem *item = &menu->items[dropdown_state.selected_item];
                if (item->submenu) {
                    /* Open submenu */
                    dropdown_state.submenu_open = true;
                    dropdown_state.submenu_selected = 0;
                } else if (item->action) {
                    /* Execute action - stay in menu */
                    item->action();
                }
            }
            return true;
            
        default:
            /* Check for hotkey */
            for (int i = 0; i < menu->item_count; i++) {
                const char *hotkey = menu->items[i].hotkey;
                if (hotkey && toupper(ch) == toupper(hotkey[0])) {
                    dropdown_state.selected_item = i;
                    MenuItem *item = &menu->items[i];
                    if (item->submenu) {
                        dropdown_state.submenu_open = true;
                        dropdown_state.submenu_selected = 0;
                    } else if (item->action) {
                        /* Execute action - stay in menu */
                        item->action();
                    }
                    return true;
                }
            }
            break;
    }
    
    return false;
}
