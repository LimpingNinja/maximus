/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * menu_parse.c - Menu configuration parser
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include "menu_data.h"
#include "ctl_parse.h"
#include "libmaxcfg.h"

static bool kw_value(const char *line, const char *kw, const char **out_value)
{
    if (line == NULL || kw == NULL) {
        return false;
    }

    size_t n = strlen(kw);
    if (strncasecmp(line, kw, n) != 0) {
        return false;
    }

    const char *p = line + n;
    if (*p != '\0' && !isspace((unsigned char)*p)) {
        return false;
    }

    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }

    if (out_value) {
        *out_value = p;
    }
    return true;
}

/* Helper to trim whitespace */
static char *trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

/* Helper to duplicate a string or return NULL */
static char *safe_strdup(const char *s) {
    return s ? strdup(s) : NULL;
}

static bool strv_add(char ***items_io, size_t *count_io, const char *s)
{
    if (items_io == NULL || count_io == NULL) {
        return false;
    }

    char *copy = strdup(s ? s : "");
    if (copy == NULL) {
        return false;
    }

    char **p = realloc(*items_io, (*count_io + 1u) * sizeof(char *));
    if (p == NULL) {
        free(copy);
        return false;
    }

    *items_io = p;
    (*items_io)[*count_io] = copy;
    (*count_io)++;
    return true;
}

static void strv_free(char ***items_io, size_t *count_io)
{
    if (items_io == NULL || *items_io == NULL) {
        return;
    }

    if (count_io) {
        for (size_t i = 0; i < *count_io; i++) {
            free((*items_io)[i]);
        }
        *count_io = 0;
    }

    free(*items_io);
    *items_io = NULL;
}

static void set_err(char *err, size_t err_len, const char *fmt, ...)
{
    if (!err || err_len == 0) return;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(err, err_len, fmt ? fmt : "", ap);
    va_end(ap);
}

/* DOS color name to value mapping */
static const char *dos_color_name(int color_val)
{
    static const char *names[] = {
        "Black", "Blue", "Green", "Cyan", "Red", "Magenta", "Brown", "Gray",
        "DarkGray", "LightBlue", "LightGreen", "LightCyan", "LightRed", "LightMagenta", "Yellow", "White"
    };
    if (color_val >= 0 && color_val < 16) {
        return names[color_val];
    }
    return "Black";
}

/* DOS color name to value */
static int dos_color_value(const char *name)
{
    if (!name) return 0;
    
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
    
    return 0;
}

/* Extract foreground and background from DOS attribute byte */
static void attr_to_colors(unsigned char attr, const char **fg, const char **bg)
{
    int fg_val = attr & 0x0F;
    int bg_val = (attr >> 4) & 0x0F;
    
    if (fg) *fg = dos_color_name(fg_val);
    if (bg) *bg = dos_color_name(bg_val);
}

/* Create DOS attribute byte from foreground and background color names */
static unsigned char colors_to_attr(const char *fg, const char *bg)
{
    int fg_val = fg ? dos_color_value(fg) : 7;
    int bg_val = bg ? dos_color_value(bg) : 0;
    
    return (unsigned char)((fg_val & 0x0F) | ((bg_val & 0x0F) << 4));
}

/* Menu type context for flags conversion helpers. */
#define MENU_TYPE_HEADER 0
#define MENU_TYPE_FOOTER 1
#define MENU_TYPE_BODY   2

