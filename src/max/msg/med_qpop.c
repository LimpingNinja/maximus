/*
 * med_qpop.c — Mystic BBS-style popup quote window for MagnEt editor
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

/*# name=MaxEd editor: Mystic-style popup quote window (ui_scrolling_region)
*/

#define MAX_LANG_global
#define MAX_LANG_m_area
#define MAX_LANG_max_bor
#include "maxed.h"
#include "m_reply.h"
#include "keys.h"
#include "mci.h"
#include "ui_scroll.h"

/** @brief Maximum lines we'll preload from the original message. */
#define QW_MAX_LINES    512

/**
 * @brief State for the popup quote window.
 *
 * Uses ui_scrolling_region_t for the content area (rendering + painting),
 * and PutsForce() with lang strings for the static frame (borders + status).
 * Raw text lines are kept separately for insertion into the editor.
 */
typedef struct
{
  ui_scrolling_region_t  scroll;         /**< Scrolling content area            */
  char                 **lines;          /**< Raw text lines for editor insert  */
  int                    line_count;     /**< Total number of lines loaded      */
  int                    highlight;      /**< Currently highlighted line index  */
  int                    content_height; /**< Number of visible content rows    */
  int                    content_width;  /**< Total columns inside borders      */
  int                    top_row;        /**< Screen row of top border (1-based)*/
  int                    total_width;    /**< Full popup width incl. borders    */
  int                    left_width;     /**< Visible width of left border str  */
  byte                   hilite_attr;    /**< Resolved highlight bar attribute  */
  byte                   qt_attr;        /**< Resolved quote prefix attribute   */
  byte                   tx_attr;        /**< Resolved body text attribute      */
  byte                   scroll_attr;    /**< Resolved scrollbar attribute      */
  char                   initials[MAX_INITIALS]; /**< Sender's quote initials   */
  int                    attributed;     /**< TRUE if attribution line inserted */
  char                   sender[XMSG_FROM_SIZE]; /**< Original msg sender name */
  char                   recipient[XMSG_TO_SIZE]; /**< Original msg recipient  */
  char                   orig_date[40];  /**< Original message date string      */
} quote_popup_t;


/* ── forward declarations ────────────────────────────────────────────── */

static int  near Quote_PreloadMessage(struct _replyp *pr, quote_popup_t *qp);
static void near Quote_FreeLines(quote_popup_t *qp);
static void near Quote_RenderFrame(const quote_popup_t *qp);
static void near Quote_RenderContent(quote_popup_t *qp);
static int  near Quote_InsertLine(const quote_popup_t *qp, const char *line);
static void near Quote_EnsureVisible(quote_popup_t *qp);
static void near Quote_LiveUpdate(quote_popup_t *qp);
static int  near Quote_KeyLoop(quote_popup_t *qp);


/* ── helper: visible length of an MCI string ─────────────────────────── */

/**
 * @brief Compute the visible (display column) width of a string that
 *        contains MCI pipe codes and/or semantic theme codes.
 *
 * Temporarily forces MCI_PARSE_ALL so that theme codes (|br, |hi, etc.)
 * are expanded to numeric |## codes, then strips all non-visible
 * sequences and returns strlen() of the result.
 *
 * @param mci_str  String potentially containing MCI codes.
 * @return         Visible column width after MCI expansion/stripping.
 */
static int near qpop_visible_len(const char *mci_str)
{
  char expanded[512], stripped[512];

  if (!mci_str || !*mci_str)
    return 0;

  /* Push full parse flags so theme codes get resolved */
  MciPushParseFlags(MCI_PARSE_ALL, MCI_PARSE_ALL);
  MciExpand(mci_str, expanded, sizeof(expanded));
  MciPopParseFlags();

  /* Strip all color/info/format sequences — leaves only visible chars */
  MciStrip(expanded, stripped, sizeof(stripped),
           MCI_STRIP_COLORS | MCI_STRIP_INFO | MCI_STRIP_FORMAT);

  return (int)strlen(stripped);
}


