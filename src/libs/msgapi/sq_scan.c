/*
 * Maximus Version 3.02+
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
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
 * @file sq_scan.c
 * @brief Bulk message header scanning for the MsgAPI.
 *
 * Provides MsgScanHeaders() — a high-performance bulk scan that reads all
 * message headers from an area in a single pass, avoiding the per-message
 * overhead of MsgOpenMsg/MsgReadMsg/MsgCloseMsg.
 *
 * For Squish areas:
 *   1. Bulk-read the entire .SQI index into memory (_SquishBeginBuffer)
 *   2. Collect frame offsets from the buffered index
 *   3. Sort by .SQD file offset for sequential I/O
 *   4. Single forward pass reading XMSG headers from .SQD
 *
 * For SDM (*.msg) areas:
 *   Falls back to standard MsgOpenMsg/MsgReadMsg/MsgCloseMsg per message,
 *   since SDM areas are rarely large enough to benefit from optimization.
 */

#define MSGAPI_HANDLERS
#define MSGAPI_NO_OLD_TYPES

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <assert.h>
#include "prog.h"
#include "msgapi.h"
#include "api_sq.h"
#include "apidebug.h"
#include "structrw.h"

/* --- Internal types for the sort-by-offset optimization --- */

/**
 * @brief Temporary record used to sort SQI entries by .SQD file offset.
 *
 * During the Squish bulk scan, we need to read XMSG headers from the .SQD
 * file.  Frames may be scattered due to deletions and reuse.  By sorting
 * entries by their .SQD offset before reading, we turn random I/O into a
 * single forward pass, which is dramatically faster for large areas.
 */
typedef struct
{
  dword idx;    /**< Index into the caller's entries[] array (== msgn - 1) */
  FOFS  ofs;    /**< Frame offset in the .SQD file */
} scan_ofs_t;


/**
 * @brief qsort comparator: ascending .SQD file offset.
 */
static int scan_ofs_cmp(const void *a, const void *b)
{
  const scan_ofs_t *sa = (const scan_ofs_t *)a;
  const scan_ofs_t *sb = (const scan_ofs_t *)b;

  if (sa->ofs < sb->ofs) return -1;
  if (sa->ofs > sb->ofs) return  1;
  return 0;
}


/* --- Squish bulk scan implementation --- */

/**
 * @brief Bulk-scan all message headers from a Squish area.
 *
 * Algorithm:
 *   1. Buffer the .SQI index (one bulk read of ~12 bytes × num_msg).
 *   2. Walk the buffered index segments, filling entries[].msgn and
 *      entries[].umsgid, while collecting frame offsets in a temp array.
 *   3. Sort the temp array by .SQD offset for sequential access.
 *   4. Single forward pass: for each sorted entry, seek to the XMSG
 *      position (frame_ofs + cbSqhdr) and read the 238-byte header
 *      directly into entries[sorted.idx].xmsg.
 *   5. Release the SQI buffer.  entries[] remains in msgn order.
 *
 * @param ha           Open Squish area handle.
 * @param entries      Caller-allocated array, sized for at least num_msg entries.
 * @param max_entries  Size of the entries[] array.
 * @return Number of entries filled, or (dword)-1 on error.
 */
