/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * lang_browse.c - TOML language file string browser for maxcfg
 *
 * Parses a TOML language file as text to extract section headers and
 * key=value pairs, then presents them in a filterable list picker.
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include "maxcfg.h"
#include "ui.h"
#include "fields.h"
#include "menu_preview.h"
#include "mci_preview.h"
#include "lang_browse.h"
#include "mci_helper.h"

/* ========================================================================== */
/* Constants                                                                   */
/* ========================================================================== */

#define LB_MAX_ENTRIES  4096    /**< Maximum string entries to display */
#define LB_MAX_KEY      256     /**< Maximum dotted key length */
#define LB_MAX_VAL      512     /**< Maximum displayed value length */
#define LB_MAX_LINE     2048    /**< Maximum line length in TOML file */

/* ========================================================================== */
/* Internal structures                                                         */
/* ========================================================================== */

/** A parsed language string entry */
typedef struct {
    char *dotted_key;   /**< "heap.symbol" */
    char *heap;         /**< Heap name (portion before dot) */
    char *symbol;       /**< Symbol name (portion after dot) */
    char *value;        /**< Display-truncated string value */
    char *full_value;   /**< Untruncated string value for editing */
    char *flags_str;    /**< Flags string (e.g. "mex") or empty */
    char *raw_line;     /**< Original TOML line for reconstruction */
    int line_num;       /**< 1-based line number in the TOML file */
    bool is_inline;     /**< true if { text = "..." } format */
} LbEntry;

/* ========================================================================== */
/* Helpers                                                                     */
/* ========================================================================== */

/** Strip leading/trailing whitespace in-place, return pointer into buf */
static char *lb_strip(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return s;
}

/** Case-insensitive substring search */
static bool lb_contains_ci(const char *haystack, const char *needle)
{
    if (!needle || !needle[0]) return true;
    if (!haystack) return false;
    size_t nlen = strlen(needle);
    for (const char *p = haystack; *p; p++) {
        if (strncasecmp(p, needle, nlen) == 0)
            return true;
    }
    return false;
}

/**
 * @brief Extract the text value from a TOML value string.
 *
 * Handles:
 *   - Simple strings: "hello world"
 *   - Inline tables: { text = "hello", ... }
 *   - Multi-line strings (first line only)
 */
static char *lb_extract_value(const char *raw)
{
    const char *p = raw;
    while (*p && isspace((unsigned char)*p)) p++;

    /* Inline table: { text = "..." ... } */
    if (*p == '{') {
        const char *txt = strstr(p, "text");
        if (!txt) txt = strstr(p, "TEXT");
        if (txt) {
            txt = strchr(txt, '=');
            if (txt) {
                txt++;
                while (*txt && isspace((unsigned char)*txt)) txt++;
                if (*txt == '"') {
                    txt++;
                    const char *end = strchr(txt, '"');
                    if (end) {
                        size_t len = (size_t)(end - txt);
                        if (len > LB_MAX_VAL - 1) len = LB_MAX_VAL - 1;
                        char *out = malloc(len + 1);
                        if (out) {
                            memcpy(out, txt, len);
                            out[len] = '\0';
                        }
                        return out;
                    }
                }
            }
        }
        return strdup("(inline table)");
    }

    /* Simple quoted string */
    if (*p == '"') {
        p++;
        const char *end = strchr(p, '"');
        if (end) {
            size_t len = (size_t)(end - p);
            if (len > LB_MAX_VAL - 1) len = LB_MAX_VAL - 1;
            char *out = malloc(len + 1);
            if (out) {
                memcpy(out, p, len);
                out[len] = '\0';
            }
            return out;
        }
    }

    /* Multi-line string """...""" — just take the marker */
    if (strncmp(p, "\"\"\"", 3) == 0) {
        return strdup("(multi-line string)");
    }

    /* Fallback: return raw trimmed */
    return strdup(p);
}

/**
 * @brief Extract full (untruncated) text value and detect inline table format.
 *
 * Same logic as lb_extract_value but without the LB_MAX_VAL clamp.
 * Sets *out_inline to true if the value uses { text = "..." } syntax.
 */
static char *lb_extract_full_value(const char *raw, bool *out_inline)
{
    const char *p = raw;
    while (*p && isspace((unsigned char)*p)) p++;

    if (*p == '{') {
        if (out_inline) *out_inline = true;
        const char *txt = strstr(p, "text");
        if (!txt) txt = strstr(p, "TEXT");
        if (txt) {
            txt = strchr(txt, '=');
            if (txt) {
                txt++;
                while (*txt && isspace((unsigned char)*txt)) txt++;
                if (*txt == '"') {
                    txt++;
                    /* Find closing quote, handling escaped quotes */
                    const char *end = txt;
                    while (*end && !(*end == '"' && *(end - 1) != '\\'))
                        end++;
                    if (*end == '"') {
                        size_t len = (size_t)(end - txt);
                        char *out = malloc(len + 1);
                        if (out) {
                            memcpy(out, txt, len);
                            out[len] = '\0';
                        }
                        return out;
                    }
                }
            }
        }
        return strdup("");
    }

    if (out_inline) *out_inline = false;

    if (*p == '"') {
        p++;
        const char *end = p;
        while (*end && !(*end == '"' && *(end - 1) != '\\'))
            end++;
        if (*end == '"') {
            size_t len = (size_t)(end - p);
            char *out = malloc(len + 1);
            if (out) {
                memcpy(out, p, len);
                out[len] = '\0';
            }
            return out;
        }
    }

    if (strncmp(p, "\"\"\"", 3) == 0)
        return strdup("(multi-line string)");

    return strdup(p);
}

/**
 * @brief Extract flags string from a raw TOML value.
 *
 * Looks for flags = ["..."] in inline table format and returns the
 * comma-joined flag values (e.g. "mex" or "mex,rip").
 * Returns empty string if no flags found.
 */
static char *lb_extract_flags(const char *raw)
{
    const char *p = raw;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '{') return strdup("");

    const char *fl = strstr(p, "flags");
    if (!fl) fl = strstr(p, "FLAGS");
    if (!fl) return strdup("");

    fl = strchr(fl, '[');
    if (!fl) return strdup("");
    fl++;

    const char *end = strchr(fl, ']');
    if (!end) return strdup("");

    /* Extract flag values from ["x", "y"] */
    char buf[128] = "";
    int bpos = 0;
    const char *c = fl;
    while (c < end && bpos < (int)sizeof(buf) - 2) {
        if (*c == '"') {
            c++;
            while (c < end && *c != '"' && bpos < (int)sizeof(buf) - 2)
                buf[bpos++] = *c++;
            if (c < end && *c == '"') c++;
            /* Add comma separator if more flags follow */
            const char *next_quote = strchr(c, '"');
            if (next_quote && next_quote < end)
                buf[bpos++] = ',';
        } else {
            c++;
        }
    }
    buf[bpos] = '\0';
    return strdup(buf);
}

/* ========================================================================== */
/* Core parser                                                                 */
/* ========================================================================== */

/**
 * @brief Parse a TOML language file into an array of LbEntry.
 *
 * Walks the file line-by-line, tracking section headers to build dotted
 * keys, and extracts simple key = "value" pairs.  Skips [metadata] and
 * [_legacy_map] sections.
 *
 * @param path       Path to the .toml file.
 * @param out_entries  Receives malloc'd array of LbEntry.
 * @param out_count    Receives the entry count.
 * @return true on success.
 */