/* ═══════════════════════════════════════════════════════════════════════ */
/*  Quote_Popup — main entry point (modal)                                */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Open the Mystic-style popup quote window.
 *
 * Loads the original message, presents a scrollable popup overlaid on
 * the editor using ui_scrolling_region_t for efficient delta painting,
 * and lets the user insert quoted lines with Enter.
 * On exit the caller should call Fix_MagnEt() to repaint the editor.
 *
 * @param pr  Reply context (NULL if not a reply).
 * @return    Number of lines inserted, or 0.
 */
int Quote_Popup(struct _replyp *pr)
{
  quote_popup_t qp;
  ui_scrolling_region_style_t style;
  int inserted;
  int total_height;

  /* ── not a reply? ──────────────────────────────────────────────────── */

  if (pr == NULL)
  {
    Goto(usrlen, 1);
    Puts(max_not_reply);
    Goto(cursor_x, cursor_y);
    vbuf_flush();

    while (!Mdm_keyp())
      Giveaway_Slice();

    Mdm_getcwcc();  /* eat the key */
    return 0;
  }

  /* ── initialise struct ─────────────────────────────────────────────── */

  memset(&qp, 0, sizeof(qp));
  qp.attributed = FALSE;

  /* ── resolve theme-aware attributes for shadow buffer content ──────── */

  qp.hilite_attr = Mci2Attr("|lb|lf", 0x07);   /* lightbar bg+fg → attr byte */
  qp.qt_attr     = Mci2Attr("|qt", 0x07);       /* quote prefix color         */
  qp.tx_attr     = Mci2Attr("|tx", 0x07);       /* body text color            */
  qp.scroll_attr = Mci2Attr("|br", 0x07);       /* scrollbar uses border slot */

  /* ── compute left border visible width from lang string ────────────── */

  {
    const char *left_str = maxlang_get(g_current_lang,
                                       "editor.quote_window_left");
    qp.left_width = qpop_visible_len(left_str);
    if (qp.left_width < 1)
      qp.left_width = 1;  /* safety: at least 1 col for border */
  }

  /* ── preload the original message into raw lines ───────────────────── */

  if (!Quote_PreloadMessage(pr, &qp))
    return 0;

  /* ── compute popup geometry ────────────────────────────────────────── */

  qp.content_height = TermLength() - 5;
  if (qp.content_height > 8)
    qp.content_height = 8;
  if (qp.content_height < 3)
    qp.content_height = 3;

  total_height = qp.content_height + 2;  /* top border + content + status/bottom */
  qp.top_row = TermLength() - total_height;  /* bottom at TermLength()-1 */

  qp.total_width = usrwidth;
  qp.content_width = usrwidth - qp.left_width - 1;  /* inside left border, -1 right margin */
  if (qp.content_width < 20)
    qp.content_width = 20;

  qp.highlight = 0;

  /* ── init scrolling region inside the border ───────────────────────── */

  ui_scrolling_region_style_default(&style);
  style.attr = qp.tx_attr;
  style.scrollbar_attr = qp.scroll_attr;
  style.flags = UI_SCROLL_REGION_SHOW_SCROLLBAR;  /* no AUTO_FOLLOW */

  ui_scrolling_region_init(&qp.scroll,
                           1 + qp.left_width,       /* x: col after left border */
                           qp.top_row + 1,           /* y: row after top border  */
                           qp.content_width,          /* text width               */
                           qp.content_height,         /* viewport height          */
                           QW_MAX_LINES,
                           &style);

  /* ── append AVATAR-formatted lines to the scrolling region ─────────── */

  {
    int i;
    for (i = 0; i < qp.line_count; i++)
    {
      const char *raw = qp.lines[i];
      char buf[MAX_LINELEN + 32];
      const char *gt;

      /* Check if line has a quote prefix (contains "> " near the start) */
      gt = strchr(raw, '>');

      if (gt && gt < raw + 6 && gt[1] == ' ')
      {
        int plen = (int)(gt - raw) + 2;  /* include "> " */

        /* Quote prefix color, then body text color (resolved theme attrs) */
        snprintf(buf, sizeof(buf), "\x16\x01%c%.*s\x16\x01%c%s",
                 (char)qp.qt_attr, plen, raw,
                 (char)qp.tx_attr, raw + plen);
      }
      else
      {
        /* No prefix — body text color */
        snprintf(buf, sizeof(buf), "\x16\x01%c%s",
                 (char)qp.tx_attr, raw);
      }

      ui_scrolling_region_append(&qp.scroll, buf, UI_SCROLL_APPEND_NOFOLLOW);
    }
  }

  /* Force view_top to 0 after loading all lines */
  qp.scroll.view_top = 0;

  /* ── draw frame and initial content ────────────────────────────────── */

  Quote_RenderFrame(&qp);
  Quote_RenderContent(&qp);
  vbuf_flush();

  inserted = Quote_KeyLoop(&qp);

  /* ── cleanup ───────────────────────────────────────────────────────── */

  ui_scrolling_region_free(&qp.scroll);
  Quote_FreeLines(&qp);

  return inserted;
}


