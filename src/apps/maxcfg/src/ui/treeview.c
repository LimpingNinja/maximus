/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * treeview.c - Tree view for hierarchical area/division editing
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "maxcfg.h"
#include "ui.h"
#include "treeview.h"
#include "area_parse.h"
#include "fields.h"

/* Flattened tree item for display */
typedef struct {
    TreeNode *node;
    int indent;           /* Visual indentation level */
    bool is_last_child;   /* Is this the last child at its level? */
    bool *parent_last;    /* Array tracking if ancestors are last children */
    int parent_depth;     /* Depth for parent_last array */
} FlatTreeItem;

/* Tree view state */
typedef struct {
    TreeNode **root_nodes;
    int root_count;
    TreeNode *focus_root;      /* If set, only show this subtree */
    
    FlatTreeItem *items;       /* Flattened tree for display */
    int item_count;
    int item_capacity;
    
    int selected;              /* Currently selected index */
    int scroll_offset;         /* First visible item index */
    int visible_rows;          /* Number of visible rows */
    
    int win_x, win_y;          /* Window position */
    int win_w, win_h;          /* Window dimensions */
} TreeViewState;

/* Forward declarations */
static void flatten_tree(TreeViewState *state);
static void flatten_node(TreeViewState *state, TreeNode *node, int indent, 
                         bool is_last, bool *parent_last, int parent_depth);
static void draw_tree_view(TreeViewState *state, const char *title);
static void draw_tree_item(TreeViewState *state, int item_idx, int row);
static bool edit_tree_item(TreeNode ***root_nodes, int *root_count, TreeNode *node);
static TreeNode *insert_tree_item(TreeNode *current, char **desired_parent_name);

bool is_none_choice(const char *s);
static void update_division_levels_recursive(TreeNode *node, int level);
TreeNode *find_division_by_name(TreeNode **roots, int root_count, const char *name);
bool treenode_detach(TreeNode ***root_nodes, int *root_count, TreeNode *node);
bool treenode_attach(TreeNode ***root_nodes, int *root_count, TreeNode *node, TreeNode *parent_div);
static bool treenode_attach_before(TreeNode ***root_nodes, int *root_count, TreeNode *node, TreeNode *parent_div, TreeNode *before);

static TreeNode *clone_node_recursive(const TreeNode *src, TreeContextType context);
static TreeNode **clone_roots(TreeNode **roots, int count, TreeContextType context);
static void free_tree_data_recursive(TreeNode *node, TreeContextType context);
static void free_tree_with_data(TreeNode **roots, int count, TreeContextType context);
static void prune_disabled_recursive(TreeNode *node, TreeContextType context);
static void prune_disabled_roots(TreeNode ***roots, int *count, TreeContextType context);

/* Current tree context - set by treeview_show */
static TreeContextType g_tree_context = TREE_CONTEXT_MESSAGE;
TreeNode *g_tree_focus_root = NULL;
bool g_tree_unfocus_requested = false;

/* Create a new tree node */
TreeNode *treenode_create(const char *name, const char *full_name,
                          const char *description, TreeNodeType type,
                          int division_level)
{
    TreeNode *node = calloc(1, sizeof(TreeNode));
    if (!node) return NULL;

    node->name = name ? strdup(name) : NULL;
    node->full_name = full_name ? strdup(full_name) : NULL;
    node->description = description ? strdup(description) : NULL;
    node->type = type;
    node->division_level = division_level;
    node->enabled = true;
    node->parent = NULL;
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    node->data = NULL;

    return node;
}