static bool lb_parse_toml(const char *path, LbEntry **out_entries, int *out_count)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return false;

    LbEntry *entries = calloc(LB_MAX_ENTRIES, sizeof(LbEntry));
    if (!entries) { fclose(fp); return false; }

    int count = 0;
    int line_num = 0;
    char section[LB_MAX_KEY] = "";
    bool skip_section = false;
    char line[LB_MAX_LINE];
    char raw_copy[LB_MAX_LINE];

    while (fgets(line, sizeof(line), fp) && count < LB_MAX_ENTRIES) {
        line_num++;

        /* Save raw line before lb_strip modifies it */
        strncpy(raw_copy, line, sizeof(raw_copy) - 1);
        raw_copy[sizeof(raw_copy) - 1] = '\0';
        /* Strip trailing newline from raw copy */
        size_t rlen = strlen(raw_copy);
        if (rlen > 0 && raw_copy[rlen - 1] == '\n') raw_copy[--rlen] = '\0';
        if (rlen > 0 && raw_copy[rlen - 1] == '\r') raw_copy[--rlen] = '\0';

        char *s = lb_strip(line);

        /* Skip empty lines and comments */
        if (!*s || *s == '#') continue;

        /* Section header: [name] */
        if (*s == '[') {
            /* Skip array-of-tables [[...]] */
            if (s[1] == '[') continue;

            char *end = strchr(s, ']');
            if (end) {
                *end = '\0';
                snprintf(section, sizeof(section), "%s", s + 1);

                /* Skip internal sections */
                skip_section = (strcmp(section, "metadata") == 0 ||
                                strcmp(section, "_legacy_map") == 0);
            }
            continue;
        }

        if (skip_section || !section[0]) continue;

        /* Key = value */
        char *eq = strchr(s, '=');
        if (!eq) continue;

        /* Extract key (strip whitespace and quotes) */
        *eq = '\0';
        char *key = lb_strip(s);
        char *val_raw = lb_strip(eq + 1);

        /* Skip empty keys */
        if (!*key) continue;

        /* Build dotted key */
        char dotted[LB_MAX_KEY];
        snprintf(dotted, sizeof(dotted), "%s.%s", section, key);

        /* Extract display value (truncated) */
        char *val = lb_extract_value(val_raw);

        /* Extract full value and inline table flag for editing */
        bool is_inline = false;
        char *full = lb_extract_full_value(val_raw, &is_inline);

        entries[count].dotted_key = strdup(dotted);
        entries[count].heap = strdup(section);
        entries[count].symbol = strdup(key);
        entries[count].value = val ? val : strdup("");
        entries[count].full_value = full ? full : strdup("");
        entries[count].flags_str = lb_extract_flags(val_raw);
        entries[count].raw_line = strdup(raw_copy);
        entries[count].line_num = line_num;
        entries[count].is_inline = is_inline;
        count++;
    }

    fclose(fp);
    *out_entries = entries;
    *out_count = count;
    return true;
}

static void lb_free_entries(LbEntry *entries, int count)
{
    for (int i = 0; i < count; i++) {
        free(entries[i].dotted_key);
        free(entries[i].heap);
        free(entries[i].symbol);
        free(entries[i].value);
        free(entries[i].full_value);
        free(entries[i].flags_str);
        free(entries[i].raw_line);
    }
    free(entries);
}

/* ========================================================================== */
/* Write-back                                                                  */
/* ========================================================================== */

/**
 * @brief Replace a single line in a TOML file with a new value.
 *
 * For inline table entries ({ text = "...", ... }), only the text portion
 * is replaced while preserving flags, rip, and other fields.
 * For simple string entries ("..."), the entire value is replaced.
 *
 * @param path       Path to the TOML file.
 * @param line_num   1-based line number to replace.
 * @param key_bare   Bare key name (without heap prefix).
 * @param new_text   New string value (unquoted).
 * @param is_inline  Whether the original was { text = "..." } format.
 * @param raw_line   Original raw line for inline table reconstruction.
 * @return true on success.
 */
static bool lb_write_back(const char *path, int line_num, const char *key_bare,
                           const char *new_text, bool is_inline, const char *raw_line)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return false;

    /* Read all lines into memory */
    char **lines = NULL;
    int total = 0, cap = 0;
    char buf[LB_MAX_LINE];

    while (fgets(buf, sizeof(buf), fp)) {
        size_t ln = strlen(buf);
        if (ln > 0 && buf[ln - 1] == '\n') buf[--ln] = '\0';
        if (ln > 0 && buf[ln - 1] == '\r') buf[--ln] = '\0';

        if (total >= cap) {
            cap = cap ? cap * 2 : 512;
            lines = realloc(lines, (size_t)cap * sizeof(char *));
        }
        lines[total++] = strdup(buf);
    }
    fclose(fp);

    if (line_num < 1 || line_num > total) {
        for (int i = 0; i < total; i++) free(lines[i]);
        free(lines);
        return false;
    }

    /* Build replacement line */
    char new_line[LB_MAX_LINE];

    if (is_inline) {
        /* Inline table: replace just the text = "..." portion.
         * Find 'text = "' in the raw line, replace the quoted value,
         * keep everything else (flags, rip, etc). */
        const char *text_start = strstr(raw_line, "text");
        if (!text_start) text_start = strstr(raw_line, "TEXT");

        if (text_start) {
            const char *eq = strchr(text_start, '=');
            if (eq) {
                eq++;
                while (*eq == ' ' || *eq == '\t') eq++;
                if (*eq == '"') {
                    /* Find closing quote */
                    const char *close = eq + 1;
                    while (*close && !(*close == '"' && *(close - 1) != '\\'))
                        close++;
                    if (*close == '"') {
                        /* Reconstruct: prefix + new_text + suffix */
                        size_t prefix_len = (size_t)(eq + 1 - raw_line);
                        snprintf(new_line, sizeof(new_line), "%.*s%s%s",
                                 (int)prefix_len, raw_line,
                                 new_text,
                                 close); /* includes closing " and rest */
                    } else {
                        /* Fallback: rebuild simple inline table */
                        snprintf(new_line, sizeof(new_line),
                                 "%s = { text = \"%s\" }", key_bare, new_text);
                    }
                } else {
                    snprintf(new_line, sizeof(new_line),
                             "%s = { text = \"%s\" }", key_bare, new_text);
                }
            } else {
                snprintf(new_line, sizeof(new_line),
                         "%s = { text = \"%s\" }", key_bare, new_text);
            }
        } else {
            snprintf(new_line, sizeof(new_line),
                     "%s = { text = \"%s\" }", key_bare, new_text);
        }
    } else {
        /* Simple string: key = "new_text" */
        snprintf(new_line, sizeof(new_line), "%s = \"%s\"", key_bare, new_text);
    }

    /* Replace the target line */
    free(lines[line_num - 1]);
    lines[line_num - 1] = strdup(new_line);

    /* Write all lines back */
    fp = fopen(path, "w");
    if (!fp) {
        for (int i = 0; i < total; i++) free(lines[i]);
        free(lines);
        return false;
    }

    for (int i = 0; i < total; i++)
        fprintf(fp, "%s\n", lines[i]);

    fclose(fp);

    for (int i = 0; i < total; i++) free(lines[i]);
    free(lines);
    return true;
}

/* ========================================================================== */
/* Heap list + filtering                                                       */
/* ========================================================================== */

static void lb_collect_heaps(LbEntry *entries, int count,
                             char ***out_heaps, int *out_hcount)
{
    char **heaps = calloc(64, sizeof(char *));
    int hc = 0;
    for (int i = 0; i < count && hc < 63; i++) {
        bool found = false;
        for (int j = 0; j < hc; j++)
            if (strcmp(heaps[j], entries[i].heap) == 0) { found = true; break; }
        if (!found) heaps[hc++] = strdup(entries[i].heap);
    }
    *out_heaps = heaps;
    *out_hcount = hc;
}

static void lb_free_heaps(char **heaps, int count)
{
    for (int i = 0; i < count; i++) free(heaps[i]);
    free(heaps);
}

