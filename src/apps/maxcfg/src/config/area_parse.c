/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * area_parse.c - Message and file area CTL parser
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "area_parse.h"

/* Helper: trim whitespace from both ends */
static char *trim(char *str)
{
    char *end;
    while (isspace((unsigned char)*str) || (unsigned char)*str == 0xA0) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && (isspace((unsigned char)*end) || (unsigned char)*end == 0xA0)) end--;
    end[1] = '\0';
    return str;
}

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
    if (*p != '\0' && !(isspace((unsigned char)*p) || (unsigned char)*p == 0xA0)) {
        return false;
    }

    while (*p != '\0' && (isspace((unsigned char)*p) || (unsigned char)*p == 0xA0)) {
        p++;
    }

    if (out_value) {
        *out_value = p;
    }
    return true;
}

void division_data_free(DivisionData *data)
{
    if (!data) return;
    free(data->acs);
    free(data->display_file);
    free(data);
}

static void free_tree_data_recursive(TreeNode *node, bool is_msg)
{
    if (!node) return;

    for (int i = 0; i < node->child_count; i++) {
        free_tree_data_recursive(node->children[i], is_msg);
    }

    if (node->type == TREENODE_DIVISION && node->data) {
        division_data_free((DivisionData *)node->data);
        node->data = NULL;
    } else if (node->type == TREENODE_AREA && node->data) {
        if (is_msg) msgarea_data_free((MsgAreaData *)node->data);
        else filearea_data_free((FileAreaData *)node->data);
        node->data = NULL;
    }
}

void free_msg_tree(TreeNode **roots, int count)
{
    if (!roots) return;
    for (int i = 0; i < count; i++) {
        free_tree_data_recursive(roots[i], true);
        treenode_free(roots[i]);
    }
    free(roots);
}

void free_file_tree(TreeNode **roots, int count)
{
    if (!roots) return;
    for (int i = 0; i < count; i++) {
        free_tree_data_recursive(roots[i], false);
        treenode_free(roots[i]);
    }
    free(roots);
}

/* Helper: parse Style keyword into flags */
static unsigned int parse_style(const char *style_str)
{
    unsigned int flags = 0;
    char *copy = strdup(style_str);
    char *token = strtok(copy, " \t");
    
    while (token) {
        if (strcasecmp(token, "Squish") == 0) flags |= MSGSTYLE_SQUISH;
        else if (strcasecmp(token, "*.MSG") == 0) flags |= MSGSTYLE_DOTMSG;
        else if (strcasecmp(token, "Local") == 0) flags |= MSGSTYLE_LOCAL;
        else if (strcasecmp(token, "Net") == 0) flags |= MSGSTYLE_NET;
        else if (strcasecmp(token, "Echo") == 0) flags |= MSGSTYLE_ECHO;
        else if (strcasecmp(token, "Conf") == 0) flags |= MSGSTYLE_CONF;
        else if (strcasecmp(token, "Pvt") == 0) flags |= MSGSTYLE_PVT;
        else if (strcasecmp(token, "Pub") == 0) flags |= MSGSTYLE_PUB;
        else if (strcasecmp(token, "HiBit") == 0 || strcasecmp(token, "HighBit") == 0) flags |= MSGSTYLE_HIBIT;
        else if (strcasecmp(token, "Anon") == 0) flags |= MSGSTYLE_ANON;
        else if (strcasecmp(token, "NoNameKludge") == 0) flags |= MSGSTYLE_NORNK;
        else if (strcasecmp(token, "RealName") == 0) flags |= MSGSTYLE_REALNAME;
        else if (strcasecmp(token, "Alias") == 0) flags |= MSGSTYLE_ALIAS;
        else if (strcasecmp(token, "Audit") == 0) flags |= MSGSTYLE_AUDIT;
        else if (strcasecmp(token, "ReadOnly") == 0) flags |= MSGSTYLE_READONLY;
        else if (strcasecmp(token, "Hidden") == 0) flags |= MSGSTYLE_HIDDEN;
        else if (strcasecmp(token, "Attach") == 0) flags |= MSGSTYLE_ATTACH;
        else if (strcasecmp(token, "NoMailCheck") == 0) flags |= MSGSTYLE_NOMAILCHK;
        
        token = strtok(NULL, " \t");
    }
    
    free(copy);
    return flags;
}

