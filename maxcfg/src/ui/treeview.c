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
static void edit_tree_item(TreeNode *node);
static TreeNode *insert_tree_item(TreeNode *current);

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
            attron(COLOR_PAIR(CP_FORM_VALUE) | A_BOLD);
            printw("%s", node->name);
            attroff(COLOR_PAIR(CP_FORM_VALUE) | A_BOLD);
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
            attron(COLOR_PAIR(CP_FORM_VALUE) | A_BOLD);
        }
        printw("%s", node->name);
        if (is_selected) {
            attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
        } else {
            attroff(COLOR_PAIR(CP_FORM_VALUE) | A_BOLD);
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
    printw("F5");
    attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
    attron(COLOR_PAIR(CP_MENU_BAR));
    printw("=Edit");
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
    printw("=Exit");
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
static void edit_tree_item(TreeNode *node)
{
    if (node->type == TREENODE_DIVISION) {
        /* Edit division */
        char *div_values[8] = { NULL };
        div_values[0] = strdup(node->name);
        div_values[1] = strdup("(None)");  /* Parent Division - TODO */
        div_values[2] = strdup(node->description ? node->description : "");
        div_values[3] = strdup("");        /* Display file */
        div_values[4] = strdup("Demoted"); /* ACS */
        
        form_edit("Edit Message Division", msg_division_fields, 
                  msg_division_field_count, div_values);
        
        /* Update node from form */
        if (div_values[0]) {
            free(node->name);
            node->name = strdup(div_values[0]);
        }
        if (div_values[2]) {
            free(node->description);
            node->description = strdup(div_values[2]);
        }
        
        for (int i = 0; i < 8; i++) free(div_values[i]);
    } else {
        /* Edit area */
        char *area_values[45] = { NULL };
        
        /* Load basic values */
        area_values[0] = strdup(node->name);
        area_values[1] = strdup("(None)");  /* Division */
        area_values[2] = strdup("");        /* Tag */
        area_values[3] = strdup("");        /* Path */
        area_values[4] = strdup(node->description ? node->description : "");
        area_values[5] = strdup("");        /* Owner */
        /* ... rest are defaults ... */
        area_values[7] = strdup("Squish");
        area_values[8] = strdup("Local");
        area_values[9] = strdup("Real Name");
        for (int i = 11; i <= 20; i++) area_values[i] = strdup("No");
        area_values[12] = strdup("Yes");  /* Pub = Yes by default */
        for (int i = 22; i <= 24; i++) area_values[i] = strdup("0");
        area_values[25] = strdup("Demoted");
        for (int i = 27; i <= 35; i++) area_values[i] = strdup("");
        
        form_edit("Edit Message Area", msg_area_fields, 
                  msg_area_field_count, area_values);
        
        /* Update node from form */
        if (area_values[0]) {
            free(node->name);
            node->name = strdup(area_values[0]);
        }
        if (area_values[4]) {
            free(node->description);
            node->description = strdup(area_values[4]);
        }
        
        for (int i = 0; i < 45; i++) free(area_values[i]);
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

/* Current tree context - set by treeview_show */
static TreeContextType g_tree_context = TREE_CONTEXT_MESSAGE;

/* Insert a new tree item - returns NULL if cancelled */
static TreeNode *insert_tree_item(TreeNode *current)
{
    /* Show picker: Area or Division - labels depend on context */
    const char *options_msg[] = { "Message Area", "Message Division", NULL };
    const char *options_file[] = { "File Area", "File Division", NULL };
    const char **options = (g_tree_context == TREE_CONTEXT_FILE) ? options_file : options_msg;
    
    int choice = dialog_option_picker("Insert New", options, 0);
    
    if (choice < 0) return NULL;
    
    /* Determine parent division based on context */
    const char *parent_div = get_insert_parent_division(current);
    int div_level = 0;
    
    if (current) {
        if (current->type == TREENODE_DIVISION) {
            /* Inserting INTO a division */
            div_level = current->division_level + 1;
        } else {
            /* Inserting as sibling to an area */
            div_level = current->division_level;
        }
    }
    
    TreeNode *new_node = NULL;
    
    if (choice == 1) {
        /* New division */
        char *div_values[8] = { NULL };
        div_values[0] = strdup("");
        div_values[1] = strdup(parent_div);  /* Pre-populate parent division */
        div_values[2] = strdup("");
        div_values[3] = strdup("");
        div_values[4] = strdup("Demoted");
        
        if (g_tree_context == TREE_CONTEXT_FILE) {
            form_edit("New File Division", file_division_fields,
                      file_division_field_count, div_values);
        } else {
            form_edit("New Message Division", msg_division_fields,
                      msg_division_field_count, div_values);
        }
        
        if (div_values[0] && div_values[0][0]) {
            new_node = treenode_create(div_values[0], div_values[0],
                                       div_values[2], TREENODE_DIVISION, div_level);
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
            
            form_edit("New File Area", file_area_fields,
                      file_area_field_count, area_values);
            
            if (area_values[0] && area_values[0][0]) {
                new_node = treenode_create(area_values[0], area_values[0],
                                           area_values[2], TREENODE_AREA, div_level);
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
            
            form_edit("New Message Area", msg_area_fields,
                      msg_area_field_count, area_values);
            
            if (area_values[0] && area_values[0][0]) {
                new_node = treenode_create(area_values[0], area_values[0],
                                           area_values[4], TREENODE_AREA, div_level);
            }
            
            for (int i = 0; i < 45; i++) free(area_values[i]);
        }
    }
    
    return new_node;
}

/* Show the tree view */
TreeViewResult treeview_show(const char *title, TreeNode **root_nodes,
                             int root_count, TreeNode *focus_node,
                             TreeContextType context)
{
    /* Store context for insert operations */
    g_tree_context = context;
    TreeViewState state = {0};
    state.root_nodes = root_nodes;
    state.root_count = root_count;
    state.focus_root = focus_node;
    
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
                    state.selected = 0;
                    state.scroll_offset = 0;
                    flatten_tree(&state);
                } else {
                    /* Edit area */
                    edit_tree_item(current->node);
                    touchwin(stdscr);
                }
                break;
                
            case KEY_F(5):
                /* F5: Edit current item */
                edit_tree_item(current->node);
                touchwin(stdscr);
                break;
                
            case KEY_IC:  /* Insert key */
            case 'i':
            case 'I':
                /* Insert: Add new item */
                {
                    TreeNode *new_node = insert_tree_item(current->node);
                    if (new_node) {
                        /* TODO: Actually insert into tree structure */
                        /* For now, just free it */
                        treenode_free(new_node);
                        result = TREEVIEW_INSERT;
                    }
                    touchwin(stdscr);
                }
                break;
                
            case 27:  /* ESC */
                if (state.focus_root) {
                    /* Go back up from drill-down */
                    TreeNode *parent = state.focus_root->parent;
                    state.focus_root = parent;  /* NULL = back to root */
                    state.selected = 0;
                    state.scroll_offset = 0;
                    flatten_tree(&state);
                } else {
                    /* Exit tree view */
                    done = true;
                    result = TREEVIEW_EXIT;
                }
                break;
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < state.item_count; i++) {
        free(state.items[i].parent_last);
    }
    free(state.items);
    
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
