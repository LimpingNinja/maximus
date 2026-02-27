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
 * @file m_ngread.c
 * @brief NG (NextGen) message reading system — entry prompts, lightbar
 *        index view, and full-screen reader loop.
 *
 * Implements the StormBBS-style message browsing experience:
 *   - Storm-style entry prompts (Forward/New/By You/Yours/Search)
 *   - Paged lightbar message index with scrolling
 *   - Full-screen reader with scroll-within-message and key dispatch
 */

#define MAX_LANG_m_browse
#define MAX_LANG_m_area

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "prog.h"
#include "mm.h"
#include "max_msg.h"
#include "m_index.h"
#include "m_full.h"
#include "protod.h"
#include "ui_lightbar.h"
#include "ui_scroll.h"
#include "ui_field.h"
#include "keys.h"
#include "mci.h"
#include "display.h"

/* Return codes from the FSR reader loop */
#define FSR_EXIT_INDEX   0   /* Return to lightbar index */
#define FSR_NEXT_BASE    1   /* Advance to next message base in scan */
#define FSR_QUIT         2   /* Exit the entire NG read system */
#define FSR_PREV_BASE    3   /* Move to previous message base in scan */

/* Index chrome helpers defined later in this file. */
static void draw_index_header(msg_index_t *idx);
static void draw_index_footer(void);

/**
 * @brief Prompt for a message number using the shared "From which message #"
 *        wording and current area's highest message number.
 *
 * Supports '=' to reuse the current last_msg, Enter/blank to default to
 * last_msg, and UMSGID conversion when enabled in session settings.
 *
 * @param highmsg   Highest message number in the current area.
 * @param out_msgn  Receives resolved message number on success.
 * @return 1 if a usable message number was entered, 0 otherwise.
 */
static int ng_prompt_from_message_number(dword highmsg, dword *out_msgn)
{
  char input[PATHLEN];
  char highmsg_buf[32];
  const char *prompt_tpl = maxlang_get(g_current_lang,
                                        "m_browse.ng_read_from_prompt");

  if (!out_msgn)
    return 0;

  snprintf(highmsg_buf, sizeof(highmsg_buf), "%lu", (unsigned long)highmsg);

  WhiteN();
  InputGets(input, (char *)prompt_tpl, highmsg_buf);

  if (!*input || eqstri(input, eq))
    *out_msgn = last_msg;
  else
  {
    *out_msgn = (dword)atol(input);

    if (ngcfg_get_bool("general.session.use_umsgids"))
      *out_msgn = MsgUidToMsgn(sq, *out_msgn, UID_EXACT);
  }

  return !!*out_msgn;
}

/**
 * @brief Prompt for a yes/no confirmation on the terminal bottom line.
 *
 * @param prompt  Prompt text (supports MCI codes).
 * @return 1 for yes, 0 for no/other.
 */
static int ng_prompt_bottom_yesno(const char *prompt)
{
  int tl = TermLength();

  ui_goto(tl, 1);
  return (GetyNAnswer((char *)prompt, CINPUT_MSGREAD | CINPUT_NOLF) == YES);
}

/**
 * @brief Find jump target in index, preferring exact match then next >= msgn.
 *
 * @param idx   Message index.
 * @param msgn  Target message number.
 * @return 0-based entry index, or -1 if no visible message qualifies.
 */
static int ng_find_jump_target(msg_index_t *idx, dword msgn)
{
  int i;
  int exact;

  if (!idx || idx->count <= 0)
    return -1;

  exact = msg_index_find_msgn(idx, msgn);
  if (exact >= 0)
    return exact;

  for (i = 0; i < idx->count; i++)
  {
    if (idx->entries[i].msgn >= msgn)
      return i;
  }

  return -1;
}

/**
 * @brief Remove the currently selected entry from the in-memory index.
 *
 * @param idx      Message index to modify.
 * @param current  In/out selected index.
 */
static void ng_remove_current_entry(msg_index_t *idx, int *current)
{
  int cur;

  if (!idx || !current || idx->count <= 0)
    return;

  cur = *current;
  if (cur < 0 || cur >= idx->count)
    return;

  if (cur < idx->count - 1)
  {
    memmove(&idx->entries[cur],
            &idx->entries[cur + 1],
            (size_t)(idx->count - cur - 1) * sizeof(msg_index_entry_t));
  }

  idx->count--;

  if (idx->count <= 0)
  {
    *current = 0;
    return;
  }

  if (cur >= idx->count)
    cur = idx->count - 1;

  *current = cur;
}

/* ------------------------------------------------------------------ */
/*  Phase 2: Storm-Style Entry Prompts                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief Display the "Read messages" filter prompt and collect user choice.
 *
 * Prompt: (F)orward, (N)ew, (B)y You, (Y)ours, (S)earch, (Q)uit?
 *
 * @param[out] filter_flags  Resulting MI_FILTER_* flags.
 * @param[out] start_msgn    Start message number (for Forward).
 * @param[out] search_buf    Search string buffer (for Search).
 * @param      search_sz     Size of search_buf.
 * @return 1 if user selected a filter, 0 if quit.
 */