static const char *flags_opts[] = { "All", "mex", "(none)" };
#define FLAGS_OPT_COUNT 3

static int *lb_build_vis(LbEntry *entries, int count, const char *filter,
                         const char *heap_filter, int flags_idx, int *out_vc)
{
    int *vis = malloc((size_t)count * sizeof(int));
    int vc = 0;
    for (int i = 0; i < count; i++) {
        if (heap_filter && heap_filter[0] &&
            strcmp(entries[i].heap, heap_filter) != 0) continue;
        if (flags_idx == 1 && (!entries[i].flags_str ||
            !strstr(entries[i].flags_str, "mex"))) continue;
        if (flags_idx == 2 && entries[i].flags_str &&
            entries[i].flags_str[0]) continue;
        if (filter && filter[0]) {
            if (!lb_contains_ci(entries[i].symbol, filter) &&
                !lb_contains_ci(entries[i].full_value, filter)) continue;
        }
        vis[vc++] = i;
    }
    *out_vc = vc;
    return vis;
}

/* ========================================================================== */
/* Preview — delegates to shared MCI interpreter (mci_preview.c)               */
/* ========================================================================== */

#define PREVIEW_W     80
#define PREVIEW_H_MIN  3   /**< Minimum preview rows */
#define PREVIEW_H_MAX 50   /**< Buffer ceiling for MCI expansion */

/**
 * @brief Count the minimum lines a string will occupy after MCI expansion.
 *
 * Scans for literal newlines and |CR codes to estimate the needed height.
 * The actual rendered height may differ (cursor codes, wrapping), so we
 * expand into a generous buffer and trim to the last used row.
 */
static int le_count_text_lines(const char *text)
{
    int lines = 1;
    for (const char *p = text; *p; p++) {
        if (*p == '\n') { lines++; continue; }
        /* |CR — carriage return + line feed */
        if (p[0] == '|' && p[1] == 'C' && p[2] == 'R') { lines++; p += 2; continue; }
        /* Backslash-n literal in TOML-escaped strings */
        if (p[0] == '\\' && p[1] == 'n') { lines++; p++; continue; }
    }
    return lines;
}

/**
 * @brief Show a bounded preview popup with full MCI/AVATAR rendering.
 *
 * Uses the shared mci_preview module for MCI expansion, then blits the
 * resulting virtual screen into an ncurses popup dialog.  The preview
 * height auto-sizes to fit all rendered content (minimum PREVIEW_H_MIN).
 */
