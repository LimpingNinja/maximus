/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * texteditor.c - Full-screen text editor for display files
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include "maxcfg.h"
#include "ui.h"
#include "texteditor.h"

/* Editor layout constants */
#define EDITOR_MENUBAR_ROW    0
#define EDITOR_SEPARATOR_ROW  1
#define EDITOR_EDIT_TOP       2
#define EDITOR_STATUS_ROW   (LINES - 1)

/* Buffer management */
#define MAX_LINE_LENGTH     256
#define INITIAL_LINE_CAPACITY 100

typedef struct {
    char **lines;           /* Array of line pointers */
    int line_count;         /* Number of lines */
    int line_capacity;      /* Allocated capacity */
    int cursor_row;         /* Current cursor row (0-based) */
    int cursor_col;         /* Current cursor column (0-based) */
    int view_top;           /* Top line visible in viewport */
    int view_left;          /* Left column visible in viewport */
    bool modified;          /* Has buffer been modified? */
    char *filepath;         /* Path to file being edited */
    FileType filetype;      /* File type for syntax highlighting */
} EditorBuffer;

/* Menu state */
typedef struct {
    int active_menu;        /* Currently active menu (0-4, -1 = none) */
    int selected_item;      /* Selected item in dropdown */
    bool dropdown_open;     /* Is dropdown currently open? */
} EditorMenuState;

/* Forward declarations */
static FileType detect_filetype(const char *filepath);
static bool load_file(EditorBuffer *buf, const char *filepath);
static bool save_file(EditorBuffer *buf);
static void free_buffer(EditorBuffer *buf);
static void draw_editor_menubar(EditorBuffer *buf, EditorMenuState *menu_state);
static void draw_editor_separator(void);
static void draw_editor_dropdown(EditorMenuState *menu_state);
static void draw_editor_content(EditorBuffer *buf);
static void draw_editor_status(EditorBuffer *buf);
static void place_editor_cursor(EditorBuffer *buf);
static void draw_software_cursor(EditorBuffer *buf);
static void handle_menu_input(EditorBuffer *buf, EditorMenuState *menu_state, int ch, bool *quit);
static void handle_edit_input(EditorBuffer *buf, int ch);
static int read_key_with_alt(bool *out_is_alt);

/* Menu definitions */
static const char *menu_labels[] = {
    "File", "Search", "Compile", "Insert", "Help"
};
#define NUM_MENUS 5

static const char *file_menu_items[] = {
    "Save", "Preview", "Exit"
};
#define FILE_MENU_COUNT 3

static const char *search_menu_items[] = {
    "Find", "Replace"
};
#define SEARCH_MENU_COUNT 2

static const char *compile_menu_items[] = {
    "Mecca", "Mex"
};
#define COMPILE_MENU_COUNT 2

static const char *insert_menu_items[] = {
    "MECCA Code", "ASCII Chart", "Line Drawing"
};
#define INSERT_MENU_COUNT 3

static int menu_x_positions[NUM_MENUS] = { 0 };

static FileType detect_filetype(const char *filepath)
{
    if (!filepath) return FILETYPE_UNKNOWN;
    
    const char *ext = strrchr(filepath, '.');
    if (!ext) return FILETYPE_TEXT;
    
    if (strcasecmp(ext, ".mec") == 0) return FILETYPE_MECCA;
    if (strcasecmp(ext, ".mex") == 0) return FILETYPE_MEX;
    
    return FILETYPE_TEXT;
}