static int ng_read_prompt(word *filter_flags, dword *start_msgn,
                          char *search_buf, size_t search_sz)
{
  char ch;

  *filter_flags = MI_FILTER_ALL;
  *start_msgn = 0;
  search_buf[0] = '\0';

  for (;;)
  {
    Puts((char *)maxlang_get(g_current_lang, "m_browse.ng_read_prompt"));

    ch = (char)toupper(KeyGetRNP(""));
    Puts("\r\n");

    switch (ch)
    {
      case 0:
      case '\r':
      case '\n':
      case 'F':
      {
        dword highmsg = (dword)MsgGetHighMsg(sq);

        if (!ng_prompt_from_message_number(highmsg, start_msgn))
          *start_msgn = 1;

        *filter_flags = MI_FILTER_FROM;
        return 1;
      }

      case 'N':
        *filter_flags = MI_FILTER_NEW;
        return 1;

      case 'B':
        *filter_flags = MI_FILTER_BY_YOU;
        return 1;

      case 'Y':
        *filter_flags = MI_FILTER_YOURS;
        return 1;

      case 'S':
      {
        InputGets(search_buf, (char *)maxlang_get(g_current_lang, "m_browse.ng_read_search_prompt"));
        if (!*search_buf)
          continue;  /* Empty search, re-prompt */
        *filter_flags = MI_FILTER_SEARCH;
        return 1;
      }

      case 'Q':
      case 27:   /* ESC */
        return 0;
    }
  }
}

/**
 * @brief Render the index chrome for an empty area and wait for navigation.
 *
 * Keeps the user inside list mode so Left/Right can still move between areas.
 *
 * @param idx Message index (used for area header text).
 * @return FSR_NEXT_BASE, FSR_PREV_BASE, or 0 (quit list view).
 */
static int ng_empty_area_index_view(msg_index_t *idx)
{
  for (;;)
  {
    int row;
    int tl = TermLength();

    draw_index_header(idx);
    draw_index_footer();

    /* Clear list body area and place a centered-ish status line. */
    for (row = 4; row <= tl - 3; row++)
    {
      ui_goto(row, 1);
      Puts("|[K");
    }

    Puts((char *)maxlang_get(g_current_lang, "m_browse.ng_no_messages_area_inline"));
    vbuf_flush();

    {
      int k = ui_read_key();

      if (k == K_LEFT)
        return FSR_PREV_BASE;

      if (k == K_RIGHT || k == 'g' || k == 'G')
        return FSR_NEXT_BASE;

      if (k == K_ESC || k == 'q' || k == 'Q')
        return 0;
    }
  }
}


/* ------------------------------------------------------------------ */
/*  Phase 3: Lightbar Message Index View                               */
/* ------------------------------------------------------------------ */

/**
 * @brief Draw the index header using lang prompts.
 *
 * Row 1: ┌── Message Index Listing ───...── <area_desc> ─┐
 * Row 2: │  ##   Subject ... From ... To             │
 * Row 3: └─────────────────────────────────────────────────┘
 */
static void draw_index_header(msg_index_t *idx)
{
  /* Precomputed fixed overhead for row 1 framing and title. */
  const int k_hdr1_overhead = 30;
  int fill = TermWidth() - k_hdr1_overhead - (int)strlen(idx->area_desc);
  if (fill < 1) fill = 1;
  char fw[8];
  snprintf(fw, sizeof(fw), "%02d", fill);

  LangPrintf(br_list_head1, idx->area_desc, fw);
  Puts(br_list_head2);
}

/**
 * @brief Draw the index footer using lang prompt.
 *
 * Footer occupies 2 rows: separator line + help text.
 */
static void draw_index_footer(void)
{
  int tl = TermLength();
  ui_goto(tl - 1, 1);
  Puts(br_list_foot);
}

/**
 * @brief Flash a short boundary notice when there is no next/prev area.
 */
static void ng_flash_area_boundary_notice(void)
{
  int tl = TermLength();
  const char *msg = maxlang_get(g_current_lang,
                                "m_browse.ng_no_adjacent_area");

  ui_goto(tl, 1);
  Puts((char *)msg);
  Puts("|[K");
  vbuf_flush();

  /* Delay uses hundredths of a second; 100 = ~1 second. */
  Delay(100);

  ui_goto(tl, 1);
  Puts("|[K");
  vbuf_flush();
}

/**
 * @brief Persist updated lastread pointer for the current area if needed.
 *
 * This keeps NG reader behavior aligned with classic message reading flow
 * when returning from FSR or hopping between areas.
 */
static void ng_flush_lastread(msg_index_t *idx)
{
  if (!idx || !sq)
    return;

  if (last_msg > idx->lastread)
  {
    FixLastread(sq, mah.ma.type, last_msg, MAS(mah, path));
    idx->lastread = last_msg;
  }
}