static bool menu_types_from_flags(word flags, int kind, char ***out_types, size_t *out_count)
{
    *out_types = NULL;
    *out_count = 0;

    word all = (kind == MENU_TYPE_HEADER) ? MFLAG_HF_ALL :
               (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_ALL : MFLAG_MF_ALL;
    if (flags == 0 || flags == all) {
        return true;
    }

    if ((flags & ((kind == MENU_TYPE_HEADER) ? MFLAG_HF_NOVICE :
                  (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_NOVICE : MFLAG_MF_NOVICE)) != 0u) { if (!strv_add(out_types, out_count, "Novice")) return false; }
    if ((flags & ((kind == MENU_TYPE_HEADER) ? MFLAG_HF_REGULAR :
                  (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_REGULAR : MFLAG_MF_REGULAR)) != 0u) { if (!strv_add(out_types, out_count, "Regular")) return false; }
    if ((flags & ((kind == MENU_TYPE_HEADER) ? MFLAG_HF_EXPERT :
                  (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_EXPERT : MFLAG_MF_EXPERT)) != 0u) { if (!strv_add(out_types, out_count, "Expert")) return false; }
    if (kind != MENU_TYPE_FOOTER &&
        (flags & ((kind == MENU_TYPE_HEADER) ? MFLAG_HF_RIP : MFLAG_MF_RIP)) != 0u) { if (!strv_add(out_types, out_count, "RIP")) return false; }

    return true;
}

static bool menu_flags_from_types(char **types, size_t count, int kind, word *out_flags)
{
    if (out_flags == NULL) {
        return false;
    }

    word flags = 0;
    for (size_t i = 0; i < count; i++) {
        const char *t = types[i];
        if (t == NULL || t[0] == '\0') continue;

        if (strcasecmp(t, "Novice") == 0)
            flags |= (kind == MENU_TYPE_HEADER) ? MFLAG_HF_NOVICE :
                     (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_NOVICE : MFLAG_MF_NOVICE;
        else if (strcasecmp(t, "Regular") == 0)
            flags |= (kind == MENU_TYPE_HEADER) ? MFLAG_HF_REGULAR :
                     (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_REGULAR : MFLAG_MF_REGULAR;
        else if (strcasecmp(t, "Expert") == 0)
            flags |= (kind == MENU_TYPE_HEADER) ? MFLAG_HF_EXPERT :
                     (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_EXPERT : MFLAG_MF_EXPERT;
        else if (strcasecmp(t, "RIP") == 0 && kind != MENU_TYPE_FOOTER)
            flags |= (kind == MENU_TYPE_HEADER) ? MFLAG_HF_RIP : MFLAG_MF_RIP;
    }

    if (flags == 0) {
        flags = (kind == MENU_TYPE_HEADER) ? MFLAG_HF_ALL :
                (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_ALL : MFLAG_MF_ALL;
    }

    *out_flags = flags;
    return true;
}

static bool menu_option_modifiers_to_bits(char **mods, size_t count, word *out_flags, byte *out_areatype)
{
    if (out_flags == NULL || out_areatype == NULL) {
        return false;
    }

    word flags = 0;
    byte areatype = ATYPE_NONE;

    for (size_t i = 0; i < count; i++) {
        const char *m = mods[i];
        if (m == NULL || m[0] == '\0') continue;

        if (strcasecmp(m, "Local") == 0) areatype |= ATYPE_LOCAL;
        else if (strcasecmp(m, "Matrix") == 0) areatype |= ATYPE_MATRIX;
        else if (strcasecmp(m, "Echo") == 0) areatype |= ATYPE_ECHO;
        else if (strcasecmp(m, "Conf") == 0) areatype |= ATYPE_CONF;
        else if (strcasecmp(m, "NoDsp") == 0) flags |= OFLAG_NODSP;
        else if (strcasecmp(m, "Ctl") == 0) flags |= OFLAG_CTL;
        else if (strcasecmp(m, "NoCLS") == 0) flags |= OFLAG_NOCLS;
        else if (strcasecmp(m, "NoRIP") == 0) flags |= OFLAG_NORIP;
        else if (strcasecmp(m, "RIP") == 0) flags |= OFLAG_RIP;
        else if (strcasecmp(m, "Then") == 0) flags |= OFLAG_THEN;
        else if (strcasecmp(m, "Else") == 0) flags |= OFLAG_ELSE;
        else if (strcasecmp(m, "Stay") == 0) flags |= OFLAG_STAY;
        else if (strcasecmp(m, "UsrLocal") == 0) flags |= OFLAG_ULOCAL;
        else if (strcasecmp(m, "UsrRemote") == 0) flags |= OFLAG_UREMOTE;
        else if (strcasecmp(m, "ReRead") == 0) flags |= OFLAG_REREAD;
    }

    *out_flags = flags;
    *out_areatype = areatype;
    return true;
}

static bool menu_definition_from_ng(const MaxCfgNgMenu *ng, MenuDefinition **out, char *err, size_t err_len)
{
    if (out == NULL) {
        set_err(err, err_len, "Invalid arguments");
        return false;
    }
    *out = NULL;

    if (ng == NULL || ng->name == NULL || ng->name[0] == '\0') {
        set_err(err, err_len, "Invalid menu");
        return false;
    }

    MenuDefinition *m = create_menu_definition(ng->name);
    if (!m) {
        set_err(err, err_len, "Out of memory");
        return false;
    }

    m->title = safe_strdup(ng->title);
    m->header_file = safe_strdup(ng->header_file);
    m->footer_file = safe_strdup(ng->footer_file);
    m->menu_file = safe_strdup(ng->menu_file);
    m->menu_length = ng->menu_length;
    m->menu_color = ng->menu_color;
    m->opt_width = ng->option_width;

    (void)menu_flags_from_types(ng->header_types, ng->header_type_count, MENU_TYPE_HEADER, &m->header_flags);
    (void)menu_flags_from_types(ng->footer_types, ng->footer_type_count, MENU_TYPE_FOOTER, &m->footer_flags);
    (void)menu_flags_from_types(ng->menu_types, ng->menu_type_count, MENU_TYPE_BODY, &m->menu_flags);

    /* Load custom menu fields if present */
    if (ng->custom_menu != NULL && ng->custom_menu->enabled) {
        const MaxCfgNgCustomMenu *cm = ng->custom_menu;
        
        m->cm_enabled = true;
        m->cm_skip_canned = cm->skip_canned_menu;
        m->cm_show_title = cm->show_title;
        m->cm_lightbar = cm->lightbar_menu;
        m->cm_lightbar_margin = cm->lightbar_margin;
        
        {
            const char *fg, *bg;
            if (cm->has_lightbar_normal) {
                attr_to_colors(cm->lightbar_normal_attr, &fg, &bg);
                m->cm_lb_normal_fg = safe_strdup(fg);
                m->cm_lb_normal_bg = safe_strdup(bg);
            }
            if (cm->has_lightbar_selected) {
                attr_to_colors(cm->lightbar_selected_attr, &fg, &bg);
                m->cm_lb_selected_fg = safe_strdup(fg);
                m->cm_lb_selected_bg = safe_strdup(bg);
            }
            if (cm->has_lightbar_high) {
                attr_to_colors(cm->lightbar_high_attr, &fg, &bg);
                m->cm_lb_high_fg = safe_strdup(fg);
                m->cm_lb_high_bg = safe_strdup(bg);
            }
            if (cm->has_lightbar_high_selected) {
                attr_to_colors(cm->lightbar_high_selected_attr, &fg, &bg);
                m->cm_lb_high_sel_fg = safe_strdup(fg);
                m->cm_lb_high_sel_bg = safe_strdup(bg);
            }
        }
        
        /* Boundary coordinates */
        m->cm_top_row = cm->top_boundary_row;
        m->cm_top_col = cm->top_boundary_col;
        m->cm_bottom_row = cm->bottom_boundary_row;
        m->cm_bottom_col = cm->bottom_boundary_col;
        
        /* Title and prompt locations */
        m->cm_title_row = cm->title_location_row;
        m->cm_title_col = cm->title_location_col;
        m->cm_prompt_row = cm->prompt_location_row;
        m->cm_prompt_col = cm->prompt_location_col;
        
        /* Layout options */
        m->cm_option_spacing = cm->option_spacing;
        m->cm_option_justify = cm->option_justify;
        m->cm_boundary_justify = cm->boundary_justify;
        m->cm_boundary_vjustify = cm->boundary_vjustify;
        m->cm_boundary_layout = cm->boundary_layout;
    }

    for (size_t i = 0; i < ng->option_count; i++) {
        const MaxCfgNgMenuOption *o = &ng->options[i];
        MenuOption *opt = create_menu_option();
        if (!opt) {
            free_menu_definition(m);
            set_err(err, err_len, "Out of memory");
            return false;
        }

        opt->command = safe_strdup(o->command);
        opt->arguments = safe_strdup(o->arguments);
        opt->priv_level = safe_strdup(o->priv_level);
        opt->description = safe_strdup(o->description);
        opt->key_poke = safe_strdup(o->key_poke);

        (void)menu_option_modifiers_to_bits(o->modifiers, o->modifier_count, &opt->flags, &opt->areatype);

        if (!add_menu_option(m, opt)) {
            free_menu_option(opt);
            free_menu_definition(m);
            set_err(err, err_len, "Out of memory");
            return false;
        }
    }

    *out = m;
    return true;
}

bool save_menu_toml(MaxCfgToml *toml, const char *toml_path, const char *toml_prefix, const MenuDefinition *menu, char *err, size_t err_len)
{
    if (toml == NULL || toml_path == NULL || toml_prefix == NULL || menu == NULL) {
        set_err(err, err_len, "Invalid arguments");
        return false;
    }

    FILE *fp = fopen(toml_path, "w");
    if (!fp) {
        set_err(err, err_len, "Cannot open %s for writing", toml_path);
        return false;
    }

    MaxCfgNgMenu ng;
    MaxCfgStatus st = maxcfg_ng_menu_init(&ng);
    if (st != MAXCFG_OK) {
        fclose(fp);
        set_err(err, err_len, "%s", maxcfg_status_string(st));
        return false;
    }

    ng.name = safe_strdup(menu->name ? menu->name : "");
    ng.title = safe_strdup(menu->title ? menu->title : "");
    ng.header_file = safe_strdup(menu->header_file ? menu->header_file : "");
    ng.footer_file = safe_strdup(menu->footer_file ? menu->footer_file : "");
    ng.menu_file = safe_strdup(menu->menu_file ? menu->menu_file : "");
    ng.menu_length = menu->menu_length;
    ng.menu_color = menu->menu_color;
    ng.option_width = menu->opt_width;

    if (!menu_types_from_flags(menu->header_flags, MENU_TYPE_HEADER, &ng.header_types, &ng.header_type_count)) {
        maxcfg_ng_menu_free(&ng);
        fclose(fp);
        set_err(err, err_len, "Out of memory");
        return false;
    }
    if (!menu_types_from_flags(menu->footer_flags, MENU_TYPE_FOOTER, &ng.footer_types, &ng.footer_type_count)) {
        maxcfg_ng_menu_free(&ng);
        fclose(fp);
        set_err(err, err_len, "Out of memory");
        return false;
    }
    if (!menu_types_from_flags(menu->menu_flags, MENU_TYPE_BODY, &ng.menu_types, &ng.menu_type_count)) {
        maxcfg_ng_menu_free(&ng);
        fclose(fp);
        set_err(err, err_len, "Out of memory");
        return false;
    }

    /* Populate custom menu fields if enabled */
    if (menu->cm_enabled) {
        MaxCfgNgCustomMenu *cm = (MaxCfgNgCustomMenu *)calloc(1, sizeof(MaxCfgNgCustomMenu));
        if (!cm) {
            maxcfg_ng_menu_free(&ng);
            fclose(fp);
            set_err(err, err_len, "Out of memory");
            return false;
        }
        
        cm->enabled = true;
        cm->skip_canned_menu = menu->cm_skip_canned;
        cm->show_title = menu->cm_show_title;
        cm->lightbar_menu = menu->cm_lightbar;
        cm->lightbar_margin = menu->cm_lightbar_margin;
        
        /* Convert color name strings to attributes */
        cm->lightbar_normal_attr = colors_to_attr(menu->cm_lb_normal_fg, menu->cm_lb_normal_bg);
        cm->lightbar_selected_attr = colors_to_attr(menu->cm_lb_selected_fg, menu->cm_lb_selected_bg);
        cm->lightbar_high_attr = colors_to_attr(menu->cm_lb_high_fg, menu->cm_lb_high_bg);
        cm->lightbar_high_selected_attr = colors_to_attr(menu->cm_lb_high_sel_fg, menu->cm_lb_high_sel_bg);
        
        /* Mark which colors are set */
        cm->has_lightbar_normal = (menu->cm_lb_normal_fg != NULL || menu->cm_lb_normal_bg != NULL);
        cm->has_lightbar_selected = (menu->cm_lb_selected_fg != NULL || menu->cm_lb_selected_bg != NULL);
        cm->has_lightbar_high = (menu->cm_lb_high_fg != NULL || menu->cm_lb_high_bg != NULL);
        cm->has_lightbar_high_selected = (menu->cm_lb_high_sel_fg != NULL || menu->cm_lb_high_sel_bg != NULL);
        
        /* Boundary coordinates */
        cm->top_boundary_row = menu->cm_top_row;
        cm->top_boundary_col = menu->cm_top_col;
        cm->bottom_boundary_row = menu->cm_bottom_row;
        cm->bottom_boundary_col = menu->cm_bottom_col;
        
        /* Title and prompt locations */
        cm->title_location_row = menu->cm_title_row;
        cm->title_location_col = menu->cm_title_col;
        cm->prompt_location_row = menu->cm_prompt_row;
        cm->prompt_location_col = menu->cm_prompt_col;
        
        /* Layout options */
        cm->option_spacing = menu->cm_option_spacing;
        cm->option_justify = menu->cm_option_justify;
        cm->boundary_justify = menu->cm_boundary_justify;
        cm->boundary_vjustify = menu->cm_boundary_vjustify;
        cm->boundary_layout = menu->cm_boundary_layout;
        
        ng.custom_menu = cm;
    }

    for (int i = 0; i < menu->option_count; i++) {
        MenuOption *opt = menu->options[i];
        if (!opt) continue;

        /* Convert bits to modifier strings */
        char **out_mods = NULL;
        size_t out_mod_count = 0;
        {
            if ((opt->areatype & ATYPE_LOCAL) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "Local")) goto oom; }
            if ((opt->areatype & ATYPE_MATRIX) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "Matrix")) goto oom; }
            if ((opt->areatype & ATYPE_ECHO) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "Echo")) goto oom; }
            if ((opt->areatype & ATYPE_CONF) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "Conf")) goto oom; }

            if ((opt->flags & OFLAG_NODSP) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "NoDsp")) goto oom; }
            if ((opt->flags & OFLAG_CTL) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "Ctl")) goto oom; }
            if ((opt->flags & OFLAG_NOCLS) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "NoCLS")) goto oom; }
            if ((opt->flags & OFLAG_NORIP) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "NoRIP")) goto oom; }
            if ((opt->flags & OFLAG_RIP) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "RIP")) goto oom; }
            if ((opt->flags & OFLAG_THEN) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "Then")) goto oom; }
            if ((opt->flags & OFLAG_ELSE) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "Else")) goto oom; }
            if ((opt->flags & OFLAG_STAY) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "Stay")) goto oom; }
            if ((opt->flags & OFLAG_ULOCAL) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "UsrLocal")) goto oom; }
            if ((opt->flags & OFLAG_UREMOTE) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "UsrRemote")) goto oom; }
            if ((opt->flags & OFLAG_REREAD) != 0u) { if (!strv_add(&out_mods, &out_mod_count, "ReRead")) goto oom; }
        }

        MaxCfgNgMenuOption ngopt = {
            .command = opt->command,
            .arguments = opt->arguments,
            .priv_level = opt->priv_level,
            .description = opt->description,
            .key_poke = opt->key_poke,
            .modifiers = out_mods,
            .modifier_count = out_mod_count
        };

        st = maxcfg_ng_menu_add_option(&ng, &ngopt);
        strv_free(&out_mods, &out_mod_count);
        if (st != MAXCFG_OK) {
            maxcfg_ng_menu_free(&ng);
            fclose(fp);
            set_err(err, err_len, "%s", maxcfg_status_string(st));
            return false;
        }
        continue;

