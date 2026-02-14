/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * menubar.c - Top menu bar for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include "compiler.h"
#include "max_u.h"
#include "userapi.h"
#include "md5.h"
#include "progprot.h"
#include "maxcfg.h"
#include "ui.h"
#include "fields.h"
#include "ctl_parse.h"
#include "area_parse.h"
#include "area_toml.h"
#include "treeview.h"
#include "menu_data.h"
#include "menu_edit.h"
#include "menu_preview.h"
#include "nextgen_export.h"
#include "lang_convert.h"
#include "lang_browse.h"
#include "texteditor.h"

/* Forward declarations for menu actions */
static void action_placeholder(void);
static void action_save_config(void);
static void action_export_nextgen_config(void);
static void action_convert_legacy_lang(void);
static void action_bbs_sysop_info(void);
static void action_system_paths(void);
static void action_msg_reader_menu(void);
static void action_display_files(void);
static void action_logging_options(void);
static void action_global_toggles(void);
static void action_login_settings(void);
static void action_new_user_defaults(void);
static void action_matrix_netmail_settings(void);
static void action_matrix_privileges(void);
static void action_matrix_message_attr_privs(void);
static void action_languages(void);
static void action_protocols(void);
static void action_events(void);
static void action_reader_settings(void);
static void action_protocol_list(void *unused);
static void action_edit_compress_cfg(void *unused);

static void action_user_editor(void);
static void action_bad_users(void);
static void action_reserved_names(void);

/* Message area actions */
static void action_msg_tree_config(void);
static void action_msg_divisions_picklist(void);
static void action_msg_areas_picklist(void);

static bool load_msg_areas_toml_for_ui(const char *sys_path, char *out_path, size_t out_path_sz, char *err, size_t err_sz);

static bool load_file_areas_toml_for_ui(const char *sys_path, char *out_path, size_t out_path_sz, char *err, size_t err_sz);

/* Menu configuration action */
static void action_menus_list(void);
static void action_lang_editor(void);

static bool edit_menu_properties(const char *sys_path, MenuDefinition **menus, int menu_count, MenuDefinition *menu);
static bool menu_options_list(const char *sys_path, MenuDefinition **menus, int menu_count, MenuDefinition *menu);
static bool edit_menu_option(const char *sys_path, MenuDefinition **menus, int menu_count, MenuDefinition *menu, int opt_index);

/* Security levels action */
static void action_security_levels(void);

static void warn_missing_paths(const char *title, const char **labels, const char **paths, const bool *exists, int count);
static void warn_missing_display_files(const char **labels, const char **paths, const bool *exists, int count);

static const char *current_sys_path(void);
static int parse_priv_level(const char *sys_path, const char *level_name);

static bool user_editor_prompt_filter(char **io_filter);
static bool user_editor_wild_match_ci(const char *pattern, const char *text);
static bool user_editor_contains_ci(const char *haystack, const char *needle);
static bool user_editor_filter_has_wildcards(const char *s);
static void user_editor_usr_field_to_cstr(char *dst, size_t dst_sz, const byte *src, size_t src_len);
static void user_editor_set_usr_field(byte *dst, size_t dst_len, const char *src);
static char *user_editor_resolve_userfile_root(const char *sys_path, const char *raw);
static void user_editor_edit_user_categories(HUF huf, long rec, const char *sys_path);
static void user_editor_edit_settings(HUF huf, long rec, const char *sys_path);
static void user_editor_edit_statistics(HUF huf, long rec, const char *sys_path);
static void user_editor_edit_dates(HUF huf, long rec, const char *sys_path);
static void user_editor_edit_keys_flags(HUF huf, long rec, const char *sys_path);
static bool user_editor_prompt_password(char *pwd_buf, size_t pwd_sz);
static void user_editor_password_action(void *ctx);
static void user_editor_edit_personal(HUF huf, long rec, const char *sys_path);
static void user_editor_edit_security(HUF huf, long rec, const char *sys_path);

/* ============================================================================
 * Menu Definitions
 * ============================================================================ */

/* Setup > Global submenu */
static MenuItem setup_global_items[] = {
    { "BBS and Sysop Information", "B", NULL, 0, action_bbs_sysop_info, true },
    { "System Paths",              "S", NULL, 0, action_system_paths, true },
    { "Message Reader Menu",       "M", NULL, 0, action_msg_reader_menu, true },
    { "Logging Options",           "L", NULL, 0, action_logging_options, true },
    { "Global Toggles",            "G", NULL, 0, action_global_toggles, true },
    { "Login Settings",            "o", NULL, 0, action_login_settings, true },
    { "New User Defaults",         "N", NULL, 0, action_new_user_defaults, true },
    { "Default Colors",            "C", NULL, 0, action_default_colors, true },
};

static void action_network_addresses(void);

/* Setup > Matrix submenu */
static MenuItem setup_matrix_items[] = {
    { "Network Addresses",  "N", NULL, 0, action_network_addresses, true },
    { "Netmail Settings",   "e", NULL, 0, action_matrix_netmail_settings, true },
    { "Privileges",        "P", NULL, 0, action_matrix_privileges, true },
    { "Message Attributes", "A", NULL, 0, action_matrix_message_attr_privs, true },
    { "Events",             "E", NULL, 0, action_events, true },
};

/* Setup menu items */
static MenuItem setup_items[] = {
    { "Global",           "G", setup_global_items, 8, NULL, true },
    { "Security Levels",  "S", NULL, 0, action_security_levels, true },
    { "Reader Settings",  "R", NULL, 0, action_reader_settings, true },
    { "Protocols",        "P", NULL, 0, action_protocols, true },
    { "Languages",        "L", NULL, 0, action_languages, true },
    { "Matrix/Echomail",  "M", setup_matrix_items, 5, NULL, true },
};

/* Content menu items */
static MenuItem content_items[] = {
    { "Menus",            "M", NULL, 0, action_menus_list, true },
    { "Display Files",    "D", NULL, 0, action_display_files, true },
    { "Language Strings", "L", NULL, 0, action_lang_editor, true },
    { "Help Files",       "H", NULL, 0, action_placeholder, true },
    { "Bulletins",        "B", NULL, 0, action_placeholder, true },
};

/* Messages > Setup Message Areas submenu */
static MenuItem msg_setup_items[] = {
    { "Tree Configuration",         "T", NULL, 0, action_msg_tree_config, true },
    { "Picklist: Message Divisions","D", NULL, 0, action_msg_divisions_picklist, true },
    { "Picklist: Message Areas",    "A", NULL, 0, action_msg_areas_picklist, true },
};

/* Messages menu items */
static MenuItem messages_items[] = {
    { "Setup Message Areas",   "S", msg_setup_items, 3, NULL, true },
    { "Netmail Aliases",       "N", NULL, 0, action_placeholder, true },
    { "Matrix and Echomail",   "M", NULL, 0, action_placeholder, true },
    { "Squish Configuration",  "q", NULL, 0, action_placeholder, true },
    { "QWK Mail and Networking","Q", NULL, 0, action_placeholder, true },
};

/* Forward declarations for file area actions */
static void action_file_tree_config(void);
static void action_file_divisions_picklist(void);
static void action_file_areas_picklist(void);

/* Files > Setup File Areas submenu */
static MenuItem file_setup_items[] = {
    { "Tree Configuration",       "T", NULL, 0, action_file_tree_config, true },
    { "Picklist: File Divisions", "D", NULL, 0, action_file_divisions_picklist, true },
    { "Picklist: File Areas",     "A", NULL, 0, action_file_areas_picklist, true },
};

/* Files menu items */
static MenuItem files_items[] = {
    { "Setup File Areas",  "S", file_setup_items, 3, NULL, true },
    { "Protocol Config",   "P", NULL, 0, action_placeholder, true },
    { "Archiver Config",   "A", NULL, 0, action_placeholder, true },
};

/* Users menu items */
static MenuItem users_items[] = {
    { "User Editor",     "U", NULL, 0, action_user_editor, true },
    { "Bad Users",       "B", NULL, 0, action_bad_users, true },
    { "Reserved Names",  "R", NULL, 0, action_reserved_names, true },
};

/* Tools menu items */
static MenuItem tools_items[] = {
    { "Save",              "S", NULL, 0, action_save_config, true },
    { "Import Legacy Config (CTL)", "I", NULL, 0, action_export_nextgen_config, true },
    { "Convert Legacy Language (MAD)", "L", NULL, 0, action_convert_legacy_lang, true },
    { "View Log",           "V", NULL, 0, action_placeholder, true },
    { "System Information", "n", NULL, 0, action_placeholder, true },
};

/* Top-level menus */
static TopMenu top_menus[] = {
    { "Setup",       setup_items,       6 },
    { "Content",     content_items,     4 },
    { "Messages",    messages_items,    5 },
    { "Files",       files_items,       3 },
    { "Users",       users_items,       3 },
    { "Tools",       tools_items,       5 },
};

#define NUM_TOP_MENUS (sizeof(top_menus) / sizeof(top_menus[0]))

/* Menu positions (calculated on init) */
static int menu_positions[NUM_TOP_MENUS];

/* ============================================================================
 * Implementation
 * ============================================================================ */

static void action_placeholder(void)
{
    dialog_message("Not Implemented", 
                   "This feature is not yet implemented.\n\n"
                   "Coming soon!");
}

typedef struct {
    const char *sys_path;
    MenuDefinition *menu;
    char **overlay_values;
    int overlay_kind;
} MenuPreviewCtx;

static int g_menu_preview_view_priv_idx = 3; /* Default to Normal */

enum {
    MENU_PREVIEW_OVERLAY_NONE = 0,
    MENU_PREVIEW_OVERLAY_PROPERTIES = 1,
    MENU_PREVIEW_OVERLAY_CUSTOMIZATION = 2,
};

static void menu_preview_overlay_free(MenuDefinition *m)
{
    if (!m) return;
    free(m->title);
    free(m->header_file);
    free(m->menu_file);
}

static void preview_fill_rect_black(int top, int left, int height, int width)
{
    if (top < 0 || left < 0 || height < 1 || width < 1) return;
    if (top >= LINES || left >= COLS) return;

    if (top + height > LINES) height = LINES - top;
    if (left + width > COLS) width = COLS - left;

    attron(COLOR_PAIR(CP_DIALOG_TEXT));
    for (int row = 0; row < height; row++) {
        move(top + row, left);
        for (int col = 0; col < width; col++) {
            addch(' ');
        }
    }
    attroff(COLOR_PAIR(CP_DIALOG_TEXT));
}

static void preview_draw_frame(int x, int y, int w, int h)
{
    attron(COLOR_PAIR(CP_DIALOG_BORDER));

    if (y > 0) {
        if (x > 0) mvaddch(y - 1, x - 1, ACS_ULCORNER);
        mvhline(y - 1, x, ACS_HLINE, w);
        if (x + w < COLS) mvaddch(y - 1, x + w, ACS_URCORNER);
    }
    if (y + h < LINES) {
        if (x > 0) mvaddch(y + h, x - 1, ACS_LLCORNER);
        mvhline(y + h, x, ACS_HLINE, w);
        if (x + w < COLS) mvaddch(y + h, x + w, ACS_LRCORNER);
    }

    if (x > 0) {
        for (int row = 0; row < h; row++) {
            mvaddch(y + row, x - 1, ACS_VLINE);
        }
    }
    if (x + w < COLS) {
        for (int row = 0; row < h; row++) {
            mvaddch(y + row, x + w, ACS_VLINE);
        }
    }

    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
}

static bool preview_terminal_is_too_small(int min_cols, int min_rows)
{
    return (COLS < min_cols || LINES < min_rows);
}

static void menu_preview_stub(void *ctx)
{
    MenuPreviewCtx *p = (MenuPreviewCtx *)ctx;
    MenuDefinition *base_menu = (p && p->menu) ? p->menu : NULL;

    int view_priv_idx = g_menu_preview_view_priv_idx;
    view_priv_idx = privilege_picker_show(view_priv_idx);
    if (view_priv_idx < 0) {
        return;
    }
    g_menu_preview_view_priv_idx = view_priv_idx;

    const char *view_priv_name = privilege_picker_get_name(view_priv_idx);
    int view_level = parse_priv_level((p && p->sys_path) ? p->sys_path : "", view_priv_name ? view_priv_name : "");

    /* ANSI preview is always non-RIP. */
    bool view_is_rip = false;

    if (preview_terminal_is_too_small(80, 25)) {
        dialog_message("Terminal too small",
                       "Menu preview requires an 80x25 terminal.\n"
                       "Restart maxcfg in an 80x25 window.");
        return;
    }

    const int pv_w = 80;
    const int pv_h = 25;

    int x = (COLS - pv_w) / 2;
    int y = (LINES - pv_h) / 2;

    WINDOW *saved = dupwin(stdscr);
    if (saved == NULL) {
        dialog_message("Error", "Unable to allocate screen buffer for preview.");
        return;
    }

    int selected = -1;

    bool done = false;
    while (!done) {
        overwrite(saved, stdscr);

        int left = (x > 0) ? x - 1 : x;
        int top = (y > 0) ? y - 1 : y;
        int right = (x + pv_w < COLS) ? x + pv_w : x + pv_w - 1;
        int bottom = (y + pv_h < LINES) ? y + pv_h : y + pv_h - 1;
        preview_fill_rect_black(top, left, bottom - top + 1, right - left + 1);
        preview_draw_frame(x, y, pv_w, pv_h);

        MenuPreviewVScreen vs;
        MenuPreviewLayout layout = {0};

        MenuDefinition overlay;
        MenuDefinition *menu = base_menu;

        if (base_menu && p && p->overlay_values && p->overlay_kind != MENU_PREVIEW_OVERLAY_NONE) {
            overlay = *base_menu;

            if (p->overlay_kind == MENU_PREVIEW_OVERLAY_PROPERTIES) {
                overlay.title = base_menu->title ? strdup(base_menu->title) : NULL;
                overlay.header_file = base_menu->header_file ? strdup(base_menu->header_file) : NULL;
                overlay.menu_file = base_menu->menu_file ? strdup(base_menu->menu_file) : NULL;
                (void)menu_save_properties_form(&overlay, p->overlay_values);
            } else if (p->overlay_kind == MENU_PREVIEW_OVERLAY_CUSTOMIZATION) {
                (void)menu_save_customization_form(&overlay, p->overlay_values);
            }

            menu = &overlay;
        }

        bool interactive = (menu && menu->cm_enabled && menu->cm_lightbar);

        MenuDefinition filtered;
        MenuDefinition *menu_for_preview = menu;
        MenuOption **filtered_options = NULL;

        if (menu_for_preview && menu_for_preview->options && menu_for_preview->option_count > 0) {
            filtered_options = calloc((size_t)menu_for_preview->option_count, sizeof(MenuOption *));
            if (filtered_options != NULL) {
                int out_count = 0;
                for (int i = 0; i < menu_for_preview->option_count; i++) {
                    MenuOption *opt = menu_for_preview->options[i];
                    if (!opt) continue;
                    if (opt->flags & OFLAG_NODSP) continue;
                    if (!opt->description || !opt->description[0]) continue;

                    if ((opt->flags & OFLAG_RIP) != 0u && !view_is_rip) continue;

                    int req_level = parse_priv_level((p && p->sys_path) ? p->sys_path : "", opt->priv_level ? opt->priv_level : "");
                    if (view_level >= req_level) {
                        filtered_options[out_count++] = opt;
                    }
                }

                filtered = *menu_for_preview;
                filtered.options = filtered_options;
                filtered.option_count = out_count;
                menu_for_preview = &filtered;
            }
        }

        menu_preview_render(menu_for_preview, &vs, &layout, interactive ? selected : -1);

        if (interactive && selected < 0 && layout.count > 0) {
            selected = 0;
        }
        if (interactive && layout.count > 0) {
            if (selected < 0) selected = 0;
            if (selected >= layout.count) selected = layout.count - 1;
        }

        attron(COLOR_PAIR(CP_DIALOG_TEXT));
        menu_preview_blit(menu_for_preview, &vs, interactive ? &layout : NULL, interactive ? selected : -1, x, y);
        if (interactive) {
            mvprintw(y + pv_h - 1, x + 0, "Arrows=Move  ENTER=Select  ESC/F4=Back");
        } else {
            mvprintw(y + pv_h - 1, x + 0, "ESC/F4 = Back");
        }
        attroff(COLOR_PAIR(CP_DIALOG_TEXT));

        if (filtered_options != NULL) {
            free(filtered_options);
        }

        if (menu == &overlay && p && p->overlay_kind == MENU_PREVIEW_OVERLAY_PROPERTIES) {
            menu_preview_overlay_free(&overlay);
        }

        doupdate();
        int ch = getch();

        if (interactive && layout.count > 0) {
            int cols = (layout.cols > 0) ? layout.cols : 1;
            int rows = (layout.count + cols - 1) / cols;
            int r = (selected >= 0) ? (selected / cols) : 0;
            int c = (selected >= 0) ? (selected % cols) : 0;

            if (ch == KEY_LEFT) {
                if (c > 0) c--;
                else c = cols - 1;
                int idx = r * cols + c;
                if (idx >= layout.count) idx = layout.count - 1;
                selected = idx;
            } else if (ch == KEY_RIGHT) {
                if (c < cols - 1) c++;
                else c = 0;
                int idx = r * cols + c;
                if (idx >= layout.count) idx = r * cols;
                if (idx >= layout.count) idx = layout.count - 1;
                selected = idx;
            } else if (ch == KEY_UP) {
                if (r > 0) r--;
                else r = rows - 1;
                int idx = r * cols + c;
                if (idx >= layout.count) {
                    idx = layout.count - 1;
                }
                selected = idx;
            } else if (ch == KEY_DOWN) {
                if (r < rows - 1) r++;
                else r = 0;
                int idx = r * cols + c;
                if (idx >= layout.count) {
                    idx = (rows - 1) * cols + c;
                    while (idx >= layout.count && idx > 0) idx--;
                }
                selected = idx;
            } else if (ch == '\n' || ch == '\r') {
                if (selected >= 0 && selected < layout.count && layout.items && layout.items[selected].desc) {
                    dialog_message("Preview", layout.items[selected].desc);
                }
            } else if (isprint(ch)) {
                int idx = -1;
                if (menu_preview_hotkey_to_index(&layout, ch, &idx)) {
                    selected = idx;
                }
            }
        }

        menu_preview_layout_free(&layout);

        switch (ch) {
            case 27:
            case KEY_F(4):
                done = true;
                break;
            case KEY_RESIZE:
                resize_term(0, 0);
                if (preview_terminal_is_too_small(80, 25)) {
                    dialog_message("Terminal too small",
                                   "Menu preview requires an 80x25 terminal.\n"
                                   "Restart maxcfg in an 80x25 window.");
                    done = true;
                    break;
                }
                x = (COLS - pv_w) / 2;
                y = (LINES - pv_h) / 2;

                delwin(saved);
                saved = dupwin(stdscr);
                if (saved == NULL) {
                    dialog_message("Error", "Unable to allocate screen buffer for preview.");
                    done = true;
                }
                break;
        }
    }

    overwrite(saved, stdscr);
    delwin(saved);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
}

static void action_save_config(void)
{
    if (g_maxcfg_toml == NULL) {
        dialog_message("Save Failed", "TOML configuration is not loaded.");
        return;
    }

    if (!g_state.dirty) {
        dialog_message("Save", "No changes to save.");
        return;
    }

    MaxCfgStatus st = maxcfg_toml_persist_overrides_and_save(g_maxcfg_toml);
    if (st != MAXCFG_OK) {
        dialog_message("Save Failed", maxcfg_status_string(st));
        return;
    }

    g_state.dirty = false;
    dialog_message("Save", "Saved.");
}

/* CTL sync removed - TOML is now authoritative */

static const char *toml_get_string_or_empty(const char *path)
{
    MaxCfgVar v;
    if (g_maxcfg_toml != NULL &&
        maxcfg_toml_get(g_maxcfg_toml, path, &v) == MAXCFG_OK &&
        v.type == MAXCFG_VAR_STRING &&
        v.v.s != NULL) {
        return v.v.s;
    }
    return "";
}

static bool toml_get_bool_or_default(const char *path, bool def)
{
    MaxCfgVar v;
    if (g_maxcfg_toml != NULL &&
        maxcfg_toml_get(g_maxcfg_toml, path, &v) == MAXCFG_OK &&
        v.type == MAXCFG_VAR_BOOL) {
        return v.v.b;
    }
    return def;
}

static int toml_get_int_or_default(const char *path, int def)
{
    MaxCfgVar v;
    if (g_maxcfg_toml != NULL &&
        maxcfg_toml_get(g_maxcfg_toml, path, &v) == MAXCFG_OK &&
        v.type == MAXCFG_VAR_INT) {
        return v.v.i;
    }
    return def;
}

static char *msg_reader_menu_values[1] = { NULL };

static const FieldDef msg_reader_menu_fields[] = {
    {
        .keyword = "msg_reader_menu",
        .label = "Reader Menu",
        .help = "Menu name used as the authoritative command set for the full-screen message reader (FSR).",
        .type = FIELD_TEXT,
        .max_length = 24,
        .default_value = "MSGREAD"
    },
};

static void action_msg_reader_menu(void)
{
    free(msg_reader_menu_values[0]);

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    const char *cur = toml_get_string_or_empty("maximus.msg_reader_menu");
    if (cur[0] == '\0') {
        cur = "MSGREAD";
    }
    msg_reader_menu_values[0] = strdup(cur);
    if (msg_reader_menu_values[0] == NULL) {
        dialog_message("Error", "Out of memory.");
        return;
    }

    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("Message Reader Menu", msg_reader_menu_fields, 1, msg_reader_menu_values, dirty_fields, &dirty_count);
    if (saved) {
        const char *v = msg_reader_menu_values[0] ? msg_reader_menu_values[0] : "";
        if (v[0] == '\0') {
            (void)maxcfg_toml_override_unset(g_maxcfg_toml, "maximus.msg_reader_menu");
        } else {
            (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.msg_reader_menu", v);
        }
        g_state.dirty = true;
    }
}

static const char *access_level_name_for_level(const char *sys_path, int level)
{
    if (sys_path == NULL || sys_path[0] == '\0') {
        if (level == 65535) {
            return "Hidden";
        }
        return NULL;
    }
    for (int i = 0; access_level_options[i]; i++) {
        if (parse_priv_level(sys_path, access_level_options[i]) == level) {
            return access_level_options[i];
        }
    }
    if (level == 65535) {
        return "Hidden";
    }
    return NULL;
}

static int parse_priv_level(const char *sys_path, const char *level_name)
{
    if (sys_path == NULL || sys_path[0] == '\0' || level_name == NULL) {
        return 0;
    }

    char tmp[128];
    if (snprintf(tmp, sizeof(tmp), "%s", level_name) >= (int)sizeof(tmp)) {
        return 0;
    }

    /* Trim whitespace in-place */
    char *t = tmp;
    while (*t && isspace((unsigned char)*t)) {
        t++;
    }
    if (*t == '\0') {
        return 0;
    }
    char *e = t + strlen(t);
    while (e > t && isspace((unsigned char)e[-1])) {
        e--;
    }
    *e = '\0';

    /* Try numeric value first */
    char *end = NULL;
    long iv = strtol(t, &end, 10);
    if (end != NULL) {
        while (*end && isspace((unsigned char)*end)) {
            end++;
        }
        if (*end == '\0') {
            return (int)iv;
        }
    }

    if (strcasecmp(t, "hidden") == 0) {
        return 65535;
    }

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

    char access_ctl[PATH_MAX];
    if (snprintf(access_ctl, sizeof(access_ctl), "%s/etc/access.ctl", sys_path) >= (int)sizeof(access_ctl)) {
        return 0;
    }

    FILE *fp = fopen(access_ctl, "r");
    if (!fp) {
        return 0;
    }

    char line[1024];
    bool in_access = false;
    bool found_name = false;
    int level = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *s = line;
        while (*s && isspace((unsigned char)*s)) {
            s++;
        }
        if (*s == '\0' || *s == '%' || *s == ';') {
            continue;
        }

        char *nl = strchr(s, '\n');
        if (nl) {
            *nl = '\0';
        }
        while (*s && isspace((unsigned char)*s)) {
            s++;
        }
        char *se = s + strlen(s);
        while (se > s && isspace((unsigned char)se[-1])) {
            se--;
        }
        *se = '\0';
        if (*s == '\0' || *s == '%' || *s == ';') {
            continue;
        }

        if (strncasecmp(s, "Access", 6) == 0 && isspace((unsigned char)s[6])) {
            char *v = s + 6;
            while (*v && isspace((unsigned char)*v)) {
                v++;
            }
            if (*v && strcasecmp(v, t) == 0) {
                in_access = true;
                found_name = true;
            } else {
                in_access = false;
            }
            continue;
        }

        if (in_access && strncasecmp(s, "Level", 5) == 0 && isspace((unsigned char)s[5])) {
            char *v = s + 5;
            while (*v && isspace((unsigned char)*v)) {
                v++;
            }
            if (*v) {
                level = atoi(v);
                break;
            }
        }

        if (strncasecmp(s, "End Access", 10) == 0) {
            if (found_name) {
                break;
            }
            in_access = false;
        }
    }

    fclose(fp);
    return level;
}

static int toml_get_table_int_or_default(const char *table_path, const char *key, int def, bool *out_found)
{
    if (out_found) {
        *out_found = false;
    }

    if (g_maxcfg_toml == NULL || table_path == NULL || key == NULL) {
        return def;
    }

    MaxCfgVar tbl;
    if (maxcfg_toml_get(g_maxcfg_toml, table_path, &tbl) != MAXCFG_OK || tbl.type != MAXCFG_VAR_TABLE) {
        return def;
    }

    MaxCfgVar v;
    MaxCfgStatus st = maxcfg_toml_table_get(&tbl, key, &v);
    if (st != MAXCFG_OK || v.type != MAXCFG_VAR_INT) {
        return def;
    }

    if (out_found) {
        *out_found = true;
    }
    return v.v.i;
}

static char *normalize_under_sys_path(const char *sys_path, const char *path)
{
    if (path == NULL) {
        return strdup("");
    }

    if (sys_path == NULL || sys_path[0] == '\0' || path[0] == '\0') {
        return strdup(path);
    }

    bool mex = false;
    const char *p = path;
    if (p[0] == ':') {
        mex = true;
        p++;
    }

    size_t sys_len = strlen(sys_path);
    while (sys_len > 1u && (sys_path[sys_len - 1u] == '/' || sys_path[sys_len - 1u] == '\\')) {
        sys_len--;
    }

    if (strncmp(p, sys_path, sys_len) == 0 && (p[sys_len] == '/' || p[sys_len] == '\\')) {
        const char *rel = p + sys_len + 1u;
        if (rel[0] == '\0') {
            return strdup(path);
        }

        size_t rel_len = strlen(rel);
        size_t out_len = rel_len + (mex ? 1u : 0u) + 1u;
        char *out = malloc(out_len);
        if (out == NULL) {
            return strdup(path);
        }
        if (mex) {
            out[0] = ':';
            memcpy(out + 1u, rel, rel_len + 1u);
        } else {
            memcpy(out, rel, rel_len + 1u);
        }
        return out;
    }

    return strdup(path);
}

static bool path_is_absolute(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return false;
    }
    if (path[0] == '/' || path[0] == '\\') {
        return true;
    }
    if (isalpha((unsigned char)path[0]) && path[1] == ':') {
        return true;
    }
    return false;
}

static bool path_exists(const char *path)
{
    struct stat st;
    if (path == NULL || path[0] == '\0') {
        return false;
    }
    return stat(path, &st) == 0;
}