static bool load_file(EditorBuffer *buf, const char *filepath)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        return false;
    }
    
    buf->lines = malloc(sizeof(char*) * INITIAL_LINE_CAPACITY);
    if (!buf->lines) {
        fclose(fp);
        return false;
    }
    
    buf->line_count = 0;
    buf->line_capacity = INITIAL_LINE_CAPACITY;
    
    char line_buf[MAX_LINE_LENGTH];
    while (fgets(line_buf, sizeof(line_buf), fp)) {
        /* Strip newline */
        size_t len = strlen(line_buf);
        if (len > 0 && line_buf[len - 1] == '\n') {
            line_buf[len - 1] = '\0';
            len--;
        }
        if (len > 0 && line_buf[len - 1] == '\r') {
            line_buf[len - 1] = '\0';
        }
        
        /* Expand capacity if needed */
        if (buf->line_count >= buf->line_capacity) {
            buf->line_capacity *= 2;
            char **new_lines = realloc(buf->lines, sizeof(char*) * buf->line_capacity);
            if (!new_lines) {
                fclose(fp);
                return false;
            }
            buf->lines = new_lines;
        }
        
        buf->lines[buf->line_count] = strdup(line_buf);
        if (!buf->lines[buf->line_count]) {
            fclose(fp);
            return false;
        }
        buf->line_count++;
    }
    
    fclose(fp);
    
    /* Ensure at least one line */
    if (buf->line_count == 0) {
        buf->lines[0] = strdup("");
        buf->line_count = 1;
    }
    
    return true;
}

static bool save_file(EditorBuffer *buf)
{
    if (!buf->filepath) return false;
    
    FILE *fp = fopen(buf->filepath, "wb");
    if (!fp) return false;
    
    for (int i = 0; i < buf->line_count; i++) {
        fprintf(fp, "%s\n", buf->lines[i]);
    }
    
    fclose(fp);
    buf->modified = false;
    return true;
}

static void free_buffer(EditorBuffer *buf)
{
    if (buf->lines) {
        for (int i = 0; i < buf->line_count; i++) {
            free(buf->lines[i]);
        }
        free(buf->lines);
    }
    free(buf->filepath);
}

static void draw_editor_menubar(EditorBuffer *buf, EditorMenuState *menu_state)
{
    attron(COLOR_PAIR(CP_TITLE_BAR));
    move(EDITOR_MENUBAR_ROW, 0);
    for (int i = 0; i < COLS; i++) {
        addch(' ');
    }
    
    int x = 2;
    for (int i = 0; i < NUM_MENUS; i++) {
        menu_x_positions[i] = x;
        move(EDITOR_MENUBAR_ROW, x);
        
        if (menu_state->active_menu == i) {
            attron(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
        } else {
            attron(COLOR_PAIR(CP_TITLE_BAR));
        }
        
        /* Draw first char as hotkey */
        attron(A_BOLD | A_UNDERLINE);
        addch(menu_labels[i][0]);
        attroff(A_BOLD | A_UNDERLINE);
        
        if (menu_state->active_menu == i) {
            attron(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
        } else {
            attron(COLOR_PAIR(CP_TITLE_BAR));
        }
        
        /* Draw rest of label */
        printw("%s", menu_labels[i] + 1);
        
        x += strlen(menu_labels[i]) + 4;
    }
    
    attroff(COLOR_PAIR(CP_TITLE_BAR));
    attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
    
    /* Draw filename on right side */
    {
        const char *base = buf && buf->filepath ? strrchr(buf->filepath, '/') : NULL;
        base = base ? (base + 1) : (buf && buf->filepath ? buf->filepath : "");
        char label[128];
        (void)snprintf(label, sizeof(label), "%s%s", base, (buf && buf->modified) ? " *" : "");

        int pos = COLS - (int)strlen(label) - 2;
        if (pos < x + 1) {
            pos = x + 1;
        }

        attron(COLOR_PAIR(CP_TITLE_BAR));
        mvprintw(EDITOR_MENUBAR_ROW, pos, "%s", label);
        attroff(COLOR_PAIR(CP_TITLE_BAR));
    }
    
    wnoutrefresh(stdscr);
}

static void draw_editor_separator(void)
{
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    move(EDITOR_SEPARATOR_ROW, 0);
    for (int i = 0; i < COLS; i++) {
        addch(ACS_HLINE);
    }
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    wnoutrefresh(stdscr);
}

static void draw_editor_dropdown(EditorMenuState *menu_state)
{
    if (!menu_state->dropdown_open || menu_state->active_menu < 0 || menu_state->active_menu >= NUM_MENUS) {
        return;
    }

    const char **items = NULL;
    int count = 0;
    switch (menu_state->active_menu) {
        case 0: items = file_menu_items; count = FILE_MENU_COUNT; break;
        case 1: items = search_menu_items; count = SEARCH_MENU_COUNT; break;
        case 2: items = compile_menu_items; count = COMPILE_MENU_COUNT; break;
        case 3: items = insert_menu_items; count = INSERT_MENU_COUNT; break;
        default: items = NULL; count = 0; break;
    }
    if (!items || count <= 0) return;

    int x = menu_x_positions[menu_state->active_menu];
    int y = EDITOR_EDIT_TOP;

    int w = 0;
    for (int i = 0; i < count; i++) {
        int len = (int)strlen(items[i]);
        if (len > w) w = len;
    }
    w += 4;
    int h = count + 2;
    if (x + w >= COLS) {
        x = COLS - w - 1;
        if (x < 0) x = 0;
    }
    if (y + h >= LINES) {
        y = LINES - h - 1;
        if (y < EDITOR_EDIT_TOP) y = EDITOR_EDIT_TOP;
    }

    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    mvaddch(y, x, ACS_ULCORNER);
    for (int i = 1; i < w - 1; i++) addch(ACS_HLINE);
    addch(ACS_URCORNER);
    for (int r = 1; r < h - 1; r++) {
        mvaddch(y + r, x, ACS_VLINE);
        mvaddch(y + r, x + w - 1, ACS_VLINE);
    }
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    for (int i = 1; i < w - 1; i++) addch(ACS_HLINE);
    addch(ACS_LRCORNER);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));

    for (int i = 0; i < count; i++) {
        int row = y + 1 + i;
        move(row, x + 1);
        if (i == menu_state->selected_item) {
            attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        } else {
            attron(COLOR_PAIR(CP_DROPDOWN));
        }
        for (int c = 0; c < w - 2; c++) addch(' ');
        mvprintw(row, x + 2, "%s", items[i]);
        if (i == menu_state->selected_item) {
            attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        } else {
            attroff(COLOR_PAIR(CP_DROPDOWN));
        }
    }

    wnoutrefresh(stdscr);
}