oom:
        strv_free(&out_mods, &out_mod_count);
        maxcfg_ng_menu_free(&ng);
        fclose(fp);
        set_err(err, err_len, "Out of memory");
        return false;
    }

    st = maxcfg_ng_write_menu_toml(fp, &ng);
    maxcfg_ng_menu_free(&ng);
    fclose(fp);
    if (st != MAXCFG_OK) {
        set_err(err, err_len, "%s", maxcfg_status_string(st));
        return false;
    }

    st = maxcfg_toml_load_file(toml, toml_path, toml_prefix);
    if (st != MAXCFG_OK) {
        set_err(err, err_len, "%s", maxcfg_status_string(st));
        return false;
    }

    return true;
}

bool load_menus_toml(MaxCfgToml *toml,
                     const char *sys_path,
                     MenuDefinition ***out_menus,
                     char ***out_paths,
                     char ***out_prefixes,
                     int *out_count,
                     char *err,
                     size_t err_len)
{
    if (out_menus == NULL || out_paths == NULL || out_prefixes == NULL || out_count == NULL) {
        set_err(err, err_len, "Invalid arguments");
        return false;
    }

    *out_menus = NULL;
    *out_paths = NULL;
    *out_prefixes = NULL;
    *out_count = 0;

    if (toml == NULL || sys_path == NULL || sys_path[0] == '\0') {
        set_err(err, err_len, "System path not configured");
        return false;
    }

    char menu_dir[512];
    if (snprintf(menu_dir, sizeof(menu_dir), "%s/config/menus", sys_path) >= (int)sizeof(menu_dir)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    DIR *d = opendir(menu_dir);
    if (d == NULL) {
        set_err(err, err_len, "Cannot open %s", menu_dir);
        return false;
    }

    int cap = 0;
    MenuDefinition **menus = NULL;
    char **paths = NULL;
    char **prefixes = NULL;
    int count = 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        const char *name = de->d_name;
        size_t n = strlen(name);
        if (n < 6) continue;
        if (strcasecmp(name + (n - 5), ".toml") != 0) continue;

        char base[128];
        size_t bn = n - 5;
        if (bn >= sizeof(base)) continue;
        memcpy(base, name, bn);
        base[bn] = '\0';

        char path[PATH_MAX];
        if (snprintf(path, sizeof(path), "%s/%s", menu_dir, name) >= (int)sizeof(path)) {
            continue;
        }

        char prefix[256];
        if (snprintf(prefix, sizeof(prefix), "menus.%s", base) >= (int)sizeof(prefix)) {
            continue;
        }

        MaxCfgStatus st = maxcfg_toml_load_file(toml, path, prefix);
        if (st != MAXCFG_OK) {
            set_err(err, err_len, "%s", maxcfg_status_string(st));
            goto fail;
        }

        MaxCfgNgMenu ng;
        st = maxcfg_ng_menu_init(&ng);
        if (st != MAXCFG_OK) {
            set_err(err, err_len, "%s", maxcfg_status_string(st));
            goto fail;
        }

        st = maxcfg_ng_get_menu(toml, prefix, &ng);
        if (st != MAXCFG_OK) {
            maxcfg_ng_menu_free(&ng);
            set_err(err, err_len, "%s", maxcfg_status_string(st));
            goto fail;
        }

        MenuDefinition *menu = NULL;
        if (!menu_definition_from_ng(&ng, &menu, err, err_len)) {
            maxcfg_ng_menu_free(&ng);
            goto fail;
        }
        maxcfg_ng_menu_free(&ng);

        if (count >= cap) {
            int new_cap = (cap == 0) ? 8 : (cap * 2);

            MenuDefinition **m2 = calloc((size_t)new_cap, sizeof(MenuDefinition *));
            char **p2 = calloc((size_t)new_cap, sizeof(char *));
            char **x2 = calloc((size_t)new_cap, sizeof(char *));
            if (m2 == NULL || p2 == NULL || x2 == NULL) {
                free(m2);
                free(p2);
                free(x2);
                free_menu_definition(menu);
                set_err(err, err_len, "Out of memory");
                goto fail;
            }

            if (menus) memcpy(m2, menus, (size_t)count * sizeof(MenuDefinition *));
            if (paths) memcpy(p2, paths, (size_t)count * sizeof(char *));
            if (prefixes) memcpy(x2, prefixes, (size_t)count * sizeof(char *));

            free(menus);
            free(paths);
            free(prefixes);

            menus = m2;
            paths = p2;
            prefixes = x2;
            cap = new_cap;
        }

        menus[count] = menu;
        paths[count] = strdup(path);
        prefixes[count] = strdup(prefix);
        if (paths[count] == NULL || prefixes[count] == NULL) {
            free_menu_definition(menu);
            set_err(err, err_len, "Out of memory");
            goto fail;
        }
        count++;
    }

    closedir(d);

    *out_menus = menus;
    *out_paths = paths;
    *out_prefixes = prefixes;
    *out_count = count;
    return true;

fail:
    closedir(d);
    for (int i = 0; i < count; i++) {
        free_menu_definition(menus[i]);
        free(paths[i]);
        free(prefixes[i]);
    }
    free(menus);
    free(paths);
    free(prefixes);
    *out_menus = NULL;
    *out_paths = NULL;
    *out_prefixes = NULL;
    *out_count = 0;
    return false;
}