/* ═══════════════════════════════════════════════════════════════════════ */
/*  Quote_PreloadMessage — read all lines from original message           */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Open the original message and load all raw lines into qp->lines.
 *
 * Each line is stored as a malloc'd string with the " xx> " prefix
 * already applied where appropriate (via QuoteThisLine()).
 * These raw lines are used for editor insertion; AVATAR-formatted
 * versions are appended to the scrolling region separately.
 *
 * @return TRUE on success, FALSE on failure.
 */
static int near Quote_PreloadMessage(struct _replyp *pr, quote_popup_t *qp)
{
  HMSG  msgh;
  XMSG  msg;
  byte  *ol[4];
  byte  lt[4];
  byte  lma;
  word  got, i;
  int   wid, swid;
  char  prefix[MAX_INITIALS + 4];  /* " xx> " */
  int   plen;

  /* Open the original message */

  msgh = MsgOpenMsg(pr->fromsq, MOPEN_READ,
                    MsgUidToMsgn(pr->fromsq, pr->original, UID_EXACT));
  if (msgh == NULL)
    return FALSE;

  /* Read header to get sender's name */

  if (MsgReadMsg(msgh, &msg, 0L, 0L, NULL, 0L, NULL) == (dword)-1)
  {
    MsgCloseMsg(msgh);
    return FALSE;
  }

  Parse_Initials(msg.from, qp->initials);

  /* Capture sender/recipient/date for attribution line */

  strncpy(qp->sender, (const char *)msg.from, sizeof(qp->sender) - 1);
  qp->sender[sizeof(qp->sender) - 1] = '\0';

  strncpy(qp->recipient, (const char *)msg.to, sizeof(qp->recipient) - 1);
  qp->recipient[sizeof(qp->recipient) - 1] = '\0';

  MsgDte((union stamp_combo *)&msg.date_written, qp->orig_date);

  /* Build the quote prefix string: " XX> " */

  snprintf(prefix, sizeof(prefix), " %s> ", qp->initials);
  plen = (int)strlen(prefix);

  /* Compute line widths (matching med_quot.c pattern) */

  wid = usrwidth - plen - HARD_SAFE;
  if (wid < 20) wid = 20;

  swid = usrwidth - plen - SOFT_SAFE;
  if (swid < 20) swid = 20;

  /* Allocate the line pointer array */

  qp->lines = (char **)malloc(QW_MAX_LINES * sizeof(char *));
  if (qp->lines == NULL)
  {
    MsgCloseMsg(msgh);
    return FALSE;
  }

  /* Allocate temp buffers for Msg_Read_Lines (needs byte* array) */

  for (i = 0; i < 4; i++)
  {
    ol[i] = (byte *)malloc(MAX_LINELEN + 2);
    if (ol[i] == NULL)
    {
      word j;
      for (j = 0; j < i; j++) free(ol[j]);
      free(qp->lines);
      qp->lines = NULL;
      MsgCloseMsg(msgh);
      return FALSE;
    }
  }

  qp->line_count = 0;
  lma = 0;

  /* Read lines in batches of 4 (matching existing pattern) */

  for (;;)
  {
    got = Msg_Read_Lines(msgh, 4, wid, swid, ol, lt, &lma, MRL_QEXP);

    for (i = 0; i < got && qp->line_count < QW_MAX_LINES; i++)
    {
      char *line;
      int  need_prefix;

      /* Skip kludge/seen-by lines the user can't see */

      if ((lt[i] & MSGLINE_SEENBY) &&
          !GEPriv(usr.priv, ngcfg_get_int("matrix.seenby_priv")))
        continue;

      if ((lt[i] & MSGLINE_KLUDGE) &&
          !GEPriv(usr.priv, ngcfg_get_int("matrix.ctla_priv")))
        continue;

      /* End-of-message sentinel */

      if (lt[i] & MSGLINE_END)
        break;

      /* Determine if this line needs a quote prefix */

      need_prefix = QuoteThisLine(ol[i]);

      /* Allocate and format the display line */

      line = (char *)malloc(MAX_LINELEN + plen + 4);
      if (line == NULL)
        break;

      if (need_prefix)
        snprintf(line, MAX_LINELEN + plen + 4, "%s%s", prefix, ol[i]);
      else
        snprintf(line, MAX_LINELEN + plen + 4, "%s", ol[i]);

      qp->lines[qp->line_count++] = line;
    }

    /* Stop if we hit end-of-message or got fewer lines than requested */

    if (got < 4 || qp->line_count >= QW_MAX_LINES)
      break;

    /* Also stop if any line was MSGLINE_END */

    {
      int found_end = FALSE;
      for (i = 0; i < got; i++)
      {
        if (lt[i] & MSGLINE_END)
        {
          found_end = TRUE;
          break;
        }
      }
      if (found_end)
        break;
    }
  }

  /* Cleanup */

  for (i = 0; i < 4; i++)
    free(ol[i]);

  MsgCloseMsg(msgh);

  /* Handle empty message */

  if (qp->line_count == 0)
  {
    free(qp->lines);
    qp->lines = NULL;
    return FALSE;
  }

  return TRUE;
}