static void draw_editor_content(EditorBuffer *buf)
{
    int edit_height = EDITOR_STATUS_ROW - EDITOR_EDIT_TOP;
    int edit_width = COLS;
    
    attron(COLOR_PAIR(CP_FORM_BG));
    
    for (int screen_row = 0; screen_row < edit_height; screen_row++) {
        int file_row = buf->view_top + screen_row;
        move(EDITOR_EDIT_TOP + screen_row, 0);
        
        /* Clear line */
        for (int i = 0; i < edit_width; i++) {
            addch(' ');
        }
        
        /* Draw line content if within buffer */
        if (file_row < buf->line_count) {
            move(EDITOR_EDIT_TOP + screen_row, 0);
            const char *line = buf->lines[file_row];
            int line_len = strlen(line);
            
            /* Draw visible portion of line */
            for (int col = buf->view_left; col < line_len && (col - buf->view_left) < edit_width; col++) {
                unsigned char ch = (unsigned char)line[col];
                
                /* Basic rendering - CP437 support would go here */
                if (ch >= 32 && ch < 127) {
                    addch(ch);
                } else if (ch >= 128) {
                    /* For now, show high-ASCII as-is; proper CP437 needs more work */
                    addch(ch);
                } else {
                    addch('.');  /* Control chars */
                }
            }
        }
    }
    
    attroff(COLOR_PAIR(CP_FORM_BG));
    
    /* Position cursor */
    int cursor_screen_row = buf->cursor_row - buf->view_top;
    int cursor_screen_col = buf->cursor_col - buf->view_left;
    
    if (cursor_screen_row >= 0 && cursor_screen_row < edit_height &&
        cursor_screen_col >= 0 && cursor_screen_col < edit_width) {
        move(EDITOR_EDIT_TOP + cursor_screen_row, cursor_screen_col);
    }
    
    wnoutrefresh(stdscr);
}