/* Create a new empty menu definition */
MenuDefinition *create_menu_definition(const char *name) {
    MenuDefinition *menu = calloc(1, sizeof(MenuDefinition));
    if (!menu) return NULL;
    
    menu->name = strdup(name ? name : "");
    menu->menu_color = -1;  /* Default: no color override */
    menu->opt_width = 0;    /* Default: use system default (20) */
    menu->option_capacity = 16;
    menu->options = calloc(menu->option_capacity, sizeof(MenuOption *));
    
    if (!menu->options) {
        free(menu->name);
        free(menu);
        return NULL;
    }
    
    /* Initialize custom menu fields with defaults */
    menu->cm_enabled = false;
    menu->cm_skip_canned = false;
    menu->cm_show_title = true;
    menu->cm_lightbar = false;
    menu->cm_lightbar_margin = 1;
    
    menu->cm_lb_normal_fg = NULL;
    menu->cm_lb_normal_bg = NULL;
    menu->cm_lb_selected_fg = NULL;
    menu->cm_lb_selected_bg = NULL;
    menu->cm_lb_high_fg = NULL;
    menu->cm_lb_high_bg = NULL;
    menu->cm_lb_high_sel_fg = NULL;
    menu->cm_lb_high_sel_bg = NULL;
    
    /* No boundaries by default */
    menu->cm_top_row = 0;
    menu->cm_top_col = 0;
    menu->cm_bottom_row = 0;
    menu->cm_bottom_col = 0;
    
    /* No title/prompt locations by default */
    menu->cm_title_row = 0;
    menu->cm_title_col = 0;
    menu->cm_prompt_row = 0;
    menu->cm_prompt_col = 0;
    
    /* Layout defaults */
    menu->cm_option_spacing = false;
    menu->cm_option_justify = 0;  /* Left */
    menu->cm_boundary_justify = 0;
    menu->cm_boundary_vjustify = 0;
    menu->cm_boundary_layout = 0;
    
    return menu;
}

