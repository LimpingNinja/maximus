/*
 * Maximus Version 3.02+
 * Copyright 2024-2026 MaximusNG contributors.
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

/**
 * @file m_index.h
 * @brief In-memory message index for the NG lightbar message browser.
 *
 * Provides a compact per-message summary array built via MsgScanHeaders()
 * bulk scan.  Used by the lightbar index view and FSR reader loop to
 * navigate messages without repeated MsgAPI calls.
 */

#ifndef M_INDEX_H_DEFINED
#define M_INDEX_H_DEFINED

#include "msgapi.h"
#include "max_area.h"

/** @brief Per-message flag bits for msg_index_entry_t.flags */
#define MI_NEW      0x01  /**< Unread message addressed to us */
#define MI_PVT      0x02  /**< Private message */
#define MI_FROM_US  0x04  /**< Message is from current user */
#define MI_TO_US    0x08  /**< Message is addressed to current user */

/** @brief Filter flags for msg_index_build_filtered() */
#define MI_FILTER_ALL     0x0000  /**< All visible messages */
#define MI_FILTER_NEW     0x0001  /**< After lastread, unread to us */
#define MI_FILTER_BY_YOU  0x0002  /**< From current user */
#define MI_FILTER_YOURS   0x0004  /**< To current user */
#define MI_FILTER_SEARCH  0x0008  /**< Matches SEARCH criteria */
#define MI_FILTER_FROM    0x0010  /**< Starting from message number N */

/**
 * @brief Compact summary of one message for index display.
 */
typedef struct
{
  dword  msgn;           /**< Message number (1-based, within area) */
  UMSGID uid;            /**< Unique message ID */
  char   from[36];       /**< Sender name */
  char   to[36];         /**< Recipient name */
  char   subj[72];       /**< Subject line */
  union _stampu date;    /**< Date written */
  dword  attr;           /**< Message attributes (MSGPRIVATE, etc.) */
  UMSGID replyto;        /**< Reply-to UMSGID (for thread nav) */
  UMSGID reply1;         /**< First reply UMSGID */
  byte   flags;          /**< MI_NEW, MI_PVT, MI_FROM_US, MI_TO_US */
} msg_index_entry_t;

/**
 * @brief In-memory message index for one area.
 */
typedef struct _msg_index
{
  msg_index_entry_t *entries;   /**< Array of index entries */
  int    count;                 /**< Number of valid entries */
  int    capacity;              /**< Allocated size of entries[] */
  char   area_name[MAX_ALEN];  /**< Area tag name */
  char   area_desc[80];        /**< Area description */
  dword  lastread;              /**< Lastread pointer for this area */
} msg_index_t;


/**
 * @brief Build an unfiltered index for the current area.
 *
 * Uses MsgScanHeaders() for bulk I/O.  Populates idx->entries with
 * all visible messages.
 *
 * @param idx   Index struct to populate (zeroed by caller).
 * @param ha    Open area handle.
 * @param pmah  Area header (for description, access checks).
 * @return Number of entries, or -1 on error.
 */
int msg_index_build(msg_index_t *idx, HAREA ha, PMAH pmah);

/**
 * @brief Build a filtered index for the current area.
 *
 * @param idx           Index struct to populate.
 * @param ha            Open area handle.
 * @param pmah          Area header.
 * @param filter_flags  Bitmask of MI_FILTER_* flags.
 * @param start_msgn    Starting message number (for MI_FILTER_FROM).
 * @param search_str    Search string (for MI_FILTER_SEARCH), or NULL.
 * @return Number of entries, or -1 on error.
 */
int msg_index_build_filtered(msg_index_t *idx, HAREA ha, PMAH pmah,
                             word filter_flags, dword start_msgn,
                             const char *search_str);

/**
 * @brief Free all memory associated with an index.
 */
void msg_index_free(msg_index_t *idx);

/**
 * @brief Lightbar get_item callback for ui_lightbar_list_run().
 *
 * Formats one index row into a fixed-width string suitable for
 * terminal display.
 *
 * @param ctx       Pointer to msg_index_t.
 * @param index     0-based index into entries[].
 * @param out       Output buffer.
 * @param out_sz    Size of output buffer.
 * @return 1 on success, 0 if index is out of range.
 */
int msg_index_format_row(void *ctx, int index, char *out, size_t out_sz);

/**
 * @brief Append a single message to an existing index.
 *
 * Reads the message header from the area, checks visibility, and
 * appends it to idx->entries[].  Grows the array if needed.
 *
 * @param idx   Index to append to.
 * @param ha    Open area handle.
 * @param msgn  Message number to add.
 * @return 0-based index of the new entry, or -1 on error/not visible.
 */
int msg_index_append_msg(msg_index_t *idx, HAREA ha, dword msgn);

/**
 * @brief Find the entries[] index for a given message number.
 *
 * @param idx   The index to search.
 * @param msgn  Message number to find.
 * @return 0-based index, or -1 if not found.
 */
int msg_index_find_msgn(msg_index_t *idx, dword msgn);

#endif /* M_INDEX_H_DEFINED */