static void warn_missing_paths(const char *title, const char **labels, const char **paths, const bool *exists, int count)
{
    if (labels == NULL || paths == NULL || exists == NULL || count <= 0) {
        return;
    }

    size_t cap = 1024;
    char *msg = (char *)malloc(cap);
    if (msg == NULL) {
        return;
    }
    msg[0] = '\0';

    bool any = false;
    for (int i = 0; i < count; i++) {
        if (exists[i]) {
            continue;
        }

        const char *label = labels[i] ? labels[i] : "";
        const char *path = paths[i] ? paths[i] : "";
        if (path[0] == '\0') {
            continue;
        }

        char line[512];
        (void)snprintf(line, sizeof(line), "%s: %s\n", label, path);

        size_t need = strlen(msg) + strlen(line) + 1u;
        if (need > cap) {
            size_t new_cap = cap;
            while (need > new_cap) {
                new_cap *= 2u;
            }
            char *p = (char *)realloc(msg, new_cap);
            if (p == NULL) {
                break;
            }
            msg = p;
            cap = new_cap;
        }

        strcat(msg, line);
        any = true;
    }

    if (any) {
        dialog_message(title ? title : "Warning", msg);
    }
    free(msg);
}

static void warn_missing_display_files(const char **labels, const char **paths, const bool *exists, int count)
{
    warn_missing_paths("Display Files Warning", labels, paths, exists, count);
}

static bool path_has_extension(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return false;
    }

    const char *slash = strrchr(path, '/');
    const char *bslash = strrchr(path, '\\');
    const char *sep = slash;
    if (bslash != NULL && (sep == NULL || bslash > sep)) {
        sep = bslash;
    }
    const char *base = sep ? (sep + 1) : path;

    return strchr(base, '.') != NULL;
}

static bool display_file_variant_exists(const char *sys_path, const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return true;
    }

    bool mex = false;
    const char *p = path;
    if (p[0] == ':') {
        mex = true;
        p++;
    }

    char resolved[MAX_PATH_LEN];
    resolved[0] = '\0';

    if (path_is_absolute(p)) {
        strncpy(resolved, p, sizeof(resolved) - 1u);
        resolved[sizeof(resolved) - 1u] = '\0';
    } else {
        if (sys_path != NULL && sys_path[0] != '\0') {
            (void)maxcfg_resolve_path(sys_path, p, resolved, sizeof(resolved));
        } else {
            strncpy(resolved, p, sizeof(resolved) - 1u);
            resolved[sizeof(resolved) - 1u] = '\0';
        }
    }

    /* If the user explicitly included an extension, validate exactly that.
     * Otherwise, treat it as a base name and accept any supported variant.
     */
    if (path_has_extension(resolved)) {
        return path_exists(resolved);
    }

    if (mex) {
        char tmp[MAX_PATH_LEN];
        if (snprintf(tmp, sizeof(tmp), "%s.vm", resolved) < (int)sizeof(tmp)) {
            if (path_exists(tmp)) {
                return true;
            }
        }
        return false;
    }

    static const char *exts[] = { ".bbs", ".gbs", ".ans", ".avt" };
    for (size_t i = 0; i < (sizeof(exts) / sizeof(exts[0])); i++) {
        char tmp[MAX_PATH_LEN];
        if (snprintf(tmp, sizeof(tmp), "%s%s", resolved, exts[i]) >= (int)sizeof(tmp)) {
            continue;
        }
        if (path_exists(tmp)) {
            return true;
        }
    }

    return false;
}

static char *canonicalize_for_display(const char *sys_path, const char *path, bool *out_exists)
{
    if (out_exists) {
        *out_exists = true;
    }

    if (path == NULL || path[0] == '\0') {
        return strdup("");
    }

    bool mex = false;
    const char *p = path;
    if (p[0] == ':') {
        mex = true;
        p++;
    }

    char resolved[MAX_PATH_LEN];
    resolved[0] = '\0';

    if (path_is_absolute(p)) {
        strncpy(resolved, p, sizeof(resolved) - 1u);
        resolved[sizeof(resolved) - 1u] = '\0';
    } else {
        if (sys_path != NULL && sys_path[0] != '\0') {
            (void)maxcfg_resolve_path(sys_path, p, resolved, sizeof(resolved));
        } else {
            strncpy(resolved, p, sizeof(resolved) - 1u);
            resolved[sizeof(resolved) - 1u] = '\0';
        }
    }

    char canon[MAX_PATH_LEN];
    char *canon_p = NULL;
    errno = 0;
    canon_p = realpath(resolved, canon);

    const char *final_abs = canon_p ? canon : resolved;
    bool exists = path_exists(final_abs);
    if (out_exists) {
        *out_exists = exists;
    }

    /* Display paths the same way we save them: relative to sys_path when under it.
     * Note: we use the resolved/canonical absolute for prefix checks so callers don't
     * get surprised by absolute paths in the UI.
     */
    char *display_path = NULL;
    if (sys_path != NULL && sys_path[0] != '\0') {
        display_path = normalize_under_sys_path(sys_path, final_abs);
    } else {
        display_path = strdup(final_abs);
    }
    if (display_path == NULL) {
        return strdup(path);
    }

    if (mex) {
        size_t n = strlen(display_path);
        char *out = malloc(n + 2u);
        if (out == NULL) {
            free(display_path);
            return strdup(path);
        }
        out[0] = ':';
        memcpy(out + 1u, display_path, n + 1u);
        free(display_path);
        return out;
    }

    return display_path;
}

static const char *current_sys_path(void)
{
    const char *sys_path = toml_get_string_or_empty("maximus.sys_path");
    if (sys_path != NULL && sys_path[0] != '\0') {
        return sys_path;
    }
    return g_state.config_path;
}

/* Current field values - loaded from PRM */
static char *bbs_sysop_values[7] = { NULL };
static char *system_paths_values[8] = { NULL };

static void action_bbs_sysop_info(void)
{
    /* Free old values */
    for (int i = 0; i < 7; i++) free(bbs_sysop_values[i]);

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    bbs_sysop_values[0] = strdup(toml_get_string_or_empty("maximus.system_name"));
    bbs_sysop_values[1] = strdup(toml_get_string_or_empty("maximus.sysop"));

    bbs_sysop_values[2] = strdup(toml_get_bool_or_default("general.session.alias_system", false) ? "Yes" : "No");
    bbs_sysop_values[3] = strdup(toml_get_bool_or_default("general.session.ask_alias", false) ? "Yes" : "No");
    bbs_sysop_values[4] = strdup(toml_get_bool_or_default("general.session.single_word_names", false) ? "Yes" : "No");
    bbs_sysop_values[5] = strdup(toml_get_bool_or_default("general.session.check_ansi", false) ? "Yes" : "No");
    bbs_sysop_values[6] = strdup(toml_get_bool_or_default("general.session.check_rip", false) ? "Yes" : "No");
    
    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("BBS and Sysop Information", bbs_sysop_fields, bbs_sysop_field_count, bbs_sysop_values, dirty_fields, &dirty_count);

    if (saved) {
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.system_name", bbs_sysop_values[0] ? bbs_sysop_values[0] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.sysop", bbs_sysop_values[1] ? bbs_sysop_values[1] : "");

        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "general.session.alias_system", strcmp(bbs_sysop_values[2], "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "general.session.ask_alias", strcmp(bbs_sysop_values[3], "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "general.session.single_word_names", strcmp(bbs_sysop_values[4], "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "general.session.check_ansi", strcmp(bbs_sysop_values[5], "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "general.session.check_rip", strcmp(bbs_sysop_values[6], "Yes") == 0);
        g_state.dirty = true;
    }
}

