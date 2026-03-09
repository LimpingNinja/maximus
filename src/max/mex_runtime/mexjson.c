/*
 * mexjson.c — MEX JSON intrinsics (cJSON bridge)
 *
 * Copyright 2026 by Kevin Morgan.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#define MAX_LANG_m_area
#include "mexall.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MEX

/*------------------------------------------------------------------------*
 * Constants                                                              *
 *------------------------------------------------------------------------*/

#define MAX_MEXJSON    16   /**< Maximum concurrent JSON handles          */
#define MAX_JSON_DEPTH 16   /**< Maximum cursor nesting depth             */

/* JSON type constants — must match json.mh */
#define MEX_JSON_NULL     0
#define MEX_JSON_BOOL     1
#define MEX_JSON_NUMBER   2
#define MEX_JSON_STRING   3
#define MEX_JSON_ARRAY    4
#define MEX_JSON_OBJECT   5
#define MEX_JSON_END     -1
#define MEX_JSON_INVALID -2

/*------------------------------------------------------------------------*
 * Handle table                                                           *
 *------------------------------------------------------------------------*/

/** Per-handle state: parsed tree + cursor + parent stack. */
typedef struct _mex_json {
    cJSON  *root;                         /**< Parsed tree root            */
    cJSON  *cursor;                       /**< Current cursor position     */
    cJSON  *stack[MAX_JSON_DEPTH];        /**< Parent stack for enter/exit */
    int     depth;                        /**< Current nesting depth       */
} MEX_JSON;

static MEX_JSON g_mex_json[MAX_MEXJSON];

/*------------------------------------------------------------------------*
 * Internal helpers                                                       *
 *------------------------------------------------------------------------*/

/**
 * @brief Map a cJSON node type to our MEX_JSON_* constants.
 */
static int mex_cjson_type(const cJSON *node)
{
    if (!node)                    return MEX_JSON_INVALID;
    if (cJSON_IsNull(node))      return MEX_JSON_NULL;
    if (cJSON_IsBool(node))      return MEX_JSON_BOOL;
    if (cJSON_IsNumber(node))    return MEX_JSON_NUMBER;
    if (cJSON_IsString(node))    return MEX_JSON_STRING;
    if (cJSON_IsArray(node))     return MEX_JSON_ARRAY;
    if (cJSON_IsObject(node))    return MEX_JSON_OBJECT;
    return MEX_JSON_INVALID;
}

/**
 * @brief Find a free slot in the handle table.
 * @return Slot index 0..MAX_MEXJSON-1 or -1 if full.
 */
static int mex_json_alloc_slot(void)
{
    for (int i = 0; i < MAX_MEXJSON; i++)
    {
        if (!g_mex_json[i].root)
            return i;
    }
    return -1;
}

/**
 * @brief Validate a handle index and return the slot, or NULL.
 */
static MEX_JSON *mex_json_slot(int jh)
{
    if (jh < 0 || jh >= MAX_MEXJSON || !g_mex_json[jh].root)
        return NULL;
    return &g_mex_json[jh];
}

/**
 * @brief Initialize a slot with a parsed root node.
 */
static void mex_json_init_slot(int slot, cJSON *root)
{
    g_mex_json[slot].root   = root;
    g_mex_json[slot].cursor = root;
    g_mex_json[slot].depth  = 0;
    memset(g_mex_json[slot].stack, 0, sizeof(g_mex_json[slot].stack));
}

/**
 * @brief Get the current parent container (top of cursor stack).
 *
 * If depth > 0, returns stack[depth-1]. Otherwise returns root.
 */
static cJSON *mex_json_parent(MEX_JSON *j)
{
    return (j->depth > 0) ? j->stack[j->depth - 1] : j->root;
}

/**
 * @brief Resolve a dotted/bracketed path against a cJSON tree.
 *
 * Supports: "key", "key.sub", "arr[0]", "obj.arr[2].name"
 * Empty string returns root.
 *
 * @param root  The cJSON root node.
 * @param path  Dot/bracket path string.
 * @return Pointer to the resolved cJSON node, or NULL if not found.
 */
