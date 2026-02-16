/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * colorform.c - Color editing form for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <spawn.h>
#include <limits.h>
#include <sys/wait.h>
#include "maxcfg.h"
#include "ui.h"

extern char **environ;

/* Color field definition */
typedef struct {
    const char *label;          /* Display label */
    const char *define_name;    /* colors.lh define name */
    const char *help;           /* Help text */
    int current_fg;             /* Current foreground (0-15) */
    int current_bg;             /* Current background (0-7) */
} ColorFieldDef;

/* Menu colors */
static ColorFieldDef menu_colors[] = {
    { "Menu name",        "COL_MNU_NAME",   "Color for menu item names", 14, 0 },
    { "Menu highlight",   "COL_MNU_HILITE", "Color for highlighted menu items", 14, 0 },
    { "Menu option",      "COL_MNU_OPTION", "Color for menu option text", 7, 0 },
};
#define NUM_MENU_COLORS (sizeof(menu_colors) / sizeof(menu_colors[0]))

/* File area colors */
static ColorFieldDef file_colors[] = {
    { "File name",        "COL_FILE_NAME",  "Color for file names in listings", 14, 0 },
    { "File size",        "COL_FILE_SIZE",  "Color for file sizes", 5, 0 },
    { "File date",        "COL_FILE_DATE",  "Color for file dates", 2, 0 },
    { "File description", "COL_FILE_DESC",  "Color for file descriptions", 3, 0 },
    { "File search match","COL_FILE_FIND",  "Color for search match highlights", 14, 0 },
    { "Offline file",     "COL_FILE_OFFLN", "Color for offline files", 4, 0 },
    { "New file",         "COL_FILE_NEW",   "Color for new files (with blink)", 3, 0 },
};
#define NUM_FILE_COLORS (sizeof(file_colors) / sizeof(file_colors[0]))

/* Message reader colors */
static ColorFieldDef msg_colors[] = {
    { "From label",       "COL_MSG_FROM",    "Color for 'From:' label", 3, 0 },
    { "From text",        "COL_MSG_FROMTXT", "Color for sender name", 14, 0 },
    { "To label",         "COL_MSG_TO",      "Color for 'To:' label", 3, 0 },
    { "To text",          "COL_MSG_TOTXT",   "Color for recipient name", 14, 0 },
    { "Subject label",    "COL_MSG_SUBJ",    "Color for 'Subject:' label", 3, 0 },
    { "Subject text",     "COL_MSG_SUBJTXT", "Color for subject text", 14, 0 },
    { "Attributes",       "COL_MSG_ATTR",    "Color for message attributes", 10, 0 },
    { "Date",             "COL_MSG_DATE",    "Color for message date", 10, 0 },
    { "Address",          "COL_MSG_ADDR",    "Color for network address", 3, 0 },
    { "Locus",            "COL_MSG_LOCUS",   "Color for message locus", 9, 0 },
    { "Message body",     "COL_MSG_BODY",    "Color for message body text", 3, 0 },
    { "Quoted text",      "COL_MSG_QUOTE",   "Color for quoted text", 7, 0 },
    { "Kludge lines",     "COL_MSG_KLUDGE",  "Color for kludge/control lines", 13, 0 },
};
#define NUM_MSG_COLORS (sizeof(msg_colors) / sizeof(msg_colors[0]))

/* Full screen reader colors (these have backgrounds!) */
static ColorFieldDef fsr_colors[] = {
    { "Message number",   "COL_FSR_MSGNUM",  "Color for message number display", 12, 1 },
    { "Links",            "COL_FSR_LINKS",   "Color for reply chain links", 14, 1 },
    { "Attributes",       "COL_FSR_ATTRIB",  "Color for message attributes", 14, 1 },
    { "Message info",     "COL_FSR_MSGINFO", "Color for message info line", 14, 1 },
    { "Date",             "COL_FSR_DATE",    "Color for date display", 15, 1 },
    { "Address",          "COL_FSR_ADDR",    "Color for network addresses", 14, 1 },
    { "Static text",      "COL_FSR_STATIC",  "Color for static labels", 15, 1 },
    { "Border",           "COL_FSR_BORDER",  "Color for window borders", 11, 1 },
    { "Locus",            "COL_FSR_LOCUS",   "Color for locus display", 15, 0 },
};
#define NUM_FSR_COLORS (sizeof(fsr_colors) / sizeof(fsr_colors[0]))

static bool run_cmd_silent(const char *path, char *const argv[])
{
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);

    int nullfd = open("/dev/null", O_WRONLY);
    if (nullfd >= 0) {
        posix_spawn_file_actions_adddup2(&fa, nullfd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&fa, nullfd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&fa, nullfd);
    }

    pid_t pid;
    int rc = posix_spawn(&pid, path, &fa, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&fa);
    if (rc != 0) {
        return false;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return false;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status) == 0;
    }

    return false;
}

static bool build_config_dir(char *out, size_t out_sz)
{
    const char *slash = strrchr(g_state.config_path, '/');
    if (!slash) {
        return snprintf(out, out_sz, ".") < (int)out_sz;
    }

    size_t dir_len = (size_t)(slash - g_state.config_path);
    if (dir_len == 0) {
        return snprintf(out, out_sz, "/") < (int)out_sz;
    }

    char dir[MAX_PATH_LEN];
    if (dir_len >= sizeof(dir)) {
        return false;
    }
    memcpy(dir, g_state.config_path, dir_len);
    dir[dir_len] = '\0';

    return snprintf(out, out_sz, "%s", dir) < (int)out_sz;
}