/* Parse msgarea.ctl */
TreeNode **parse_msgarea_ctl(const char *sys_path, int *count, char *err, size_t err_sz)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/etc/msgarea.ctl", sys_path);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        if (err && err_sz) snprintf(err, err_sz, "Cannot open %s", path);
        *count = 0;
        return NULL;
    }
    
    /* First pass: count areas and divisions */
    char line[1024];
    int area_count = 0;
    int div_count = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim(line);
        if (strncasecmp(trimmed, "MsgArea ", 8) == 0) area_count++;
        if (strncasecmp(trimmed, "MsgDivisionBegin ", 17) == 0) div_count++;
    }
    
    /* Allocate arrays */
    MsgAreaData **areas = calloc(area_count, sizeof(MsgAreaData*));
    TreeNode **divisions = calloc(div_count, sizeof(TreeNode*));
    TreeNode **roots = calloc(area_count + div_count, sizeof(TreeNode*));
    
    /* Second pass: parse areas and divisions */
    rewind(fp);
    
    int area_idx = 0;
    int div_idx = 0;
    int root_idx = 0;
    TreeNode *current_div = NULL;
    MsgAreaData *current_area = NULL;
    bool in_area = false;
    
    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim(line);
        
        /* Skip comments and empty lines */
        if (trimmed[0] == '%' || trimmed[0] == '\0') continue;
        
        /* Division begin */
        if (strncasecmp(trimmed, "MsgDivisionBegin ", 17) == 0) {
            char *rest = trimmed + 17;
            char name[128], acs[128], display_file[128], desc[256];
            name[0] = acs[0] = display_file[0] = desc[0] = '\0';
            sscanf(rest, "%127s %127s %127s %255[^\n]", name, acs, display_file, desc);

            char full_name[512];
            if (current_div && current_div->full_name && current_div->full_name[0]) {
                snprintf(full_name, sizeof(full_name), "%s.%s", current_div->full_name, name);
            } else {
                snprintf(full_name, sizeof(full_name), "%s", name);
            }

            TreeNode *div_node = treenode_create(name, full_name, desc, TREENODE_DIVISION,
                                                 current_div ? (current_div->division_level + 1) : 0);
            if (div_node) {
                DivisionData *div_data = calloc(1, sizeof(DivisionData));
                if (div_data) {
                    div_data->acs = (acs[0] ? strdup(acs) : NULL);
                    div_data->display_file = (display_file[0] ? strdup(display_file) : NULL);
                    div_node->data = div_data;
                }

                divisions[div_idx++] = div_node;
                if (current_div) {
                    treenode_add_child(current_div, div_node);
                } else {
                    roots[root_idx++] = div_node;
                }
                current_div = div_node;
            }
            continue;
        }

        /* Division end */
        if (strncasecmp(trimmed, "MsgDivisionEnd", 14) == 0) {
            if (current_div) current_div = current_div->parent;
            continue;
        }
        
        /* Area begin */
        if (strncasecmp(trimmed, "MsgArea ", 8) == 0) {
            char *name = trim(trimmed + 8);
            current_area = calloc(1, sizeof(MsgAreaData));
            current_area->name = strdup(name);
            current_area->style = MSGSTYLE_SQUISH | MSGSTYLE_LOCAL | MSGSTYLE_PUB; /* defaults */
            areas[area_idx++] = current_area;
            in_area = true;
            continue;
        }
        
        /* Area end */
        if (strncasecmp(trimmed, "End MsgArea", 11) == 0) {
            if (current_area) {
                /* Create tree node for this area */
                char desc_buf[128];
                snprintf(desc_buf, sizeof(desc_buf), "%s", 
                         current_area->desc ? current_area->desc : current_area->name);
                
                char full_name[512];
                if (current_div && current_div->full_name && current_div->full_name[0]) {
                    snprintf(full_name, sizeof(full_name), "%s.%s", current_div->full_name, current_area->name);
                } else {
                    snprintf(full_name, sizeof(full_name), "%s", current_area->name);
                }

                TreeNode *area_node = treenode_create(
                    current_area->name,
                    full_name,
                    desc_buf,
                    TREENODE_AREA,
                    current_div ? (current_div->division_level + 1) : 0
                );
                
                /* Attach data */
                area_node->data = current_area;
                
                /* Add to tree */
                if (current_div) {
                    treenode_add_child(current_div, area_node);
                } else {
                    roots[root_idx++] = area_node;
                }
                
                current_area = NULL;
            }
            in_area = false;
            continue;
        }
        
        /* Parse area keywords */
        if (in_area && current_area) {
            const char *v;
            if (kw_value(trimmed, "Desc", &v)) {
                current_area->desc = strdup(v);
            }
            else if (kw_value(trimmed, "Path", &v)) {
                current_area->path = strdup(v);
            }
            else if (kw_value(trimmed, "Tag", &v)) {
                current_area->tag = strdup(v);
            }
            else if (kw_value(trimmed, "ACS", &v)) {
                current_area->acs = strdup(v);
            }
            else if (kw_value(trimmed, "Owner", &v)) {
                current_area->owner = strdup(v);
            }
            else if (kw_value(trimmed, "Origin", &v)) {
                current_area->origin = strdup(v);
            }
            else if (kw_value(trimmed, "AttachPath", &v)) {
                current_area->attachpath = strdup(v);
            }
            else if (kw_value(trimmed, "Barricade", &v)) {
                current_area->barricade = strdup(v);
            }
            else if (kw_value(trimmed, "MenuName", &v)) {
                current_area->menuname = strdup(v);
            }
            else if (kw_value(trimmed, "Style", &v)) {
                current_area->style = parse_style(v);
            }
            else if (kw_value(trimmed, "Renum Max", &v)) {
                current_area->renum_max = atoi(v);
            }
            else if (kw_value(trimmed, "Renum Days", &v)) {
                current_area->renum_days = atoi(v);
            }
        }
    }
    
    fclose(fp);
    free(areas);
    free(divisions);
    
    *count = root_idx;
    return roots;
}

