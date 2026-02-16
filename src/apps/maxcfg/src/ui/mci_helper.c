/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * mci_helper.c - MCI code reference helper dialog for the language editor
 *
 * Presents a tabbed popup with all MCI code categories.  Color codes
 * render an inline color sample using ncurses color pairs.  Based on
 * the picker_with_help pattern used by the command picker.
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "maxcfg.h"
#include "ui.h"
#include "menu_preview.h"
#include "mci_helper.h"

/* ======================================================================== */
/* MCI code reference data                                                  */
/* ======================================================================== */

/** @brief One entry in the MCI code reference table. */
typedef struct {
    const char *code;       /**< MCI code string (e.g. "|14") */
    const char *desc;       /**< Human description */
    const char *category;   /**< Tab category name */
    int  color_idx;         /**< For color codes: DOS color 0-15 fg, 16-23 bg; -1 = not a color */
} MciRef;

static const MciRef mci_refs[] = {
    /* ---- Foreground Colors ---- */
    { "|00", "Black",          "FG Colors",  0 },
    { "|01", "Blue",           "FG Colors",  1 },
    { "|02", "Green",          "FG Colors",  2 },
    { "|03", "Cyan",           "FG Colors",  3 },
    { "|04", "Red",            "FG Colors",  4 },
    { "|05", "Magenta",        "FG Colors",  5 },
    { "|06", "Brown",          "FG Colors",  6 },
    { "|07", "Light Grey",     "FG Colors",  7 },
    { "|08", "Dark Grey",      "FG Colors",  8 },
    { "|09", "Light Blue",     "FG Colors",  9 },
    { "|10", "Light Green",    "FG Colors", 10 },
    { "|11", "Light Cyan",     "FG Colors", 11 },
    { "|12", "Light Red",      "FG Colors", 12 },
    { "|13", "Light Magenta",  "FG Colors", 13 },
    { "|14", "Yellow",         "FG Colors", 14 },
    { "|15", "Bright White",   "FG Colors", 15 },

    /* ---- Background Colors ---- */
    { "|16", "Black BG",       "BG Colors", 16 },
    { "|17", "Blue BG",        "BG Colors", 17 },
    { "|18", "Green BG",       "BG Colors", 18 },
    { "|19", "Cyan BG",        "BG Colors", 19 },
    { "|20", "Red BG",         "BG Colors", 20 },
    { "|21", "Magenta BG",     "BG Colors", 21 },
    { "|22", "Brown BG",       "BG Colors", 22 },
    { "|23", "Grey BG",        "BG Colors", 23 },

    /* ---- Info Codes ---- */
    { "|BN", "System/BBS name",              "Info", -1 },
    { "|SN", "Sysop name",                   "Info", -1 },
    { "|UN", "User name",                    "Info", -1 },
    { "|UH", "User alias/handle",            "Info", -1 },
    { "|UR", "User real name",               "Info", -1 },
    { "|UC", "User city",                    "Info", -1 },
    { "|UP", "User phone",                   "Info", -1 },
    { "|UD", "User data phone",              "Info", -1 },
    { "|U#", "User number",                  "Info", -1 },
    { "|CS", "Times called (lifetime)",      "Info", -1 },
    { "|CT", "Calls today",                  "Info", -1 },
    { "|MP", "Messages posted",              "Info", -1 },
    { "|DK", "KB downloaded (lifetime)",     "Info", -1 },
    { "|FK", "KB uploaded (lifetime)",       "Info", -1 },
    { "|DL", "Files downloaded (lifetime)",  "Info", -1 },
    { "|FU", "Files uploaded (lifetime)",    "Info", -1 },
    { "|DT", "KB downloaded today",          "Info", -1 },
    { "|TL", "Time left (minutes)",          "Info", -1 },
    { "|US", "Screen length",               "Info", -1 },
    { "|TE", "Terminal emulation",           "Info", -1 },
    { "|MB", "Current message area",         "Info", -1 },
    { "|MD", "Current message area (alt)",   "Info", -1 },
    { "|FB", "Current file area",            "Info", -1 },
    { "|FD", "Current file area (alt)",      "Info", -1 },
    { "|DA", "Current date",                 "Info", -1 },
    { "|TM", "Current time (HH:MM)",        "Info", -1 },
    { "|TS", "Current time (HH:MM:SS)",     "Info", -1 },

    /* ---- Terminal Control ---- */
    { "|CL", "Clear screen",                 "Terminal", -1 },
    { "|CR", "Carriage return + line feed",  "Terminal", -1 },
    { "|CD", "Reset to default color",       "Terminal", -1 },
    { "|BS", "Destructive backspace",        "Terminal", -1 },
    { "|SA", "Save cursor + attributes",     "Terminal", -1 },
    { "|RA", "Restore cursor + attributes",  "Terminal", -1 },
    { "|SS", "Save screen (alt buffer)",     "Terminal", -1 },
    { "|RS", "Restore screen (main buffer)", "Terminal", -1 },
    { "|LC", "Load last color mode",         "Terminal", -1 },
    { "|LF", "Load last font",              "Terminal", -1 },
    { "|&&", "Cursor Position Report (DSR)", "Terminal", -1 },
    { "|PD", "Pad space before next code",   "Terminal", -1 },
    { "||",  "Literal pipe character",       "Terminal", -1 },

    /* ---- Format Operators ---- */
    { "$R##",  "Right-pad to ## cols (space)",   "Format", -1 },
    { "$L##",  "Left-pad to ## cols (space)",    "Format", -1 },
    { "$C##",  "Center-pad to ## cols (space)",  "Format", -1 },
    { "$T##",  "Trim to ## visible chars",       "Format", -1 },
    { "$r##X", "Right-pad to ## cols (char X)",  "Format", -1 },
    { "$l##X", "Left-pad to ## cols (char X)",   "Format", -1 },
    { "$c##X", "Center-pad to ## cols (char X)", "Format", -1 },
    { "$D##C", "Repeat char C ## times",         "Format", -1 },
    { "$X##C", "Goto col ## with fill char C",   "Format", -1 },
    { "$$",    "Literal dollar sign",            "Format", -1 },

    /* ---- Positional Params ---- */
    { "|!1", "Positional param 1 (string)",  "Params", -1 },
    { "|!2", "Positional param 2",           "Params", -1 },
    { "|!3", "Positional param 3",           "Params", -1 },
    { "|!4", "Positional param 4",           "Params", -1 },
    { "|!5", "Positional param 5",           "Params", -1 },
    { "|!6", "Positional param 6",           "Params", -1 },
    { "|!7", "Positional param 7",           "Params", -1 },
    { "|!8", "Positional param 8",           "Params", -1 },
    { "|!9", "Positional param 9",           "Params", -1 },
    { "|!A", "Positional param 10",          "Params", -1 },
    { "|!B", "Positional param 11",          "Params", -1 },
    { "|!C", "Positional param 12",          "Params", -1 },
    { "|!D", "Positional param 13",          "Params", -1 },
    { "|!E", "Positional param 14",          "Params", -1 },
    { "|!F", "Positional param 15",          "Params", -1 },
    { "$D|!NC", "Repeat char C (param count)", "Params", -1 },
    { "$X|!NC", "Goto col (param) fill C",     "Params", -1 },

    /* ---- Cursor Control ---- */
    { "[X##", "Set cursor column to ##",     "Cursor", -1 },
    { "[Y##", "Set cursor row to ##",        "Cursor", -1 },
    { "[A##", "Move cursor up ## rows",      "Cursor", -1 },
    { "[B##", "Move cursor down ## rows",    "Cursor", -1 },
    { "[C##", "Move cursor right ## cols",   "Cursor", -1 },
    { "[D##", "Move cursor left ## cols",    "Cursor", -1 },
    { "[K",   "Clear to end of line",        "Cursor", -1 },

    /* ---- Theme Color Slots ---- */
    { "|tx", "Normal body text",              "Theme", -1 },
    { "|hi", "Emphasized text",               "Theme", -1 },
    { "|pr", "User-facing prompts",           "Theme", -1 },
    { "|in", "User keystroke echo",           "Theme", -1 },
    { "|tf", "Text input field foreground",   "Theme", -1 },
    { "|tb", "Text input field background",   "Theme", -1 },
    { "|hd", "Section headings",              "Theme", -1 },
    { "|lf", "Lightbar selected foreground",  "Theme", -1 },
    { "|lb", "Lightbar selected background",  "Theme", -1 },
    { "|er", "Error messages",                "Theme", -1 },
    { "|wn", "Warnings",                      "Theme", -1 },
    { "|ok", "Confirmations / success",       "Theme", -1 },
    { "|dm", "De-emphasized / help text",     "Theme", -1 },
    { "|fi", "File descriptions",             "Theme", -1 },
    { "|sy", "SysOp-only text",               "Theme", -1 },
    { "|qt", "Quoted message text",           "Theme", -1 },
    { "|br", "Box borders, dividers",         "Theme", -1 },
    { "|cd", "Reset to default theme color",  "Theme", -1 },

    { NULL, NULL, NULL, -1 }
};

