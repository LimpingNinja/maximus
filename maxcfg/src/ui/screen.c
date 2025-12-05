/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * screen.c - Screen management for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <string.h>
#include "maxcfg.h"
#include "ui.h"

void init_colors(void)
{
    start_color();
    use_default_colors();

    /* Define color pairs matching MAXTEL style */
    
    /* CP_SCREEN_BG: Cyan on black for shaded background (like MAXTEL) */
    init_pair(CP_SCREEN_BG, COLOR_CYAN, COLOR_BLACK);
    
    /* CP_TITLE_BAR: Black on grey/white (top bar) */
    init_pair(CP_TITLE_BAR, COLOR_BLACK, COLOR_WHITE);
    
    /* CP_MENU_BAR: Grey text on black (normal menu items) */
    init_pair(CP_MENU_BAR, COLOR_WHITE, COLOR_BLACK);
    
    /* CP_MENU_HOTKEY: Bold yellow on black (hotkey letters) */
    init_pair(CP_MENU_HOTKEY, COLOR_YELLOW, COLOR_BLACK);
    
    /* CP_MENU_HIGHLIGHT: Bold yellow on blue (highlighted menu item) */
    init_pair(CP_MENU_HIGHLIGHT, COLOR_YELLOW, COLOR_BLUE);
    
    /* CP_DROPDOWN: Grey text on black (dropdown items) */
    init_pair(CP_DROPDOWN, COLOR_WHITE, COLOR_BLACK);
    
    /* CP_DROPDOWN_HIGHLIGHT: Bold yellow on blue (selected dropdown item) */
    init_pair(CP_DROPDOWN_HIGHLIGHT, COLOR_YELLOW, COLOR_BLUE);
    
    /* CP_FORM_BG: Default on black */
    init_pair(CP_FORM_BG, COLOR_WHITE, COLOR_BLACK);
    
    /* CP_FORM_LABEL: Cyan on black */
    init_pair(CP_FORM_LABEL, COLOR_CYAN, COLOR_BLACK);
    
    /* CP_FORM_VALUE: Yellow on black */
    init_pair(CP_FORM_VALUE, COLOR_YELLOW, COLOR_BLACK);
    
    /* CP_FORM_HIGHLIGHT: Black on white */
    init_pair(CP_FORM_HIGHLIGHT, COLOR_BLACK, COLOR_WHITE);
    
    /* CP_STATUS_BAR: Black on grey/white (bottom bar, same as title) */
    init_pair(CP_STATUS_BAR, COLOR_BLACK, COLOR_WHITE);
    
    /* CP_DIALOG_BORDER: Cyan on black */
    init_pair(CP_DIALOG_BORDER, COLOR_CYAN, COLOR_BLACK);
    
    /* CP_DIALOG_TEXT: Yellow on black */
    init_pair(CP_DIALOG_TEXT, COLOR_YELLOW, COLOR_BLACK);
    
    /* CP_DIALOG_TITLE: White on black (use with A_BOLD) */
    init_pair(CP_DIALOG_TITLE, COLOR_WHITE, COLOR_BLACK);
    
    /* CP_DIALOG_MSG: Brown/Yellow on black */
    init_pair(CP_DIALOG_MSG, COLOR_YELLOW, COLOR_BLACK);
    
    /* CP_DIALOG_BTN_BRACKET: Cyan on black */
    init_pair(CP_DIALOG_BTN_BRACKET, COLOR_CYAN, COLOR_BLACK);
    
    /* CP_DIALOG_BTN_HOTKEY: Yellow on black (use with A_BOLD) */
    init_pair(CP_DIALOG_BTN_HOTKEY, COLOR_YELLOW, COLOR_BLACK);
    
    /* CP_DIALOG_BTN_TEXT: Grey/white on black (dim) */
    init_pair(CP_DIALOG_BTN_TEXT, COLOR_WHITE, COLOR_BLACK);
    
    /* CP_DIALOG_BTN_SEL: White on blue (use with A_BOLD) */
    init_pair(CP_DIALOG_BTN_SEL, COLOR_WHITE, COLOR_BLUE);
    
    /* CP_ERROR: Red on black */
    init_pair(CP_ERROR, COLOR_RED, COLOR_BLACK);
    
    /* CP_HELP: White on blue */
    init_pair(CP_HELP, COLOR_WHITE, COLOR_BLUE);
}

void screen_init(void)
{
    /* Set ESCDELAY before initscr to reduce ESC key lag */
    /* Default is 1000ms, we want it much faster */
    set_escdelay(25);
    
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);  /* Hide cursor */
    
    if (has_colors()) {
        init_colors();
    }
    
    /* Initial screen draw */
    screen_refresh();
}

void screen_cleanup(void)
{
    endwin();
}

void draw_title_bar(void)
{
    char title[128];
    int padding;

    /* Build title string */
    snprintf(title, sizeof(title), " %s              Maximus Configuration Editor",
             MAXCFG_TITLE);

    /* Calculate padding to right-align "F1=Help" */
    padding = COLS - strlen(title) - 10;

    attron(COLOR_PAIR(CP_TITLE_BAR));
    
    /* Clear the line */
    move(TITLE_ROW, 0);
    for (int i = 0; i < COLS; i++) {
        addch(' ');
    }
    
    /* Draw title */
    mvprintw(TITLE_ROW, 0, "%s", title);
    
    /* Right-align F1=Help */
    if (padding > 0) {
        mvprintw(TITLE_ROW, COLS - 9, "F1=Help ");
    }
    
    attroff(COLOR_PAIR(CP_TITLE_BAR));
    
    wnoutrefresh(stdscr);
}

void draw_status_bar(const char *text)
{
    attron(COLOR_PAIR(CP_STATUS_BAR));
    
    /* Clear the line */
    move(STATUS_ROW, 0);
    for (int i = 0; i < COLS; i++) {
        addch(' ');
    }
    
    /* Center the text */
    if (text) {
        int len = strlen(text);
        int x = (COLS - len) / 2;
        if (x < 0) x = 0;
        mvprintw(STATUS_ROW, x, "%s", text);
    }
    
    attroff(COLOR_PAIR(CP_STATUS_BAR));
    
    wnoutrefresh(stdscr);
}

void draw_work_area(void)
{
    /* Fill work area with cyan background using light shade character */
    /* Unicode light shade: ░ (U+2591), or ACS_CKBOARD for compatibility */
    attron(COLOR_PAIR(CP_SCREEN_BG));
    
    for (int y = WORK_AREA_TOP; y < STATUS_ROW; y++) {
        move(y, 0);
        for (int x = 0; x < COLS; x++) {
            /* Use ACS_CKBOARD (checkerboard/shade) for the background pattern */
            addch(ACS_CKBOARD);
        }
    }
    
    attroff(COLOR_PAIR(CP_SCREEN_BG));
    
    wnoutrefresh(stdscr);
}

void screen_refresh(void)
{
    draw_title_bar();
    draw_work_area();
    draw_status_bar(NULL);
    doupdate();
}

void draw_box(int y, int x, int height, int width, int color_pair)
{
    attron(COLOR_PAIR(color_pair));
    
    /* Top border */
    mvaddch(y, x, ACS_ULCORNER);
    for (int i = 1; i < width - 1; i++) {
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
    
    attroff(COLOR_PAIR(color_pair));
}