/* Parse filearea.ctl */
TreeNode **parse_filearea_ctl(const char *sys_path, int *count, char *err, size_t err_sz)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/etc/filearea.ctl", sys_path);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        if (err && err_sz) snprintf(err, err_sz, "Cannot open %s", path);
        *count = 0;
        return NULL;
    }
    
    /* First pass: count areas and divisions */
    char line[1024];
    int area_count = 0;
    int div_count = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim(line);
        if (strncasecmp(trimmed, "FileArea ", 9) == 0) area_count++;
        if (strncasecmp(trimmed, "FileDivisionBegin ", 18) == 0) div_count++;
    }
    
    /* Allocate arrays */
    FileAreaData **areas = calloc(area_count, sizeof(FileAreaData*));
    TreeNode **divisions = calloc(div_count, sizeof(TreeNode*));
    TreeNode **roots = calloc(area_count + div_count, sizeof(TreeNode*));
    
    /* Second pass: parse areas */
    rewind(fp);
    
    int area_idx = 0;
    int div_idx = 0;
    int root_idx = 0;
    TreeNode *current_div = NULL;
    FileAreaData *current_area = NULL;
    bool in_area = false;
    
    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim(line);
        
        /* Skip comments and empty lines */
        if (trimmed[0] == '%' || trimmed[0] == '\0') continue;
        
        if (strncasecmp(trimmed, "FileDivisionBegin ", 18) == 0) {
            char *rest = trimmed + 18;
            char name[128], acs[128], display_file[128], desc[256];
            name[0] = acs[0] = display_file[0] = desc[0] = '\0';
            sscanf(rest, "%127s %127s %127s %255[^\n]", name, acs, display_file, desc);

            char full_name[512];
            if (current_div && current_div->full_name && current_div->full_name[0]) {
                snprintf(full_name, sizeof(full_name), "%s.%s", current_div->full_name, name);
            } else {
                snprintf(full_name, sizeof(full_name), "%s", name);
            }

            TreeNode *div_node = treenode_create(name, full_name, desc, TREENODE_DIVISION,
                                                 current_div ? (current_div->division_level + 1) : 0);
            if (div_node) {
                DivisionData *div_data = calloc(1, sizeof(DivisionData));
                if (div_data) {
                    div_data->acs = (acs[0] ? strdup(acs) : NULL);
                    div_data->display_file = (display_file[0] ? strdup(display_file) : NULL);
                    div_node->data = div_data;
                }
                divisions[div_idx++] = div_node;
                if (current_div) {
                    treenode_add_child(current_div, div_node);
                } else {
                    roots[root_idx++] = div_node;
                }
                current_div = div_node;
            }
            continue;
        }

        if (strncasecmp(trimmed, "FileDivisionEnd", 15) == 0) {
            if (current_div) current_div = current_div->parent;
            continue;
        }

        /* Area begin */
        if (strncasecmp(trimmed, "FileArea ", 9) == 0) {
            char *name = trim(trimmed + 9);
            current_area = calloc(1, sizeof(FileAreaData));
            current_area->name = strdup(name);
            areas[area_idx++] = current_area;
            in_area = true;
            continue;
        }
        
        /* Area end */
        if (strncasecmp(trimmed, "End FileArea", 12) == 0) {
            if (current_area) {
                /* Create tree node for this area */
                char desc_buf[128];
                snprintf(desc_buf, sizeof(desc_buf), "%s", 
                         current_area->desc ? current_area->desc : current_area->name);
                
                char full_name[512];
                if (current_div && current_div->full_name && current_div->full_name[0]) {
                    snprintf(full_name, sizeof(full_name), "%s.%s", current_div->full_name, current_area->name);
                } else {
                    snprintf(full_name, sizeof(full_name), "%s", current_area->name);
                }

                TreeNode *area_node = treenode_create(
                    current_area->name,
                    full_name,
                    desc_buf,
                    TREENODE_AREA,
                    current_div ? (current_div->division_level + 1) : 0
                );
                
                /* Attach data */
                area_node->data = current_area;
                
                if (current_div) {
                    treenode_add_child(current_div, area_node);
                } else {
                    roots[root_idx++] = area_node;
                }
                current_area = NULL;
            }
            in_area = false;
            continue;
        }
        
        /* Parse area keywords */
        if (in_area && current_area) {
            const char *v;
            if (kw_value(trimmed, "Desc", &v)) {
                current_area->desc = strdup(v);
            }
            else if (kw_value(trimmed, "Download", &v)) {
                current_area->download = strdup(v);
            }
            else if (kw_value(trimmed, "Upload", &v)) {
                current_area->upload = strdup(v);
            }
            else if (kw_value(trimmed, "ACS", &v)) {
                current_area->acs = strdup(v);
            }
            else if (kw_value(trimmed, "FileList", &v)) {
                current_area->filelist = strdup(v);
            }
            else if (kw_value(trimmed, "Barricade", &v)) {
                current_area->barricade = strdup(v);
            }
            else if (kw_value(trimmed, "MenuName", &v)) {
                current_area->menuname = strdup(v);
            }
            else if (kw_value(trimmed, "Type", &v)) {
                char *type_str = trim((char *)v);
                if (strstr(type_str, "Slow")) current_area->type_slow = true;
                if (strstr(type_str, "Staged")) current_area->type_staged = true;
                if (strstr(type_str, "NoNew")) current_area->type_nonew = true;
                if (strcasecmp(type_str, "CD") == 0) {
                    current_area->type_slow = true;
                    current_area->type_staged = true;
                    current_area->type_nonew = true;
                }
            }
        }
    }
    
    fclose(fp);
    free(areas);
    free(divisions);
    
    *count = root_idx;
    return roots;
}