/* ======================================================================== */
/* Category management                                                      */
/* ======================================================================== */

typedef struct {
    const char *name;
    int *indices;
    int count;
} MciCategory;

#define MCI_MAX_CATS 10

/** @brief Build category index arrays from the flat reference table. */
static int mci_build_categories(MciCategory cats[MCI_MAX_CATS], int num_refs)
{
    int num_cats = 0;
    for (int i = 0; i < num_refs; i++) {
        const char *cat = mci_refs[i].category;
        int found = -1;
        for (int j = 0; j < num_cats; j++) {
            if (strcmp(cats[j].name, cat) == 0) { found = j; break; }
        }
        if (found == -1 && num_cats < MCI_MAX_CATS) {
            cats[num_cats].name = cat;
            cats[num_cats].indices = calloc((size_t)num_refs, sizeof(int));
            cats[num_cats].indices[0] = i;
            cats[num_cats].count = 1;
            num_cats++;
        } else if (found >= 0) {
            cats[found].indices[cats[found].count++] = i;
        }
    }
    return num_cats;
}

static void mci_free_categories(MciCategory *cats, int num_cats)
{
    for (int i = 0; i < num_cats; i++)
        free(cats[i].indices);
}

/* ======================================================================== */
/* Color sample rendering helper                                            */
/* ======================================================================== */