/* Add a child to a tree node */
void treenode_add_child(TreeNode *parent, TreeNode *child)
{
    if (!parent || !child) return;
    
    /* Grow array if needed */
    if (parent->child_count >= parent->child_capacity) {
        int new_cap = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
        TreeNode **new_children = realloc(parent->children, new_cap * sizeof(TreeNode *));
        if (!new_children) return;
        parent->children = new_children;
        parent->child_capacity = new_cap;
    }
    
    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

/* Free a tree node and all its children */
void treenode_free(TreeNode *node)
{
    if (!node) return;
    
    /* Free children recursively */
    for (int i = 0; i < node->child_count; i++) {
        treenode_free(node->children[i]);
    }
    free(node->children);
    
    free(node->name);
    free(node->full_name);
    free(node->description);
    free(node);
}

/* Free an array of root nodes */
void treenode_array_free(TreeNode **nodes, int count)
{
    if (!nodes) return;
    for (int i = 0; i < count; i++) {
        treenode_free(nodes[i]);
    }
    free(nodes);
}

bool is_none_choice(const char *s)
{
    return (!s || !s[0] || strcmp(s, "(None)") == 0);
}

static void update_division_levels_recursive(TreeNode *node, int level)
{
    if (!node) return;
    node->division_level = level;
    for (int i = 0; i < node->child_count; i++) {
        update_division_levels_recursive(node->children[i], level + 1);
    }
}

static TreeNode *find_division_by_name_recursive(TreeNode *node, const char *name)
{
    if (!node || !name || !name[0]) return NULL;
    if (node->type == TREENODE_DIVISION && node->name && strcmp(node->name, name) == 0) {
        return node;
    }
    for (int i = 0; i < node->child_count; i++) {
        TreeNode *found = find_division_by_name_recursive(node->children[i], name);
        if (found) return found;
    }
    return NULL;
}

TreeNode *find_division_by_name(TreeNode **roots, int root_count, const char *name)
{
    if (!roots || root_count <= 0 || !name || !name[0]) return NULL;
    for (int i = 0; i < root_count; i++) {
        TreeNode *found = find_division_by_name_recursive(roots[i], name);
        if (found) return found;
    }
    return NULL;
}

bool treenode_detach(TreeNode ***root_nodes, int *root_count, TreeNode *node)
{
    if (!node || !root_nodes || !root_count || !*root_nodes) return false;

    TreeNode *parent = node->parent;
    if (parent && parent->children && parent->child_count > 0) {
        int idx = -1;
        for (int i = 0; i < parent->child_count; i++) {
            if (parent->children[i] == node) {
                idx = i;
                break;
            }
        }
        if (idx >= 0) {
            for (int i = idx; i < parent->child_count - 1; i++) {
                parent->children[i] = parent->children[i + 1];
            }
            parent->child_count--;
            node->parent = NULL;
            return true;
        }
    }

    for (int i = 0; i < *root_count; i++) {
        if ((*root_nodes)[i] == node) {
            for (int j = i; j < *root_count - 1; j++) {
                (*root_nodes)[j] = (*root_nodes)[j + 1];
            }
            (*root_count)--;
            node->parent = NULL;
            return true;
        }
    }

    return false;
}

bool treenode_attach(TreeNode ***root_nodes, int *root_count, TreeNode *node, TreeNode *parent_div)
{
    if (!node || !root_nodes || !root_count || !*root_nodes) return false;

    if (parent_div) {
        treenode_add_child(parent_div, node);
        update_division_levels_recursive(node, parent_div->division_level + 1);
        return true;
    }

    TreeNode **new_roots = realloc(*root_nodes, (size_t)(*root_count + 1) * sizeof(TreeNode *));
    if (!new_roots) return false;
    *root_nodes = new_roots;
    new_roots[*root_count] = node;
    (*root_count)++;
    update_division_levels_recursive(node, 0);
    return true;
}

static bool treenode_attach_before(TreeNode ***root_nodes, int *root_count, TreeNode *node, TreeNode *parent_div, TreeNode *before)
{
    if (!node || !root_nodes || !root_count || !*root_nodes) return false;

    if (parent_div) {
        int insert_idx = parent_div->child_count;
        if (before && before->parent == parent_div) {
            for (int i = 0; i < parent_div->child_count; i++) {
                if (parent_div->children[i] == before) {
                    insert_idx = i;
                    break;
                }
            }
        }

        if (parent_div->child_count >= parent_div->child_capacity) {
            int new_cap = parent_div->child_capacity == 0 ? 4 : parent_div->child_capacity * 2;
            TreeNode **new_children = realloc(parent_div->children, (size_t)new_cap * sizeof(TreeNode *));
            if (!new_children) return false;
            parent_div->children = new_children;
            parent_div->child_capacity = new_cap;
        }

        for (int i = parent_div->child_count; i > insert_idx; i--) {
            parent_div->children[i] = parent_div->children[i - 1];
        }
        parent_div->children[insert_idx] = node;
        parent_div->child_count++;
        node->parent = parent_div;
        update_division_levels_recursive(node, parent_div->division_level + 1);
        return true;
    }

    int insert_idx = *root_count;
    if (before && before->parent == NULL) {
        for (int i = 0; i < *root_count; i++) {
            if ((*root_nodes)[i] == before) {
                insert_idx = i;
                break;
            }
        }
    }

    TreeNode **new_roots = realloc(*root_nodes, (size_t)(*root_count + 1) * sizeof(TreeNode *));
    if (!new_roots) return false;
    *root_nodes = new_roots;

    for (int i = *root_count; i > insert_idx; i--) {
        new_roots[i] = new_roots[i - 1];
    }
    new_roots[insert_idx] = node;
    (*root_count)++;
    node->parent = NULL;
    update_division_levels_recursive(node, 0);
    return true;
}

static void free_tree_data_recursive(TreeNode *node, TreeContextType context)
{
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        free_tree_data_recursive(node->children[i], context);
    }

    if (node->type == TREENODE_DIVISION) {
        if (node->data) {
            division_data_free((DivisionData *)node->data);
            node->data = NULL;
        }
    } else {
        if (node->data) {
            if (context == TREE_CONTEXT_MESSAGE) msgarea_data_free((MsgAreaData *)node->data);
            else filearea_data_free((FileAreaData *)node->data);
            node->data = NULL;
        }
    }
}

static void free_tree_with_data(TreeNode **roots, int count, TreeContextType context)
{
    if (!roots) return;
    for (int i = 0; i < count; i++) {
        free_tree_data_recursive(roots[i], context);
        treenode_free(roots[i]);
    }
    free(roots);
}

static TreeNode *clone_node_recursive(const TreeNode *src, TreeContextType context)
{
    if (!src) return NULL;

    TreeNode *dst = treenode_create(src->name, src->full_name, src->description, src->type, src->division_level);
    if (!dst) return NULL;
    dst->enabled = src->enabled;

    if (src->type == TREENODE_DIVISION) {
        DivisionData *sdd = (DivisionData *)src->data;
        if (sdd) {
            DivisionData *dd = calloc(1, sizeof(DivisionData));
            if (dd) {
                dd->acs = sdd->acs ? strdup(sdd->acs) : NULL;
                dd->display_file = sdd->display_file ? strdup(sdd->display_file) : NULL;
            }
            dst->data = dd;
        }
    } else {
        if (context == TREE_CONTEXT_MESSAGE) {
            MsgAreaData *sa = (MsgAreaData *)src->data;
            if (sa) {
                MsgAreaData *a = calloc(1, sizeof(MsgAreaData));
                if (a) {
                    a->name = sa->name ? strdup(sa->name) : NULL;
                    a->tag = sa->tag ? strdup(sa->tag) : NULL;
                    a->path = sa->path ? strdup(sa->path) : NULL;
                    a->desc = sa->desc ? strdup(sa->desc) : NULL;
                    a->acs = sa->acs ? strdup(sa->acs) : NULL;
                    a->owner = sa->owner ? strdup(sa->owner) : NULL;
                    a->origin = sa->origin ? strdup(sa->origin) : NULL;
                    a->attachpath = sa->attachpath ? strdup(sa->attachpath) : NULL;
                    a->barricade = sa->barricade ? strdup(sa->barricade) : NULL;
                    a->menuname = sa->menuname ? strdup(sa->menuname) : NULL;
                    a->style = sa->style;
                    a->renum_max = sa->renum_max;
                    a->renum_days = sa->renum_days;
                }
                dst->data = a;
            }
        } else {
            FileAreaData *sa = (FileAreaData *)src->data;
            if (sa) {
                FileAreaData *a = calloc(1, sizeof(FileAreaData));
                if (a) {
                    a->name = sa->name ? strdup(sa->name) : NULL;
                    a->desc = sa->desc ? strdup(sa->desc) : NULL;
                    a->acs = sa->acs ? strdup(sa->acs) : NULL;
                    a->download = sa->download ? strdup(sa->download) : NULL;
                    a->upload = sa->upload ? strdup(sa->upload) : NULL;
                    a->filelist = sa->filelist ? strdup(sa->filelist) : NULL;
                    a->barricade = sa->barricade ? strdup(sa->barricade) : NULL;
                    a->menuname = sa->menuname ? strdup(sa->menuname) : NULL;
                    a->type_slow = sa->type_slow;
                    a->type_staged = sa->type_staged;
                    a->type_nonew = sa->type_nonew;
                }
                dst->data = a;
            }
        }
    }

    for (int i = 0; i < src->child_count; i++) {
        TreeNode *child = clone_node_recursive(src->children[i], context);
        if (child) treenode_add_child(dst, child);
    }

    return dst;
}