/**
 * @brief Find the next or previous visible message area within the division.
 *
 * Mirrors the SearchArea() logic in m_area.c but is accessible from NG reader.
 * On success, copies the new area into *pmahDest and updates usr.msg.
 *
 * @param direction  1 = next area, -1 = previous area.
 * @param pmahDest   Receives the new area header on success.
 * @return 1 if a new area was found, 0 if we're at the boundary.
 */
static int ng_find_adjacent_area(int direction, PMAH pmahDest)
{
  MAH ma;
  HAFF haff;
  BARINFO bi;
  int found = 0;

  memset(&ma, 0, sizeof(ma));
  memset(&bi, 0, sizeof(bi));

  /* Open area file positioned at current area */
  haff = AreaFileFindOpen(ham, usr.msg, 0);
  if (!haff)
    return 0;

  /* Confirm current area exists */
  if (AreaFileFindNext(haff, &ma, FALSE) != 0)
  {
    AreaFileFindClose(haff);
    return 0;
  }

  /* Switch to wildcard search for next/prev */
  AreaFileFindChange(haff, NULL, 0);

  while ((direction == -1 ? AreaFileFindPrior : AreaFileFindNext)(haff, &ma, TRUE) == 0)
  {
    if ((ma.ma.attribs & MA_HIDDN) == 0 &&
        (ma.ma.attribs_2 & MA2_EMAIL) == 0 &&
        ValidMsgArea(NULL, &ma, VA_VAL | VA_PWD | VA_EXTONLY, &bi))
    {
      SetAreaName(usr.msg, MAS(ma, name));
      CopyMsgArea(pmahDest, &ma);
      found = 1;
      break;
    }
  }

  AreaFileFindClose(haff);
  DisposeMah(&ma);
  return found;
}

/**
 * @brief Parse a color nibble (0..15) from a name or hex digit string.
 */
static int lb_mr_parse_color_nibble(const char *s, int *out)
{
  if (!s || !*s || !out) return 0;
  if (*s == '|') s++;
  if (eqstri((char *)s, "black"))       { *out = 0;  return 1; }
  if (eqstri((char *)s, "blue"))        { *out = 1;  return 1; }
  if (eqstri((char *)s, "green"))       { *out = 2;  return 1; }
  if (eqstri((char *)s, "cyan"))        { *out = 3;  return 1; }
  if (eqstri((char *)s, "red"))         { *out = 4;  return 1; }
  if (eqstri((char *)s, "magenta"))     { *out = 5;  return 1; }
  if (eqstri((char *)s, "brown"))       { *out = 6;  return 1; }
  if (eqstri((char *)s, "gray") || eqstri((char *)s, "grey"))
                                         { *out = 7;  return 1; }
  if (eqstri((char *)s, "darkgray") || eqstri((char *)s, "darkgrey"))
                                         { *out = 8;  return 1; }
  if (eqstri((char *)s, "lightblue"))   { *out = 9;  return 1; }
  if (eqstri((char *)s, "lightgreen"))  { *out = 10; return 1; }
  if (eqstri((char *)s, "lightcyan"))   { *out = 11; return 1; }
  if (eqstri((char *)s, "lightred"))    { *out = 12; return 1; }
  if (eqstri((char *)s, "lightmagenta")){ *out = 13; return 1; }
  if (eqstri((char *)s, "yellow"))      { *out = 14; return 1; }
  if (eqstri((char *)s, "white"))       { *out = 15; return 1; }
  char *end = NULL;
  long v = strtol(s, &end, 16);
  if (end && *end == '\0' && v >= 0 && v <= 15)
    { *out = (int)v; return 1; }
  return 0;
}

/**
 * @brief Build lightbar attrs from display.toml [msg_reader] settings.
 *
 * Default: normal = 0x07, selected = background-only (blue, 0x10)
 * so MCI foreground colors in br_list_format bleed through.
 */
static void lb_mr_get_lightbar_attrs(byte *normal_attr, byte *selected_attr)
{
  byte normal = Mci2Attr("|tx", 0x07);
  byte bg_default = Mci2Attr("|17", 0x17);
  const char *fore = ngcfg_get_string_raw("general.display.msg_reader.lightbar_fore");
  const char *back = ngcfg_get_string_raw("general.display.msg_reader.lightbar_back");
  int fore_nibble = -1;
  int back_nibble = -1;

  if (fore && *fore)
    lb_mr_parse_color_nibble(fore, &fore_nibble);
  if (back && *back)
    lb_mr_parse_color_nibble(back, &back_nibble);

  /* Default highlight: preserve normal foreground, use theme lightbar background.
   * MCI/theme pipe codes in the row can still override the foreground.
   */
  byte selected = (byte)((normal & 0x0f) | (bg_default & 0x70));

  if (fore_nibble >= 0)
    selected = (byte)((selected & 0xf0) | (byte)fore_nibble);
  if (back_nibble >= 0)
    selected = (byte)((selected & 0x0f) | (byte)(back_nibble << 4));

  *normal_attr = normal;
  *selected_attr = selected;
}

/**
 * @brief Run the lightbar index view for the given message index.
 *
 * @param idx          Pre-built message index.
 * @param start_index  Initial selected index (0-based).
 * @return 0 = normal exit, FSR_NEXT_BASE = advance to next base.
 */