static bool build_bbs_root_dir(char *out, size_t out_sz)
{
    /* Get sys_path from TOML configuration */
    extern MaxCfgToml *g_maxcfg_toml;
    if (g_maxcfg_toml) {
        MaxCfgVar v;
        if (maxcfg_toml_get(g_maxcfg_toml, "maximus.sys_path", &v) == MAXCFG_OK &&
            v.type == MAXCFG_VAR_STRING && v.v.s && v.v.s[0]) {
            /* Trim trailing slash to make path joins predictable */
            size_t n = strlen(v.v.s);
            while (n > 1 && v.v.s[n - 1] == '/') n--;
            if (n < out_sz) {
                memcpy(out, v.v.s, n);
                out[n] = '\0';
                return true;
            }
        }
    }

    char config_dir[MAX_PATH_LEN];
    if (!build_config_dir(config_dir, sizeof(config_dir))) {
        return false;
    }

    if (strcmp(config_dir, ".") == 0) {
        return snprintf(out, out_sz, ".") < (int)out_sz;
    }

    const char *slash = strrchr(config_dir, '/');
    if (!slash) {
        return snprintf(out, out_sz, ".") < (int)out_sz;
    }

    size_t dir_len = (size_t)(slash - config_dir);
    if (dir_len == 0) {
        return snprintf(out, out_sz, "/") < (int)out_sz;
    }

    if (dir_len >= out_sz) {
        return false;
    }

    memcpy(out, config_dir, dir_len);
    out[dir_len] = '\0';
    return true;
}

static bool build_config_basename(char *out, size_t out_sz)
{
    const char *slash = strrchr(g_state.config_path, '/');
    const char *base = slash ? slash + 1 : g_state.config_path;
    if (!base || !*base) {
        return false;
    }
    return snprintf(out, out_sz, "%s", base) < (int)out_sz;
}

/**
 * @brief Resolve lang_path from TOML, joining against sys_path if relative.
 * @return true if out was written successfully.
 */
static bool resolve_lang_path(char *out, size_t out_sz)
{
    extern MaxCfgToml *g_maxcfg_toml;
    if (!g_maxcfg_toml) return false;

    MaxCfgVar v;
    const char *lang_rel = NULL;
    if (maxcfg_toml_get(g_maxcfg_toml, "maximus.lang_path", &v) == MAXCFG_OK &&
        v.type == MAXCFG_VAR_STRING && v.v.s && v.v.s[0]) {
        lang_rel = v.v.s;
    }

    /* If lang_path is absolute, use directly */
    if (lang_rel && (lang_rel[0] == '/' || lang_rel[0] == '\\'))
        return snprintf(out, out_sz, "%s", lang_rel) < (int)out_sz;

    /* Resolve relative lang_path against sys_path */
    char root[MAX_PATH_LEN];
    if (!build_bbs_root_dir(root, sizeof(root))) return false;

    if (lang_rel)
        return snprintf(out, out_sz, "%s/%s", root, lang_rel) < (int)out_sz;

    /* Last resort: config_path/lang */
    const char *cfg_rel = NULL;
    if (maxcfg_toml_get(g_maxcfg_toml, "maximus.config_path", &v) == MAXCFG_OK &&
        v.type == MAXCFG_VAR_STRING && v.v.s && v.v.s[0])
        cfg_rel = v.v.s;

    return snprintf(out, out_sz, "%s/%s/lang", root,
                    cfg_rel ? cfg_rel : "config") < (int)out_sz;
}

static bool build_colors_lh_path(char *out, size_t out_sz)
{
    char lang_dir[MAX_PATH_LEN];
    if (!resolve_lang_path(lang_dir, sizeof(lang_dir))) return false;
    return snprintf(out, out_sz, "%s/colors.lh", lang_dir) < (int)out_sz;
}

static bool build_lang_dir(char *out, size_t out_sz)
{
    return resolve_lang_path(out, out_sz);
}

static bool build_maid_path(char *out, size_t out_sz)
{
    char root[MAX_PATH_LEN];
    if (!build_bbs_root_dir(root, sizeof(root))) {
        return false;
    }

    if (strcmp(root, ".") != 0 && strcmp(root, "/") != 0) {
        return snprintf(out, out_sz, "%s/bin/maid", root) < (int)out_sz;
    }

    if (strcmp(root, "/") == 0) {
        return snprintf(out, out_sz, "/bin/maid") < (int)out_sz;
    }

    if (strcmp(root, ".") == 0) {
        return snprintf(out, out_sz, "bin/maid") < (int)out_sz;
    }

    return snprintf(out, out_sz, "%s/bin/maid", root) < (int)out_sz;
}