static TreeNode **clone_roots(TreeNode **roots, int count, TreeContextType context)
{
    if (!roots || count <= 0) return NULL;
    TreeNode **copy = calloc((size_t)count, sizeof(TreeNode *));
    if (!copy) return NULL;
    for (int i = 0; i < count; i++) {
        copy[i] = clone_node_recursive(roots[i], context);
    }
    return copy;
}

static void prune_disabled_recursive(TreeNode *node, TreeContextType context)
{
    if (!node || node->child_count <= 0) return;

    int write_idx = 0;
    for (int i = 0; i < node->child_count; i++) {
        TreeNode *child = node->children[i];
        if (!child) continue;
        if (!child->enabled) {
            free_tree_data_recursive(child, context);
            treenode_free(child);
            continue;
        }
        prune_disabled_recursive(child, context);
        node->children[write_idx++] = child;
    }
    node->child_count = write_idx;
}

static void prune_disabled_roots(TreeNode ***roots, int *count, TreeContextType context)
{
    if (!roots || !count || !*roots || *count <= 0) return;

    int write_idx = 0;
    for (int i = 0; i < *count; i++) {
        TreeNode *node = (*roots)[i];
        if (!node) continue;
        if (!node->enabled) {
            free_tree_data_recursive(node, context);
            treenode_free(node);
            continue;
        }
        prune_disabled_recursive(node, context);
        (*roots)[write_idx++] = node;
    }
    *count = write_idx;
}

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

static void populate_division_options_current_level(TreeNode **roots, int root_count, TreeContextType context, const TreeNode *exclude)
{
    const char **options = (context == TREE_CONTEXT_FILE) ? file_division_options : msg_division_options;

    int idx = 0;
    options[idx++] = "(None)";

    if (g_tree_focus_root && g_tree_focus_root->type == TREENODE_DIVISION) {
        if (g_tree_focus_root != exclude && g_tree_focus_root->name && g_tree_focus_root->name[0] && idx < 15) {
            options[idx++] = g_tree_focus_root->name;
        }
        for (int i = 0; i < g_tree_focus_root->child_count && idx < 15; i++) {
            TreeNode *child = g_tree_focus_root->children[i];
            if (!child || child->type != TREENODE_DIVISION) continue;
            if (child == exclude) continue;
            if (!child->name || !child->name[0]) continue;
            options[idx++] = child->name;
        }
    } else {
        for (int i = 0; i < root_count && idx < 15; i++) {
            TreeNode *n = roots[i];
            if (!n || n->type != TREENODE_DIVISION) continue;
            if (n == exclude) continue;
            if (!n->name || !n->name[0]) continue;
            options[idx++] = n->name;
        }
    }

    options[idx] = NULL;
}

/* Public wrapper for picklists to use */
void populate_division_options_for_context(TreeNode **roots, int root_count, TreeContextType context, const TreeNode *exclude)
{
    populate_division_options_current_level(roots, root_count, context, exclude);
}

/* Add item to flattened list */
static void add_flat_item(TreeViewState *state, TreeNode *node, int indent,
                          bool is_last, bool *parent_last, int parent_depth)
{
    if (state->item_count >= state->item_capacity) {
        int new_cap = state->item_capacity == 0 ? 32 : state->item_capacity * 2;
        FlatTreeItem *new_items = realloc(state->items, new_cap * sizeof(FlatTreeItem));
        if (!new_items) return;
        state->items = new_items;
        state->item_capacity = new_cap;
    }
    
    FlatTreeItem *item = &state->items[state->item_count++];
    item->node = node;
    item->indent = indent;
    item->is_last_child = is_last;
    item->parent_depth = parent_depth;
    
    /* Copy parent_last array */
    if (parent_depth > 0 && parent_last) {
        item->parent_last = malloc(parent_depth * sizeof(bool));
        if (item->parent_last) {
            memcpy(item->parent_last, parent_last, parent_depth * sizeof(bool));
        }
    } else {
        item->parent_last = NULL;
    }
}

/* Flatten a tree node and its children */
static void flatten_node(TreeViewState *state, TreeNode *node, int indent,
                         bool is_last, bool *parent_last, int parent_depth)
{
    add_flat_item(state, node, indent, is_last, parent_last, parent_depth);
    
    /* Prepare parent_last for children */
    bool *child_parent_last = NULL;
    if (node->child_count > 0) {
        child_parent_last = malloc((parent_depth + 1) * sizeof(bool));
        if (child_parent_last) {
            if (parent_last && parent_depth > 0) {
                memcpy(child_parent_last, parent_last, parent_depth * sizeof(bool));
            }
            child_parent_last[parent_depth] = is_last;
        }
    }
    
    /* Flatten children */
    for (int i = 0; i < node->child_count; i++) {
        bool child_is_last = (i == node->child_count - 1);
        flatten_node(state, node->children[i], indent + 1, child_is_last,
                     child_parent_last, parent_depth + 1);
    }
    
    free(child_parent_last);
}