static void action_matrix_privileges(void)
{
    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    char **values = calloc((size_t)matrix_privileges_field_count, sizeof(char *));
    if (values == NULL) {
        dialog_message("Out of Memory", "Unable to allocate form values.");
        return;
    }

    const char *sys_path = current_sys_path();
    {
        int pv = toml_get_int_or_default("matrix.private_priv", 0);
        const char *nm = access_level_name_for_level(sys_path, pv);
        values[0] = strdup(nm ? nm : "");
    }
    {
        int pv = toml_get_int_or_default("matrix.fromfile_priv", 0);
        const char *nm = access_level_name_for_level(sys_path, pv);
        values[1] = strdup(nm ? nm : "");
    }
    {
        int pv = toml_get_int_or_default("matrix.unlisted_priv", 0);
        const char *nm = access_level_name_for_level(sys_path, pv);
        values[2] = strdup(nm ? nm : "");
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", toml_get_int_or_default("matrix.unlisted_cost", 0));
    values[3] = strdup(buf);
    values[4] = strdup(toml_get_bool_or_default("matrix.log_echomail", false) ? "Yes" : "No");

    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("Matrix Privileges", matrix_privileges_fields, matrix_privileges_field_count, values, dirty_fields, &dirty_count);

    if (saved) {
        if (values[0] == NULL || values[0][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "matrix.private_priv");
        else (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "matrix.private_priv", parse_priv_level(sys_path, values[0]));

        if (values[1] == NULL || values[1][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "matrix.fromfile_priv");
        else (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "matrix.fromfile_priv", parse_priv_level(sys_path, values[1]));

        if (values[2] == NULL || values[2][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "matrix.unlisted_priv");
        else (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "matrix.unlisted_priv", parse_priv_level(sys_path, values[2]));

        if (values[3] == NULL || values[3][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "matrix.unlisted_cost");
        else (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "matrix.unlisted_cost", atoi(values[3]));

        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "matrix.log_echomail", strcmp(values[4] ? values[4] : "No", "Yes") == 0);
        g_state.dirty = true;
    }

    for (int i = 0; i < matrix_privileges_field_count; i++) {
        free(values[i]);
    }
    free(values);
}

static bool edit_matrix_attr_table_entry(const char *table_path, const char *attribute)
{
    if (g_maxcfg_toml == NULL || table_path == NULL || attribute == NULL) {
        return false;
    }

    bool found = false;
    int pv = toml_get_table_int_or_default(table_path, attribute, 0, &found);

    char *values[2];
    values[0] = strdup(attribute);
    const char *nm = access_level_name_for_level(current_sys_path(), pv);
    values[1] = strdup(nm ? nm : "");

    int dirty[8];
    int dirty_count = 0;
    bool saved = form_edit("Edit Attribute Privilege", matrix_message_attr_priv_fields, matrix_message_attr_priv_field_count, values, dirty, &dirty_count);

    if (saved) {
        char path[256];
        if (snprintf(path, sizeof(path), "%s.%s", table_path, attribute) < (int)sizeof(path)) {
            if (values[1] == NULL || values[1][0] == '\0') {
                (void)maxcfg_toml_override_unset(g_maxcfg_toml, path);
            } else {
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, parse_priv_level(current_sys_path(), values[1]));
            }
            g_state.dirty = true;
        }
    }

    free(values[0]);
    free(values[1]);
    return saved || found;
}

static void edit_matrix_attr_table(const char *title, const char *table_path)
{
    static const char *attrs[] = {
        "private",
        "crash",
        "fileattach",
        "killsent",
        "hold",
        "filerequest",
        "updaterequest",
        "localattach",
    };

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    int selected = 0;
    ListPickResult result;
    do {
        ListItem items[8];
        for (int i = 0; i < 8; i++) {
            bool found = false;
            (void)toml_get_table_int_or_default(table_path, attrs[i], 0, &found);
            items[i].name = strdup(attrs[i]);
            items[i].extra = found ? "" : "(unset)";
            items[i].enabled = true;
            items[i].data = NULL;
        }

        result = listpicker_show(title, items, 8, &selected);

        if (result == LISTPICK_EDIT && selected >= 0 && selected < 8) {
            (void)edit_matrix_attr_table_entry(table_path, attrs[selected]);
        } else if (result == LISTPICK_DELETE && selected >= 0 && selected < 8) {
            char path[256];
            if (snprintf(path, sizeof(path), "%s.%s", table_path, attrs[selected]) < (int)sizeof(path)) {
                (void)maxcfg_toml_override_unset(g_maxcfg_toml, path);
                g_state.dirty = true;
            }
        }

        for (int i = 0; i < 8; i++) {
            free(items[i].name);
        }
    } while (result != LISTPICK_EXIT);
}

static void action_matrix_message_attr_privs(void)
{
    static const char *options[] = { "Ask", "Assume" };
    int pick = dialog_option_picker("Message Attribute Privileges", options, 0);
    if (pick < 0) {
        return;
    }

    if (pick == 0) {
        edit_matrix_attr_table("Message Edit Ask", "matrix.message_edit.ask");
    } else {
        edit_matrix_attr_table("Message Edit Assume", "matrix.message_edit.assume");
    }
}

static void action_system_paths(void)
{
    /* Free old values */
    for (int i = 0; i < 8; i++) free(system_paths_values[i]);

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    const char *sys_path = toml_get_string_or_empty("maximus.sys_path");
    if (sys_path == NULL || sys_path[0] == '\0') {
        sys_path = current_sys_path();
    }

    bool exists[8] = { false };
    system_paths_values[0] = canonicalize_for_display("", toml_get_string_or_empty("maximus.sys_path"), &exists[0]);
    system_paths_values[1] = canonicalize_for_display(sys_path, toml_get_string_or_empty("maximus.misc_path"), &exists[1]);
    system_paths_values[2] = canonicalize_for_display(sys_path, toml_get_string_or_empty("maximus.lang_path"), &exists[2]);
    system_paths_values[3] = canonicalize_for_display(sys_path, toml_get_string_or_empty("maximus.temp_path"), &exists[3]);
    system_paths_values[4] = canonicalize_for_display(sys_path, toml_get_string_or_empty("maximus.ipc_path"), &exists[4]);
    system_paths_values[5] = canonicalize_for_display(sys_path, toml_get_string_or_empty("maximus.file_password"), &exists[5]);
    system_paths_values[6] = canonicalize_for_display(sys_path, toml_get_string_or_empty("maximus.file_access"), &exists[6]);
    system_paths_values[7] = canonicalize_for_display(sys_path, toml_get_string_or_empty("maximus.log_file"), &exists[7]);

    {
        const char *labels[8] = {
            "System Path",
            "Misc Path",
            "Language Path",
            "Temp Path",
            "IPC Path",
            "User File",
            "Access File",
            "Log File",
        };
        const char *paths[8] = {
            system_paths_values[0],
            system_paths_values[1],
            system_paths_values[2],
            system_paths_values[3],
            system_paths_values[4],
            system_paths_values[5],
            system_paths_values[6],
            system_paths_values[7],
        };
        warn_missing_paths("Path Warning", labels, paths, exists, 8);
    }
    
    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("System Paths", system_paths_fields, system_paths_field_count, system_paths_values, dirty_fields, &dirty_count);

    if (saved) {
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.sys_path", system_paths_values[0] ? system_paths_values[0] : "");

        const char *sys_path = system_paths_values[0] ? system_paths_values[0] : current_sys_path();

        char *misc_path = normalize_under_sys_path(sys_path, system_paths_values[1] ? system_paths_values[1] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.misc_path", misc_path ? misc_path : "");
        free(misc_path);

        char *lang_path = normalize_under_sys_path(sys_path, system_paths_values[2] ? system_paths_values[2] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.lang_path", lang_path ? lang_path : "");
        free(lang_path);

        char *temp_path = normalize_under_sys_path(sys_path, system_paths_values[3] ? system_paths_values[3] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.temp_path", temp_path ? temp_path : "");
        free(temp_path);

        char *ipc_path = normalize_under_sys_path(sys_path, system_paths_values[4] ? system_paths_values[4] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.ipc_path", ipc_path ? ipc_path : "");
        free(ipc_path);

        char *file_password = normalize_under_sys_path(sys_path, system_paths_values[5] ? system_paths_values[5] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.file_password", file_password ? file_password : "");
        free(file_password);

        char *file_access = normalize_under_sys_path(sys_path, system_paths_values[6] ? system_paths_values[6] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.file_access", file_access ? file_access : "");
        free(file_access);

        char *log_file = normalize_under_sys_path(sys_path, system_paths_values[7] ? system_paths_values[7] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.log_file", log_file ? log_file : "");
        free(log_file);
        g_state.dirty = true;
    }
} 

static void action_display_files(void)
{
    static const char *display_files_paths[] = {
        "general.display_files.logo",
        "general.display_files.not_found",
        "general.display_files.application",
        "general.display_files.welcome",
        "general.display_files.new_user1",
        "general.display_files.new_user2",
        "general.display_files.rookie",
        "general.display_files.not_configured",
        "general.display_files.quote",
        "general.display_files.day_limit",
        "general.display_files.time_warn",
        "general.display_files.too_slow",
        "general.display_files.bye_bye",
        "general.display_files.bad_logon",
        "general.display_files.barricade",
        "general.display_files.no_space",
        "general.display_files.no_mail",
        "general.display_files.area_not_exist",
        "general.display_files.chat_begin",
        "general.display_files.chat_end",
        "general.display_files.out_leaving",
        "general.display_files.out_return",
        "general.display_files.shell_to_dos",
        "general.display_files.back_from_dos",
        "general.display_files.locate",
        "general.display_files.contents",
        "general.display_files.oped_help",
        "general.display_files.line_ed_help",
        "general.display_files.replace_help",
        "general.display_files.inquire_help",
        "general.display_files.scan_help",
        "general.display_files.list_help",
        "general.display_files.header_help",
        "general.display_files.entry_help",
        "general.display_files.xfer_baud",
        "general.display_files.file_area_list",
        "general.display_files.msg_area_list",
        "general.display_files.protocol_dump",
        "general.display_files.fname_format",
        "general.display_files.tune",
    };

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    if (display_files_field_count != (int)(sizeof(display_files_paths) / sizeof(display_files_paths[0]))) {
        dialog_message("Internal Error", "Display files field mapping mismatch.");
        return;
    }

    char **values = calloc((size_t)display_files_field_count, sizeof(char *));
    if (values == NULL) {
        dialog_message("Out of Memory", "Unable to allocate form values.");
        return;
    }
 
    const char *sys_path = current_sys_path();
    bool *exists = calloc((size_t)display_files_field_count, sizeof(bool));
    if (exists == NULL) {
        dialog_message("Out of Memory", "Unable to allocate validation state.");
        for (int i = 0; i < display_files_field_count; i++) {
            values[i] = strdup(toml_get_string_or_empty(display_files_paths[i]));
        }
    } else {
        for (int i = 0; i < display_files_field_count; i++) {
            const char *raw = toml_get_string_or_empty(display_files_paths[i]);
            values[i] = canonicalize_for_display(sys_path, raw, NULL);
            exists[i] = display_file_variant_exists(sys_path, raw);
        }
 
        {
            const char **labels = calloc((size_t)display_files_field_count, sizeof(char *));
            const char **paths = calloc((size_t)display_files_field_count, sizeof(char *));
            if (labels != NULL && paths != NULL) {
                for (int i = 0; i < display_files_field_count; i++) {
                    labels[i] = display_files_fields[i].label;
                    paths[i] = values[i];
                }
                warn_missing_display_files(labels, paths, exists, display_files_field_count);
            }
            free(labels);
            free(paths);
        }
    }
 
    int dirty_fields[128];
    int dirty_count = 0;
    bool saved = form_edit("Display Files", display_files_fields, display_files_field_count, values, dirty_fields, &dirty_count);
 
    if (saved) {
        const char *sys_path = current_sys_path();
        for (int i = 0; i < display_files_field_count; i++) {
            const char *v = (values[i] != NULL) ? values[i] : "";
            char *norm = normalize_under_sys_path(sys_path, v);
            (void)maxcfg_toml_override_set_string(g_maxcfg_toml, display_files_paths[i], norm ? norm : "");
            free(norm);
        }
        g_state.dirty = true;
    }
 
    for (int i = 0; i < display_files_field_count; i++) {
        free(values[i]);
    }
    free(values);
 
    free(exists);
}

static char *logging_options_values[3] = { NULL };

static void action_logging_options(void)
{
    free(logging_options_values[0]);
    free(logging_options_values[1]);
    free(logging_options_values[2]);

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    const char *sys_path = current_sys_path();
    bool exists[2] = { false };
    logging_options_values[0] = canonicalize_for_display("", toml_get_string_or_empty("maximus.log_file"), &exists[0]);

    {
        const char *lm = toml_get_string_or_empty("maximus.log_mode");
        if (lm[0] == '\0') {
            lm = "Verbose";
        }
        logging_options_values[1] = strdup(lm);
    }

    logging_options_values[2] = canonicalize_for_display(sys_path, toml_get_string_or_empty("maximus.file_callers"), &exists[1]);

    {
        const char *labels[2] = { "Log File", "Callers File" };
        const char *paths[2] = { logging_options_values[0], logging_options_values[2] };
        warn_missing_paths("Logging Warning", labels, paths, exists, 2);
    }
    
    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("Logging Options", logging_options_fields, logging_options_field_count, logging_options_values, dirty_fields, &dirty_count);

    if (saved) {
        const char *sys_path = current_sys_path();
        char *log_file = normalize_under_sys_path(sys_path, logging_options_values[0] ? logging_options_values[0] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.log_file", log_file ? log_file : "");
        free(log_file);
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.log_mode", logging_options_values[1] ? logging_options_values[1] : "");
        char *file_callers = normalize_under_sys_path(sys_path, logging_options_values[2] ? logging_options_values[2] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.file_callers", file_callers ? file_callers : "");
        free(file_callers);
        g_state.dirty = true;
    }
}

static char *global_toggles_values[6] = { NULL };

static void action_global_toggles(void)
{
    free(global_toggles_values[0]);
    free(global_toggles_values[1]);
    free(global_toggles_values[2]);
    free(global_toggles_values[3]);
    free(global_toggles_values[4]);
    free(global_toggles_values[5]);

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }
     
    global_toggles_values[0] = strdup(toml_get_bool_or_default("maximus.snoop", true) ? "Yes" : "No");
    global_toggles_values[1] = strdup(toml_get_bool_or_default("maximus.no_password_encryption", false) ? "No" : "Yes");  /* Inverted */
    global_toggles_values[2] = strdup(toml_get_bool_or_default("maximus.reboot", false) ? "Yes" : "No");
    global_toggles_values[3] = strdup(toml_get_bool_or_default("maximus.swap", false) ? "Yes" : "No");
    global_toggles_values[4] = strdup(toml_get_bool_or_default("maximus.local_input_timeout", false) ? "Yes" : "No");
    global_toggles_values[5] = strdup(toml_get_bool_or_default("maximus.status_line", true) ? "Yes" : "No");
    
    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("Global Toggles", global_toggles_fields, global_toggles_field_count, global_toggles_values, dirty_fields, &dirty_count);
    
    if (saved) {
        bool encrypt = (strcmp(global_toggles_values[1], "Yes") == 0);

        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "maximus.snoop", strcmp(global_toggles_values[0], "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "maximus.no_password_encryption", !encrypt);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "maximus.reboot", strcmp(global_toggles_values[2], "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "maximus.swap", strcmp(global_toggles_values[3], "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "maximus.local_input_timeout", strcmp(global_toggles_values[4], "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "maximus.status_line", strcmp(global_toggles_values[5], "Yes") == 0);
        g_state.dirty = true;
    }
}

/* Login Settings values */
static char *login_settings_values[8] = { NULL };

static void action_login_settings(void)
{
    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    char **values = calloc((size_t)login_settings_field_count, sizeof(char *));
    if (values == NULL) {
        dialog_message("Out of Memory", "Unable to allocate form values.");
        return;
    }

    const char *sys_path = current_sys_path();
    {
        int pv = toml_get_int_or_default("general.session.logon_priv", 0);
        const char *nm = access_level_name_for_level(sys_path, pv);
        values[0] = strdup(nm ? nm : "");

        char buf[32];
        snprintf(buf, sizeof(buf), "%d", toml_get_int_or_default("general.session.logon_timelimit", 0));
        values[1] = strdup(buf);
        snprintf(buf, sizeof(buf), "%d", toml_get_int_or_default("general.session.min_logon_baud", 0));
        values[2] = strdup(buf);
        snprintf(buf, sizeof(buf), "%d", toml_get_int_or_default("general.session.min_graphics_baud", 0));
        values[3] = strdup(buf);
        snprintf(buf, sizeof(buf), "%d", toml_get_int_or_default("general.session.min_rip_baud", 0));
        values[4] = strdup(buf);
        snprintf(buf, sizeof(buf), "%d", toml_get_int_or_default("general.session.input_timeout", 0));
        values[5] = strdup(buf);
    }
    values[6] = strdup(toml_get_bool_or_default("general.session.check_ansi", false) ? "Yes" : "No");
    values[7] = strdup(toml_get_bool_or_default("general.session.check_rip", false) ? "Yes" : "No");

    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("Login Settings", login_settings_fields, login_settings_field_count, values, dirty_fields, &dirty_count);

    if (saved) {
        if (values[0] == NULL || values[0][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "general.session.logon_priv");
        else (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "general.session.logon_priv", parse_priv_level(sys_path, values[0]));
        if (values[1] == NULL || values[1][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "general.session.logon_timelimit");
        else (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "general.session.logon_timelimit", atoi(values[1]));
        if (values[2] == NULL || values[2][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "general.session.min_logon_baud");
        else (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "general.session.min_logon_baud", atoi(values[2]));
        if (values[3] == NULL || values[3][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "general.session.min_graphics_baud");
        else (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "general.session.min_graphics_baud", atoi(values[3]));
        if (values[4] == NULL || values[4][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "general.session.min_rip_baud");
        else (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "general.session.min_rip_baud", atoi(values[4]));
        if (values[5] == NULL || values[5][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "general.session.input_timeout");
        else (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "general.session.input_timeout", atoi(values[5]));
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "general.session.check_ansi", strcmp(values[6] ? values[6] : "No", "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "general.session.check_rip", strcmp(values[7] ? values[7] : "No", "Yes") == 0);
        g_state.dirty = true;
    }

    for (int i = 0; i < login_settings_field_count; i++) {
        free(values[i]);
    }
    free(values);
}

/* New User Defaults values */
static char *new_user_defaults_values[8] = { NULL };

static void action_new_user_defaults(void)
{
    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    char **values = calloc((size_t)new_user_defaults_field_count, sizeof(char *));
    if (values == NULL) {
        dialog_message("Out of Memory", "Unable to allocate form values.");
        return;
    }

    values[0] = strdup(toml_get_bool_or_default("general.session.ask_phone", false) ? "Yes" : "No");
    values[1] = strdup(toml_get_bool_or_default("general.session.ask_alias", false) ? "Yes" : "No");
    values[2] = strdup(toml_get_bool_or_default("general.session.alias_system", false) ? "Yes" : "No");
    values[3] = strdup(toml_get_bool_or_default("general.session.single_word_names", false) ? "Yes" : "No");
    values[4] = strdup(toml_get_bool_or_default("general.session.no_real_name", false) ? "Yes" : "No");
    values[5] = strdup(toml_get_string_or_empty("general.session.first_menu"));
    values[6] = strdup(toml_get_string_or_empty("general.session.first_file_area"));
    values[7] = strdup(toml_get_string_or_empty("general.session.first_message_area"));

    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("New User Defaults", new_user_defaults_fields, new_user_defaults_field_count, values, dirty_fields, &dirty_count);

    if (saved) {
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "general.session.ask_phone", strcmp(values[0] ? values[0] : "No", "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "general.session.ask_alias", strcmp(values[1] ? values[1] : "No", "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "general.session.alias_system", strcmp(values[2] ? values[2] : "No", "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "general.session.single_word_names", strcmp(values[3] ? values[3] : "No", "Yes") == 0);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, "general.session.no_real_name", strcmp(values[4] ? values[4] : "No", "Yes") == 0);
        if (values[5] == NULL || values[5][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "general.session.first_menu");
        else (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "general.session.first_menu", values[5]);
        if (values[6] == NULL || values[6][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "general.session.first_file_area");
        else (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "general.session.first_file_area", values[6]);
        if (values[7] == NULL || values[7][0] == '\0') (void)maxcfg_toml_override_unset(g_maxcfg_toml, "general.session.first_message_area");
        else (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "general.session.first_message_area", values[7]);
        g_state.dirty = true;
    }

    for (int i = 0; i < new_user_defaults_field_count; i++) {
        free(values[i]);
    }
    free(values);
}

static char *matrix_netmail_values[5] = { NULL };

static void action_matrix_netmail_settings(void)
{
    for (int i = 0; i < 5; i++) free(matrix_netmail_values[i]);

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    const char *sys_path = current_sys_path();
    matrix_netmail_values[0] = strdup(toml_get_string_or_empty("matrix.nodelist_version"));
    if (matrix_netmail_values[0][0] == '\0') {
        free(matrix_netmail_values[0]);
        matrix_netmail_values[0] = strdup("7");
    }

    bool exists[2] = { false };
    matrix_netmail_values[1] = canonicalize_for_display(sys_path, toml_get_string_or_empty("matrix.echotoss_name"), &exists[0]);
    matrix_netmail_values[2] = canonicalize_for_display(sys_path, toml_get_string_or_empty("matrix.fidouser"), &exists[1]);
    {
        int ctla = toml_get_int_or_default("matrix.ctla_priv", 0);
        const char *ctla_name = access_level_name_for_level(sys_path, ctla);
        matrix_netmail_values[3] = strdup(ctla_name ? ctla_name : "");
    }
    {
        int seenby = toml_get_int_or_default("matrix.seenby_priv", 0);
        const char *seenby_name = access_level_name_for_level(sys_path, seenby);
        matrix_netmail_values[4] = strdup(seenby_name ? seenby_name : "");
    }

    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("Netmail Settings", matrix_netmail_fields, matrix_netmail_field_count, matrix_netmail_values, dirty_fields, &dirty_count);

    if (saved) {
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "matrix.nodelist_version", matrix_netmail_values[0] ? matrix_netmail_values[0] : "7");
        
        char *echotoss = normalize_under_sys_path(sys_path, matrix_netmail_values[1] ? matrix_netmail_values[1] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "matrix.echotoss_name", echotoss ? echotoss : "");
        free(echotoss);
        
        char *fidouser = normalize_under_sys_path(sys_path, matrix_netmail_values[2] ? matrix_netmail_values[2] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "matrix.fidouser", fidouser ? fidouser : "");
        free(fidouser);
        
        int ctla = parse_priv_level(sys_path, matrix_netmail_values[3] ? matrix_netmail_values[3] : "");
        int seenby = parse_priv_level(sys_path, matrix_netmail_values[4] ? matrix_netmail_values[4] : "");
        (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "matrix.ctla_priv", ctla);
        (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "matrix.seenby_priv", seenby);
        g_state.dirty = true;
    }
}

static void action_network_addresses(void)
{
    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    ListItem items[16];
    int item_count = 0;
    
    for (int i = 0; i < 16; i++) {
        char path[128];
        snprintf(path, sizeof(path), "matrix.address[%d].zone", i);
        int zone = toml_get_int_or_default(path, -1);
        if (zone < 0) break;
        
        snprintf(path, sizeof(path), "matrix.address[%d].net", i);
        int net = toml_get_int_or_default(path, 0);
        snprintf(path, sizeof(path), "matrix.address[%d].node", i);
        int node = toml_get_int_or_default(path, 0);
        snprintf(path, sizeof(path), "matrix.address[%d].point", i);
        int point = toml_get_int_or_default(path, 0);
        
        char label[64];
        if (point > 0) {
            snprintf(label, sizeof(label), "%d:%d/%d.%d", zone, net, node, point);
        } else {
            snprintf(label, sizeof(label), "%d:%d/%d", zone, net, node);
        }
        
        items[item_count].name = strdup(label);
        items[item_count].extra = (i == 0) ? "(primary)" : "";
        items[item_count].enabled = true;
        items[item_count].data = NULL;
        item_count++;
    }

    int selected = 0;
    ListPickResult result;
    
    do {
        result = listpicker_show("Network Addresses (first=primary, max 16)", items, item_count, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < item_count) {
            char path[128];
            char *values[5];
            
            snprintf(path, sizeof(path), "matrix.address[%d].zone", selected);
            int zone = toml_get_int_or_default(path, 1);
            snprintf(path, sizeof(path), "matrix.address[%d].net", selected);
            int net = toml_get_int_or_default(path, 1);
            snprintf(path, sizeof(path), "matrix.address[%d].node", selected);
            int node = toml_get_int_or_default(path, 1);
            snprintf(path, sizeof(path), "matrix.address[%d].point", selected);
            int point = toml_get_int_or_default(path, 0);
            snprintf(path, sizeof(path), "matrix.address[%d].domain", selected);
            const char *domain = toml_get_string_or_empty(path);
            
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", zone);
            values[0] = strdup(buf);
            snprintf(buf, sizeof(buf), "%d", net);
            values[1] = strdup(buf);
            snprintf(buf, sizeof(buf), "%d", node);
            values[2] = strdup(buf);
            snprintf(buf, sizeof(buf), "%d", point);
            values[3] = strdup(buf);
            values[4] = strdup(domain);
            
            int dirty[5], dirty_count = 0;
            if (form_edit("Edit Address", matrix_address_fields, matrix_address_field_count, values, dirty, &dirty_count)) {
                int new_zone = atoi(values[0]);
                int new_net = atoi(values[1]);
                int new_node = atoi(values[2]);
                int new_point = atoi(values[3]);
                
                snprintf(path, sizeof(path), "matrix.address[%d].zone", selected);
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, new_zone);
                snprintf(path, sizeof(path), "matrix.address[%d].net", selected);
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, new_net);
                snprintf(path, sizeof(path), "matrix.address[%d].node", selected);
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, new_node);
                snprintf(path, sizeof(path), "matrix.address[%d].point", selected);
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, new_point);
                snprintf(path, sizeof(path), "matrix.address[%d].domain", selected);
                (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, values[4]);
                
                free((char*)items[selected].name);
                char label[64];
                if (new_point > 0) {
                    snprintf(label, sizeof(label), "%d:%d/%d.%d", new_zone, new_net, new_node, new_point);
                } else {
                    snprintf(label, sizeof(label), "%d:%d/%d", new_zone, new_net, new_node);
                }
                items[selected].name = strdup(label);
                
                g_state.dirty = true;
            }
            
            for (int i = 0; i < 5; i++) free(values[i]);
        }
        else if (result == LISTPICK_ADD && item_count < 16) {
            char *values[5] = { strdup("1"), strdup("1"), strdup("1"), strdup("0"), strdup("") };
            
            int dirty[5], dirty_count = 0;
            if (form_edit("Add Address", matrix_address_fields, matrix_address_field_count, values, dirty, &dirty_count)) {
                int new_zone = atoi(values[0]);
                int new_net = atoi(values[1]);
                int new_node = atoi(values[2]);
                int new_point = atoi(values[3]);
                
                char path[128];
                snprintf(path, sizeof(path), "matrix.address[%d].zone", item_count);
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, new_zone);
                snprintf(path, sizeof(path), "matrix.address[%d].net", item_count);
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, new_net);
                snprintf(path, sizeof(path), "matrix.address[%d].node", item_count);
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, new_node);
                snprintf(path, sizeof(path), "matrix.address[%d].point", item_count);
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, new_point);
                snprintf(path, sizeof(path), "matrix.address[%d].domain", item_count);
                (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, values[4]);
                
                char label[64];
                if (new_point > 0) {
                    snprintf(label, sizeof(label), "%d:%d/%d.%d", new_zone, new_net, new_node, new_point);
                } else {
                    snprintf(label, sizeof(label), "%d:%d/%d", new_zone, new_net, new_node);
                }
                items[item_count].name = strdup(label);
                items[item_count].extra = (item_count == 0) ? "(primary)" : "";
                items[item_count].enabled = true;
                items[item_count].data = NULL;
                item_count++;
                
                g_state.dirty = true;
            }
            
            for (int i = 0; i < 5; i++) free(values[i]);
        }
        else if (result == LISTPICK_DELETE && selected >= 0 && selected < item_count) {
            free((char*)items[selected].name);
            for (int i = selected; i < item_count - 1; i++) {
                items[i] = items[i + 1];
            }
            item_count--;
            
            for (int i = 0; i < item_count; i++) {
                char path[128];
                char src_path[128];
                
                snprintf(src_path, sizeof(src_path), "matrix.address[%d].zone", i + 1);
                int zone = toml_get_int_or_default(src_path, 1);
                snprintf(path, sizeof(path), "matrix.address[%d].zone", i);
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, zone);
                
                snprintf(src_path, sizeof(src_path), "matrix.address[%d].net", i + 1);
                int net = toml_get_int_or_default(src_path, 1);
                snprintf(path, sizeof(path), "matrix.address[%d].net", i);
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, net);
                
                snprintf(src_path, sizeof(src_path), "matrix.address[%d].node", i + 1);
                int node = toml_get_int_or_default(src_path, 1);
                snprintf(path, sizeof(path), "matrix.address[%d].node", i);
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, node);
                
                snprintf(src_path, sizeof(src_path), "matrix.address[%d].point", i + 1);
                int point = toml_get_int_or_default(src_path, 0);
                snprintf(path, sizeof(path), "matrix.address[%d].point", i);
                (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, point);
                
                snprintf(src_path, sizeof(src_path), "matrix.address[%d].domain", i + 1);
                const char *domain = toml_get_string_or_empty(src_path);
                snprintf(path, sizeof(path), "matrix.address[%d].domain", i);
                (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, domain);
            }
            
            if (item_count < 16) {
                char path[128];
                snprintf(path, sizeof(path), "matrix.address[%d].zone", item_count);
                (void)maxcfg_toml_override_unset(g_maxcfg_toml, path);
                snprintf(path, sizeof(path), "matrix.address[%d].net", item_count);
                (void)maxcfg_toml_override_unset(g_maxcfg_toml, path);
                snprintf(path, sizeof(path), "matrix.address[%d].node", item_count);
                (void)maxcfg_toml_override_unset(g_maxcfg_toml, path);
                snprintf(path, sizeof(path), "matrix.address[%d].point", item_count);
                (void)maxcfg_toml_override_unset(g_maxcfg_toml, path);
                snprintf(path, sizeof(path), "matrix.address[%d].domain", item_count);
                (void)maxcfg_toml_override_unset(g_maxcfg_toml, path);
            }
            
            if (selected >= item_count && item_count > 0) selected = item_count - 1;
            g_state.dirty = true;
        }
        
    } while (result != LISTPICK_EXIT);
    
    for (int i = 0; i < item_count; i++) {
        free((char*)items[i].name);
    }
}

static void action_edit_lang_file_list(void *unused);

static char *language_settings_values[4] = { NULL };

static const FieldDef language_settings_with_list[] = {
    {
        .keyword = "default_language",
        .label = "Default Language",
        .help = "Name of the default language file (without .LTF extension) used for new users and when no language is specified.",
        .type = FIELD_TEXT,
        .max_length = 20,
        .default_value = "english"
    },
    {
        .keyword = "lang_path",
        .label = "Language Path",
        .help = "Directory containing language files (.LTF, .MAD, .LTH). Must contain at minimum an .LTF file for each declared language.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "etc/lang"
    },
    {
        .keyword = NULL,
        .label = "Edit Language Files...",
        .help = "Edit the list of available language files. First entry is the default, up to 8 languages supported.",
        .type = FIELD_ACTION,
        .max_length = 0,
        .default_value = "[Press Enter to edit]",
        .action = action_edit_lang_file_list
    },
};

static void action_languages(void)
{
    for (int i = 0; i < 4; i++) free(language_settings_values[i]);

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    const char *sys_path = current_sys_path();
    language_settings_values[0] = strdup(toml_get_string_or_empty("general.language.default_language"));
    if (language_settings_values[0][0] == '\0') {
        free(language_settings_values[0]);
        language_settings_values[0] = strdup("english");
    }
    language_settings_values[1] = canonicalize_for_display(sys_path, toml_get_string_or_empty("maximus.lang_path"), NULL);
    language_settings_values[2] = strdup("[Press Enter to edit]");
    language_settings_values[3] = strdup("[Press Enter to browse]");

    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("Language Settings", language_settings_with_list, 4, language_settings_values, dirty_fields, &dirty_count);

    if (saved) {
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "general.language.default_language", language_settings_values[0] ? language_settings_values[0] : "english");
        
        char *lang_path = normalize_under_sys_path(sys_path, language_settings_values[1] ? language_settings_values[1] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "maximus.lang_path", lang_path ? lang_path : "");
        free(lang_path);
        
        g_state.dirty = true;
    }
}

static void action_edit_lang_file_list(void *unused)
{
    (void)unused;
    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    ListItem items[8];
    int item_count = 0;
    
    for (int i = 0; i < 8; i++) {
        char path[128];
        snprintf(path, sizeof(path), "general.language.lang_file[%d]", i);
        const char *lang = toml_get_string_or_empty(path);
        if (lang && lang[0] != '\0') {
            items[item_count].name = strdup(lang);
            items[item_count].extra = (i == 0) ? "(default)" : "";
            items[item_count].enabled = true;
            items[item_count].data = NULL;
            item_count++;
        }
    }

    int selected = 0;
    ListPickResult result;
    
    do {
        result = listpicker_show("Language Files (first=default, max 8)", items, item_count, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < item_count) {
            char *values[1] = { strdup(items[selected].name) };
            const FieldDef lang_field[] = {
                {
                    .keyword = "language",
                    .label = "Language Root",
                    .help = "Language file root name (without .LTF extension)",
                    .type = FIELD_TEXT,
                    .max_length = 20,
                    .default_value = ""
                }
            };
            
            int dirty[1], dirty_count = 0;
            if (form_edit("Edit Language", lang_field, 1, values, dirty, &dirty_count)) {
                free((char*)items[selected].name);
                items[selected].name = strdup(values[0]);
                
                char path[128];
                snprintf(path, sizeof(path), "general.language.lang_file[%d]", selected);
                (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, values[0]);
                g_state.dirty = true;
            }
            free(values[0]);
        }
        else if (result == LISTPICK_ADD && item_count < 8) {
            char *values[1] = { strdup("") };
            const FieldDef lang_field[] = {
                {
                    .keyword = "language",
                    .label = "Language Root",
                    .help = "Language file root name (without .LTF extension)",
                    .type = FIELD_TEXT,
                    .max_length = 20,
                    .default_value = ""
                }
            };
            
            int dirty[1], dirty_count = 0;
            if (form_edit("Add Language", lang_field, 1, values, dirty, &dirty_count)) {
                items[item_count].name = strdup(values[0]);
                items[item_count].extra = (item_count == 0) ? "(default)" : "";
                items[item_count].enabled = true;
                items[item_count].data = NULL;
                
                char path[128];
                snprintf(path, sizeof(path), "general.language.lang_file[%d]", item_count);
                (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, values[0]);
                item_count++;
                g_state.dirty = true;
            }
            free(values[0]);
        }
        else if (result == LISTPICK_DELETE && selected >= 0 && selected < item_count) {
            free((char*)items[selected].name);
            for (int i = selected; i < item_count - 1; i++) {
                items[i] = items[i + 1];
            }
            item_count--;
            
            for (int i = 0; i < item_count; i++) {
                char path[128];
                snprintf(path, sizeof(path), "general.language.lang_file[%d]", i);
                (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, items[i].name);
            }
            
            if (item_count < 8) {
                char path[128];
                snprintf(path, sizeof(path), "general.language.lang_file[%d]", item_count);
                (void)maxcfg_toml_override_unset(g_maxcfg_toml, path);
            }
            
            if (selected >= item_count && item_count > 0) selected = item_count - 1;
            g_state.dirty = true;
        }
        
    } while (result != LISTPICK_EXIT);
    
    for (int i = 0; i < item_count; i++) {
        free((char*)items[i].name);
    }
}

static char *protocol_settings_values2[2] = { NULL };

static const FieldDef protocol_settings_with_list[] = {
    {
        .keyword = "protoexit",
        .label = "Protocol Exit Level",
        .help = "Error level returned to batch files after external protocol transfer. Used for post-transfer processing and error handling.",
        .type = FIELD_TEXT,
        .max_length = 10,
        .default_value = "0"
    },
    {
        .keyword = NULL,
        .label = "Edit Protocol Definitions...",
        .help = "Edit external transfer protocol definitions (insert/edit/delete).",
        .type = FIELD_ACTION,
        .max_length = 0,
        .default_value = "[Press Enter to edit]",
        .action = action_protocol_list,
        .action_ctx = NULL
    },
};

static void action_protocols(void)
{
    free(protocol_settings_values2[0]);
    free(protocol_settings_values2[1]);

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    protocol_settings_values2[0] = strdup(toml_get_string_or_empty("general.protocol.protoexit"));
    if (protocol_settings_values2[0][0] == '\0') {
        free(protocol_settings_values2[0]);
        protocol_settings_values2[0] = strdup("0");
    }
    protocol_settings_values2[1] = strdup("[Press Enter to edit]");

    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("Protocol Settings", protocol_settings_with_list, 2, protocol_settings_values2, dirty_fields, &dirty_count);

    if (saved) {
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "general.protocol.protoexit", protocol_settings_values2[0] ? protocol_settings_values2[0] : "0");
        g_state.dirty = true;
    }
}

typedef struct {
    char *name;
    char *program;
    bool batch;
    bool exitlevel;
    bool bi;
    bool opus;
    char *log_file;
    char *control_file;
    char *download_cmd;
    char *upload_cmd;
    char *download_string;
    char *upload_string;
    char *download_keyword;
    char *upload_keyword;
    int filename_word;
    int descript_word;
} ProtoEntry;

static void proto_entry_free(ProtoEntry *p)
{
    if (!p) return;
    free(p->name);
    free(p->program);
    free(p->log_file);
    free(p->control_file);
    free(p->download_cmd);
    free(p->upload_cmd);
    free(p->download_string);
    free(p->upload_string);
    free(p->download_keyword);
    free(p->upload_keyword);
    memset(p, 0, sizeof(*p));
}

static bool proto_entry_load(int idx, ProtoEntry *out)
{
    if (!out || idx < 0) return false;
    char path[256];

    snprintf(path, sizeof(path), "general.protocol.protocol[%d].name", idx);
    const char *name = toml_get_string_or_empty(path);
    if (name == NULL || name[0] == '\0') {
        return false;
    }
    out->name = strdup(name);

    snprintf(path, sizeof(path), "general.protocol.protocol[%d].program", idx);
    out->program = strdup(toml_get_string_or_empty(path));

    snprintf(path, sizeof(path), "general.protocol.protocol[%d].batch", idx);
    out->batch = toml_get_bool_or_default(path, false);
    snprintf(path, sizeof(path), "general.protocol.protocol[%d].exitlevel", idx);
    out->exitlevel = toml_get_bool_or_default(path, false);
    snprintf(path, sizeof(path), "general.protocol.protocol[%d].bi", idx);
    out->bi = toml_get_bool_or_default(path, false);
    snprintf(path, sizeof(path), "general.protocol.protocol[%d].opus", idx);
    out->opus = toml_get_bool_or_default(path, false);

    snprintf(path, sizeof(path), "general.protocol.protocol[%d].log_file", idx);
    out->log_file = strdup(toml_get_string_or_empty(path));
    snprintf(path, sizeof(path), "general.protocol.protocol[%d].control_file", idx);
    out->control_file = strdup(toml_get_string_or_empty(path));
    snprintf(path, sizeof(path), "general.protocol.protocol[%d].download_cmd", idx);
    out->download_cmd = strdup(toml_get_string_or_empty(path));
    snprintf(path, sizeof(path), "general.protocol.protocol[%d].upload_cmd", idx);
    out->upload_cmd = strdup(toml_get_string_or_empty(path));

    snprintf(path, sizeof(path), "general.protocol.protocol[%d].download_string", idx);
    out->download_string = strdup(toml_get_string_or_empty(path));
    snprintf(path, sizeof(path), "general.protocol.protocol[%d].upload_string", idx);
    out->upload_string = strdup(toml_get_string_or_empty(path));

    snprintf(path, sizeof(path), "general.protocol.protocol[%d].download_keyword", idx);
    out->download_keyword = strdup(toml_get_string_or_empty(path));
    snprintf(path, sizeof(path), "general.protocol.protocol[%d].upload_keyword", idx);
    out->upload_keyword = strdup(toml_get_string_or_empty(path));

    snprintf(path, sizeof(path), "general.protocol.protocol[%d].filename_word", idx);
    out->filename_word = toml_get_int_or_default(path, 0);
    snprintf(path, sizeof(path), "general.protocol.protocol[%d].descript_word", idx);
    out->descript_word = toml_get_int_or_default(path, 0);
    return true;
}

static void proto_entry_write_all(ProtoEntry *arr, int count)
{
    (void)maxcfg_toml_override_set_table_array_empty(g_maxcfg_toml, "general.protocol.protocol");

    for (int i = 0; i < count; i++) {
        char path[256];

        snprintf(path, sizeof(path), "general.protocol.protocol[%d].index", i);
        (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, i);

        snprintf(path, sizeof(path), "general.protocol.protocol[%d].name", i);
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, arr[i].name ? arr[i].name : "");

        snprintf(path, sizeof(path), "general.protocol.protocol[%d].program", i);
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, arr[i].program ? arr[i].program : "");

        snprintf(path, sizeof(path), "general.protocol.protocol[%d].batch", i);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, path, arr[i].batch);
        snprintf(path, sizeof(path), "general.protocol.protocol[%d].exitlevel", i);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, path, arr[i].exitlevel);
        snprintf(path, sizeof(path), "general.protocol.protocol[%d].bi", i);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, path, arr[i].bi);
        snprintf(path, sizeof(path), "general.protocol.protocol[%d].opus", i);
        (void)maxcfg_toml_override_set_bool(g_maxcfg_toml, path, arr[i].opus);

        snprintf(path, sizeof(path), "general.protocol.protocol[%d].log_file", i);
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, arr[i].log_file ? arr[i].log_file : "");
        snprintf(path, sizeof(path), "general.protocol.protocol[%d].control_file", i);
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, arr[i].control_file ? arr[i].control_file : "");
        snprintf(path, sizeof(path), "general.protocol.protocol[%d].download_cmd", i);
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, arr[i].download_cmd ? arr[i].download_cmd : "");
        snprintf(path, sizeof(path), "general.protocol.protocol[%d].upload_cmd", i);
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, arr[i].upload_cmd ? arr[i].upload_cmd : "");

        snprintf(path, sizeof(path), "general.protocol.protocol[%d].download_string", i);
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, arr[i].download_string ? arr[i].download_string : "");
        snprintf(path, sizeof(path), "general.protocol.protocol[%d].upload_string", i);
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, arr[i].upload_string ? arr[i].upload_string : "");

        snprintf(path, sizeof(path), "general.protocol.protocol[%d].download_keyword", i);
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, arr[i].download_keyword ? arr[i].download_keyword : "");
        snprintf(path, sizeof(path), "general.protocol.protocol[%d].upload_keyword", i);
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, path, arr[i].upload_keyword ? arr[i].upload_keyword : "");

        snprintf(path, sizeof(path), "general.protocol.protocol[%d].filename_word", i);
        (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, arr[i].filename_word);
        snprintf(path, sizeof(path), "general.protocol.protocol[%d].descript_word", i);
        (void)maxcfg_toml_override_set_int(g_maxcfg_toml, path, arr[i].descript_word);
    }
}

static void proto_rebuild_items(ListItem *items, int max_items, ProtoEntry *protos, int proto_count)
{
    if (!items || !protos || max_items <= 0) return;

    for (int i = 0; i < max_items; i++) {
        free(items[i].name);
        free(items[i].extra);
        items[i].name = NULL;
        items[i].extra = NULL;
        items[i].enabled = true;
        items[i].data = NULL;
    }
    for (int i = 0; i < proto_count && i < max_items; i++) {
        char label[96];
        snprintf(label, sizeof(label), "%d: %s", i, protos[i].name ? protos[i].name : "");
        items[i].name = strdup(label);
        items[i].extra = strdup((protos[i].program && protos[i].program[0]) ? protos[i].program : "");
        items[i].enabled = true;
        items[i].data = (void *)(intptr_t)i;
    }
}

