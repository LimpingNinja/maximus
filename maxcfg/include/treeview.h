/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * treeview.h - Tree view for hierarchical area/division editing
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#ifndef TREEVIEW_H
#define TREEVIEW_H

#include <stdbool.h>

/* Tree node types */
typedef enum {
    TREENODE_DIVISION,
    TREENODE_AREA
} TreeNodeType;

/* Tree node structure */
typedef struct _TreeNode {
    char *name;              /* Short name (e.g., "c", "programming") */
    char *full_name;         /* Full path name (e.g., "programming.languages.c") */
    char *description;       /* Description text */
    TreeNodeType type;       /* Division or area */
    int division_level;      /* Nesting depth (0=top, 1=inside div, etc) */
    bool enabled;            /* Is this item enabled? */
    
    struct _TreeNode *parent;     /* Parent node (NULL for root items) */
    struct _TreeNode **children;  /* Array of child nodes */
    int child_count;              /* Number of children */
    int child_capacity;           /* Allocated capacity for children */
    
    void *data;              /* User data pointer (for future use) */
} TreeNode;

/* Tree view result codes */
typedef enum {
    TREEVIEW_EXIT,           /* User pressed ESC at root level */
    TREEVIEW_BACK,           /* User pressed ESC to go back up */
    TREEVIEW_EDIT,           /* User edited an item */
    TREEVIEW_INSERT          /* User inserted an item */
} TreeViewResult;

/* Tree context type - determines labels and field definitions */
typedef enum {
    TREE_CONTEXT_MESSAGE,    /* Message areas/divisions */
    TREE_CONTEXT_FILE        /* File areas/divisions */
} TreeContextType;

/* Create a new tree node */
TreeNode *treenode_create(const char *name, const char *full_name, 
                          const char *description, TreeNodeType type,
                          int division_level);

/* Add a child to a tree node */
void treenode_add_child(TreeNode *parent, TreeNode *child);

/* Free a tree node and all its children */
void treenode_free(TreeNode *node);

/* Free an array of root nodes */
void treenode_array_free(TreeNode **nodes, int count);

/* Show the tree view
 * title: Window title
 * root_nodes: Array of top-level nodes (divisions and areas at div=0)
 * root_count: Number of root nodes
 * focus_node: If not NULL, start with view focused on this division's subtree
 * context: TREE_CONTEXT_MESSAGE or TREE_CONTEXT_FILE for proper labels
 * Returns: Result code
 */
TreeViewResult treeview_show(const char *title, TreeNode **root_nodes, 
                             int root_count, TreeNode *focus_node,
                             TreeContextType context);

/* Build sample tree for testing */
TreeNode **treeview_build_sample(int *count);

#endif /* TREEVIEW_H */