static void draw_editor_status(EditorBuffer *buf)
{
    attron(COLOR_PAIR(CP_STATUS_BAR));
    move(EDITOR_STATUS_ROW, 0);
    for (int i = 0; i < COLS; i++) {
        addch(' ');
    }
    
    /* Show cursor position and modified flag */
    char status[128];
    const char *basename = buf && buf->filepath ? strrchr(buf->filepath, '/') : NULL;
    if (basename) basename++; else basename = buf && buf->filepath ? buf->filepath : "untitled";
    
    snprintf(status, sizeof(status), " %s%s    Line %d, Col %d ",
             basename,
             buf->modified ? " *" : "",
             buf->cursor_row + 1,
             buf->cursor_col + 1);
    
    mvprintw(EDITOR_STATUS_ROW, 0, "%s", status);
    attroff(COLOR_PAIR(CP_STATUS_BAR));
    
    wnoutrefresh(stdscr);
}

static void place_editor_cursor(EditorBuffer *buf)
{
    int edit_height = EDITOR_STATUS_ROW - EDITOR_EDIT_TOP;
    int edit_width = COLS;

    if (!buf || !buf->lines || buf->line_count <= 0) {
        return;
    }

    int cursor_screen_row = buf->cursor_row - buf->view_top;
    int cursor_screen_col = buf->cursor_col - buf->view_left;

    if (cursor_screen_row >= 0 && cursor_screen_row < edit_height &&
        cursor_screen_col >= 0 && cursor_screen_col < edit_width) {
        move(EDITOR_EDIT_TOP + cursor_screen_row, cursor_screen_col);
    }
}

static void draw_software_cursor(EditorBuffer *buf)
{
    int edit_height = EDITOR_STATUS_ROW - EDITOR_EDIT_TOP;
    int edit_width = COLS;

    if (!buf || !buf->lines || buf->line_count <= 0) {
        return;
    }

    int cursor_screen_row = buf->cursor_row - buf->view_top;
    int cursor_screen_col = buf->cursor_col - buf->view_left;

    if (cursor_screen_row < 0 || cursor_screen_row >= edit_height ||
        cursor_screen_col < 0 || cursor_screen_col >= edit_width) {
        return;
    }

    int y = EDITOR_EDIT_TOP + cursor_screen_row;
    int x = cursor_screen_col;

    char c = ' ';
    if (buf->cursor_row >= 0 && buf->cursor_row < buf->line_count) {
        const char *line = buf->lines[buf->cursor_row] ? buf->lines[buf->cursor_row] : "";
        int len = (int)strlen(line);
        if (buf->cursor_col >= 0 && buf->cursor_col < len) {
            unsigned char ch = (unsigned char)line[buf->cursor_col];
            c = (ch >= 32) ? (char)ch : ' ';
        }
    }

    /* Draw a grey/white block with dark text so the character is readable under the cursor.
     * We reuse CP_TITLE_BAR (black on white) for this.
     */
    attron(COLOR_PAIR(CP_TITLE_BAR));
    mvaddch(y, x, c);
    attroff(COLOR_PAIR(CP_TITLE_BAR));

    wnoutrefresh(stdscr);
}