/* Free area data */
void msgarea_data_free(MsgAreaData *data)
{
    if (!data) return;
    free(data->name);
    free(data->tag);
    free(data->path);
    free(data->desc);
    free(data->acs);
    free(data->owner);
    free(data->origin);
    free(data->attachpath);
    free(data->barricade);
    free(data->menuname);
    free(data);
}

void filearea_data_free(FileAreaData *data)
{
    if (!data) return;
    free(data->name);
    free(data->desc);
    free(data->acs);
    free(data->download);
    free(data->upload);
    free(data->filelist);
    free(data->barricade);
    free(data->menuname);
    free(data);
}

#if 0
/* Helper: create .bak backup if it doesn't exist */
static bool create_backup(const char *path, char *err, size_t err_sz)
{
    char bak_path[1024];
    snprintf(bak_path, sizeof(bak_path), "%s.bak", path);
    
    /* Check if .bak already exists */
    if (access(bak_path, F_OK) == 0) {
        return true; /* Backup already exists, don't overwrite */
    }
    
    /* Copy original to .bak */
    FILE *src = fopen(path, "r");
    if (!src) {
        if (err && err_sz) snprintf(err, err_sz, "Cannot open %s for backup", path);
        return false;
    }
    
    FILE *dst = fopen(bak_path, "w");
    if (!dst) {
        fclose(src);
        if (err && err_sz) snprintf(err, err_sz, "Cannot create backup %s", bak_path);
        return false;
    }
    
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n) {
            fclose(src);
            fclose(dst);
            unlink(bak_path);
            if (err && err_sz) snprintf(err, err_sz, "Failed to write backup");
            return false;
        }
    }
    
    fclose(src);
    fclose(dst);
    return true;
}

