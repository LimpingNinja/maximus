/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ui.h - UI component declarations for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef UI_H
#define UI_H

#include "maxcfg.h"

/*
 * Menu item structure
 */
typedef struct MenuItem {
    const char *label;          /* Display label (first char is hotkey) */
    const char *hotkey;         /* Hotkey character (or NULL) */
    struct MenuItem *submenu;   /* Submenu items (or NULL) */
    int submenu_count;          /* Number of submenu items */
    void (*action)(void);       /* Action function (or NULL if has submenu) */
    bool enabled;               /* Is this item selectable? */
} MenuItem;

/*
 * Top-level menu definition
 */
typedef struct {
    const char *label;
    MenuItem *items;
    int item_count;
} TopMenu;

/* Dialog result codes */
typedef enum {
    DIALOG_SAVE_EXIT = 0,
    DIALOG_ABORT = 1,
    DIALOG_RETURN = 2,
    DIALOG_CANCEL = -1
} DialogResult;

/* ============================================================================
 * screen.c - Screen management
 * ============================================================================ */

/* Initialize ncurses and colors */
void screen_init(void);

/* Cleanup ncurses */
void screen_cleanup(void);

/* Initialize color pairs */
void init_colors(void);

/* Draw the title bar */
void draw_title_bar(void);

/* Draw the status bar with given text */
void draw_status_bar(const char *text);

/* Fill the work area with background */
void draw_work_area(void);

/* Refresh all screen components */
void screen_refresh(void);

/* ============================================================================
 * menubar.c - Top menu bar
 * ============================================================================ */

/* Initialize menu definitions */
void menubar_init(void);

/* Draw the menu bar */
void draw_menubar(void);

/* Handle menu bar input, returns true if handled */
bool menubar_handle_key(int ch);

/* Get currently selected top menu index */
int menubar_get_current(void);

/* Set the current menu (for external control) */
void menubar_set_current(int index);

/* ============================================================================
 * dropdown.c - Dropdown and submenu handling
 * ============================================================================ */

/* Open a dropdown menu at specified position */
void dropdown_open(int menu_index);

/* Close any open dropdown */
void dropdown_close(void);

/* Draw the currently open dropdown (if any) */
void draw_dropdown(void);

/* Handle dropdown input, returns true if handled */
bool dropdown_handle_key(int ch);

/* Check if dropdown is currently open */
bool dropdown_is_open(void);

/* ============================================================================
 * dialog.c - Pop-up dialogs
 * ============================================================================ */

/* Show the save/abort/return prompt */
DialogResult dialog_save_prompt(void);

/* Show a simple message box */
void dialog_message(const char *title, const char *message);

/* Show a confirmation dialog, returns true if confirmed */
bool dialog_confirm(const char *title, const char *message);

/* Show an option picker dialog, returns selected index or -1 if cancelled */
int dialog_option_picker(const char *title, const char **options, int current_idx);

typedef struct {
    const char *name;
    const char *help;
    const char *category;
} PickerOption;

int picker_with_help_show(const char *title, const PickerOption *options, int num_options, int current_idx);

int modifier_picker_show(int current_idx);
const char *modifier_picker_get_name(int index);

int command_picker_show(int current_idx);
const char *command_picker_get_name(int index);

int privilege_picker_show(int current_idx);
const char *privilege_picker_get_name(int index);

/* Draw a bordered box */
void draw_box(int y, int x, int height, int width, int color_pair);

/* ============================================================================
 * colorpicker.c - Color picker
 * ============================================================================ */

/* Initialize color picker color pairs */
void colorpicker_init(void);

/* Get the display name for a DOS color (0-15) */
const char *color_get_name(int color);

/* Show a simple color picker dropdown, returns color (0-15) or -1 if cancelled */
int colorpicker_select(int current, int screen_y, int screen_x);

/* Show full color grid picker (fg + bg), returns 1 if selected, 0 if cancelled */
int colorpicker_select_full(int current_fg, int current_bg, int *out_fg, int *out_bg);