static void handle_menu_input(EditorBuffer *buf, EditorMenuState *menu_state, int ch, bool *quit)
{
    if (!menu_state->dropdown_open) {
        /* Open dropdown on Enter */
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            if (menu_state->active_menu >= 0) {
                menu_state->dropdown_open = true;
                menu_state->selected_item = 0;
            }
        }
        return;
    }

    switch (ch) {
        case KEY_UP:
            if (menu_state->selected_item > 0) menu_state->selected_item--;
            break;
        case KEY_DOWN:
            menu_state->selected_item++;
            {
                int max = 0;
                if (menu_state->active_menu == 0) max = FILE_MENU_COUNT;
                else if (menu_state->active_menu == 1) max = SEARCH_MENU_COUNT;
                else if (menu_state->active_menu == 2) max = COMPILE_MENU_COUNT;
                else if (menu_state->active_menu == 3) max = INSERT_MENU_COUNT;
                if (max > 0 && menu_state->selected_item >= max) menu_state->selected_item = max - 1;
            }
            break;
        case KEY_LEFT:
            if (menu_state->active_menu > 0) {
                menu_state->active_menu--;
                menu_state->selected_item = 0;
            }
            break;
        case KEY_RIGHT:
            if (menu_state->active_menu < NUM_MENUS - 1) {
                menu_state->active_menu++;
                menu_state->selected_item = 0;
            }
            break;
        case 27:
            menu_state->dropdown_open = false;
            menu_state->active_menu = -1;
            break;
        case '\n':
        case '\r':
        case KEY_ENTER:
            /* Activate selected menu item */
            if (menu_state->active_menu == 0) {
                if (menu_state->selected_item == 0) {
                    if (save_file(buf)) {
                        draw_status_bar("Saved");
                    } else {
                        dialog_message("Save Failed", "Unable to save file.");
                    }
                } else if (menu_state->selected_item == 1) {
                    dialog_message("Not Implemented", "Preview is not implemented yet.");
                } else if (menu_state->selected_item == 2) {
                    if (buf && buf->modified) {
                        if (!dialog_confirm("Discard Changes?", "Exit without saving changes?")) {
                            break;
                        }
                    }
                    *quit = true;
                }
            } else if (menu_state->active_menu == 1) {
                dialog_message("Not Implemented", "Search is not implemented yet.");
            } else if (menu_state->active_menu == 2) {
                dialog_message("Not Implemented", "Compile is not implemented yet.");
            } else if (menu_state->active_menu == 3) {
                dialog_message("Not Implemented", "Insert is not implemented yet.");
            }
            menu_state->dropdown_open = false;
            menu_state->active_menu = -1;
            break;
    }
}