/* Flatten the tree for display */
static void flatten_tree(TreeViewState *state)
{
    /* Free old items */
    for (int i = 0; i < state->item_count; i++) {
        free(state->items[i].parent_last);
    }
    state->item_count = 0;
    
    TreeNode **roots = state->root_nodes;
    int count = state->root_count;
    
    /* If focused on a subtree, only show that */
    if (state->focus_root) {
        /* Show focus node as root */
        flatten_node(state, state->focus_root, 0, true, NULL, 0);
    } else {
        /* Show all root nodes */
        for (int i = 0; i < count; i++) {
            bool is_last = (i == count - 1);
            flatten_node(state, roots[i], 0, is_last, NULL, 0);
        }
    }
}

/* Draw a single tree item */
static void draw_tree_item(TreeViewState *state, int item_idx, int row)
{
    FlatTreeItem *item = &state->items[item_idx];
    TreeNode *node = item->node;
    bool is_selected = (item_idx == state->selected);
    bool is_disabled = !node->enabled;
    
    /* Position: 1 row from top border, 2 cols from left (1 padding + 1 space) */
    int y = state->win_y + 2 + row;  /* +1 border +1 padding row */
    int x = state->win_x + 2;        /* +1 border +1 padding col */
    int max_width = state->win_w - 4; /* -2 borders -2 padding */
    
    /* Position cursor */
    move(y, x);
    
    int col = 0;
    
    /* Draw tree connectors - always cyan */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    for (int i = 0; i < item->indent; i++) {
        if (i == item->indent - 1) {
            /* This is the connector position for this item */
            if (item->is_last_child) {
                addch(ACS_LLCORNER);  /* └ */
            } else {
                addch(ACS_LTEE);      /* ├ */
            }
            addch(ACS_HLINE);         /* ─ */
        } else if (i < item->parent_depth && item->parent_last) {
            /* Continuation line from ancestor */
            if (item->parent_last[i]) {
                printw("  ");  /* No line needed, ancestor was last child */
            } else {
                addch(ACS_VLINE);     /* │ */
                addch(' ');
            }
        } else {
            printw("  ");
        }
        col += 2;
    }
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Draw name with proper colors */
    if (node->type == TREENODE_DIVISION) {
        /* Division: cyan brackets, bold yellow name */
        if (is_selected) {
            attron(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
            printw("[%s]", node->name);
            attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
        } else {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            addch('[');
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
            if (is_disabled) attron(COLOR_PAIR(CP_DROPDOWN));
            else attron(COLOR_PAIR(CP_FORM_VALUE) | A_BOLD);
            printw("%s", node->name);
            if (is_disabled) attroff(COLOR_PAIR(CP_DROPDOWN));
            else attroff(COLOR_PAIR(CP_FORM_VALUE) | A_BOLD);
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            addch(']');
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        col += strlen(node->name) + 2;
    } else {
        /* Area: bold yellow name */
        if (is_selected) {
            attron(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
        } else {
            if (is_disabled) attron(COLOR_PAIR(CP_DROPDOWN));
            else attron(COLOR_PAIR(CP_FORM_VALUE) | A_BOLD);
        }
        printw("%s", node->name);
        if (is_selected) {
            attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
        } else {
            if (is_disabled) attroff(COLOR_PAIR(CP_DROPDOWN));
            else attroff(COLOR_PAIR(CP_FORM_VALUE) | A_BOLD);
        }
        col += strlen(node->name);
    }
    
    /* Draw description if room - grey (dim) */
    if (node->description && col < max_width - 10) {
        if (is_selected) {
            attron(COLOR_PAIR(CP_MENU_HIGHLIGHT));
        } else {
            attron(COLOR_PAIR(CP_DROPDOWN));  /* Grey text */
        }
        printw(": ");
        col += 2;
        
        /* Truncate description if needed */
        int desc_max = max_width - col - 12;  /* Leave room for (div=N) */
        if (desc_max > 0) {
            if ((int)strlen(node->description) > desc_max) {
                printw("%.*s...", desc_max - 3, node->description);
            } else {
                printw("%s", node->description);
            }
        }
        if (is_selected) {
            attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT));
        } else {
            attroff(COLOR_PAIR(CP_DROPDOWN));
        }
    }
    
    /* Draw division level at end - grey */
    int div_str_len = 8;  /* (div=N) */
    if (state->win_w - 3 - div_str_len > col) {
        move(y, state->win_x + state->win_w - 2 - div_str_len);
        if (is_selected) {
            attron(COLOR_PAIR(CP_MENU_HIGHLIGHT));
        } else {
            attron(COLOR_PAIR(CP_DROPDOWN));
        }
        printw("(div=%d)", node->division_level);
        if (is_selected) {
            attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT));
        } else {
            attroff(COLOR_PAIR(CP_DROPDOWN));
        }
    }
}

