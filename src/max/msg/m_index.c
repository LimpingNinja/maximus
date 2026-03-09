/*
 * m_index.c — In-memory message index for the NG message browser
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

#define MAX_LANG_m_browse

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prog.h"
#include "mm.h"
#include "mci.h"
#include "max_msg.h"
#include "m_index.h"
#include "protod.h"
#include "newarea.h"

/* --- Internal helpers --- */

/**
 * @brief Compute MI_* flags for one message based on attributes and user.
 */
static byte compute_flags(XMSG *xmsg, dword msgn, dword lastread)
{
  byte f = 0;

  if (xmsg->attr & MSGPRIVATE)
    f |= MI_PVT;

  /* Check if message is from current user */
  if (eqstri(xmsg->from, usr.name) ||
      (*usr.alias && eqstri(xmsg->from, usr.alias)))
    f |= MI_FROM_US;

  /* Check if message is to current user */
  if (eqstri(xmsg->to, usr.name) ||
      (*usr.alias && eqstri(xmsg->to, usr.alias)))
    f |= MI_TO_US;

  /* New = after lastread AND (to us or echomail) AND not yet read */
  if (msgn > lastread && !(xmsg->attr & MSGREAD))
    f |= MI_NEW;

  return f;
}

/**
 * @brief Copy relevant XMSG fields into a msg_index_entry_t.
 */
static void fill_entry(msg_index_entry_t *e, MSGSCAN_ENTRY *scan)
{
  e->msgn = scan->msgn;
  e->uid  = scan->umsgid;

  memcpy(e->from, scan->xmsg.from, sizeof(e->from));
  e->from[sizeof(e->from) - 1] = '\0';

  memcpy(e->to, scan->xmsg.to, sizeof(e->to));
  e->to[sizeof(e->to) - 1] = '\0';

  memcpy(e->subj, scan->xmsg.subj, sizeof(e->subj));
  e->subj[sizeof(e->subj) - 1] = '\0';

  e->date    = scan->xmsg.date_written;
  e->attr    = scan->xmsg.attr;
  e->replyto = scan->xmsg.replyto;
  e->reply1  = scan->xmsg.replies[0];
}

/**
 * @brief Case-insensitive substring search.
 */
static int has_substr(const char *haystack, const char *needle)
{
  if (!needle || !*needle)
    return 1;
  return stristr((char *)haystack, (char *)needle) != NULL;
}


/* --- Public API --- */

/**
 * @brief Build an unfiltered index of all visible messages in the area.
 *
 * Convenience wrapper around msg_index_build_filtered() with MI_FILTER_ALL.
 *
 * @param idx   Index struct to populate (zeroed on entry).
 * @param ha    Open area handle.
 * @param pmah  Area header (for description, access checks).
 * @return Number of entries, or -1 on error.
 */
int msg_index_build(msg_index_t *idx, HAREA ha, PMAH pmah)
{
  return msg_index_build_filtered(idx, ha, pmah, MI_FILTER_ALL, 0, NULL);
}


/**
 * @brief Build a filtered message index using bulk header scanning.
 *
 * Scans all headers via MsgScanHeaders(), applies visibility and filter
 * checks, and populates idx->entries with matching messages.
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
                             const char *search_str)
{
  dword num;
  MSGSCAN_ENTRY *scan = NULL;
  dword got;
  dword i;

  memset(idx, 0, sizeof(*idx));

  /* Copy area metadata even if this area has no messages, so callers can
   * still render proper area chrome in empty-state views. */
  strnncpy(idx->area_name, PMAS(pmah, name), sizeof(idx->area_name));
  strnncpy(idx->area_desc, PMAS(pmah, descript), sizeof(idx->area_desc));
  idx->lastread = last_msg;  /* Current lastread pointer for this area */

  num = MsgGetNumMsg(ha);
  if (num == 0)
    return 0;

  /* Bulk-scan all headers */
  scan = malloc((size_t)num * sizeof(MSGSCAN_ENTRY));
  if (!scan)
    return -1;

  got = MsgScanHeaders(ha, scan, num);
  if (got == (dword)-1L)
  {
    free(scan);
    return -1;
  }

  /* Allocate entries array (may over-allocate; filtered builds use fewer) */
  idx->entries = malloc((size_t)got * sizeof(msg_index_entry_t));
  if (!idx->entries)
  {
    free(scan);
    return -1;
  }
  idx->capacity = (int)got;
  idx->count = 0;

  /* Populate entries with filtering */
  for (i = 0; i < got; i++)
  {
    XMSG *xmsg = &scan[i].xmsg;
    dword msgn = scan[i].msgn;

    /* Visibility check — skip private messages we can't see */
    if (!CanSeeMsg(xmsg))
      continue;

    /* Apply filters */
    if (filter_flags & MI_FILTER_FROM)
    {
      if (msgn < start_msgn)
        continue;
    }

    byte f = compute_flags(xmsg, msgn, idx->lastread);

    if (filter_flags & MI_FILTER_NEW)
    {
      if (!(f & MI_NEW))
        continue;
    }

    if (filter_flags & MI_FILTER_BY_YOU)
    {
      if (!(f & MI_FROM_US))
        continue;
    }

    if (filter_flags & MI_FILTER_YOURS)
    {
      if (!(f & MI_TO_US))
        continue;
    }

    if (filter_flags & MI_FILTER_SEARCH)
    {
      /* Search in from, to, and subject fields */
      if (!has_substr((char *)xmsg->from, search_str) &&
          !has_substr((char *)xmsg->to, search_str) &&
          !has_substr((char *)xmsg->subj, search_str))
        continue;
    }

    /* Entry passed all filters — add to index */
    msg_index_entry_t *e = &idx->entries[idx->count];
    fill_entry(e, &scan[i]);
    e->flags = f;
    idx->count++;
  }

  free(scan);
  return idx->count;
}


