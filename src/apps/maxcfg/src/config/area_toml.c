/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * area_toml.c - Message and file area TOML loader/saver
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include "libmaxcfg.h"
#include "area_toml.h"
#include "area_parse.h"
#include "treeview.h"

/* Helper to set error message */
static void set_err(char *err, size_t err_sz, const char *msg)
{
    if (err && err_sz > 0) {
        strncpy(err, msg, err_sz - 1);
        err[err_sz - 1] = '\0';
    }
}

/* Helper to duplicate string or return NULL */
static char *dup_str_or_null(const char *s)
{
    return (s && s[0]) ? strdup(s) : NULL;
}

/* Convert message area style strings to bitmask */
static unsigned int msg_style_from_strings(char **styles, size_t count)
{
    unsigned int mask = 0;
    for (size_t i = 0; i < count; i++) {
        const char *s = styles[i];
        if (!s) continue;
        
        if (strcasecmp(s, "Squish") == 0) mask |= MSGSTYLE_SQUISH;
        else if (strcasecmp(s, "SDM") == 0 || strcasecmp(s, ".MSG") == 0) mask |= MSGSTYLE_DOTMSG;
        else if (strcasecmp(s, "Local") == 0) mask |= MSGSTYLE_LOCAL;
        else if (strcasecmp(s, "Net") == 0 || strcasecmp(s, "NetMail") == 0) mask |= MSGSTYLE_NET;
        else if (strcasecmp(s, "Echo") == 0 || strcasecmp(s, "EchoMail") == 0) mask |= MSGSTYLE_ECHO;
        else if (strcasecmp(s, "Conf") == 0) mask |= MSGSTYLE_CONF;
        else if (strcasecmp(s, "Pvt") == 0 || strcasecmp(s, "Private") == 0) mask |= MSGSTYLE_PVT;
        else if (strcasecmp(s, "Pub") == 0 || strcasecmp(s, "Public") == 0) mask |= MSGSTYLE_PUB;
        else if (strcasecmp(s, "HiBit") == 0) mask |= MSGSTYLE_HIBIT;
        else if (strcasecmp(s, "Anon") == 0) mask |= MSGSTYLE_ANON;
        else if (strcasecmp(s, "NoRnk") == 0) mask |= MSGSTYLE_NORNK;
        else if (strcasecmp(s, "RealName") == 0) mask |= MSGSTYLE_REALNAME;
        else if (strcasecmp(s, "Alias") == 0) mask |= MSGSTYLE_ALIAS;
        else if (strcasecmp(s, "Audit") == 0) mask |= MSGSTYLE_AUDIT;
        else if (strcasecmp(s, "ReadOnly") == 0) mask |= MSGSTYLE_READONLY;
        else if (strcasecmp(s, "Hidden") == 0) mask |= MSGSTYLE_HIDDEN;
        else if (strcasecmp(s, "Attach") == 0) mask |= MSGSTYLE_ATTACH;
        else if (strcasecmp(s, "NoMailChk") == 0) mask |= MSGSTYLE_NOMAILCHK;
    }
    return mask;
}

/**
 * Append a string to a dynamic `char **` list.
 *
 * The resulting list is owned by the caller and must be freed with `strv_free()`.
 */
static bool strv_add(char ***items_io, size_t *count_io, const char *s)
{
    if (items_io == NULL || count_io == NULL) {
        return false;
    }

    char **p = (char **)realloc(*items_io, (*count_io + 1u) * sizeof(**items_io));
    if (p == NULL) {
        return false;
    }
    *items_io = p;

    (*items_io)[*count_io] = strdup(s ? s : "");
    if ((*items_io)[*count_io] == NULL) {
        return false;
    }
    (*count_io)++;
    return true;
}

/**
 * Free a dynamic `char **` list allocated via `strv_add()`.
 */