static int ng_msg_index_view(msg_index_t *idx, int start_index)
{
  if (idx->count == 0)
  {
    Puts((char *)maxlang_get(g_current_lang, "m_browse.ng_no_messages_found"));
    return 0;
  }

  for (;;)
  {
    draw_index_header(idx);
    draw_index_footer();

    int tl = TermLength();

    ui_lightbar_list_t list;
    memset(&list, 0, sizeof(list));

    list.x = 1;
    list.y = 4;               /* After header (3 rows) */
    list.width = TermWidth();
    list.height = tl - 6;     /* Keep one extra row clear to avoid scroll/clipping */
    list.count = idx->count;
    list.initial_index = start_index;
    list.get_item = msg_index_format_row;
    list.ctx = idx;
    byte n_attr, s_attr;
    lb_mr_get_lightbar_attrs(&n_attr, &s_attr);
    list.normal_attr = n_attr;
    list.selected_attr = s_attr;
    list.wrap = 0;
    list.passthrough_lr_keys = 1;

    int out_key = 0;
    list.out_key = &out_key;

    int result = ui_lightbar_list_run(&list);

    if (result >= 0)
    {
      /* User selected a message — enter the FSR reader */
      int reader_pos = result;
      int fsr_result = ng_msg_reader_loop(idx, result, &reader_pos);

      /* Persist any read-progress before changing context. */
      ng_flush_lastread(idx);

      if (fsr_result == FSR_NEXT_BASE)
        return FSR_NEXT_BASE;

      if (fsr_result == FSR_PREV_BASE)
        return FSR_PREV_BASE;

      if (fsr_result == FSR_QUIT)
        return 0;

      /* FSR_EXIT_INDEX: return to index at the message they were viewing */
      start_index = reader_pos;
      continue;
    }
    else if (result == -1)  /* ESC */
    {
      ng_flush_lastread(idx);
      return 0;
    }
    else if (result == LB_LIST_KEY_PASSTHROUGH)
    {
      if (out_key == 'G' || out_key == 'g' || out_key == K_RIGHT)
      {
        ng_flush_lastread(idx);
        return FSR_NEXT_BASE;
      }

      if (out_key == K_LEFT)
      {
        ng_flush_lastread(idx);
        return FSR_PREV_BASE;
      }

      /* Ignore other keys, re-display */
      start_index = list.initial_index;
    }
  }
}


/* ------------------------------------------------------------------ */
/*  Phase 4: Full-Screen Reader Loop                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief Read message header and body from the message base.
 *
 * @param ha       Open message area handle.
 * @param msgn     Message number to read.
 * @param out_xmsg If non-NULL, receives the XMSG header.
 * @return Allocated body text (caller frees), or NULL on error.
 */
static char *read_message(HAREA ha, dword msgn, XMSG *out_xmsg)
{
  HMSG hmsg;
  XMSG xmsg;
  dword text_len;
  char *body = NULL;

  hmsg = MsgOpenMsg(ha, MOPEN_READ, msgn);
  if (!hmsg)
    return NULL;

  /* Read header to position, then get text length */
  if (MsgReadMsg(hmsg, &xmsg, 0L, 0L, NULL, 0L, NULL) == (dword)-1L)
  {
    MsgCloseMsg(hmsg);
    return NULL;
  }

  if (out_xmsg)
    *out_xmsg = xmsg;

  text_len = MsgGetTextLen(hmsg);
  if (text_len == 0 || text_len == (dword)-1L)
  {
    MsgCloseMsg(hmsg);
    /* Return empty string rather than NULL for empty messages */
    body = malloc(1);
    if (body) body[0] = '\0';
    return body;
  }

  body = malloc((size_t)(text_len + 1));
  if (!body)
  {
    MsgCloseMsg(hmsg);
    return NULL;
  }

  if (MsgReadMsg(hmsg, NULL, 0L, text_len, (byte *)body, 0L, NULL)
      == (dword)-1L)
  {
    free(body);
    MsgCloseMsg(hmsg);
    return NULL;
  }

  body[text_len] = '\0';
  MsgCloseMsg(hmsg);

  /* Normalize FidoNet line endings: bare \r → \n so the text viewer
   * splits lines correctly.  \r\n pairs are left alone (the viewer
   * strips trailing \r before \n). */
  {
    char *p;
    for (p = body; *p; p++)
    {
      if (*p == '\r' && p[1] != '\n')
        *p = '\n';
    }
  }

  return body;
}

/* draw_reader_header removed — we now use the existing
 * DrawReaderScreen() + DisplayMessageHeader() from m_full.c
 * which are fully themed via lang strings. */

/**
 * @brief Return true if a message line is a quoted line.
 *
 * Matches the classic Maximus heuristic: a '>' appearing within the
 * first five characters of the line (allows "XX> ", "> ", etc.).
 */
static int near ng_line_is_quote(const char *line)
{
  int i;
  for (i = 0; i < 5 && line[i]; i++)
    if (line[i] == '>')
      return 1;
  return 0;
}