static void handle_edit_input(EditorBuffer *buf, int ch)
{
    int edit_height = EDITOR_STATUS_ROW - EDITOR_EDIT_TOP;
    int edit_width = COLS;

    if (!buf || !buf->lines || buf->cursor_row < 0 || buf->cursor_row >= buf->line_count) {
        return;
    }

    /* Basic editing */
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
        char *line = buf->lines[buf->cursor_row];
        int len = (int)strlen(line);
        if (buf->cursor_col > len) buf->cursor_col = len;

        char *left = strndup(line, (size_t)buf->cursor_col);
        char *right = strdup(line + buf->cursor_col);
        if (!left || !right) {
            free(left);
            free(right);
            return;
        }

        /* Ensure capacity */
        if (buf->line_count >= buf->line_capacity) {
            buf->line_capacity *= 2;
            char **nl = realloc(buf->lines, sizeof(char*) * (size_t)buf->line_capacity);
            if (!nl) {
                free(left);
                free(right);
                return;
            }
            buf->lines = nl;
        }

        /* Shift lines down */
        for (int i = buf->line_count; i > buf->cursor_row + 1; i--) {
            buf->lines[i] = buf->lines[i - 1];
        }
        free(buf->lines[buf->cursor_row]);
        buf->lines[buf->cursor_row] = left;
        buf->lines[buf->cursor_row + 1] = right;
        buf->line_count++;

        buf->cursor_row++;
        buf->cursor_col = 0;
        buf->modified = true;
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (buf->cursor_col > 0) {
            char *line = buf->lines[buf->cursor_row];
            int len = (int)strlen(line);
            if (buf->cursor_col <= len) {
                memmove(line + buf->cursor_col - 1, line + buf->cursor_col, (size_t)(len - buf->cursor_col + 1));
                buf->cursor_col--;
                buf->modified = true;
            }
        } else if (buf->cursor_row > 0) {
            /* Join with previous line */
            int prev = buf->cursor_row - 1;
            int prev_len = (int)strlen(buf->lines[prev]);
            int cur_len = (int)strlen(buf->lines[buf->cursor_row]);

            char *joined = malloc((size_t)prev_len + (size_t)cur_len + 1u);
            if (!joined) return;
            memcpy(joined, buf->lines[prev], (size_t)prev_len);
            memcpy(joined + prev_len, buf->lines[buf->cursor_row], (size_t)cur_len + 1u);

            free(buf->lines[prev]);
            free(buf->lines[buf->cursor_row]);
            buf->lines[prev] = joined;
            for (int i = buf->cursor_row; i < buf->line_count - 1; i++) {
                buf->lines[i] = buf->lines[i + 1];
            }
            buf->line_count--;
            buf->cursor_row = prev;
            buf->cursor_col = prev_len;
            buf->modified = true;
        }
    } else if (ch == KEY_DC) {
        char *line = buf->lines[buf->cursor_row];
        int len = (int)strlen(line);
        if (buf->cursor_col < len) {
            memmove(line + buf->cursor_col, line + buf->cursor_col + 1, (size_t)(len - buf->cursor_col));
            buf->modified = true;
        } else if (buf->cursor_row < buf->line_count - 1) {
            /* Join with next line */
            int next = buf->cursor_row + 1;
            int next_len = (int)strlen(buf->lines[next]);
            char *joined = malloc((size_t)len + (size_t)next_len + 1u);
            if (!joined) return;
            memcpy(joined, line, (size_t)len);
            memcpy(joined + len, buf->lines[next], (size_t)next_len + 1u);
            free(buf->lines[buf->cursor_row]);
            free(buf->lines[next]);
            buf->lines[buf->cursor_row] = joined;
            for (int i = next; i < buf->line_count - 1; i++) {
                buf->lines[i] = buf->lines[i + 1];
            }
            buf->line_count--;
            buf->modified = true;
        }
    } else if (ch >= 32 && ch <= 126) {
        char *line = buf->lines[buf->cursor_row];
        int len = (int)strlen(line);
        if (len < MAX_LINE_LENGTH - 2) {
            if (buf->cursor_col > len) buf->cursor_col = len;
            char *new_line = malloc((size_t)len + 2u);
            if (!new_line) return;
            memcpy(new_line, line, (size_t)buf->cursor_col);
            new_line[buf->cursor_col] = (char)ch;
            memcpy(new_line + buf->cursor_col + 1, line + buf->cursor_col, (size_t)(len - buf->cursor_col + 1));
            free(buf->lines[buf->cursor_row]);
            buf->lines[buf->cursor_row] = new_line;
            buf->cursor_col++;
            buf->modified = true;
        }
    }
    
    switch (ch) {
        case KEY_UP:
            if (buf->cursor_row > 0) {
                buf->cursor_row--;
                /* Adjust view if needed */
                if (buf->cursor_row < buf->view_top) {
                    buf->view_top = buf->cursor_row;
                }
                /* Clamp column to line length */
                int line_len = strlen(buf->lines[buf->cursor_row]);
                if (buf->cursor_col > line_len) {
                    buf->cursor_col = line_len;
                }
            }
            break;
            
        case KEY_DOWN:
            if (buf->cursor_row < buf->line_count - 1) {
                buf->cursor_row++;
                /* Adjust view if needed */
                if (buf->cursor_row >= buf->view_top + edit_height) {
                    buf->view_top = buf->cursor_row - edit_height + 1;
                }
                /* Clamp column to line length */
                int line_len = strlen(buf->lines[buf->cursor_row]);
                if (buf->cursor_col > line_len) {
                    buf->cursor_col = line_len;
                }
            }
            break;
            
        case KEY_LEFT:
            if (buf->cursor_col > 0) {
                buf->cursor_col--;
                if (buf->cursor_col < buf->view_left) {
                    buf->view_left = buf->cursor_col;
                }
            }
            break;
            
        case KEY_RIGHT:
            {
                int line_len = strlen(buf->lines[buf->cursor_row]);
                if (buf->cursor_col < line_len) {
                    buf->cursor_col++;
                    if (buf->cursor_col >= buf->view_left + edit_width) {
                        buf->view_left = buf->cursor_col - edit_width + 1;
                    }
                }
            }
            break;
            
        case KEY_HOME:
            buf->cursor_col = 0;
            buf->view_left = 0;
            break;
            
        case KEY_END:
            buf->cursor_col = strlen(buf->lines[buf->cursor_row]);
            if (buf->cursor_col >= edit_width) {
                buf->view_left = buf->cursor_col - edit_width + 1;
            }
            break;
            
        case KEY_PPAGE:  /* Page Up */
            buf->cursor_row -= edit_height;
            if (buf->cursor_row < 0) buf->cursor_row = 0;
            buf->view_top = buf->cursor_row;
            break;
            
        case KEY_NPAGE:  /* Page Down */
            buf->cursor_row += edit_height;
            if (buf->cursor_row >= buf->line_count) {
                buf->cursor_row = buf->line_count - 1;
            }
            buf->view_top = buf->cursor_row;
            break;
    }
}