/* Helper: format Style flags back to string */
static void format_style(unsigned int style, char *buf, size_t buf_sz)
{
    buf[0] = '\0';
    
    /* Format */
    if (style & MSGSTYLE_SQUISH) strcat(buf, "Squish ");
    else if (style & MSGSTYLE_DOTMSG) strcat(buf, "*.MSG ");
    
    /* Type */
    if (style & MSGSTYLE_LOCAL) strcat(buf, "Local ");
    else if (style & MSGSTYLE_NET) strcat(buf, "Net ");
    else if (style & MSGSTYLE_ECHO) strcat(buf, "Echo ");
    else if (style & MSGSTYLE_CONF) strcat(buf, "Conf ");
    
    /* Visibility */
    if (style & MSGSTYLE_PVT) strcat(buf, "Pvt ");
    if (style & MSGSTYLE_PUB) strcat(buf, "Pub ");
    
    /* Options */
    if (style & MSGSTYLE_HIBIT) strcat(buf, "HiBit ");
    if (style & MSGSTYLE_ANON) strcat(buf, "Anon ");
    if (style & MSGSTYLE_NORNK) strcat(buf, "NoNameKludge ");
    if (style & MSGSTYLE_REALNAME) strcat(buf, "RealName ");
    if (style & MSGSTYLE_ALIAS) strcat(buf, "Alias ");
    if (style & MSGSTYLE_AUDIT) strcat(buf, "Audit ");
    if (style & MSGSTYLE_READONLY) strcat(buf, "ReadOnly ");
    if (style & MSGSTYLE_HIDDEN) strcat(buf, "Hidden ");
    if (style & MSGSTYLE_ATTACH) strcat(buf, "Attach ");
    if (style & MSGSTYLE_NOMAILCHK) strcat(buf, "NoMailCheck ");
    
    /* Trim trailing space */
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == ' ') buf[len-1] = '\0';
}

/* Helper: write message area to file */
static void write_indent(FILE *fp, int indent)
{
    for (int i = 0; i < indent; i++) fputc('\t', fp);
}