static void le_preview(const char *text)
{
    /* Load mock data for info code / %t expansion */
    MciMockData mock;
    mci_mock_load(&mock);

    /* Estimate needed rows; clamp to buffer max */
    int est_rows = le_count_text_lines(text);
    if (est_rows < PREVIEW_H_MIN) est_rows = PREVIEW_H_MIN;
    int buf_rows = est_rows + 4; /* headroom for cursor movement / wrapping */
    if (buf_rows > PREVIEW_H_MAX) buf_rows = PREVIEW_H_MAX;

    /* Dynamically allocate virtual screen buffers */
    size_t cells = (size_t)buf_rows * PREVIEW_W;
    char    *vch_buf  = malloc(cells);
    uint8_t *va_buf   = calloc(cells, sizeof(uint8_t));
    if (!vch_buf || !va_buf) { free(vch_buf); free(va_buf); return; }

    MciVScreen vs = { .ch = vch_buf, .attr = va_buf,
                       .cols = PREVIEW_W, .rows = buf_rows };
    mci_vs_clear(&vs);

    /* Run the shared MCI interpreter */
    MciState st;
    mci_state_init(&st);
    mci_preview_expand(&vs, &st, &mock, text);

    /* Find the last row with non-space content */
    int used_rows = 1;
    for (int r = buf_rows - 1; r >= 0; r--) {
        for (int c = 0; c < PREVIEW_W; c++) {
            if (vch_buf[r * PREVIEW_W + c] != ' ' ||
                va_buf[r * PREVIEW_W + c] != 0x07) {
                used_rows = r + 1;
                goto found_last;
            }
        }
    }
found_last:
    /* Also include at least the cursor row (in case text ends on a blank line) */
    if (st.cy + 1 > used_rows) used_rows = st.cy + 1;
    if (used_rows < PREVIEW_H_MIN) used_rows = PREVIEW_H_MIN;

    /* Cap to available screen height (leave room for borders) */
    int max_h = LINES - 4;
    if (used_rows > max_h) used_rows = max_h;

    /* Determine if we have room for left/right borders (need PREVIEW_W+4).
     * On narrow terminals (COLS <= PREVIEW_W+3), go edge-to-edge with only
     * solid horizontal top/bottom lines — no side borders. */
    bool narrow = (COLS <= PREVIEW_W + 3);
    int bw, bh, bx, by;
    int content_x;

    if (narrow) {
        bw = COLS;
        bh = used_rows + 2; /* top line + content + bottom line */
        bx = 0;
        by = (LINES - bh) / 2;
        if (by < 0) by = 0;
        content_x = 0;
    } else {
        bw = PREVIEW_W + 4;
        bh = used_rows + 4;
        bx = (COLS - bw) / 2;
        by = (LINES - bh) / 2;
        if (bx < 0) bx = 0;
        if (by < 0) by = 0;
        content_x = bx + 2;
    }

    /* Clear area */
    attron(COLOR_PAIR(CP_FORM_BG));
    for (int r = by; r < by + bh; r++) {
        move(r, bx);
        for (int c = 0; c < bw; c++) addch(' ');
    }
    attroff(COLOR_PAIR(CP_FORM_BG));

    if (narrow) {
        /* Top horizontal line (solid, no corners) */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        move(by, bx);
        for (int c = 0; c < bw; c++) addch(ACS_HLINE);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));

        attron(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
        mvprintw(by, bx + 1, " Preview ");
        attroff(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);

        /* Bottom horizontal line (solid, no corners) */
        attron(COLOR_PAIR(CP_DIALOG_BORDER));
        move(by + bh - 1, bx);
        for (int c = 0; c < bw; c++) addch(ACS_HLINE);
        attroff(COLOR_PAIR(CP_DIALOG_BORDER));

        attron(COLOR_PAIR(CP_DIALOG_BTN_TEXT));
        mvprintw(by + bh - 1, bx + 1, " Press any key ");
        attroff(COLOR_PAIR(CP_DIALOG_BTN_TEXT));
    } else {
        draw_box(by, bx, bh, bw, CP_DIALOG_BORDER);

        attron(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
        mvprintw(by, bx + 2, " Preview ");
        attroff(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
    }

    /* Blit virtual screen using menu_preview color infrastructure */
    menu_preview_pairs_reset();

    int content_y = narrow ? by + 1 : by + 2;
    int max_col = narrow ? COLS : COLS - 2;

    for (int row = 0; row < used_rows; row++) {
        for (int col = 0; col < PREVIEW_W && content_x + col < max_col; col++) {
            uint8_t attr = va_buf[row * PREVIEW_W + col];
            int fg = attr & 0x0f;
            int bg = (attr >> 4) & 0x07;

            int attrs = 0;
            if (fg == 8) { attrs |= A_DIM; fg = 7; }
            else if (fg >= 9) { attrs |= A_BOLD; fg -= 8; }

            int pair = dos_pair_for_fg_bg(fg, bg);

#if defined(HAVE_WIDE_CURSES)
            {
                wchar_t wch = cp437_to_unicode((unsigned char)vch_buf[row * PREVIEW_W + col]);
                wchar_t wstr[2] = { wch, 0 };
                cchar_t cc;
                setcchar(&cc, wstr, attrs, (short)pair, NULL);
                mvadd_wch(content_y + row, content_x + col, &cc);
            }
#else
            attron(COLOR_PAIR(pair) | attrs);
            mvaddch(content_y + row, content_x + col,
                    (unsigned char)vch_buf[row * PREVIEW_W + col]);
            attroff(COLOR_PAIR(pair) | attrs);
#endif
        }
    }

    if (!narrow) {
        attron(COLOR_PAIR(CP_DIALOG_BTN_TEXT));
        mvprintw(by + bh - 1, bx + 2, " Press any key ");
        attroff(COLOR_PAIR(CP_DIALOG_BTN_TEXT));
    }

    refresh();
    getch();

    free(vch_buf);
    free(va_buf);
}

/* ========================================================================== */
/* Multi-line text helpers                                                     */
/* ========================================================================== */

#define ED_MAX_LINES 128
#define ED_MAX_COLS  1024

static int le_split_lines(const char *text, char lines[][ED_MAX_COLS], int max)
{
    int lc = 0, col = 0;
    const char *p = text;
    lines[0][0] = '\0';
    while (*p && lc < max) {
        if (p[0] == '\\' && p[1] == 'n') {
            lines[lc][col] = '\0'; lc++; col = 0;
            if (lc < max) lines[lc][0] = '\0';
            p += 2;
        } else {
            if (col < ED_MAX_COLS-1) lines[lc][col++] = *p;
            p++;
        }
    }
    if (lc < max) lines[lc][col] = '\0';
    return lc + 1;
}

static char *le_join_lines(char lines[][ED_MAX_COLS], int lc)
{
    size_t total = 0;
    for (int i = 0; i < lc; i++) total += strlen(lines[i]) + 2;
    char *out = malloc(total + 1);
    out[0] = '\0';
    for (int i = 0; i < lc; i++) {
        strcat(out, lines[i]);
        if (i < lc - 1) strcat(out, "\\n");
    }
    return out;
}

/* ========================================================================== */
/* Params metadata extraction                                                  */
/* ========================================================================== */

#define LE_MAX_PARAMS 15

/** Extracted parameter metadata from the TOML params field */
typedef struct {
    char name[64];
    char type[16];
    char desc[128];
} LeParam;

/**
 * @brief Extract params metadata from a raw TOML line.
 *
 * Parses `params = [{name = "...", type = "...", desc = "..."}]` from
 * the raw line and populates an array of LeParam structs.
 *
 * @param raw_line  The full TOML line (e.g. from LbEntry.raw_line).
 * @param out       Output array of LeParam (must hold LE_MAX_PARAMS).
 * @return Number of params extracted.
 */
static int le_extract_params(const char *raw_line, LeParam out[LE_MAX_PARAMS])
{
    if (!raw_line) return 0;

    /* Find "params" key in the line */
    const char *ps = strstr(raw_line, "params");
    if (!ps) return 0;

    /* Skip to the '=' */
    ps = strchr(ps, '=');
    if (!ps) return 0;
    ps++;
    while (*ps == ' ') ps++;
    if (*ps != '[') return 0;
    ps++;

    int count = 0;
    while (*ps && count < LE_MAX_PARAMS) {
        while (*ps == ' ' || *ps == ',') ps++;
        if (*ps == ']') break;
        if (*ps != '{') break;
        ps++; /* skip '{' */

        LeParam p = {{0}, {0}, {0}};

        /* Parse key = "value" pairs inside the inline table */
        while (*ps && *ps != '}') {
            while (*ps == ' ' || *ps == ',') ps++;
            if (*ps == '}') break;

            /* Read key */
            const char *ks = ps;
            while (*ps && *ps != '=' && *ps != ' ') ps++;
            size_t klen = (size_t)(ps - ks);
            char key[32] = {0};
            if (klen >= sizeof(key)) klen = sizeof(key) - 1;
            memcpy(key, ks, klen);

            /* Skip to value */
            while (*ps == ' ' || *ps == '=') ps++;
            if (*ps != '"') break;
            ps++; /* skip opening quote */

            const char *vs = ps;
            while (*ps && *ps != '"') ps++;
            size_t vlen = (size_t)(ps - vs);
            if (*ps == '"') ps++;

            if (strcmp(key, "name") == 0) {
                if (vlen >= sizeof(p.name)) vlen = sizeof(p.name) - 1;
                memcpy(p.name, vs, vlen);
                p.name[vlen] = '\0';
            } else if (strcmp(key, "type") == 0) {
                if (vlen >= sizeof(p.type)) vlen = sizeof(p.type) - 1;
                memcpy(p.type, vs, vlen);
                p.type[vlen] = '\0';
            } else if (strcmp(key, "desc") == 0) {
                if (vlen >= sizeof(p.desc)) vlen = sizeof(p.desc) - 1;
                memcpy(p.desc, vs, vlen);
                p.desc[vlen] = '\0';
            }
        }
        if (*ps == '}') ps++;
        out[count++] = p;
    }
    return count;
}

/* ========================================================================== */
/* Params popup                                                                */
/* ========================================================================== */

/**
 * @brief Show a scrollable popup listing all parameter metadata.
 *
 * Each line is colored:
 *   yellow [N]  bold-cyan name (cyan type bold-cyan)  grey - desc
 *
 * @param meta   Array of extracted LeParam structs.
 * @param count  Number of params.
 */
static void le_show_params_popup(const LeParam *meta, int count)
{
    if (count <= 0) return;

    /* Dialog geometry */
    int pw = 72;
    if (pw > COLS - 4) pw = COLS - 4;
    int ph = count + 4; /* border + title + entries + border */
    if (ph > LINES - 4) ph = LINES - 4;
    int list_h = ph - 4;
    int px = (COLS - pw) / 2;
    int py = (LINES - ph) / 2;

    int scroll = 0;
    int selected = 0;

    for (;;) {
        /* Clamp */
        if (selected < 0) selected = 0;
        if (selected >= count) selected = count - 1;
        if (scroll > selected) scroll = selected;
        if (selected >= scroll + list_h) scroll = selected - list_h + 1;

        /* Clear + border */
        attron(COLOR_PAIR(CP_FORM_BG));
        for (int r = py; r < py + ph; r++) {
            move(r, px);
            for (int c = 0; c < pw; c++) addch(' ');
        }
        attroff(COLOR_PAIR(CP_FORM_BG));
        draw_box(py, px, ph, pw, CP_DIALOG_BORDER);

        /* Title */
        attron(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
        mvprintw(py, px + 2, " Parameters (%d) ", count);
        attroff(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);

        /* Param list */
        for (int r = 0; r < list_h && (scroll + r) < count; r++) {
            int idx = scroll + r;
            int row = py + 2 + r;
            bool is_sel = (idx == selected);

            /* Slot character: 1-9, A-F */
            char slot = (idx < 9) ? (char)('1' + idx) : (char)('A' + idx - 9);

            /* [N] in yellow */
            attron(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_VALUE) | A_BOLD);
            mvprintw(row, px + 2, "[%c]", slot);
            attroff(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_VALUE) | A_BOLD);

            /* name in bold cyan */
            attron(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_LABEL) | A_BOLD);
            printw(" %s", meta[idx].name);
            attroff(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_LABEL) | A_BOLD);

            /* (type) in cyan */
            attron(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_LABEL));
            printw(" (");
            attroff(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_LABEL));
            attron(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_DIALOG_BORDER));
            printw("%s", meta[idx].type[0] ? meta[idx].type : "string");
            attroff(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_DIALOG_BORDER));
            attron(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_LABEL));
            printw(")");
            attroff(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_LABEL));

            /* - desc in grey */
            if (meta[idx].desc[0]) {
                attron(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_BG));
                int cx = getcurx(stdscr);
                int remain = px + pw - 3 - cx;
                if (remain > 3) {
                    printw(" - %.*s", remain - 3, meta[idx].desc);
                }
                attroff(COLOR_PAIR(is_sel ? CP_MENU_HIGHLIGHT : CP_FORM_BG));
            }
        }

        /* Scroll indicators */
        if (scroll > 0) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(py + 2, px + pw - 2, ACS_UARROW);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }
        if (scroll + list_h < count) {
            attron(COLOR_PAIR(CP_DIALOG_BORDER));
            mvaddch(py + 2 + list_h - 1, px + pw - 2, ACS_DARROW);
            attroff(COLOR_PAIR(CP_DIALOG_BORDER));
        }

        /* Footer */
        attron(COLOR_PAIR(CP_DIALOG_BTN_TEXT));
        mvprintw(py + ph - 1, px + 2, " [Esc] Close ");
        attroff(COLOR_PAIR(CP_DIALOG_BTN_TEXT));

        refresh();
        int ch = getch();

        switch (ch) {
        case 27: /* Esc */
        case 'q': case 'Q':
        case 's': case 'S':
            return;
        case KEY_UP:
            if (selected > 0) selected--;
            break;
        case KEY_DOWN:
            if (selected < count - 1) selected++;
            break;
        case KEY_PPAGE:
            selected -= list_h;
            break;
        case KEY_NPAGE:
            selected += list_h;
            break;
        case KEY_HOME:
            selected = 0; scroll = 0;
            break;
        case KEY_END:
            selected = count - 1;
            break;
        default:
            break;
        }
    }
}

