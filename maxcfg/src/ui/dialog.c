/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * dialog.c - Pop-up dialogs for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <string.h>
#include "maxcfg.h"
#include "ui.h"

/*
 * Draw a dialog box with title
 */
static void draw_dialog_box(int y, int x, int height, int width, const char *title)
{
    /* Fill background */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    for (int row = 0; row < height; row++) {
        move(y + row, x);
        for (int col = 0; col < width; col++) {
            addch(' ');
        }
    }
    
    /* Draw border with title bracket */
    /* Top border */
    mvaddch(y, x, ACS_ULCORNER);
    addch(ACS_HLINE);
    addch('[');
    
    /* Title indicator (■) - using # as placeholder */
    attron(A_BOLD);
    addch('#');
    attroff(A_BOLD);
    
    addch(']');
    for (int i = 5; i < width - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_URCORNER);
    
    /* Sides */
    for (int i = 1; i < height - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + width - 1, ACS_VLINE);
    }
    
    /* Bottom border */
    mvaddch(y + height - 1, x, ACS_LLCORNER);
    for (int i = 1; i < width - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_LRCORNER);
    
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
}

/*
 * Show the save/abort/return dialog
 * Returns: DIALOG_SAVE_EXIT, DIALOG_ABORT, DIALOG_RETURN, or DIALOG_CANCEL
 */
DialogResult dialog_save_prompt(void)
{
    const char *options[] = {
        "Save and Exit",
        "Abort without saving",
        "Return to edit screen"
    };
    const int num_options = 3;
    int selected = 0;
    
    /* Dialog dimensions */
    int width = 30;
    int height = num_options + 4;  /* options + border + status line */
    int x = (COLS - width) / 2;
    int y = (LINES - height) / 2;
    
    /* Save screen area (simplified - just redraw after) */
    
    int ch;
    bool done = false;
    DialogResult result = DIALOG_CANCEL;
    
    while (!done) {
        /* Draw dialog box */
        draw_dialog_box(y, x, height, width, NULL);
        
        /* Draw options */
        for (int i = 0; i < num_options; i++) {
            if (i == selected) {
                /* Highlighted: bold yellow on blue */
                attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
                mvprintw(y + 1 + i, x + 2, " %-*s ", width - 6, options[i]);
                attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
            } else {
                /* Normal: bold yellow first char, grey rest */
                const char *opt = options[i];
                move(y + 1 + i, x + 2);
                addch(' ');
                attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
                addch(opt[0]);
                attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
                attron(COLOR_PAIR(CP_DIALOG_TEXT));
                printw("%-*s ", width - 8, opt + 1);
                attroff(COLOR_PAIR(CP_DIALOG_TEXT));
            }
        }
        
        /* Draw bottom status */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvprintw(y + height - 2, x + 1, "%*s", width - 2, "");
        int status_x = x + (width - 12) / 2;
        mvprintw(y + height - 2, status_x, "ENTER=Select");
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        doupdate();
        
        ch = getch();
        
        switch (ch) {
            case KEY_UP:
                if (selected > 0) selected--;
                break;
                
            case KEY_DOWN:
                if (selected < num_options - 1) selected++;
                break;
                
            case '\n':
            case '\r':
                result = (DialogResult)selected;
                done = true;
                break;
                
            case 27:  /* ESC */
                result = DIALOG_CANCEL;
                done = true;
                break;
                
            case 's':
            case 'S':
                result = DIALOG_SAVE_EXIT;
                done = true;
                break;
                
            case 'a':
            case 'A':
                result = DIALOG_ABORT;
                done = true;
                break;
                
            case 'r':
            case 'R':
                result = DIALOG_RETURN;
                done = true;
                break;
        }
    }
    
    /* Redraw screen - preserve menu state */
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    
    return result;
}

/*
 * Show a simple message box
 */