static void action_protocol_list(void *unused)
{
    (void)unused;
    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    const int MAX_PROTOCOLS = 64;
    ProtoEntry protos[64];
    memset(protos, 0, sizeof(protos));

    int proto_count = 0;
    for (int i = 0; i < MAX_PROTOCOLS; i++) {
        if (!proto_entry_load(i, &protos[proto_count])) {
            break;
        }
        proto_count++;
    }

    ListItem items[64];
    memset(items, 0, sizeof(items));
    proto_rebuild_items(items, MAX_PROTOCOLS, protos, proto_count);

    int selected = 0;
    ListPickResult result;

    do {
        result = listpicker_show("Protocols", items, proto_count, &selected);

        if (result == LISTPICK_EDIT && selected >= 0 && selected < proto_count) {
            int idx = (int)(intptr_t)items[selected].data;
            if (idx < 0 || idx >= proto_count) continue;

            char **values = calloc((size_t)protocol_entry_field_count, sizeof(char *));
            if (!values) {
                dialog_message("Out of Memory", "Unable to allocate protocol form values.");
                continue;
            }

            char buf[64];
            snprintf(buf, sizeof(buf), "%d", idx);
            values[0] = strdup(buf);
            values[1] = strdup(protos[idx].name ? protos[idx].name : "");
            values[2] = strdup(protos[idx].program ? protos[idx].program : "");
            values[3] = strdup(protos[idx].batch ? "Yes" : "No");
            values[4] = strdup(protos[idx].exitlevel ? "Yes" : "No");
            values[5] = strdup(protos[idx].bi ? "Yes" : "No");
            values[6] = strdup(protos[idx].opus ? "Yes" : "No");
            values[7] = strdup(protos[idx].log_file ? protos[idx].log_file : "");
            values[8] = strdup(protos[idx].control_file ? protos[idx].control_file : "");
            values[9] = strdup(protos[idx].download_cmd ? protos[idx].download_cmd : "");
            values[10] = strdup(protos[idx].upload_cmd ? protos[idx].upload_cmd : "");
            values[11] = strdup(protos[idx].download_string ? protos[idx].download_string : "");
            values[12] = strdup(protos[idx].upload_string ? protos[idx].upload_string : "");
            values[13] = strdup(protos[idx].download_keyword ? protos[idx].download_keyword : "");
            values[14] = strdup(protos[idx].upload_keyword ? protos[idx].upload_keyword : "");
            snprintf(buf, sizeof(buf), "%d", protos[idx].filename_word);
            values[15] = strdup(buf);
            snprintf(buf, sizeof(buf), "%d", protos[idx].descript_word);
            values[16] = strdup(buf);

            if (form_edit("Edit Protocol", protocol_entry_fields, protocol_entry_field_count, values, NULL, NULL)) {
                free(protos[idx].name);
                free(protos[idx].program);
                free(protos[idx].log_file);
                free(protos[idx].control_file);
                free(protos[idx].download_cmd);
                free(protos[idx].upload_cmd);
                free(protos[idx].download_string);
                free(protos[idx].upload_string);
                free(protos[idx].download_keyword);
                free(protos[idx].upload_keyword);

                protos[idx].name = strdup(values[1] ? values[1] : "");
                protos[idx].program = strdup(values[2] ? values[2] : "");
                protos[idx].batch = (values[3] && strcmp(values[3], "Yes") == 0);
                protos[idx].exitlevel = (values[4] && strcmp(values[4], "Yes") == 0);
                protos[idx].bi = (values[5] && strcmp(values[5], "Yes") == 0);
                protos[idx].opus = (values[6] && strcmp(values[6], "Yes") == 0);
                protos[idx].log_file = strdup(values[7] ? values[7] : "");
                protos[idx].control_file = strdup(values[8] ? values[8] : "");
                protos[idx].download_cmd = strdup(values[9] ? values[9] : "");
                protos[idx].upload_cmd = strdup(values[10] ? values[10] : "");
                protos[idx].download_string = strdup(values[11] ? values[11] : "");
                protos[idx].upload_string = strdup(values[12] ? values[12] : "");
                protos[idx].download_keyword = strdup(values[13] ? values[13] : "");
                protos[idx].upload_keyword = strdup(values[14] ? values[14] : "");
                protos[idx].filename_word = values[15] ? atoi(values[15]) : 0;
                protos[idx].descript_word = values[16] ? atoi(values[16]) : 0;

                proto_entry_write_all(protos, proto_count);
                g_state.dirty = true;
                proto_rebuild_items(items, MAX_PROTOCOLS, protos, proto_count);
            }

            for (int i = 0; i < protocol_entry_field_count; i++) {
                free(values[i]);
            }
            free(values);
        } else if ((result == LISTPICK_INSERT || result == LISTPICK_ADD) && proto_count < MAX_PROTOCOLS) {
            int insert_at = (result == LISTPICK_INSERT && selected >= 0 && selected <= proto_count) ? selected : proto_count;

            char **values = calloc((size_t)protocol_entry_field_count, sizeof(char *));
            if (!values) {
                dialog_message("Out of Memory", "Unable to allocate protocol form values.");
                continue;
            }

            char buf[64];
            snprintf(buf, sizeof(buf), "%d", insert_at);
            values[0] = strdup(buf);
            values[1] = strdup("");
            values[2] = strdup("");
            values[3] = strdup("No");
            values[4] = strdup("No");
            values[5] = strdup("No");
            values[6] = strdup("No");
            values[7] = strdup("");
            values[8] = strdup("");
            values[9] = strdup("");
            values[10] = strdup("");
            values[11] = strdup("");
            values[12] = strdup("");
            values[13] = strdup("");
            values[14] = strdup("");
            values[15] = strdup("0");
            values[16] = strdup("0");

            if (form_edit("New Protocol", protocol_entry_fields, protocol_entry_field_count, values, NULL, NULL)) {
                if (values[1] && values[1][0]) {
                    for (int i = proto_count; i > insert_at; i--) {
                        protos[i] = protos[i - 1];
                        memset(&protos[i - 1], 0, sizeof(protos[i - 1]));
                    }

                    ProtoEntry *p = &protos[insert_at];
                    p->name = strdup(values[1]);
                    p->program = strdup(values[2] ? values[2] : "");
                    p->batch = (values[3] && strcmp(values[3], "Yes") == 0);
                    p->exitlevel = (values[4] && strcmp(values[4], "Yes") == 0);
                    p->bi = (values[5] && strcmp(values[5], "Yes") == 0);
                    p->opus = (values[6] && strcmp(values[6], "Yes") == 0);
                    p->log_file = strdup(values[7] ? values[7] : "");
                    p->control_file = strdup(values[8] ? values[8] : "");
                    p->download_cmd = strdup(values[9] ? values[9] : "");
                    p->upload_cmd = strdup(values[10] ? values[10] : "");
                    p->download_string = strdup(values[11] ? values[11] : "");
                    p->upload_string = strdup(values[12] ? values[12] : "");
                    p->download_keyword = strdup(values[13] ? values[13] : "");
                    p->upload_keyword = strdup(values[14] ? values[14] : "");
                    p->filename_word = values[15] ? atoi(values[15]) : 0;
                    p->descript_word = values[16] ? atoi(values[16]) : 0;

                    proto_count++;
                    proto_entry_write_all(protos, proto_count);
                    g_state.dirty = true;
                    proto_rebuild_items(items, MAX_PROTOCOLS, protos, proto_count);
                    selected = insert_at;
                }
            }

            for (int i = 0; i < protocol_entry_field_count; i++) {
                free(values[i]);
            }
            free(values);
        } else if (result == LISTPICK_DELETE && selected >= 0 && selected < proto_count) {
            int del = selected;
            proto_entry_free(&protos[del]);
            for (int i = del; i < proto_count - 1; i++) {
                protos[i] = protos[i + 1];
                memset(&protos[i + 1], 0, sizeof(protos[i + 1]));
            }
            proto_count--;
            proto_entry_write_all(protos, proto_count);
            g_state.dirty = true;
            proto_rebuild_items(items, MAX_PROTOCOLS, protos, proto_count);
            if (selected >= proto_count && proto_count > 0) selected = proto_count - 1;
        }

    } while (result != LISTPICK_EXIT);

    for (int i = 0; i < proto_count; i++) {
        proto_entry_free(&protos[i]);
    }
    for (int i = 0; i < MAX_PROTOCOLS; i++) {
        free(items[i].name);
        free(items[i].extra);
    }
}

static char *matrix_events_values[3] = { NULL };

static void action_events(void)
{
    for (int i = 0; i < 3; i++) free(matrix_events_values[i]);

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", toml_get_int_or_default("matrix.after_edit_exit", 0));
    matrix_events_values[0] = strdup(buf);

    snprintf(buf, sizeof(buf), "%d", toml_get_int_or_default("matrix.after_echomail_exit", 0));
    matrix_events_values[1] = strdup(buf);

    snprintf(buf, sizeof(buf), "%d", toml_get_int_or_default("matrix.after_local_exit", 0));
    matrix_events_values[2] = strdup(buf);

    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("Events", matrix_events_fields, matrix_events_field_count, matrix_events_values, dirty_fields, &dirty_count);

    if (saved) {
        int val0 = matrix_events_values[0] ? atoi(matrix_events_values[0]) : 0;
        int val1 = matrix_events_values[1] ? atoi(matrix_events_values[1]) : 0;
        int val2 = matrix_events_values[2] ? atoi(matrix_events_values[2]) : 0;
        
        (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "matrix.after_edit_exit", val0);
        (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "matrix.after_echomail_exit", val1);
        (void)maxcfg_toml_override_set_int(g_maxcfg_toml, "matrix.after_local_exit", val2);
        g_state.dirty = true;
    }
}

static char *reader_settings_values[6] = { NULL };

static const FieldDef reader_settings_with_stub[] = {
    {
        .keyword = "archivers_ctl",
        .label = "Archivers Config",
        .help = "Path to compress.cfg which defines archiving/unarchiving programs for QWK bundles. Maximus and Squish use compatible formats.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "etc/compress.cfg"
    },
    {
        .keyword = "packet_name",
        .label = "Packet Name",
        .help = "Base filename for QWK packets. Keep to 8 characters, no spaces, DOS-safe characters only.",
        .type = FIELD_TEXT,
        .max_length = 8,
        .default_value = "MAXIMUS"
    },
    {
        .keyword = "work_directory",
        .label = "Work Directory",
        .help = "Blank work directory for offline reader operations. Maximus creates subdirectories here - do not modify manually while in use.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "tmp/reader"
    },
    {
        .keyword = "phone",
        .label = "Phone Number",
        .help = "Phone number embedded into downloaded packets. Some readers expect format (xxx) yyy-zzzz.",
        .type = FIELD_TEXT,
        .max_length = 20,
        .default_value = ""
    },
    {
        .keyword = "max_pack",
        .label = "Max Messages",
        .help = "Maximum number of messages that can be downloaded in one browse/download session.",
        .type = FIELD_TEXT,
        .max_length = 10,
        .default_value = "500"
    },
    {
        .keyword = NULL,
        .label = "Edit compress.cfg...",
        .help = "Stub: a future editor for compress.cfg. This will later migrate to TOML.",
        .type = FIELD_ACTION,
        .max_length = 0,
        .default_value = "[Press Enter]",
        .action = action_edit_compress_cfg,
        .action_ctx = NULL
    },
};

static void action_reader_settings(void)
{
    for (int i = 0; i < 6; i++) free(reader_settings_values[i]);

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    const char *sys_path = current_sys_path();
    reader_settings_values[0] = canonicalize_for_display(sys_path, toml_get_string_or_empty("general.reader.archivers_ctl"), NULL);
    reader_settings_values[1] = strdup(toml_get_string_or_empty("general.reader.packet_name"));
    reader_settings_values[2] = canonicalize_for_display(sys_path, toml_get_string_or_empty("general.reader.work_directory"), NULL);
    reader_settings_values[3] = strdup(toml_get_string_or_empty("general.reader.phone"));
    reader_settings_values[4] = strdup(toml_get_string_or_empty("general.reader.max_pack"));
    if (reader_settings_values[4][0] == '\0') {
        free(reader_settings_values[4]);
        reader_settings_values[4] = strdup("500");
    }
    reader_settings_values[5] = strdup("[Press Enter]");

    int dirty_fields[32];
    int dirty_count = 0;
    bool saved = form_edit("Reader Settings", reader_settings_with_stub, 6, reader_settings_values, dirty_fields, &dirty_count);

    if (saved) {
        char *archivers = normalize_under_sys_path(sys_path, reader_settings_values[0] ? reader_settings_values[0] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "general.reader.archivers_ctl", archivers ? archivers : "");
        free(archivers);
        
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "general.reader.packet_name", reader_settings_values[1] ? reader_settings_values[1] : "");
        
        char *work_dir = normalize_under_sys_path(sys_path, reader_settings_values[2] ? reader_settings_values[2] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "general.reader.work_directory", work_dir ? work_dir : "");
        free(work_dir);
        
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "general.reader.phone", reader_settings_values[3] ? reader_settings_values[3] : "");
        (void)maxcfg_toml_override_set_string(g_maxcfg_toml, "general.reader.max_pack", reader_settings_values[4] ? reader_settings_values[4] : "500");
        
        g_state.dirty = true;
    }
}

static void action_edit_compress_cfg(void *unused)
{
    (void)unused;
    dialog_message("Not Implemented", "compress.cfg editor is stubbed for now.");
}

/* ============================================================================
 * Message Area Functions
 * ============================================================================ */

/* Sample data for demo - will be replaced with actual CTL parsing */
static ListItem sample_divisions[] = {
    { "Programming Languages", "5 areas", true, NULL },
    { "Gaming", "3 areas", true, NULL },
    { "General", "2 areas", true, NULL },
};

static ListItem sample_areas[] = {
    { "Main", "Main", true, NULL },
    { "Fidonet Netmail", "Fido Netmail", true, NULL },
    { "Trashcan Conference", "Lost mail", true, NULL },
    { "My Conference", NULL, true, NULL },
    { "Pascal", NULL, true, NULL },
    { "Fun Conference", NULL, true, NULL },
    { "Another Conference without a Div", NULL, true, NULL },
    { "C++", NULL, true, NULL },
    { "Ferrari", NULL, false, NULL },
    { "Mazda", NULL, true, NULL },
};

/* Build tree from sample_divisions and sample_areas */
static TreeNode **build_tree_from_samples(int *count)
{
    /* 
     * Map our sample data to a tree structure:
     * - Programming Languages (division, div=0)
     *   - Pascal (area, div=1)
     *   - C++ (area, div=1)
     * - Gaming (division, div=0)
     *   - Fun Conference (area, div=1)
     *   - Ferrari (area, div=1)
     *   - Mazda (area, div=1)
     * - General (division, div=0)
     *   - My Conference (area, div=1)
     *   - Another Conference without a Div (area, div=1)
     * - Main (area, div=0) - no division
     * - Fidonet Netmail (area, div=0)
     * - Trashcan Conference (area, div=0)
     */
    
    /* We'll have 3 divisions + 3 top-level areas = 6 root nodes */
    TreeNode **roots = malloc(6 * sizeof(TreeNode *));
    if (!roots) {
        *count = 0;
        return NULL;
    }
    
    int idx = 0;
    
    /* Division 0: Programming Languages */
    roots[idx] = treenode_create("Programming Languages", "Programming Languages",
                                 sample_divisions[0].extra,
                                 TREENODE_DIVISION, 0);
    /* Add Pascal and C++ under it */
    TreeNode *pascal_area = treenode_create("Pascal", "Programming Languages.Pascal",
                                            "Programming language area",
                                            TREENODE_AREA, 1);
    treenode_add_child(roots[idx], pascal_area);
    
    TreeNode *cpp = treenode_create("C++", "Programming Languages.C++",
                                    "C++ programming discussions",
                                    TREENODE_AREA, 1);
    treenode_add_child(roots[idx], cpp);
    idx++;
    
    /* Division 1: Gaming */
    roots[idx] = treenode_create("Gaming", "Gaming",
                                 sample_divisions[1].extra,
                                 TREENODE_DIVISION, 0);
    /* Add Fun Conference, Ferrari, Mazda under it */
    TreeNode *fun = treenode_create("Fun Conference", "Gaming.Fun Conference",
                                    "Fun gaming discussions",
                                    TREENODE_AREA, 1);
    treenode_add_child(roots[idx], fun);
    
    TreeNode *ferrari = treenode_create("Ferrari", "Gaming.Ferrari",
                                        "Racing games - Ferrari",
                                        TREENODE_AREA, 1);
    ferrari->enabled = false;  /* Disabled in sample data */
    treenode_add_child(roots[idx], ferrari);
    
    TreeNode *mazda = treenode_create("Mazda", "Gaming.Mazda",
                                      "Racing games - Mazda",
                                      TREENODE_AREA, 1);
    treenode_add_child(roots[idx], mazda);
    idx++;
    
    /* Division 2: General */
    roots[idx] = treenode_create("General", "General",
                                 sample_divisions[2].extra,
                                 TREENODE_DIVISION, 0);
    /* Add My Conference and Another Conference under it */
    TreeNode *myconf = treenode_create("My Conference", "General.My Conference",
                                       "General discussions",
                                       TREENODE_AREA, 1);
    treenode_add_child(roots[idx], myconf);
    
    TreeNode *another = treenode_create("Another Conference", "General.Another Conference",
                                        "Another conference area",
                                        TREENODE_AREA, 1);
    treenode_add_child(roots[idx], another);
    idx++;
    
    /* Top-level areas (no division) */
    roots[idx++] = treenode_create("Main", "Main",
                                   sample_areas[0].extra,
                                   TREENODE_AREA, 0);
    
    roots[idx++] = treenode_create("Fidonet Netmail", "Fidonet Netmail",
                                   sample_areas[1].extra,
                                   TREENODE_AREA, 0);
    
    roots[idx++] = treenode_create("Trashcan Conference", "Trashcan Conference",
                                   sample_areas[2].extra,
                                   TREENODE_AREA, 0);
    
    *count = idx;
    return roots;
}

static void action_msg_tree_config(void)
{
    const char *sys_path = current_sys_path();
    if (!sys_path || !sys_path[0]) {
        dialog_message("Error", "System path not configured.");
        return;
    }
    
    /* Load msg areas TOML */
    char err[256];
    char toml_path[MAX_PATH_LEN];
    if (!load_msg_areas_toml_for_ui(sys_path, toml_path, sizeof(toml_path), err, sizeof(err))) {
        dialog_message("Load Error", err[0] ? err : "Failed to load message areas TOML");
        return;
    }

    int root_count = 0;
    TreeNode **roots = load_msgarea_toml(g_maxcfg_toml, &root_count, err, sizeof(err));
     
    if (!roots) {
        dialog_message("Load Error", err[0] ? err : "Failed to load message areas");
        return;
    }
    
    /* Show tree view */
    TreeViewResult result = treeview_show("Message Area Configuration", &roots, &root_count, NULL, TREE_CONTEXT_MESSAGE);
    
    /* Save if user made changes */
    if (result == TREEVIEW_EDIT || result == TREEVIEW_INSERT) {
        if (!save_msgarea_toml(g_maxcfg_toml, toml_path, roots, root_count, err, sizeof(err))) {
            dialog_message("Save Error", err[0] ? err : "Failed to save message areas TOML");
        } else {
            g_state.tree_reload_needed = true;
        }
    }
    
    /* Cleanup */
    free_msg_tree(roots, root_count);
    
    /* Redraw screen */
    touchwin(stdscr);
    wnoutrefresh(stdscr);
}

static bool user_editor_filter_has_wildcards(const char *s)
{
    if (s == NULL) {
        return false;
    }
    for (const char *p = s; *p; p++) {
        if (*p == '*' || *p == '%') {
            return true;
        }
    }
    return false;
}

static bool user_editor_contains_ci(const char *haystack, const char *needle)
{
    if (haystack == NULL || needle == NULL) {
        return false;
    }
    if (*needle == '\0') {
        return true;
    }
    for (const char *h = haystack; *h; h++) {
        const char *hp = h;
        const char *np = needle;
        while (*hp && *np && tolower((unsigned char)*hp) == tolower((unsigned char)*np)) {
            hp++;
            np++;
        }
        if (*np == '\0') {
            return true;
        }
    }
    return false;
}

static bool user_editor_wild_match_ci(const char *pattern, const char *text)
{
    if (pattern == NULL || text == NULL) {
        return false;
    }

    while (*pattern) {
        char pc = *pattern;
        if (pc == '%') {
            pc = '*';
        }

        if (pc == '*') {
            while (*pattern == '*' || *pattern == '%') {
                pattern++;
            }
            if (*pattern == '\0') {
                return true;
            }
            for (const char *t = text; ; t++) {
                if (user_editor_wild_match_ci(pattern, t)) {
                    return true;
                }
                if (*t == '\0') {
                    break;
                }
            }
            return false;
        }

        if (*text == '\0') {
            return false;
        }

        if (tolower((unsigned char)pc) != tolower((unsigned char)*text)) {
            return false;
        }

        pattern++;
        text++;
    }

    return *text == '\0';
}

static void user_editor_usr_field_to_cstr(char *dst, size_t dst_sz, const byte *src, size_t src_len)
{
    if (dst == NULL || dst_sz == 0) {
        return;
    }
    dst[0] = '\0';
    if (src == NULL || src_len == 0) {
        return;
    }

    size_t n = 0;
    while (n + 1u < dst_sz && n < src_len && src[n] != 0) {
        dst[n] = (char)src[n];
        n++;
    }
    dst[n] = '\0';
    while (n > 0 && isspace((unsigned char)dst[n - 1])) {
        dst[n - 1] = '\0';
        n--;
    }
}

static void user_editor_set_usr_field(byte *dst, size_t dst_len, const char *src)
{
    if (dst == NULL || dst_len == 0) {
        return;
    }
    memset(dst, 0, dst_len);
    if (src == NULL) {
        return;
    }
    size_t n = strlen(src);
    if (n >= dst_len) {
        n = dst_len - 1u;
    }
    memcpy(dst, src, n);
    dst[n] = 0;
}

static char *user_editor_resolve_userfile_root(const char *sys_path, const char *raw)
{
    if (raw == NULL || raw[0] == '\0') {
        return NULL;
    }

    const char *p = raw;
    if (p[0] == ':') {
        p++;
    }

    size_t len = strlen(p);
    if (len >= 4) {
        const char *ext = p + len - 4;
        if (strcasecmp(ext, ".bbs") == 0 || strcasecmp(ext, ".idx") == 0) {
            len -= 4;
        }
    }

    char *tmp = malloc(len + 1u);
    if (tmp == NULL) {
        return NULL;
    }
    memcpy(tmp, p, len);
    tmp[len] = '\0';

    if (path_is_absolute(tmp)) {
        return tmp;
    }

    if (sys_path == NULL || sys_path[0] == '\0') {
        return tmp;
    }

    size_t sys_len = strlen(sys_path);
    while (sys_len > 1u && (sys_path[sys_len - 1u] == '/' || sys_path[sys_len - 1u] == '\\')) {
        sys_len--;
    }

    size_t out_len = sys_len + 1u + strlen(tmp) + 1u;
    char *out = malloc(out_len);
    if (out == NULL) {
        return tmp;
    }
    memcpy(out, sys_path, sys_len);
    out[sys_len] = '/';
    memcpy(out + sys_len + 1u, tmp, strlen(tmp) + 1u);
    free(tmp);
    return out;
}

static bool user_editor_prompt_filter(char **io_filter)
{
    if (io_filter == NULL) {
        return false;
    }

    static const FieldDef fields[] = {
        {
            .keyword = "Filter",
            .label = "Filter",
            .help = "Enter a name or alias filter. Use * or % as wildcards.",
            .type = FIELD_TEXT,
            .max_length = 60,
            .default_value = "",
            .toggle_options = NULL,
            .file_filter = NULL,
            .file_base_path = NULL,
            .can_disable = false,
            .supports_mex = false,
            .pair_with_next = false,
            .action = NULL,
            .action_ctx = NULL,
        },
    };

    char *values[1] = { NULL };
    values[0] = strdup((*io_filter) ? (*io_filter) : "");
    if (values[0] == NULL) {
        return false;
    }

    bool saved = form_edit("User List Filter", fields, 1, values, NULL, NULL);
    if (saved) {
        free(*io_filter);
        *io_filter = strdup(values[0] ? values[0] : "");
    }

    free(values[0]);
    return saved;
}

static void user_editor_edit_personal(HUF huf, long rec, const char *sys_path)
{
    (void)sys_path;
    if (huf == NULL || rec < 0) {
        return;
    }

    struct _usr usr;
    if (!UserFileSeek(huf, rec, &usr, sizeof usr)) {
        dialog_message("User Editor", "Unable to load user record.");
        return;
    }

    char old_name[64];
    char old_alias[64];
    user_editor_usr_field_to_cstr(old_name, sizeof(old_name), usr.name, sizeof(usr.name));
    user_editor_usr_field_to_cstr(old_alias, sizeof(old_alias), usr.alias, sizeof(usr.alias));

    char name[64];
    char alias[64];
    char city[64];
    char phone[64];
    char dataphone[64];
    user_editor_usr_field_to_cstr(name, sizeof(name), usr.name, sizeof(usr.name));
    user_editor_usr_field_to_cstr(alias, sizeof(alias), usr.alias, sizeof(usr.alias));
    user_editor_usr_field_to_cstr(city, sizeof(city), usr.city, sizeof(usr.city));
    user_editor_usr_field_to_cstr(phone, sizeof(phone), usr.phone, sizeof(usr.phone));
    user_editor_usr_field_to_cstr(dataphone, sizeof(dataphone), (const byte *)usr.dataphone, sizeof(usr.dataphone));

    char *values[5] = {
        strdup(name),
        strdup(alias),
        strdup(city),
        strdup(phone),
        strdup(dataphone),
    };
    for (int i = 0; i < 5; i++) {
        if (values[i] == NULL) {
            for (int j = 0; j < 5; j++) {
                free(values[j]);
            }
            dialog_message("Out of Memory", "Unable to allocate form values.");
            return;
        }
    }

    static const FieldDef fields[] = {
        { "Name", "Name", "User's real name.", FIELD_TEXT, 35, "", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "Alias", "Alias", "User's handle/alias.", FIELD_TEXT, 20, "", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "City", "City", "User's location.", FIELD_TEXT, 35, "", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "Phone", "Phone", "User's phone number.", FIELD_TEXT, 14, "", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "DataPhone", "Data/Business", "User's data/business phone number.", FIELD_TEXT, 18, "", NULL, NULL, NULL, false, false, false, NULL, NULL },
    };

    bool saved = form_edit("User Editor: Personal Information", fields, 5, values, NULL, NULL);
    if (saved) {
        user_editor_set_usr_field(usr.name, sizeof(usr.name), values[0] ? values[0] : "");
        user_editor_set_usr_field(usr.alias, sizeof(usr.alias), values[1] ? values[1] : "");
        user_editor_set_usr_field(usr.city, sizeof(usr.city), values[2] ? values[2] : "");
        user_editor_set_usr_field(usr.phone, sizeof(usr.phone), values[3] ? values[3] : "");
        user_editor_set_usr_field((byte *)usr.dataphone, sizeof(usr.dataphone), values[4] ? values[4] : "");

        if (!UserFileUpdate(huf, old_name[0] ? old_name : NULL, old_alias[0] ? old_alias : NULL, &usr)) {
            dialog_message("User Editor", "Failed to update user record.");
        }
    }

    for (int i = 0; i < 5; i++) {
        free(values[i]);
    }
}

typedef struct {
    HUF huf;
    long rec;
    struct _usr *usr;
    char *old_name;
    char *old_alias;
} UserEditorPasswordActionCtx;

static void user_editor_password_action(void *ctx)
{
    UserEditorPasswordActionCtx *a = (UserEditorPasswordActionCtx *)ctx;
    if (a == NULL || a->usr == NULL) {
        return;
    }

    char new_pwd[16];
    new_pwd[0] = '\0';

    if (a->usr->bits & BITS_ENCRYPT) {
        if (!user_editor_prompt_password(new_pwd, sizeof(new_pwd))) {
            return;
        }
    } else {
        user_editor_usr_field_to_cstr(new_pwd, sizeof(new_pwd), a->usr->pwd, sizeof(a->usr->pwd));
        if (!user_editor_prompt_password(new_pwd, sizeof(new_pwd))) {
            return;
        }
    }

    if (new_pwd[0] == '\0') {
        memset(a->usr->pwd, 0, sizeof(a->usr->pwd));
        a->usr->bits &= ~BITS_ENCRYPT;
    } else {
        char fancy[32];
        strncpy(fancy, new_pwd, sizeof(fancy) - 1);
        fancy[sizeof(fancy) - 1] = '\0';
        strlwr(fancy);

        char md5_hash[MD5_SIZE];
        string_to_MD5(fancy, md5_hash);
        memcpy(a->usr->pwd, md5_hash, sizeof(a->usr->pwd));
        a->usr->bits |= BITS_ENCRYPT;

        Get_Dos_Date(&a->usr->date_pwd_chg);
    }

    if (!UserFileUpdate(a->huf, a->old_name && a->old_name[0] ? a->old_name : NULL,
                        a->old_alias && a->old_alias[0] ? a->old_alias : NULL, a->usr)) {
        dialog_message("User Editor", "Failed to update user record.");
    }
}

static bool user_editor_prompt_password(char *pwd_buf, size_t pwd_sz)
{
    if (pwd_buf == NULL || pwd_sz == 0) {
        return false;
    }

    const FieldDef fields[] = {
        { "Password", "Password", "Enter new password (max 15 chars). Leave blank to clear.", FIELD_TEXT, 15, "", NULL, NULL, NULL, false, false, false, NULL, NULL },
    };

    char *values[1] = { strdup(pwd_buf) };
    if (values[0] == NULL) {
        return false;
    }

    bool saved = form_edit("Set Password", fields, 1, values, NULL, NULL);
    if (saved) {
        strncpy(pwd_buf, values[0] ? values[0] : "", pwd_sz - 1);
        pwd_buf[pwd_sz - 1] = '\0';
    }

    free(values[0]);
    return saved;
}

