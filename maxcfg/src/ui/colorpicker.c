/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * colorpicker.c - Color picker for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <string.h>
#include <stdlib.h>
#include "maxcfg.h"
#include "ui.h"

/* DOS/ANSI color names */
static const char *color_names[16] = {
    "Black",        /* 0 */
    "Blue",         /* 1 */
    "Green",        /* 2 */
    "Cyan",         /* 3 */
    "Red",          /* 4 */
    "Magenta",      /* 5 */
    "Brown",        /* 6 */
    "Light Gray",   /* 7 */
    "Dark Gray",    /* 8 */
    "Light Blue",   /* 9 */
    "Light Green",  /* 10 */
    "Light Cyan",   /* 11 */
    "Light Red",    /* 12 */
    "Light Magenta",/* 13 */
    "Yellow",       /* 14 */
    "White"         /* 15 */
};

/* Map DOS colors to ncurses colors */
static int dos_to_ncurses_fg[16] = {
    COLOR_BLACK,    /* 0: Black */
    COLOR_BLUE,     /* 1: Blue */
    COLOR_GREEN,    /* 2: Green */
    COLOR_CYAN,     /* 3: Cyan */
    COLOR_RED,      /* 4: Red */
    COLOR_MAGENTA,  /* 5: Magenta */
    COLOR_YELLOW,   /* 6: Brown (use yellow, dim) */
    COLOR_WHITE,    /* 7: Light Gray */
    COLOR_BLACK,    /* 8: Dark Gray (black + bold) */
    COLOR_BLUE,     /* 9: Light Blue (blue + bold) */
    COLOR_GREEN,    /* 10: Light Green (green + bold) */
    COLOR_CYAN,     /* 11: Light Cyan (cyan + bold) */
    COLOR_RED,      /* 12: Light Red (red + bold) */
    COLOR_MAGENTA,  /* 13: Light Magenta (magenta + bold) */
    COLOR_YELLOW,   /* 14: Yellow (yellow + bold) */
    COLOR_WHITE     /* 15: White (white + bold) */
};

/* Whether color needs A_BOLD for bright */
static int dos_color_bold[16] = {
    0, 0, 0, 0, 0, 0, 0, 0,  /* 0-7: normal */
    1, 1, 1, 1, 1, 1, 1, 1   /* 8-15: bold/bright */
};

/* Color pair base for picker (we'll init these) */
#define CP_PICKER_BASE 30

/* Initialize color pairs for picker display */
void colorpicker_init(void)
{
    /* Create color pairs for each DOS foreground on black background */
    for (int i = 0; i < 16; i++) {
        init_pair(CP_PICKER_BASE + i, dos_to_ncurses_fg[i], COLOR_BLACK);
    }
}

/* Get the display name for a color */
const char *color_get_name(int color)
{
    if (color >= 0 && color < 16) {
        return color_names[color];
    }
    return "Unknown";
}