/**
 * @brief Free all memory associated with a message index.
 *
 * @param idx  Index to release; entries pointer is set to NULL.
 */
void msg_index_free(msg_index_t *idx)
{
  if (idx->entries)
  {
    free(idx->entries);
    idx->entries = NULL;
  }
  idx->count = 0;
  idx->capacity = 0;
}


/**
 * @brief Lightbar get_item callback — format one index row for display.
 *
 * Binds message fields into lang params and runs MciExpand() to produce
 * a fixed-width terminal-ready string from the br_list_format template.
 *
 * @param ctx     Pointer to msg_index_t.
 * @param index   0-based index into entries[].
 * @param out     Output buffer for the formatted row.
 * @param out_sz  Size of output buffer.
 * @return 0 on success, -1 if index is out of range.
 */
int msg_index_format_row(void *ctx, int index, char *out, size_t out_sz)
{
  msg_index_t *idx = (msg_index_t *)ctx;

  if (index < 0 || index >= idx->count)
    return -1;

  msg_index_entry_t *e = &idx->entries[index];

  /* Format message number as string */
  char num_buf[32];
  snprintf(num_buf, sizeof(num_buf), "%lu", (unsigned long)e->msgn);

  /* Status flag: new marker with color, or blank */
  const char *status = (e->flags & MI_NEW) ? br_msg_new
                     : ((e->flags & MI_PVT) ? br_msg_pvt : br_msg_notnew);

  /* Match area/file list behavior:
   * - Bind |!N / |#N params via g_lang_params
   * - Run MciExpand() so $L/$R/$T ops are applied before the lightbar prints.
   */
  {
    const char *v_num = num_buf;
    const char *v_status = status;
    const char *v_from = Strip_Ansi(e->from, NULL, 0);
    const char *v_to = Strip_Ansi(e->to, NULL, 0);
    const char *v_subj = Strip_Ansi(e->subj, NULL, 0);

    MciLangParams lp = {0};
    lp.count = 5;
    lp.values[0] = v_num;
    lp.values[1] = v_status;
    lp.values[2] = v_from;
    lp.values[3] = v_to;
    lp.values[4] = v_subj;

    char expanded[PATHLEN * 2];
    expanded[0] = '\0';

    g_lang_params = &lp;
    MciExpand(br_list_format, expanded, sizeof(expanded));
    g_lang_params = NULL;

    if (out_sz > 0)
    {
      snprintf(out, out_sz, "%s", expanded);

      /* Strip trailing newline just in case the lang string includes one. */
      size_t len = strlen(out);
      while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r'))
        out[--len] = '\0';
    }
  }

  return 0;
}


/**
 * @brief Append a single message to an existing index.
 *
 * Reads the header, checks visibility, and grows the array if needed.
 *
 * @param idx   Index to append to.
 * @param ha    Open area handle.
 * @param msgn  Message number to add.
 * @return 0-based index of the new entry, or -1 on error/not visible.
 */
int msg_index_append_msg(msg_index_t *idx, HAREA ha, dword msgn)
{
  HMSG hmsg;
  XMSG xmsg;
  MSGSCAN_ENTRY scan;

  if (!idx || !ha || msgn == 0)
    return -1;

  hmsg = MsgOpenMsg(ha, MOPEN_READ, msgn);
  if (!hmsg)
    return -1;

  if (MsgReadMsg(hmsg, &xmsg, 0L, 0L, NULL, 0L, NULL) == (dword)-1L)
  {
    MsgCloseMsg(hmsg);
    return -1;
  }
  MsgCloseMsg(hmsg);

  /* Visibility check */
  if (!CanSeeMsg(&xmsg))
    return -1;

  /* Grow entries array if needed */
  if (idx->count >= idx->capacity)
  {
    int new_cap = idx->capacity ? idx->capacity * 2 : 16;
    msg_index_entry_t *new_ent = realloc(idx->entries,
                                         (size_t)new_cap * sizeof(msg_index_entry_t));
    if (!new_ent)
      return -1;
    idx->entries = new_ent;
    idx->capacity = new_cap;
  }

  /* Build a MSGSCAN_ENTRY for fill_entry() */
  memset(&scan, 0, sizeof(scan));
  scan.msgn   = msgn;
  scan.umsgid = MsgMsgnToUid(ha, msgn);
  scan.xmsg   = xmsg;

  msg_index_entry_t *e = &idx->entries[idx->count];
  fill_entry(e, &scan);
  e->flags = compute_flags(&xmsg, msgn, idx->lastread);
  idx->count++;

  return idx->count - 1;
}


/**
 * @brief Find the entries[] index for a given message number.
 *
 * Linear scan; suitable for typical BBS area sizes.
 *
 * @param idx   The index to search.
 * @param msgn  Message number to find.
 * @return 0-based index, or -1 if not found.
 */
int msg_index_find_msgn(msg_index_t *idx, dword msgn)
{
  int i;

  for (i = 0; i < idx->count; i++)
  {
    if (idx->entries[i].msgn == msgn)
      return i;
  }

  return -1;
}