static bool build_silt_path(char *out, size_t out_sz)
{
    char root[MAX_PATH_LEN];
    if (!build_bbs_root_dir(root, sizeof(root))) {
        return false;
    }

    if (strcmp(root, ".") != 0 && strcmp(root, "/") != 0) {
        return snprintf(out, out_sz, "%s/bin/silt", root) < (int)out_sz;
    }

    if (strcmp(root, "/") == 0) {
        return snprintf(out, out_sz, "/bin/silt") < (int)out_sz;
    }

    if (strcmp(root, ".") == 0) {
        return snprintf(out, out_sz, "bin/silt") < (int)out_sz;
    }

    return snprintf(out, out_sz, "%s/bin/silt", root) < (int)out_sz;
}

static bool colorslh_find_field(const char *define_name, ColorFieldDef **out_fields, int *out_index)
{
    ColorFieldDef *groups[] = {
        menu_colors,
        file_colors,
        msg_colors,
        fsr_colors
    };
    int group_counts[] = {
        (int)NUM_MENU_COLORS,
        (int)NUM_FILE_COLORS,
        (int)NUM_MSG_COLORS,
        (int)NUM_FSR_COLORS
    };

    for (int g = 0; g < 4; g++) {
        for (int i = 0; i < group_counts[g]; i++) {
            if (groups[g][i].define_name && strcmp(groups[g][i].define_name, define_name) == 0) {
                if (out_fields) *out_fields = groups[g];
                if (out_index) *out_index = i;
                return true;
            }
        }
    }
    return false;
}

static bool parse_attr_from_define_line(const char *line, unsigned *out_attr)
{
    const char *p = strstr(line, "\\x16\\x01\\x");
    if (!p) return false;
    p += strlen("\\x16\\x01\\x");

    if (!isxdigit((unsigned char)p[0]) || !isxdigit((unsigned char)p[1])) {
        return false;
    }

    char hex[3];
    hex[0] = p[0];
    hex[1] = p[1];
    hex[2] = '\0';

    char *end = NULL;
    unsigned v = (unsigned)strtoul(hex, &end, 16);
    if (!end || *end != '\0') {
        return false;
    }

    *out_attr = v & 0xFFu;
    return true;
}

static void colorslh_load_into_fields(void)
{
    char path[MAX_PATH_LEN];
    if (!build_colors_lh_path(path, sizeof(path))) {
        return;
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "#define ", 8) != 0) {
            continue;
        }

        char name[96];
        if (sscanf(line, "#define %95s", name) != 1) {
            continue;
        }

        ColorFieldDef *fields = NULL;
        int idx = -1;
        if (!colorslh_find_field(name, &fields, &idx)) {
            continue;
        }

        unsigned attr;
        if (!parse_attr_from_define_line(line, &attr)) {
            continue;
        }

        fields[idx].current_fg = (int)(attr & 0x0Fu);
        fields[idx].current_bg = (int)((attr >> 4) & 0x07u);
    }

    fclose(fp);
}

static bool colorslh_write_from_fields(void)
{
    char path[MAX_PATH_LEN];
    if (!build_colors_lh_path(path, sizeof(path))) {
        return false;
    }

    char tmp_path[MAX_PATH_LEN + 8];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE *in = fopen(path, "r");
    if (!in) {
        return false;
    }
    FILE *out = fopen(tmp_path, "w");
    if (!out) {
        fclose(in);
        return false;
    }

    char line[512];
    while (fgets(line, sizeof(line), in)) {
        if (strncmp(line, "#define ", 8) == 0) {
            char name[96];
            if (sscanf(line, "#define %95s", name) == 1) {
                ColorFieldDef *fields = NULL;
                int idx = -1;
                if (colorslh_find_field(name, &fields, &idx)) {
                    unsigned orig_attr;
                    if (parse_attr_from_define_line(line, &orig_attr)) {
                        unsigned new_attr = (orig_attr & 0x80u) |
                                            (((unsigned)(fields[idx].current_bg & 0x07) << 4) & 0x70u) |
                                            ((unsigned)(fields[idx].current_fg & 0x0F));

                        char *p = strstr(line, "\\x16\\x01\\x");
                        if (p) {
                            p += (int)strlen("\\x16\\x01\\x");
                            static const char hex[] = "0123456789abcdef";
                            p[0] = hex[(new_attr >> 4) & 0x0F];
                            p[1] = hex[new_attr & 0x0F];
                        }
                    }
                }
            }
        }

        fputs(line, out);
    }

    fclose(in);
    fclose(out);

    if (rename(tmp_path, path) != 0) {
        unlink(tmp_path);
        return false;
    }

    return true;
}