/* Draw the tree view */
static void draw_tree_view(TreeViewState *state, const char *title)
{
    /* Fill entire interior with black background first */
    attron(COLOR_PAIR(CP_FORM_BG));
    for (int row = 1; row < state->win_h - 1; row++) {
        mvhline(state->win_y + row, state->win_x + 1, ' ', state->win_w - 2);
    }
    attroff(COLOR_PAIR(CP_FORM_BG));
    
    /* Draw window border */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Top border with title */
    mvaddch(state->win_y, state->win_x, ACS_ULCORNER);
    for (int i = 1; i < state->win_w - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_URCORNER);
    
    /* Title centered */
    if (title) {
        int title_x = state->win_x + (state->win_w - strlen(title)) / 2;
        mvaddch(state->win_y, title_x - 1, ' ');
        attron(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
        mvprintw(state->win_y, title_x, "%s", title);
        attroff(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        addch(' ');
    }
    
    /* Side borders */
    for (int i = 1; i < state->win_h - 1; i++) {
        mvaddch(state->win_y + i, state->win_x, ACS_VLINE);
        mvaddch(state->win_y + i, state->win_x + state->win_w - 1, ACS_VLINE);
    }
    
    /* Bottom border with status */
    mvaddch(state->win_y + state->win_h - 1, state->win_x, ACS_LLCORNER);
    addch(ACS_HLINE);
    addch(' ');
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Status items */
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("F1");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=Help");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(ACS_HLINE);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("INS");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=(");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("I");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw(")nsert");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(ACS_HLINE);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("A");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=Add");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(ACS_HLINE);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("Enter");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=View");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(ACS_HLINE);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("ESC");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=Back");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(ACS_HLINE);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("DEL");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=Delete");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    addch(ACS_HLINE);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    printw("F10");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=Save/Exit");
    attroff(COLOR_PAIR(CP_MENU_BAR));
    
    /* Fill rest of bottom border */
    attron(COLOR_PAIR(CP_DIALOG_BORDER));
    int cur_x = getcurx(stdscr);
    for (int i = cur_x; i < state->win_x + state->win_w - 1; i++) {
        addch(ACS_HLINE);
    }
    mvaddch(state->win_y + state->win_h - 1, state->win_x + state->win_w - 1, ACS_LRCORNER);
    attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    
    /* Draw items - background already filled */
    for (int i = 0; i < state->visible_rows; i++) {
        int item_idx = state->scroll_offset + i;
        if (item_idx < state->item_count) {
            draw_tree_item(state, item_idx, i);
        }
        /* Empty rows already have black background from fill above */
    }
    
    /* Scroll indicators */
    if (state->scroll_offset > 0) {
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvaddch(state->win_y + 2, state->win_x + state->win_w - 2, ACS_UARROW);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    }
    if (state->scroll_offset + state->visible_rows < state->item_count) {
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        mvaddch(state->win_y + state->win_h - 3, state->win_x + state->win_w - 2, ACS_DARROW);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));
    }
    
    refresh();
}

/* Edit a tree item */
static bool edit_tree_item(TreeNode ***root_nodes, int *root_count, TreeNode *node)
{
    if (!node || !root_nodes || !root_count) return false;

    if (node->type == TREENODE_DIVISION) {
        populate_division_options_current_level(*root_nodes, *root_count, g_tree_context, node);

        /* Edit division using shared helper */
        char *div_values[8] = { NULL };
        treenode_load_division_form(node, div_values);

        bool ok;
        if (g_tree_context == TREE_CONTEXT_FILE) {
            ok = form_edit("Edit File Division", file_division_fields,
                            file_division_field_count, div_values, NULL, NULL);
        } else {
            ok = form_edit("Edit Message Division", msg_division_fields,
                            msg_division_field_count, div_values, NULL, NULL);
        }

        if (!ok) {
            for (int i = 0; i < 8; i++) free(div_values[i]);
            return false;
        }

        bool modified = treenode_save_division_form(root_nodes, root_count, node, div_values, g_tree_context);
        
        for (int i = 0; i < 8; i++) free(div_values[i]);
        return modified;
    } else {
        populate_division_options_current_level(*root_nodes, *root_count, g_tree_context, NULL);

        if (g_tree_context == TREE_CONTEXT_FILE) {
            /* Edit file area using shared helper */
            char *area_values[25] = { NULL };
            treenode_load_filearea_form(node, area_values);

            bool ok = form_edit("Edit File Area", file_area_fields,
                                file_area_field_count, area_values, NULL, NULL);
            
            bool modified = false;
            if (ok) {
                modified = treenode_save_filearea_form(root_nodes, root_count, node, area_values);
            }

            for (int i = 0; i < 25; i++) free(area_values[i]);
            return ok && modified;
        }

        /* Edit message area using shared helper */
        char *area_values[45] = { NULL };
        treenode_load_msgarea_form(node, area_values);

        bool ok = form_edit("Edit Message Area", msg_area_fields,
                            msg_area_field_count, area_values, NULL, NULL);
        
        bool modified = false;
        if (ok) {
            modified = treenode_save_msgarea_form(root_nodes, root_count, node, area_values);
        }

        for (int i = 0; i < 45; i++) free(area_values[i]);
        return ok && modified;
    }
}

/* Get the parent division name for insert context */
static const char *get_insert_parent_division(TreeNode *current)
{
    if (!current) return "(None)";
    
    /* If current is a division, insert INTO it - use current as parent */
    if (current->type == TREENODE_DIVISION) {
        return current->name;
    }
    
    /* If current is an area, use its parent (if any) */
    if (current->parent && current->parent->type == TREENODE_DIVISION) {
        return current->parent->name;
    }
    
    return "(None)";
}