/* ═══════════════════════════════════════════════════════════════════════ */
/*  Quote_FreeLines — release all preloaded raw text lines                */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Release all preloaded raw text lines and reset the line count.
 *
 * @param qp  Popup state whose lines[] array will be freed.
 */
static void near Quote_FreeLines(quote_popup_t *qp)
{
  if (qp->lines)
  {
    int i;
    for (i = 0; i < qp->line_count; i++)
    {
      if (qp->lines[i])
        free(qp->lines[i]);
    }
    free(qp->lines);
    qp->lines = NULL;
  }
  qp->line_count = 0;
}


/* ═══════════════════════════════════════════════════════════════════════ */
/*  qpop_compute_pad — measure base visible width, return fill count      */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Compute the $D padding count for a lang string with |!1 repeat slot.
 *
 * Expands the template with "00" as |!1 (producing $D00\xC4 → 0 repeats),
 * measures the resulting visible base width, and returns the fill count
 * needed to reach @p total_width.  Clamped to [0, 99] since $D takes a
 * two-digit repeat count.
 *
 * @param tmpl        Resolved lang string (from maxlang_get).
 * @param total_width Target total visible width.
 * @return Padding repeat count (clamped to [0, 99]).
 */
static int near qpop_compute_pad(const char *tmpl, int total_width)
{
  char expanded[512];
  int base_vis, pad;

  if (!tmpl || !*tmpl)
    return 0;

  /* Expand |!1 with "00" — $D|!1\xC4 becomes $D00\xC4 → 0 repeats */
  LangSprintf(expanded, sizeof(expanded), tmpl, "00");

  /* Measure visible width after full MCI expansion + stripping */
  base_vis = qpop_visible_len(expanded);

  pad = total_width - base_vis;
  if (pad < 0)  pad = 0;
  if (pad > 99) pad = 99;  /* $D takes 2-digit count */
  return pad;
}


/* ═══════════════════════════════════════════════════════════════════════ */
/*  Quote_RenderFrame — draw border + title + status bar (one-shot)       */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Draw the popup border, title, and status/bottom bar.
 *
 * Called once at popup open and again after each live update.
 * Padding is handled entirely by $D|!1\xC4 in the lang strings —
 * the C code computes the repeat count and passes it as |!1 via
 * LangPrintfForce().
 *
 * The border/title/status colors come from semantic MCI codes in the
 * lang strings (|br, |hi, |hk, |ac), resolved through the theme system.
 * Closing corner characters (\xBF ┐, \xD9 ┘) are embedded in the
 * lang strings after the $D repeat, not emitted by C code.
 */