static void write_msgarea(FILE *fp, MsgAreaData *area, int indent)
{
    write_indent(fp, indent);
    fprintf(fp, "MsgArea %s\n", area->name);
    if (area->acs) { write_indent(fp, indent + 1); fprintf(fp, "ACS\t%s\n", area->acs); }
    if (area->desc) { write_indent(fp, indent + 1); fprintf(fp, "Desc\t%s\n", area->desc); }
    
    char style_buf[512];
    format_style(area->style, style_buf, sizeof(style_buf));
    if (style_buf[0]) { write_indent(fp, indent + 1); fprintf(fp, "Style\t%s\n", style_buf); }
    
    if (area->path) { write_indent(fp, indent + 1); fprintf(fp, "Path\t%s\n", area->path); }
    if (area->tag) { write_indent(fp, indent + 1); fprintf(fp, "Tag\t%s\n", area->tag); }
    if (area->owner) { write_indent(fp, indent + 1); fprintf(fp, "Owner\t%s\n", area->owner); }
    if (area->origin) { write_indent(fp, indent + 1); fprintf(fp, "Origin\t%s\n", area->origin); }
    if (area->attachpath) { write_indent(fp, indent + 1); fprintf(fp, "AttachPath\t%s\n", area->attachpath); }
    if (area->barricade) { write_indent(fp, indent + 1); fprintf(fp, "Barricade\t%s\n", area->barricade); }
    if (area->menuname) { write_indent(fp, indent + 1); fprintf(fp, "MenuName\t%s\n", area->menuname); }
    if (area->renum_max > 0) { write_indent(fp, indent + 1); fprintf(fp, "Renum Max\t%d\n", area->renum_max); }
    if (area->renum_days > 0) { write_indent(fp, indent + 1); fprintf(fp, "Renum Days\t%d\n", area->renum_days); }

    write_indent(fp, indent);
    fprintf(fp, "End MsgArea\n\n");
}

/* Helper: write file area to file */
static void write_filearea(FILE *fp, FileAreaData *area, int indent)
{
    write_indent(fp, indent);
    fprintf(fp, "FileArea %s\n", area->name);
    if (area->acs) { write_indent(fp, indent + 1); fprintf(fp, "ACS\t%s\n", area->acs); }
    if (area->desc) { write_indent(fp, indent + 1); fprintf(fp, "Desc\t%s\n", area->desc); }
    if (area->download) { write_indent(fp, indent + 1); fprintf(fp, "Download\t%s\n", area->download); }
    if (area->upload) { write_indent(fp, indent + 1); fprintf(fp, "Upload\t%s\n", area->upload); }
    if (area->filelist) { write_indent(fp, indent + 1); fprintf(fp, "FileList\t%s\n", area->filelist); }
    if (area->barricade) { write_indent(fp, indent + 1); fprintf(fp, "Barricade\t%s\n", area->barricade); }
    if (area->menuname) { write_indent(fp, indent + 1); fprintf(fp, "MenuName\t%s\n", area->menuname); }
    
    if (area->type_slow || area->type_staged || area->type_nonew) {
        write_indent(fp, indent + 1);
        fprintf(fp, "Type\t");
        if (area->type_slow && area->type_staged && area->type_nonew) {
            fprintf(fp, "CD");
        } else {
            if (area->type_slow) fprintf(fp, "Slow ");
            if (area->type_staged) fprintf(fp, "Staged ");
            if (area->type_nonew) fprintf(fp, "NoNew");
        }
        fprintf(fp, "\n");
    }
    
    write_indent(fp, indent);
    fprintf(fp, "End FileArea\n\n");
}