/* ========================================================================== */
/* String editor dialog                                                        */
/* ========================================================================== */

/**
 * @brief Update in-memory entry and raw_line after a successful write-back.
 */
static void le_update_entry(LbEntry *entry, const char *new_text,
                            const char *toml_path)
{
    free(entry->full_value);
    entry->full_value = strdup(new_text);

    free(entry->value);
    char trunc[LB_MAX_VAL];
    snprintf(trunc, sizeof(trunc), "%.508s%s",
             new_text, strlen(new_text) > 508 ? "..." : "");
    entry->value = strdup(trunc);

    /* Re-read the raw line from file for accuracy */
    char new_raw[LB_MAX_LINE];
    FILE *rf = fopen(toml_path, "r");
    if (rf) {
        int ln = 0;
        while (fgets(new_raw, sizeof(new_raw), rf)) {
            ln++;
            if (ln == entry->line_num) {
                size_t rl = strlen(new_raw);
                if (rl > 0 && new_raw[rl-1] == '\n') new_raw[--rl] = '\0';
                if (rl > 0 && new_raw[rl-1] == '\r') new_raw[--rl] = '\0';
                free(entry->raw_line);
                entry->raw_line = strdup(new_raw);
                break;
            }
        }
        fclose(rf);
    }
}

/**
 * @brief Edit a language string in a custom multi-line editor dialog.
 *
 * Shows readonly Heap, Symbol, Flags, Params at top and a bordered
 * multi-line text area below.  F4=Preview, F5=Params, F3=MCI insert,
 * F2/F10=Save, Esc=Cancel.  All single-letter keys go to text input.
 *
 * @param entry      Pointer to the LbEntry being edited.
 * @param toml_path  Path to the TOML file for write-back.
 * @return true if the value was modified and saved.
 */