static cJSON *json_resolve_path(cJSON *root, const char *path)
{
    if (!root || !path || !*path)
        return root;

    cJSON *cur = root;
    char buf[512];
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char *p = buf;

    while (*p && cur)
    {
        /* Skip leading dots */
        if (*p == '.')
            p++;

        if (!*p)
            break;

        /* Array index: [N] */
        if (*p == '[')
        {
            int idx = atoi(p + 1);
            cur = cJSON_GetArrayItem(cur, idx);
            p = strchr(p, ']');
            if (p) p++;
            else break;
            continue;
        }

        /* Object key: up to next '.' or '[' */
        char *end = strpbrk(p, ".[");
        char saved = 0;
        if (end)
        {
            saved = *end;
            *end = '\0';
        }

        cur = cJSON_GetObjectItemCaseSensitive(cur, p);

        if (end)
        {
            *end = saved;
            p = end;
        }
        else
            break;
    }

    return cur;
}

/**
 * @brief Convert a cJSON node's value to a string representation.
 *
 * Auto-converts numbers, booleans, and null. Writes into caller-supplied buf.
 * Returns a pointer to either node->valuestring or buf.
 */
static const char *mex_json_value_as_str(const cJSON *node, char *buf, size_t bufsz)
{
    if (!node)
        return "";

    if (cJSON_IsString(node) && node->valuestring)
        return node->valuestring;

    if (cJSON_IsNumber(node))
    {
        snprintf(buf, bufsz, "%g", node->valuedouble);
        return buf;
    }

    if (cJSON_IsBool(node))
        return cJSON_IsTrue(node) ? "true" : "false";

    if (cJSON_IsNull(node))
        return "null";

    return "";
}

/*------------------------------------------------------------------------*
 * Cleanup — called from intrin_term() on MEX session exit                *
 *------------------------------------------------------------------------*/

/**
 * @brief Free all open JSON handles. Call on MEX session teardown.
 */
void MexJsonCleanup(void)
{
    for (int i = 0; i < MAX_MEXJSON; i++)
    {
        if (g_mex_json[i].root)
        {
            cJSON_Delete(g_mex_json[i].root);
            memset(&g_mex_json[i], 0, sizeof(g_mex_json[i]));
        }
    }
}

/*========================================================================*
 * LIFECYCLE INTRINSICS                                                   *
 *========================================================================*/

/** Parse a JSON string. Returns handle (0..15) or -1 on error. */
word EXPENTRY intrin_json_open(void)
{
    MA ma;
    MexArgBegin(&ma);
    char *text = MexArgGetString(&ma, FALSE);

    regs_2[0] = (word)-1;

    if (!text)
        return MexArgEnd(&ma);

    int slot = mex_json_alloc_slot();
    if (slot == -1)
    {
        logit("!MEX json_open: no free slots");
        free(text);
        return MexArgEnd(&ma);
    }

    cJSON *root = cJSON_Parse(text);
    free(text);

    if (!root)
    {
        logit("!MEX json_open: parse error");
        return MexArgEnd(&ma);
    }

    mex_json_init_slot(slot, root);
    regs_2[0] = (word)slot;
    return MexArgEnd(&ma);
}

/** Create an empty JSON object. Returns handle or -1. */
word EXPENTRY intrin_json_create(void)
{
    MA ma;
    MexArgBegin(&ma);

    regs_2[0] = (word)-1;

    int slot = mex_json_alloc_slot();
    if (slot == -1)
    {
        logit("!MEX json_create: no free slots");
        return MexArgEnd(&ma);
    }

    cJSON *root = cJSON_CreateObject();
    if (!root)
        return MexArgEnd(&ma);

    mex_json_init_slot(slot, root);
    regs_2[0] = (word)slot;
    return MexArgEnd(&ma);
}

/** Create an empty JSON array. Returns handle or -1. */
word EXPENTRY intrin_json_create_array(void)
{
    MA ma;
    MexArgBegin(&ma);

    regs_2[0] = (word)-1;

    int slot = mex_json_alloc_slot();
    if (slot == -1)
    {
        logit("!MEX json_create_array: no free slots");
        return MexArgEnd(&ma);
    }

    cJSON *root = cJSON_CreateArray();
    if (!root)
        return MexArgEnd(&ma);

    mex_json_init_slot(slot, root);
    regs_2[0] = (word)slot;
    return MexArgEnd(&ma);
}

