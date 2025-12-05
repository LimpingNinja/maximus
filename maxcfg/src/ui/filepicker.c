/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * filepicker.c - File selection dialog for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include "maxcfg.h"
#include "ui.h"

#define MAX_FILES 100
#define MAX_VISIBLE 10

typedef struct {
    char **files;       /* Array of filenames (without extension) */
    int count;          /* Number of files */
    int selected;       /* Currently selected index */
    int scroll_offset;  /* Scroll position for long lists */
} FileList;

/* Get extension from filter (e.g., "*.bbs" -> ".bbs") */
static const char *get_extension(const char *filter)
{
    if (!filter) return NULL;
    const char *star = strchr(filter, '*');
    if (star && star[1] == '.') {
        return star + 1;  /* Return ".bbs" */
    }
    return NULL;
}

/* Check if filename matches filter */
static int matches_filter(const char *filename, const char *ext)
{
    if (!ext) return 1;  /* No filter, match all */
    
    size_t name_len = strlen(filename);
    size_t ext_len = strlen(ext);
    
    if (name_len <= ext_len) return 0;
    
    return strcasecmp(filename + name_len - ext_len, ext) == 0;
}

/* Strip extension from filename */
static char *strip_extension(const char *filename, const char *ext)
{
    if (!ext) return strdup(filename);
    
    size_t name_len = strlen(filename);
    size_t ext_len = strlen(ext);
    
    if (name_len > ext_len && strcasecmp(filename + name_len - ext_len, ext) == 0) {
        char *result = malloc(name_len - ext_len + 1);
        if (result) {
            strncpy(result, filename, name_len - ext_len);
            result[name_len - ext_len] = '\0';
        }
        return result;
    }
    
    return strdup(filename);
}

/* Load files from directory matching filter */
static void load_files(FileList *list, const char *base_path, const char *filter)
{
    list->files = malloc(MAX_FILES * sizeof(char *));
    list->count = 0;
    list->selected = 0;
    list->scroll_offset = 0;
    
    if (!list->files) return;
    
    const char *ext = get_extension(filter);
    
    DIR *dir = opendir(base_path);
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && list->count < MAX_FILES) {
        /* Skip . and .. */
        if (entry->d_name[0] == '.') continue;
        
        /* Check if it matches filter */
        if (!matches_filter(entry->d_name, ext)) continue;
        
        /* Check if it's a regular file */
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        
        /* Add to list (strip extension) */
        list->files[list->count] = strip_extension(entry->d_name, ext);
        if (list->files[list->count]) {
            list->count++;
        }
    }
    
    closedir(dir);
    
    /* Sort alphabetically */
    for (int i = 0; i < list->count - 1; i++) {
        for (int j = i + 1; j < list->count; j++) {
            if (strcasecmp(list->files[i], list->files[j]) > 0) {
                char *tmp = list->files[i];
                list->files[i] = list->files[j];
                list->files[j] = tmp;
            }
        }
    }
}

/* Free file list */
static void free_files(FileList *list)
{
    if (list->files) {
        for (int i = 0; i < list->count; i++) {
            free(list->files[i]);
        }
        free(list->files);
    }
    list->files = NULL;
    list->count = 0;
}

/* Find index of current value in list */
static int find_current(FileList *list, const char *current, const char *base_path)
{
    if (!current || !list->files) return 0;
    
    /* Extract just the filename from current (e.g., "etc/misc/logo" -> "logo") */
    const char *filename = current;
    if (base_path) {
        size_t base_len = strlen(base_path);
        if (strncmp(current, base_path, base_len) == 0) {
            filename = current + base_len;
            if (*filename == '/') filename++;
        }
    }
    
    for (int i = 0; i < list->count; i++) {
        if (strcasecmp(list->files[i], filename) == 0) {
            return i;
        }
    }
    
    return 0;
}