static bool le_edit_entry(LbEntry *entry, const char *toml_path)
{
    char lines[ED_MAX_LINES][ED_MAX_COLS];
    int line_count = le_split_lines(
        entry->full_value ? entry->full_value : "", lines, ED_MAX_LINES);
    int cr = 0, cc = 0, st = 0; /* cursor row, col, scroll top */
    bool modified = false;

    /* Dialog geometry */
    int dw = COLS - 4;
    if (dw > 100) dw = 100;
    int dh = LINES - 4;
    if (dh > 24) dh = 24;
    int dx = (COLS - dw) / 2;
    int dy = (LINES - dh) / 2;

    /* Extract structured params metadata from raw TOML line */
    LeParam meta_params[LE_MAX_PARAMS];
    int meta_count = le_extract_params(entry->raw_line, meta_params);

    /* If no metadata, scan text for |!N slots and build mock params */
    if (meta_count == 0) {
        const char *v = entry->full_value ? entry->full_value : "";
        for (const char *s = v; *s; s++) {
            if (s[0] == '|' && s[1] == '!' && s[2]) {
                int idx = -1;
                if (s[2] >= '1' && s[2] <= '9') idx = s[2] - '1';
                else if (s[2] >= 'A' && s[2] <= 'F') idx = s[2] - 'A' + 9;
                if (idx >= 0 && idx < 15 && meta_count < LE_MAX_PARAMS) {
                    snprintf(meta_params[meta_count].name, sizeof(meta_params[0].name),
                             "|!%c", s[2]);
                    snprintf(meta_params[meta_count].type, sizeof(meta_params[0].type),
                             "string");
                    snprintf(meta_params[meta_count].desc, sizeof(meta_params[0].desc),
                             "mock=%s", mci_pos_mocks[idx]);
                    meta_count++;
                }
            }
        }
    }

    /* Params display: show up to 2 inline, hint [S] for more */
    int params_inline = (meta_count > 2) ? 2 : meta_count;
    int params_extra_rows = params_inline; /* one row per inline param */
    if (meta_count > 2) params_extra_rows++; /* +1 for the [S] hint line */

    /* Text area placement — dynamic info_h based on param count */
    int info_h = 6 + params_extra_rows; /* Heap + Symbol + Flags + Params: + inline rows + blank + Text: */
    int ta_y = dy + 1 + info_h;
    int ta_x = dx + 2;
    int ta_w = dw - 4;
    int ta_h = dh - info_h - 4;
    if (ta_h < 3) ta_h = 3;

    for (;;) {
        /* Clear dialog area */
        for (int r = dy; r < dy + dh; r++) {
            move(r, dx);
            attron(COLOR_PAIR(CP_FORM_BG));
            for (int c = 0; c < dw; c++) addch(' ');
            attroff(COLOR_PAIR(CP_FORM_BG));
        }
        draw_box(dy, dx, dh, dw, CP_DIALOG_BORDER);

        /* Title */
        char title[128];
        snprintf(title, sizeof(title), " Edit: %s ", entry->dotted_key);
        attron(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);
        mvprintw(dy, dx + 2, "%s", title);
        attroff(COLOR_PAIR(CP_DIALOG_TITLE) | A_BOLD);

        /* Info fields */
        int iy = dy + 2;
        attron(COLOR_PAIR(CP_FORM_LABEL));
        mvprintw(iy, dx + 2, "Heap:   ");
        attroff(COLOR_PAIR(CP_FORM_LABEL));
        attron(COLOR_PAIR(CP_FORM_VALUE));
        printw("%s", entry->heap);
        attroff(COLOR_PAIR(CP_FORM_VALUE));

        iy++;
        attron(COLOR_PAIR(CP_FORM_LABEL));
        mvprintw(iy, dx + 2, "Symbol: ");
        attroff(COLOR_PAIR(CP_FORM_LABEL));
        attron(COLOR_PAIR(CP_FORM_VALUE));
        printw("%s", entry->symbol);
        attroff(COLOR_PAIR(CP_FORM_VALUE));

        iy++;
        attron(COLOR_PAIR(CP_FORM_LABEL));
        mvprintw(iy, dx + 2, "Flags:  ");
        attroff(COLOR_PAIR(CP_FORM_LABEL));
        attron(COLOR_PAIR(CP_FORM_VALUE));
        printw("%s", entry->flags_str[0] ? entry->flags_str : "(none)");
        attroff(COLOR_PAIR(CP_FORM_VALUE));

        iy++;
        attron(COLOR_PAIR(CP_FORM_LABEL));
        mvprintw(iy, dx + 2, "Params:");
        attroff(COLOR_PAIR(CP_FORM_LABEL));

        if (meta_count == 0) {
            attron(COLOR_PAIR(CP_FORM_BG));
            printw(" (none)");
            attroff(COLOR_PAIR(CP_FORM_BG));
        } else {
            /* Render up to params_inline entries as colored lines */
            for (int pi = 0; pi < params_inline; pi++) {
                iy++;
                char slot = (pi < 9) ? (char)('1' + pi) : (char)('A' + pi - 9);

                /* [N] in yellow */
                attron(COLOR_PAIR(CP_FORM_VALUE) | A_BOLD);
                mvprintw(iy, dx + 4, "[%c]", slot);
                attroff(COLOR_PAIR(CP_FORM_VALUE) | A_BOLD);

                /* name in bold cyan */
                attron(COLOR_PAIR(CP_FORM_LABEL) | A_BOLD);
                printw(" %s", meta_params[pi].name);
                attroff(COLOR_PAIR(CP_FORM_LABEL) | A_BOLD);

                /* (type) in cyan */
                attron(COLOR_PAIR(CP_FORM_LABEL));
                printw(" (");
                attroff(COLOR_PAIR(CP_FORM_LABEL));
                attron(COLOR_PAIR(CP_DIALOG_BORDER));
                printw("%s", meta_params[pi].type[0] ? meta_params[pi].type : "string");
                attroff(COLOR_PAIR(CP_DIALOG_BORDER));
                attron(COLOR_PAIR(CP_FORM_LABEL));
                printw(")");
                attroff(COLOR_PAIR(CP_FORM_LABEL));

                /* - desc in grey */
                if (meta_params[pi].desc[0]) {
                    attron(COLOR_PAIR(CP_FORM_BG));
                    int cx = getcurx(stdscr);
                    int remain = dx + dw - 3 - cx;
                    if (remain > 3)
                        printw(" - %.*s", remain - 3, meta_params[pi].desc);
                    attroff(COLOR_PAIR(CP_FORM_BG));
                }
            }

            /* If more than 2, show overflow hint */
            if (meta_count > 2) {
                iy++;
                attron(COLOR_PAIR(CP_DIALOG_BTN_TEXT));
                mvprintw(iy, dx + 4, "[.. %d more, press F5 to view all]",
                         meta_count - params_inline);
                attroff(COLOR_PAIR(CP_DIALOG_BTN_TEXT));
            }
        }

        iy += 2;
        attron(COLOR_PAIR(CP_FORM_LABEL));
        mvprintw(iy, dx + 2, "Text:");
        attroff(COLOR_PAIR(CP_FORM_LABEL));

        /* Text area border + content */
        draw_box(ta_y - 1, ta_x - 1, ta_h + 2, ta_w + 2, CP_DIALOG_BORDER);

        if (st > cr) st = cr;
        if (cr >= st + ta_h) st = cr - ta_h + 1;

        for (int r = 0; r < ta_h; r++) {
            int lr = st + r;
            move(ta_y + r, ta_x);
            if (lr < line_count) {
                attron(COLOR_PAIR(CP_FORM_VALUE));
                int len = (int)strlen(lines[lr]);
                for (int c = 0; c < ta_w; c++)
                    addch(c < len ? (unsigned char)lines[lr][c] : ' ');
                attroff(COLOR_PAIR(CP_FORM_VALUE));
            } else {
                attron(COLOR_PAIR(CP_FORM_BG));
                for (int c = 0; c < ta_w; c++) addch(' ');
                attroff(COLOR_PAIR(CP_FORM_BG));
            }
        }

        /* Footer */
        attron(COLOR_PAIR(CP_DIALOG_BTN_TEXT));
        mvprintw(dy + dh - 1, dx + 2,
                 " [F10] Save  [F4] Preview  [F5] Params  [F3] MCI  [Esc] Cancel ");
        attroff(COLOR_PAIR(CP_DIALOG_BTN_TEXT));
        if (modified) {
            attron(COLOR_PAIR(CP_ERROR));
            mvprintw(dy + dh - 1, dx + dw - 12, " Modified ");
            attroff(COLOR_PAIR(CP_ERROR));
        }

        /* Cursor */
        curs_set(1);
        int scr_y = ta_y + (cr - st);
        int scr_x = ta_x + cc;
        if (scr_x >= ta_x + ta_w) scr_x = ta_x + ta_w - 1;
        move(scr_y, scr_x);
        refresh();

        int ch = getch();

        switch (ch) {
        case 27: /* Esc */
            if (modified) {
                if (!dialog_confirm("Unsaved Changes",
                                    "Discard changes?"))
                    break;
            }
            curs_set(0);
            return false;

        case KEY_F(2): case KEY_F(10): { /* Save */
            char *joined = le_join_lines(lines, line_count);
            const char *bare = entry->symbol;
            if (lb_write_back(toml_path, entry->line_num, bare,
                              joined, entry->is_inline, entry->raw_line)) {
                le_update_entry(entry, joined, toml_path);
                free(joined);
                curs_set(0);
                return true;
            }
            free(joined);
            dialog_message("Write Error",
                           "Failed to write changes to TOML file.");
            break;
        }

        case KEY_F(4): {
            char *joined = le_join_lines(lines, line_count);
            le_preview(joined);
            free(joined);
            break;
        }

        case KEY_F(5):
            /* Show full params popup */
            if (meta_count > 0) {
                curs_set(0);
                le_show_params_popup(meta_params, meta_count);
            }
            break;

        case KEY_F(3): {
            /* MCI code helper — insert selected code at cursor */
            curs_set(0);
            const char *code = mci_helper_show();
            if (code) {
                int clen = (int)strlen(code);
                int llen = (int)strlen(lines[cr]);
                if (llen + clen < ED_MAX_COLS - 1) {
                    memmove(lines[cr] + cc + clen, lines[cr] + cc,
                            (size_t)(llen - cc + 1));
                    memcpy(lines[cr] + cc, code, (size_t)clen);
                    cc += clen;
                    modified = true;
                }
            }
            break;
        }

        /* Cursor movement */
        case KEY_UP:
            if (cr > 0) { cr--; if (cc > (int)strlen(lines[cr])) cc = (int)strlen(lines[cr]); }
            break;
        case KEY_DOWN:
            if (cr < line_count - 1) { cr++; if (cc > (int)strlen(lines[cr])) cc = (int)strlen(lines[cr]); }
            break;
        case KEY_LEFT:
            if (cc > 0) cc--;
            else if (cr > 0) { cr--; cc = (int)strlen(lines[cr]); }
            break;
        case KEY_RIGHT:
            if (cc < (int)strlen(lines[cr])) cc++;
            else if (cr < line_count - 1) { cr++; cc = 0; }
            break;
        case KEY_HOME:
            cc = 0;
            break;
        case KEY_END:
            cc = (int)strlen(lines[cr]);
            break;

        /* Enter — split line */
        case '\n': case '\r': case KEY_ENTER:
            if (line_count < ED_MAX_LINES) {
                for (int i = line_count; i > cr + 1; i--)
                    memcpy(lines[i], lines[i-1], ED_MAX_COLS);
                int len = (int)strlen(lines[cr]);
                strcpy(lines[cr + 1], lines[cr] + cc);
                lines[cr][cc] = '\0';
                (void)len;
                line_count++;
                cr++;
                cc = 0;
                modified = true;
            }
            break;

        /* Backspace */
        case KEY_BACKSPACE: case 127: case 8:
            if (cc > 0) {
                int len = (int)strlen(lines[cr]);
                memmove(lines[cr] + cc - 1, lines[cr] + cc, (size_t)(len - cc + 1));
                cc--;
                modified = true;
            } else if (cr > 0) {
                /* Join with previous line */
                int prev_len = (int)strlen(lines[cr - 1]);
                if (prev_len + (int)strlen(lines[cr]) < ED_MAX_COLS - 1) {
                    strcat(lines[cr - 1], lines[cr]);
                    for (int i = cr; i < line_count - 1; i++)
                        memcpy(lines[i], lines[i+1], ED_MAX_COLS);
                    line_count--;
                    cr--;
                    cc = prev_len;
                    modified = true;
                }
            }
            break;

        /* Delete */
        case KEY_DC:
            if (cc < (int)strlen(lines[cr])) {
                int len = (int)strlen(lines[cr]);
                memmove(lines[cr] + cc, lines[cr] + cc + 1, (size_t)(len - cc));
                modified = true;
            } else if (cr < line_count - 1) {
                /* Join with next line */
                if ((int)strlen(lines[cr]) + (int)strlen(lines[cr+1]) < ED_MAX_COLS - 1) {
                    strcat(lines[cr], lines[cr + 1]);
                    for (int i = cr + 1; i < line_count - 1; i++)
                        memcpy(lines[i], lines[i+1], ED_MAX_COLS);
                    line_count--;
                    modified = true;
                }
            }
            break;

        default:
            /* Printable character — insert */
            if (isprint(ch) || ch == '\t') {
                int len = (int)strlen(lines[cr]);
                if (len < ED_MAX_COLS - 2) {
                    memmove(lines[cr] + cc + 1, lines[cr] + cc,
                            (size_t)(len - cc + 1));
                    lines[cr][cc] = (char)ch;
                    cc++;
                    modified = true;
                }
            }
            break;
        }
    }
}