/**
 * @brief Colorize message body text using Avatar attribute sequences.
 *
 * Walks the raw body line by line. Quote lines are wrapped with
 * \x16\x01<quote_attr> at the start and \x16\x01<body_attr> at the
 * end so that ui_text_viewer / ui_shadowbuf_normalize_line picks them
 * up without needing MCI injection.
 *
 * @param body       Raw message body (NUL-terminated).
 * @param body_attr  Default body attribute byte.
 * @param quote_attr Quote line attribute byte.
 * @return Newly allocated colorized string (caller frees), or NULL.
 */
static char *ng_colorize_body(const char *body, byte body_attr, byte quote_attr)
{
  /* Worst case: every line is a quote line adding 6 extra bytes (two
   * 3-byte Avatar sequences). Over-allocate generously. */
  size_t in_len = strlen(body);
  size_t out_cap = in_len + (in_len / 2) + 64;
  char *out = (char *)malloc(out_cap);
  char *wp;
  const char *p;
  const char *line_start;

  if (!out)
    return NULL;

  wp = out;
  p = body;
  line_start = p;

  while (1)
  {
    if (*p == '\n' || *p == '\0')
    {
      /* Ensure room for avatar prefix (3) + line + avatar suffix (3) + newline */
      size_t line_len = (size_t)(p - line_start);
      size_t needed = (size_t)(wp - out) + line_len + 8;

      if (needed >= out_cap)
      {
        size_t wp_off = (size_t)(wp - out);
        out_cap = needed * 2 + 64;
        char *new_out = (char *)realloc(out, out_cap);
        if (!new_out) { free(out); return NULL; }
        out = new_out;
        wp = out + wp_off;
      }

      if (ng_line_is_quote(line_start))
      {
        /* Inject quote color at start of line */
        *wp++ = '\x16'; *wp++ = '\x01'; *wp++ = (char)quote_attr;
        memcpy(wp, line_start, line_len);
        wp += line_len;
        /* Reset to body color at end of line */
        *wp++ = '\x16'; *wp++ = '\x01'; *wp++ = (char)body_attr;
      }
      else
      {
        memcpy(wp, line_start, line_len);
        wp += line_len;
      }

      if (*p == '\0')
        break;

      *wp++ = '\n';
      p++;
      line_start = p;
      continue;
    }

    if (*p == '\0')
      break;

    p++;
  }

  *wp = '\0';
  return out;
}

/**
 * @brief Draw the FSR footer status line.
 */
static void draw_reader_footer(int line_pos, int total_lines)
{
  int tl = TermLength();

  ui_goto(tl, 1);
  Puts((char *)maxlang_get(g_current_lang, "m_browse.ng_reader_footer"));
}

/**
 * @brief The full-screen reader loop.
 *
 * Displays one message at a time in a text viewer with scroll support.
 * Left/Right navigate between messages. [/] follow thread links.
 * ESC returns to the lightbar index.
 *
 * @param idx           Message index.
 * @param entry_index   Starting index into idx->entries[].
 * @param out_index     If non-NULL, receives the index of the message
 *                      the user was viewing when they exited.
 * @return FSR_EXIT_INDEX, FSR_NEXT_BASE, or FSR_QUIT.
 */
