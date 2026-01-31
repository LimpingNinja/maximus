/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * treenode_edit.c - Shared form helpers for TreeNode editing
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "treeview.h"
#include "area_parse.h"

/* External globals from treeview.c */
extern TreeNode *g_tree_focus_root;
extern bool g_tree_unfocus_requested;

/* Helper to check if node is descendant of ancestor */
static bool is_descendant_or_self(const TreeNode *ancestor, const TreeNode *node)
{
    if (!ancestor || !node) return false;
    const TreeNode *cur = node;
    while (cur) {
        if (cur == ancestor) return true;
        cur = cur->parent;
    }
    return false;
}

static void rebuild_full_name_recursive(TreeNode *node)
{
    if (node == NULL) {
        return;
    }

    char full_name[512];
    if (node->parent && node->parent->type == TREENODE_DIVISION && node->parent->full_name && node->parent->full_name[0]) {
        if (snprintf(full_name, sizeof(full_name), "%s.%s", node->parent->full_name, node->name ? node->name : "") >= (int)sizeof(full_name)) {
            strncpy(full_name, node->name ? node->name : "", sizeof(full_name) - 1u);
            full_name[sizeof(full_name) - 1u] = '\0';
        }
    } else {
        strncpy(full_name, node->name ? node->name : "", sizeof(full_name) - 1u);
        full_name[sizeof(full_name) - 1u] = '\0';
    }

    free(node->full_name);
    node->full_name = strdup(full_name);

    for (int i = 0; i < node->child_count; i++) {
        rebuild_full_name_recursive(node->children[i]);
    }
}

/* Load division form values from TreeNode */
void treenode_load_division_form(TreeNode *node, char **values)
{
    if (!node || !values) return;
    
    values[0] = strdup(node->name ? node->name : "");
    values[1] = strdup((node->parent && node->parent->type == TREENODE_DIVISION && node->parent->name) 
                       ? node->parent->name : "(None)");
    values[2] = strdup(node->description ? node->description : "");
    
    DivisionData *dd = (DivisionData *)node->data;
    values[3] = strdup((dd && dd->display_file) ? dd->display_file : "");
    values[4] = strdup((dd && dd->acs) ? dd->acs : "Demoted");
}

/* Load message area form values from TreeNode */
void treenode_load_msgarea_form(TreeNode *node, char **values)
{
    if (!node || !values) return;
    
    MsgAreaData *area = (MsgAreaData *)node->data;
    
    /* Basic fields */
    values[0] = strdup(area && area->name ? area->name : (node->name ? node->name : ""));
    values[1] = strdup((node->parent && node->parent->type == TREENODE_DIVISION && node->parent->name) 
                       ? node->parent->name : "(None)");
    values[2] = strdup(area && area->tag ? area->tag : "");
    values[3] = strdup(area && area->path ? area->path : "");
    values[4] = strdup(area && area->desc ? area->desc : (node->description ? node->description : ""));
    values[5] = strdup(area && area->owner ? area->owner : "");
    
    /* Style fields */
    values[7] = strdup("Squish");
    values[8] = strdup("Local");
    values[9] = strdup("Real Name");
    
    /* Style toggles - default to No */
    for (int i = 11; i <= 20; i++) values[i] = strdup("No");
    values[12] = strdup("Yes");  /* Public allowed default */
    
    /* Renum fields */
    for (int i = 22; i <= 24; i++) values[i] = strdup("0");
    
    /* ACS */
    values[25] = strdup(area && area->acs ? area->acs : "Demoted");
    
    /* Paths */
    for (int i = 27; i <= 35; i++) values[i] = strdup("");
}