/* Draw the file picker dialog */
static void draw_picker(FileList *list, const char *title, int win_y, int win_x, int win_w, int win_h)
{
    int visible = win_h - 4;  /* Account for borders and title */
    if (visible > list->count) visible = list->count;
    
    /* Draw window border */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Top border with title */
    mvaddch(win_y, win_x, ACS_ULCORNER);
    mvaddch(win_y, win_x + 1, ACS_HLINE);
    addch('[');
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_DIALOG_TEXT) | A_BOLD);
    printw("%s", title);
    attroff(COLOR_PAIR(CP_DIALOG_TEXT) | A_BOLD);
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(']');
    for (int i = strlen(title) + 4; i < win_w - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_URCORNER);
    
    /* Sides and content area */
    for (int i = 1; i < win_h - 1; i++) {
        mvaddch(win_y + i, win_x, ACS_VLINE);
        
        /* Clear content area */
        attron(COLOR_PAIR(CP_FORM_BG));
        for (int j = 1; j < win_w - 1; j++) {
            addch(' ');
        }
        attroff(COLOR_PAIR(CP_FORM_BG));
        
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvaddch(win_y + i, win_x + win_w - 1, ACS_VLINE);
    }
    
    /* Bottom border */
    mvaddch(win_y + win_h - 1, win_x, ACS_LLCORNER);
    for (int i = 1; i < win_w - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_LRCORNER);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Draw file list */
    int start = list->scroll_offset;
    int end = start + visible;
    if (end > list->count) end = list->count;
    
    for (int i = start; i < end; i++) {
        int y = win_y + 1 + (i - start);
        int x = win_x + 2;
        
        if (i == list->selected) {
            attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        } else {
            attron(COLOR_PAIR(CP_FORM_VALUE));
        }
        
        mvprintw(y, x, "%-*.*s", win_w - 4, win_w - 4, list->files[i]);
        
        if (i == list->selected) {
            attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        } else {
            attroff(COLOR_PAIR(CP_FORM_VALUE));
        }
    }
    
    /* Draw scroll indicators if needed */
    if (list->scroll_offset > 0) {
        attron(COLOR_PAIR(CP_DIALOG_TEXT) | A_BOLD);
        mvaddch(win_y + 1, win_x + win_w - 2, ACS_UARROW);
        attroff(COLOR_PAIR(CP_DIALOG_TEXT) | A_BOLD);
    }
    if (end < list->count) {
        attron(COLOR_PAIR(CP_DIALOG_TEXT) | A_BOLD);
        mvaddch(win_y + win_h - 2, win_x + win_w - 2, ACS_DARROW);
        attroff(COLOR_PAIR(CP_DIALOG_TEXT) | A_BOLD);
    }
    
    /* Status line */
    attron(COLOR_PAIR(CP_MENU_BAR));
    mvprintw(win_y + win_h - 2, win_x + 2, "Enter=Select  ESC=Cancel");
    attroff(COLOR_PAIR(CP_MENU_BAR));
}

char *filepicker_select(const char *base_path, const char *filter, const char *current)
{
    FileList list;
    load_files(&list, base_path, filter);
    
    if (list.count == 0) {
        dialog_message("No Files", "No matching files found in the specified directory.");
        free_files(&list);
        return NULL;
    }
    
    /* Find and select current value */
    list.selected = find_current(&list, current, base_path);
    
    /* Adjust scroll to show selected */
    int visible = MAX_VISIBLE;
    if (list.selected >= visible) {
        list.scroll_offset = list.selected - visible + 1;
    }
    
    /* Calculate window size */
    int max_name_len = 0;
    for (int i = 0; i < list.count; i++) {
        int len = strlen(list.files[i]);
        if (len > max_name_len) max_name_len = len;
    }
    
    int win_w = max_name_len + 6;
    if (win_w < 30) win_w = 30;
    if (win_w > COLS - 4) win_w = COLS - 4;
    
    int win_h = (list.count < visible ? list.count : visible) + 4;
    if (win_h > LINES - 6) win_h = LINES - 6;
    
    int win_y = (LINES - win_h) / 2;
    int win_x = (COLS - win_w) / 2;
    
    char *result = NULL;
    bool done = false;
    
    while (!done) {
        draw_picker(&list, "Select File", win_y, win_x, win_w, win_h);
        refresh();
        
        int ch = getch();
        
        switch (ch) {
            case KEY_UP:
                if (list.selected > 0) {
                    list.selected--;
                    if (list.selected < list.scroll_offset) {
                        list.scroll_offset = list.selected;
                    }
                }
                break;
                
            case KEY_DOWN:
                if (list.selected < list.count - 1) {
                    list.selected++;
                    if (list.selected >= list.scroll_offset + visible) {
                        list.scroll_offset = list.selected - visible + 1;
                    }
                }
                break;
                
            case KEY_PPAGE:
                list.selected -= visible;
                if (list.selected < 0) list.selected = 0;
                list.scroll_offset = list.selected;
                break;
                
            case KEY_NPAGE:
                list.selected += visible;
                if (list.selected >= list.count) list.selected = list.count - 1;
                if (list.selected >= list.scroll_offset + visible) {
                    list.scroll_offset = list.selected - visible + 1;
                }
                break;
                
            case KEY_HOME:
                list.selected = 0;
                list.scroll_offset = 0;
                break;
                
            case KEY_END:
                list.selected = list.count - 1;
                if (list.selected >= visible) {
                    list.scroll_offset = list.selected - visible + 1;
                }
                break;
                
            case '\n':
            case '\r':
            case KEY_ENTER:
                /* Build result: base_path/filename */
                {
                    size_t len = strlen(base_path) + 1 + strlen(list.files[list.selected]) + 1;
                    result = malloc(len);
                    if (result) {
                        snprintf(result, len, "%s/%s", base_path, list.files[list.selected]);
                    }
                }
                done = true;
                break;
                
            case 27:  /* ESC */
                done = true;
                break;
        }
    }
    
    free_files(&list);
    return result;
}