int ng_msg_reader_loop(msg_index_t *idx, int entry_index, int *out_index)
{
  int current = entry_index;

  while (current >= 0 && current < idx->count)
  {
    msg_index_entry_t *e = &idx->entries[current];

    /* Read message header + body */
    XMSG xmsg;
    char *body = read_message(sq, e->msgn, &xmsg);
    if (!body)
    {
      char _ib[32], out[128];
      snprintf(_ib, sizeof(_ib), "%lu", (unsigned long)e->msgn);
      LangSprintf(out, sizeof(out),
                  maxlang_get(g_current_lang, "m_browse.ng_cannot_read_msg"),
                  _ib);
      Puts(out);
      return FSR_EXIT_INDEX;
    }

    /* Draw the standard reader header chrome (themed via lang strings) */
    word msgoffset = 0;
    long highmsg = (long)MsgGetHighMsg(sq);
    DrawReaderScreen(&mah, FALSE);
    DisplayMessageHeader(&xmsg, &msgoffset, (long)e->msgn, highmsg, &mah);

    /* Set up text viewer for the body area */
    int tl = TermLength();
    int tw = TermWidth();
    int body_y = (int)msgoffset + 1; /* msgoffset is the header line count */
    int body_h = tl - body_y;        /* Leave 1 row for footer */
    if (body_h < 3) body_h = 3;

    ui_text_viewer_style_t vs;
    ui_text_viewer_style_default(&vs);
    vs.attr = Mci2Attr("|tx", 0x07);
    vs.flags = UI_TBV_SHOW_SCROLLBAR | UI_TBV_SHOW_STATUS;
    vs.scrollbar_attr = Mci2Attr("|dm", 0x08);

    byte body_attr  = Mci2Attr("|tx", 0x07);
    byte quote_attr = Mci2Attr("|qt", 0x09);
    char *cbody = ng_colorize_body(body, body_attr, quote_attr);

    ui_text_viewer_t viewer;
    ui_text_viewer_init(&viewer, 1, body_y, tw - 1, body_h, &vs);
    ui_text_viewer_set_text(&viewer, cbody ? cbody : body);
    if (cbody) free(cbody);
    ui_text_viewer_render(&viewer);

    draw_reader_footer(0, viewer.line_count);

    /* Update lastread pointer and mark as read */
    if (e->msgn > last_msg)
      last_msg = e->msgn;

    /* Keep the current lightbar index visually in sync: once opened/read,
     * this entry should no longer show the "new" marker on return. */
    e->flags &= (byte)~MI_NEW;

    /* Key dispatch loop */
    int action = FSR_EXIT_INDEX;
    for (;;)
    {
      int k = ui_text_viewer_read_key(&viewer);
      if (k == 0)
        continue;  /* Key was consumed by viewer (scroll) */

      /* Navigation: Left = previous message */
      if (k == K_LEFT || k == 'p' || k == 'P')
      {
        if (current > 0)
          current--;
        break;
      }

      /* Navigation: Right = next message */
      if (k == K_RIGHT || k == 'n' || k == 'N')
      {
        if (current < idx->count - 1)
          current++;
        else
        {
          /* At last message */
          action = FSR_EXIT_INDEX;
          goto done_viewer;
        }
        break;
      }

      /* Thread navigation: [ = reply-to (parent) */
      if (k == '[')
      {
        if (e->replyto != 0)
        {
          /* Search for the replyto UMSGID in the index */
          int found = -1;
          for (int i = 0; i < idx->count; i++)
          {
            if (idx->entries[i].uid == e->replyto)
            {
              found = i;
              break;
            }
          }
          if (found >= 0)
          {
            current = found;
            break;
          }
        }
        /* Not found or no replyto — beep */
        Putc('\a');
        continue;
      }

      /* Thread navigation: ] = first reply (child) */
      if (k == ']')
      {
        if (e->reply1 != 0)
        {
          int found = -1;
          for (int i = 0; i < idx->count; i++)
          {
            if (idx->entries[i].uid == e->reply1)
            {
              found = i;
              break;
            }
          }
          if (found >= 0)
          {
            current = found;
            break;
          }
        }
        Putc('\a');
        continue;
      }

      /* ? = show full-screen reader help file, then redraw current message */
      if (k == '?')
      {
        const char *scan_help = ngcfg_get_path("general.display_files.scan_help");

        if (scan_help && *scan_help)
        {
          /* Ensure .bbs help chrome renders pipe/MCI codes even if caller context suppressed parsing. */
          MciPushParseFlags(MCI_PARSE_ALL, MCI_PARSE_ALL);
          Display_File(DISPLAY_NONE, NULL, (char *)scan_help);
          MciPopParseFlags();
        }

        DrawReaderScreen(&mah, FALSE);
        DisplayMessageHeader(&xmsg, &msgoffset, (long)e->msgn, highmsg, &mah);
        ui_text_viewer_render(&viewer);
        draw_reader_footer(0, viewer.line_count);
        continue;
      }

      /* J = jump directly to a message number in this area/index scope */
      if (k == 'J' || k == 'j')
      {
        dword jump_msgn;
        int target;

        if (!ng_prompt_from_message_number((dword)highmsg, &jump_msgn))
          jump_msgn = 1;

        target = ng_find_jump_target(idx, jump_msgn);
        if (target >= 0)
        {
          current = target;
          break;
        }

        Putc('\a');
        DrawReaderScreen(&mah, FALSE);
        DisplayMessageHeader(&xmsg, &msgoffset, (long)e->msgn, highmsg, &mah);
        ui_text_viewer_render(&viewer);
        draw_reader_footer(0, viewer.line_count);
        continue;
      }

      /* L = return to lightbar list (same as ESC) */
      if (k == 'L' || k == 'l')
      {
        action = FSR_EXIT_INDEX;
        if (out_index) *out_index = current;
        goto done_viewer;
      }

      /* ESC = return to index */
      if (k == 27)
      {
        action = FSR_EXIT_INDEX;
        if (out_index) *out_index = current;
        goto done_viewer;
      }

      /* G = next base */
      if (k == 'G' || k == 'g')
      {
        action = FSR_NEXT_BASE;
        if (out_index) *out_index = current;
        goto done_viewer;
      }

      /* R = reply */
      if (k == 'R' || k == 'r')
      {
        dword pre_high = (dword)MsgGetHighMsg(sq);

        ui_text_viewer_free(&viewer);
        free(body);

        /* Reset attributes so editor doesn't inherit footer colors */
        Puts("|CD");

        /* Set last_msg so Msg_Reply knows which message to reply to */
        last_msg = e->msgn;
        Msg_Reply();

        /* Append any newly created message(s) to the index */
        { dword new_high = (dword)MsgGetHighMsg(sq);
          dword m;
          for (m = pre_high + 1; m <= new_high; m++)
            msg_index_append_msg(idx, sq, m);
          if (new_high > pre_high)
            highmsg = (long)new_high;
        }

        /* After reply, redisplay current message */
        body = read_message(sq, e->msgn, &xmsg);
        if (!body)
          return FSR_EXIT_INDEX;

        DrawReaderScreen(&mah, FALSE);
        DisplayMessageHeader(&xmsg, &msgoffset, (long)e->msgn, highmsg, &mah);
        body_y = (int)msgoffset + 1;
        body_h = tl - body_y;
        if (body_h < 3) body_h = 3;
        ui_text_viewer_init(&viewer, 1, body_y, tw - 1, body_h, &vs);
        ui_text_viewer_set_text(&viewer, body);
        ui_text_viewer_render(&viewer);
        draw_reader_footer(0, viewer.line_count);
        continue;
      }

      /* E = edit current message */
      if (k == 'E' || k == 'e')
      {
        if (!CanAccessMsgCommand(&mah, msg_change, 0))
        {
          Putc('\a');
          continue;
        }

        ui_text_viewer_free(&viewer);
        free(body);

        /* Reset attributes so editor doesn't inherit footer colors */
        Puts("|CD");

        last_msg = e->msgn;
        snprintf((char *)linebuf, PATHLEN, "%lu", (unsigned long)e->msgn);
        Msg_Change();

        body = read_message(sq, e->msgn, &xmsg);
        if (!body)
          return FSR_EXIT_INDEX;

        DrawReaderScreen(&mah, FALSE);
        DisplayMessageHeader(&xmsg, &msgoffset, (long)e->msgn, highmsg, &mah);
        body_y = (int)msgoffset + 1;
        body_h = tl - body_y;
        if (body_h < 3) body_h = 3;
        ui_text_viewer_init(&viewer, 1, body_y, tw - 1, body_h, &vs);
        ui_text_viewer_set_text(&viewer, body);
        ui_text_viewer_render(&viewer);
        draw_reader_footer(0, viewer.line_count);
        continue;
      }

      /* F = forward current message */
      if (k == 'F' || k == 'f')
      {
        if (!CanAccessMsgCommand(&mah, forward, 0))
        {
          Putc('\a');
          continue;
        }

        dword pre_high_f = (dword)MsgGetHighMsg(sq);

        ui_text_viewer_free(&viewer);
        free(body);

        /* Reset attributes so editor doesn't inherit footer colors */
        Puts("|CD");

        last_msg = e->msgn;
        Msg_Forward(NULL);

        /* Append any newly created message(s) to the index */
        { dword new_high_f = (dword)MsgGetHighMsg(sq);
          dword m;
          for (m = pre_high_f + 1; m <= new_high_f; m++)
            msg_index_append_msg(idx, sq, m);
          if (new_high_f > pre_high_f)
            highmsg = (long)new_high_f;
        }

        body = read_message(sq, e->msgn, &xmsg);
        if (!body)
          return FSR_EXIT_INDEX;

        DrawReaderScreen(&mah, FALSE);
        DisplayMessageHeader(&xmsg, &msgoffset, (long)e->msgn, highmsg, &mah);
        body_y = (int)msgoffset + 1;
        body_h = tl - body_y;
        if (body_h < 3) body_h = 3;
        ui_text_viewer_init(&viewer, 1, body_y, tw - 1, body_h, &vs);
        ui_text_viewer_set_text(&viewer, body);
        ui_text_viewer_render(&viewer);
        draw_reader_footer(0, viewer.line_count);
        continue;
      }

      /* D/K = delete current message */
      if (k == 'D' || k == 'd' || k == 'K' || k == 'k')
      {
        UMSGID uid = e->uid;

        if (!CanAccessMsgCommand(&mah, msg_kill, 0) || !CanKillMsg(&xmsg))
        {
          Putc('\a');
          DrawReaderScreen(&mah, FALSE);
          DisplayMessageHeader(&xmsg, &msgoffset, (long)e->msgn, highmsg, &mah);
          ui_text_viewer_render(&viewer);
          draw_reader_footer(0, viewer.line_count);
          continue;
        }

        if (!ng_prompt_bottom_yesno(maxlang_get(g_current_lang,
                                                "m_browse.ng_reader_delete_confirm")))
        {
          DrawReaderScreen(&mah, FALSE);
          DisplayMessageHeader(&xmsg, &msgoffset, (long)e->msgn, highmsg, &mah);
          ui_text_viewer_render(&viewer);
          draw_reader_footer(0, viewer.line_count);
          continue;
        }

        Msg_Kill((long)e->msgn);

        if (MsgUidToMsgn(sq, uid, UID_EXACT) == 0)
          ng_remove_current_entry(idx, &current);

        if (idx->count <= 0)
          return FSR_EXIT_INDEX;

        break;  /* Re-enter outer loop */
      }

      /* M = move current message (classic Hurl behavior), stay in area */
      if (k == 'M' || k == 'm')
      {
        UMSGID uid = e->uid;

        if (!CanAccessMsgCommand(&mah, msg_hurl, 0) || !CanKillMsg(&xmsg))
        {
          Putc('\a');
          DrawReaderScreen(&mah, FALSE);
          DisplayMessageHeader(&xmsg, &msgoffset, (long)e->msgn, highmsg, &mah);
          ui_text_viewer_render(&viewer);
          draw_reader_footer(0, viewer.line_count);
          continue;
        }

        if (!ng_prompt_bottom_yesno(maxlang_get(g_current_lang,
                                                "m_browse.ng_reader_move_confirm")))
        {
          DrawReaderScreen(&mah, FALSE);
          DisplayMessageHeader(&xmsg, &msgoffset, (long)e->msgn, highmsg, &mah);
          ui_text_viewer_render(&viewer);
          draw_reader_footer(0, viewer.line_count);
          continue;
        }

        /* Reset attributes so prompts don't inherit footer colors */
        Puts("|CD");

        last_msg = e->msgn;
        Msg_Hurl();

        if (MsgUidToMsgn(sq, uid, UID_EXACT) == 0)
          ng_remove_current_entry(idx, &current);

        if (idx->count <= 0)
          return FSR_EXIT_INDEX;

        break;  /* Re-enter outer loop */
      }

      /* Ignore other keys */
    }

    ui_text_viewer_free(&viewer);
    free(body);
    continue;

done_viewer:
    ui_text_viewer_free(&viewer);
    free(body);
    return action;
  }

  return FSR_EXIT_INDEX;
}