void dialog_message(const char *title, const char *message)
{
    /* Calculate dimensions based on message */
    int msg_len = strlen(message);
    int title_len = title ? strlen(title) : 0;
    
    /* Find longest line and count lines */
    int max_line = 0;
    int num_lines = 1;
    int current_line = 0;
    
    for (int i = 0; i <= msg_len; i++) {
        if (message[i] == '\n' || message[i] == '\0') {
            if (current_line > max_line) {
                max_line = current_line;
            }
            if (message[i] == '\n') {
                num_lines++;
            }
            current_line = 0;
        } else {
            current_line++;
        }
    }
    
    int width = (max_line > title_len ? max_line : title_len) + 6;
    if (width < 30) width = 30;
    if (width > COLS - 4) width = COLS - 4;
    
    int height = num_lines + 5;
    if (height > LINES - 4) height = LINES - 4;
    
    int x = (COLS - width) / 2;
    int y = (LINES - height) / 2;
    
    /* Draw dialog */
    draw_dialog_box(y, x, height, width, title);
    
    /* Draw title if provided */
    if (title) {
        attron(COLOR_PAIR(CP_DIALOG_TEXT) | A_BOLD);
        int title_x = x + (width - title_len) / 2;
        mvprintw(y + 1, title_x, "%s", title);
        attroff(COLOR_PAIR(CP_DIALOG_TEXT) | A_BOLD);
    }
    
    /* Draw message */
    attron(COLOR_PAIR(CP_DIALOG_TEXT));
    int line = 0;
    int col = 0;
    int start_y = y + (title ? 3 : 2);
    
    for (int i = 0; i <= msg_len; i++) {
        if (message[i] == '\n' || message[i] == '\0') {
            line++;
            col = 0;
        } else {
            if (line < height - 4) {
                mvaddch(start_y + line, x + 2 + col, message[i]);
            }
            col++;
        }
    }
    attroff(COLOR_PAIR(CP_DIALOG_TEXT));
    
    /* Draw "Press any key" prompt */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    const char *prompt = "Press any key to continue";
    int prompt_x = x + (width - strlen(prompt)) / 2;
    mvprintw(y + height - 2, prompt_x, "%s", prompt);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    doupdate();
    
    /* Wait for key */
    getch();
    
    /* Redraw screen - don't clear, just redraw over the dialog */
    touchwin(stdscr);
    wnoutrefresh(stdscr);
}

/*
 * Draw a styled button: [ Yes ] or [ No ]
 * When selected: blue background, bold white text
 * When not selected: cyan brackets, bold yellow hotkey, grey rest
 */
static void draw_styled_button(int y, int x, const char *text, bool selected)
{
    if (selected) {
        /* Selected: blue background, bold white */
        attron(COLOR_PAIR(CP_DIALOG_BTN_SEL) | A_BOLD);
        mvprintw(y, x, "[ %s ]", text);
        attroff(COLOR_PAIR(CP_DIALOG_BTN_SEL) | A_BOLD);
    } else {
        /* Not selected: styled with individual colors */
        /* Cyan bracket */
        attron(COLOR_PAIR(CP_DIALOG_BTN_BRACKET));
        mvaddch(y, x, '[');
        mvaddch(y, x + 2 + strlen(text) + 1, ']');
        attroff(COLOR_PAIR(CP_DIALOG_BTN_BRACKET));
        
        /* Space */
        mvaddch(y, x + 1, ' ');
        mvaddch(y, x + 2 + strlen(text), ' ');
        
        /* Bold yellow hotkey (first char) */
        attron(COLOR_PAIR(CP_DIALOG_BTN_HOTKEY) | A_BOLD);
        mvaddch(y, x + 2, text[0]);
        attroff(COLOR_PAIR(CP_DIALOG_BTN_HOTKEY) | A_BOLD);
        
        /* Grey rest of text */
        attron(COLOR_PAIR(CP_DIALOG_BTN_TEXT));
        mvprintw(y, x + 3, "%s", text + 1);
        attroff(COLOR_PAIR(CP_DIALOG_BTN_TEXT));
    }
}

/*
 * Show a confirmation dialog
 * Returns true if confirmed (Yes), false if cancelled (No)
 */