/** Destroy a parsed/created JSON tree and free the handle. */
word EXPENTRY intrin_json_close(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh = (int)MexArgGetWord(&ma);

    MEX_JSON *j = mex_json_slot(jh);
    if (j)
    {
        cJSON_Delete(j->root);
        memset(j, 0, sizeof(*j));
    }

    return MexArgEnd(&ma);
}

/*========================================================================*
 * CURSOR NAVIGATION INTRINSICS                                           *
 *========================================================================*/

/**
 * Descend into the current object or array. Sets cursor to NULL
 * (pre-first-child); the first json_next() returns the first child.
 * Returns 0 on success, -1 on error.
 */
word EXPENTRY intrin_json_enter(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh = (int)MexArgGetWord(&ma);

    regs_2[0] = (word)-1;

    MEX_JSON *j = mex_json_slot(jh);
    if (!j || !j->cursor)
        return MexArgEnd(&ma);

    /* Cursor must be at an object or array */
    if (!cJSON_IsObject(j->cursor) && !cJSON_IsArray(j->cursor))
        return MexArgEnd(&ma);

    if (j->depth >= MAX_JSON_DEPTH)
        return MexArgEnd(&ma);

    /* Push current cursor as parent, set cursor to NULL (pre-first-child) */
    j->stack[j->depth++] = j->cursor;
    j->cursor = NULL;

    regs_2[0] = 0;
    return MexArgEnd(&ma);
}

/**
 * Advance cursor to the next sibling. Returns JSON type or JSON_END.
 *
 * After json_enter(), cursor is NULL. First json_next() returns the
 * first child. Subsequent calls advance through siblings via cJSON's
 * linked list. Returns MEX_JSON_END when no more siblings remain.
 */
word EXPENTRY intrin_json_next(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh = (int)MexArgGetWord(&ma);

    regs_2[0] = (word)MEX_JSON_END;

    MEX_JSON *j = mex_json_slot(jh);
    if (!j || j->depth <= 0)
        return MexArgEnd(&ma);

    if (j->cursor == NULL)
    {
        /* First call after enter — move to first child */
        j->cursor = j->stack[j->depth - 1]->child;
    }
    else
    {
        /* Advance to next sibling */
        j->cursor = j->cursor->next;
    }

    if (!j->cursor)
    {
        regs_2[0] = (word)MEX_JSON_END;
        return MexArgEnd(&ma);
    }

    regs_2[0] = (word)mex_cjson_type(j->cursor);
    return MexArgEnd(&ma);
}

/** Ascend back to the parent container. Returns 0 or -1. */
word EXPENTRY intrin_json_exit(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh = (int)MexArgGetWord(&ma);

    regs_2[0] = (word)-1;

    MEX_JSON *j = mex_json_slot(jh);
    if (!j || j->depth <= 0)
        return MexArgEnd(&ma);

    j->cursor = j->stack[--j->depth];

    regs_2[0] = 0;
    return MexArgEnd(&ma);
}

/**
 * Within the current container (must be object), jump cursor to the
 * child with the given key name. Returns 0 or -1.
 */
word EXPENTRY intrin_json_find(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh    = (int)MexArgGetWord(&ma);
    char *key = MexArgGetString(&ma, FALSE);

    regs_2[0] = (word)-1;

    MEX_JSON *j = mex_json_slot(jh);
    if (!j || !key)
    {
        if (key) free(key);
        return MexArgEnd(&ma);
    }

    cJSON *parent = mex_json_parent(j);

    if (!cJSON_IsObject(parent))
    {
        free(key);
        return MexArgEnd(&ma);
    }

    cJSON *found = cJSON_GetObjectItemCaseSensitive(parent, key);
    free(key);

    if (!found)
        return MexArgEnd(&ma);

    j->cursor = found;
    regs_2[0] = 0;
    return MexArgEnd(&ma);
}

/** Reset cursor to before-first-child of the current container. */
word EXPENTRY intrin_json_rewind(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh = (int)MexArgGetWord(&ma);

    MEX_JSON *j = mex_json_slot(jh);
    if (j && j->depth > 0)
        j->cursor = NULL;  /* next json_next() returns first child again */

    return MexArgEnd(&ma);
}