/* Load file area form values from TreeNode */
void treenode_load_filearea_form(TreeNode *node, char **values)
{
    if (!node || !values) return;
    
    FileAreaData *area = (FileAreaData *)node->data;
    
    /* Basic fields */
    values[0] = strdup(area && area->name ? area->name : (node->name ? node->name : ""));
    values[1] = strdup((node->parent && node->parent->type == TREENODE_DIVISION && node->parent->name) 
                       ? node->parent->name : "(None)");
    values[2] = strdup(area && area->desc ? area->desc : (node->description ? node->description : ""));
    values[4] = strdup(area && area->download ? area->download : "");
    values[5] = strdup(area && area->upload ? area->upload : "");
    values[6] = strdup(area && area->filelist ? area->filelist : "");
    values[8] = strdup("Default");
    values[9] = strdup((area && area->type_slow) ? "Yes" : "No");
    values[10] = strdup((area && area->type_staged) ? "Yes" : "No");
    values[11] = strdup((area && area->type_nonew) ? "Yes" : "No");
    values[12] = strdup("No");
    values[13] = strdup("No");
    values[14] = strdup("No");
    values[15] = strdup("No");
    values[17] = strdup(area && area->acs ? area->acs : "Demoted");
    values[19] = strdup(area && area->barricade ? area->barricade : "");
    values[20] = strdup("");
    values[21] = strdup(area && area->menuname ? area->menuname : "");
    values[22] = strdup("");
}

/* Save division form values back to TreeNode */
bool treenode_save_division_form(TreeNode ***roots, int *root_count, TreeNode *node, 
                                 char **values, TreeContextType context)
{
    if (!node || !values || !roots || !root_count) return false;
    
    bool modified = false;
    
    /* Update name */
    if (values[0] && strcmp(values[0], node->name ? node->name : "") != 0) {
        free(node->name);
        node->name = strdup(values[0]);
        modified = true;
    }
    
    /* Update description */
    if (values[2] && strcmp(values[2], node->description ? node->description : "") != 0) {
        free(node->description);
        node->description = strdup(values[2]);
        modified = true;
    }
    
    /* Update division data */
    if (!node->data) {
        node->data = calloc(1, sizeof(DivisionData));
    }
    DivisionData *dd = (DivisionData *)node->data;
    if (dd) {
        const char *new_df = (values[3] && values[3][0]) ? values[3] : NULL;
        const char *new_acs = (values[4] && values[4][0]) ? values[4] : NULL;
        
        if ((new_df && (!dd->display_file || strcmp(new_df, dd->display_file) != 0)) || 
            (!new_df && dd->display_file)) {
            free(dd->display_file);
            dd->display_file = new_df ? strdup(new_df) : NULL;
            modified = true;
        }
        if ((new_acs && (!dd->acs || strcmp(new_acs, dd->acs) != 0)) || 
            (!new_acs && dd->acs)) {
            free(dd->acs);
            dd->acs = new_acs ? strdup(new_acs) : NULL;
            modified = true;
        }
    }
    
    /* Handle parent division change */
    if (values[1]) {
        TreeNode *new_parent = NULL;
        if (!is_none_choice(values[1])) {
            new_parent = find_division_by_name(*roots, *root_count, values[1]);
        }
        
        TreeNode *old_parent = (node->parent && node->parent->type == TREENODE_DIVISION) ? node->parent : NULL;
        if (new_parent == node || (new_parent && is_descendant_or_self(node, new_parent))) {
            new_parent = old_parent;
        }
        
        if (new_parent != old_parent) {
            /* Check if node will leave focused subtree BEFORE detaching */
            bool will_leave_focus = false;
            if (g_tree_focus_root) {
                bool currently_in_focus = is_descendant_or_self(g_tree_focus_root, node);
                bool will_be_in_focus = (new_parent && is_descendant_or_self(g_tree_focus_root, new_parent));
                will_leave_focus = currently_in_focus && !will_be_in_focus;
            }
            
            treenode_detach(roots, root_count, node);
            treenode_attach(roots, root_count, node, new_parent);
            modified = true;
            
            if (will_leave_focus) {
                g_tree_unfocus_requested = true;
            }
        }
    }
    
    if (modified) {
        rebuild_full_name_recursive(node);
    }
    
    return modified;
}