bool dialog_confirm(const char *title, const char *message)
{
    int selected = 1;  /* Default to No */
    
    /* Calculate dimensions */
    int msg_len = strlen(message);
    int title_len = title ? strlen(title) : 0;
    int width = (msg_len > title_len ? msg_len : title_len) + 6;
    if (width < 30) width = 30;
    if (width > COLS - 4) width = COLS - 4;
    
    int height = 8;
    int x = (COLS - width) / 2;
    int y = (LINES - height) / 2;
    
    int ch;
    bool done = false;
    bool result = false;
    
    while (!done) {
        /* Draw dialog box (border only) */
        draw_dialog_box(y, x, height, width, NULL);
        
        /* Draw title - bold white, centered */
        if (title) {
            attron(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
            int title_x = x + (width - title_len) / 2;
            mvprintw(y + 1, title_x, "%s", title);
            attroff(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
        }
        
        /* Draw message - brown/yellow (non-bold yellow = brown) */
        attron(COLOR_PAIR(CP_DIALOG_MSG));
        mvprintw(y + 3, x + 3, "%s", message);
        attroff(COLOR_PAIR(CP_DIALOG_MSG));
        
        /* Draw buttons */
        int btn_y = y + 5;
        int yes_x = x + width / 2 - 10;
        int no_x = x + width / 2 + 3;
        
        draw_styled_button(btn_y, yes_x, "Yes", selected == 0);
        draw_styled_button(btn_y, no_x, "No", selected == 1);
        
        doupdate();
        
        ch = getch();
        
        switch (ch) {
            case KEY_LEFT:
            case KEY_RIGHT:
            case '\t':
                selected = 1 - selected;
                break;
                
            case '\n':
            case '\r':
                result = (selected == 0);
                done = true;
                break;
                
            case 27:  /* ESC */
                result = false;
                done = true;
                break;
                
            case 'y':
            case 'Y':
                result = true;
                done = true;
                break;
                
            case 'n':
            case 'N':
                result = false;
                done = true;
                break;
        }
    }
    
    /* Redraw screen - preserve menu state */
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    
    return result;
}

/*
 * Show an option picker dialog
 * Returns: selected index, or -1 if cancelled
 */
int dialog_option_picker(const char *title, const char **options, int current_idx)
{
    /* Count options and find max width */
    int num_options = 0;
    int max_width = title ? strlen(title) : 0;
    
    while (options[num_options]) {
        int len = strlen(options[num_options]);
        if (len > max_width) max_width = len;
        num_options++;
    }
    
    if (num_options == 0) return -1;
    
    /* Dialog dimensions */
    int width = max_width + 6;  /* padding + border */
    if (width < 20) width = 20;
    int height = num_options + 3;  /* options + border + status */
    
    /* Cap height if too many options */
    int max_visible = LINES - 8;
    if (height > max_visible + 3) height = max_visible + 3;
    int visible_options = height - 3;
    
    int x = (COLS - width) / 2;
    int y = (LINES - height) / 2;
    
    int selected = current_idx >= 0 && current_idx < num_options ? current_idx : 0;
    int scroll_offset = 0;
    
    /* Adjust scroll to show selected item */
    if (selected >= visible_options) {
        scroll_offset = selected - visible_options + 1;
    }
    
    bool done = false;
    int result = -1;
    
    curs_set(0);
    
    while (!done) {
        /* Draw dialog box */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        
        /* Fill background */
        for (int row = 0; row < height; row++) {
            mvhline(y + row, x, ' ', width);
        }
        
        /* Top border with title */
        mvaddch(y, x, ACS_ULCORNER);
        for (int i = 1; i < width - 1; i++) {
            addch(ACS_HLINE);
        }
        addch(ACS_URCORNER);
        
        if (title) {
            int title_x = x + (width - strlen(title)) / 2;
            mvaddch(y, title_x - 1, ' ');
            attron(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
            mvprintw(y, title_x, "%s", title);
            attroff(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            addch(' ');
        }
        
        /* Side borders */
        for (int i = 1; i < height - 1; i++) {
            mvaddch(y + i, x, ACS_VLINE);
            mvaddch(y + i, x + width - 1, ACS_VLINE);
        }
        
        /* Bottom border with status */
        mvaddch(y + height - 1, x, ACS_LLCORNER);
        addch(ACS_HLINE);
        addch(' ');
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        printw("ENTER");
        attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("=Sel");
        attroff(COLOR_PAIR(CP_MENU_BAR));
        
        /* Only show ESC=Cancel if there's room (width >= 28) */
        if (width >= 28) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            printw(" ");
            addch(ACS_HLINE);
            printw(" ");
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
            
            attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
            printw("ESC");
            attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
            attron(COLOR_PAIR(CP_MENU_BAR));
            printw("=Cancel");
            attroff(COLOR_PAIR(CP_MENU_BAR));
        }
        
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        printw(" ");
        int cur_x = getcurx(stdscr);
        for (int i = cur_x; i < x + width - 1; i++) {
            addch(ACS_HLINE);
        }
        mvaddch(y + height - 1, x + width - 1, ACS_LRCORNER);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        /* Draw options */
        for (int i = 0; i < visible_options && (i + scroll_offset) < num_options; i++) {
            int opt_idx = i + scroll_offset;
            int row = y + 1 + i;
            
            if (opt_idx == selected) {
                attron(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
                mvprintw(row, x + 2, "%-*s", width - 4, options[opt_idx]);
                attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
            } else {
                attron(COLOR_PAIR(CP_DIALOG_TEXT));
                mvprintw(row, x + 2, "%-*s", width - 4, options[opt_idx]);
                attroff(COLOR_PAIR(CP_DIALOG_TEXT));
            }
        }
        
        /* Scroll indicators */
        if (scroll_offset > 0) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(y + 1, x + width - 2, ACS_UARROW);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        if (scroll_offset + visible_options < num_options) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(y + height - 2, x + width - 2, ACS_DARROW);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        
        refresh();
        
        int ch = getch();
        switch (ch) {
            case KEY_UP:
            case 'k':
                if (selected > 0) {
                    selected--;
                    if (selected < scroll_offset) {
                        scroll_offset = selected;
                    }
                }
                break;
                
            case KEY_DOWN:
            case 'j':
                if (selected < num_options - 1) {
                    selected++;
                    if (selected >= scroll_offset + visible_options) {
                        scroll_offset = selected - visible_options + 1;
                    }
                }
                break;
                
            case KEY_HOME:
                selected = 0;
                scroll_offset = 0;
                break;
                
            case KEY_END:
                selected = num_options - 1;
                if (selected >= visible_options) {
                    scroll_offset = selected - visible_options + 1;
                }
                break;
                
            case '\n':
            case '\r':
                result = selected;
                done = true;
                break;
                
            case 27:  /* ESC */
                result = -1;
                done = true;
                break;
        }
    }
    
    /* Redraw screen */
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    
    return result;
}