static void strv_free(char ***items_io, size_t *count_io)
{
    if (items_io == NULL || *items_io == NULL) {
        if (count_io) {
            *count_io = 0;
        }
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

static void file_area_types_from_strings(FileAreaData *a, char **types, size_t count);
static bool file_area_types_to_strings(const FileAreaData *a, char ***out_types, size_t *out_count);

/**
 * Convert the message area style bitmask to the TOML string list form.
 */
static bool msg_style_to_strings(unsigned int style, char ***out_styles, size_t *out_count)
{
    *out_styles = NULL;
    *out_count = 0;

    if ((style & MSGSTYLE_SQUISH) != 0u) { if (!strv_add(out_styles, out_count, "Squish")) return false; }
    else if ((style & MSGSTYLE_DOTMSG) != 0u) { if (!strv_add(out_styles, out_count, "*.MSG")) return false; }

    if ((style & MSGSTYLE_LOCAL) != 0u) { if (!strv_add(out_styles, out_count, "Local")) return false; }
    else if ((style & MSGSTYLE_NET) != 0u) { if (!strv_add(out_styles, out_count, "Net")) return false; }
    else if ((style & MSGSTYLE_ECHO) != 0u) { if (!strv_add(out_styles, out_count, "Echo")) return false; }
    else if ((style & MSGSTYLE_CONF) != 0u) { if (!strv_add(out_styles, out_count, "Conf")) return false; }

    if ((style & MSGSTYLE_PVT) != 0u) { if (!strv_add(out_styles, out_count, "Pvt")) return false; }
    if ((style & MSGSTYLE_PUB) != 0u) { if (!strv_add(out_styles, out_count, "Pub")) return false; }

    if ((style & MSGSTYLE_HIBIT) != 0u) { if (!strv_add(out_styles, out_count, "HiBit")) return false; }
    if ((style & MSGSTYLE_ANON) != 0u) { if (!strv_add(out_styles, out_count, "Anon")) return false; }
    if ((style & MSGSTYLE_NORNK) != 0u) { if (!strv_add(out_styles, out_count, "NoNameKludge")) return false; }
    if ((style & MSGSTYLE_REALNAME) != 0u) { if (!strv_add(out_styles, out_count, "RealName")) return false; }
    if ((style & MSGSTYLE_ALIAS) != 0u) { if (!strv_add(out_styles, out_count, "Alias")) return false; }
    if ((style & MSGSTYLE_AUDIT) != 0u) { if (!strv_add(out_styles, out_count, "Audit")) return false; }
    if ((style & MSGSTYLE_READONLY) != 0u) { if (!strv_add(out_styles, out_count, "ReadOnly")) return false; }
    if ((style & MSGSTYLE_HIDDEN) != 0u) { if (!strv_add(out_styles, out_count, "Hidden")) return false; }
    if ((style & MSGSTYLE_ATTACH) != 0u) { if (!strv_add(out_styles, out_count, "Attach")) return false; }
    if ((style & MSGSTYLE_NOMAILCHK) != 0u) { if (!strv_add(out_styles, out_count, "NoMailCheck")) return false; }

    return true;
}

/**
 * Convert a TreeNode division subtree into the typed libmaxcfg NG division list.
 */
static bool build_msg_divisions_recursive(MaxCfgNgDivisionList *divs, TreeNode *node, char *err, size_t err_sz)
{
    if (divs == NULL || node == NULL) {
        return true;
    }

    if (node->type == TREENODE_DIVISION) {
        DivisionData *d = (DivisionData *)node->data;
        MaxCfgNgDivision div = {
            .name = node->name,
            .key = node->full_name,
            .description = node->description,
            .acs = d ? d->acs : NULL,
            .display_file = d ? d->display_file : NULL,
            .level = node->division_level
        };

        MaxCfgStatus st = maxcfg_ng_division_list_add(divs, &div);
        if (st != MAXCFG_OK) {
            set_err(err, err_sz, maxcfg_status_string(st));
            return false;
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        if (!build_msg_divisions_recursive(divs, node->children[i], err, err_sz)) {
            return false;
        }
    }

    return true;
}

/**
 * Convert a TreeNode message area subtree into the typed libmaxcfg NG message area list.
 */
static bool build_msg_areas_recursive(MaxCfgNgMsgAreaList *areas, TreeNode *node, const char *division_key, char *err, size_t err_sz)
{
    if (areas == NULL || node == NULL) {
        return true;
    }

    const char *div_for_children = division_key;
    if (node->type == TREENODE_DIVISION) {
        div_for_children = node->full_name;
    } else if (node->type == TREENODE_AREA) {
        MsgAreaData *a = (MsgAreaData *)node->data;
        if (a != NULL) {
            char **styles = NULL;
            size_t style_count = 0;
            if (!msg_style_to_strings(a->style, &styles, &style_count)) {
                strv_free(&styles, &style_count);
                set_err(err, err_sz, "Out of memory");
                return false;
            }

            MaxCfgNgMsgArea area = {
                .name = a->name,
                .description = a->desc,
                .acs = a->acs,
                .menu = a->menuname,
                .division = (char *)division_key,
                .tag = a->tag,
                .path = a->path,
                .owner = a->owner,
                .origin = a->origin,
                .attach_path = a->attachpath,
                .barricade = a->barricade,
                .style = styles,
                .style_count = style_count,
                .renum_max = a->renum_max,
                .renum_days = a->renum_days
            };

            MaxCfgStatus st = maxcfg_ng_msg_area_list_add(areas, &area);
            strv_free(&styles, &style_count);
            if (st != MAXCFG_OK) {
                set_err(err, err_sz, maxcfg_status_string(st));
                return false;
            }
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        if (!build_msg_areas_recursive(areas, node->children[i], div_for_children, err, err_sz)) {
            return false;
        }
    }

    return true;
}

/**
 * Convert a TreeNode file area subtree into the typed libmaxcfg NG file area list.
 */
static bool build_file_areas_recursive(MaxCfgNgFileAreaList *areas, TreeNode *node, const char *division_key, char *err, size_t err_sz)
{
    if (areas == NULL || node == NULL) {
        return true;
    }

    const char *div_for_children = division_key;
    if (node->type == TREENODE_DIVISION) {
        div_for_children = node->full_name;
    } else if (node->type == TREENODE_AREA) {
        FileAreaData *a = (FileAreaData *)node->data;
        if (a != NULL) {
            char **types = NULL;
            size_t type_count = 0;
            if (!file_area_types_to_strings(a, &types, &type_count)) {
                strv_free(&types, &type_count);
                set_err(err, err_sz, "Out of memory");
                return false;
            }

            MaxCfgNgFileArea area = {
                .name = a->name,
                .description = a->desc,
                .acs = a->acs,
                .download = a->download,
                .upload = a->upload,
                .filelist = a->filelist,
                .barricade = a->barricade,
                .menu = a->menuname,
                .division = (char *)division_key,
                .types = types,
                .type_count = type_count
            };

            MaxCfgStatus st = maxcfg_ng_file_area_list_add(areas, &area);
            strv_free(&types, &type_count);
            if (st != MAXCFG_OK) {
                set_err(err, err_sz, maxcfg_status_string(st));
                return false;
            }
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        if (!build_file_areas_recursive(areas, node->children[i], div_for_children, err, err_sz)) {
            return false;
        }
    }

    return true;
}

/* Load message areas from TOML */
TreeNode **load_msgarea_toml(MaxCfgToml *toml, int *count, char *err, size_t err_sz)
{
    if (!toml || !count) {
        set_err(err, err_sz, "Invalid arguments");
        return NULL;
    }

    *count = 0;

    /* Use libmaxcfg typed reader to get divisions and areas */
    MaxCfgNgDivisionList divisions = {0};
    MaxCfgNgMsgAreaList areas = {0};
    
    MaxCfgStatus status = maxcfg_ng_get_msg_areas(toml, "areas.msg", &divisions, &areas);
    if (status != MAXCFG_OK) {
        set_err(err, err_sz, "Failed to read message areas from TOML");
        return NULL;
    }

    /* Build division lookup map: key -> TreeNode */
    typedef struct { char *key; TreeNode *node; } DivMap;
    DivMap *div_map = NULL;
    size_t div_map_count = 0;
    
    if (divisions.count > 0) {
        div_map = calloc(divisions.count, sizeof(DivMap));
        if (!div_map) {
            maxcfg_ng_division_list_free(&divisions);
            maxcfg_ng_msg_area_list_free(&areas);
            set_err(err, err_sz, "Out of memory");
            return NULL;
        }
    }

    /* Create division TreeNodes */
    for (size_t i = 0; i < divisions.count; i++) {
        MaxCfgNgDivision *div = &divisions.items[i];
        const char *div_key = (div->key && div->key[0]) ? div->key : div->name;
        
        TreeNode *div_node = treenode_create(
            div->name,
            div_key,
            div->description,
            TREENODE_DIVISION,
            div->level
        );
        
        if (!div_node) {
            /* Cleanup on failure */
            for (size_t j = 0; j < div_map_count; j++) {
                treenode_free(div_map[j].node);
            }
            free(div_map);
            maxcfg_ng_division_list_free(&divisions);
            maxcfg_ng_msg_area_list_free(&areas);
            set_err(err, err_sz, "Failed to create division node");
            return NULL;
        }

        /* Attach division data */
        DivisionData *dd = calloc(1, sizeof(DivisionData));
        if (dd) {
            dd->display_file = dup_str_or_null(div->display_file);
            dd->acs = dup_str_or_null(div->acs);
        }
        div_node->data = dd;

        /* Add to map */
        div_map[div_map_count].key = strdup(div_key);
        div_map[div_map_count].node = div_node;
        div_map_count++;
    }

    /* Attach divisions to their parents based on dotted keys (e.g. "a.b.c") */
    for (size_t i = 0; i < div_map_count; i++) {
        if (div_map[i].key == NULL || div_map[i].node == NULL) {
            continue;
        }

        char *dot = strrchr(div_map[i].key, '.');
        if (dot == NULL) {
            continue;
        }

        size_t parent_len = (size_t)(dot - div_map[i].key);
        if (parent_len == 0u) {
            continue;
        }

        char parent_key[512];
        if (parent_len >= sizeof(parent_key)) {
            continue;
        }
        memcpy(parent_key, div_map[i].key, parent_len);
        parent_key[parent_len] = '\0';

        for (size_t j = 0; j < div_map_count; j++) {
            if (div_map[j].key && strcmp(div_map[j].key, parent_key) == 0) {
                treenode_add_child(div_map[j].node, div_map[i].node);
                break;
            }
        }
    }

    /* Create area TreeNodes and attach to divisions */
    TreeNode **orphan_areas = NULL;
    int orphan_count = 0;
    int orphan_cap = 0;
    for (size_t i = 0; i < areas.count; i++) {
        MaxCfgNgMsgArea *area = &areas.items[i];
        
        /* Find parent division if specified */
        TreeNode *parent_div = NULL;
        if (area->division && area->division[0]) {
            for (size_t j = 0; j < div_map_count; j++) {
                if (strcmp(div_map[j].key, area->division) == 0) {
                    parent_div = div_map[j].node;
                    break;
                }
            }
        }

        /* Build full_name */
        char full_name[512];
        if (parent_div && parent_div->full_name) {
            snprintf(full_name, sizeof(full_name), "%s.%s", parent_div->full_name, area->name);
        } else {
            snprintf(full_name, sizeof(full_name), "%s", area->name);
        }

        TreeNode *area_node = treenode_create(
            area->name,
            full_name,
            area->description,
            TREENODE_AREA,
            parent_div ? (parent_div->division_level + 1) : 0
        );
        
        if (!area_node) {
            /* Cleanup on failure */
            for (size_t j = 0; j < div_map_count; j++) {
                treenode_free(div_map[j].node);
            }
            free(div_map);
            for (int j = 0; j < orphan_count; j++) {
                treenode_free(orphan_areas[j]);
            }
            free(orphan_areas);
            maxcfg_ng_division_list_free(&divisions);
            maxcfg_ng_msg_area_list_free(&areas);
            set_err(err, err_sz, "Failed to create area node");
            return NULL;
        }

        /* Attach area data */
        MsgAreaData *mad = calloc(1, sizeof(MsgAreaData));
        if (mad) {
            mad->name = dup_str_or_null(area->name);
            mad->desc = dup_str_or_null(area->description);
            mad->tag = dup_str_or_null(area->tag);
            mad->path = dup_str_or_null(area->path);
            mad->acs = dup_str_or_null(area->acs);
            mad->owner = dup_str_or_null(area->owner);
            mad->origin = dup_str_or_null(area->origin);
            mad->attachpath = dup_str_or_null(area->attach_path);
            mad->barricade = dup_str_or_null(area->barricade);
            mad->menuname = dup_str_or_null(area->menu);
            mad->renum_max = area->renum_max;
            mad->renum_days = area->renum_days;
            
            /* Convert style strings to bitmask */
            mad->style = msg_style_from_strings(area->style, area->style_count);
        }
        area_node->data = mad;

        /* Attach to parent division or add to roots */
        if (parent_div) {
            treenode_add_child(parent_div, area_node);
        } else {
            if (orphan_count >= orphan_cap) {
                int new_cap = (orphan_cap == 0) ? 8 : (orphan_cap * 2);
                TreeNode **p = realloc(orphan_areas, (size_t)new_cap * sizeof(*p));
                if (!p) {
                    treenode_free(area_node);
                    set_err(err, err_sz, "Out of memory");
                    for (size_t j = 0; j < div_map_count; j++) {
                        treenode_free(div_map[j].node);
                    }
                    free(div_map);
                    for (int j = 0; j < orphan_count; j++) {
                        treenode_free(orphan_areas[j]);
                    }
                    free(orphan_areas);
                    maxcfg_ng_division_list_free(&divisions);
                    maxcfg_ng_msg_area_list_free(&areas);
                    return NULL;
                }
                orphan_areas = p;
                orphan_cap = new_cap;
            }
            orphan_areas[orphan_count++] = area_node;
        }
    }

    /* Build roots array from top-level divisions + orphan areas */
    TreeNode **roots = NULL;
    int root_count = 0;

    int root_div_count = 0;
    for (size_t i = 0; i < div_map_count; i++) {
        if (div_map[i].node && div_map[i].node->parent == NULL) {
            root_div_count++;
        }
    }

    int total_roots = root_div_count + orphan_count;
    if (total_roots == 0) {
        roots = calloc(1u, sizeof(TreeNode *));
    } else {
        roots = calloc((size_t)total_roots, sizeof(TreeNode *));
    }

    if (roots == NULL) {
        for (size_t j = 0; j < div_map_count; j++) {
            treenode_free(div_map[j].node);
        }
        free(div_map);
        for (int j = 0; j < orphan_count; j++) {
            treenode_free(orphan_areas[j]);
        }
        free(orphan_areas);
        maxcfg_ng_division_list_free(&divisions);
        maxcfg_ng_msg_area_list_free(&areas);
        set_err(err, err_sz, "Out of memory");
        return NULL;
    }

    for (size_t i = 0; i < div_map_count; i++) {
        if (div_map[i].node && div_map[i].node->parent == NULL) {
            roots[root_count++] = div_map[i].node;
        }
    }
    for (int i = 0; i < orphan_count; i++) {
        roots[root_count++] = orphan_areas[i];
    }

    /* Cleanup */
    for (size_t i = 0; i < div_map_count; i++) {
        free(div_map[i].key);
    }
    free(div_map);
    free(orphan_areas);
    maxcfg_ng_division_list_free(&divisions);
    maxcfg_ng_msg_area_list_free(&areas);

    *count = root_count;
    return roots;
}

/* Save message areas to TOML */
bool save_msgarea_toml(MaxCfgToml *toml, const char *toml_path, TreeNode **roots, int count, char *err, size_t err_sz)
{
    if (!toml || !toml_path || !roots) {
        set_err(err, err_sz, "Invalid arguments");
        return false;
    }

    MaxCfgNgDivisionList divisions = {0};
    MaxCfgNgMsgAreaList areas = {0};
    (void)maxcfg_ng_division_list_init(&divisions);
    (void)maxcfg_ng_msg_area_list_init(&areas);

    for (int i = 0; i < count; i++) {
        if (!roots[i]) continue;
        if (!build_msg_divisions_recursive(&divisions, roots[i], err, err_sz)) {
            maxcfg_ng_division_list_free(&divisions);
            maxcfg_ng_msg_area_list_free(&areas);
            return false;
        }
    }

    for (int i = 0; i < count; i++) {
        if (!roots[i]) continue;
        if (!build_msg_areas_recursive(&areas, roots[i], NULL, err, err_sz)) {
            maxcfg_ng_division_list_free(&divisions);
            maxcfg_ng_msg_area_list_free(&areas);
            return false;
        }
    }

    FILE *fp = fopen(toml_path, "w");
    if (!fp) {
        set_err(err, err_sz, "Failed to open TOML file for writing");
        maxcfg_ng_division_list_free(&divisions);
        maxcfg_ng_msg_area_list_free(&areas);
        return false;
    }

    MaxCfgStatus st = maxcfg_ng_write_msg_areas_toml(fp, &divisions, &areas);
    fclose(fp);
    maxcfg_ng_division_list_free(&divisions);
    maxcfg_ng_msg_area_list_free(&areas);

    if (st != MAXCFG_OK) {
        set_err(err, err_sz, maxcfg_status_string(st));
        return false;
    }

    st = maxcfg_toml_load_file(toml, toml_path, "areas.msg");
    if (st != MAXCFG_OK) {
        set_err(err, err_sz, maxcfg_status_string(st));
        return false;
    }

    return true;
}

/**
 * Convert file area type strings (TOML) into FileAreaData flags.
 */
static void file_area_types_from_strings(FileAreaData *a, char **types, size_t count)
{
    if (a == NULL) {
        return;
    }

    a->type_slow = false;
    a->type_staged = false;
    a->type_nonew = false;

    for (size_t i = 0; i < count; i++) {
        const char *t = types[i];
        if (t == NULL || t[0] == '\0') {
            continue;
        }
        if (strcasecmp(t, "CD") == 0) {
            a->type_slow = true;
            a->type_staged = true;
            a->type_nonew = true;
            return;
        }
        if (strcasecmp(t, "Slow") == 0) {
            a->type_slow = true;
        } else if (strcasecmp(t, "Staged") == 0) {
            a->type_staged = true;
        } else if (strcasecmp(t, "NoNew") == 0) {
            a->type_nonew = true;
        }
    }
}

/**
 * Convert FileAreaData flags into TOML-friendly type string list.
 */
static bool file_area_types_to_strings(const FileAreaData *a, char ***out_types, size_t *out_count)
{
    *out_types = NULL;
    *out_count = 0;

    if (a == NULL) {
        return true;
    }

    if (a->type_slow && a->type_staged && a->type_nonew) {
        return strv_add(out_types, out_count, "CD");
    }

    if (a->type_slow) { if (!strv_add(out_types, out_count, "Slow")) return false; }
    if (a->type_staged) { if (!strv_add(out_types, out_count, "Staged")) return false; }
    if (a->type_nonew) { if (!strv_add(out_types, out_count, "NoNew")) return false; }

    return true;
}

/* Load file areas from TOML */
TreeNode **load_filearea_toml(MaxCfgToml *toml, int *count, char *err, size_t err_sz)
{
    if (!toml || !count) {
        set_err(err, err_sz, "Invalid arguments");
        return NULL;
    }

    *count = 0;

    /* Use libmaxcfg typed reader to get divisions and areas */
    MaxCfgNgDivisionList divisions = {0};
    MaxCfgNgFileAreaList areas = {0};

    MaxCfgStatus status = maxcfg_ng_get_file_areas(toml, "areas.file", &divisions, &areas);
    if (status != MAXCFG_OK) {
        set_err(err, err_sz, "Failed to read file areas from TOML");
        return NULL;
    }

    /* Build division lookup map: key -> TreeNode */
    typedef struct { char *key; TreeNode *node; } DivMap;
    DivMap *div_map = NULL;
    size_t div_map_count = 0;

    if (divisions.count > 0) {
        div_map = calloc(divisions.count, sizeof(DivMap));
        if (!div_map) {
            maxcfg_ng_division_list_free(&divisions);
            maxcfg_ng_file_area_list_free(&areas);
            set_err(err, err_sz, "Out of memory");
            return NULL;
        }
    }

    /* Create division TreeNodes */
    for (size_t i = 0; i < divisions.count; i++) {
        MaxCfgNgDivision *div = &divisions.items[i];
        const char *div_key = (div->key && div->key[0]) ? div->key : div->name;

        TreeNode *div_node = treenode_create(div->name, div_key, div->description, TREENODE_DIVISION, div->level);
        if (!div_node) {
            for (size_t j = 0; j < div_map_count; j++) {
                treenode_free(div_map[j].node);
            }
            free(div_map);
            maxcfg_ng_division_list_free(&divisions);
            maxcfg_ng_file_area_list_free(&areas);
            set_err(err, err_sz, "Failed to create division node");
            return NULL;
        }

        DivisionData *dd = calloc(1, sizeof(DivisionData));
        if (dd) {
            dd->display_file = dup_str_or_null(div->display_file);
            dd->acs = dup_str_or_null(div->acs);
        }
        div_node->data = dd;

        div_map[div_map_count].key = strdup(div_key);
        div_map[div_map_count].node = div_node;
        div_map_count++;
    }

    /* Attach divisions to their parents based on dotted keys */
    for (size_t i = 0; i < div_map_count; i++) {
        if (div_map[i].key == NULL || div_map[i].node == NULL) {
            continue;
        }
        char *dot = strrchr(div_map[i].key, '.');
        if (dot == NULL) {
            continue;
        }

        size_t parent_len = (size_t)(dot - div_map[i].key);
        if (parent_len == 0u) {
            continue;
        }

        char parent_key[512];
        if (parent_len >= sizeof(parent_key)) {
            continue;
        }
        memcpy(parent_key, div_map[i].key, parent_len);
        parent_key[parent_len] = '\0';

        for (size_t j = 0; j < div_map_count; j++) {
            if (div_map[j].key && strcmp(div_map[j].key, parent_key) == 0) {
                treenode_add_child(div_map[j].node, div_map[i].node);
                break;
            }
        }
    }

    /* Create area TreeNodes */
    TreeNode **orphan_areas = NULL;
    int orphan_count = 0;
    int orphan_cap = 0;

    for (size_t i = 0; i < areas.count; i++) {
        MaxCfgNgFileArea *area = &areas.items[i];

        TreeNode *parent_div = NULL;
        if (area->division && area->division[0]) {
            for (size_t j = 0; j < div_map_count; j++) {
                if (strcmp(div_map[j].key, area->division) == 0) {
                    parent_div = div_map[j].node;
                    break;
                }
            }
        }

        char full_name[512];
        if (parent_div && parent_div->full_name) {
            snprintf(full_name, sizeof(full_name), "%s.%s", parent_div->full_name, area->name);
        } else {
            snprintf(full_name, sizeof(full_name), "%s", area->name);
        }

        TreeNode *area_node = treenode_create(area->name, full_name, area->description, TREENODE_AREA,
                                              parent_div ? (parent_div->division_level + 1) : 0);
        if (!area_node) {
            for (size_t j = 0; j < div_map_count; j++) {
                treenode_free(div_map[j].node);
            }
            free(div_map);
            for (int j = 0; j < orphan_count; j++) {
                treenode_free(orphan_areas[j]);
            }
            free(orphan_areas);
            maxcfg_ng_division_list_free(&divisions);
            maxcfg_ng_file_area_list_free(&areas);
            set_err(err, err_sz, "Failed to create file area node");
            return NULL;
        }

        FileAreaData *fad = calloc(1, sizeof(FileAreaData));
        if (fad) {
            fad->name = dup_str_or_null(area->name);
            fad->desc = dup_str_or_null(area->description);
            fad->acs = dup_str_or_null(area->acs);
            fad->download = dup_str_or_null(area->download);
            fad->upload = dup_str_or_null(area->upload);
            fad->filelist = dup_str_or_null(area->filelist);
            fad->barricade = dup_str_or_null(area->barricade);
            fad->menuname = dup_str_or_null(area->menu);
            file_area_types_from_strings(fad, area->types, area->type_count);
        }
        area_node->data = fad;

        if (parent_div) {
            treenode_add_child(parent_div, area_node);
        } else {
            if (orphan_count >= orphan_cap) {
                int new_cap = (orphan_cap == 0) ? 8 : (orphan_cap * 2);
                TreeNode **p = realloc(orphan_areas, (size_t)new_cap * sizeof(*p));
                if (!p) {
                    treenode_free(area_node);
                    set_err(err, err_sz, "Out of memory");
                    for (size_t j = 0; j < div_map_count; j++) {
                        treenode_free(div_map[j].node);
                    }
                    free(div_map);
                    for (int j = 0; j < orphan_count; j++) {
                        treenode_free(orphan_areas[j]);
                    }
                    free(orphan_areas);
                    maxcfg_ng_division_list_free(&divisions);
                    maxcfg_ng_file_area_list_free(&areas);
                    return NULL;
                }
                orphan_areas = p;
                orphan_cap = new_cap;
            }
            orphan_areas[orphan_count++] = area_node;
        }
    }

    int root_div_count = 0;
    for (size_t i = 0; i < div_map_count; i++) {
        if (div_map[i].node && div_map[i].node->parent == NULL) {
            root_div_count++;
        }
    }

    int total_roots = root_div_count + orphan_count;
    TreeNode **roots = (total_roots == 0) ? calloc(1u, sizeof(TreeNode *)) : calloc((size_t)total_roots, sizeof(TreeNode *));
    if (roots == NULL) {
        for (size_t j = 0; j < div_map_count; j++) {
            treenode_free(div_map[j].node);
        }
        free(div_map);
        for (int j = 0; j < orphan_count; j++) {
            treenode_free(orphan_areas[j]);
        }
        free(orphan_areas);
        maxcfg_ng_division_list_free(&divisions);
        maxcfg_ng_file_area_list_free(&areas);
        set_err(err, err_sz, "Out of memory");
        return NULL;
    }

    int root_count = 0;
    for (size_t i = 0; i < div_map_count; i++) {
        if (div_map[i].node && div_map[i].node->parent == NULL) {
            roots[root_count++] = div_map[i].node;
        }
    }
    for (int i = 0; i < orphan_count; i++) {
        roots[root_count++] = orphan_areas[i];
    }

    for (size_t i = 0; i < div_map_count; i++) {
        free(div_map[i].key);
    }
    free(div_map);
    free(orphan_areas);
    maxcfg_ng_division_list_free(&divisions);
    maxcfg_ng_file_area_list_free(&areas);

    *count = root_count;
    return roots;
}

/* Save file areas to TOML */
bool save_filearea_toml(MaxCfgToml *toml, const char *toml_path, TreeNode **roots, int count, char *err, size_t err_sz)
{
    if (!toml || !toml_path || !roots) {
        set_err(err, err_sz, "Invalid arguments");
        return false;
    }

    MaxCfgNgDivisionList divisions = {0};
    MaxCfgNgFileAreaList areas = {0};
    (void)maxcfg_ng_division_list_init(&divisions);
    (void)maxcfg_ng_file_area_list_init(&areas);

    for (int i = 0; i < count; i++) {
        if (!roots[i]) continue;
        if (!build_msg_divisions_recursive(&divisions, roots[i], err, err_sz)) {
            maxcfg_ng_division_list_free(&divisions);
            maxcfg_ng_file_area_list_free(&areas);
            return false;
        }
    }

    for (int i = 0; i < count; i++) {
        if (!roots[i]) continue;
        if (!build_file_areas_recursive(&areas, roots[i], NULL, err, err_sz)) {
            maxcfg_ng_division_list_free(&divisions);
            maxcfg_ng_file_area_list_free(&areas);
            return false;
        }
    }

    FILE *fp = fopen(toml_path, "w");
    if (!fp) {
        set_err(err, err_sz, "Failed to open TOML file for writing");
        maxcfg_ng_division_list_free(&divisions);
        maxcfg_ng_file_area_list_free(&areas);
        return false;
    }

    MaxCfgStatus st = maxcfg_ng_write_file_areas_toml(fp, &divisions, &areas);
    fclose(fp);
    maxcfg_ng_division_list_free(&divisions);
    maxcfg_ng_file_area_list_free(&areas);

    if (st != MAXCFG_OK) {
        set_err(err, err_sz, maxcfg_status_string(st));
        return false;
    }

    st = maxcfg_toml_load_file(toml, toml_path, "areas.file");
    if (st != MAXCFG_OK) {
        set_err(err, err_sz, maxcfg_status_string(st));
        return false;
    }

    return true;
}