/* Create a new empty menu option */
MenuOption *create_menu_option(void) {
    MenuOption *opt = calloc(1, sizeof(MenuOption));
    return opt;
}

/* Free a menu option */
void free_menu_option(MenuOption *option) {
    if (!option) return;
    free(option->command);
    free(option->arguments);
    free(option->priv_level);
    free(option->description);
    free(option->key_poke);
    free(option);
}

/* Free a single menu definition */
void free_menu_definition(MenuDefinition *menu) {
    if (!menu) return;
    
    free(menu->name);
    free(menu->title);
    free(menu->header_file);
    free(menu->footer_file);
    free(menu->menu_file);
    
    /* Free custom menu color strings */
    free(menu->cm_lb_normal_fg);
    free(menu->cm_lb_normal_bg);
    free(menu->cm_lb_selected_fg);
    free(menu->cm_lb_selected_bg);
    free(menu->cm_lb_high_fg);
    free(menu->cm_lb_high_bg);
    free(menu->cm_lb_high_sel_fg);
    free(menu->cm_lb_high_sel_bg);
    
    for (int i = 0; i < menu->option_count; i++) {
        free_menu_option(menu->options[i]);
    }
    free(menu->options);
    free(menu);
}

/* Free menu definitions */
void free_menu_definitions(MenuDefinition **menus, int menu_count) {
    if (!menus) return;
    for (int i = 0; i < menu_count; i++) {
        free_menu_definition(menus[i]);
    }
    free(menus);
}

/* Add an option to a menu */
bool add_menu_option(MenuDefinition *menu, MenuOption *option) {
    if (!menu || !option) return false;
    
    /* Expand capacity if needed */
    if (menu->option_count >= menu->option_capacity) {
        int new_capacity = menu->option_capacity * 2;
        MenuOption **new_options = realloc(menu->options, new_capacity * sizeof(MenuOption *));
        if (!new_options) return false;
        menu->options = new_options;
        menu->option_capacity = new_capacity;
    }
    
    menu->options[menu->option_count++] = option;
    return true;
}

/* Insert an option at a specific position in a menu */
bool insert_menu_option(MenuDefinition *menu, MenuOption *option, int index) {
    if (!menu || !option || index < 0 || index > menu->option_count) return false;
    
    /* Expand capacity if needed */
    if (menu->option_count >= menu->option_capacity) {
        int new_capacity = menu->option_capacity * 2;
        MenuOption **new_options = realloc(menu->options, new_capacity * sizeof(MenuOption *));
        if (!new_options) return false;
        menu->options = new_options;
        menu->option_capacity = new_capacity;
    }
    
    /* Shift options after insert position */
    for (int i = menu->option_count; i > index; i--) {
        menu->options[i] = menu->options[i - 1];
    }
    
    menu->options[index] = option;
    menu->option_count++;
    return true;
}