static bool rebuild_after_colors(char *debug, size_t debug_sz)
{
    if (debug && debug_sz > 0) debug[0] = '\0';

    char lang_dir_rel[MAX_PATH_LEN];
    if (!build_lang_dir(lang_dir_rel, sizeof(lang_dir_rel))) {
        return false;
    }

    char maid_path_rel[MAX_PATH_LEN];
    if (!build_maid_path(maid_path_rel, sizeof(maid_path_rel))) {
        return false;
    }

    char silt_path_rel[MAX_PATH_LEN];
    if (!build_silt_path(silt_path_rel, sizeof(silt_path_rel))) {
        return false;
    }

    char base[MAX_PATH_LEN];
    if (!build_config_basename(base, sizeof(base))) {
        return false;
    }

    char root[MAX_PATH_LEN];
    if (!build_bbs_root_dir(root, sizeof(root))) {
        return false;
    }

    char oldcwd[MAX_PATH_LEN];
    if (!getcwd(oldcwd, sizeof(oldcwd))) {
        return false;
    }

    char lang_dir[PATH_MAX];
    if (!realpath(lang_dir_rel, lang_dir)) {
        if (debug && debug_sz > 0) {
            snprintf(debug, debug_sz, "lang_dir=%s\nmaid=%s\nsilt=%s\nerror=realpath(lang_dir) failed (%s)",
                     lang_dir_rel, maid_path_rel, silt_path_rel, strerror(errno));
        }
        return false;
    }

    char maid_path[PATH_MAX];
    if (!realpath(maid_path_rel, maid_path)) {
        if (debug && debug_sz > 0) {
            snprintf(debug, debug_sz, "lang_dir=%s\nmaid=%s\nsilt=%s\nerror=realpath(maid) failed (%s)",
                     lang_dir, maid_path_rel, silt_path_rel, strerror(errno));
        }
        return false;
    }

    char silt_path[PATH_MAX];
    if (!realpath(silt_path_rel, silt_path)) {
        if (debug && debug_sz > 0) {
            snprintf(debug, debug_sz, "lang_dir=%s\nmaid=%s\nsilt=%s\nerror=realpath(silt) failed (%s)",
                     lang_dir, maid_path, silt_path_rel, strerror(errno));
        }
        return false;
    }

    char root_abs[PATH_MAX];
    if (!realpath(root, root_abs)) {
        if (debug && debug_sz > 0) {
            snprintf(debug, debug_sz, "root=%s\nlang_dir=%s\nmaid=%s\nsilt=%s\nerror=realpath(root) failed (%s)",
                     root, lang_dir, maid_path, silt_path, strerror(errno));
        }
        return false;
    }

    bool ok_maid_p = false;
    bool ok_silt = false;
    bool ok_maid_link = false;

    if (chdir(lang_dir) != 0) {
        if (debug && debug_sz > 0) {
            snprintf(debug, debug_sz, "lang_dir=%s\nmaid=%s\nsilt=%s\nerror=chdir(lang_dir) failed (%s)",
                     lang_dir, maid_path, silt_path, strerror(errno));
        }
        return false;
    }

    char *argv1[] = { (char *)maid_path, "english", "-p", NULL };
    ok_maid_p = run_cmd_silent(maid_path, argv1);

    if (chdir(root_abs) != 0) {
        if (debug && debug_sz > 0) {
            snprintf(debug, debug_sz, "root=%s\nerror=chdir(root) failed (%s)", root_abs, strerror(errno));
        }
        (void)chdir(oldcwd);
        return false;
    }

    char *argv2[] = { (char *)silt_path, g_state.config_path, "-x", NULL };
    ok_silt = ok_maid_p ? run_cmd_silent(silt_path, argv2) : false;

    if (chdir(lang_dir) != 0) {
        if (debug && debug_sz > 0) {
            snprintf(debug, debug_sz, "lang_dir=%s\nerror=chdir(lang_dir) failed (%s)", lang_dir, strerror(errno));
        }
        (void)chdir(oldcwd);
        return false;
    }

    char prm_arg[128];
    snprintf(prm_arg, sizeof(prm_arg), "-p../%s", base);
    char *argv3[] = { (char *)maid_path, "english", "-d", "-s", prm_arg, NULL };
    ok_maid_link = ok_silt ? run_cmd_silent(maid_path, argv3) : false;

    if (debug && debug_sz > 0) {
        snprintf(debug, debug_sz,
                 "cwd=%s\nroot=%s\nlang_dir=%s\nmaid=%s\nsilt=%s\nconfig=%s\nprm_arg=%s\nmaid_p=%s\nsilt=%s\nmaid_link=%s",
                 oldcwd, root_abs, lang_dir, maid_path, silt_path, g_state.config_path, prm_arg,
                 ok_maid_p ? "ok" : "fail", ok_silt ? "ok" : "fail", ok_maid_link ? "ok" : "fail");
    }

    (void)chdir(oldcwd);
    return ok_maid_p && ok_silt && ok_maid_link;
}

/* Color pair base for preview */
#define CP_PREVIEW_BASE 50

/* Form geometry */
typedef struct {
    int win_x, win_y;
    int win_w, win_h;
    int help_y, help_h;
    int field_x, field_y;
    int label_w, value_w;
    int max_visible;        /* Max fields that fit in window */
} ColorFormGeometry;

#define MAX_VISIBLE_FIELDS 10  /* Max fields before scrolling */

static ColorFormGeometry calc_color_geometry(const char *title, int field_count)
{
    ColorFormGeometry g;
    
    g.label_w = 18;
    g.value_w = 22;  /* "Light Magenta on Blue" format */
    
    int content_w = g.label_w + 2 + g.value_w;
    int title_len = title ? strlen(title) : 0;
    if (title_len + 4 > content_w) content_w = title_len + 4;
    
    /* Cap visible fields */
    g.max_visible = field_count;
    if (g.max_visible > MAX_VISIBLE_FIELDS) {
        g.max_visible = MAX_VISIBLE_FIELDS;
    }
    
    g.win_w = content_w + 6;
    g.win_h = g.max_visible + 9;  /* visible fields + help + borders */
    
    if (g.win_w > COLS - 4) g.win_w = COLS - 4;
    if (g.win_h > LINES - 4) g.win_h = LINES - 4;
    
    g.win_x = (COLS - g.win_w) / 2;
    g.win_y = (LINES - g.win_h) / 2;
    
    g.field_x = g.win_x + 2;
    g.field_y = g.win_y + 2;
    g.help_y = g.win_y + g.win_h - 5;
    g.help_h = 3;
    
    return g;
}