/* Save message area form values back to TreeNode */
bool treenode_save_msgarea_form(TreeNode ***roots, int *root_count, TreeNode *node, char **values)
{
    if (!node || !values || !roots || !root_count) return false;
    
    bool modified = false;
    
    /* Ensure data structure exists */
    if (!node->data) {
        MsgAreaData *area = calloc(1, sizeof(MsgAreaData));
        if (area) area->style = MSGSTYLE_SQUISH | MSGSTYLE_LOCAL | MSGSTYLE_PUB;
        node->data = area;
    }
    MsgAreaData *area = (MsgAreaData *)node->data;
    if (!area) return false;
    
    /* Update basic fields */
    if (strcmp(values[0] ? values[0] : "", area->name ? area->name : "") != 0) {
        free(area->name);
        area->name = strdup(values[0] ? values[0] : "");
        modified = true;
    }
    
    const char *new_tag = (values[2] && values[2][0]) ? values[2] : NULL;
    if ((new_tag && (!area->tag || strcmp(new_tag, area->tag) != 0)) || (!new_tag && area->tag)) {
        free(area->tag);
        area->tag = new_tag ? strdup(new_tag) : NULL;
        modified = true;
    }
    
    if (strcmp(values[3] ? values[3] : "", area->path ? area->path : "") != 0) {
        free(area->path);
        area->path = strdup(values[3] ? values[3] : "");
        modified = true;
    }
    
    if (strcmp(values[4] ? values[4] : "", area->desc ? area->desc : "") != 0) {
        free(area->desc);
        area->desc = strdup(values[4] ? values[4] : "");
        modified = true;
    }
    
    const char *new_owner = (values[5] && values[5][0]) ? values[5] : NULL;
    if ((new_owner && (!area->owner || strcmp(new_owner, area->owner) != 0)) || (!new_owner && area->owner)) {
        free(area->owner);
        area->owner = new_owner ? strdup(new_owner) : NULL;
        modified = true;
    }
    
    if (strcmp(values[25] ? values[25] : "", area->acs ? area->acs : "") != 0) {
        free(area->acs);
        area->acs = strdup(values[25] ? values[25] : "");
        modified = true;
    }
    
    /* Update TreeNode name/description from area data */
    free(node->name);
    node->name = strdup(area->name ? area->name : "");
    free(node->description);
    node->description = strdup(area->desc ? area->desc : "");
    
    /* Handle parent division change */
    if (values[1]) {
        TreeNode *new_parent = NULL;
        if (!is_none_choice(values[1])) {
            new_parent = find_division_by_name(*roots, *root_count, values[1]);
        }
        TreeNode *old_parent = (node->parent && node->parent->type == TREENODE_DIVISION) ? node->parent : NULL;
        if (new_parent != old_parent) {
            /* Check if node will leave focused subtree BEFORE detaching */
            bool will_leave_focus = false;
            if (g_tree_focus_root) {
                bool currently_in_focus = is_descendant_or_self(g_tree_focus_root, node);
                bool will_be_in_focus = (new_parent && is_descendant_or_self(g_tree_focus_root, new_parent));
                will_leave_focus = currently_in_focus && !will_be_in_focus;
            }
            
            treenode_detach(roots, root_count, node);
            treenode_attach(roots, root_count, node, new_parent);
            modified = true;
            
            if (will_leave_focus) {
                g_tree_unfocus_requested = true;
            }
        }
    }
    
    if (modified) {
        rebuild_full_name_recursive(node);
    }
    
    return modified;
}