/* Remove an option from a menu */
bool remove_menu_option(MenuDefinition *menu, int index) {
    if (!menu || index < 0 || index >= menu->option_count) return false;
    
    free_menu_option(menu->options[index]);
    
    /* Shift remaining options down */
    for (int i = index; i < menu->option_count - 1; i++) {
        menu->options[i] = menu->options[i + 1];
    }
    menu->option_count--;
    return true;
}

/* Helper to parse flag types from space-separated list */
static word parse_type_flags(const char *types, int kind) {
    word flags = 0;
    if (!types || !*types) {
        return (kind == MENU_TYPE_HEADER) ? MFLAG_HF_ALL :
               (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_ALL : MFLAG_MF_ALL;
    }
    
    char *types_copy = strdup(types);
    char *token = strtok(types_copy, " \t");
    
    while (token) {
        if (strcasecmp(token, "Novice") == 0) {
            flags |= (kind == MENU_TYPE_HEADER) ? MFLAG_HF_NOVICE :
                     (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_NOVICE : MFLAG_MF_NOVICE;
        } else if (strcasecmp(token, "Regular") == 0) {
            flags |= (kind == MENU_TYPE_HEADER) ? MFLAG_HF_REGULAR :
                     (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_REGULAR : MFLAG_MF_REGULAR;
        } else if (strcasecmp(token, "Expert") == 0) {
            flags |= (kind == MENU_TYPE_HEADER) ? MFLAG_HF_EXPERT :
                     (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_EXPERT : MFLAG_MF_EXPERT;
        } else if (strcasecmp(token, "RIP") == 0 && kind != MENU_TYPE_FOOTER) {
            flags |= (kind == MENU_TYPE_HEADER) ? MFLAG_HF_RIP : MFLAG_MF_RIP;
        }
        token = strtok(NULL, " \t");
    }
    
    free(types_copy);
    return flags ? flags : ((kind == MENU_TYPE_HEADER) ? MFLAG_HF_ALL :
                            (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_ALL : MFLAG_MF_ALL);
}

/* Helper to parse option modifiers and return flags */
static word parse_option_modifiers(char **line_ptr, byte *areatype) {
    word flags = 0;
    *areatype = ATYPE_NONE;
    char *line = *line_ptr;
    
    while (*line) {
        /* Skip whitespace */
        while (isspace((unsigned char)*line)) line++;
        if (!*line) break;
        
        /* Check for modifiers */
        if (strncasecmp(line, "NoDsp", 5) == 0 && (isspace(line[5]) || !line[5])) {
            flags |= OFLAG_NODSP;
            line += 5;
        } else if (strncasecmp(line, "Ctl", 3) == 0 && (isspace(line[3]) || !line[3])) {
            flags |= OFLAG_CTL;
            line += 3;
        } else if (strncasecmp(line, "NoCLS", 5) == 0 && (isspace(line[5]) || !line[5])) {
            flags |= OFLAG_NOCLS;
            line += 5;
        } else if (strncasecmp(line, "NoRIP", 5) == 0 && (isspace(line[5]) || !line[5])) {
            flags |= OFLAG_NORIP;
            line += 5;
        } else if (strncasecmp(line, "RIP", 3) == 0 && (isspace(line[3]) || !line[3])) {
            flags |= OFLAG_RIP;
            line += 3;
        } else if (strncasecmp(line, "Then", 4) == 0 && (isspace(line[4]) || !line[4])) {
            flags |= OFLAG_THEN;
            line += 4;
        } else if (strncasecmp(line, "Else", 4) == 0 && (isspace(line[4]) || !line[4])) {
            flags |= OFLAG_ELSE;
            line += 4;
        } else if (strncasecmp(line, "Stay", 4) == 0 && (isspace(line[4]) || !line[4])) {
            flags |= OFLAG_STAY;
            line += 4;
        } else if (strncasecmp(line, "Local", 5) == 0 && (isspace(line[5]) || !line[5])) {
            *areatype |= ATYPE_LOCAL;
            line += 5;
        } else if (strncasecmp(line, "Matrix", 6) == 0 && (isspace(line[6]) || !line[6])) {
            *areatype |= ATYPE_MATRIX;
            line += 6;
        } else if (strncasecmp(line, "Echo", 4) == 0 && (isspace(line[4]) || !line[4])) {
            *areatype |= ATYPE_ECHO;
            line += 4;
        } else if (strncasecmp(line, "Conf", 4) == 0 && (isspace(line[4]) || !line[4])) {
            *areatype |= ATYPE_CONF;
            line += 4;
        } else if (strncasecmp(line, "UsrLocal", 8) == 0 && (isspace(line[8]) || !line[8])) {
            flags |= OFLAG_ULOCAL;
            line += 8;
        } else if (strncasecmp(line, "UsrRemote", 9) == 0 && (isspace(line[9]) || !line[9])) {
            flags |= OFLAG_UREMOTE;
            line += 9;
        } else if (strncasecmp(line, "ReRead", 6) == 0 && (isspace(line[6]) || !line[6])) {
            flags |= OFLAG_REREAD;
            line += 6;
        } else {
            /* Not a modifier, stop parsing */
            break;
        }
    }
    
    *line_ptr = line;
    return flags;
}

/* Parse menus.ctl */
MenuDefinition **parse_menus_ctl(const char *sys_path, int *menu_count, char *err, size_t err_len) {
    if (!sys_path || !menu_count) {
        if (err) snprintf(err, err_len, "Invalid parameters");
        return NULL;
    }
    
    *menu_count = 0;
    
    char path[512];
    snprintf(path, sizeof(path), "%s/config/legacy/menus.ctl", sys_path);
    
    FILE *f = fopen(path, "r");
    if (!f) {
        if (err) snprintf(err, err_len, "Cannot open %s", path);
        return NULL;
    }
    
    MenuDefinition **menus = NULL;
    int capacity = 0;
    MenuDefinition *current_menu = NULL;
    char line[1024];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), f)) {
        line_num++;
        
        /* Remove newline */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        
        /* Skip comment lines (menus.ctl uses % at line start; % may also appear in values like "(%t mins)") */
        {
            char *p = line;
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == '%' || *p == '#') continue;
        }
        
        /* Trim whitespace */
        char *trimmed = trim_whitespace(line);
        if (!*trimmed) continue;
        
        /* Parse keywords */
        {
            const char *v;
            if (kw_value(trimmed, "Menu", &v)) {
            /* Start new menu */
            char *menu_name = trim_whitespace((char *)v);
            current_menu = create_menu_definition(menu_name);
            if (!current_menu) {
                if (err) snprintf(err, err_len, "Out of memory at line %d", line_num);
                goto error;
            }
            
            /* Expand menus array */
            if (*menu_count >= capacity) {
                capacity = capacity ? capacity * 2 : 8;
                MenuDefinition **new_menus = realloc(menus, capacity * sizeof(MenuDefinition *));
                if (!new_menus) {
                    free_menu_definition(current_menu);
                    if (err) snprintf(err, err_len, "Out of memory at line %d", line_num);
                    goto error;
                }
                menus = new_menus;
            }
            menus[(*menu_count)++] = current_menu;
            
            } else if (strncasecmp(trimmed, "End Menu", 8) == 0 ||
                       strncasecmp(trimmed, "End", 3) == 0) {
            current_menu = NULL;
            
            } else if (current_menu) {
            /* Parse menu properties and options */
                const char *v2;
                if (kw_value(trimmed, "Title", &v2)) {
                    free(current_menu->title);
                    current_menu->title = safe_strdup(trim_whitespace((char *)v2));
                
                } else if (kw_value(trimmed, "HeaderFile", &v2)) {
                    char *rest = trim_whitespace((char *)v2);
                    char *space = rest;
                    while (*space && !isspace((unsigned char)*space)) space++;
                    if (*space) {
                        *space = '\0';
                        space++;
                        while (*space && isspace((unsigned char)*space)) space++;
                        current_menu->header_file = safe_strdup(rest);
                        current_menu->header_flags = parse_type_flags(space, MENU_TYPE_HEADER);
                    } else {
                        current_menu->header_file = safe_strdup(rest);
                        current_menu->header_flags = MFLAG_HF_ALL;
                    }

                } else if (kw_value(trimmed, "FooterFile", &v2)) {
                    char *rest = trim_whitespace((char *)v2);
                    char *space = rest;
                    while (*space && !isspace((unsigned char)*space)) space++;
                    if (*space) {
                        *space = '\0';
                        space++;
                        while (*space && isspace((unsigned char)*space)) space++;
                        current_menu->footer_file = safe_strdup(rest);
                        current_menu->footer_flags = parse_type_flags(space, MENU_TYPE_FOOTER);
                    } else {
                        current_menu->footer_file = safe_strdup(rest);
                        current_menu->footer_flags = MFLAG_FF_ALL;
                    }

                } else if (kw_value(trimmed, "MenuFile", &v2)) {
                    char *rest = trim_whitespace((char *)v2);
                    char *space = rest;
                    while (*space && !isspace((unsigned char)*space)) space++;
                    if (*space) {
                        *space = '\0';
                        space++;
                        while (*space && isspace((unsigned char)*space)) space++;
                        current_menu->menu_file = safe_strdup(rest);
                        current_menu->menu_flags = parse_type_flags(space, MENU_TYPE_BODY);
                    } else {
                        current_menu->menu_file = safe_strdup(rest);
                        current_menu->menu_flags = MFLAG_MF_ALL;
                    }
                
                } else if (kw_value(trimmed, "MenuLength", &v2)) {
                    current_menu->menu_length = atoi(v2);
                
                } else if (kw_value(trimmed, "MenuColor", &v2) || kw_value(trimmed, "MenuColour", &v2)) {
                    current_menu->menu_color = atoi(v2);
                
                } else if (kw_value(trimmed, "OptionWidth", &v2)) {
                    current_menu->opt_width = atoi(v2);
                
                } else {
                /* Try to parse as menu option */
                MenuOption opt = {0};
                char *line_ptr = trimmed;
                
                /* Parse modifiers */
                opt.flags = parse_option_modifiers(&line_ptr, &opt.areatype);
                line_ptr = trim_whitespace(line_ptr);
                
                /* Parse command */
                char *space = strchr(line_ptr, ' ');
                if (space) {
                    *space = '\0';
                    opt.command = safe_strdup(line_ptr);
                    line_ptr = trim_whitespace(space + 1);
                    
                    /* Parse arguments and privilege level */
                    char *quote1 = strchr(line_ptr, '"');
                    if (quote1) {
                        /* Everything before first quote is arguments and priv */
                        *quote1 = '\0';
                        char *args_and_priv = trim_whitespace(line_ptr);
                        
                        /* Last word is priv level */
                        char *last_space = strrchr(args_and_priv, ' ');
                        if (last_space) {
                            *last_space = '\0';
                            opt.arguments = safe_strdup(trim_whitespace(args_and_priv));
                            opt.priv_level = safe_strdup(trim_whitespace(last_space + 1));
                        } else {
                            opt.priv_level = safe_strdup(args_and_priv);
                        }
                        
                        /* Parse description */
                        quote1++;
                        char *quote2 = strchr(quote1, '"');
                        if (quote2) {
                            *quote2 = '\0';
                            opt.description = safe_strdup(quote1);
                            
                            /* Check for optional key-poke string */
                            char *quote3 = strchr(quote2 + 1, '"');
                            if (quote3) {
                                quote3++;
                                char *quote4 = strchr(quote3, '"');
                                if (quote4) {
                                    *quote4 = '\0';
                                    opt.key_poke = safe_strdup(quote3);
                                }
                            }
                        }
                    }
                    
                    /* Add option if we got at least command and description */
                    if (opt.command && opt.description) {
                        MenuOption *new_opt = create_menu_option();
                        if (new_opt) {
                            *new_opt = opt;
                            if (!add_menu_option(current_menu, new_opt)) {
                                free_menu_option(new_opt);
                            }
                        }
                    } else {
                        /* Free partially parsed option */
                        free(opt.command);
                        free(opt.arguments);
                        free(opt.priv_level);
                        free(opt.description);
                        free(opt.key_poke);
                    }
                }
            }
            }
        }
    }
    
    fclose(f);
    return menus;

error:
    if (f) fclose(f);
    free_menu_definitions(menus, *menu_count);
    *menu_count = 0;
    return NULL;
}

#if 0
static void write_type_flags(FILE *f, word flags, bool is_header)
{
    word all = is_header ? MFLAG_HF_ALL : MFLAG_MF_ALL;
    if (flags == 0 || flags == all) return;

    if (flags & (is_header ? MFLAG_HF_NOVICE : MFLAG_MF_NOVICE)) fprintf(f, " Novice");
    if (flags & (is_header ? MFLAG_HF_REGULAR : MFLAG_MF_REGULAR)) fprintf(f, " Regular");
    if (flags & (is_header ? MFLAG_HF_EXPERT : MFLAG_MF_EXPERT)) fprintf(f, " Expert");
    if (flags & (is_header ? MFLAG_HF_RIP : MFLAG_MF_RIP)) fprintf(f, " RIP");
}

static void write_menu_option(FILE *f, MenuOption *opt) {
    if (!opt || !opt->command) return;

    fprintf(f, "        ");

    if (opt->areatype & ATYPE_LOCAL) fprintf(f, "Local ");
    if (opt->areatype & ATYPE_MATRIX) fprintf(f, "Matrix ");
    if (opt->areatype & ATYPE_ECHO) fprintf(f, "Echo ");
    if (opt->areatype & ATYPE_CONF) fprintf(f, "Conf ");

    if (opt->flags & OFLAG_NODSP) fprintf(f, "NoDsp ");
    if (opt->flags & OFLAG_CTL) fprintf(f, "Ctl ");
    if (opt->flags & OFLAG_NOCLS) fprintf(f, "NoCLS ");
    if (opt->flags & OFLAG_NORIP) fprintf(f, "NoRIP ");
    if (opt->flags & OFLAG_RIP) fprintf(f, "RIP ");
    if (opt->flags & OFLAG_THEN) fprintf(f, "Then ");
    if (opt->flags & OFLAG_ELSE) fprintf(f, "Else ");
    if (opt->flags & OFLAG_STAY) fprintf(f, "Stay ");
    if (opt->flags & OFLAG_ULOCAL) fprintf(f, "UsrLocal ");
    if (opt->flags & OFLAG_UREMOTE) fprintf(f, "UsrRemote ");
    if (opt->flags & OFLAG_REREAD) fprintf(f, "ReRead ");

    fprintf(f, "%s", opt->command);

    if (opt->arguments && opt->arguments[0]) {
        fprintf(f, " %s", opt->arguments);
    }

    if (opt->priv_level && opt->priv_level[0]) {
        fprintf(f, " %s", opt->priv_level);
    }

    if (opt->description && opt->description[0]) {
        fprintf(f, " \"%s\"", opt->description);
    }

    if (opt->key_poke && opt->key_poke[0]) {
        fprintf(f, " \"%s\"", opt->key_poke);
    }

    fprintf(f, "\n");
}

bool save_menus_ctl(const char *sys_path, MenuDefinition **menus, int menu_count, char *err, size_t err_len) {
    if (!sys_path || !menus) {
        if (err) snprintf(err, err_len, "Invalid parameters");
        return false;
    }
    
    char path[512];
    snprintf(path, sizeof(path), "%s/config/legacy/menus.ctl", sys_path);
    
    char backup_path[512];
    snprintf(backup_path, sizeof(backup_path), "%s.bak", path);
    
    struct stat st;
    if (stat(backup_path, &st) != 0) {
        FILE *src = fopen(path, "r");
        if (src) {
            FILE *dst = fopen(backup_path, "w");
            if (dst) {
                char buf[4096];
                size_t n;
                while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
                    fwrite(buf, 1, n, dst);
                }
                fclose(dst);
            }
            fclose(src);
        }
    }
    
    FILE *f = fopen(path, "w");
    if (!f) {
        if (err) snprintf(err, err_len, "Cannot open %s for writing", path);
        return false;
    }
    
    fprintf(f, "%% MENUS.CTL - Menu configuration file\n");
    fprintf(f, "%% Written by MAXCFG\n");
    fprintf(f, "%%\n");
    fprintf(f, "%% This file controls all aspects of Max's menus, including\n");
    fprintf(f, "%% the actions performed by each option, the overall menu structure,\n");
    fprintf(f, "%% and the appearance of each menu.\n\n");
    
    for (int i = 0; i < menu_count; i++) {
        MenuDefinition *menu = menus[i];
        if (!menu || !menu->name) continue;
        
        fprintf(f, "Menu %s\n", menu->name);
        
        if (menu->title && menu->title[0]) {
            fprintf(f, "        Title           %s\n", menu->title);
        }
        
        if (menu->header_file && menu->header_file[0]) {
            fprintf(f, "        HeaderFile      %s", menu->header_file);
            write_type_flags(f, menu->header_flags, true);
            fprintf(f, "\n");
        }
        
        if (menu->menu_file && menu->menu_file[0]) {
            fprintf(f, "        MenuFile        %s", menu->menu_file);
            write_type_flags(f, menu->menu_flags, false);
            fprintf(f, "\n");
        }
        
        if (menu->menu_length > 0) {
            fprintf(f, "        MenuLength      %d\n", menu->menu_length);
        }
        
        if (menu->menu_color >= 0) {
            fprintf(f, "        MenuColour      %d\n", menu->menu_color);
        }
        
        if (menu->opt_width > 0) {
            fprintf(f, "        OptionWidth     %d\n", menu->opt_width);
        }

        for (int j = 0; j < menu->option_count; j++) {
            write_menu_option(f, menu->options[j]);
        }

        fprintf(f, "End Menu\n\n");
    }
    
    fclose(f);
    return true;
}

#endif