/* ========================================================================== */
/* Public API                                                                  */
/* ========================================================================== */

void lang_browse_strings(const char *toml_path)
{
    LbEntry *entries = NULL;
    int entry_count = 0;

    if (!lb_parse_toml(toml_path, &entries, &entry_count)) {
        dialog_message("Language Editor", "Failed to parse TOML language file.");
        return;
    }
    if (entry_count == 0) {
        dialog_message("Language Editor", "No string entries found in this file.");
        lb_free_entries(entries, entry_count);
        return;
    }

    /* Collect unique heap names for dropdown */
    char **heaps = NULL;
    int heap_count = 0;
    lb_collect_heaps(entries, entry_count, &heaps, &heap_count);

    /* Browser state */
    char filter[64] = "";
    int filter_len = 0;
    int heap_idx = 0;      /* 0 = All, 1..N = specific heap */
    int flags_idx = 0;     /* 0 = All, 1 = mex, 2 = (none) */
    int selected = 0;
    int scroll_top = 0;
    bool filter_active = false; /* true when typing in filter field */

    /* Column widths */
    int col_heap = 14;
    int col_sym  = 22;

    for (;;) {
        /* Build filtered visibility list */
        int vis_count = 0;
        const char *hf = (heap_idx > 0 && heap_idx <= heap_count)
                         ? heaps[heap_idx - 1] : "";
        int *vis = lb_build_vis(entries, entry_count, filter, hf,
                                flags_idx, &vis_count);

        if (selected >= vis_count) selected = vis_count - 1;
        if (selected < 0) selected = 0;

        /* Screen geometry */
        int box_y = WORK_AREA_TOP;
        int box_x = 0;
        int box_w = COLS;
        int box_h = LINES - WORK_AREA_TOP - 1; /* leave status row */
        int list_top = box_y + 4; /* after title + filter bar + header + sep */
        int list_h = box_h - 6;   /* room for footer */
        int col_text = box_w - col_heap - col_sym - 6;
        if (col_text < 10) col_text = 10;

        /* Scroll */
        if (scroll_top > selected) scroll_top = selected;
        if (selected >= scroll_top + list_h) scroll_top = selected - list_h + 1;
        if (scroll_top < 0) scroll_top = 0;

        /* Clear entire browser area with solid background */
        attron(COLOR_PAIR(CP_FORM_BG));
        for (int r = box_y; r < box_y + box_h; r++) {
            move(r, box_x);
            for (int c = 0; c < box_w; c++) addch(' ');
        }
        attroff(COLOR_PAIR(CP_FORM_BG));

        /* Title bar */
        const char *fname = strrchr(toml_path, '/');
        fname = fname ? fname + 1 : toml_path;
        attron(COLOR_PAIR(CP_TITLE_BAR));
        move(box_y, box_x);
        for (int i = 0; i < box_w; i++) addch(' ');
        mvprintw(box_y, box_x + 1, " Language Editor: %s  (%d/%d)",
                 fname, vis_count, entry_count);
        attroff(COLOR_PAIR(CP_TITLE_BAR));

        /* Filter bar */
        int fb_y = box_y + 1;
        attron(COLOR_PAIR(CP_FORM_BG));
        move(fb_y, box_x);
        for (int i = 0; i < box_w; i++) addch(' ');
        attroff(COLOR_PAIR(CP_FORM_BG));

        attron(COLOR_PAIR(CP_FORM_LABEL));
        mvprintw(fb_y, box_x + 1, "Filter:");
        attroff(COLOR_PAIR(CP_FORM_LABEL));

        /* Filter text field */
        if (filter_active)
            attron(COLOR_PAIR(CP_FORM_HIGHLIGHT));
        else
            attron(COLOR_PAIR(CP_FORM_VALUE));
        mvprintw(fb_y, box_x + 9, "%-16s", filter);
        if (filter_active)
            attroff(COLOR_PAIR(CP_FORM_HIGHLIGHT));
        else
            attroff(COLOR_PAIR(CP_FORM_VALUE));

        /* Heap dropdown */
        attron(COLOR_PAIR(CP_FORM_LABEL));
        mvprintw(fb_y, box_x + 27, "Heap:");
        attroff(COLOR_PAIR(CP_FORM_LABEL));
        attron(COLOR_PAIR(CP_FORM_VALUE));
        if (heap_idx == 0)
            mvprintw(fb_y, box_x + 33, "[All       ]");
        else
            mvprintw(fb_y, box_x + 33, "[%-10.10s]", heaps[heap_idx - 1]);
        attroff(COLOR_PAIR(CP_FORM_VALUE));

        /* Flags dropdown */
        attron(COLOR_PAIR(CP_FORM_LABEL));
        mvprintw(fb_y, box_x + 48, "Flags:");
        attroff(COLOR_PAIR(CP_FORM_LABEL));
        attron(COLOR_PAIR(CP_FORM_VALUE));
        mvprintw(fb_y, box_x + 55, "[%-6s]", flags_opts[flags_idx]);
        attroff(COLOR_PAIR(CP_FORM_VALUE));

        /* Column headers */
        int hdr_y = box_y + 2;
        attron(COLOR_PAIR(CP_FORM_LABEL) | A_BOLD);
        mvprintw(hdr_y, box_x + 1, "%-*s %-*s %s",
                 col_heap, "Heap", col_sym, "Symbol", "Text");
        attroff(COLOR_PAIR(CP_FORM_LABEL) | A_BOLD);

        /* Separator */
        attron(COLOR_PAIR(CP_FORM_BG));
        move(hdr_y + 1, box_x);
        for (int i = 0; i < box_w; i++) addch(ACS_HLINE);
        attroff(COLOR_PAIR(CP_FORM_BG));

        /* List rows — per-column coloring:
         *   heap   = yellow  (CP_FORM_VALUE)
         *   symbol = white   (CP_FORM_BG + A_BOLD)
         *   text   = grey    (CP_FORM_BG, no bold)
         * Selected row uses CP_FORM_HIGHLIGHT uniformly. */
        for (int r = 0; r < list_h; r++) {
            int vi = scroll_top + r;
            move(list_top + r, box_x);
            if (vi < vis_count) {
                int ei = vis[vi];
                LbEntry *e = &entries[ei];
                bool is_sel = (vi == selected);

                /* Heap column — yellow (or highlight if selected) */
                int hp = is_sel ? CP_FORM_HIGHLIGHT : CP_FORM_VALUE;
                attron(COLOR_PAIR(hp));
                char hbuf[32]; snprintf(hbuf, sizeof(hbuf), "%-*.*s",
                                        col_heap, col_heap, e->heap);
                printw("%s ", hbuf);
                attroff(COLOR_PAIR(hp));

                /* Symbol column — bold white (or highlight if selected) */
                int sp = is_sel ? CP_FORM_HIGHLIGHT : CP_FORM_BG;
                int sa = is_sel ? 0 : A_BOLD;
                attron(COLOR_PAIR(sp) | sa);
                char sbuf[32]; snprintf(sbuf, sizeof(sbuf), "%-*.*s",
                                        col_sym, col_sym, e->symbol);
                printw("%s ", sbuf);
                attroff(COLOR_PAIR(sp) | sa);

                /* Text column — grey (or highlight if selected) */
                int tp = is_sel ? CP_FORM_HIGHLIGHT : CP_FORM_BG;
                attron(COLOR_PAIR(tp));
                int tlen = (int)strlen(e->value);
                int tw = col_text;
                if (tlen > tw - 1) {
                    char tbuf[512];
                    snprintf(tbuf, sizeof(tbuf), "%.*s...", tw - 4, e->value);
                    printw("%-*s", tw, tbuf);
                } else {
                    printw("%-*s", tw, e->value);
                }
                attroff(COLOR_PAIR(tp));
            } else {
                /* Empty row */
                attron(COLOR_PAIR(CP_FORM_BG));
                for (int c = 0; c < box_w; c++) addch(' ');
                attroff(COLOR_PAIR(CP_FORM_BG));
            }
        }

        /* Footer */
        int ft_y = box_y + box_h - 1;
        attron(COLOR_PAIR(CP_STATUS_BAR));
        move(ft_y, box_x);
        for (int i = 0; i < box_w; i++) addch(' ');
        if (filter_active)
            mvprintw(ft_y, box_x + 1,
                     " Type to filter, [Enter] apply, [Esc] cancel ");
        else
            mvprintw(ft_y, box_x + 1,
                     " [Enter] Edit  [/] Filter  [H] Heap  [G] Flags  [Esc] Quit ");
        attroff(COLOR_PAIR(CP_STATUS_BAR));

        curs_set(filter_active ? 1 : 0);
        if (filter_active)
            move(fb_y, box_x + 9 + filter_len);

        refresh();

        int ch = getch();
        free(vis);

        if (filter_active) {
            /* Filter input mode */
            if (ch == 27) { /* Esc — cancel filter */
                filter_active = false;
            } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                filter_active = false;
                selected = 0; scroll_top = 0;
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                if (filter_len > 0) filter[--filter_len] = '\0';
                selected = 0; scroll_top = 0;
            } else if (isprint(ch) && filter_len < 63) {
                filter[filter_len++] = (char)ch;
                filter[filter_len] = '\0';
                selected = 0; scroll_top = 0;
            }
            continue;
        }

        /* Normal mode */
        switch (ch) {
        case 27: /* Esc */
            curs_set(0);
            lb_free_heaps(heaps, heap_count);
            lb_free_entries(entries, entry_count);
            draw_work_area();
            return;
        case '/': case 'f': case 'F':
            filter_active = true;
            break;
        case 'h': case 'H':
            heap_idx = (heap_idx + 1) % (heap_count + 1);
            selected = 0; scroll_top = 0;
            break;
        case 'g': case 'G':
            flags_idx = (flags_idx + 1) % FLAGS_OPT_COUNT;
            selected = 0; scroll_top = 0;
            break;
        case KEY_UP:
            if (selected > 0) selected--;
            break;
        case KEY_DOWN:
            selected++;
            break;
        case KEY_PPAGE:
            selected -= list_h;
            if (selected < 0) selected = 0;
            break;
        case KEY_NPAGE:
            selected += list_h;
            break;
        case KEY_HOME:
            selected = 0;
            break;
        case KEY_END:
            selected = vis_count - 1;
            break;
        case '\n': case '\r': case KEY_ENTER: {
            /* Rebuild vis for the current filters to get the entry index */
            int vc2 = 0;
            int *v2 = lb_build_vis(entries, entry_count, filter, hf,
                                   flags_idx, &vc2);
            if (selected >= 0 && selected < vc2) {
                int ei = v2[selected];
                le_edit_entry(&entries[ei], toml_path);
            }
            free(v2);
            break;
        }
        default:
            break;
        }
    }
}