static void write_msg_division_recursive(FILE *fp, TreeNode *div, int indent)
{
    if (!div || div->type != TREENODE_DIVISION) return;

    DivisionData *dd = (DivisionData *)div->data;
    const char *acs = (dd && dd->acs && dd->acs[0]) ? dd->acs : "Demoted";
    const char *display_file = (dd && dd->display_file && dd->display_file[0]) ? dd->display_file : ".";
    const char *desc = (div->description && div->description[0]) ? div->description : ".";

    write_indent(fp, indent);
    fprintf(fp, "MsgDivisionBegin %s %s %s %s\n", div->name, acs, display_file, desc);

    for (int i = 0; i < div->child_count; i++) {
        TreeNode *child = div->children[i];
        if (child->type == TREENODE_DIVISION) {
            write_msg_division_recursive(fp, child, indent + 1);
        } else if (child->type == TREENODE_AREA && child->data) {
            write_msgarea(fp, (MsgAreaData *)child->data, indent + 1);
        }
    }

    write_indent(fp, indent);
    fprintf(fp, "MsgDivisionEnd\n\n");
}

static void write_file_division_recursive(FILE *fp, TreeNode *div, int indent)
{
    if (!div || div->type != TREENODE_DIVISION) return;

    DivisionData *dd = (DivisionData *)div->data;
    const char *acs = (dd && dd->acs && dd->acs[0]) ? dd->acs : "Demoted";
    const char *display_file = (dd && dd->display_file && dd->display_file[0]) ? dd->display_file : ".";
    const char *desc = (div->description && div->description[0]) ? div->description : ".";

    write_indent(fp, indent);
    fprintf(fp, "FileDivisionBegin %s %s %s %s\n", div->name, acs, display_file, desc);

    for (int i = 0; i < div->child_count; i++) {
        TreeNode *child = div->children[i];
        if (child->type == TREENODE_DIVISION) {
            write_file_division_recursive(fp, child, indent + 1);
        } else if (child->type == TREENODE_AREA && child->data) {
            write_filearea(fp, (FileAreaData *)child->data, indent + 1);
        }
    }

    write_indent(fp, indent);
    fprintf(fp, "FileDivisionEnd\n\n");
}

/* Save msgarea.ctl */
bool save_msgarea_ctl(const char *sys_path, TreeNode **roots, int count, char *err, size_t err_sz)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/etc/msgarea.ctl", sys_path);
    
    /* Create backup if it doesn't exist */
    if (!create_backup(path, err, err_sz)) {
        return false;
    }
    
    /* Write new file */
    FILE *fp = fopen(path, "w");
    if (!fp) {
        if (err && err_sz) snprintf(err, err_sz, "Cannot write %s", path);
        return false;
    }
    
    fprintf(fp, "%%\n%% msgarea.ctl - Message Area Configuration\n");
    fprintf(fp, "%% Generated by MAXCFG\n%%\n\n");
    
    for (int i = 0; i < count; i++) {
        TreeNode *node = roots[i];
        if (!node) continue;
        if (node->type == TREENODE_DIVISION) {
            write_msg_division_recursive(fp, node, 0);
        } else if (node->type == TREENODE_AREA && node->data) {
            write_msgarea(fp, (MsgAreaData*)node->data, 0);
        }
    }
     
     fclose(fp);
     return true;
 }

/* Save filearea.ctl */
bool save_filearea_ctl(const char *sys_path, TreeNode **roots, int count, char *err, size_t err_sz)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/etc/filearea.ctl", sys_path);
    
    /* Create backup if it doesn't exist */
    if (!create_backup(path, err, err_sz)) {
        return false;
    }
    
    /* Write new file */
    FILE *fp = fopen(path, "w");
    if (!fp) {
        if (err && err_sz) snprintf(err, err_sz, "Cannot write %s", path);
        return false;
    }
    
    fprintf(fp, "%%\n%% filearea.ctl - File Area Configuration\n");
    fprintf(fp, "%% Generated by MAXCFG\n%%\n\n");
    
    for (int i = 0; i < count; i++) {
        TreeNode *node = roots[i];
        if (!node) continue;
        if (node->type == TREENODE_DIVISION) {
            write_file_division_recursive(fp, node, 0);
        } else if (node->type == TREENODE_AREA && node->data) {
            write_filearea(fp, (FileAreaData*)node->data, 0);
        }
    }
    
    fclose(fp);
    return true;
}

 #endif