static void near Quote_RenderFrame(const quote_popup_t *qp)
{
  int row, i;
  int pad_top, pad_bot;
  char pad_str[16];

  const char *top_str;
  const char *left_str;
  const char *bot_str;

  /* ── fetch lang strings ────────────────────────────────────────────── */

  top_str = maxlang_get(g_current_lang, "editor.quote_window_top");
  left_str = maxlang_get(g_current_lang, "editor.quote_window_left");
  bot_str = maxlang_get(g_current_lang, "editor.quote_window_bottom");

  /* ── compute $D repeat counts for padding ──────────────────────────── */

  pad_top = qpop_compute_pad(top_str, qp->total_width);
  pad_bot = qpop_compute_pad(bot_str, qp->total_width);

  /* ── top border ────────────────────────────────────────────────────── */

  row = qp->top_row;
  Goto(row, 1);
  snprintf(pad_str, sizeof(pad_str), "%02d", pad_top);
  LangPrintfForce((char *)top_str, pad_str);

  /* ── left border for content rows ──────────────────────────────────── */
  /* The scrolling region paints the content area + scrollbar column.     */
  /* We only need the left border string at column 1 for each row.       */

  for (i = 0; i < qp->content_height; i++)
  {
    Goto(row + 1 + i, 1);
    PutsForce(left_str);
  }

  /* ── status/bottom bar ─────────────────────────────────────────────── */

  Goto(row + 1 + qp->content_height, 1);
  snprintf(pad_str, sizeof(pad_str), "%02d", pad_bot);
  LangPrintfForce((char *)bot_str, pad_str);

  /* Reset to normal text color */
  PutsForce("|cd");
  EMIT_MSG_TEXT_COL();
}


/* ═══════════════════════════════════════════════════════════════════════ */
/*  Quote_RenderContent — render scrolling region + highlight bar         */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Render the content area via the scrolling region, then apply
 *        the highlight bar as an attribute override.
 *
 * 1. ui_scrolling_region_render() rebuilds the shadow buffer from line
 *    data and paints the full viewport (fixing partial-scroll bug).
 * 2. The highlighted row's cell attributes are overwritten to hilite_attr.
 * 3. Only the highlight row is re-painted (single-row delta).
 */
static void near Quote_RenderContent(quote_popup_t *qp)
{
  int highlight_vis;

  /* Full render + paint of content area */
  ui_scrolling_region_render(&qp->scroll);

  /* Apply highlight bar attribute override (theme-resolved) */
  highlight_vis = qp->highlight - qp->scroll.view_top;

  if (highlight_vis >= 0 && highlight_vis < qp->scroll.height)
  {
    int col;
    int sb_row = highlight_vis + 1;  /* shadow buffer rows are 1-based */

    for (col = 1; col <= qp->scroll.sb.width; col++)
    {
      int idx = (sb_row - 1) * qp->scroll.sb.width + (col - 1);
      qp->scroll.sb.cells[idx].attr = qp->hilite_attr;
    }

    /* Repaint just the highlight row */
    ui_shadowbuf_paint_region(&qp->scroll.sb,
                              qp->scroll.x, qp->scroll.y,
                              1, sb_row,
                              qp->scroll.sb.width, sb_row);
  }
}


/* ═══════════════════════════════════════════════════════════════════════ */
/*  Quote_InsertLine — insert a quoted line into editor's screen[]        */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Insert a single quoted line at the current cursor position.
 *
 * Follows the exact pattern from the old Quote_Copy() in med_quot.c:
 * allocate a line, shift lines down, copy text, increment num_lines.
 *
 * @param qp   Popup state (unused — lines are pre-prefixed).
 * @param line The pre-formatted line text to insert.
 * @return     TRUE on success, FALSE on overflow.
 */