static void draw_color_window(const ColorFormGeometry *g, const char *title)
{
    int x = g->win_x;
    int y = g->win_y;
    int w = g->win_w;
    int h = g->win_h;
    
    /* Draw border */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    
    mvaddch(y, x, ACS_ULCORNER);
    addch(ACS_HLINE);
    addch(' ');
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("%s", title);
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(' ');
    for (int i = strlen(title) + 4; i < w - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_URCORNER);
    
    for (int i = 1; i < h - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + w - 1, ACS_VLINE);
    }
    
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    for (int i = 1; i < w - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_LRCORNER);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Fill interior */
    attron(COLOR_PAIR(CP_FORM_BG));
    for (int i = 1; i < h - 1; i++) {
        mvhline(y + i, x + 1, ' ', w - 2);
    }
    attroff(COLOR_PAIR(CP_FORM_BG));
}

static void draw_color_help_separator(const ColorFormGeometry *g)
{
    int y = g->help_y;
    int x = g->win_x;
    int w = g->win_w;
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    mvaddch(y, x, ACS_LTEE);
    addch(ACS_HLINE);
    addch(' ');
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("Help");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    printw(" ");
    addch(ACS_HLINE);
    printw(" ");
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("F2");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=Pick Color");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    /* Fill rest with line */
    int cur_x = getcurx(stdscr);
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    printw(" ");
    for (int i = cur_x + 2; i < x + w - 1; i++) {
        addch(ACS_HLINE);
    }
    mvaddch(y, x + w - 1, ACS_RTEE);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
}

static void draw_color_field(const ColorFormGeometry *g, int idx, ColorFieldDef *field, bool selected)
{
    int y = g->field_y + idx;
    int label_x = g->field_x;
    int value_x = g->field_x + g->label_w + 2;
    
    /* Draw label */
    if (selected) {
        attron(COLOR_PAIR(CP_MENU_BAR) | A_BOLD);
    } else {
        attron(COLOR_PAIR(CP_MENU_BAR));
    }
    mvprintw(y, label_x, "%*s", g->label_w, field->label);
    attroff(COLOR_PAIR(CP_MENU_BAR) | A_BOLD);
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    printw(": ");
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Create preview color pair */
    int pair_num = CP_PREVIEW_BASE + idx;
    int fg_ncurses[] = { COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
                         COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE,
                         COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
                         COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE };
    int bg_ncurses[] = { COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
                         COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE };
    
    init_pair(pair_num, fg_ncurses[field->current_fg], bg_ncurses[field->current_bg]);
    
    /* Draw value with preview - show fg on bg if background is non-black */
    char value_str[40];
    if (field->current_bg > 0) {
        snprintf(value_str, sizeof(value_str), "%s on %s", 
                 color_get_name(field->current_fg),
                 color_get_name(field->current_bg));
    } else {
        snprintf(value_str, sizeof(value_str), "%s", 
                 color_get_name(field->current_fg));
    }
    
    if (selected) {
        attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
        mvprintw(y, value_x, " %-16s", value_str);
        attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
    } else {
        /* Show color in its actual color */
        attron(COLOR_PAIR(pair_num));
        if (field->current_fg >= 8) attron(A_BOLD);
        mvprintw(y, value_x, " %-16s", value_str);
        if (field->current_fg >= 8) attroff(A_BOLD);
        attroff(COLOR_PAIR(pair_num));
    }
}