/* ============================================================================
 * colorform.c - Color editing form
 * ============================================================================ */

/* Edit default colors (menu, file, message, FSR colors) */
void action_default_colors(void);
void action_file_colors(void);
void action_msg_colors(void);
void action_fsr_colors(void);

/* ============================================================================
 * form.c - Form editor
 * ============================================================================ */

#include "fields.h"

/* Show a form for editing fields, returns true if saved
 * If dirty_fields is not NULL, it will be filled with indices of modified fields
 * and dirty_count will be set to the number of dirty fields */
bool form_edit(const char *title, const FieldDef *fields, int field_count, char **values,
               int *dirty_fields, int *dirty_count);

/* Optional preview hook for form_edit (invoked on F4 when enabled).
 * Callers should set this before calling form_edit, then clear it after.
 */
void form_set_preview_action(void (*action)(void *ctx), void *ctx);

extern int g_form_last_action_key;

/* ============================================================================
 * filepicker.c - File selection dialog
 * ============================================================================ */

/* Show file picker dialog, returns selected filename (without extension) or NULL if cancelled
 * base_path: directory to list files from (e.g., "display/screens")
 * filter: extension filter (e.g., "*.bbs")
 * current: current value to highlight, or NULL
 * Returns: malloc'd string with full path (e.g., "display/screens/logo") or NULL
 */
char *filepicker_select(const char *base_path, const char *filter, const char *current);

/* ============================================================================
 * text_list_editor.c - Simple text list editor
 * ============================================================================ */

/* Edit a simple text file as a list of entries (one per line)
 * Returns true if the list was modified */
bool text_list_editor(const char *title, const char *filepath, const char *help_text);

/* ============================================================================
 * listpicker.c - Scrollable list picker dialog
 * ============================================================================ */

/* List picker result codes */
typedef enum {
    LISTPICK_NONE = 0,
    LISTPICK_EDIT,      /* User pressed ENTER */
    LISTPICK_INSERT,    /* User pressed INS */
    LISTPICK_ADD,       /* User pressed A */
    LISTPICK_DELETE,    /* User pressed DEL (toggle enable/disable) */
    LISTPICK_FILTER,    /* User pressed SPACE (filter/search) */
    LISTPICK_CLEAR,
    LISTPICK_EXIT       /* User pressed ESC */
} ListPickResult;

/* List item structure */
typedef struct {
    char *name;         /* Display name */
    char *extra;        /* Extra info (shown in parentheses) */
    bool enabled;       /* Is this item enabled? */
    void *data;         /* User data pointer */
} ListItem;

/* Show a list picker dialog
 * title: Dialog title
 * items: Array of list items
 * item_count: Number of items
 * selected: Pointer to selected index (updated on return)
 * Returns: ListPickResult indicating what action was taken
 */
ListPickResult listpicker_show(const char *title, ListItem *items, int item_count, int *selected);

ListPickResult listpicker_show_ex(const char *title, ListItem *items, int item_count, int *selected, bool space_is_filter);

/* Helper functions for managing list items */
ListItem *listitem_create(const char *name, const char *extra, void *data);
void listitem_free(ListItem *item);
void listitem_array_free(ListItem *items, int count);

/* ============================================================================
 * checkpicker.c - Multi-select checkbox picker dialog
 * ============================================================================ */

/* Check item structure */
typedef struct {
    const char *name;   /* Display name */
    const char *value;  /* Value to use in result string (if different) */
    bool checked;       /* Is this item checked? */
} CheckItem;

/* Show a multi-select checkbox picker dialog
 * title: Dialog title
 * items: Array of check items (modified in place)
 * item_count: Number of items
 * Returns: true if user confirmed, false if cancelled
 */
bool checkpicker_show(const char *title, CheckItem *items, int item_count);

/* Helper to build space-separated string from checked items */
char *checkpicker_build_string(CheckItem *items, int item_count);

/* Helper to set checks from space-separated string */
void checkpicker_parse_string(CheckItem *items, int item_count, const char *str);

#endif /* UI_H */