/* Save file area form values back to TreeNode */
bool treenode_save_filearea_form(TreeNode ***roots, int *root_count, TreeNode *node, char **values)
{
    if (!node || !values || !roots || !root_count) return false;
    
    bool modified = false;
    
    /* Ensure data structure exists */
    if (!node->data) {
        node->data = calloc(1, sizeof(FileAreaData));
    }
    FileAreaData *area = (FileAreaData *)node->data;
    if (!area) return false;
    
    /* Update basic fields */
    if (strcmp(values[0] ? values[0] : "", area->name ? area->name : "") != 0) {
        free(area->name);
        area->name = strdup(values[0] ? values[0] : "");
        modified = true;
    }
    
    if (strcmp(values[2] ? values[2] : "", area->desc ? area->desc : "") != 0) {
        free(area->desc);
        area->desc = strdup(values[2] ? values[2] : "");
        modified = true;
    }
    
    if (strcmp(values[4] ? values[4] : "", area->download ? area->download : "") != 0) {
        free(area->download);
        area->download = strdup(values[4] ? values[4] : "");
        modified = true;
    }
    
    if (strcmp(values[5] ? values[5] : "", area->upload ? area->upload : "") != 0) {
        free(area->upload);
        area->upload = strdup(values[5] ? values[5] : "");
        modified = true;
    }
    
    const char *new_filelist = (values[6] && values[6][0]) ? values[6] : NULL;
    if ((new_filelist && (!area->filelist || strcmp(new_filelist, area->filelist) != 0)) || 
        (!new_filelist && area->filelist)) {
        free(area->filelist);
        area->filelist = new_filelist ? strdup(new_filelist) : NULL;
        modified = true;
    }
    
    bool ns = (values[9] && strcmp(values[9], "Yes") == 0);
    bool nst = (values[10] && strcmp(values[10], "Yes") == 0);
    bool nnw = (values[11] && strcmp(values[11], "Yes") == 0);
    if (area->type_slow != ns || area->type_staged != nst || area->type_nonew != nnw) {
        area->type_slow = ns;
        area->type_staged = nst;
        area->type_nonew = nnw;
        modified = true;
    }
    
    if (strcmp(values[17] ? values[17] : "", area->acs ? area->acs : "") != 0) {
        free(area->acs);
        area->acs = strdup(values[17] ? values[17] : "");
        modified = true;
    }
    
    const char *new_barr = (values[19] && values[19][0]) ? values[19] : NULL;
    if ((new_barr && (!area->barricade || strcmp(new_barr, area->barricade) != 0)) || 
        (!new_barr && area->barricade)) {
        free(area->barricade);
        area->barricade = new_barr ? strdup(new_barr) : NULL;
        modified = true;
    }
    
    const char *new_menu = (values[21] && values[21][0]) ? values[21] : NULL;
    if ((new_menu && (!area->menuname || strcmp(new_menu, area->menuname) != 0)) || 
        (!new_menu && area->menuname)) {
        free(area->menuname);
        area->menuname = new_menu ? strdup(new_menu) : NULL;
        modified = true;
    }
    
    /* Update TreeNode name/description from area data */
    free(node->name);
    node->name = strdup(area->name ? area->name : "");
    free(node->description);
    node->description = strdup(area->desc ? area->desc : "");
    
    /* Handle parent division change */
    if (values[1]) {
        TreeNode *new_parent = NULL;
        if (!is_none_choice(values[1])) {
            new_parent = find_division_by_name(*roots, *root_count, values[1]);
        }
        TreeNode *old_parent = (node->parent && node->parent->type == TREENODE_DIVISION) ? node->parent : NULL;
        if (new_parent != old_parent) {
            /* Check if node will leave focused subtree BEFORE detaching */
            bool will_leave_focus = false;
            if (g_tree_focus_root) {
                bool currently_in_focus = is_descendant_or_self(g_tree_focus_root, node);
                bool will_be_in_focus = (new_parent && is_descendant_or_self(g_tree_focus_root, new_parent));
                will_leave_focus = currently_in_focus && !will_be_in_focus;
            }
            
            treenode_detach(roots, root_count, node);
            treenode_attach(roots, root_count, node, new_parent);
            modified = true;
            
            if (will_leave_focus) {
                g_tree_unfocus_requested = true;
            }
        }
    }
    
    return modified;
}
