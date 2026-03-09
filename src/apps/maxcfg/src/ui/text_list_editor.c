/*
 * text_list_editor.c — Multi-line text list editor
 *
 * Copyright 2026 by Kevin Morgan.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "ui.h"
#include "fields.h"

#define MAX_LIST_ITEMS 1000
#define MAX_LINE_LEN 256

typedef struct {
    char **items;
    int count;
    int capacity;
    char **header_comments;
    int header_count;
} TextList;

/** @brief Allocate and initialize an empty TextList. */
static TextList *text_list_create(void)
{
    TextList *list = malloc(sizeof(TextList));
    if (!list) return NULL;
    
    list->capacity = 64;
    list->items = malloc(list->capacity * sizeof(char *));
    if (!list->items) {
        free(list);
        return NULL;
    }
    list->count = 0;
    
    list->header_comments = malloc(64 * sizeof(char *));
    if (!list->header_comments) {
        free(list->items);
        free(list);
        return NULL;
    }
    list->header_count = 0;
    return list;
}

/** @brief Free a TextList and all its contents. */
static void text_list_free(TextList *list)
{
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        free(list->items[i]);
    }
    free(list->items);
    for (int i = 0; i < list->header_count; i++) {
        free(list->header_comments[i]);
    }
    free(list->header_comments);
    free(list);
}

/**
 * @brief Append an item string to the text list, growing capacity as needed.
 *
 * @param list Text list to append to.
 * @param item String to add (duplicated internally).
 * @return true on success.
 */
static bool text_list_add(TextList *list, const char *item)
{
    if (!list || !item) return false;
    
    if (list->count >= list->capacity) {
        int new_cap = list->capacity * 2;
        char **new_items = realloc(list->items, new_cap * sizeof(char *));
        if (!new_items) return false;
        list->items = new_items;
        list->capacity = new_cap;
    }
    
    list->items[list->count] = strdup(item);
    if (!list->items[list->count]) return false;
    list->count++;
    return true;
}

/**
 * @brief Load a text list from a file, preserving header comments.
 *
 * @param list     Text list to populate.
 * @param filepath Path to the file to load.
 * @return true on success.
 */
static bool text_list_load(TextList *list, const char *filepath)
{
    FILE *f = fopen(filepath, "r");
    if (!f) return false;
    
    bool in_header = true;
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }
        
        if (len == 0) {
            continue;
        }
        
        if (line[0] == ';') {
            if (in_header) {
                list->header_comments[list->header_count] = strdup(line);
                if (!list->header_comments[list->header_count]) {
                    fclose(f);
                    return false;
                }
                list->header_count++;
            }
            continue;
        }
        
        in_header = false;
        
        if (!text_list_add(list, line)) {
            fclose(f);
            return false;
        }
    }
    
    fclose(f);
    return true;
}

/**
 * @brief Save a text list back to a file, including preserved header comments.
 *
 * @param list     Text list to save.
 * @param filepath Path to write to.
 * @return true on success.
 */
static bool text_list_save(TextList *list, const char *filepath)
{
    FILE *f = fopen(filepath, "w");
    if (!f) return false;
    
    for (int i = 0; i < list->header_count; i++) {
        fprintf(f, "%s\n", list->header_comments[i]);
    }
    
    if (list->header_count > 0 && list->count > 0) {
        fprintf(f, "\n");
    }
    
    for (int i = 0; i < list->count; i++) {
        fprintf(f, "%s\n", list->items[i]);
    }
    
    fclose(f);
    return true;
}

/**
 * @brief Open an interactive list editor for a text file (e.g. Bad Users).
 *
 * Supports add, edit, delete operations. Prompts to save on exit if modified.
 *
 * @param title     Dialog title.
 * @param filepath  Path to the text file to edit.
 * @param help_text Help string shown in add/edit dialogs.
 * @return true if the file was modified and saved.
 */
bool text_list_editor(const char *title, const char *filepath, const char *help_text)
{
    TextList *list = text_list_create();
    if (!list) {
        dialog_message("Error", "Failed to create list.");
        return false;
    }
    
    text_list_load(list, filepath);
    
    bool done = false;
    bool modified = false;
    int selected = 0;
    
    while (!done) {
        ListItem *items = malloc((list->count + 1) * sizeof(ListItem));
        if (!items) break;
        
        for (int i = 0; i < list->count; i++) {
            items[i].name = strdup(list->items[i]);
            items[i].extra = NULL;
            items[i].enabled = true;
            items[i].data = NULL;
        }
        
        int item_count = list->count;
        
        if (selected < 0) selected = 0;
        if (selected >= item_count && item_count > 0) selected = item_count - 1;
        
        ListPickResult r = listpicker_show(title, items, item_count, &selected);
        
        for (int i = 0; i < item_count; i++) {
            free(items[i].name);
        }
        free(items);
        
        if (r == LISTPICK_EXIT) {
            if (modified) {
                int choice = dialog_confirm("Save Changes?", "Save changes to file?");
                if (choice == 1) {
                    text_list_save(list, filepath);
                }
            }
            done = true;
        } else if (r == LISTPICK_INSERT || r == LISTPICK_ADD) {
            const FieldDef fields[] = {
                { "Entry", "Entry", help_text, FIELD_TEXT, MAX_LINE_LEN - 1, "", NULL, NULL, NULL, false, false, false, NULL, NULL },
            };
            char *values[1] = { strdup("") };
            if (values[0] && form_edit("Add Entry", fields, 1, values, NULL, NULL)) {
                if (values[0] && values[0][0]) {
                    text_list_add(list, values[0]);
                    modified = true;
                }
            }
            free(values[0]);
        } else if (r == LISTPICK_EDIT && selected >= 0 && selected < list->count) {
            const FieldDef fields[] = {
                { "Entry", "Entry", help_text, FIELD_TEXT, MAX_LINE_LEN - 1, "", NULL, NULL, NULL, false, false, false, NULL, NULL },
            };
            char *values[1] = { strdup(list->items[selected]) };
            if (values[0] && form_edit("Edit Entry", fields, 1, values, NULL, NULL)) {
                if (values[0] && values[0][0]) {
                    free(list->items[selected]);
                    list->items[selected] = strdup(values[0]);
                    modified = true;
                }
            }
            free(values[0]);
        } else if (r == LISTPICK_DELETE && selected >= 0 && selected < list->count) {
            free(list->items[selected]);
            for (int i = selected; i < list->count - 1; i++) {
                list->items[i] = list->items[i + 1];
            }
            list->count--;
            modified = true;
        }
    }
    
    text_list_free(list);
    return modified;
}