/* Show a color picker dropdown and return selected color (-1 if cancelled) */
int colorpicker_select(int current, int screen_y, int screen_x)
{
    int selected = (current >= 0 && current < 16) ? current : 0;
    int ch;
    bool done = false;
    int result = -1;
    
    /* Picker dimensions */
    int width = 18;  /* Enough for "Light Magenta" + padding */
    int height = 18; /* 16 colors + border */
    
    /* Adjust position to fit on screen */
    if (screen_x + width > COLS - 2) {
        screen_x = COLS - width - 2;
    }
    if (screen_y + height > LINES - 2) {
        screen_y = LINES - height - 2;
    }
    
    int x = screen_x;
    int y = screen_y;
    
    while (!done) {
        /* Draw border */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        
        /* Top */
        mvaddch(y, x, ACS_ULCORNER);
        for (int i = 1; i < width - 1; i++) {
            addch(ACS_HLINE);
        }
        addch(ACS_URCORNER);
        
        /* Sides and fill */
        for (int i = 1; i < height - 1; i++) {
            mvaddch(y + i, x, ACS_VLINE);
            attron(COLOR_PAIR(CP_FORM_BG));
            for (int j = 1; j < width - 1; j++) {
                addch(' ');
            }
            attroff(COLOR_PAIR(CP_FORM_BG));
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(y + i, x + width - 1, ACS_VLINE);
        }
        
        /* Bottom */
        mvaddch(y + height - 1, x, ACS_LLCORNER);
        for (int i = 1; i < width - 1; i++) {
            addch(ACS_HLINE);
        }
        addch(ACS_LRCORNER);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        /* Draw color options */
        for (int i = 0; i < 16; i++) {
            int item_y = y + 1 + i;
            
            if (i == selected) {
                /* Highlight bar */
                attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
                mvprintw(item_y, x + 1, " %-*s ", width - 4, color_names[i]);
                attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
            } else {
                /* Show color in its actual color */
                attron(COLOR_PAIR(CP_PICKER_BASE + i));
                if (dos_color_bold[i]) {
                    attron(A_BOLD);
                }
                /* Black text needs special handling */
                if (i == 0) {
                    /* Show (Black) in parentheses since it's invisible */
                    mvprintw(item_y, x + 1, " (%-*s)", width - 5, color_names[i]);
                } else {
                    mvprintw(item_y, x + 1, " %-*s ", width - 4, color_names[i]);
                }
                if (dos_color_bold[i]) {
                    attroff(A_BOLD);
                }
                attroff(COLOR_PAIR(CP_PICKER_BASE + i));
            }
        }
        
        /* Show arrow pointing to current selection */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvprintw(y + 1 + current, x - 3, "-->");
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        refresh();
        
        ch = getch();
        
        switch (ch) {
            case KEY_UP:
                if (selected > 0) selected--;
                break;
                
            case KEY_DOWN:
                if (selected < 15) selected++;
                break;
                
            case KEY_HOME:
                selected = 0;
                break;
                
            case KEY_END:
                selected = 15;
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
    
    return result;
}

/* Show a full color picker grid (foreground + background) */
int colorpicker_select_full(int current_fg, int current_bg, int *out_fg, int *out_bg)
{
    int sel_fg = (current_fg >= 0 && current_fg < 16) ? current_fg : 7;
    int sel_bg = (current_bg >= 0 && current_bg < 8) ? current_bg : 0;
    int ch;
    bool done = false;
    int result = 0;  /* 0 = cancelled, 1 = selected */
    
    /* Grid dimensions: 16 columns (fg), 8 rows (bg), 1 char per color */
    int grid_w = 16 + 4;      /* 16 colors + margins + borders */
    int grid_h = 8 + 5;       /* 8 rows + margins + borders + instruction line */
    
    int x = (COLS - grid_w) / 2;
    int y = (LINES - grid_h) / 2;
    
    while (!done) {
        /* Draw border */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        
        /* Top border with title */
        mvaddch(y, x, ACS_ULCORNER);
        addch(ACS_HLINE);
        addch(' ');
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("Colors");
        attroff(COLOR_PAIR(CP_MENU_BAR));
        
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        addch(' ');
        for (int i = 10; i < grid_w - 1; i++) {
            addch(ACS_HLINE);
        }
        addch(ACS_URCORNER);
        
        /* Sides */
        for (int i = 1; i < grid_h - 1; i++) {
            mvaddch(y + i, x, ACS_VLINE);
            mvaddch(y + i, x + grid_w - 1, ACS_VLINE);
        }
        
        /* Bottom */
        mvaddch(y + grid_h - 1, x, ACS_LLCORNER);
        for (int i = 1; i < grid_w - 1; i++) {
            addch(ACS_HLINE);
        }
        addch(ACS_LRCORNER);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        /* Fill background */
        attron(COLOR_PAIR(CP_FORM_BG));
        for (int row = 1; row < grid_h - 1; row++) {
            mvhline(y + row, x + 1, ' ', grid_w - 2);
        }
        attroff(COLOR_PAIR(CP_FORM_BG));
        
        /* Draw color grid - 1 char per color, with 1 space margin */
        for (int bg = 0; bg < 8; bg++) {
            for (int fg = 0; fg < 16; fg++) {
                int cell_x = x + 2 + fg;       /* 1 space margin from border */
                int cell_y = y + 2 + bg;       /* 1 space margin from border */
                
                /* Create temp color pair for this combo */
                int pair_num = CP_PICKER_BASE + 16 + bg * 16 + fg;
                init_pair(pair_num, dos_to_ncurses_fg[fg], dos_to_ncurses_fg[bg]);
                
                attron(COLOR_PAIR(pair_num));
                if (dos_color_bold[fg]) attron(A_BOLD);
                
                mvaddch(cell_y, cell_x, 'X');
                
                if (dos_color_bold[fg]) attroff(A_BOLD);
                attroff(COLOR_PAIR(pair_num));
            }
        }
        
        /* Draw selection box AFTER all colors so it's not overwritten */
        {
            int cell_x = x + 2 + sel_fg;
            int cell_y = y + 2 + sel_bg;
            
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            /* Top of box */
            mvaddch(cell_y - 1, cell_x - 1, ACS_ULCORNER);
            addch(ACS_HLINE);
            addch(ACS_URCORNER);
            /* Sides */
            mvaddch(cell_y, cell_x - 1, ACS_VLINE);
            mvaddch(cell_y, cell_x + 1, ACS_VLINE);
            /* Bottom of box */
            mvaddch(cell_y + 1, cell_x - 1, ACS_LLCORNER);
            addch(ACS_HLINE);
            addch(ACS_LRCORNER);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        
        /* Instructions */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvprintw(y + grid_h - 2, x + 2, "Arrows=Move  Enter=OK");
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        refresh();
        
        ch = getch();
        
        switch (ch) {
            case KEY_LEFT:
                if (sel_fg > 0) sel_fg--;
                break;
                
            case KEY_RIGHT:
                if (sel_fg < 15) sel_fg++;
                break;
                
            case KEY_UP:
                if (sel_bg > 0) sel_bg--;
                break;
                
            case KEY_DOWN:
                if (sel_bg < 7) sel_bg++;
                break;
                
            case KEY_PPAGE:  /* Page Up */
                if (sel_bg > 0) sel_bg--;
                break;
                
            case KEY_NPAGE:  /* Page Down */
                if (sel_bg < 7) sel_bg++;
                break;
                
            case '\n':
            case '\r':
                *out_fg = sel_fg;
                *out_bg = sel_bg;
                result = 1;
                done = true;
                break;
                
            case 27:  /* ESC */
                result = 0;
                done = true;
                break;
        }
    }
    
    return result;
}