/* Edit a color category */
static bool colorform_edit(const char *title, ColorFieldDef *fields, int field_count)
{
    int selected = 0;
    int scroll_offset = 0;
    bool dirty = false;
    bool done = false;
    bool saved = false;
    int ch;
    
    ColorFormGeometry g = calc_color_geometry(title, field_count);
    
    while (!done) {
        /* Adjust scroll offset to keep selection visible */
        if (selected < scroll_offset) {
            scroll_offset = selected;
        } else if (selected >= scroll_offset + g.max_visible) {
            scroll_offset = selected - g.max_visible + 1;
        }
        
        draw_work_area();
        draw_color_window(&g, title);
        draw_color_help_separator(&g);
        
        /* Draw visible fields */
        for (int i = 0; i < g.max_visible && (scroll_offset + i) < field_count; i++) {
            int field_idx = scroll_offset + i;
            draw_color_field(&g, i, &fields[field_idx], field_idx == selected);
        }
        
        /* Draw scroll indicators if needed */
        if (field_count > g.max_visible) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            if (scroll_offset > 0) {
                mvprintw(g.field_y - 1, g.win_x + g.win_w - 4, "^^^");
            }
            if (scroll_offset + g.max_visible < field_count) {
                mvprintw(g.field_y + g.max_visible, g.win_x + g.win_w - 4, "vvv");
            }
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        
        /* Draw help text */
        attron(COLOR_PAIR(CP_MENU_BAR));
        mvprintw(g.help_y + 1, g.win_x + 2, "%-*.*s", g.win_w - 4, g.win_w - 4, fields[selected].help);
        attroff(COLOR_PAIR(CP_MENU_BAR));
        
        draw_status_bar("ESC=Abort  F10=Save/Exit  F2/Enter=Pick Color");
        
        refresh();
        
        ch = getch();
        
        switch (ch) {
            case KEY_UP:
                if (selected > 0) selected--;
                break;
                
            case KEY_DOWN:
                if (selected < field_count - 1) selected++;
                break;
                
            case KEY_HOME:
                selected = 0;
                break;
                
            case KEY_END:
                selected = field_count - 1;
                break;
                
            case KEY_PPAGE:  /* Page Up */
                selected -= g.max_visible;
                if (selected < 0) selected = 0;
                break;
                
            case KEY_NPAGE:  /* Page Down */
                selected += g.max_visible;
                if (selected >= field_count) selected = field_count - 1;
                break;
                
            case '\n':
            case '\r':
            case KEY_F(2):
                {
                    /* Open full color grid picker */
                    int new_fg, new_bg;
                    if (colorpicker_select_full(fields[selected].current_fg,
                                                fields[selected].current_bg,
                                                &new_fg, &new_bg)) {
                        fields[selected].current_fg = new_fg;
                        fields[selected].current_bg = new_bg;
                        dirty = true;
                    }
                }
                break;
                
            case KEY_F(10):
                saved = true;
                done = true;
                break;
                
            case 27:
                if (dirty) {
                    if (dialog_confirm("Abort Changes", "Abort changes without saving?")) {
                        done = true;
                    }
                } else {
                    done = true;
                }
                break;
        }
    }
    
    if (saved) {
        g_state.dirty = true;
    }
    
    return saved;
}

/* ============================================================================
 * Theme color editor — edits |xx semantic color slots as MCI pipe strings
 * ============================================================================ */

/**
 * @brief Save current g_theme_colors to TOML via the override system.
 * @return true on success.
 */
static bool theme_colors_save(void)
{
    extern MaxCfgToml *g_maxcfg_toml;
    extern MaxCfgThemeColors g_theme_colors;
    if (!g_maxcfg_toml) return false;

    for (int i = 0; i < MCI_THEME_SLOT_COUNT; i++) {
        char path[128];
        snprintf(path, sizeof(path), "colors.theme.colors.%s", g_theme_colors.slots[i].key);
        if (maxcfg_toml_override_set_string(g_maxcfg_toml, path,
                                             g_theme_colors.slots[i].value) != MAXCFG_OK)
            return false;
    }
    return true;
}

/**
 * @brief Edit theme color slots in a scrollable form.
 *
 * Each slot shows its description and current MCI pipe string.
 * Enter/F2 opens the color picker; the chosen fg/bg is written as |NN codes.
 * @return true if the user saved changes.
 */