/*========================================================================*
 * CURSOR READING INTRINSICS                                              *
 *========================================================================*/

/** Get JSON type at cursor. */
word EXPENTRY intrin_json_type(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh = (int)MexArgGetWord(&ma);

    MEX_JSON *j = mex_json_slot(jh);
    regs_2[0] = (word)(j ? mex_cjson_type(j->cursor) : MEX_JSON_INVALID);
    return MexArgEnd(&ma);
}

/** Get key name at cursor (when inside an object). Returns "" otherwise. */
word EXPENTRY intrin_json_key(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh = (int)MexArgGetWord(&ma);

    MEX_JSON *j = mex_json_slot(jh);
    if (j && j->cursor && j->cursor->string)
        MexReturnString(j->cursor->string);
    else
        MexReturnString("");

    return MexArgEnd(&ma);
}

/** Get cursor value as string. Auto-converts numbers/bools/null. */
word EXPENTRY intrin_json_str(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh = (int)MexArgGetWord(&ma);

    MEX_JSON *j = mex_json_slot(jh);
    if (!j || !j->cursor)
    {
        MexReturnString("");
        return MexArgEnd(&ma);
    }

    char buf[64];
    const char *val = mex_json_value_as_str(j->cursor, buf, sizeof(buf));
    MexReturnString((char *)val);

    return MexArgEnd(&ma);
}

/** Get cursor value as long. Returns 0 if not a number. */
word EXPENTRY intrin_json_num(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh = (int)MexArgGetWord(&ma);

    regs_4[0] = 0;

    MEX_JSON *j = mex_json_slot(jh);
    if (j && j->cursor && cJSON_IsNumber(j->cursor))
        regs_4[0] = (dword)(long)j->cursor->valuedouble;

    return MexArgEnd(&ma);
}

/** Get cursor value as boolean (0 or 1). */
word EXPENTRY intrin_json_bool(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh = (int)MexArgGetWord(&ma);

    regs_2[0] = 0;

    MEX_JSON *j = mex_json_slot(jh);
    if (j && j->cursor && cJSON_IsBool(j->cursor))
        regs_2[0] = (word)cJSON_IsTrue(j->cursor);

    return MexArgEnd(&ma);
}

/** Count children of the node at cursor (array/object). */
word EXPENTRY intrin_json_count(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh = (int)MexArgGetWord(&ma);

    regs_2[0] = 0;

    MEX_JSON *j = mex_json_slot(jh);
    if (j && j->cursor && (cJSON_IsArray(j->cursor) || cJSON_IsObject(j->cursor)))
        regs_2[0] = (word)cJSON_GetArraySize(j->cursor);

    return MexArgEnd(&ma);
}

/*========================================================================*
 * PATH-BASED CONVENIENCE ACCESSORS (do NOT move cursor)                  *
 *========================================================================*/

/** Get string by path. Returns "" if not found. */
word EXPENTRY intrin_json_get_str(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh    = (int)MexArgGetWord(&ma);
    char *path = MexArgGetString(&ma, FALSE);

    MEX_JSON *j = mex_json_slot(jh);
    if (!j || !path)
    {
        MexReturnString("");
        if (path) free(path);
        return MexArgEnd(&ma);
    }

    cJSON *node = json_resolve_path(j->root, path);
    free(path);

    char buf[64];
    const char *val = mex_json_value_as_str(node, buf, sizeof(buf));
    MexReturnString((char *)val);

    return MexArgEnd(&ma);
}

/** Get number by path. Returns 0 if not found. */
word EXPENTRY intrin_json_get_num(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh    = (int)MexArgGetWord(&ma);
    char *path = MexArgGetString(&ma, FALSE);

    regs_4[0] = 0;

    MEX_JSON *j = mex_json_slot(jh);
    if (j && path)
    {
        cJSON *node = json_resolve_path(j->root, path);
        if (node && cJSON_IsNumber(node))
            regs_4[0] = (dword)(long)node->valuedouble;
    }

    if (path) free(path);
    return MexArgEnd(&ma);
}