/**
 * @brief Render a color sample swatch at the given screen position.
 *
 * For foreground colors (0-15): renders "[sample]" in that color on black bg.
 * For background colors (16-23): renders "[sample]" with white fg on that bg.
 */
static void render_color_sample(int screen_y, int screen_x, int color_idx)
{
    int fg, bg, attrs = 0;

    if (color_idx <= 15) {
        /* Foreground color sample */
        fg = color_idx & 0x0f;
        bg = 0; /* black bg */
        if (fg == 0) bg = 7; /* black on grey so it's visible */
        if (fg >= 8) { attrs = A_BOLD; fg -= 8; }
    } else {
        /* Background color sample */
        fg = 7; /* white text */
        bg = color_idx - 16;
        if (bg == 7) fg = 0; /* black text on grey bg */
    }

    int pair = dos_pair_for_fg_bg(fg, bg);
    attron(COLOR_PAIR(pair) | attrs);
    mvprintw(screen_y, screen_x, "[sample]");
    attroff(COLOR_PAIR(pair) | attrs);
}

/* ======================================================================== */
/* Main MCI helper dialog                                                   */
/* ======================================================================== */

const char *mci_helper_show(void)
{
    /* Count total entries */
    int num_refs = 0;
    while (mci_refs[num_refs].code) num_refs++;

    /* Build category indices */
    MciCategory cats[MCI_MAX_CATS];
    int num_cats = mci_build_categories(cats, num_refs);

    /* Dialog geometry */
    int width = 68;
    if (width > COLS - 4) width = COLS - 4;
    int max_list_h = LINES - 12;
    if (max_list_h < 8) max_list_h = 8;
    int list_h = max_list_h;
    int help_h = 3;
    int height = list_h + help_h + 5; /* border + tabs + separator + help + border */
    int x = (COLS - width) / 2;
    int y = (LINES - height) / 2;
    if (y < 1) y = 1;

    int cur_cat = 0;
    int selected = 0;
    int scroll = 0;
    const char *result = NULL;
    bool done = false;

    curs_set(0);

    while (!done) {
        /* Reset color pair pool once per frame so each sample gets a unique pair */
        menu_preview_pairs_reset();

        int display_count = cats[cur_cat].count;

        /* Clamp selection */
        if (selected >= display_count) selected = display_count - 1;
        if (selected < 0) selected = 0;
        if (scroll > selected) scroll = selected;
        if (selected >= scroll + list_h) scroll = selected - list_h + 1;
        if (scroll < 0) scroll = 0;

        /* Clear dialog area */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        for (int r = 0; r < height; r++)
            mvhline(y + r, x, ' ', width);

        /* Border */
        mvaddch(y, x, ACS_ULCORNER);
        for (int i = 1; i < width - 1; i++) addch(ACS_HLINE);
        addch(ACS_URCORNER);
        for (int i = 1; i < height - 1; i++) {
            mvaddch(y + i, x, ACS_VLINE);
            mvaddch(y + i, x + width - 1, ACS_VLINE);
        }
        mvaddch(y + height - 1, x, ACS_LLCORNER);
        for (int i = 1; i < width - 1; i++) addch(ACS_HLINE);
        addch(ACS_LRCORNER);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));

        /* Title */
        const char *title = "MCI Code Reference";
        int title_x = x + (width - (int)strlen(title)) / 2;
        attron(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
        mvprintw(y, title_x, " %s ", title);
        attroff(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);

        /* Tab bar */
        int tab_y = y + 1;
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvhline(tab_y, x + 1, ' ', width - 2);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));

        int center_x = x + width / 2;
        int cur_tab_len = (int)strlen(cats[cur_cat].name) + 2;
        int cur_tab_start = center_x - cur_tab_len / 2;

        /* Current tab highlighted */
        attron(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
        mvprintw(tab_y, cur_tab_start, " %s ", cats[cur_cat].name);
        attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);

        /* Tabs to the left */
        int left_x = cur_tab_start - 1;
        for (int i = cur_cat - 1; i >= 0 && left_x > x + 2; i--) {
            int tlen = (int)strlen(cats[i].name) + 2;
            if (left_x - tlen < x + 2) break;
            left_x -= tlen;
            attron(COLOR_PAIR(CP_MENU_BAR));
            mvprintw(tab_y, left_x, " %s ", cats[i].name);
            attroff(COLOR_PAIR(CP_MENU_BAR));
        }

        /* Tabs to the right */
        int right_x = cur_tab_start + cur_tab_len + 1;
        for (int i = cur_cat + 1; i < num_cats && right_x < x + width - 2; i++) {
            int tlen = (int)strlen(cats[i].name) + 2;
            if (right_x + tlen > x + width - 2) break;
            attron(COLOR_PAIR(CP_MENU_BAR));
            mvprintw(tab_y, right_x, " %s ", cats[i].name);
            attroff(COLOR_PAIR(CP_MENU_BAR));
            right_x += tlen;
        }

        /* Arrow indicators */
        if (cur_cat > 0) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(tab_y, x + 1, ACS_LARROW);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        if (cur_cat < num_cats - 1) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(tab_y, x + width - 2, ACS_RARROW);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }

        /* List items */
        int content_y = tab_y + 1;
        for (int r = 0; r < list_h; r++) {
            int vi = scroll + r;
            int row = content_y + r;
            if (vi >= display_count) {
                /* Empty row */
                attron(COLOR_PAIR(CP_DIALOG_BORDER));
                mvhline(row, x + 1, ' ', width - 2);
                attroff(COLOR_PAIR(CP_DIALOG_BORDER));
                continue;
            }

            int ri = cats[cur_cat].indices[vi];
            const MciRef *ref = &mci_refs[ri];
            bool is_sel = (vi == selected);

            /* Clear row */
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvhline(row, x + 1, ' ', width - 2);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));

            /* Code column — yellow (or highlight) */
            int cp = is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_VALUE;
            attron(COLOR_PAIR(cp) | A_BOLD);
            mvprintw(row, x + 2, "%-7s", ref->code);
            attroff(COLOR_PAIR(cp) | A_BOLD);

            /* Color sample or dash separator */
            int desc_x = x + 10;
            if (ref->color_idx >= 0) {
                /* Render " - [sample] " then description */
                attron(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_LABEL));
                mvprintw(row, desc_x, "- ");
                attroff(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_LABEL));

                if (!is_sel) {
                    render_color_sample(row, desc_x + 2, ref->color_idx);
                } else {
                    /* When selected, show sample then highlight rest */
                    render_color_sample(row, desc_x + 2, ref->color_idx);
                }

                /* Description after sample */
                int after_sample = desc_x + 11;
                attron(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_BG));
                mvprintw(row, after_sample, "%s", ref->desc);
                attroff(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_BG));
            } else {
                /* Non-color: " - description" */
                attron(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_LABEL));
                mvprintw(row, desc_x, "- ");
                attroff(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_LABEL));

                attron(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_BG));
                mvprintw(row, desc_x + 2, "%s", ref->desc);
                attroff(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_BG));
            }
        }

        /* Scroll indicators */
        if (scroll > 0) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(content_y, x + width - 2, ACS_UARROW);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        if (scroll + list_h < display_count) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(content_y + list_h - 1, x + width - 2, ACS_DARROW);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }

        /* Separator before help area */
        int sep_y = content_y + list_h;
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvaddch(sep_y, x, ACS_LTEE);
        for (int i = 1; i < width - 1; i++) addch(ACS_HLINE);
        mvaddch(sep_y, x + width - 1, ACS_RTEE);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));

        /* Help area — show full description of selected item */
        int help_y = sep_y + 1;
        attron(COLOR_PAIR(CP_MENU_BAR));
        for (int r = 0; r < help_h; r++)
            mvhline(help_y + r, x + 1, ' ', width - 2);

        if (selected >= 0 && selected < display_count) {
            int ri = cats[cur_cat].indices[selected];
            const MciRef *ref = &mci_refs[ri];
            mvprintw(help_y, x + 2, "Code: %s", ref->code);
            mvprintw(help_y + 1, x + 2, "%s", ref->desc);
        }
        attroff(COLOR_PAIR(CP_MENU_BAR));

        /* Footer with keybindings */
        int foot_y = y + height - 1;
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvaddch(foot_y, x, ACS_LLCORNER);
        addch(ACS_HLINE);
        addch(' ');
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));

        attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        printw("ENTER");
        attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("=Insert");
        attroff(COLOR_PAIR(CP_MENU_BAR));

        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        printw(" ");
        addch(ACS_HLINE);
        printw(" ");
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));

        attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        printw("ESC");
        attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("=Close");
        attroff(COLOR_PAIR(CP_MENU_BAR));

        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        printw(" ");
        addch(ACS_HLINE);
        printw(" ");
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));

        attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        printw("<-/->");
        attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("=Tab");
        attroff(COLOR_PAIR(CP_MENU_BAR));

        /* Fill remaining footer with border */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        {
            int cur_cx = getcurx(stdscr);
            for (int i = cur_cx; i < x + width - 1; i++) addch(ACS_HLINE);
        }
        mvaddch(foot_y, x + width - 1, ACS_LRCORNER);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));

        refresh();

        int ch = getch();
        switch (ch) {
        case KEY_LEFT:
            if (cur_cat > 0) {
                cur_cat--;
                selected = 0;
                scroll = 0;
            }
            break;

        case KEY_RIGHT:
            if (cur_cat < num_cats - 1) {
                cur_cat++;
                selected = 0;
                scroll = 0;
            }
            break;

        case KEY_UP: case 'k':
            if (selected > 0) selected--;
            break;

        case KEY_DOWN: case 'j':
            if (selected < display_count - 1) selected++;
            break;

        case KEY_HOME:
            selected = 0;
            scroll = 0;
            break;

        case KEY_END:
            selected = display_count - 1;
            break;

        case KEY_PPAGE:
            selected -= list_h;
            if (selected < 0) selected = 0;
            break;

        case KEY_NPAGE:
            selected += list_h;
            if (selected >= display_count) selected = display_count - 1;
            break;

        case '\n': case '\r': case KEY_ENTER:
            if (selected >= 0 && selected < display_count) {
                int ri = cats[cur_cat].indices[selected];
                result = mci_refs[ri].code;
            }
            done = true;
            break;

        case 27: /* Esc */
            result = NULL;
            done = true;
            break;
        }
    }

    mci_free_categories(cats, num_cats);

    touchwin(stdscr);
    wnoutrefresh(stdscr);

    return result;
}