static int near Quote_InsertLine(const quote_popup_t *qp, const char *line)
{
  word x;
  byte *p, *n;

  (void)qp;  /* lines are already prefixed */

  if (num_lines >= max_lines - 1)
    return FALSE;

  /* Allocate a new line at the end */

  if (Allocate_Line(num_lines + 1))
  {
    EdMemOvfl();
    return FALSE;
  }

  /* Shift everything down to make room at the cursor position */

  p = screen[num_lines];

  for (x = num_lines - 1; x >= offset + cursor_x; x--)
  {
    screen[x + 1] = screen[x];
    update_table[x + 1] = TRUE;
  }

  /* Place the newly-allocated line at the cursor position */

  screen[offset + cursor_x] = n = p;

  /* Hard-CR delimited */

  *n++ = HARD_CR;

  /* Copy the quoted text */

  strncpy(n, line, MAX_LINELEN - 2);
  n[MAX_LINELEN - 2] = '\0';

  update_table[offset + cursor_x] = TRUE;
  cursor_x++;

  return TRUE;
}


/* ═══════════════════════════════════════════════════════════════════════ */
/*  Quote_EnsureVisible — adjust view_top so highlight is visible         */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Clamp highlight and adjust the scrolling region's view_top
 *        so the highlighted line is within the visible viewport.
 */
static void near Quote_EnsureVisible(quote_popup_t *qp)
{
  /* Clamp highlight */

  if (qp->highlight < 0)
    qp->highlight = 0;
  if (qp->highlight >= qp->line_count)
    qp->highlight = qp->line_count - 1;

  /* Adjust the scroll region's viewport to track the highlight */

  if (qp->highlight < qp->scroll.view_top)
    qp->scroll.view_top = qp->highlight;
  else if (qp->highlight >= qp->scroll.view_top + qp->content_height)
    qp->scroll.view_top = qp->highlight - qp->content_height + 1;

  if (qp->scroll.view_top < 0)
    qp->scroll.view_top = 0;
}


/* ═══════════════════════════════════════════════════════════════════════ */
/*  Quote_LiveUpdate — repaint editor text + popup after a line insert     */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Repaint the editor text area and then restore the popup overlay.
 *
 * After Quote_InsertLine() modifies screen[]/num_lines, the editor rows
 * above the popup are stale. This helper:
 *   1. Marks all editor lines dirty via Redraw_Text()
 *   2. Runs Do_Update() to actually repaint them (may bleed into popup area)
 *   3. Repaints the MagnEt status bar
 *   4. Redraws the static popup frame (covers any editor bleed)
 *   5. Invalidates the scroll region shadow buffer so it fully repaints
 *   6. Renders popup content with highlight bar
 *
 * The result is a flicker-free live update: the user sees each inserted
 * line appear in the editor while the popup stays visible.
 */
static void near Quote_LiveUpdate(quote_popup_t *qp)
{
  /* Mark all editor lines dirty and repaint them */
  Redraw_Text();
  Do_Update();

  /* Refresh the editor status bar (row usrlen) */
  Redraw_StatusLine();

  /* Redraw the static popup frame (borders + title + help bar) */
  Quote_RenderFrame(qp);

  /* Invalidate shadow buffer so content area fully repaints */
  qp->scroll.sb_valid = 0;

  /* Repaint popup content with highlight overlay */
  Quote_RenderContent(qp);
}


/* ═══════════════════════════════════════════════════════════════════════ */
/*  Quote_KeyLoop — modal key loop for the popup                          */
/* ═══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Run the modal key loop for the quote popup.
 *
 * Manages highlight and view_top manually. After each change,
 * calls Quote_RenderContent() which uses the scrolling region
 * for efficient full-viewport rendering with highlight overlay.
 *
 * @param qp  Popup state.
 * @return    Number of lines inserted.
 */