static bool themeform_edit(void)
{
    extern MaxCfgThemeColors g_theme_colors;
    int selected = 0;
    int scroll_offset = 0;
    bool dirty = false;
    bool done = false;
    bool saved = false;

    const char *title = "Theme Colors";
    int field_count = MCI_THEME_SLOT_COUNT;
    int max_visible = field_count;
    if (max_visible > MAX_VISIBLE_FIELDS) max_visible = MAX_VISIBLE_FIELDS;

    int label_w = 28;
    int value_w = 16;
    int content_w = label_w + 2 + value_w;
    int title_len = (int)strlen(title);
    if (title_len + 4 > content_w) content_w = title_len + 4;

    int win_w = content_w + 6;
    int win_h = max_visible + 9;
    if (win_w > COLS - 4) win_w = COLS - 4;
    if (win_h > LINES - 4) win_h = LINES - 4;

    int win_x = (COLS - win_w) / 2;
    int win_y = (LINES - win_h) / 2;
    int field_x = win_x + 2;
    int field_y = win_y + 2;
    int help_y  = win_y + win_h - 5;

    while (!done) {
        if (selected < scroll_offset) scroll_offset = selected;
        else if (selected >= scroll_offset + max_visible)
            scroll_offset = selected - max_visible + 1;

        draw_work_area();

        /* --- Window chrome --- */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvaddch(win_y, win_x, ACS_ULCORNER);
        addch(ACS_HLINE); addch(' ');
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("%s", title);
        attroff(COLOR_PAIR(CP_MENU_BAR));
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        addch(' ');
        for (int i = title_len + 4; i < win_w - 1; i++) addch(ACS_HLINE);
        addch(ACS_URCORNER);
        for (int i = 1; i < win_h - 1; i++) {
            mvaddch(win_y + i, win_x, ACS_VLINE);
            mvaddch(win_y + i, win_x + win_w - 1, ACS_VLINE);
        }
        mvaddch(win_y + win_h - 1, win_x, ACS_LLCORNER);
        for (int i = 1; i < win_w - 1; i++) addch(ACS_HLINE);
        addch(ACS_LRCORNER);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        attron(COLOR_PAIR(CP_FORM_BG));
        for (int i = 1; i < win_h - 1; i++) mvhline(win_y + i, win_x + 1, ' ', win_w - 2);
        attroff(COLOR_PAIR(CP_FORM_BG));

        /* --- Help separator --- */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvaddch(help_y, win_x, ACS_LTEE); addch(ACS_HLINE); addch(' ');
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("Help");
        attroff(COLOR_PAIR(CP_MENU_BAR));
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        addch(' ');
        for (int c = getcurx(stdscr) + 1; c < win_x + win_w - 1; c++) addch(ACS_HLINE);
        mvaddch(help_y, win_x + win_w - 1, ACS_RTEE);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));

        /* --- Draw visible slots --- */
        for (int i = 0; i < max_visible && (scroll_offset + i) < field_count; i++) {
            int idx = scroll_offset + i;
            MaxCfgThemeSlot *slot = &g_theme_colors.slots[idx];
            int y = field_y + i;

            /* Label: "|xx  Description" */
            if (idx == selected) attron(COLOR_PAIR(CP_MENU_BAR) | A_BOLD);
            else                 attron(COLOR_PAIR(CP_MENU_BAR));
            mvprintw(y, field_x, "|%s  %-*s", slot->code, label_w - 5, slot->desc);
            attroff(COLOR_PAIR(CP_MENU_BAR) | A_BOLD);

            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            printw(": ");
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));

            /* Value — show the raw MCI string with a color swatch */
            if (idx == selected) {
                attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
                mvprintw(y, field_x + label_w + 2, " %-*s", value_w - 1, slot->value);
                attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
            } else {
                attron(COLOR_PAIR(CP_FORM_VALUE));
                mvprintw(y, field_x + label_w + 2, " %-*s", value_w - 1, slot->value);
                attroff(COLOR_PAIR(CP_FORM_VALUE));
            }
        }

        /* Scroll indicators */
        if (field_count > max_visible) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            if (scroll_offset > 0)
                mvprintw(field_y - 1, win_x + win_w - 4, "^^^");
            if (scroll_offset + max_visible < field_count)
                mvprintw(field_y + max_visible, win_x + win_w - 4, "vvv");
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }

        /* Help text */
        attron(COLOR_PAIR(CP_MENU_BAR));
        mvprintw(help_y + 1, win_x + 2, "%-*.*s", win_w - 4, win_w - 4,
                 g_theme_colors.slots[selected].desc);
        attroff(COLOR_PAIR(CP_MENU_BAR));

        draw_status_bar("ESC=Abort  F10=Save/Exit  F2/Enter=Pick Color");
        refresh();

        int ch = getch();
        switch (ch) {
        case KEY_UP:    if (selected > 0) selected--; break;
        case KEY_DOWN:  if (selected < field_count - 1) selected++; break;
        case KEY_HOME:  selected = 0; break;
        case KEY_END:   selected = field_count - 1; break;
        case KEY_PPAGE: selected -= max_visible; if (selected < 0) selected = 0; break;
        case KEY_NPAGE: selected += max_visible; if (selected >= field_count) selected = field_count - 1; break;
        case '\n': case '\r': case KEY_F(2): {
            /* Parse current value to seed the picker */
            int cur_fg = 7, cur_bg = 0;
            const char *v = g_theme_colors.slots[selected].value;
            /* Simple parse: first |NN sets fg, second sets bg */
            for (const char *s = v; *s; s++) {
                if (s[0] == '|' && isdigit((unsigned char)s[1]) && isdigit((unsigned char)s[2])) {
                    int code = (s[1] - '0') * 10 + (s[2] - '0');
                    if (code <= 15) cur_fg = code;
                    else if (code <= 23) cur_bg = code - 16;
                    s += 2;
                }
            }
            int new_fg, new_bg;
            if (colorpicker_select_full(cur_fg, cur_bg, &new_fg, &new_bg)) {
                MaxCfgNgColor c = { .fg = new_fg, .bg = new_bg, .blink = false };
                maxcfg_ng_color_to_mci(&c, g_theme_colors.slots[selected].value,
                                       sizeof(g_theme_colors.slots[selected].value));
                dirty = true;
            }
            break;
        }
        case KEY_F(10):
            saved = true;
            done = true;
            break;
        case 27:
            if (dirty) {
                if (dialog_confirm("Abort Changes", "Abort changes without saving?"))
                    done = true;
            } else {
                done = true;
            }
            break;
        }
    }

    if (saved) {
        theme_colors_save();
        g_state.dirty = true;
    }

    return saved;
}