/** Get boolean by path. Returns 0 if not found. */
word EXPENTRY intrin_json_get_bool(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh    = (int)MexArgGetWord(&ma);
    char *path = MexArgGetString(&ma, FALSE);

    regs_2[0] = 0;

    MEX_JSON *j = mex_json_slot(jh);
    if (j && path)
    {
        cJSON *node = json_resolve_path(j->root, path);
        if (node && cJSON_IsBool(node))
            regs_2[0] = (word)cJSON_IsTrue(node);
    }

    if (path) free(path);
    return MexArgEnd(&ma);
}

/** Get type by path. */
word EXPENTRY intrin_json_get_type(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh    = (int)MexArgGetWord(&ma);
    char *path = MexArgGetString(&ma, FALSE);

    regs_2[0] = (word)MEX_JSON_INVALID;

    MEX_JSON *j = mex_json_slot(jh);
    if (j && path)
    {
        cJSON *node = json_resolve_path(j->root, path);
        regs_2[0] = (word)mex_cjson_type(node);
    }

    if (path) free(path);
    return MexArgEnd(&ma);
}

/** Count children at path. */
word EXPENTRY intrin_json_get_count(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh    = (int)MexArgGetWord(&ma);
    char *path = MexArgGetString(&ma, FALSE);

    regs_2[0] = 0;

    MEX_JSON *j = mex_json_slot(jh);
    if (j && path)
    {
        cJSON *node = json_resolve_path(j->root, path);
        if (node && (cJSON_IsArray(node) || cJSON_IsObject(node)))
            regs_2[0] = (word)cJSON_GetArraySize(node);
    }

    if (path) free(path);
    return MexArgEnd(&ma);
}

/*========================================================================*
 * BUILDER INTRINSICS                                                     *
 *========================================================================*/

/** Set a string value at key in current parent object. Returns 0 or -1. */
word EXPENTRY intrin_json_set_str(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh    = (int)MexArgGetWord(&ma);
    char *key = MexArgGetString(&ma, FALSE);
    char *val = MexArgGetString(&ma, FALSE);

    regs_2[0] = (word)-1;

    MEX_JSON *j = mex_json_slot(jh);
    if (!j || !key || !val)
    {
        if (key) free(key);
        if (val) free(val);
        return MexArgEnd(&ma);
    }

    cJSON *parent = mex_json_parent(j);
    if (!cJSON_IsObject(parent))
    {
        free(key);
        free(val);
        return MexArgEnd(&ma);
    }

    /* Replace if exists, otherwise add */
    cJSON *existing = cJSON_GetObjectItemCaseSensitive(parent, key);
    if (existing)
        cJSON_SetValuestring(existing, val);
    else
        cJSON_AddItemToObject(parent, key, cJSON_CreateString(val));

    free(key);
    free(val);
    regs_2[0] = 0;
    return MexArgEnd(&ma);
}

/** Set a numeric value at key in current parent object. */
word EXPENTRY intrin_json_set_num(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh    = (int)MexArgGetWord(&ma);
    char *key = MexArgGetString(&ma, FALSE);
    dword val = MexArgGetDword(&ma);

    regs_2[0] = (word)-1;

    MEX_JSON *j = mex_json_slot(jh);
    if (!j || !key)
    {
        if (key) free(key);
        return MexArgEnd(&ma);
    }

    cJSON *parent = mex_json_parent(j);
    if (!cJSON_IsObject(parent))
    {
        free(key);
        return MexArgEnd(&ma);
    }

    cJSON *existing = cJSON_GetObjectItemCaseSensitive(parent, key);
    if (existing)
        cJSON_SetNumberValue(existing, (double)(long)val);
    else
        cJSON_AddItemToObject(parent, key, cJSON_CreateNumber((double)(long)val));

    free(key);
    regs_2[0] = 0;
    return MexArgEnd(&ma);
}