/* Insert a new tree item - returns NULL if cancelled */
static TreeNode *insert_tree_item(TreeNode *current, char **desired_parent_name)
{
    if (desired_parent_name) *desired_parent_name = NULL;

    /* Show picker: Area or Division - labels depend on context */
    const char *options_msg[] = { "Message Area", "Message Division", NULL };
    const char *options_file[] = { "File Area", "File Division", NULL };
    const char **options = (g_tree_context == TREE_CONTEXT_FILE) ? options_file : options_msg;
    
    int choice = dialog_option_picker("Insert New", options, 0);
    
    if (choice < 0) return NULL;
    
    /* Determine parent division based on context */
    const char *parent_div = get_insert_parent_division(current);

    populate_division_options_current_level(g_tree_focus_root ? &g_tree_focus_root : NULL,
                                            g_tree_focus_root ? 1 : 0,
                                            g_tree_context,
                                            NULL);
    
    TreeNode *new_node = NULL;
    
    if (choice == 1) {
        /* New division */
        char *div_values[8] = { NULL };
        div_values[0] = strdup("");
        div_values[1] = strdup(parent_div);  /* Pre-populate parent division */
        div_values[2] = strdup("");
        div_values[3] = strdup("");
        div_values[4] = strdup("Demoted");
        
        bool ok;
        if (g_tree_context == TREE_CONTEXT_FILE) {
            ok = form_edit("New File Division", file_division_fields,
                            file_division_field_count, div_values, NULL, NULL);
        } else {
            ok = form_edit("New Message Division", msg_division_fields,
                            msg_division_field_count, div_values, NULL, NULL);
        }

        if (ok && div_values[0] && div_values[0][0]) {
            new_node = treenode_create(div_values[0], div_values[0],
                                        div_values[2], TREENODE_DIVISION, 0);
            if (new_node) {
                DivisionData *dd = calloc(1, sizeof(DivisionData));
                if (dd) {
                    dd->display_file = (div_values[3] && div_values[3][0]) ? strdup(div_values[3]) : NULL;
                    dd->acs = (div_values[4] && div_values[4][0]) ? strdup(div_values[4]) : NULL;
                }
                new_node->data = dd;
                if (desired_parent_name && div_values[1]) {
                    *desired_parent_name = strdup(div_values[1]);
                }
            }
        }
        
        for (int i = 0; i < 8; i++) free(div_values[i]);
    } else {
        /* New area */
        if (g_tree_context == TREE_CONTEXT_FILE) {
            /* File area form */
            char *area_values[25] = { NULL };
            area_values[0] = strdup("");
            area_values[1] = strdup(parent_div);  /* Pre-populate division */
            area_values[2] = strdup("");
            area_values[4] = strdup("");           /* Download path */
            area_values[5] = strdup("");           /* Upload path */
            area_values[6] = strdup("");           /* FILES.BBS path */
            area_values[8] = strdup("Default");    /* Date style */
            area_values[9] = strdup("No");         /* Slow */
            area_values[10] = strdup("No");        /* Staged */
            area_values[11] = strdup("No");        /* NoNew */
            area_values[12] = strdup("No");        /* Hidden */
            area_values[13] = strdup("No");        /* FreeTime */
            area_values[14] = strdup("No");        /* FreeBytes */
            area_values[15] = strdup("No");        /* NoIndex */
            area_values[17] = strdup("Demoted");   /* ACS */
            area_values[19] = strdup("");          /* Barricade menu */
            area_values[20] = strdup("");          /* Barricade file */
            area_values[21] = strdup("");          /* Custom menu */
            area_values[22] = strdup("");          /* Replace menu */
            
            bool ok = form_edit("New File Area", file_area_fields,
                                file_area_field_count, area_values, NULL, NULL);
            
            if (ok && area_values[0] && area_values[0][0]) {
                FileAreaData *fa = calloc(1, sizeof(FileAreaData));
                if (fa) {
                    fa->name = strdup(area_values[0]);
                    fa->desc = strdup(area_values[2] ? area_values[2] : "");
                    fa->download = strdup(area_values[4] ? area_values[4] : "");
                    fa->upload = strdup(area_values[5] ? area_values[5] : "");
                    fa->filelist = (area_values[6] && area_values[6][0]) ? strdup(area_values[6]) : NULL;
                    fa->type_slow = (area_values[9] && strcmp(area_values[9], "Yes") == 0);
                    fa->type_staged = (area_values[10] && strcmp(area_values[10], "Yes") == 0);
                    fa->type_nonew = (area_values[11] && strcmp(area_values[11], "Yes") == 0);
                    fa->acs = strdup(area_values[17] ? area_values[17] : "Demoted");
                    fa->barricade = (area_values[19] && area_values[19][0]) ? strdup(area_values[19]) : NULL;
                    fa->menuname = (area_values[21] && area_values[21][0]) ? strdup(area_values[21]) : NULL;
                }

                new_node = treenode_create(fa ? fa->name : area_values[0],
                                           fa ? fa->name : area_values[0],
                                           fa ? (fa->desc ? fa->desc : "") : (area_values[2] ? area_values[2] : ""),
                                           TREENODE_AREA, 0);
                if (new_node) {
                    new_node->data = fa;
                    if (desired_parent_name && area_values[1]) {
                        *desired_parent_name = strdup(area_values[1]);
                    }
                } else if (fa) {
                    filearea_data_free(fa);
                }
            }
            
            for (int i = 0; i < 25; i++) free(area_values[i]);
        } else {
            /* Message area - full form */
            char *area_values[45] = { NULL };
            area_values[0] = strdup("");
            area_values[1] = strdup(parent_div);  /* Pre-populate division */
            area_values[2] = strdup("");
            area_values[3] = strdup("");
            area_values[4] = strdup("");
            area_values[5] = strdup("");
            area_values[7] = strdup("Squish");
            area_values[8] = strdup("Local");
            area_values[9] = strdup("Real Name");
            for (int i = 11; i <= 20; i++) area_values[i] = strdup("No");
            area_values[12] = strdup("Yes");
            for (int i = 22; i <= 24; i++) area_values[i] = strdup("0");
            area_values[25] = strdup("Demoted");
            for (int i = 27; i <= 35; i++) area_values[i] = strdup("");
            
            bool ok = form_edit("New Message Area", msg_area_fields,
                                msg_area_field_count, area_values, NULL, NULL);
            
            if (ok && area_values[0] && area_values[0][0]) {
                MsgAreaData *ma = calloc(1, sizeof(MsgAreaData));
                if (ma) {
                    ma->name = strdup(area_values[0]);
                    ma->tag = (area_values[2] && area_values[2][0]) ? strdup(area_values[2]) : NULL;
                    ma->path = strdup(area_values[3] ? area_values[3] : "");
                    ma->desc = strdup(area_values[4] ? area_values[4] : "");
                    ma->owner = (area_values[5] && area_values[5][0]) ? strdup(area_values[5]) : NULL;
                    ma->style = MSGSTYLE_SQUISH | MSGSTYLE_LOCAL | MSGSTYLE_PUB;
                    ma->acs = strdup(area_values[25] ? area_values[25] : "Demoted");
                }

                new_node = treenode_create(ma ? ma->name : area_values[0],
                                           ma ? ma->name : area_values[0],
                                           ma ? (ma->desc ? ma->desc : "") : (area_values[4] ? area_values[4] : ""),
                                           TREENODE_AREA, 0);
                if (new_node) {
                    new_node->data = ma;
                    if (desired_parent_name && area_values[1]) {
                        *desired_parent_name = strdup(area_values[1]);
                    }
                } else if (ma) {
                    msgarea_data_free(ma);
                }
            }
            
            for (int i = 0; i < 45; i++) free(area_values[i]);
        }
    }

    return new_node;
}