/* Action for Default Colors menu item - shows category picker */
void action_default_colors(void)
{
    colorslh_load_into_fields();

    const char *categories[] = {
        "Menu Colors",
        "File Colors", 
        "Message Colors",
        "Reader Colors",
        "Theme Colors"
    };
    int num_categories = 5;
    int selected = 0;
    int ch;
    bool done = false;
    
    int width = 22;
    int height = 9;
    int x = (COLS - width) / 2;
    int y = (LINES - height) / 2;
    
    while (!done) {
        /* Draw border */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvaddch(y, x, ACS_ULCORNER);
        addch(ACS_HLINE);
        addch(' ');
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        attron(COLOR_PAIR(CP_MENU_BAR));
        printw("Default Colors");
        attroff(COLOR_PAIR(CP_MENU_BAR));
        
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        addch(' ');
        for (int i = 18; i < width - 1; i++) addch(ACS_HLINE);
        addch(ACS_URCORNER);
        
        for (int i = 1; i < height - 1; i++) {
            mvaddch(y + i, x, ACS_VLINE);
            attron(COLOR_PAIR(CP_FORM_BG));
            for (int j = 1; j < width - 1; j++) addch(' ');
            attroff(COLOR_PAIR(CP_FORM_BG));
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(y + i, x + width - 1, ACS_VLINE);
        }
        
        mvaddch(y + height - 1, x, ACS_LLCORNER);
        for (int i = 1; i < width - 1; i++) addch(ACS_HLINE);
        addch(ACS_LRCORNER);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        
        /* Draw options */
        for (int i = 0; i < num_categories; i++) {
            if (i == selected) {
                attron(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
            } else {
                attron(COLOR_PAIR(CP_MENU_BAR));
            }
            mvprintw(y + 2 + i, x + 2, " %-16s ", categories[i]);
            if (i == selected) {
                attroff(COLOR_PAIR(CP_DROPDOWN_HIGHLIGHT) | A_BOLD);
            } else {
                attroff(COLOR_PAIR(CP_MENU_BAR));
            }
        }
        
        refresh();
        ch = getch();
        
        switch (ch) {
            case KEY_UP:
                if (selected > 0) selected--;
                break;
            case KEY_DOWN:
                if (selected < num_categories - 1) selected++;
                break;
            case '\n':
            case '\r':
                done = true;
                break;
            case 27:
                return;  /* Cancel */
        }
    }
    
    /* Open selected category */
    switch (selected) {
        case 0:
            if (colorform_edit("Menu Colors", menu_colors, NUM_MENU_COLORS)) {
                if (!colorslh_write_from_fields()) {
                    dialog_message("Save Failed", "Failed to write colors.lh");
                    break;
                }
                if (dialog_confirm("Rebuild", "Changes won't show until you rebuild (MAID + SILT). Rebuild now?")) {
                    draw_status_bar("Rebuilding (MAID + SILT)...");
                    doupdate();
                    char debug[512];
                    if (!rebuild_after_colors(debug, sizeof(debug))) {
                        dialog_message("Rebuild Failed", debug[0] ? debug : "Rebuild failed.");
                    }
                    draw_status_bar("F1=Help  ESC=Menu  Ctrl+Q=Quit");
                }
            }
            break;
        case 1:
            if (colorform_edit("File Area Colors", file_colors, NUM_FILE_COLORS)) {
                if (!colorslh_write_from_fields()) {
                    dialog_message("Save Failed", "Failed to write colors.lh");
                    break;
                }
                if (dialog_confirm("Rebuild", "Changes won't show until you rebuild (MAID + SILT). Rebuild now?")) {
                    draw_status_bar("Rebuilding (MAID + SILT)...");
                    doupdate();
                    char debug[512];
                    if (!rebuild_after_colors(debug, sizeof(debug))) {
                        dialog_message("Rebuild Failed", debug[0] ? debug : "Rebuild failed.");
                    }
                    draw_status_bar("F1=Help  ESC=Menu  Ctrl+Q=Quit");
                }
            }
            break;
        case 2:
            if (colorform_edit("Message Colors", msg_colors, NUM_MSG_COLORS)) {
                if (!colorslh_write_from_fields()) {
                    dialog_message("Save Failed", "Failed to write colors.lh");
                    break;
                }
                if (dialog_confirm("Rebuild", "Changes won't show until you rebuild (MAID + SILT). Rebuild now?")) {
                    draw_status_bar("Rebuilding (MAID + SILT)...");
                    doupdate();
                    char debug[512];
                    if (!rebuild_after_colors(debug, sizeof(debug))) {
                        dialog_message("Rebuild Failed", debug[0] ? debug : "Rebuild failed.");
                    }
                    draw_status_bar("F1=Help  ESC=Menu  Ctrl+Q=Quit");
                }
            }
            break;
        case 3:
            if (colorform_edit("Full Screen Reader Colors", fsr_colors, NUM_FSR_COLORS)) {
                if (!colorslh_write_from_fields()) {
                    dialog_message("Save Failed", "Failed to write colors.lh");
                    break;
                }
                if (dialog_confirm("Rebuild", "Changes won't show until you rebuild (MAID + SILT). Rebuild now?")) {
                    draw_status_bar("Rebuilding (MAID + SILT)...");
                    doupdate();
                    char debug[512];
                    if (!rebuild_after_colors(debug, sizeof(debug))) {
                        dialog_message("Rebuild Failed", debug[0] ? debug : "Rebuild failed.");
                    }
                    draw_status_bar("F1=Help  ESC=Menu  Ctrl+Q=Quit");
                }
            }
            break;
        case 4:
            themeform_edit();
            break;
    }
}

/* Action for File Colors */
void action_file_colors(void)
{
    colorform_edit("File Area Colors", file_colors, NUM_FILE_COLORS);
}

/* Action for Message Colors */
void action_msg_colors(void)
{
    colorform_edit("Message Colors", msg_colors, NUM_MSG_COLORS);
}

/* Action for Full Screen Reader Colors */
void action_fsr_colors(void)
{
    colorform_edit("Full Screen Reader Colors", fsr_colors, NUM_FSR_COLORS);
}