static void user_editor_edit_security(HUF huf, long rec, const char *sys_path)
{
    if (huf == NULL || rec < 0) {
        return;
    }

    struct _usr usr;
    if (!UserFileSeek(huf, rec, &usr, sizeof usr)) {
        dialog_message("User Editor", "Unable to load user record.");
        return;
    }

    char old_name[64];
    char old_alias[64];
    user_editor_usr_field_to_cstr(old_name, sizeof(old_name), usr.name, sizeof(usr.name));
    user_editor_usr_field_to_cstr(old_alias, sizeof(old_alias), usr.alias, sizeof(usr.alias));

    const char *priv_name = access_level_name_for_level(sys_path ? sys_path : "", (int)usr.priv);
    char priv_buf[64];
    snprintf(priv_buf, sizeof(priv_buf), "%s", priv_name ? priv_name : "");

    char pwd_display[32];
    if (usr.bits & BITS_ENCRYPT) {
        strcpy(pwd_display, "*******");
    } else {
        user_editor_usr_field_to_cstr(pwd_display, sizeof(pwd_display), usr.pwd, sizeof(usr.pwd));
        if (pwd_display[0] == '\0') {
            strcpy(pwd_display, "(none)");
        }
    }

    char *values[2] = { strdup(priv_buf), strdup(pwd_display) };
    for (int i = 0; i < 2; i++) {
        if (values[i] == NULL) {
            free(values[0]);
            free(values[1]);
            dialog_message("Out of Memory", "Unable to allocate form values.");
            return;
        }
    }

    UserEditorPasswordActionCtx pwd_ctx = {
        .huf = huf,
        .rec = rec,
        .usr = &usr,
        .old_name = old_name,
        .old_alias = old_alias
    };

    const FieldDef fields[] = {
        { "Priv", "Privilege", "User access/privilege level.", FIELD_SELECT, 0, "", access_level_options, NULL, NULL, false, false, false, NULL, NULL },
        { "Password", "Password", "Press P to edit password, Space to clear.", FIELD_ACTION, 0, "", NULL, NULL, NULL, false, false, false, user_editor_password_action, &pwd_ctx },
    };

    bool saved = form_edit("User Editor: Security", fields, 2, values, NULL, NULL);
    if (saved) {
        int lvl = parse_priv_level(sys_path ? sys_path : "", values[0]);
        usr.priv = (word)lvl;
        if (!UserFileUpdate(huf, old_name[0] ? old_name : NULL, old_alias[0] ? old_alias : NULL, &usr)) {
            dialog_message("User Editor", "Failed to update user record.");
        }
    }

    for (int i = 0; i < 2; i++) {
        free(values[i]);
    }
}

static void user_editor_edit_settings(HUF huf, long rec, const char *sys_path)
{
    (void)sys_path;
    if (huf == NULL || rec < 0) {
        return;
    }

    struct _usr usr;
    if (!UserFileSeek(huf, rec, &usr, sizeof usr)) {
        dialog_message("User Editor", "Unable to load user record.");
        return;
    }

    char old_name[64];
    char old_alias[64];
    user_editor_usr_field_to_cstr(old_name, sizeof(old_name), usr.name, sizeof(usr.name));
    user_editor_usr_field_to_cstr(old_alias, sizeof(old_alias), usr.alias, sizeof(usr.alias));

    static const char *video_opts[] = { "TTY", "ANSI", "Avatar", "RIP", NULL };
    static const char *help_opts[] = { "Novice", "Regular", "Expert", NULL };
    static const char *sex_opts[] = { "Unknown", "Male", "Female", NULL };
    static const char *yesno_opts[] = { "No", "Yes", NULL };

    char video_val[16];
    if (usr.video == GRAPH_ANSI) strcpy(video_val, "ANSI");
    else if (usr.video == GRAPH_AVATAR) strcpy(video_val, "Avatar");
    else if (usr.video == GRAPH_RIP) strcpy(video_val, "RIP");
    else strcpy(video_val, "TTY");

    char help_val[16];
    if (usr.help == 0) strcpy(help_val, "Novice");
    else if (usr.help == 1) strcpy(help_val, "Regular");
    else strcpy(help_val, "Expert");

    char sex_val[16];
    if (usr.sex == SEX_MALE) strcpy(sex_val, "Male");
    else if (usr.sex == SEX_FEMALE) strcpy(sex_val, "Female");
    else strcpy(sex_val, "Unknown");

    char width_buf[8], len_buf[8], nulls_buf[8];
    snprintf(width_buf, sizeof(width_buf), "%u", (unsigned)usr.width);
    snprintf(len_buf, sizeof(len_buf), "%u", (unsigned)usr.len);
    snprintf(nulls_buf, sizeof(nulls_buf), "%u", (unsigned)usr.nulls);

    char *values[10] = {
        strdup(video_val),
        strdup(help_val),
        strdup(sex_val),
        strdup(width_buf),
        strdup(len_buf),
        strdup(nulls_buf),
        strdup((usr.bits & BITS_HOTKEYS) ? "Yes" : "No"),
        strdup((usr.bits2 & BITS2_MORE) ? "Yes" : "No"),
        strdup((usr.bits & BITS_FSR) ? "Yes" : "No"),
        strdup((usr.bits2 & BITS2_IBMCHARS) ? "Yes" : "No"),
    };
    for (int i = 0; i < 10; i++) {
        if (values[i] == NULL) {
            for (int j = 0; j < 10; j++) free(values[j]);
            dialog_message("Out of Memory", "Unable to allocate form values.");
            return;
        }
    }

    const FieldDef fields[] = {
        { "Video", "Video Mode", "Terminal graphics mode.", FIELD_SELECT, 0, "ANSI", video_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "Help", "Help Level", "User help level.", FIELD_SELECT, 0, "Regular", help_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "Sex", "Sex", "User gender.", FIELD_SELECT, 0, "Unknown", sex_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "Width", "Screen Width", "Terminal width in columns.", FIELD_TEXT, 3, "80", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "Length", "Screen Length", "Terminal height in rows.", FIELD_TEXT, 3, "24", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "Nulls", "Nulls", "Number of nulls after CR.", FIELD_TEXT, 3, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "Hotkeys", "Hotkeys", "Enable hotkeys?", FIELD_SELECT, 0, "No", yesno_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "More", "More Prompt", "Show MORE? prompt?", FIELD_SELECT, 0, "Yes", yesno_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "FSR", "Full Screen Reader", "Use full-screen message reader?", FIELD_SELECT, 0, "No", yesno_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "IBMChars", "IBM Characters", "Can receive high-bit chars?", FIELD_SELECT, 0, "Yes", yesno_opts, NULL, NULL, false, false, false, NULL, NULL },
    };

    bool saved = form_edit("User Editor: Settings", fields, 10, values, NULL, NULL);
    if (saved) {
        if (strcasecmp(values[0], "ANSI") == 0) usr.video = GRAPH_ANSI;
        else if (strcasecmp(values[0], "Avatar") == 0) usr.video = GRAPH_AVATAR;
        else if (strcasecmp(values[0], "RIP") == 0) usr.video = GRAPH_RIP;
        else usr.video = GRAPH_TTY;

        if (strcasecmp(values[1], "Novice") == 0) usr.help = 0;
        else if (strcasecmp(values[1], "Regular") == 0) usr.help = 1;
        else usr.help = 2;

        if (strcasecmp(values[2], "Male") == 0) usr.sex = SEX_MALE;
        else if (strcasecmp(values[2], "Female") == 0) usr.sex = SEX_FEMALE;
        else usr.sex = SEX_UNKNOWN;

        usr.width = (byte)atoi(values[3]);
        usr.len = (byte)atoi(values[4]);
        usr.nulls = (byte)atoi(values[5]);

        if (strcasecmp(values[6], "Yes") == 0) usr.bits |= BITS_HOTKEYS;
        else usr.bits &= ~BITS_HOTKEYS;

        if (strcasecmp(values[7], "Yes") == 0) usr.bits2 |= BITS2_MORE;
        else usr.bits2 &= ~BITS2_MORE;

        if (strcasecmp(values[8], "Yes") == 0) usr.bits |= BITS_FSR;
        else usr.bits &= ~BITS_FSR;

        if (strcasecmp(values[9], "Yes") == 0) usr.bits2 |= BITS2_IBMCHARS;
        else usr.bits2 &= ~BITS2_IBMCHARS;

        if (!UserFileUpdate(huf, old_name[0] ? old_name : NULL, old_alias[0] ? old_alias : NULL, &usr)) {
            dialog_message("User Editor", "Failed to update user record.");
        }
    }

    for (int i = 0; i < 10; i++) {
        free(values[i]);
    }
}