static int near Quote_KeyLoop(quote_popup_t *qp)
{
  int ch;
  int inserted = 0;
  int old_highlight;
  int old_view_top;
  int running = TRUE;

  while (running)
  {
    ch = Mdm_getcwcc();
    old_highlight = qp->highlight;
    old_view_top = qp->scroll.view_top;

    switch (ch)
    {
      case K_CTRLE:           /* ^E = up */
        qp->highlight--;
        break;

      case K_CTRLX:           /* ^X = down */
        qp->highlight++;
        break;

      case K_CTRLR:           /* ^R = page up */
        qp->highlight -= qp->content_height;
        break;

      case K_CTRLC:           /* ^C = page down */
        qp->highlight += qp->content_height;
        break;

      case K_CR:              /* Enter = accept line */
        if (qp->highlight >= 0 && qp->highlight < qp->line_count)
        {
          /* Insert attribution line on first quote of this session */
          if (!qp->attributed)
          {
            char attr_line[256];
            const char *attr_fmt = maxlang_get(g_current_lang,
                                               "editor.quote_attribution");

            if (attr_fmt && *attr_fmt)
            {
              LangSprintf(attr_line, sizeof(attr_line), attr_fmt,
                          qp->sender, qp->recipient, qp->orig_date);

              Quote_InsertLine(qp, attr_line);
            }

            qp->attributed = TRUE;
          }

          if (Quote_InsertLine(qp, qp->lines[qp->highlight]))
          {
            inserted++;
            qp->highlight++;

            /* Auto-scroll editor to keep cursor visible above the popup */
            {
              int vis_rows = qp->top_row - 2;  /* visible editor rows above popup */
              if (vis_rows < 1)
                vis_rows = 1;

              if (cursor_x >= (word)vis_rows)
              {
                int excess = cursor_x - vis_rows + 1;
                offset += excess;
                cursor_x -= excess;
              }
            }

            /* Live-repaint editor area above popup so user sees the insert */
            Quote_LiveUpdate(qp);
            vbuf_flush();
          }
        }
        break;

      case K_CTRLQ:           /* ^Q = close popup */
      case K_ESC:
        /* ESC could be literal ESC or start of ANSI sequence.
         * Check for ANSI: ESC [ ... */
        if (ch == K_ESC)
        {
          /* Peek: if next char is '[' or 'O', it's ANSI */
          int ch2 = Mdm_getcwcc();

          if (ch2 == '[' || ch2 == 'O')
          {
            /* Parse ANSI sequence */
            int ch3 = Mdm_getcwcc();

            switch (ch3)
            {
              case 'A':       /* Up arrow */
                qp->highlight--;
                break;

              case 'B':       /* Down arrow */
                qp->highlight++;
                break;

              case '5':       /* PgUp: ESC[5~ */
                Mdm_getcwcc();  /* eat the '~' */
                qp->highlight -= qp->content_height;
                break;

              case '6':       /* PgDn: ESC[6~ */
                Mdm_getcwcc();  /* eat the '~' */
                qp->highlight += qp->content_height;
                break;

              case '1':       /* Home: ESC[1~ */
                Mdm_getcwcc();  /* eat the '~' */
                qp->highlight = 0;
                break;

              case '4':       /* End: ESC[4~ */
                Mdm_getcwcc();  /* eat the '~' */
                qp->highlight = qp->line_count - 1;
                break;

              case 'H':       /* Home (alt) */
                qp->highlight = 0;
                break;

              case 'F':       /* End (alt) */
                qp->highlight = qp->line_count - 1;
                break;

              default:
                break;  /* Unknown ANSI sequence — ignore */
            }
          }
          else if (ch2 == K_ESC)
          {
            /* Double-ESC = exit */
            running = FALSE;
          }
          else
          {
            /* Bare ESC followed by non-ANSI — treat as exit */
            running = FALSE;
          }
        }
        else
        {
          /* ^Q = exit */
          running = FALSE;
        }
        break;

      case 0:                 /* Scan code (local console) */
      {
        int sc = Mdm_getcwcc();

        switch (sc)
        {
          case 72:            /* Up arrow */
            qp->highlight--;
            break;

          case 80:            /* Down arrow */
            qp->highlight++;
            break;

          case 73:            /* PgUp */
            qp->highlight -= qp->content_height;
            break;

          case 81:            /* PgDn */
            qp->highlight += qp->content_height;
            break;

          case 71:            /* Home */
            qp->highlight = 0;
            break;

          case 79:            /* End */
            qp->highlight = qp->line_count - 1;
            break;

          default:
            break;
        }
        break;
      }

      default:
        break;  /* Ignore unrecognised keys */
    }

    /* Clamp highlight and adjust viewport */

    Quote_EnsureVisible(qp);

    /* Repaint content if anything changed */

    if (qp->highlight != old_highlight || qp->scroll.view_top != old_view_top)
    {
      Quote_RenderContent(qp);
      vbuf_flush();
    }
  }

  return inserted;
}