/* ------------------------------------------------------------------ */
/*  Phase 5: Public Entry Points                                       */
/* ------------------------------------------------------------------ */

/**
 * @brief "Read messages" — single-area NG reader entry point.
 *
 * Prompts for filter type, builds the index, launches the lightbar
 * index view, which in turn enters the FSR reader.
 */
void Msg_NG_Read(void)
{
  word filter_flags;
  dword start_msgn;
  char search_buf[80];

  if (!ng_read_prompt(&filter_flags, &start_msgn, search_buf,
                      sizeof(search_buf)))
    return;  /* User chose Quit */

  /* "From #" should not filter the list; it only controls initial focus. */
  word build_flags = filter_flags;
  if (build_flags & MI_FILTER_FROM)
    build_flags &= (word)~MI_FILTER_FROM;

  /* Area-switching loop: Left/Right in the index view will cycle areas. */
  for (;;)
  {
    msg_index_t idx;
    int view_rc;

    if (msg_index_build_filtered(&idx, sq, &mah, build_flags,
                                 start_msgn,
                                 (build_flags & MI_FILTER_SEARCH)
                                   ? search_buf : NULL) < 0)
    {
      Puts((char *)maxlang_get(g_current_lang, "m_browse.ng_err_build_index"));
      return;
    }

    if (idx.count == 0)
    {
      view_rc = ng_empty_area_index_view(&idx);
      msg_index_free(&idx);
    }
    else
    {
      /* Determine starting position */
      int start = 0;
      if (filter_flags & MI_FILTER_NEW)
      {
        start = 0;
      }
      else if (filter_flags & MI_FILTER_FROM)
      {
        int found = msg_index_find_msgn(&idx, start_msgn);
        if (found >= 0)
          start = found;
        else
        {
          int i;
          for (i = 0; i < idx.count; i++)
          {
            if (idx.entries[i].msgn >= start_msgn)
            {
              start = i;
              break;
            }
          }
        }
      }

      view_rc = ng_msg_index_view(&idx, start);
      msg_index_free(&idx);
    }

    if (view_rc == FSR_NEXT_BASE || view_rc == FSR_PREV_BASE)
    {
      int dir = (view_rc == FSR_NEXT_BASE) ? 1 : -1;
      MAH new_mah;
      memset(&new_mah, 0, sizeof(new_mah));

      if (!ng_find_adjacent_area(dir, &new_mah))
      {
        /* No more areas in that direction — stay put. */
        DisposeMah(&new_mah);
        ng_flash_area_boundary_notice();
        continue;
      }

      /* Switch to the new area (handles ExitMsgArea + EnterMsgArea). */
      if (!PopPushMsgAreaSt(&new_mah, NULL))
      {
        DisposeMah(&new_mah);
        Puts((char *)maxlang_get(g_current_lang, "m_browse.ng_err_open_area"));
        return;
      }
      DisposeMah(&new_mah);

      /* Reset filter for new area: show all, start from top. */
      build_flags = MI_FILTER_ALL;
      filter_flags = MI_FILTER_ALL;
      start_msgn = 0;
      continue;
    }

    /* Normal exit (ESC) or FSR_QUIT */
    return;
  }
}

/**
 * @brief "Find messages" — multi-area NG reader entry point (stub).
 *
 * For now, this delegates to single-area Msg_NG_Read.
 * Multi-area scanning will be implemented in a later phase.
 */
void Msg_NG_Find(void)
{
  /* TODO: Add scope prompt (Current/Group/All) + area iteration */
  Msg_NG_Read();
}