static int read_key_with_alt(bool *out_is_alt)
{
    if (out_is_alt) {
        *out_is_alt = false;
    }

    int ch = getch();
    if (ch != 27) {
        return ch;
    }

    /* Treat Alt as ESC prefix + immediate next key.
     * If nothing follows immediately, this is a real ESC keypress.
     */
    timeout(0);
    int next = getch();
    timeout(-1);

    if (next == ERR) {
        return 27;
    }

    if (out_is_alt) {
        *out_is_alt = true;
    }
    return next;
}

EditorResult text_editor_edit(const char *filepath)
{
    if (!filepath) return EDITOR_ERROR;
    
    /* Initialize buffer */
    EditorBuffer buf = {0};
    buf.filepath = strdup(filepath);
    buf.filetype = detect_filetype(filepath);
    
    if (!load_file(&buf, filepath)) {
        free_buffer(&buf);
        return EDITOR_ERROR;
    }
    
    /* Initialize menu state */
    EditorMenuState menu_state = {0};
    menu_state.active_menu = -1;  /* No menu active initially */
    
    /* Hide the terminal cursor; we draw a software cursor so it doesn't bleed over menus
     * and we can control the foreground color under the cursor.
     */
    (void)curs_set(0);
    
    bool quit = false;
    bool esc_armed = false;
    
    /* Main editor loop */
    while (!quit) {
        draw_editor_menubar(&buf, &menu_state);
        draw_editor_separator();
        draw_editor_content(&buf);
        draw_editor_status(&buf);
        draw_editor_dropdown(&menu_state);
        /* status/menu drawing moves the curses cursor; keep the logical caret consistent */
        place_editor_cursor(&buf);
        /* Draw the software cursor last, and never over an open menu */
        if (!menu_state.dropdown_open && menu_state.active_menu < 0) {
            draw_software_cursor(&buf);
        }
        doupdate();
        
        bool is_alt = false;
        int ch = read_key_with_alt(&is_alt);

        if (ch == 27) {
            /* ESC closes menu/dropdown; does not exit.
             * If no menu is open, double-tap ESC opens the menu.
             */
            if (menu_state.dropdown_open) {
                menu_state.dropdown_open = false;
                menu_state.active_menu = -1;
                esc_armed = false;
            } else if (menu_state.active_menu >= 0) {
                menu_state.active_menu = -1;
                esc_armed = false;
            } else {
                if (esc_armed) {
                    menu_state.active_menu = 0;
                    menu_state.dropdown_open = true;
                    menu_state.selected_item = 0;
                    esc_armed = false;
                } else {
                    esc_armed = true;
                }
            }
            continue;
        }

        /* Any non-ESC key disarms the double-ESC latch */
        esc_armed = false;

        if (is_alt) {
            /* Alt+letter opens corresponding menu */
            if (ch == 'f' || ch == 'F') menu_state.active_menu = 0;
            else if (ch == 's' || ch == 'S') menu_state.active_menu = 1;
            else if (ch == 'c' || ch == 'C') menu_state.active_menu = 2;
            else if (ch == 'i' || ch == 'I') menu_state.active_menu = 3;
            else if (ch == 'h' || ch == 'H') menu_state.active_menu = 4;
            else menu_state.active_menu = -1;

            if (menu_state.active_menu >= 0) {
                menu_state.dropdown_open = true;
                menu_state.selected_item = 0;
            }
            continue;
        }

        if (menu_state.dropdown_open) {
            handle_menu_input(&buf, &menu_state, ch, &quit);
        } else {
            handle_edit_input(&buf, ch);
        }
    }
    
    /* Restore cursor state */
    curs_set(0);
    
    EditorResult result = EDITOR_CANCELLED;
    
    free_buffer(&buf);
    
    /* Restore main screen */
    screen_refresh();
    
    return result;
}