/* Show the tree view */
TreeViewResult treeview_show(const char *title, TreeNode ***root_nodes, int *root_count, TreeNode *focus_node, TreeContextType context)
{
    /* Store context for insert operations */
    g_tree_context = context;
    TreeViewState state = {0};
    if (!root_nodes || !*root_nodes || !root_count) {
        return TREEVIEW_EXIT;
    }
    TreeNode **orig_roots = clone_roots(*root_nodes, *root_count, context);
    int orig_count = *root_count;
    bool dirty = false;

    state.root_nodes = *root_nodes;
    state.root_count = *root_count;
    state.focus_root = focus_node;
    g_tree_focus_root = state.focus_root;
    g_tree_unfocus_requested = false;
    
    /* Calculate window dimensions */
    state.win_w = COLS - 4;
    state.win_h = LINES - 4;
    state.win_x = 2;
    state.win_y = 2;
    state.visible_rows = state.win_h - 4;  /* Minus borders (2) and padding (2) */
    
    /* Flatten tree */
    flatten_tree(&state);
    
    if (state.item_count == 0) {
        dialog_message("Tree View", "No items to display.");
        return TREEVIEW_EXIT;
    }
    
    curs_set(0);
    TreeViewResult result = TREEVIEW_EXIT;
    bool done = false;
    
    while (!done) {
        draw_tree_view(&state, title);
        
        int ch = getch();
        FlatTreeItem *current = &state.items[state.selected];
        
        switch (ch) {
            case KEY_UP:
            case 'k':
                if (state.selected > 0) {
                    state.selected--;
                    if (state.selected < state.scroll_offset) {
                        state.scroll_offset = state.selected;
                    }
                }
                break;
                
            case KEY_DOWN:
            case 'j':
                if (state.selected < state.item_count - 1) {
                    state.selected++;
                    if (state.selected >= state.scroll_offset + state.visible_rows) {
                        state.scroll_offset = state.selected - state.visible_rows + 1;
                    }
                }
                break;
                
            case KEY_PPAGE:
                state.selected -= state.visible_rows;
                if (state.selected < 0) state.selected = 0;
                state.scroll_offset = state.selected;
                break;
                
            case KEY_NPAGE:
                state.selected += state.visible_rows;
                if (state.selected >= state.item_count) {
                    state.selected = state.item_count - 1;
                }
                if (state.selected >= state.scroll_offset + state.visible_rows) {
                    state.scroll_offset = state.selected - state.visible_rows + 1;
                }
                break;
                
            case KEY_HOME:
                state.selected = 0;
                state.scroll_offset = 0;
                break;
                
            case KEY_END:
                state.selected = state.item_count - 1;
                if (state.selected >= state.visible_rows) {
                    state.scroll_offset = state.selected - state.visible_rows + 1;
                }
                break;
                
            case '\n':
            case '\r':
                /* Enter: drill down on division, edit on area */
                if (current->node->type == TREENODE_DIVISION) {
                    /* Drill down into division */
                    state.focus_root = current->node;
                    g_tree_focus_root = state.focus_root;
                    state.selected = 0;
                    state.scroll_offset = 0;
                    flatten_tree(&state);
                } else {
                    /* Edit area */
                    if (edit_tree_item(root_nodes, root_count, current->node)) {
                        dirty = true;
                        /* Update state pointers in case tree structure changed */
                        state.root_nodes = *root_nodes;
                        state.root_count = *root_count;
                        flatten_tree(&state);
                        if (g_tree_unfocus_requested) {
                            state.focus_root = NULL;
                            g_tree_focus_root = NULL;
                            state.selected = 0;
                            state.scroll_offset = 0;
                            g_tree_unfocus_requested = false;
                            flatten_tree(&state);
                        }
                        touchwin(stdscr);
                        refresh();
                    }
                }
                break;
                
            case KEY_F(5):
                /* F5: Edit current item */
                if (edit_tree_item(root_nodes, root_count, current->node)) {
                    dirty = true;
                    /* Update state pointers in case tree structure changed */
                    state.root_nodes = *root_nodes;
                    state.root_count = *root_count;
                    flatten_tree(&state);
                    if (g_tree_unfocus_requested) {
                        state.focus_root = NULL;
                        g_tree_focus_root = NULL;
                        state.selected = 0;
                        state.scroll_offset = 0;
                        g_tree_unfocus_requested = false;
                        flatten_tree(&state);
                    }
                    touchwin(stdscr);
                    refresh();
                }
                break;

            case 'a':
            case 'A':
                /* Add: Add new item at bottom of current view */
                {
                    TreeNode *context_node = state.focus_root;
                    char *desired_parent = NULL;
                    TreeNode *new_node = insert_tree_item(context_node, &desired_parent);
                    if (new_node) {
                        TreeNode *parent = (state.focus_root && state.focus_root->type == TREENODE_DIVISION) ? state.focus_root : NULL;

                        if (!treenode_attach_before(root_nodes, root_count, new_node, parent, NULL)) {
                            if (desired_parent) free(desired_parent);
                            treenode_free(new_node);
                            break;
                        }

                        state.root_nodes = *root_nodes;
                        state.root_count = *root_count;
                        flatten_tree(&state);
                        dirty = true;
                        if (g_tree_unfocus_requested) {
                            state.focus_root = NULL;
                            g_tree_focus_root = NULL;
                            state.selected = 0;
                            state.scroll_offset = 0;
                            g_tree_unfocus_requested = false;
                            flatten_tree(&state);
                        }
                    }
                    free(desired_parent);
                    touchwin(stdscr);
                }
                break;
                
            case KEY_IC:  /* Insert key */
            case 'i':
            case 'I':
                /* Insert: Add new item */
                {
                    char *desired_parent = NULL;
                    TreeNode *context_node = current->node;
                    if (current->node && current->node->type == TREENODE_DIVISION) {
                        context_node = current->node->parent;
                    }
                    TreeNode *new_node = insert_tree_item(context_node, &desired_parent);
                    if (new_node) {
                        TreeNode *parent = NULL;
                        if (!is_none_choice(desired_parent)) {
                            parent = find_division_by_name(*root_nodes, *root_count, desired_parent);
                        }
                        if (!parent) {
                            if (current->node && current->node->parent && current->node->parent->type == TREENODE_DIVISION) {
                                parent = current->node->parent;
                            }
                        }

                        TreeNode *insert_before = NULL;
                        if (current->node) {
                            if (parent) {
                                if (current->node->parent == parent) insert_before = current->node;
                            } else {
                                if (current->node->parent == NULL) insert_before = current->node;
                            }
                        }

                        if (!treenode_attach_before(root_nodes, root_count, new_node, parent, insert_before)) {
                            if (desired_parent) free(desired_parent);
                            treenode_free(new_node);
                            break;
                        }
                        state.root_nodes = *root_nodes;
                        state.root_count = *root_count;
                        
                        /* Rebuild flat list */
                        flatten_tree(&state);
                        dirty = true;
                        if (g_tree_unfocus_requested) {
                            state.focus_root = NULL;
                            g_tree_focus_root = NULL;
                            state.selected = 0;
                            state.scroll_offset = 0;
                            g_tree_unfocus_requested = false;
                            flatten_tree(&state);
                        }
                    }
                    free(desired_parent);
                    touchwin(stdscr);
                }
                break;

            case KEY_DC:
                if (state.item_count > 0) {
                    current->node->enabled = !current->node->enabled;
                    dirty = true;
                    touchwin(stdscr);
                }
                break;

            case KEY_F(10):
                if (dirty) {
                    prune_disabled_roots(root_nodes, root_count, context);
                    state.root_nodes = *root_nodes;
                    state.root_count = *root_count;
                    result = TREEVIEW_EDIT;
                }
                done = true;
                break;
                 
            case 27:  /* ESC */
                if (state.focus_root) {
                    /* Go back up from drill-down */
                    TreeNode *parent = state.focus_root->parent;
                    state.focus_root = parent;  /* NULL = back to root */
                    g_tree_focus_root = state.focus_root;
                    state.selected = 0;
                    state.scroll_offset = 0;
                    flatten_tree(&state);
                } else {
                    if (dirty) {
                        DialogResult r = dialog_save_prompt();
                        if (r == DIALOG_SAVE_EXIT) {
                            prune_disabled_roots(root_nodes, root_count, context);
                            state.root_nodes = *root_nodes;
                            state.root_count = *root_count;
                            result = TREEVIEW_EDIT;
                            done = true;
                        } else if (r == DIALOG_ABORT) {
                            free_tree_with_data(*root_nodes, *root_count, context);
                            *root_nodes = orig_roots;
                            *root_count = orig_count;
                            orig_roots = NULL;
                            result = TREEVIEW_EXIT;
                            done = true;
                        } else {
                            touchwin(stdscr);
                        }
                    } else {
                        done = true;
                    }
                }
                break;
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < state.item_count; i++) {
        free(state.items[i].parent_last);
    }
    free(state.items);

    if (orig_roots) {
        free_tree_with_data(orig_roots, orig_count, context);
    }
     
    return result;
}

/* Build sample tree for testing */
TreeNode **treeview_build_sample(int *count)
{
    TreeNode **roots = malloc(4 * sizeof(TreeNode *));
    if (!roots) {
        *count = 0;
        return NULL;
    }
    
    /* main: top-level area */
    roots[0] = treenode_create("main", "main", 
                               "Sample Message Area Description, no division",
                               TREENODE_AREA, 0);
    
    /* programming: division with nested division */
    roots[1] = treenode_create("programming", "programming",
                               "Programming division description",
                               TREENODE_DIVISION, 0);
    
    /* programming.languages: nested division */
    TreeNode *languages = treenode_create("languages", "programming.languages",
                                          "Languages subdiv description truncated he...",
                                          TREENODE_DIVISION, 1);
    treenode_add_child(roots[1], languages);
    
    /* programming.languages.c */
    TreeNode *c_area = treenode_create("c", "programming.languages.c",
                                       "A message area programming.languages.c",
                                       TREENODE_AREA, 2);
    treenode_add_child(languages, c_area);
    
    /* programming.languages.pascal */
    TreeNode *pascal = treenode_create("pascal", "programming.languages.pascal",
                                       "An area supporting Pascal",
                                       TREENODE_AREA, 2);
    treenode_add_child(languages, pascal);
    
    /* programming.tools */
    TreeNode *tools = treenode_create("tools", "programming.tools",
                                      "All about programming tools",
                                      TREENODE_AREA, 1);
    treenode_add_child(roots[1], tools);
    
    /* garden: division */
    roots[2] = treenode_create("garden", "garden",
                               "A division around gardens",
                               TREENODE_DIVISION, 0);
    
    /* garden.flowers */
    TreeNode *flowers = treenode_create("flowers", "garden.flowers",
                                        "An area all about flowers",
                                        TREENODE_AREA, 1);
    treenode_add_child(roots[2], flowers);
    
    /* chitchat: top-level area */
    roots[3] = treenode_create("chitchat", "chitchat",
                               "Random message forum",
                               TREENODE_AREA, 0);
    
    *count = 4;
    return roots;
}