/** Set a boolean value at key in current parent object. */
word EXPENTRY intrin_json_set_bool(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh    = (int)MexArgGetWord(&ma);
    char *key = MexArgGetString(&ma, FALSE);
    word val  = MexArgGetWord(&ma);

    regs_2[0] = (word)-1;

    MEX_JSON *j = mex_json_slot(jh);
    if (!j || !key)
    {
        if (key) free(key);
        return MexArgEnd(&ma);
    }

    cJSON *parent = mex_json_parent(j);
    if (!cJSON_IsObject(parent))
    {
        free(key);
        return MexArgEnd(&ma);
    }

    cJSON *existing = cJSON_GetObjectItemCaseSensitive(parent, key);
    if (existing)
    {
        /* cJSON doesn't have SetBoolValue; delete and re-add */
        cJSON_DeleteItemFromObjectCaseSensitive(parent, key);
    }
    cJSON_AddItemToObject(parent, key, cJSON_CreateBool(val ? 1 : 0));

    free(key);
    regs_2[0] = 0;
    return MexArgEnd(&ma);
}

/** Add an empty sub-object at key in current parent object. */
word EXPENTRY intrin_json_add_object(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh    = (int)MexArgGetWord(&ma);
    char *key = MexArgGetString(&ma, FALSE);

    regs_2[0] = (word)-1;

    MEX_JSON *j = mex_json_slot(jh);
    if (!j || !key)
    {
        if (key) free(key);
        return MexArgEnd(&ma);
    }

    cJSON *parent = mex_json_parent(j);
    if (!cJSON_IsObject(parent))
    {
        free(key);
        return MexArgEnd(&ma);
    }

    cJSON_AddItemToObject(parent, key, cJSON_CreateObject());
    free(key);
    regs_2[0] = 0;
    return MexArgEnd(&ma);
}

/** Add an empty sub-array at key in current parent object. */
word EXPENTRY intrin_json_add_array(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh    = (int)MexArgGetWord(&ma);
    char *key = MexArgGetString(&ma, FALSE);

    regs_2[0] = (word)-1;

    MEX_JSON *j = mex_json_slot(jh);
    if (!j || !key)
    {
        if (key) free(key);
        return MexArgEnd(&ma);
    }

    cJSON *parent = mex_json_parent(j);
    if (!cJSON_IsObject(parent))
    {
        free(key);
        return MexArgEnd(&ma);
    }

    cJSON_AddItemToObject(parent, key, cJSON_CreateArray());
    free(key);
    regs_2[0] = 0;
    return MexArgEnd(&ma);
}

/** Append a string value to the current parent array. */
word EXPENTRY intrin_json_array_push_str(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh    = (int)MexArgGetWord(&ma);
    char *val = MexArgGetString(&ma, FALSE);

    regs_2[0] = (word)-1;

    MEX_JSON *j = mex_json_slot(jh);
    if (!j || !val)
    {
        if (val) free(val);
        return MexArgEnd(&ma);
    }

    cJSON *parent = mex_json_parent(j);
    if (!cJSON_IsArray(parent))
    {
        free(val);
        return MexArgEnd(&ma);
    }

    cJSON_AddItemToArray(parent, cJSON_CreateString(val));
    free(val);
    regs_2[0] = 0;
    return MexArgEnd(&ma);
}

/** Append a numeric value to the current parent array. */
word EXPENTRY intrin_json_array_push_num(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh   = (int)MexArgGetWord(&ma);
    dword val = MexArgGetDword(&ma);

    regs_2[0] = (word)-1;

    MEX_JSON *j = mex_json_slot(jh);
    if (!j)
        return MexArgEnd(&ma);

    cJSON *parent = mex_json_parent(j);
    if (!cJSON_IsArray(parent))
        return MexArgEnd(&ma);

    cJSON_AddItemToArray(parent, cJSON_CreateNumber((double)(long)val));
    regs_2[0] = 0;
    return MexArgEnd(&ma);
}

/** Serialize the entire JSON tree to a compact string. */
word EXPENTRY intrin_json_serialize(void)
{
    MA ma;
    MexArgBegin(&ma);
    int jh = (int)MexArgGetWord(&ma);

    MEX_JSON *j = mex_json_slot(jh);
    if (!j)
    {
        MexReturnString("");
        return MexArgEnd(&ma);
    }

    char *out = cJSON_PrintUnformatted(j->root);
    if (out)
    {
        MexReturnString(out);
        cJSON_free(out);
    }
    else
        MexReturnString("");

    return MexArgEnd(&ma);
}

#endif /* MEX */