static void user_editor_edit_statistics(HUF huf, long rec, const char *sys_path)
{
    (void)sys_path;
    if (huf == NULL || rec < 0) {
        return;
    }

    struct _usr usr;
    if (!UserFileSeek(huf, rec, &usr, sizeof usr)) {
        dialog_message("User Editor", "Unable to load user record.");
        return;
    }

    char old_name[64];
    char old_alias[64];
    user_editor_usr_field_to_cstr(old_name, sizeof(old_name), usr.name, sizeof(usr.name));
    user_editor_usr_field_to_cstr(old_alias, sizeof(old_alias), usr.alias, sizeof(usr.alias));

    char times_buf[16], call_buf[16], time_buf[16], time_added_buf[16];
    char msgs_posted_buf[16], msgs_read_buf[16];
    char up_buf[16], down_buf[16], downtoday_buf[16];
    char nup_buf[16], ndown_buf[16], ndowntoday_buf[16];
    char credit_buf[16], debit_buf[16];

    snprintf(times_buf, sizeof(times_buf), "%u", (unsigned)usr.times);
    snprintf(call_buf, sizeof(call_buf), "%u", (unsigned)usr.call);
    snprintf(time_buf, sizeof(time_buf), "%u", (unsigned)usr.time);
    snprintf(time_added_buf, sizeof(time_added_buf), "%u", (unsigned)usr.time_added);
    snprintf(msgs_posted_buf, sizeof(msgs_posted_buf), "%lu", (unsigned long)usr.msgs_posted);
    snprintf(msgs_read_buf, sizeof(msgs_read_buf), "%lu", (unsigned long)usr.msgs_read);
    snprintf(up_buf, sizeof(up_buf), "%lu", (unsigned long)usr.up);
    snprintf(down_buf, sizeof(down_buf), "%lu", (unsigned long)usr.down);
    snprintf(downtoday_buf, sizeof(downtoday_buf), "%ld", (long)usr.downtoday);
    snprintf(nup_buf, sizeof(nup_buf), "%lu", (unsigned long)usr.nup);
    snprintf(ndown_buf, sizeof(ndown_buf), "%lu", (unsigned long)usr.ndown);
    snprintf(ndowntoday_buf, sizeof(ndowntoday_buf), "%ld", (long)usr.ndowntoday);
    snprintf(credit_buf, sizeof(credit_buf), "%u", (unsigned)usr.credit);
    snprintf(debit_buf, sizeof(debit_buf), "%u", (unsigned)usr.debit);

    char *values[14] = {
        strdup(times_buf), strdup(call_buf), strdup(time_buf), strdup(time_added_buf),
        strdup(msgs_posted_buf), strdup(msgs_read_buf),
        strdup(up_buf), strdup(down_buf), strdup(downtoday_buf),
        strdup(nup_buf), strdup(ndown_buf), strdup(ndowntoday_buf),
        strdup(credit_buf), strdup(debit_buf),
    };
    for (int i = 0; i < 14; i++) {
        if (values[i] == NULL) {
            for (int j = 0; j < 14; j++) free(values[j]);
            dialog_message("Out of Memory", "Unable to allocate form values.");
            return;
        }
    }

    static const FieldDef fields[] = {
        { "Times", "Total Calls", "Total number of calls to system.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "Call", "Calls Today", "Number of calls today.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "Time", "Time Today", "Minutes online today.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "TimeAdded", "Time Added", "Minutes credited today.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "MsgsPosted", "Messages Posted", "Total messages posted.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "MsgsRead", "Messages Read", "Total messages read.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "Up", "KB Uploaded", "Total KB uploaded.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "Down", "KB Downloaded", "Total KB downloaded.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "DownToday", "KB Down Today", "KB downloaded today.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "NUp", "Files Uploaded", "Total files uploaded.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "NDown", "Files Downloaded", "Total files downloaded.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "NDownToday", "Files Down Today", "Files downloaded today.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "Credit", "Credit (cents)", "Matrix credit in cents.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "Debit", "Debit (cents)", "Matrix debit in cents.", FIELD_TEXT, 10, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
    };

    bool saved = form_edit("User Editor: Statistics", fields, 14, values, NULL, NULL);
    if (saved) {
        usr.times = (word)atoi(values[0]);
        usr.call = (word)atoi(values[1]);
        usr.time = (word)atoi(values[2]);
        usr.time_added = (word)atoi(values[3]);
        usr.msgs_posted = (dword)strtoul(values[4], NULL, 10);
        usr.msgs_read = (dword)strtoul(values[5], NULL, 10);
        usr.up = (dword)strtoul(values[6], NULL, 10);
        usr.down = (dword)strtoul(values[7], NULL, 10);
        usr.downtoday = (sdword)strtol(values[8], NULL, 10);
        usr.nup = (dword)strtoul(values[9], NULL, 10);
        usr.ndown = (dword)strtoul(values[10], NULL, 10);
        usr.ndowntoday = (sdword)strtol(values[11], NULL, 10);
        usr.credit = (word)atoi(values[12]);
        usr.debit = (word)atoi(values[13]);

        if (!UserFileUpdate(huf, old_name[0] ? old_name : NULL, old_alias[0] ? old_alias : NULL, &usr)) {
            dialog_message("User Editor", "Failed to update user record.");
        }
    }

    for (int i = 0; i < 14; i++) {
        free(values[i]);
    }
}

static void user_editor_edit_dates(HUF huf, long rec, const char *sys_path)
{
    (void)sys_path;
    if (huf == NULL || rec < 0) {
        return;
    }

    struct _usr usr;
    if (!UserFileSeek(huf, rec, &usr, sizeof usr)) {
        dialog_message("User Editor", "Unable to load user record.");
        return;
    }

    char old_name[64];
    char old_alias[64];
    user_editor_usr_field_to_cstr(old_name, sizeof(old_name), usr.name, sizeof(usr.name));
    user_editor_usr_field_to_cstr(old_alias, sizeof(old_alias), usr.alias, sizeof(usr.alias));

    char dob_buf[32];
    snprintf(dob_buf, sizeof(dob_buf), "%04u-%02u-%02u", 
             (unsigned)usr.dob_year, (unsigned)usr.dob_month, (unsigned)usr.dob_day);

    char *values[1] = { strdup(dob_buf) };
    if (values[0] == NULL) {
        dialog_message("Out of Memory", "Unable to allocate form values.");
        return;
    }

    static const FieldDef fields[] = {
        { "DOB", "Date of Birth", "Format: YYYY-MM-DD", FIELD_TEXT, 10, "1900-01-01", NULL, NULL, NULL, false, false, false, NULL, NULL },
    };

    bool saved = form_edit("User Editor: Dates", fields, 1, values, NULL, NULL);
    if (saved && values[0]) {
        unsigned y = 1900, m = 1, d = 1;
        if (sscanf(values[0], "%u-%u-%u", &y, &m, &d) == 3) {
            usr.dob_year = (word)y;
            usr.dob_month = (byte)m;
            usr.dob_day = (byte)d;

            if (!UserFileUpdate(huf, old_name[0] ? old_name : NULL, old_alias[0] ? old_alias : NULL, &usr)) {
                dialog_message("User Editor", "Failed to update user record.");
            }
        } else {
            dialog_message("Invalid Date", "Date must be in YYYY-MM-DD format.");
        }
    }

    free(values[0]);
}

static void user_editor_edit_keys_flags(HUF huf, long rec, const char *sys_path)
{
    (void)sys_path;
    if (huf == NULL || rec < 0) {
        return;
    }

    struct _usr usr;
    if (!UserFileSeek(huf, rec, &usr, sizeof usr)) {
        dialog_message("User Editor", "Unable to load user record.");
        return;
    }

    char old_name[64];
    char old_alias[64];
    user_editor_usr_field_to_cstr(old_name, sizeof(old_name), usr.name, sizeof(usr.name));
    user_editor_usr_field_to_cstr(old_alias, sizeof(old_alias), usr.alias, sizeof(usr.alias));

    static const char *yesno_opts[] = { "No", "Yes", NULL };

    char *values[8] = {
        strdup((usr.bits & BITS_NOTAVAIL) ? "Yes" : "No"),
        strdup((usr.bits & BITS_NERD) ? "Yes" : "No"),
        strdup((usr.bits & BITS_NOULIST) ? "Yes" : "No"),
        strdup((usr.bits & BITS_TABS) ? "Yes" : "No"),
        strdup((usr.bits & BITS_RIP) ? "Yes" : "No"),
        strdup((usr.bits2 & BITS2_BADLOGON) ? "Yes" : "No"),
        strdup((usr.bits2 & BITS2_BORED) ? "Yes" : "No"),
        strdup((usr.bits2 & BITS2_CLS) ? "Yes" : "No"),
    };
    for (int i = 0; i < 8; i++) {
        if (values[i] == NULL) {
            for (int j = 0; j < 8; j++) free(values[j]);
            dialog_message("Out of Memory", "Unable to allocate form values.");
            return;
        }
    }

    const FieldDef fields[] = {
        { "NotAvail", "Not Available", "User not available for chat?", FIELD_SELECT, 0, "No", yesno_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "Nerd", "Nerd Mode", "Yelling makes no noise?", FIELD_SELECT, 0, "No", yesno_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "NoUList", "Hide from Userlist", "Don't show in userlist?", FIELD_SELECT, 0, "No", yesno_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "Tabs", "Raw Tabs", "Can handle raw tabs?", FIELD_SELECT, 0, "No", yesno_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "RIP", "RIP Graphics", "RIP support?", FIELD_SELECT, 0, "No", yesno_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "BadLogon", "Bad Logon Flag", "Last logon was bad?", FIELD_SELECT, 0, "No", yesno_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "Bored", "Line Editor", "Use line-oriented editor?", FIELD_SELECT, 0, "No", yesno_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "CLS", "Clear Screen", "Transmit clear screen?", FIELD_SELECT, 0, "Yes", yesno_opts, NULL, NULL, false, false, false, NULL, NULL },
    };

    bool saved = form_edit("User Editor: Keys/Flags", fields, 8, values, NULL, NULL);
    if (saved) {
        if (strcasecmp(values[0], "Yes") == 0) usr.bits |= BITS_NOTAVAIL;
        else usr.bits &= ~BITS_NOTAVAIL;

        if (strcasecmp(values[1], "Yes") == 0) usr.bits |= BITS_NERD;
        else usr.bits &= ~BITS_NERD;

        if (strcasecmp(values[2], "Yes") == 0) usr.bits |= BITS_NOULIST;
        else usr.bits &= ~BITS_NOULIST;

        if (strcasecmp(values[3], "Yes") == 0) usr.bits |= BITS_TABS;
        else usr.bits &= ~BITS_TABS;

        if (strcasecmp(values[4], "Yes") == 0) usr.bits |= BITS_RIP;
        else usr.bits &= ~BITS_RIP;

        if (strcasecmp(values[5], "Yes") == 0) usr.bits2 |= BITS2_BADLOGON;
        else usr.bits2 &= ~BITS2_BADLOGON;

        if (strcasecmp(values[6], "Yes") == 0) usr.bits2 |= BITS2_BORED;
        else usr.bits2 &= ~BITS2_BORED;

        if (strcasecmp(values[7], "Yes") == 0) usr.bits2 |= BITS2_CLS;
        else usr.bits2 &= ~BITS2_CLS;

        if (!UserFileUpdate(huf, old_name[0] ? old_name : NULL, old_alias[0] ? old_alias : NULL, &usr)) {
            dialog_message("User Editor", "Failed to update user record.");
        }
    }

    for (int i = 0; i < 8; i++) {
        free(values[i]);
    }
}

static void user_editor_edit_user_categories(HUF huf, long rec, const char *sys_path)
{
    const char *options[] = {
        "Personal Information",
        "Security",
        "Settings",
        "Statistics",
        "Dates",
        "Keys/Flags",
        NULL
    };

    int sel = 0;
    for (;;) {
        int r = dialog_option_picker("User Categories", options, sel);
        if (r < 0) {
            return;
        }
        sel = r;
        if (r == 0) {
            user_editor_edit_personal(huf, rec, sys_path);
        } else if (r == 1) {
            user_editor_edit_security(huf, rec, sys_path);
        } else if (r == 2) {
            user_editor_edit_settings(huf, rec, sys_path);
        } else if (r == 3) {
            user_editor_edit_statistics(huf, rec, sys_path);
        } else if (r == 4) {
            user_editor_edit_dates(huf, rec, sys_path);
        } else if (r == 5) {
            user_editor_edit_keys_flags(huf, rec, sys_path);
        }
    }
}

static void action_user_editor(void)
{
    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    const char *sys_path = current_sys_path();
    const char *raw_userfile = toml_get_string_or_empty("maximus.file_password");
    char *userfile_root = user_editor_resolve_userfile_root(sys_path, raw_userfile);
    if (userfile_root == NULL || userfile_root[0] == '\0') {
        free(userfile_root);
        dialog_message("User Editor", "Missing maximus.file_password (user file root path).");
        return;
    }

    HUF huf = UserFileOpen(userfile_root, 0);
    if (huf == NULL) {
        free(userfile_root);
        dialog_message("User Editor", "Unable to open user file.");
        return;
    }

    char *filter = strdup("");
    if (filter == NULL) {
        UserFileClose(huf);
        free(userfile_root);
        dialog_message("Out of Memory", "Unable to allocate filter string.");
        return;
    }

    int selected = 0;

    for (;;) {
        long size = UserFileSize(huf);
        if (size < 0) {
            dialog_message("User Editor", "Unable to read user file.");
            break;
        }

        ListItem *items = NULL;
        int item_count = 0;
        int cap = 0;

        HUFF huff = UserFileFindSeqOpen(huf);
        if (huff != NULL) {
            do {
                const long rec = huff->lLastUser;

                char name[64];
                char alias[64];
                user_editor_usr_field_to_cstr(name, sizeof(name), huff->usr.name, sizeof(huff->usr.name));
                user_editor_usr_field_to_cstr(alias, sizeof(alias), huff->usr.alias, sizeof(huff->usr.alias));

                bool match = true;
                if (filter && filter[0]) {
                    if (user_editor_filter_has_wildcards(filter)) {
                        match = user_editor_wild_match_ci(filter, name) || user_editor_wild_match_ci(filter, alias);
                    } else {
                        match = user_editor_contains_ci(name, filter) || user_editor_contains_ci(alias, filter);
                    }
                }

                if (match) {
                    if (item_count >= cap) {
                        int new_cap = (cap == 0) ? 64 : cap * 2;
                        ListItem *ni = realloc(items, (size_t)new_cap * sizeof(ListItem));
                        if (ni == NULL) {
                            break;
                        }
                        items = ni;
                        cap = new_cap;
                    }

                    items[item_count].name = strdup(name[0] ? name : "(unnamed)");
                    items[item_count].extra = strdup(alias);
                    items[item_count].enabled = true;
                    items[item_count].data = (void *)(intptr_t)rec;
                    if (items[item_count].name == NULL || items[item_count].extra == NULL) {
                        free(items[item_count].name);
                        free(items[item_count].extra);
                        break;
                    }
                    item_count++;
                }

            } while (UserFileFindSeqNext(huff));
            UserFileFindSeqClose(huff);
        }

        if (item_count <= 0) {
            listitem_array_free(items, item_count);
            if (!user_editor_prompt_filter(&filter)) {
                break;
            }
            selected = 0;
            continue;
        }

        if (selected < 0) {
            selected = 0;
        }
        if (selected >= item_count) {
            selected = item_count - 1;
        }

        char list_title[128];
        if (filter && filter[0]) {
            snprintf(list_title, sizeof(list_title), "Users (Filter: %s)", filter);
        } else {
            strcpy(list_title, "Users (Name or Alias)");
        }

        ListPickResult r = listpicker_show_ex(list_title, items, item_count, &selected, true);

        if (r == LISTPICK_EXIT) {
            listitem_array_free(items, item_count);
            break;
        }
        if (r == LISTPICK_FILTER) {
            listitem_array_free(items, item_count);
            if (!user_editor_prompt_filter(&filter)) {
                break;
            }
            selected = 0;
            continue;
        }
        if (r == LISTPICK_CLEAR) {
            listitem_array_free(items, item_count);
            free(filter);
            filter = strdup("");
            selected = 0;
            continue;
        }
        if (r == LISTPICK_EDIT && selected >= 0 && selected < item_count) {
            long rec = (long)(intptr_t)items[selected].data;
            listitem_array_free(items, item_count);
            user_editor_edit_user_categories(huf, rec, sys_path);
            continue;
        }

        listitem_array_free(items, item_count);
    }

    free(filter);
    UserFileClose(huf);
    free(userfile_root);
}

static void action_bad_users(void)
{
    const char *sys_path = current_sys_path();
    if (!sys_path || !sys_path[0]) {
        dialog_message("Configuration Error", "System path not configured.");
        return;
    }

    char filepath[MAX_PATH_LEN];
    snprintf(filepath, sizeof(filepath), "%s/etc/baduser.bbs", sys_path);

    text_list_editor("Bad Users List", filepath,
                     "Enter a name/word to block. Use ~ prefix for 'contains' match (e.g., ~ass). Lines starting with ; are comments.");
}

static void action_reserved_names(void)
{
    const char *sys_path = current_sys_path();
    if (!sys_path || !sys_path[0]) {
        dialog_message("Configuration Error", "System path not configured.");
        return;
    }

    char filepath[MAX_PATH_LEN];
    snprintf(filepath, sizeof(filepath), "%s/etc/reserved.bbs", sys_path);

    text_list_editor("Reserved Names List", filepath,
                     "Enter a name to reserve. Use ~ prefix for 'contains' match. Lines starting with ; are comments.");
}

static bool load_msg_areas_toml_for_ui(const char *sys_path, char *out_path, size_t out_path_sz, char *err, size_t err_sz)
{
    if (out_path && out_path_sz > 0) {
        out_path[0] = '\0';
    }

    if (g_maxcfg_toml == NULL) {
        dialog_message("Error", "TOML configuration is not loaded.");
        return false;
    }
    if (sys_path == NULL || sys_path[0] == '\0') {
        if (err && err_sz > 0) {
            snprintf(err, err_sz, "%s", "System path not configured");
        }
        return false;
    }

    char path[MAX_PATH_LEN];
    (void)maxcfg_resolve_path(sys_path, "config/areas/msg/areas.toml", path, sizeof(path));

    MaxCfgStatus st = maxcfg_toml_load_file(g_maxcfg_toml, path, "areas.msg");
    if (st != MAXCFG_OK) {
        if (err && err_sz > 0) {
            snprintf(err, err_sz, "%s", maxcfg_status_string(st));
        }
        return false;
    }

    if (out_path && out_path_sz > 0) {
        strncpy(out_path, path, out_path_sz - 1u);
        out_path[out_path_sz - 1u] = '\0';
    }
    return true;
}

static bool load_file_areas_toml_for_ui(const char *sys_path, char *out_path, size_t out_path_sz, char *err, size_t err_sz)
{
    if (out_path && out_path_sz > 0) {
        out_path[0] = '\0';
    }

    if (g_maxcfg_toml == NULL) {
        dialog_message("Error", "TOML configuration is not loaded.");
        return false;
    }
    if (sys_path == NULL || sys_path[0] == '\0') {
        if (err && err_sz > 0) {
            snprintf(err, err_sz, "%s", "System path not configured");
        }
        return false;
    }

    char path[MAX_PATH_LEN];
    (void)maxcfg_resolve_path(sys_path, "config/areas/file/areas.toml", path, sizeof(path));

    MaxCfgStatus st = maxcfg_toml_load_file(g_maxcfg_toml, path, "areas.file");
    if (st != MAXCFG_OK) {
        if (err && err_sz > 0) {
            snprintf(err, err_sz, "%s", maxcfg_status_string(st));
        }
        return false;
    }

    if (out_path && out_path_sz > 0) {
        strncpy(out_path, path, out_path_sz - 1u);
        out_path[out_path_sz - 1u] = '\0';
    }

    return true;
}

static void open_menu_options_action(void *ctx)
{
    MenuEditContext *mctx = (MenuEditContext *)ctx;
    if (mctx && mctx->current_menu) {
        bool modified = menu_options_list(mctx->sys_path, mctx->menus, mctx->menu_count, mctx->current_menu);
        if (modified && mctx->options_modified) {
            *mctx->options_modified = true;
        }
    }
}

/* Color picker actions for custom menu lightbar colors */
typedef struct {
    MenuEditContext *mctx;
    char **values;
    int value_index;
    char **menu_fg;
    char **menu_bg;
} MenuColorActionCtx;

static void pick_normal_color(void *ctx);
static void pick_selected_color(void *ctx);
static void pick_high_color(void *ctx);
static void pick_high_sel_color(void *ctx);

static void set_owned_string(char **dst, const char *src)
{
    if (dst == NULL) {
        return;
    }
    free(*dst);
    *dst = NULL;
    if (src && *src) {
        *dst = strdup(src);
    }
}

static int color_name_to_value(const char *name)
{
    if (!name || !*name) return -1;
    if (strcasecmp(name, "Black") == 0) return 0;
    if (strcasecmp(name, "Blue") == 0) return 1;
    if (strcasecmp(name, "Green") == 0) return 2;
    if (strcasecmp(name, "Cyan") == 0) return 3;
    if (strcasecmp(name, "Red") == 0) return 4;
    if (strcasecmp(name, "Magenta") == 0) return 5;
    if (strcasecmp(name, "Brown") == 0) return 6;
    if (strcasecmp(name, "Gray") == 0 || strcasecmp(name, "Grey") == 0) return 7;
    if (strcasecmp(name, "DarkGray") == 0 || strcasecmp(name, "DarkGrey") == 0) return 8;
    if (strcasecmp(name, "LightBlue") == 0) return 9;
    if (strcasecmp(name, "LightGreen") == 0) return 10;
    if (strcasecmp(name, "LightCyan") == 0) return 11;
    if (strcasecmp(name, "LightRed") == 0) return 12;
    if (strcasecmp(name, "LightMagenta") == 0) return 13;
    if (strcasecmp(name, "Yellow") == 0) return 14;
    if (strcasecmp(name, "White") == 0) return 15;
    return -1;
}

static const char *color_value_to_name(int val)
{
    static const char *names[] = {
        "Black", "Blue", "Green", "Cyan", "Red", "Magenta", "Brown", "Gray",
        "DarkGray", "LightBlue", "LightGreen", "LightCyan", "LightRed", "LightMagenta", "Yellow", "White"
    };
    if (val >= 0 && val < 16) return names[val];
    return "Gray";
}

static char *format_color_pair(const char *fg, const char *bg)
{
    char buf[64];
    if (!fg && !bg) {
        snprintf(buf, sizeof(buf), "(default)");
    } else if (!fg) {
        snprintf(buf, sizeof(buf), "(default FG) on %s", bg);
    } else if (!bg) {
        snprintf(buf, sizeof(buf), "%s on (default BG)", fg);
    } else {
        snprintf(buf, sizeof(buf), "%s on %s", fg, bg);
    }
    return strdup(buf);
}

static void pick_menu_color(void *ctx)
{
    MenuColorActionCtx *a = (MenuColorActionCtx *)ctx;
    if (a == NULL || a->mctx == NULL || a->mctx->current_menu == NULL ||
        a->values == NULL || a->value_index < 0 || a->menu_fg == NULL || a->menu_bg == NULL) {
        return;
    }

    int cur_fg = color_name_to_value(*a->menu_fg);
    int cur_bg = color_name_to_value(*a->menu_bg);
    if (cur_fg < 0) cur_fg = 7;
    if (cur_bg < 0) cur_bg = 0;
    if (cur_bg > 7) cur_bg = 0;

    int new_fg = cur_fg;
    int new_bg = cur_bg;
    if (!colorpicker_select_full(cur_fg, cur_bg, &new_fg, &new_bg)) {
        return;
    }

    const char *fg_name = color_value_to_name(new_fg);
    const char *bg_name = color_value_to_name(new_bg);

    set_owned_string(a->menu_fg, fg_name);
    set_owned_string(a->menu_bg, bg_name);

    free(a->values[a->value_index]);
    a->values[a->value_index] = format_color_pair(*a->menu_fg, *a->menu_bg);

    if (a->mctx->options_modified) {
        *a->mctx->options_modified = true;
    }
}

static void open_menu_customization_action(void *ctx)
{
    MenuEditContext *mctx = (MenuEditContext *)ctx;
    if (mctx == NULL || mctx->current_menu == NULL) {
        return;
    }

    char *old_lb_normal_fg = mctx->current_menu->cm_lb_normal_fg ? strdup(mctx->current_menu->cm_lb_normal_fg) : NULL;
    char *old_lb_normal_bg = mctx->current_menu->cm_lb_normal_bg ? strdup(mctx->current_menu->cm_lb_normal_bg) : NULL;
    char *old_lb_selected_fg = mctx->current_menu->cm_lb_selected_fg ? strdup(mctx->current_menu->cm_lb_selected_fg) : NULL;
    char *old_lb_selected_bg = mctx->current_menu->cm_lb_selected_bg ? strdup(mctx->current_menu->cm_lb_selected_bg) : NULL;
    char *old_lb_high_fg = mctx->current_menu->cm_lb_high_fg ? strdup(mctx->current_menu->cm_lb_high_fg) : NULL;
    char *old_lb_high_bg = mctx->current_menu->cm_lb_high_bg ? strdup(mctx->current_menu->cm_lb_high_bg) : NULL;
    char *old_lb_high_sel_fg = mctx->current_menu->cm_lb_high_sel_fg ? strdup(mctx->current_menu->cm_lb_high_sel_fg) : NULL;
    char *old_lb_high_sel_bg = mctx->current_menu->cm_lb_high_sel_bg ? strdup(mctx->current_menu->cm_lb_high_sel_bg) : NULL;

    static const char *justify_opts[] = { "Left", "Center", "Right", NULL };
    static const char *boundary_justify_opts[] = {
        "Left Top", "Left Center", "Left Bottom",
        "Center Top", "Center Center", "Center Bottom",
        "Right Top", "Right Center", "Right Bottom",
        NULL
    };
    static const char *layout_opts[] = { "Grid", "Tight", "Spread", "Spread Width", "Spread Height", NULL };

    char *values[32] = { NULL };
    menu_load_customization_form(mctx->current_menu, values);

    MenuColorActionCtx normal_ctx = {
        .mctx = mctx,
        .values = values,
        .value_index = 6,
        .menu_fg = &mctx->current_menu->cm_lb_normal_fg,
        .menu_bg = &mctx->current_menu->cm_lb_normal_bg
    };
    MenuColorActionCtx selected_ctx = {
        .mctx = mctx,
        .values = values,
        .value_index = 7,
        .menu_fg = &mctx->current_menu->cm_lb_selected_fg,
        .menu_bg = &mctx->current_menu->cm_lb_selected_bg
    };
    MenuColorActionCtx high_ctx = {
        .mctx = mctx,
        .values = values,
        .value_index = 8,
        .menu_fg = &mctx->current_menu->cm_lb_high_fg,
        .menu_bg = &mctx->current_menu->cm_lb_high_bg
    };
    MenuColorActionCtx high_sel_ctx = {
        .mctx = mctx,
        .values = values,
        .value_index = 9,
        .menu_fg = &mctx->current_menu->cm_lb_high_sel_fg,
        .menu_bg = &mctx->current_menu->cm_lb_high_sel_bg
    };

    FieldDef fields[] = {
        { "CustomEnabled", "Enable customization", "Enable custom menu rendering (hybrid drawn menu +\nbounded canned options). Allows mixing ANSI art with\nMaximus-generated option lists.", FIELD_TOGGLE, 0, "No", toggle_yes_no, NULL, NULL, false, false, false, NULL, NULL },
        { "SkipCanned", "Skip canned menu", "If Yes, show menu file only (no canned options).\nUseful when your custom screen already includes the\nfull menu text.", FIELD_TOGGLE, 0, "No", toggle_yes_no, NULL, NULL, false, false, false, NULL, NULL },
        { "ShowTitle", "Show title", "Print the menu title when rendering canned options.\nIf title_location is set, prints at that position.\nOtherwise prints at current cursor.", FIELD_TOGGLE, 0, "Yes", toggle_yes_no, NULL, NULL, false, false, false, NULL, NULL },
        { "Lightbar", "Lightbar menu", "Enable arrow-key navigation (highlight bar) over the\ncanned option list. Designed for bounded NOVICE menus.", FIELD_TOGGLE, 0, "No", toggle_yes_no, NULL, NULL, false, false, false, NULL, NULL },
        { "LightbarMargin", "Lightbar margin", "Left/right margin (spaces) around each lightbar item.\nTotal width = option_width + (margin * 2). Default: 1.\nSet to 0 for no padding.", FIELD_NUMBER, 3, "1", NULL, NULL, NULL, false, false, false, NULL, NULL },

        { NULL, NULL, NULL, FIELD_SEPARATOR, 0, NULL, NULL, NULL, NULL, false, false, false, NULL, NULL },

        { "LbNormal", "Normal color", "Press ENTER or F2 to pick lightbar normal colors (foreground and background).", FIELD_ACTION, 0, "", NULL, NULL, NULL, false, false, false, pick_normal_color, &normal_ctx },
        { "LbSelected", "Selected color", "Press ENTER or F2 to pick lightbar selected colors (foreground and background).", FIELD_ACTION, 0, "", NULL, NULL, NULL, false, false, false, pick_selected_color, &selected_ctx },
        { "LbHigh", "High color", "Press ENTER or F2 to pick lightbar hotkey highlight colors (foreground and background).", FIELD_ACTION, 0, "", NULL, NULL, NULL, false, false, false, pick_high_color, &high_ctx },
        { "LbHighSel", "High+Sel color", "Press ENTER or F2 to pick lightbar high+selected colors (foreground and background).", FIELD_ACTION, 0, "", NULL, NULL, NULL, false, false, false, pick_high_sel_color, &high_sel_ctx },

        { NULL, NULL, NULL, FIELD_SEPARATOR, 0, NULL, NULL, NULL, NULL, false, false, false, NULL, NULL },

        { "TopRow", "Top row", "Top boundary row (1-based). Defines rectangle where\ncanned options print. Set both top & bottom to enable.\nExample: top=[8,8] bottom=[20,61]", FIELD_NUMBER, 5, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "TopCol", "Top col", "Top boundary column (1-based). Works with top_row to\ndefine upper-left corner of option rectangle.", FIELD_NUMBER, 5, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "BottomRow", "Bottom row", "Bottom boundary row (1-based, inclusive). Works with\nbottom_col to define lower-right corner of rectangle.", FIELD_NUMBER, 5, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "BottomCol", "Bottom col", "Bottom boundary column (1-based, inclusive). Boundary\nwidth = bottom_col - top_col + 1.", FIELD_NUMBER, 5, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "TitleRow", "Title row", "Where to print menu title (1-based). 0 = current\ncursor position. Only used if show_title=Yes.", FIELD_NUMBER, 5, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "TitleCol", "Title col", "Title column (1-based). Works with title_row.\n0 = use current cursor position.", FIELD_NUMBER, 5, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "PromptRow", "Prompt row", "Where to print NOVICE prompt (\"Select:\"). 1-based.\nPrevents \"prompt disappears\" with drawn menus.\n0 = current cursor.", FIELD_NUMBER, 5, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },
        { "PromptCol", "Prompt col", "Prompt column (1-based). Works with prompt_row.\n0 = use current cursor position.", FIELD_NUMBER, 5, "0", NULL, NULL, NULL, false, false, false, NULL, NULL },

        { NULL, NULL, NULL, FIELD_SEPARATOR, 0, NULL, NULL, NULL, NULL, false, false, false, NULL, NULL },

        { "OptionSpacing", "Option spacing", "Add extra blank line between option rows. Reduces how\nmany rows fit in boundary. Affects spread_height calc.", FIELD_TOGGLE, 0, "No", toggle_yes_no, NULL, NULL, false, false, false, NULL, NULL },
        { "OptionJustify", "Option justify", "Align option text inside fixed-width field:\nLeft (classic), Center (balanced), Right.\nAffects NOVICE display.", FIELD_SELECT, 0, "Left", justify_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "BoundaryJustify", "Boundary justify", "Where option grid sits in boundary (when boundary is\nlarger than grid). Format: \"horiz [vert]\" e.g.\n\"center top\", \"left center\", \"right bottom\".", FIELD_SELECT, 0, "Left Top", boundary_justify_opts, NULL, NULL, false, false, false, NULL, NULL },
        { "BoundaryLayout", "Boundary layout", "Column layout: Grid (fixed), Tight (last row centered),\nSpread (fill space), Spread_Width, Spread_Height.\nSpread distributes whitespace gracefully.", FIELD_SELECT, 0, "Grid", layout_opts, NULL, NULL, false, false, false, NULL, NULL },
    };

    MenuPreviewCtx preview_ctx = {
        .sys_path = mctx->sys_path,
        .menu = mctx->current_menu,
        .overlay_values = values,
        .overlay_kind = MENU_PREVIEW_OVERLAY_CUSTOMIZATION
    };
    form_set_preview_action(menu_preview_stub, &preview_ctx);

    bool saved = form_edit("Menu Customization",
                           fields,
                           (int)(sizeof(fields) / sizeof(fields[0])),
                           values,
                           NULL,
                           NULL);

    form_set_preview_action(NULL, NULL);

    bool changed = false;
    if (saved) {
        changed = menu_save_customization_form(mctx->current_menu, values);
    } else {
        set_owned_string(&mctx->current_menu->cm_lb_normal_fg, old_lb_normal_fg);
        set_owned_string(&mctx->current_menu->cm_lb_normal_bg, old_lb_normal_bg);
        set_owned_string(&mctx->current_menu->cm_lb_selected_fg, old_lb_selected_fg);
        set_owned_string(&mctx->current_menu->cm_lb_selected_bg, old_lb_selected_bg);
        set_owned_string(&mctx->current_menu->cm_lb_high_fg, old_lb_high_fg);
        set_owned_string(&mctx->current_menu->cm_lb_high_bg, old_lb_high_bg);
        set_owned_string(&mctx->current_menu->cm_lb_high_sel_fg, old_lb_high_sel_fg);
        set_owned_string(&mctx->current_menu->cm_lb_high_sel_bg, old_lb_high_sel_bg);
    }

    free(old_lb_normal_fg);
    free(old_lb_normal_bg);
    free(old_lb_selected_fg);
    free(old_lb_selected_bg);
    free(old_lb_high_fg);
    free(old_lb_high_bg);
    free(old_lb_high_sel_fg);
    free(old_lb_high_sel_bg);

    menu_free_values(values, (int)(sizeof(fields) / sizeof(fields[0])));
    if (changed && mctx->options_modified) {
        *mctx->options_modified = true;
    }
}

/* Implement the color picker actions */
static void pick_normal_color(void *ctx) { pick_menu_color(ctx); }
static void pick_selected_color(void *ctx) { pick_menu_color(ctx); }
static void pick_high_color(void *ctx) { pick_menu_color(ctx); }
static void pick_high_sel_color(void *ctx) { pick_menu_color(ctx); }

static bool edit_menu_properties(const char *sys_path, MenuDefinition **menus, int menu_count, MenuDefinition *menu)
{
    if (!menu) return false;

    bool options_modified = false;
    MenuEditContext mctx = {
        .sys_path = sys_path,
        .menus = menus,
        .menu_count = menu_count,
        .current_menu = menu,
        .options_modified = &options_modified
    };

    FieldDef fields_with_action[20];
    for (int i = 0; i < menu_properties_field_count; i++) {
        fields_with_action[i] = menu_properties_fields[i];
    }

    fields_with_action[menu_properties_field_count] = (FieldDef){
        .keyword = "MenuCustomization",
        .label = "Customize",
        .help = "Press ENTER or F2 to edit custom menu rendering options (lightbar, boundaries, colors, layout).",
        .type = FIELD_ACTION,
        .max_length = 0,
        .default_value = "",
        .toggle_options = NULL,
        .action = open_menu_customization_action,
        .action_ctx = &mctx
    };

    int customization_idx = menu_properties_field_count;

    char options_label[80];
    snprintf(options_label, sizeof(options_label), "Menu options (%d defined)", menu->option_count);

    fields_with_action[customization_idx + 1] = (FieldDef){
        .keyword = "MenuOptions",
        .label = "Menu options",
        .help = "Press ENTER or F2 to edit menu options (commands shown to users).",
        .type = FIELD_ACTION,
        .max_length = 0,
        .default_value = "",
        .toggle_options = NULL,
        .action = open_menu_options_action,
        .action_ctx = &mctx
    };

    int field_count = customization_idx + 2;

    char *values[16] = { NULL };
    menu_load_properties_form(menu, values);
    values[customization_idx] = strdup("(edit...)");
    values[customization_idx + 1] = strdup(options_label);

    MenuPreviewCtx preview_ctx = {
        .sys_path = sys_path,
        .menu = menu,
        .overlay_values = values,
        .overlay_kind = MENU_PREVIEW_OVERLAY_PROPERTIES
    };
    form_set_preview_action(menu_preview_stub, &preview_ctx);

    bool saved = form_edit(menu->name ? menu->name : "Menu Properties",
                           fields_with_action,
                           field_count,
                           values,
                           NULL,
                           NULL);

    form_set_preview_action(NULL, NULL);

    bool modified = false;
    if (saved) {
        modified = menu_save_properties_form(menu, values);
    }

    if (options_modified) {
        modified = true;
    }

    menu_free_values(values, field_count);
    return modified;
}

static const char **build_menu_name_options(MenuDefinition **menus, int menu_count)
{
    int count = 0;
    for (int i = 0; i < menu_count; i++) {
        if (menus[i] && menus[i]->name && menus[i]->name[0]) count++;
    }

    const char **opts = calloc((size_t)count + 1, sizeof(char *));
    if (!opts) return NULL;

    int idx = 0;
    for (int i = 0; i < menu_count; i++) {
        if (!menus[i] || !menus[i]->name || !menus[i]->name[0]) continue;
        opts[idx++] = menus[i]->name;
    }
    opts[idx] = NULL;
    return opts;
}

static bool menu_options_list(const char *sys_path, MenuDefinition **menus, int menu_count, MenuDefinition *menu)
{
    if (!menu) return false;

    bool menu_modified = false;

    int selected = 0;
    ListPickResult result;

    do {
        ListItem *items = calloc(menu->option_count > 0 ? (size_t)menu->option_count : 1, sizeof(ListItem));
        if (!items) {
            dialog_message("Error", "Out of memory");
            return false;
        }

        for (int i = 0; i < menu->option_count; i++) {
            MenuOption *opt = menu->options[i];
            const char *desc = (opt && opt->description) ? opt->description : "(no description)";
            const char *cmd = (opt && opt->command) ? opt->command : "";
            const char *arg = (opt && opt->arguments) ? opt->arguments : "";
            const char *priv = (opt && opt->priv_level) ? opt->priv_level : "";

            char name[256];
            if (arg[0]) {
                snprintf(name, sizeof(name), "%s -> %s %s", desc, cmd, arg);
            } else {
                snprintf(name, sizeof(name), "%s -> %s", desc, cmd);
            }

            items[i].name = strdup(name);
            items[i].extra = priv[0] ? strdup(priv) : NULL;
            items[i].enabled = true;
            items[i].data = opt;
        }

        char title[128];
        snprintf(title, sizeof(title), "Menu Options: %s", menu->name ? menu->name : "(unnamed)");

        result = listpicker_show(title, items, menu->option_count, &selected);

        if (result == LISTPICK_EDIT && selected >= 0 && selected < menu->option_count) {
            if (edit_menu_option(sys_path, menus, menu_count, menu, selected)) {
                menu_modified = true;
            }
        } else if (result == LISTPICK_INSERT) {
            MenuOption *opt = create_menu_option();
            if (!opt) {
                dialog_message("Error", "Out of memory");
            } else {
                opt->priv_level = strdup("Demoted");
                opt->description = strdup("New option");
                int insert_pos = (selected >= 0 && selected < menu->option_count) ? selected : menu->option_count;
                if (!insert_menu_option(menu, opt, insert_pos)) {
                    free_menu_option(opt);
                    dialog_message("Error", "Failed to insert option");
                } else {
                    selected = insert_pos;
                    menu_modified = true;
                    if (edit_menu_option(sys_path, menus, menu_count, menu, selected)) {
                        menu_modified = true;
                    }
                }
            }
        } else if (result == LISTPICK_DELETE && selected >= 0 && selected < menu->option_count) {
            if (dialog_confirm("Delete Option", "Delete this menu option?")) {
                (void)remove_menu_option(menu, selected);
                menu_modified = true;
                if (selected >= menu->option_count) selected = menu->option_count - 1;
                if (selected < 0) selected = 0;
            }
        }

        listitem_array_free(items, menu->option_count);
    } while (result != LISTPICK_EXIT);

    return menu_modified;
}

static bool edit_menu_option(const char *sys_path, MenuDefinition **menus, int menu_count, MenuDefinition *menu, int opt_index)
{
    (void)sys_path;
    if (!menu || opt_index < 0 || opt_index >= menu->option_count) return false;

    MenuOption *opt = menu->options[opt_index];
    if (!opt) return false;

    char *values[8] = { NULL };
    menu_load_option_form(opt, values);

    /* Build a local FieldDef array so we can provide menu-name options for Argument F2 */
    FieldDef fields_local[8];
    for (int i = 0; i < menu_option_field_count; i++) {
        fields_local[i] = menu_option_fields[i];
    }

    const char **menu_name_opts = build_menu_name_options(menus, menu_count);
    fields_local[1].toggle_options = menu_name_opts;

    int dirty_fields[8];
    int dirty_count = 0;
    bool saved = form_edit("Edit Menu Option",
                           fields_local,
                           menu_option_field_count,
                           values,
                           dirty_fields,
                           &dirty_count);

    bool modified = false;
    if (saved) {
        modified = menu_save_option_form(opt, values);
    }

    menu_free_values(values, menu_option_field_count);
    if (menu_name_opts) free(menu_name_opts);
    return modified;
}

/* Helper to show division form with given values (for edit or insert) */
static void show_division_form(const char *title, char **div_values)
{
    form_edit(title, msg_division_fields, msg_division_field_count, div_values, NULL, NULL);
}

/* Helper to initialize default division values */
static void init_default_division_values(char **div_values)
{
    div_values[0] = strdup("");           /* Name */
    div_values[1] = strdup("(None)");     /* Parent Division */
    div_values[2] = strdup("");           /* Description */
    div_values[3] = strdup("");           /* Display file */
    div_values[4] = strdup("Demoted");    /* ACS */
}

/* Forward declaration */
static void populate_division_options(void);

static void free_msg_tree_data(TreeNode *node)
{
    if (!node) return;
    if (node->type == TREENODE_DIVISION && node->data) {
        division_data_free((DivisionData *)node->data);
        node->data = NULL;
    } else if (node->type == TREENODE_AREA && node->data) {
        msgarea_data_free((MsgAreaData*)node->data);
        node->data = NULL;
    }
    for (int i = 0; i < node->child_count; i++) {
        free_msg_tree_data(node->children[i]);
    }
}

static void free_file_tree_data(TreeNode *node)
{
    if (!node) return;
    if (node->type == TREENODE_DIVISION && node->data) {
        division_data_free((DivisionData *)node->data);
        node->data = NULL;
    } else if (node->type == TREENODE_AREA && node->data) {
        filearea_data_free((FileAreaData*)node->data);
        node->data = NULL;
    }
    for (int i = 0; i < node->child_count; i++) {
        free_file_tree_data(node->children[i]);
    }
}

static int count_area_nodes_recursive(TreeNode *node)
{
    if (!node) return 0;
    int count = 0;
    if (node->type == TREENODE_AREA) count++;
    for (int i = 0; i < node->child_count; i++) {
        count += count_area_nodes_recursive(node->children[i]);
    }
    return count;
}

static int fill_area_items_recursive(TreeNode *node, ListItem *items, int idx)
{
    if (!node || !items) return idx;
    if (node->type == TREENODE_AREA) {
        items[idx].name = strdup(node->name);
        items[idx].extra = strdup(node->description ? node->description : "");
        items[idx].enabled = node->enabled;
        items[idx].data = node;
        idx++;
    }
    for (int i = 0; i < node->child_count; i++) {
        idx = fill_area_items_recursive(node->children[i], items, idx);
    }
    return idx;
}

static void update_division_levels_recursive_local(TreeNode *node, int level)
{
    if (!node) return;
    node->division_level = level;
    for (int i = 0; i < node->child_count; i++) {
        update_division_levels_recursive_local(node->children[i], level + 1);
    }
}

static bool insert_root_before(TreeNode ***roots, int *root_count, TreeNode *node, TreeNode *before)
{
    if (!roots || !root_count || !*roots || !node) return false;
    int insert_idx = *root_count;
    if (before) {
        for (int i = 0; i < *root_count; i++) {
            if ((*roots)[i] == before) {
                insert_idx = i;
                break;
            }
        }
    }
    TreeNode **new_roots = realloc(*roots, (size_t)(*root_count + 1) * sizeof(TreeNode *));
    if (!new_roots) return false;
    *roots = new_roots;
    for (int i = *root_count; i > insert_idx; i--) {
        new_roots[i] = new_roots[i - 1];
    }
    new_roots[insert_idx] = node;
    (*root_count)++;
    node->parent = NULL;
    update_division_levels_recursive_local(node, 0);
    return true;
}

static bool insert_child_before(TreeNode *parent, TreeNode *node, TreeNode *before)
{
    if (!parent || !node) return false;
    int insert_idx = parent->child_count;
    if (before && before->parent == parent) {
        for (int i = 0; i < parent->child_count; i++) {
            if (parent->children[i] == before) {
                insert_idx = i;
                break;
            }
        }
    }
    if (parent->child_count >= parent->child_capacity) {
        int new_cap = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
        TreeNode **new_children = realloc(parent->children, (size_t)new_cap * sizeof(TreeNode *));
        if (!new_children) return false;
        parent->children = new_children;
        parent->child_capacity = new_cap;
    }
    for (int i = parent->child_count; i > insert_idx; i--) {
        parent->children[i] = parent->children[i - 1];
    }
    parent->children[insert_idx] = node;
    parent->child_count++;
    node->parent = parent;
    update_division_levels_recursive_local(node, parent->division_level + 1);
    return true;
}

static void action_msg_divisions_picklist(void)
{
    const char *sys_path = current_sys_path();
    if (!sys_path || !sys_path[0]) {
        dialog_message("Error", "System path not configured.");
        return;
    }
    
    /* Load msg areas TOML */
    char err[256];
    char toml_path[MAX_PATH_LEN];
    if (!load_msg_areas_toml_for_ui(sys_path, toml_path, sizeof(toml_path), err, sizeof(err))) {
        dialog_message("Load Error", err[0] ? err : "Failed to load message areas TOML");
        return;
    }

    int root_count = 0;
    TreeNode **roots = load_msgarea_toml(g_maxcfg_toml, &root_count, err, sizeof(err));
     
    if (!roots) {
        dialog_message("Load Error", err[0] ? err : "Failed to load message areas");
        return;
    }
    
    /* Count divisions */
    int div_count = 0;
    for (int i = 0; i < root_count; i++) {
        if (roots[i]->type == TREENODE_DIVISION) div_count++;
    }
    
    if (div_count == 0) {
        dialog_message("Message Divisions", "No divisions found in message areas");
        free_msg_tree(roots, root_count);
        return;
    }
    
    /* Convert to ListItem array */
    ListItem *items = calloc(div_count, sizeof(ListItem));
    int item_idx = 0;
    for (int i = 0; i < root_count; i++) {
        if (roots[i]->type == TREENODE_DIVISION) {
            items[item_idx].name = strdup(roots[i]->name);
            items[item_idx].extra = strdup(roots[i]->description ? roots[i]->description : "");
            items[item_idx].enabled = roots[i]->enabled;
            items[item_idx].data = roots[i];
            item_idx++;
        }
    }
    
    int selected = 0;
    ListPickResult result;
    bool modified = false;
    
    do {
        result = listpicker_show("Message Divisions", items, div_count, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < div_count) {
            TreeNode *div = (TreeNode*)items[selected].data;
            if (div) {
                populate_division_options_for_context(roots, root_count, TREE_CONTEXT_MESSAGE, div);
                
                char *div_values[8] = { NULL };
                treenode_load_division_form(div, div_values);
                
                if (form_edit("Edit Message Division", msg_division_fields, msg_division_field_count, div_values, NULL, NULL)) {
                    if (treenode_save_division_form(&roots, &root_count, div, div_values, TREE_CONTEXT_MESSAGE)) {
                        free(items[selected].name);
                        free(items[selected].extra);
                        items[selected].name = strdup(div->name);
                        items[selected].extra = strdup(div->description);
                        modified = true;
                    }
                }
                
                for (int i = 0; i < 8; i++) free(div_values[i]);
            }
        }
        else if (result == LISTPICK_INSERT || result == LISTPICK_ADD) {
            /* Insert new division */
            populate_division_options_for_context(roots, root_count, TREE_CONTEXT_MESSAGE, NULL);
            
            char *div_values[8] = { NULL };
            div_values[0] = strdup("");
            div_values[1] = strdup("(None)");
            div_values[2] = strdup("");
            div_values[3] = strdup("");
            div_values[4] = strdup("Demoted");
            
            if (form_edit("New Message Division", msg_division_fields, msg_division_field_count, div_values, NULL, NULL)) {
                if (div_values[0] && div_values[0][0]) {
                    /* Create new division node */
                    TreeNode *new_div = treenode_create(div_values[0], div_values[0], div_values[2], TREENODE_DIVISION, 0);

                    TreeNode *before = NULL;
                    if (result == LISTPICK_INSERT && selected >= 0 && selected < div_count) {
                        before = (TreeNode *)items[selected].data;
                    }

                    if (!insert_root_before(&roots, &root_count, new_div, before)) {
                        free_msg_tree_data(new_div);
                        treenode_free(new_div);
                    } else {
                        
                        /* Rebuild items list */
                        for (int i = 0; i < div_count; i++) {
                            free(items[i].name);
                            free(items[i].extra);
                        }
                        free(items);
                        
                        div_count++;
                        items = calloc(div_count, sizeof(ListItem));
                        int idx = 0;
                        for (int i = 0; i < root_count; i++) {
                            if (roots[i]->type == TREENODE_DIVISION) {
                                items[idx].name = strdup(roots[i]->name);
                                items[idx].extra = strdup(roots[i]->description ? roots[i]->description : "");
                                items[idx].enabled = roots[i]->enabled;
                                items[idx].data = roots[i];
                                idx++;
                            }
                        }

                        if (result == LISTPICK_ADD) {
                            selected = div_count - 1;
                        }
                        
                        modified = true;
                    }
                }
            }
            
            for (int i = 0; i < 8; i++) free(div_values[i]);
        }
        else if (result == LISTPICK_DELETE && selected >= 0 && selected < div_count) {
            TreeNode *div = (TreeNode *)items[selected].data;
            if (div) {
                div->enabled = !div->enabled;
                items[selected].enabled = div->enabled;
                modified = true;
            }
        }
        
    } while (result != LISTPICK_EXIT);
    
    /* Save if modified */
    if (modified) {
        if (!save_msgarea_toml(g_maxcfg_toml, toml_path, roots, root_count, err, sizeof(err))) {
            dialog_message("Save Error", err[0] ? err : "Failed to save message areas TOML");
        } else {
            g_state.tree_reload_needed = true;
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < div_count; i++) {
        free(items[i].name);
        free(items[i].extra);
    }
    free(items);
    free_msg_tree(roots, root_count);
}

/* Populate the global msg_division_options array from sample_divisions */
static void populate_division_options(void)
{
    /* Build options: (None) + all division names + NULL terminator */
    /* msg_division_options is declared in fields.c */
    extern const char *msg_division_options[16];
    
    int idx = 0;
    msg_division_options[idx++] = "(None)";
    for (int i = 0; i < 3 && idx < 15; i++) {  /* 3 = sample_divisions count */
        msg_division_options[idx++] = sample_divisions[i].name;
    }
    msg_division_options[idx] = NULL;
}

/* Helper to initialize default area values */
static void init_default_area_values(char **area_values)
{
    /* Group 1: Basic info (6 fields) */
    area_values[0] = strdup("");                /* MsgArea */
    area_values[1] = strdup("(None)");          /* Division */
    area_values[2] = strdup("");                /* Tag */
    area_values[3] = strdup("");                /* Path */
    area_values[4] = strdup("");                /* Desc */
    area_values[5] = strdup("");                /* Owner */
    /* 6 = separator */
    
    /* Group 2: Format/Type (3 fields) */
    area_values[7] = strdup("Squish");          /* Style_Format */
    area_values[8] = strdup("Local");           /* Style_Type */
    area_values[9] = strdup("Real Name");       /* Style_Name */
    /* 10 = separator */
    
    /* Group 3: Style toggles (10 fields, paired) */
    area_values[11] = strdup("No");             /* Style_Pvt */
    area_values[12] = strdup("Yes");            /* Style_Pub */
    area_values[13] = strdup("No");             /* Style_HiBit */
    area_values[14] = strdup("No");             /* Style_Anon */
    area_values[15] = strdup("No");             /* Style_NoRNK */
    area_values[16] = strdup("No");             /* Style_Audit */
    area_values[17] = strdup("No");             /* Style_ReadOnly */
    area_values[18] = strdup("No");             /* Style_Hidden */
    area_values[19] = strdup("No");             /* Style_Attach */
    area_values[20] = strdup("No");             /* Style_NoMailChk */
    /* 21 = separator */
    
    /* Group 4: Renum (3 fields) */
    area_values[22] = strdup("0");              /* Renum_Max */
    area_values[23] = strdup("0");              /* Renum_Days */
    area_values[24] = strdup("0");              /* Renum_Skip */
    
    /* Group 5: Access (1 field) */
    area_values[25] = strdup("Demoted");        /* ACS */
    /* 26 = separator */
    
    /* Group 6: Origin (3 fields) */
    area_values[27] = strdup("");               /* Origin_Addr */
    area_values[28] = strdup("");               /* Origin_SeenBy */
    area_values[29] = strdup("");               /* Origin_Line */
    /* 30 = separator */
    
    /* Group 7: Advanced (5 fields) */
    area_values[31] = strdup("");               /* Barricade_Menu */
    area_values[32] = strdup("");               /* Barricade_File */
    area_values[33] = strdup("");               /* MenuName */
    area_values[34] = strdup("");               /* MenuReplace */
    area_values[35] = strdup("");               /* AttachPath */
}

/* Helper to load area values for editing */
static void load_area_values(char **area_values, int selected)
{
    /* Group 1: Basic info (6 fields) */
    area_values[0] = strdup(sample_areas[selected].name);                /* MsgArea */
    area_values[1] = strdup("(None)");                                   /* Division - TODO: get from data */
    area_values[2] = strdup(sample_areas[selected].extra ? sample_areas[selected].extra : ""); /* Tag */
    area_values[3] = strdup("spool/msgbase/area");                       /* Path */
    area_values[4] = strdup("Sample message area description");          /* Desc */
    area_values[5] = strdup("");                                         /* Owner */
    /* 6 = separator */
    
    /* Group 2: Format/Type (3 fields) */
    area_values[7] = strdup("Squish");                                   /* Style_Format */
    area_values[8] = strdup("Local");                                    /* Style_Type */
    area_values[9] = strdup("Real Name");                                /* Style_Name */
    /* 10 = separator */
    
    /* Group 3: Style toggles (10 fields, paired) */
    area_values[11] = strdup("No");                                      /* Style_Pvt */
    area_values[12] = strdup("Yes");                                     /* Style_Pub */
    area_values[13] = strdup("No");                                      /* Style_HiBit */
    area_values[14] = strdup("No");                                      /* Style_Anon */
    area_values[15] = strdup("No");                                      /* Style_NoRNK */
    area_values[16] = strdup("No");                                      /* Style_Audit */
    area_values[17] = strdup("No");                                      /* Style_ReadOnly */
    area_values[18] = strdup("No");                                      /* Style_Hidden */
    area_values[19] = strdup("No");                                      /* Style_Attach */
    area_values[20] = strdup("No");                                      /* Style_NoMailChk */
    /* 21 = separator */
    
    /* Group 4: Renum (3 fields) */
    area_values[22] = strdup("0");                                       /* Renum_Max */
    area_values[23] = strdup("0");                                       /* Renum_Days */
    area_values[24] = strdup("0");                                       /* Renum_Skip */
    
    /* Group 5: Access (1 field) */
    area_values[25] = strdup("Demoted");                                 /* ACS */
    /* 26 = separator */
    
    /* Group 6: Origin (3 fields) */
    area_values[27] = strdup("");                                        /* Origin_Addr */
    area_values[28] = strdup("");                                        /* Origin_SeenBy */
    area_values[29] = strdup("");                                        /* Origin_Line */
    /* 30 = separator */
    
    /* Group 7: Advanced (5 fields) */
    area_values[31] = strdup("");                                        /* Barricade_Menu */
    area_values[32] = strdup("");                                        /* Barricade_File */
    area_values[33] = strdup("");                                        /* MenuName */
    area_values[34] = strdup("");                                        /* MenuReplace */
    area_values[35] = strdup("");                                        /* AttachPath */
}

static void action_msg_areas_picklist(void)
{
    const char *sys_path = current_sys_path();
    if (!sys_path || !sys_path[0]) {
        dialog_message("Error", "System path not configured.");
        return;
    }
    
    /* Load msg areas TOML */
    char err[256];
    char toml_path[MAX_PATH_LEN];
    if (!load_msg_areas_toml_for_ui(sys_path, toml_path, sizeof(toml_path), err, sizeof(err))) {
        dialog_message("Load Error", err[0] ? err : "Failed to load message areas TOML");
        return;
    }

    int root_count = 0;
    TreeNode **roots = load_msgarea_toml(g_maxcfg_toml, &root_count, err, sizeof(err));
     
    if (!roots) {
        dialog_message("Load Error", err[0] ? err : "Failed to load message areas");
        return;
    }
    
    /* Flatten tree to get all areas (skip divisions) */
    int area_count = 0;
    for (int i = 0; i < root_count; i++) {
        area_count += count_area_nodes_recursive(roots[i]);
    }

    /* Convert to ListItem array */
    ListItem *items = calloc(area_count, sizeof(ListItem));
    int item_idx = 0;
    for (int i = 0; i < root_count; i++) {
        item_idx = fill_area_items_recursive(roots[i], items, item_idx);
    }
    
    int selected = 0;
    ListPickResult result;
    bool modified = false;
    
    do {
        result = listpicker_show("Message Areas", items, area_count, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < area_count) {
            TreeNode *area_node = (TreeNode*)items[selected].data;
            if (area_node) {
                populate_division_options_for_context(roots, root_count, TREE_CONTEXT_MESSAGE, NULL);
                
                char *area_values[45] = { NULL };
                treenode_load_msgarea_form(area_node, area_values);
                
                if (form_edit("Edit Message Area", msg_area_fields, msg_area_field_count, area_values, NULL, NULL)) {
                    if (treenode_save_msgarea_form(&roots, &root_count, area_node, area_values)) {
                        /* Update ListItem display */
                        free(items[selected].name);
                        free(items[selected].extra);
                        items[selected].name = strdup(area_node->name);
                        items[selected].extra = strdup(area_node->description);
                        modified = true;
                    }
                }
                
                for (int i = 0; i < 45; i++) free(area_values[i]);
            }
        }
        else if (result == LISTPICK_INSERT || result == LISTPICK_ADD) {
            /* Insert new message area */
            char *area_values[45] = { NULL };
            
            /* Initialize with defaults */
            area_values[0] = strdup("");
            area_values[1] = strdup("(None)");
            for (int i = 2; i <= 5; i++) area_values[i] = strdup("");
            area_values[7] = strdup("Squish");
            area_values[8] = strdup("Local");
            area_values[9] = strdup("Real Name");
            for (int i = 11; i <= 20; i++) area_values[i] = strdup("No");
            area_values[12] = strdup("Yes");
            for (int i = 22; i <= 24; i++) area_values[i] = strdup("0");
            area_values[25] = strdup("Demoted");
            for (int i = 27; i <= 35; i++) area_values[i] = strdup("");
            
            if (form_edit("New Message Area", msg_area_fields, msg_area_field_count, area_values, NULL, NULL)) {
                if (area_values[0] && area_values[0][0]) {
                    /* Create new area data */
                    MsgAreaData *new_area = calloc(1, sizeof(MsgAreaData));
                    new_area->name = strdup(area_values[0]);
                    new_area->tag = area_values[2][0] ? strdup(area_values[2]) : NULL;
                    new_area->path = strdup(area_values[3]);
                    new_area->desc = strdup(area_values[4]);
                    new_area->owner = area_values[5][0] ? strdup(area_values[5]) : NULL;
                    
                    /* Set style flags */
                    new_area->style = 0;
                    if (strcmp(area_values[7], "Squish") == 0) new_area->style |= MSGSTYLE_SQUISH;
                    else new_area->style |= MSGSTYLE_DOTMSG;
                    if (strcmp(area_values[8], "Local") == 0) new_area->style |= MSGSTYLE_LOCAL;
                    else if (strcmp(area_values[8], "NetMail") == 0) new_area->style |= MSGSTYLE_NET;
                    else if (strcmp(area_values[8], "EchoMail") == 0) new_area->style |= MSGSTYLE_ECHO;
                    else if (strcmp(area_values[8], "Conference") == 0) new_area->style |= MSGSTYLE_CONF;
                    if (strcmp(area_values[11], "Yes") == 0) new_area->style |= MSGSTYLE_PVT;
                    if (strcmp(area_values[12], "Yes") == 0) new_area->style |= MSGSTYLE_PUB;
                    
                    new_area->renum_max = atoi(area_values[22]);
                    new_area->acs = strdup(area_values[25]);
                    
                    /* Create tree node and add to roots */
                    TreeNode *new_node = treenode_create(new_area->name, new_area->name, new_area->desc, TREENODE_AREA, 0);
                    new_node->data = new_area;

                    TreeNode *before = NULL;
                    TreeNode *parent = NULL;
                    if (selected >= 0 && selected < area_count) {
                        TreeNode *current = (TreeNode *)items[selected].data;
                        parent = current ? current->parent : NULL;
                        if (result == LISTPICK_INSERT) before = current;
                    }

                    bool ok_attach = false;
                    if (parent) {
                        ok_attach = insert_child_before(parent, new_node, before);
                    } else {
                        ok_attach = insert_root_before(&roots, &root_count, new_node, before);
                    }

                    if (ok_attach) {
                        
                        /* Rebuild items list */
                        for (int i = 0; i < area_count; i++) {
                            free(items[i].name);
                            free(items[i].extra);
                        }
                        free(items);

                        area_count = 0;
                        for (int i = 0; i < root_count; i++) {
                            area_count += count_area_nodes_recursive(roots[i]);
                        }
                        items = calloc(area_count, sizeof(ListItem));
                        int idx = 0;
                        for (int i = 0; i < root_count; i++) {
                            idx = fill_area_items_recursive(roots[i], items, idx);
                        }

                        selected = 0;
                        for (int i = 0; i < area_count; i++) {
                            if (items[i].data == new_node) {
                                selected = i;
                                break;
                            }
                        }
                        
                        modified = true;
                    } else {
                        msgarea_data_free(new_area);
                        treenode_free(new_node);
                    }
                }
            }
            
            for (int i = 0; i < 45; i++) free(area_values[i]);
        }

        else if (result == LISTPICK_DELETE && selected >= 0 && selected < area_count) {
            TreeNode *area_node = (TreeNode*)items[selected].data;
            if (area_node) {
                area_node->enabled = !area_node->enabled;
                items[selected].enabled = area_node->enabled;
                modified = true;
            }
        }
        
    } while (result != LISTPICK_EXIT);
    
    /* Save if modified */
    if (modified) {
        if (!save_msgarea_toml(g_maxcfg_toml, toml_path, roots, root_count, err, sizeof(err))) {
            dialog_message("Save Error", err[0] ? err : "Failed to save message areas TOML");
        } else {
            g_state.tree_reload_needed = true;
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < area_count; i++) {
        free(items[i].name);
        free(items[i].extra);
    }
    free(items);
    free_msg_tree(roots, root_count);
}

/* ============================================================================
 * File Area Functions
 * ============================================================================ */

/* Sample data for demo - will be replaced with actual CTL parsing */
static ListItem sample_file_divisions[] = {
    { "Games", "Game files and patches", true, NULL },
    { "Utilities", "System utilities", true, NULL },
    { "Development", "Programming tools", true, NULL },
};

static ListItem sample_file_areas[] = {
    { "Uploads", "New uploads awaiting processing", true, NULL },
    { "DOS Games", "Classic DOS games", true, NULL },
    { "Windows Games", "Windows game files", true, NULL },
    { "Archivers", "ZIP, ARJ, RAR utilities", true, NULL },
    { "Disk Utils", "Disk management tools", true, NULL },
    { "Compilers", "C/C++/Pascal compilers", true, NULL },
    { "Editors", "Text and code editors", true, NULL },
    { "Sysop Tools", "BBS utilities", false, NULL },
};

/* Build file tree from sample_file_divisions and sample_file_areas */
static TreeNode **build_file_tree_from_samples(int *count)
{
    /* Map our sample data to a tree structure */
    TreeNode **roots = malloc(5 * sizeof(TreeNode *));
    if (!roots) {
        *count = 0;
        return NULL;
    }
    
    int idx = 0;
    
    /* Division 0: Games */
    roots[idx] = treenode_create("Games", "Games",
                                 sample_file_divisions[0].extra,
                                 TREENODE_DIVISION, 0);
    TreeNode *dos = treenode_create("DOS Games", "Games.DOS Games",
                                    "Classic DOS games",
                                    TREENODE_AREA, 1);
    treenode_add_child(roots[idx], dos);
    TreeNode *win = treenode_create("Windows Games", "Games.Windows Games",
                                    "Windows game files",
                                    TREENODE_AREA, 1);
    treenode_add_child(roots[idx], win);
    idx++;
    
    /* Division 1: Utilities */
    roots[idx] = treenode_create("Utilities", "Utilities",
                                 sample_file_divisions[1].extra,
                                 TREENODE_DIVISION, 0);
    TreeNode *arch = treenode_create("Archivers", "Utilities.Archivers",
                                     "ZIP, ARJ, RAR utilities",
                                     TREENODE_AREA, 1);
    treenode_add_child(roots[idx], arch);
    TreeNode *disk = treenode_create("Disk Utils", "Utilities.Disk Utils",
                                     "Disk management tools",
                                     TREENODE_AREA, 1);
    treenode_add_child(roots[idx], disk);
    idx++;
    
    /* Division 2: Development */
    roots[idx] = treenode_create("Development", "Development",
                                 sample_file_divisions[2].extra,
                                 TREENODE_DIVISION, 0);
    TreeNode *comp = treenode_create("Compilers", "Development.Compilers",
                                     "C/C++/Pascal compilers",
                                     TREENODE_AREA, 1);
    treenode_add_child(roots[idx], comp);
    TreeNode *edit = treenode_create("Editors", "Development.Editors",
                                     "Text and code editors",
                                     TREENODE_AREA, 1);
    treenode_add_child(roots[idx], edit);
    idx++;
    
    /* Top-level areas (no division) */
    roots[idx++] = treenode_create("Uploads", "Uploads",
                                   sample_file_areas[0].extra,
                                   TREENODE_AREA, 0);
    
    TreeNode *sysop = treenode_create("Sysop Tools", "Sysop Tools",
                                      "BBS utilities",
                                      TREENODE_AREA, 0);
    sysop->enabled = false;
    roots[idx++] = sysop;
    
    *count = idx;
    return roots;
}

static void action_file_tree_config(void)
{
    const char *sys_path = current_sys_path();
    if (!sys_path || !sys_path[0]) {
        dialog_message("Error", "System path not configured.");
        return;
    }
    
     /* Load file areas TOML */
     char err[256];
     char toml_path[MAX_PATH_LEN];
     if (!load_file_areas_toml_for_ui(sys_path, toml_path, sizeof(toml_path), err, sizeof(err))) {
         dialog_message("Load Error", err[0] ? err : "Failed to load file areas TOML");
         return;
     }

     int root_count = 0;
     TreeNode **roots = load_filearea_toml(g_maxcfg_toml, &root_count, err, sizeof(err));
    
    if (!roots) {
        dialog_message("Load Error", err[0] ? err : "Failed to load file areas");
        return;
    }
    
    /* Show tree view */
    TreeViewResult result = treeview_show("File Area Configuration", &roots, &root_count, NULL, TREE_CONTEXT_FILE);
    
    /* Save if user made changes */
    if (result == TREEVIEW_EDIT || result == TREEVIEW_INSERT) {
        if (!save_filearea_toml(g_maxcfg_toml, toml_path, roots, root_count, err, sizeof(err))) {
            dialog_message("Save Error", err[0] ? err : "Failed to save file areas TOML");
        } else {
            g_state.tree_reload_needed = true;
        }
    }
    
    /* Cleanup */
    free_file_tree(roots, root_count);
    
    /* Redraw screen */
    touchwin(stdscr);
    wnoutrefresh(stdscr);
}

/* Helper to show file division form */
static void show_file_division_form(const char *title, char **div_values)
{
    form_edit(title, file_division_fields, file_division_field_count, div_values, NULL, NULL);
}

/* Helper to initialize default file division values */
static void init_default_file_division_values(char **div_values)
{
    div_values[0] = strdup("");           /* Name */
    div_values[1] = strdup("(None)");     /* Parent Division */
    div_values[2] = strdup("");           /* Description */
    div_values[3] = strdup("");           /* Display file */
    div_values[4] = strdup("Demoted");    /* ACS */
}

/* Populate the file division options array */
static void populate_file_division_options(void)
{
    extern const char *file_division_options[16];
    
    int idx = 0;
    file_division_options[idx++] = "(None)";
    for (int i = 0; i < 3 && idx < 15; i++) {
        file_division_options[idx++] = sample_file_divisions[i].name;
    }
    file_division_options[idx] = NULL;
}

static void action_file_divisions_picklist(void)
{
    const char *sys_path = current_sys_path();
    if (!sys_path || !sys_path[0]) {
        dialog_message("Error", "System path not configured.");
        return;
    }

    /* Load file areas TOML */
    char err[256];
    char toml_path[MAX_PATH_LEN];
    if (!load_file_areas_toml_for_ui(sys_path, toml_path, sizeof(toml_path), err, sizeof(err))) {
        dialog_message("Load Error", err[0] ? err : "Failed to load file areas TOML");
        return;
    }

    int root_count = 0;
    TreeNode **roots = load_filearea_toml(g_maxcfg_toml, &root_count, err, sizeof(err));
    
    if (!roots) {
        dialog_message("Load Error", err[0] ? err : "Failed to load file areas");
        return;
    }
    
    /* Count divisions */
    int div_count = 0;
    for (int i = 0; i < root_count; i++) {
        if (roots[i]->type == TREENODE_DIVISION) div_count++;
    }
    
    if (div_count == 0) {
        dialog_message("File Divisions", "No divisions found in file areas");
        free_file_tree(roots, root_count);
        return;
    }
    
    /* Convert to ListItem array */
    ListItem *items = calloc(div_count, sizeof(ListItem));
    int item_idx = 0;
    for (int i = 0; i < root_count; i++) {
        if (roots[i]->type == TREENODE_DIVISION) {
            items[item_idx].name = strdup(roots[i]->name);
            items[item_idx].extra = strdup(roots[i]->description ? roots[i]->description : "");
            items[item_idx].enabled = roots[i]->enabled;
            items[item_idx].data = roots[i];
            item_idx++;
        }
    }
    
    int selected = 0;
    ListPickResult result;
    bool modified = false;
    
    do {
        result = listpicker_show("File Divisions", items, div_count, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < div_count) {
            TreeNode *div = (TreeNode*)items[selected].data;
            if (div) {
                populate_division_options_for_context(roots, root_count, TREE_CONTEXT_FILE, div);
                
                char *div_values[8] = { NULL };
                treenode_load_division_form(div, div_values);
                
                if (form_edit("Edit File Division", file_division_fields, file_division_field_count, div_values, NULL, NULL)) {
                    if (treenode_save_division_form(&roots, &root_count, div, div_values, TREE_CONTEXT_FILE)) {
                        free(items[selected].name);
                        free(items[selected].extra);
                        items[selected].name = strdup(div->name);
                        items[selected].extra = strdup(div->description);
                        modified = true;
                    }
                }
                
                for (int i = 0; i < 8; i++) free(div_values[i]);
            }
        }
        else if (result == LISTPICK_INSERT || result == LISTPICK_ADD) {
            /* Insert new division */
            char *div_values[8] = { NULL };
            div_values[0] = strdup("");
            div_values[1] = strdup("(None)");
            div_values[2] = strdup("");
            div_values[3] = strdup("");
            div_values[4] = strdup("Demoted");
            
            if (form_edit("New File Division", file_division_fields, file_division_field_count, div_values, NULL, NULL)) {
                if (div_values[0] && div_values[0][0]) {
                    /* Create new division node */
                    TreeNode *new_div = treenode_create(div_values[0], div_values[0], div_values[2], TREENODE_DIVISION, 0);

                    TreeNode *before = NULL;
                    if (result == LISTPICK_INSERT && selected >= 0 && selected < div_count) {
                        before = (TreeNode *)items[selected].data;
                    }

                    if (!insert_root_before(&roots, &root_count, new_div, before)) {
                        free_file_tree_data(new_div);
                        treenode_free(new_div);
                    } else {
                        
                        /* Rebuild items list */
                        for (int i = 0; i < div_count; i++) {
                            free(items[i].name);
                            free(items[i].extra);
                        }
                        free(items);
                        
                        div_count++;
                        items = calloc(div_count, sizeof(ListItem));
                        int idx = 0;
                        for (int i = 0; i < root_count; i++) {
                            if (roots[i]->type == TREENODE_DIVISION) {
                                items[idx].name = strdup(roots[i]->name);
                                items[idx].extra = strdup(roots[i]->description ? roots[i]->description : "");
                                items[idx].enabled = roots[i]->enabled;
                                items[idx].data = roots[i];
                                idx++;
                            }
                        }

                        if (result == LISTPICK_ADD) {
                            selected = div_count - 1;
                        }
                        
                        modified = true;
                    }
                }
            }
            
            for (int i = 0; i < 8; i++) free(div_values[i]);
        }
        else if (result == LISTPICK_DELETE && selected >= 0 && selected < div_count) {
            TreeNode *div = (TreeNode *)items[selected].data;
            if (div) {
                div->enabled = !div->enabled;
                items[selected].enabled = div->enabled;
                modified = true;
            }
        }
        
    } while (result != LISTPICK_EXIT);
    
    /* Save if modified */
    if (modified) {
        if (!save_filearea_toml(g_maxcfg_toml, toml_path, roots, root_count, err, sizeof(err))) {
            dialog_message("Save Error", err[0] ? err : "Failed to save file areas TOML");
        } else {
            g_state.tree_reload_needed = true;
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < div_count; i++) {
        free(items[i].name);
        free(items[i].extra);
    }
    free(items);
    free_file_tree(roots, root_count);
}

/* Helper to initialize default file area values */
static void init_default_file_area_values(char **area_values)
{
    area_values[0] = strdup("");           /* Area tag */
    area_values[1] = strdup("(None)");     /* Division */
    area_values[2] = strdup("");           /* Description */
    /* separator at 3 */
    area_values[4] = strdup("");           /* Download path */
    area_values[5] = strdup("");           /* Upload path */
    area_values[6] = strdup("");           /* FILES.BBS path */
    /* separator at 7 */
    area_values[8] = strdup("Default");    /* Date style */
    area_values[9] = strdup("No");         /* Slow */
    area_values[10] = strdup("No");        /* Staged */
    area_values[11] = strdup("No");        /* NoNew */
    area_values[12] = strdup("No");        /* Hidden */
    area_values[13] = strdup("No");        /* FreeTime */
    area_values[14] = strdup("No");        /* FreeBytes */
    area_values[15] = strdup("No");        /* NoIndex */
    /* separator at 16 */
    area_values[17] = strdup("Demoted");   /* ACS */
    /* separator at 18 */
    area_values[19] = strdup("");          /* Barricade menu */
    area_values[20] = strdup("");          /* Barricade file */
    area_values[21] = strdup("");          /* Custom menu */
    area_values[22] = strdup("");          /* Replace menu */
}

/* Helper to load file area values from sample data */
static void load_file_area_values(char **area_values, int idx)
{
    area_values[0] = strdup(sample_file_areas[idx].name);
    area_values[1] = strdup("(None)");  /* TODO: get from data */
    area_values[2] = strdup(sample_file_areas[idx].extra ? sample_file_areas[idx].extra : "");
    area_values[4] = strdup("/var/max/files");
    area_values[5] = strdup("/var/max/upload");
    area_values[6] = strdup("");
    area_values[8] = strdup("Default");
    area_values[9] = strdup("No");         /* Slow */
    area_values[10] = strdup("No");        /* Staged */
    area_values[11] = strdup("No");        /* NoNew */
    area_values[12] = strdup(sample_file_areas[idx].enabled ? "No" : "Yes");  /* Hidden */
    area_values[13] = strdup("No");        /* FreeTime */
    area_values[14] = strdup("No");        /* FreeBytes */
    area_values[15] = strdup("No");        /* NoIndex */
    area_values[17] = strdup("Demoted");
    area_values[19] = strdup("");
    area_values[20] = strdup("");
    area_values[21] = strdup("");
    area_values[22] = strdup("");
}

static void action_file_areas_picklist(void)
{
    const char *sys_path = current_sys_path();
    if (!sys_path || !sys_path[0]) {
        dialog_message("Error", "System path not configured.");
        return;
    }

    /* Load file areas TOML */
    char err[256];
    char toml_path[MAX_PATH_LEN];
    if (!load_file_areas_toml_for_ui(sys_path, toml_path, sizeof(toml_path), err, sizeof(err))) {
        dialog_message("Load Error", err[0] ? err : "Failed to load file areas TOML");
        return;
    }

    int root_count = 0;
    TreeNode **roots = load_filearea_toml(g_maxcfg_toml, &root_count, err, sizeof(err));
    
    if (!roots) {
        dialog_message("Load Error", err[0] ? err : "Failed to load file areas");
        return;
    }
    
    /* Flatten tree to get all areas (skip divisions) */
    int area_count = 0;
    for (int i = 0; i < root_count; i++) {
        area_count += count_area_nodes_recursive(roots[i]);
    }

    /* Convert to ListItem array */
    ListItem *items = calloc(area_count, sizeof(ListItem));
    int item_idx = 0;
    for (int i = 0; i < root_count; i++) {
        item_idx = fill_area_items_recursive(roots[i], items, item_idx);
    }
    
    int selected = 0;
    ListPickResult result;
    bool modified = false;
    
    do {
        result = listpicker_show("File Areas", items, area_count, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < area_count) {
            TreeNode *area_node = (TreeNode*)items[selected].data;
            if (area_node) {
                populate_division_options_for_context(roots, root_count, TREE_CONTEXT_FILE, NULL);
                
                char *area_values[25] = { NULL };
                treenode_load_filearea_form(area_node, area_values);
                
                if (form_edit("Edit File Area", file_area_fields, file_area_field_count, area_values, NULL, NULL)) {
                    if (treenode_save_filearea_form(&roots, &root_count, area_node, area_values)) {
                        /* Update list display */
                        free(items[selected].name);
                        free(items[selected].extra);
                        items[selected].name = strdup(area_node->name);
                        items[selected].extra = strdup(area_node->description);
                        modified = true;
                    }
                }
                
                for (int i = 0; i < 25; i++) free(area_values[i]);
            }
        }
        else if (result == LISTPICK_INSERT || result == LISTPICK_ADD) {
            /* Insert new file area */
            char *area_values[25] = { NULL };
            
            /* Initialize with defaults */
            area_values[0] = strdup("");
            area_values[1] = strdup("(None)");
            area_values[2] = strdup("");
            area_values[4] = strdup("");
            area_values[5] = strdup("");
            area_values[6] = strdup("");
            area_values[8] = strdup("Default");
            for (int i = 9; i <= 15; i++) area_values[i] = strdup("No");
            area_values[17] = strdup("Demoted");
            for (int i = 19; i <= 22; i++) area_values[i] = strdup("");
            
            if (form_edit("New File Area", file_area_fields, file_area_field_count, area_values, NULL, NULL)) {
                if (area_values[0] && area_values[0][0]) {
                    /* Create new area data */
                    FileAreaData *new_area = calloc(1, sizeof(FileAreaData));
                    new_area->name = strdup(area_values[0]);
                    new_area->desc = strdup(area_values[2]);
                    new_area->download = strdup(area_values[4]);
                    new_area->upload = strdup(area_values[5]);
                    new_area->filelist = area_values[6][0] ? strdup(area_values[6]) : NULL;
                    new_area->type_slow = strcmp(area_values[9], "Yes") == 0;
                    new_area->type_staged = strcmp(area_values[10], "Yes") == 0;
                    new_area->type_nonew = strcmp(area_values[11], "Yes") == 0;
                    new_area->acs = strdup(area_values[17]);
                    new_area->barricade = area_values[19][0] ? strdup(area_values[19]) : NULL;
                    new_area->menuname = area_values[21][0] ? strdup(area_values[21]) : NULL;
                    
                    /* Create tree node and add to roots */
                    TreeNode *new_node = treenode_create(new_area->name, new_area->name, new_area->desc, TREENODE_AREA, 0);
                    new_node->data = new_area;

                    TreeNode *before = NULL;
                    TreeNode *parent = NULL;
                    if (selected >= 0 && selected < area_count) {
                        TreeNode *current = (TreeNode *)items[selected].data;
                        parent = current ? current->parent : NULL;
                        if (result == LISTPICK_INSERT) before = current;
                    }

                    bool ok_attach = false;
                    if (parent) {
                        ok_attach = insert_child_before(parent, new_node, before);
                    } else {
                        ok_attach = insert_root_before(&roots, &root_count, new_node, before);
                    }

                    if (ok_attach) {
                        /* Rebuild items list */
                        for (int i = 0; i < area_count; i++) {
                            free(items[i].name);
                            free(items[i].extra);
                        }
                        free(items);

                        area_count = 0;
                        for (int i = 0; i < root_count; i++) {
                            area_count += count_area_nodes_recursive(roots[i]);
                        }
                        items = calloc(area_count, sizeof(ListItem));
                        int idx = 0;
                        for (int i = 0; i < root_count; i++) {
                            idx = fill_area_items_recursive(roots[i], items, idx);
                        }

                        selected = 0;
                        for (int i = 0; i < area_count; i++) {
                            if (items[i].data == new_node) {
                                selected = i;
                                break;
                            }
                        }

                        modified = true;
                    } else {
                        filearea_data_free(new_area);
                        treenode_free(new_node);
                    }
                }
            }
            
            for (int i = 0; i < 25; i++) free(area_values[i]);
        }

        else if (result == LISTPICK_DELETE && selected >= 0 && selected < area_count) {
            TreeNode *area_node = (TreeNode*)items[selected].data;
            if (area_node) {
                area_node->enabled = !area_node->enabled;
                items[selected].enabled = area_node->enabled;
                modified = true;
            }
        }
        
    } while (result != LISTPICK_EXIT);
    
    /* Save if modified */
    if (modified) {
        if (!save_filearea_toml(g_maxcfg_toml, toml_path, roots, root_count, err, sizeof(err))) {
            dialog_message("Save Error", err[0] ? err : "Failed to save file areas TOML");
        } else {
            g_state.tree_reload_needed = true;
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < area_count; i++) {
        free(items[i].name);
        free(items[i].extra);
    }
    free(items);
    free_file_tree(roots, root_count);
}

/* ============================================================================
 * Security/Access Levels Functions
 * ============================================================================ */

/* Sample data for demo - will be replaced with actual CTL parsing */
static ListItem sample_access_levels[] = {
    { "Transient",  "Level 0 - Twit/Banned users", true, NULL },
    { "Demoted",    "Level 10 - Restricted access", true, NULL },
    { "Limited",    "Level 20 - Limited user", true, NULL },
    { "Normal",     "Level 30 - Standard user", true, NULL },
    { "Worthy",     "Level 40 - Trusted user", true, NULL },
    { "Privil",     "Level 50 - Privileged user", true, NULL },
    { "Favored",    "Level 60 - Favored user", true, NULL },
    { "Extra",      "Level 70 - Extra privileges", true, NULL },
    { "AsstSysop",  "Level 80 - Assistant Sysop", true, NULL },
    { "Sysop",      "Level 100 - System Operator", true, NULL },
    { "Hidden",     "Level 65535 - Hidden/Internal", false, NULL },
};

#define NUM_SAMPLE_ACCESS_LEVELS 11

/* Helper to initialize default access level values */
static void init_default_access_values(char **values)
{
    values[0] = strdup("");           /* Access name */
    values[1] = strdup("0");          /* Level */
    values[2] = strdup("");           /* Description */
    values[3] = strdup("");           /* Alias */
    values[4] = strdup("");           /* Key */
    /* separator at 5 */
    values[6] = strdup("60");         /* Session time */
    values[7] = strdup("90");         /* Daily time */
    values[8] = strdup("-1");         /* Daily calls */
    /* separator at 9 */
    values[10] = strdup("5000");      /* Download limit */
    values[11] = strdup("0");         /* File ratio */
    values[12] = strdup("1000");      /* Ratio-free */
    values[13] = strdup("100");       /* Upload reward */
    /* separator at 14 */
    values[15] = strdup("300");       /* Logon baud */
    values[16] = strdup("300");       /* Xfer baud */
    /* separator at 17 */
    values[18] = strdup("");          /* Login file */
    /* separator at 19 */
    values[20] = strdup("");          /* Flags */
    values[21] = strdup("");          /* Mail flags */
    values[22] = strdup("0");         /* User flags */
    /* separator at 23 */
    values[24] = strdup("0");         /* Oldpriv */
}

/* Helper to load access level values from sample data */
static void load_access_level_values(char **values, int idx)
{
    /* Sample level numbers matching access.ctl */
    static const int level_numbers[] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 100, 65535 };
    static const int oldpriv_values[] = { -2, 0, 1, 2, 3, 4, 5, 6, 7, 10, 11 };
    char buf[16];
    
    values[0] = strdup(sample_access_levels[idx].name);
    snprintf(buf, sizeof(buf), "%d", level_numbers[idx]);
    values[1] = strdup(buf);
    values[2] = strdup(sample_access_levels[idx].extra ? sample_access_levels[idx].extra : "");
    values[3] = strdup("");           /* Alias */
    values[4] = strdup("");           /* Key - will use first letter */
    values[6] = strdup("60");
    values[7] = strdup("90");
    values[8] = strdup("-1");
    values[10] = strdup("5000");
    values[11] = strdup("0");
    values[12] = strdup("1000");
    values[13] = strdup("100");
    values[15] = strdup("300");
    values[16] = strdup("300");
    values[18] = strdup("");
    values[20] = strdup(idx >= 9 ? "NoLimits" : "");  /* Sysop/Hidden get NoLimits */
    values[21] = strdup(idx >= 9 ? "ShowPvt MsgAttrAny" : "");
    values[22] = strdup("0");
    snprintf(buf, sizeof(buf), "%d", oldpriv_values[idx]);
    values[24] = strdup(buf);
}

static void action_security_levels(void)
{
    int selected = 0;
    ListPickResult result;
    
    do {
        result = listpicker_show("Security Levels", sample_access_levels, 
                                 NUM_SAMPLE_ACCESS_LEVELS, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < NUM_SAMPLE_ACCESS_LEVELS) {
            char *values[30] = { NULL };
            load_access_level_values(values, selected);
            
            form_edit("Edit Access Level", access_level_fields, 
                      access_level_field_count, values, NULL, NULL);
            
            /* TODO: Save changes when CTL support is added */
            
            for (int i = 0; i < 30; i++) free(values[i]);
        }
        else if (result == LISTPICK_INSERT || result == LISTPICK_ADD) {
            char *values[30] = { NULL };
            init_default_access_values(values);
            
            form_edit("New Access Level", access_level_fields,
                      access_level_field_count, values, NULL, NULL);
            
            /* TODO: Insert into data structure when CTL support is added */
            
            for (int i = 0; i < 30; i++) free(values[i]);
        }
        else if (result == LISTPICK_DELETE && selected >= 0 && selected < NUM_SAMPLE_ACCESS_LEVELS) {
            /* Toggle enabled state - actual deletion would need confirmation */
            sample_access_levels[selected].enabled = !sample_access_levels[selected].enabled;
        }
        
    } while (result != LISTPICK_EXIT);
}

void menubar_init(void)
{
    /* Calculate menu positions */
    int x = 2;  /* Start with some padding */
    
    for (size_t i = 0; i < NUM_TOP_MENUS; i++) {
        menu_positions[i] = x;
        x += strlen(top_menus[i].label) + 3;  /* label + spacing */
    }
}

void draw_menubar(void)
{
    attron(COLOR_PAIR(CP_MENU_BAR));
    
    /* Clear the menu bar line with black background */
    move(MENUBAR_ROW, 0);
    for (int i = 0; i < COLS; i++) {
        addch(' ');
    }
    
    /* Draw each menu item */
    for (size_t i = 0; i < NUM_TOP_MENUS; i++) {
        const char *label = top_menus[i].label;
        int x = menu_positions[i];
        
        if ((int)i == g_state.current_menu) {
            /* Highlighted: bold yellow on blue for entire item */
            attron(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
            mvprintw(MENUBAR_ROW, x, " %s ", label);
            attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
        } else {
            /* Normal: bold yellow hotkey, grey rest */
            move(MENUBAR_ROW, x + 1);
            
            /* First char is hotkey - bold yellow */
            attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
            addch(label[0]);
            attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
            
            /* Rest of label - grey */
            attron(COLOR_PAIR(CP_MENU_BAR));
            printw("%s", label + 1);
            attroff(COLOR_PAIR(CP_MENU_BAR));
        }
    }
    
    wnoutrefresh(stdscr);
}

bool menubar_handle_key(int ch)
{
    switch (ch) {
        case KEY_LEFT:
            if (g_state.current_menu > 0) {
                g_state.current_menu--;
                if (dropdown_is_open()) {
                    dropdown_open(g_state.current_menu);
                }
            }
            return true;
            
        case KEY_RIGHT:
            if (g_state.current_menu < (int)NUM_TOP_MENUS - 1) {
                g_state.current_menu++;
                if (dropdown_is_open()) {
                    dropdown_open(g_state.current_menu);
                }
            }
            return true;
            
        case KEY_DOWN:
        case '\n':
        case '\r':
            dropdown_open(g_state.current_menu);
            return true;
            
        default:
            /* Check for hotkey (first letter of menu) */
            for (size_t i = 0; i < NUM_TOP_MENUS; i++) {
                if (toupper(ch) == toupper(top_menus[i].label[0])) {
                    g_state.current_menu = i;
                    dropdown_open(g_state.current_menu);
                    return true;
                }
            }
            break;
    }
    
    return false;
}

int menubar_get_current(void)
{
    return g_state.current_menu;
}

void menubar_set_current(int index)
{
    if (index >= 0 && index < (int)NUM_TOP_MENUS) {
        g_state.current_menu = index;
    }
}

/* Get top menu data (used by dropdown) */
TopMenu* menubar_get_menu(int index)
{
    if (index >= 0 && index < (int)NUM_TOP_MENUS) {
        return &top_menus[index];
    }
    return NULL;
}

int menubar_get_position(int index)
{
    if (index >= 0 && index < (int)NUM_TOP_MENUS) {
        return menu_positions[index];
    }
    return 0;
}

/* ============================================================================
 * Export Actions
 * ============================================================================ */

static const FieldDef import_legacy_fields[] = {
    {
        NULL,
        "Config output directory",
        "Directory where next-gen TOML config files will be written.",
        FIELD_TEXT,
        255,
        "",
        NULL,
        NULL,
        NULL,
        false,
        false,
        false,
        NULL,
        NULL,
    },
};

static void action_export_nextgen_config(void)
{
    const char *sys_path = current_sys_path();
    if (!sys_path || !sys_path[0]) {
        dialog_message("Error", "System path not configured.");
        return;
    }

    char maxctl_path[512];
    if (snprintf(maxctl_path, sizeof(maxctl_path), "%s/etc/max.ctl", sys_path) >= (int)sizeof(maxctl_path)) {
        dialog_message("Error", "Path too long.");
        return;
    }

    if (!path_exists(maxctl_path)) {
        dialog_message("Import Legacy Config (CTL)", "Legacy CTL file not found: <sys_path>/etc/max.ctl");
        return;
    }

    char default_config_dir[512];
    if (snprintf(default_config_dir, sizeof(default_config_dir), "%s/config", sys_path) >= (int)sizeof(default_config_dir)) {
        dialog_message("Error", "Default export path too long.");
        return;
    }

    char *values[1] = { NULL };
    values[0] = strdup(default_config_dir);
    if (values[0] == NULL) {
        dialog_message("Error", "Out of memory.");
        return;
    }

    int dirty_fields[4];
    int dirty_count = 0;
    bool saved = form_edit("Import Legacy Config (CTL)",
                           import_legacy_fields,
                           1,
                           values,
                           dirty_fields,
                           &dirty_count);

    if (!saved) {
        free(values[0]);
        return;
    }

    char config_dir[512];
    if (values[0] == NULL || values[0][0] == '\0') {
        free(values[0]);
        dialog_message("Import Legacy Config (CTL)", "Config output directory is required.");
        return;
    }

    if (snprintf(config_dir, sizeof(config_dir), "%s", values[0]) >= (int)sizeof(config_dir)) {
        free(values[0]);
        dialog_message("Error", "Export path too long.");
        return;
    }
    free(values[0]);

    if (!dialog_confirm("Import Legacy Config (CTL)",
                        "This will overwrite your current configuration, are you sure")) {
        return;
    }

    char err[512];
    err[0] = '\0';

    if (nextgen_export_config_from_maxctl(maxctl_path, config_dir, NG_EXPORT_ALL, err, sizeof(err))) {
        dialog_message("Import Complete", "Legacy configuration imported successfully.");
    } else {
        dialog_message("Import Failed", err[0] ? err : "Failed to import legacy configuration.");
    }
}

/* ============================================================================
 * Convert Legacy Language (MAD → TOML)
 * ============================================================================ */

static const FieldDef convert_lang_fields[] = {
    {
        NULL,
        "Language file directory",
        "Directory containing .MAD language files to convert.",
        FIELD_TEXT,
        255,
        "",
        NULL,
        NULL,
        NULL,
        false,
        false,
        false,
        NULL,
        NULL,
    },
};

static void action_convert_legacy_lang(void)
{
    const char *sys_path = current_sys_path();
    if (!sys_path || !sys_path[0]) {
        dialog_message("Error", "System path not configured.");
        return;
    }

    char default_lang_dir[512];
    if (snprintf(default_lang_dir, sizeof(default_lang_dir),
                 "%s/etc/lang", sys_path) >= (int)sizeof(default_lang_dir)) {
        dialog_message("Error", "Default language path too long.");
        return;
    }

    char *values[1] = { NULL };
    values[0] = strdup(default_lang_dir);
    if (values[0] == NULL) {
        dialog_message("Error", "Out of memory.");
        return;
    }

    int dirty_fields[4];
    int dirty_count = 0;
    bool saved = form_edit("Convert Legacy Language (MAD)",
                           convert_lang_fields,
                           1,
                           values,
                           dirty_fields,
                           &dirty_count);

    if (!saved) {
        free(values[0]);
        return;
    }

    if (values[0] == NULL || values[0][0] == '\0') {
        free(values[0]);
        dialog_message("Convert Language", "Language directory is required.");
        return;
    }

    char lang_dir[512];
    if (snprintf(lang_dir, sizeof(lang_dir), "%s", values[0]) >= (int)sizeof(lang_dir)) {
        free(values[0]);
        dialog_message("Error", "Language path too long.");
        return;
    }
    free(values[0]);

    if (!dialog_confirm("Convert Legacy Language (MAD)",
                        "Convert all .MAD files in this directory to TOML?")) {
        return;
    }

    char err[512];
    err[0] = '\0';
    int count = lang_convert_all_mad(lang_dir, NULL, err, sizeof(err));
    if (count < 0) {
        dialog_message("Conversion Failed",
                       err[0] ? err : "Failed to convert language files.");
    } else if (count == 0) {
        dialog_message("Convert Language", "No .MAD files found in the specified directory.");
    } else {
        char msg[256];
        snprintf(msg, sizeof(msg), "Successfully converted %d .MAD file(s) to TOML.", count);
        if (err[0]) {
            /* Append warning about partial failures */
            size_t mlen = strlen(msg);
            snprintf(msg + mlen, sizeof(msg) - mlen, "\n\nWarning: %s", err);
        }
        dialog_message("Conversion Complete", msg);
    }
}

static void action_lang_editor(void)
{
    action_browse_lang_strings(NULL);
}

static void action_menus_list(void)
{
    const char *sys_path = current_sys_path();
    if (!sys_path || !sys_path[0]) {
        dialog_message("Error", "System path not configured.");
        return;
    }

    /*
     * Menu editing writes per-menu TOML directly via save_menu_toml().
     *
     * g_state.dirty is used elsewhere to mean "there are pending MaxCfgToml
     * overrides to persist". The generic form editor will set g_state.dirty
     * for any changed form, but menus are not saved through the override system.
     *
     * Snapshot and restore the override-dirty flag so editing/saving menus does
     * not cause an extra save prompt on application exit or trigger a different
     * TOML serialization path.
     */
    const bool dirty_before = g_state.dirty;

    /* Load menus from TOML */
    char err[256];
    int menu_count = 0;
    MenuDefinition **menus = NULL;
    char **menu_paths = NULL;
    char **menu_prefixes = NULL;

    if (g_maxcfg_toml == NULL) {
        dialog_message("Error", "TOML configuration is not loaded.");
        return;
    }

    if (!load_menus_toml(g_maxcfg_toml, sys_path, &menus, &menu_paths, &menu_prefixes, &menu_count, err, sizeof(err))) {
        dialog_message("Load Error", err[0] ? err : "Failed to load menus from TOML");
        return;
    }

    if (menu_count <= 0 || menus == NULL) {
        dialog_message("Menu Configuration", "No menus found in config/menus");
        free(menus);
        free(menu_paths);
        free(menu_prefixes);
        return;
    }
    
    /* Build list items */
    ListItem *items = calloc(menu_count, sizeof(ListItem));
    if (!items) {
        free_menu_definitions(menus, menu_count);
        for (int i = 0; i < menu_count; i++) {
            free(menu_paths[i]);
            free(menu_prefixes[i]);
        }
        free(menu_paths);
        free(menu_prefixes);
        dialog_message("Error", "Out of memory");
        return;
    }
    
    for (int i = 0; i < menu_count; i++) {
        MenuDefinition *menu = menus[i];
        
        /* Format: "MAIN - MAIN (%t mins) [12 options]" */
        char display[256];
        snprintf(display, sizeof(display), "%s - %s [%d option%s]",
                 menu->name ? menu->name : "(unnamed)",
                 menu->title ? menu->title : "(no title)",
                 menu->option_count,
                 menu->option_count == 1 ? "" : "s");
        
        items[i].name = strdup(display);
        items[i].extra = strdup(menu->name ? menu->name : "");
        items[i].enabled = true;
        items[i].data = menu;
    }
    
    /* Show picklist */
    ListPickResult result;
    int selected = 0;
    
    bool menus_modified = false;
    do {
        result = listpicker_show("Menu Configuration", items, menu_count, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < menu_count) {
            MenuDefinition *menu = menus[selected];
            if (edit_menu_properties(sys_path, menus, menu_count, menu)) {
                menus_modified = true;
            }

            /* Refresh selected row display after edits */
            free(items[selected].name);
            free(items[selected].extra);
            {
                char display[256];
                snprintf(display, sizeof(display), "%s - %s [%d option%s]",
                         menu->name ? menu->name : "(unnamed)",
                         menu->title ? menu->title : "(no title)",
                         menu->option_count,
                         menu->option_count == 1 ? "" : "s");
                items[selected].name = strdup(display);
                items[selected].extra = strdup(menu->name ? menu->name : "");
            }
        }
        else if (result == LISTPICK_INSERT) {
            /* TODO: Add new menu */
            dialog_message("Not Implemented", "Adding menus will be implemented next.");
        }
        else if (result == LISTPICK_DELETE && selected >= 0 && selected < menu_count) {
            /* TODO: Delete menu */
            dialog_message("Not Implemented", "Deleting menus will be implemented next.");
        }
        
    } while (result != LISTPICK_EXIT);
    
    /* Save TOML menus if any changes were made */
    if (menus_modified) {
        for (int i = 0; i < menu_count; i++) {
            if (menu_paths && menu_prefixes && menus && menus[i]) {
                if (!save_menu_toml(g_maxcfg_toml, menu_paths[i], menu_prefixes[i], menus[i], err, sizeof(err))) {
                    dialog_message("Save Error", err[0] ? err : "Failed to save menu TOML");
                    break;
                }
            }
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < menu_count; i++) {
        free(items[i].name);
        free(items[i].extra);
    }
    free(items);
    free_menu_definitions(menus, menu_count);
    for (int i = 0; i < menu_count; i++) {
        free(menu_paths[i]);
        free(menu_prefixes[i]);
    }
    free(menu_paths);
    free(menu_prefixes);

    /* Restore override-dirty state (see note at top of function). */
    g_state.dirty = dirty_before;
    
    /* Redraw screen */
    touchwin(stdscr);
    wnoutrefresh(stdscr);
}