static dword near _SquishScanHeaders(HAREA ha, MSGSCAN_ENTRY *entries,
                                     dword max_entries)
{
  struct _sqdata *sqd = (struct _sqdata *)ha->apidata;
  dword num_msg = ha->num_msg;
  dword count;
  scan_ofs_t *sorted = NULL;
  SQIDX sqi;
  dword i, seg_start;
  int seg;
  int had_buffer;

  if (num_msg == 0)
    return 0;

  count = (num_msg < max_entries) ? num_msg : max_entries;

  /* Step 1: Buffer the SQI index.  _SquishBeginBuffer is ref-counted,
   *         so it's safe even if already buffered. */
  had_buffer = sqd->hix->fBuffer;
  if (!_SquishBeginBuffer(sqd->hix))
  {
    msgapierr = MERR_NOMEM;
    return (dword)-1L;
  }

  /* Step 2: Allocate temp array for sort-by-offset */
  sorted = (scan_ofs_t *)palloc((size_t)count * sizeof(scan_ofs_t));
  if (!sorted)
  {
    if (!had_buffer)
      _SquishEndBuffer(sqd->hix);
    msgapierr = MERR_NOMEM;
    return (dword)-1L;
  }

  /* Step 3: Walk buffered SQI segments, fill entries[] and sorted[] */
  seg_start = 0;  /* 0-based offset into message numbering */

  for (seg = 0, i = 0; seg < sqd->hix->cSeg && i < count; seg++)
  {
    dword seg_used = sqd->hix->pss[seg].dwUsed;
    dword j;

    for (j = 0; j < seg_used && i < count; j++, i++)
    {
      SQIDX far *psqi = &sqd->hix->pss[seg].psqi[j];

      entries[i].msgn   = i + 1;            /* 1-based message number */
      entries[i].umsgid = psqi->umsgid;

      sorted[i].idx = i;
      sorted[i].ofs = psqi->ofs;
    }
  }

  /* Step 4: Sort by .SQD offset for sequential I/O */
  qsort(sorted, (size_t)count, sizeof(scan_ofs_t), scan_ofs_cmp);

  /* Step 5: Single forward pass reading XMSG headers */
  for (i = 0; i < count; i++)
  {
    dword target_idx = sorted[i].idx;
    FOFS  frame_ofs  = sorted[i].ofs;
    long  xmsg_ofs;

    /* Skip invalid/deleted frames */
    if (frame_ofs == 0 || frame_ofs == (FOFS)-1L)
    {
      memset(&entries[target_idx].xmsg, 0, sizeof(XMSG));
      continue;
    }

    xmsg_ofs = (long)frame_ofs + (long)sqd->cbSqhdr;

    if (lseek(sqd->sfd, xmsg_ofs, SEEK_SET) != xmsg_ofs)
    {
      memset(&entries[target_idx].xmsg, 0, sizeof(XMSG));
      continue;
    }

    if (read_xmsg(sqd->sfd, &entries[target_idx].xmsg) != 1)
    {
      memset(&entries[target_idx].xmsg, 0, sizeof(XMSG));
      continue;
    }

    /* Extract UMSGID from the xmsg if present (and scrub it like
     * _SquishReadXmsg does) */
    if (entries[target_idx].xmsg.attr & MSGUID)
    {
      entries[target_idx].xmsg.attr &= ~MSGUID;
      entries[target_idx].xmsg.umsgid = 0L;
    }
  }

  /* Cleanup */
  pfree(sorted);

  if (!had_buffer)
    _SquishEndBuffer(sqd->hix);

  return count;
}


/* --- SDM fallback implementation --- */

/**
 * @brief Scan all message headers from an SDM (*.msg) area.
 *
 * Uses the standard MsgOpenMsg/MsgReadMsg/MsgCloseMsg path.  SDM areas
 * are typically small (hundreds, not thousands of messages), so the
 * per-message overhead is acceptable.
 *
 * @param ha           Open SDM area handle.
 * @param entries      Caller-allocated array.
 * @param max_entries  Size of the entries[] array.
 * @return Number of entries filled, or (dword)-1 on error.
 */
static dword near _SdmScanHeaders(HAREA ha, MSGSCAN_ENTRY *entries,
                                   dword max_entries)
{
  dword num_msg = ha->num_msg;
  dword count;
  dword i;
  dword filled = 0;

  if (num_msg == 0)
    return 0;

  count = (num_msg < max_entries) ? num_msg : max_entries;

  for (i = 1; i <= count; i++)
  {
    HMSG hmsg = MsgOpenMsg(ha, MOPEN_READ, i);

    if (!hmsg)
    {
      /* Message may have been deleted/corrupted; skip it */
      continue;
    }

    entries[filled].msgn   = i;
    entries[filled].umsgid = MsgMsgnToUid(ha, i);

    if (MsgReadMsg(hmsg, &entries[filled].xmsg, 0L, 0L, NULL, 0L, NULL)
        == (dword)-1L)
    {
      /* Header read failed; zero it out and still record the entry */
      memset(&entries[filled].xmsg, 0, sizeof(XMSG));
    }

    MsgCloseMsg(hmsg);
    filled++;
  }

  return filled;
}


/* --- Public API --- */

/**
 * @brief Bulk-scan all message headers from an open area.
 *
 * This is the primary entry point for building a message index.  It
 * dispatches to a format-specific implementation based on the area type.
 *
 * For Squish areas, this is dramatically faster than per-message
 * MsgOpenMsg/MsgReadMsg/MsgCloseMsg:
 *   - The .SQI index is bulk-read into memory (one I/O operation)
 *   - XMSG headers are read from .SQD in file-offset order (sequential I/O)
 *   - No per-message handle allocation/deallocation
 *
 * The entries[] array is filled in message-number order (entries[0] is
 * message 1, entries[1] is message 2, etc.).
 *
 * @param ha           Open area handle (Squish or SDM).
 * @param entries      Caller-allocated array of MSGSCAN_ENTRY.
 * @param max_entries  Number of elements in entries[].
 * @return Number of entries filled, or (dword)-1 on error.
 *         Check msgapierr for the specific error code.
 */
dword MAPIENTRY MsgScanHeaders(HAREA ha, MSGSCAN_ENTRY *entries,
                               dword max_entries)
{
  if (MsgInvalidHarea(ha) || !entries || max_entries == 0)
  {
    msgapierr = MERR_BADA;
    return (dword)-1L;
  }

  if (ha->type & MSGTYPE_SQUISH)
    return _SquishScanHeaders(ha, entries, max_entries);
  else
    return _SdmScanHeaders(ha, entries, max_entries);
}