void action_browse_lang_strings(void *unused)
{
    (void)unused;

    if (g_maxcfg_toml == NULL) {
        dialog_message("Configuration Not Loaded", "TOML configuration is not loaded.");
        return;
    }

    /* Resolve language directory from config */
    const char *sys_path = NULL;
    {
        MaxCfgVar v;
        if (maxcfg_toml_get(g_maxcfg_toml, "maximus.sys_path", &v) == MAXCFG_OK &&
            v.type == MAXCFG_VAR_STRING && v.v.s) {
            sys_path = v.v.s;
        }
    }

    const char *lang_rel = NULL;
    {
        MaxCfgVar v;
        if (maxcfg_toml_get(g_maxcfg_toml, "maximus.lang_path", &v) == MAXCFG_OK &&
            v.type == MAXCFG_VAR_STRING && v.v.s) {
            lang_rel = v.v.s;
        }
    }

    char lang_dir[1024];
    if (lang_rel && lang_rel[0] == '/') {
        snprintf(lang_dir, sizeof(lang_dir), "%s", lang_rel);
    } else if (sys_path && lang_rel) {
        snprintf(lang_dir, sizeof(lang_dir), "%s/%s", sys_path, lang_rel);
    } else if (sys_path) {
        /* Fallback: derive from config_path TOML key + /lang */
        const char *cfg_rel = "config";
        MaxCfgVar cv;
        if (maxcfg_toml_get(g_maxcfg_toml, "maximus.config_path", &cv) == MAXCFG_OK &&
            cv.type == MAXCFG_VAR_STRING && cv.v.s && cv.v.s[0])
            cfg_rel = cv.v.s;
        snprintf(lang_dir, sizeof(lang_dir), "%s/%s/lang", sys_path, cfg_rel);
    } else {
        dialog_message("Language Browser", "Cannot determine language directory.\n"
                       "Set maximus.sys_path and maximus.lang_path first.");
        return;
    }

    /* Scan for .toml files */
    DIR *d = opendir(lang_dir);
    if (!d) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Cannot open language directory:\n%s", lang_dir);
        dialog_message("Language Browser", msg);
        return;
    }

    ListItem files[32];
    int file_count = 0;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && file_count < 32) {
        size_t nlen = strlen(ent->d_name);
        if (nlen > 5 && strcmp(ent->d_name + nlen - 5, ".toml") == 0) {
            files[file_count].name = strdup(ent->d_name);
            files[file_count].extra = NULL;
            files[file_count].enabled = true;
            files[file_count].data = NULL;
            file_count++;
        }
    }
    closedir(d);

    if (file_count == 0) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "No .toml language files found in:\n%s\n\n"
                 "Use Tools > Convert Legacy Language to create one.", lang_dir);
        dialog_message("Language Browser", msg);
        return;
    }

    /* If only one file, open it directly; otherwise pick */
    int pick = 0;
    if (file_count > 1) {
        ListPickResult r = listpicker_show("Select Language File", files, file_count, &pick);
        if (r != LISTPICK_EDIT) {
            for (int i = 0; i < file_count; i++) free(files[i].name);
            return;
        }
    }

    /* Build full path and browse */
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", lang_dir, files[pick].name);

    for (int i = 0; i < file_count; i++) free(files[i].name);

    lang_browse_strings(full_path);
}
