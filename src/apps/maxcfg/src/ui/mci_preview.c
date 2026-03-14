/*
 * mci_preview.c — MCI preview/expand renderer
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
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mci_preview.h"
#include "libmaxcfg.h"
#include "libmaxdb.h"

/* Declared in maxcfg.c — global TOML handle */
extern MaxCfgToml *g_maxcfg_toml;

/* ======================================================================== */
/* Mock positional parameter values                                         */
/* ======================================================================== */

/** @brief Mock numeric values for |!1..|!F substitution.
 *  Numeric so they work as both display text and as counts in $D/$X. */
const char *mci_pos_mocks[15] = {
    "35","10","78","5","99","01","15","25","08","30","00","02","07","20","50"
};

/* ======================================================================== */
/* State / screen helpers                                                   */
/* ======================================================================== */

/**
 * @brief Initialize an MCI interpreter state to defaults.
 *
 * @param st State struct to initialize.
 */
void mci_state_init(MciState *st)
{
    st->cx = 0;
    st->cy = 0;
    st->ca = 0x07;
    st->pending_fmt = MCI_FMT_NONE;
    st->pending_width = -1;
    st->pending_padch = ' ';
    st->pending_trim = -1;
    st->pending_pad_space = 0;
}

/**
 * @brief Clear a virtual screen buffer to spaces with default attribute.
 *
 * @param vs Virtual screen to clear.
 */
void mci_vs_clear(MciVScreen *vs)
{
    memset(vs->ch,   ' ',  (size_t)(vs->rows * vs->cols));
    memset(vs->attr, 0x07, (size_t)(vs->rows * vs->cols));
}

/* ======================================================================== */
/* Mock data loader                                                         */
/* ======================================================================== */

/**
 * @brief Load mock user/system data for MCI preview rendering.
 *
 * Populates sensible defaults, then overrides with values from the
 * loaded TOML config and the first user in the user database.
 *
 * @param m Mock data struct to populate.
 */
void mci_mock_load(MciMockData *m)
{
    /* Hardcoded defaults */
    strcpy(m->user_name,  "Test User");
    strcpy(m->user_alias, "Tester");
    strcpy(m->user_city,  "Anytown, USA");
    strcpy(m->user_phone, "555-1234");
    strcpy(m->user_dataphone, "555-5678");
    strcpy(m->system_name, "Maximus BBS");
    strcpy(m->sysop_name,  "SysOp");
    m->times_called  = 42;
    m->calls_today   = 1;
    m->msgs_posted   = 10;
    m->kb_down       = 1024;
    m->kb_up         = 512;
    m->files_down    = 5;
    m->files_up      = 2;
    m->kb_down_today = 128;
    m->time_left     = 60;
    m->screen_len    = 24;
    m->screen_width  = 80;
    strcpy(m->term_emul, "ANSI");
    strcpy(m->msg_area,  "General");
    strcpy(m->file_area, "Uploads");

    if (!g_maxcfg_toml) return;

    /* System info from TOML config */
    MaxCfgVar v;
    if (maxcfg_toml_get(g_maxcfg_toml, "maximus.system_name", &v) == MAXCFG_OK
        && v.type == MAXCFG_VAR_STRING && v.v.s)
        snprintf(m->system_name, sizeof(m->system_name), "%s", v.v.s);
    if (maxcfg_toml_get(g_maxcfg_toml, "maximus.sysop", &v) == MAXCFG_OK
        && v.type == MAXCFG_VAR_STRING && v.v.s)
        snprintf(m->sysop_name, sizeof(m->sysop_name), "%s", v.v.s);

    /* First user from userdb */
    const char *sys_path = NULL;
    if (maxcfg_toml_get(g_maxcfg_toml, "maximus.sys_path", &v) == MAXCFG_OK
        && v.type == MAXCFG_VAR_STRING && v.v.s)
        sys_path = v.v.s;

    if (!sys_path) return;

    char db_path[1024];
    snprintf(db_path, sizeof(db_path), "%s/data/users/user.db", sys_path);
    MaxDB *db = maxdb_open(db_path, MAXDB_OPEN_READONLY);
    if (!db) return;

    MaxDBUser *u = maxdb_user_find_by_id(db, 0);
    if (!u) u = maxdb_user_find_by_id(db, 1);
    if (u) {
        if (u->name[0])      snprintf(m->user_name, sizeof(m->user_name), "%s", u->name);
        if (u->alias[0])     snprintf(m->user_alias, sizeof(m->user_alias), "%s", u->alias);
        if (u->city[0])      snprintf(m->user_city, sizeof(m->user_city), "%s", u->city);
        if (u->phone[0])     snprintf(m->user_phone, sizeof(m->user_phone), "%s", u->phone);
        if (u->dataphone[0]) snprintf(m->user_dataphone, sizeof(m->user_dataphone), "%s", u->dataphone);
        m->times_called  = u->times;
        m->calls_today   = u->call;
        m->msgs_posted   = u->msgs_posted;
        m->kb_down       = u->down;
        m->kb_up         = u->up;
        m->files_down    = u->ndown;
        m->files_up      = u->nup;
        m->kb_down_today = u->downtoday;
        m->screen_len    = u->len ? u->len : 24;
        m->screen_width  = u->width ? u->width : 80;
        if (u->msg[0])   snprintf(m->msg_area, sizeof(m->msg_area), "%s", u->msg);
        if (u->files[0]) snprintf(m->file_area, sizeof(m->file_area), "%s", u->files);
        maxdb_user_free(u);
    }
    maxdb_close(db);
}

/* ======================================================================== */
/* Internal helpers                                                         */
/* ======================================================================== */

/** @brief Parse two decimal digits at p, return value or -1. */
static int parse_2dig(const char *p)
{
    if (isdigit((unsigned char)p[0]) && isdigit((unsigned char)p[1]))
        return (p[0] - '0') * 10 + (p[1] - '0');
    return -1;
}

/** @brief Parse a |!N positional index character, return 0-based index or -1. */
static int parse_pos_idx(char ch)
{
    if (ch >= '1' && ch <= '9') return ch - '1';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 9;
    return -1;
}

/** @brief Check if ch is a TOML-converter type suffix (d=int, l=long, u=uint, c=char). */
static inline bool is_type_suffix(char ch)
{
    return ch == 'd' || ch == 'l' || ch == 'u' || ch == 'c';
}

/** @brief Put a single character + attribute into the virtual screen. */
static inline void vs_putc(MciVScreen *vs, MciState *st, char ch)
{
    if (st->cy < vs->rows && st->cx < vs->cols) {
        int off = st->cy * vs->cols + st->cx;
        vs->ch[off]   = ch;
        vs->attr[off] = st->ca;
        st->cx++;
    }
    if (st->cx >= vs->cols) { st->cy++; st->cx = 0; }
}

/** @brief Write a NUL-terminated string into the virtual screen. */
static void vs_puts(MciVScreen *vs, MciState *st, const char *s)
{
    while (*s && st->cy < vs->rows) {
        vs_putc(vs, st, *s);
        s++;
    }
}

/**
 * @brief Apply pending format modifiers (trim, pad, pad-space) to an expansion.
 *
 * Writes the formatted result into buf.  Resets all consumed pending state.
 */
static void apply_fmt(char *buf, size_t bufsz, const char *expanded,
                      MciState *st)
{
    char tmp[256];

    /* Optionally prepend space for |PD */
    if (st->pending_pad_space && expanded[0]) {
        snprintf(tmp, sizeof(tmp), " %s", expanded);
        st->pending_pad_space = 0;
    } else {
        snprintf(tmp, sizeof(tmp), "%s", expanded);
    }

    /* Apply trim — truncate visible length */
    if (st->pending_trim >= 0) {
        int len = (int)strlen(tmp);
        if (len > st->pending_trim)
            tmp[st->pending_trim] = '\0';
        st->pending_trim = -1;
    }

    /* Apply padding */
    if (st->pending_fmt != MCI_FMT_NONE && st->pending_width >= 0) {
        int vlen = (int)strlen(tmp);
        int pad  = (st->pending_width > vlen) ? (st->pending_width - vlen) : 0;

        if (pad > 0 && (size_t)(vlen + pad + 1) < sizeof(tmp)) {
            char padded[256];
            if (st->pending_fmt == MCI_FMT_LEFTPAD) {
                memset(padded, st->pending_padch, pad);
                memcpy(padded + pad, tmp, vlen + 1);
            } else if (st->pending_fmt == MCI_FMT_RIGHTPAD) {
                memcpy(padded, tmp, vlen);
                memset(padded + vlen, st->pending_padch, pad);
                padded[vlen + pad] = '\0';
            } else { /* MCI_FMT_CENTER */
                int left  = pad / 2;
                int right = pad - left;
                memset(padded, st->pending_padch, left);
                memcpy(padded + left, tmp, vlen);
                memset(padded + left + vlen, st->pending_padch, right);
                padded[left + vlen + right] = '\0';
            }
            snprintf(tmp, sizeof(tmp), "%s", padded);
        }

        st->pending_fmt   = MCI_FMT_NONE;
        st->pending_width  = -1;
        st->pending_padch  = ' ';
    }

    snprintf(buf, bufsz, "%s", tmp);
}

/**
 * @brief Expand an MCI info code (two uppercase letters) into mock text.
 *
 * Returns a pointer to a static buffer, or NULL if unrecognized.
 */
static const char *expand_info(char a, char b, const MciMockData *mock)
{
    static char buf[256];
    buf[0] = '\0';

    if (!mock) return NULL;

    if (a == 'B' && b == 'N') return mock->system_name;
    if (a == 'S' && b == 'N') return mock->sysop_name;
    if (a == 'U' && b == 'N') return mock->user_name;
    if (a == 'U' && b == 'H') return mock->user_alias;
    if (a == 'U' && b == 'R') return mock->user_name;
    if (a == 'U' && b == 'C') return mock->user_city;
    if (a == 'U' && b == 'P') return mock->user_phone;
    if (a == 'U' && b == 'D') return mock->user_dataphone;
    if (a == 'C' && b == 'S') { snprintf(buf, sizeof(buf), "%lu", mock->times_called); return buf; }
    if (a == 'C' && b == 'T') { snprintf(buf, sizeof(buf), "%lu", mock->calls_today); return buf; }
    if (a == 'M' && b == 'P') { snprintf(buf, sizeof(buf), "%lu", mock->msgs_posted); return buf; }
    if (a == 'D' && b == 'K') { snprintf(buf, sizeof(buf), "%lu", mock->kb_down); return buf; }
    if (a == 'F' && b == 'K') { snprintf(buf, sizeof(buf), "%lu", mock->kb_up); return buf; }
    if (a == 'D' && b == 'L') { snprintf(buf, sizeof(buf), "%lu", mock->files_down); return buf; }
    if (a == 'F' && b == 'U') { snprintf(buf, sizeof(buf), "%lu", mock->files_up); return buf; }
    if (a == 'D' && b == 'T') { snprintf(buf, sizeof(buf), "%ld", mock->kb_down_today); return buf; }
    if (a == 'T' && b == 'L') { snprintf(buf, sizeof(buf), "%d",  mock->time_left); return buf; }
    if (a == 'U' && b == 'S') { snprintf(buf, sizeof(buf), "%d",  mock->screen_len); return buf; }
    if (a == 'T' && b == 'E') return mock->term_emul;

    /* Terminal geometry codes */
    if (a == 'T' && b == 'W') { int w = mock->screen_width ? mock->screen_width : 80; snprintf(buf, sizeof(buf), "%02d", w > 99 ? 99 : w); return buf; }
    if (a == 'T' && b == 'C') { int w = mock->screen_width ? mock->screen_width : 80; snprintf(buf, sizeof(buf), "%02d", (w + 1) / 2); return buf; }
    if (a == 'T' && b == 'H') { int w = mock->screen_width ? mock->screen_width : 80; snprintf(buf, sizeof(buf), "%02d", w / 2); return buf; }
    if (a == 'M' && b == 'B') return mock->msg_area;
    if (a == 'M' && b == 'D') return mock->msg_area;
    if (a == 'F' && b == 'B') return mock->file_area;
    if (a == 'F' && b == 'D') return mock->file_area;
    if (a == 'U' && b == '#') { snprintf(buf, sizeof(buf), "%d", 1); return buf; }

    if (a == 'D' && b == 'A') {
        time_t now = time(NULL);
        strftime(buf, sizeof(buf), "%d %b %y", localtime(&now));
        return buf;
    }
    if (a == 'T' && b == 'M') {
        time_t now = time(NULL);
        strftime(buf, sizeof(buf), "%H:%M", localtime(&now));
        return buf;
    }
    if (a == 'T' && b == 'S') {
        time_t now = time(NULL);
        strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now));
        return buf;
    }

    return NULL;
}

/**
 * @brief Parse a format-op width: literal 2-digit number or |XY info code.
 *
 * Mirrors mci_parse_width() in mci.c for the preview engine.
 *
 * @param p        Pointer into the input string (right after the op letter).
 * @param mock     Mock data for info code expansion (may be NULL).
 * @param out_val  Receives the parsed integer (0-99).
 * @param out_len  Receives the number of input characters consumed.
 * @return         1 on success, 0 on failure.
 */
static int parse_width_preview(const char *p, const MciMockData *mock,
                               int *out_val, int *out_len)
{
    /* Fast path: literal two digits */
    int n = parse_2dig(p);
    if (n >= 0) {
        *out_val = n;
        *out_len = 2;
        return 1;
    }

    /* Slow path: |XY info code that resolves to a number */
    if (p[0] == '|' && p[1] && p[2] &&
        ((p[1] >= 'A' && p[1] <= 'Z' && p[2] >= 'A' && p[2] <= 'Z') ||
         (p[1] == 'U' && p[2] == '#')))
    {
        const char *val = expand_info(p[1], p[2], mock);
        if (val && val[0]) {
            char *end = NULL;
            long v = strtol(val, &end, 10);
            if (end != val && *end == '\0' && v >= 0 && v <= 99) {
                *out_val = (int)v;
                *out_len = 3; /* consumed |XY */
                return 1;
            }
        }
    }

    return 0;
}

/* ======================================================================== */
/* Main interpreter                                                         */
/* ======================================================================== */

/**
 * @brief Expand an MCI-encoded text string into a virtual screen buffer.
 *
 * Interprets pipe color codes, cursor control, format operators, positional
 * parameters, theme color slots, and info code substitutions.
 *
 * @param vs   Virtual screen buffer to render into.
 * @param st   Interpreter state (cursor position, current attribute, etc.).
 * @param mock Mock data for info code expansion (may be NULL).
 * @param text The MCI-encoded input string.
 */
void mci_preview_expand(MciVScreen *vs, MciState *st,
                        const MciMockData *mock, const char *text)
{
    if (!vs || !st || !text) return;

    const char *p = text;
    while (*p && st->cy < vs->rows) {

        /* ---- Backslash escape sequences ---- */
        if (p[0] == '\\' && p[1]) {
            switch (p[1]) {
            case 'n': st->cy++; st->cx = 0; p += 2; continue;
            case 'r': case 'a': p += 2; continue;
            case 't': st->cx = (st->cx + 8) & ~7; p += 2; continue;
            case 'x':
                if (isxdigit((unsigned char)p[2]) && isxdigit((unsigned char)p[3])) {
                    char hex[3] = { p[2], p[3], '\0' };
                    unsigned char byte = (unsigned char)strtoul(hex, NULL, 16);
                    if (byte == 0x16 && p[4]) {
                        /* AVATAR attribute: \x16 <attr_byte> */
                        if (p[4] == '\\' && p[5] == 'x' &&
                            isxdigit((unsigned char)p[6]) && isxdigit((unsigned char)p[7])) {
                            char ah[3] = { p[6], p[7], '\0' };
                            st->ca = (uint8_t)strtoul(ah, NULL, 16);
                            p += 8;
                        } else {
                            st->ca = (uint8_t)p[4];
                            p += 5;
                        }
                        continue;
                    }
                    vs_putc(vs, st, (char)byte);
                    p += 4; continue;
                }
                break;
            default: break;
            }
        }

        /* ---- || literal pipe ---- */
        if (p[0] == '|' && p[1] == '|') {
            vs_putc(vs, st, '|');
            p += 2; continue;
        }

        /* ---- $$ literal dollar ---- */
        if (p[0] == '$' && p[1] == '$') {
            vs_putc(vs, st, '$');
            p += 2; continue;
        }

        /* ---- %t legacy time-left substitution ---- */
        if (p[0] == '%' && p[1] == 't') {
            char tbuf[16];
            snprintf(tbuf, sizeof(tbuf), "%d", mock ? mock->time_left : 60);
            char fmtbuf[256];
            apply_fmt(fmtbuf, sizeof(fmtbuf), tbuf, st);
            vs_puts(vs, st, fmtbuf);
            p += 2; continue;
        }

        /* ---- $ format operators ---- */
        if (p[0] == '$' && p[1]) {
            char op = p[1];
            int n = 0;
            int wlen = 0; /* chars consumed by width (2 for ##, 3 for |XY) */

            /* $C/$L/$R/$T — pending format, space pad */
            if ((op == 'C' || op == 'L' || op == 'R' || op == 'T') &&
                parse_width_preview(p + 2, mock, &n, &wlen)) {
                if (op == 'T') {
                    st->pending_trim = n;
                } else {
                    st->pending_width  = n;
                    st->pending_padch  = ' ';
                    st->pending_fmt = (op == 'C') ? MCI_FMT_CENTER
                                    : (op == 'L') ? MCI_FMT_LEFTPAD
                                                  : MCI_FMT_RIGHTPAD;
                }
                p += 2 + wlen; continue;
            }

            /* $c/$l/$r — pending format, custom pad char */
            if ((op == 'c' || op == 'l' || op == 'r') &&
                parse_width_preview(p + 2, mock, &n, &wlen) && p[2 + wlen]) {
                st->pending_width  = n;
                st->pending_padch  = p[2 + wlen];
                st->pending_fmt = (op == 'c') ? MCI_FMT_CENTER
                                : (op == 'l') ? MCI_FMT_LEFTPAD
                                              : MCI_FMT_RIGHTPAD;
                p += 2 + wlen + 1; continue;
            }

            /* $D — repeat character */
            if (op == 'D') {
                if (parse_width_preview(p + 2, mock, &n, &wlen) && p[2 + wlen]) {
                    char ch = p[2 + wlen];
                    for (int i = 0; i < n && st->cy < vs->rows; i++)
                        vs_putc(vs, st, ch);
                    p += 2 + wlen + 1; continue;
                }
                /* $D|!N[suffix]C — positional param as count, then char.
                 * Optional type suffix (d/l/u/c) between slot and fill char. */
                if (p[2] == '|' && p[3] == '!' && p[4] && p[5]) {
                    int idx = parse_pos_idx(p[4]);
                    int skip = 6;
                    char ch = p[5];
                    if (is_type_suffix(p[5]) && p[6]) { ch = p[6]; skip = 7; }
                    if (idx >= 0 && idx < 15) {
                        int cnt = atoi(mci_pos_mocks[idx]);
                        for (int i = 0; i < cnt && st->cy < vs->rows; i++)
                            vs_putc(vs, st, ch);
                    }
                    p += skip; continue;
                }
            }

            /* $X — goto column with fill */
            if (op == 'X') {
                if (parse_width_preview(p + 2, mock, &n, &wlen) && p[2 + wlen]) {
                    char ch = p[2 + wlen];
                    int target = n - 1;
                    while (st->cx < target && st->cx < vs->cols && st->cy < vs->rows)
                        vs_putc(vs, st, ch);
                    p += 2 + wlen + 1; continue;
                }
                /* $X|!N[suffix]C — same type suffix handling as $D */
                if (p[2] == '|' && p[3] == '!' && p[4] && p[5]) {
                    int idx = parse_pos_idx(p[4]);
                    int skip = 6;
                    char ch = p[5];
                    if (is_type_suffix(p[5]) && p[6]) { ch = p[6]; skip = 7; }
                    if (idx >= 0 && idx < 15) {
                        int target = atoi(mci_pos_mocks[idx]) - 1;
                        while (st->cx < target && st->cx < vs->cols && st->cy < vs->rows)
                            vs_putc(vs, st, ch);
                    }
                    p += skip; continue;
                }
            }
        }

        /* ---- Cursor codes (|[X##, |[Y##, |[K, |[0, |[1, |[<, |[>, |[H, etc.) ---- */
        if (p[0] == '|' && p[1] == '[') {
            char cc = p[2];
            int n = 0, wlen = 0;

            /* Parameterless codes */
            if (cc == 'K') {
                if (st->cy >= 0 && st->cy < vs->rows) {
                    for (int c = st->cx; c < vs->cols; c++) {
                        int off = st->cy * vs->cols + c;
                        vs->ch[off]   = ' ';
                        vs->attr[off] = st->ca;
                    }
                }
                p += 3; continue;
            }
            if (cc == '0' || cc == '1') { p += 3; continue; }
            /* |[< — beginning of line */
            if (cc == '<') { st->cx = 0; p += 3; continue; }
            /* |[> — end of line */
            if (cc == '>') {
                int w = mock ? (mock->screen_width ? mock->screen_width : 80) : 80;
                st->cx = w - 1;
                p += 3; continue;
            }
            /* |[H — center column */
            if (cc == 'H') {
                int w = mock ? (mock->screen_width ? mock->screen_width : 80) : 80;
                st->cx = ((w + 1) / 2) - 1;
                p += 3; continue;
            }

            /* Parametric codes — ## or |XY width */
            if (cc == 'X' && parse_width_preview(p + 3, mock, &n, &wlen)) {
                st->cx = n - 1; if (st->cx < 0) st->cx = 0;
                p += 3 + wlen; continue;
            }
            if (cc == 'Y' && parse_width_preview(p + 3, mock, &n, &wlen)) {
                st->cy = n - 1; if (st->cy < 0) st->cy = 0;
                p += 3 + wlen; continue;
            }
            if (cc == 'A' && parse_width_preview(p + 3, mock, &n, &wlen)) {
                st->cy -= n; if (st->cy < 0) st->cy = 0;
                p += 3 + wlen; continue;
            }
            if (cc == 'B' && parse_width_preview(p + 3, mock, &n, &wlen)) {
                st->cy += n; p += 3 + wlen; continue;
            }
            if (cc == 'C' && parse_width_preview(p + 3, mock, &n, &wlen)) {
                st->cx += n; p += 3 + wlen; continue;
            }
            if (cc == 'D' && parse_width_preview(p + 3, mock, &n, &wlen)) {
                st->cx -= n; if (st->cx < 0) st->cx = 0;
                p += 3 + wlen; continue;
            }
            if (cc == 'L' && parse_width_preview(p + 3, mock, &n, &wlen)) {
                st->cx = n - 1; if (st->cx < 0) st->cx = 0;
                if (st->cy >= 0 && st->cy < vs->rows) {
                    for (int c = st->cx; c < vs->cols; c++) {
                        int off = st->cy * vs->cols + c;
                        vs->ch[off]   = ' ';
                        vs->attr[off] = st->ca;
                    }
                }
                p += 3 + wlen; continue;
            }
        }

        /* ---- Pipe codes ---- */

        if (p[0] == '|' && p[1]) {
            /* |!N[suffix] — positional parameter substitution (with format).
             * The TOML converter may append a type suffix (d/l/u/c) after
             * the slot character — skip it so it doesn't render as text. */
            if (p[1] == '!' && p[2]) {
                int idx = parse_pos_idx(p[2]);
                if (idx >= 0 && idx < 15) {
                    char fmtbuf[256];
                    apply_fmt(fmtbuf, sizeof(fmtbuf), mci_pos_mocks[idx], st);
                    vs_puts(vs, st, fmtbuf);
                }
                p += 3;
                if (is_type_suffix(*p)) p++; /* skip optional type suffix */
                continue;
            }

            /* |xx — lowercase semantic theme color codes.
             * Look up in the global theme table and recursively expand the
             * stored pipe string (which contains |NN color codes). */
            if (p[1] >= 'a' && p[1] <= 'z' && p[2] >= 'a' && p[2] <= 'z') {
                extern MaxCfgThemeColors g_theme_colors;
                const char *exp = maxcfg_theme_lookup(&g_theme_colors, p[1], p[2]);
                if (exp) {
                    /* Walk the expansion string and apply any |NN codes it contains */
                    const char *e = exp;
                    while (*e) {
                        if (e[0] == '|' && isdigit((unsigned char)e[1]) && isdigit((unsigned char)e[2])) {
                            int code = (e[1] - '0') * 10 + (e[2] - '0');
                            if (code >= 0 && code <= 15)
                                st->ca = (st->ca & 0xf0) | (uint8_t)(code & 0x0f);
                            else if (code >= 16 && code <= 23)
                                st->ca = (st->ca & 0x0f) | (uint8_t)((code - 16) << 4);
                            else if (code >= 24 && code <= 31)
                                st->ca = (st->ca & 0x0f) | (uint8_t)((code - 24) << 4);
                            e += 3;
                        } else {
                            e++;
                        }
                    }
                }
                p += 3; continue;
            }

            /* |NN — color codes 00–31 */
            if (isdigit((unsigned char)p[1]) && isdigit((unsigned char)p[2])) {
                int code = (p[1] - '0') * 10 + (p[2] - '0');
                if (code >= 0 && code <= 15)
                    st->ca = (st->ca & 0xf0) | (uint8_t)(code & 0x0f);
                else if (code >= 16 && code <= 23)
                    st->ca = (st->ca & 0x0f) | (uint8_t)((code - 16) << 4);
                else if (code >= 24 && code <= 31)
                    st->ca = (st->ca & 0x0f) | (uint8_t)((code - 24) << 4);
                p += 3; continue;
            }

            /* |DF{path} — display file embedding.
             * In preview, render a placeholder since we can't display files. */
            if (p[1] == 'D' && p[2] == 'F' && p[3] == '{') {
                const char *end = strchr(p + 4, '}');
                if (end) {
                    /* Show [FILE:basename] placeholder in dim text */
                    uint8_t saved_ca = st->ca;
                    st->ca = 0x08; /* dark gray */
                    vs_puts(vs, st, "[FILE]");
                    st->ca = saved_ca;
                    p = end + 1; continue;
                }
            }

            /* |{string} — inline string literal, usable as a format-op value.
             * Everything between '{' and '}' is the literal text. */
            if (p[1] == '{') {
                const char *end = strchr(p + 2, '}');
                if (end) {
                    size_t slen = (size_t)(end - (p + 2));
                    char raw[256];
                    if (slen >= sizeof(raw)) slen = sizeof(raw) - 1;
                    memcpy(raw, p + 2, slen);
                    raw[slen] = '\0';

                    char fmtbuf[256];
                    apply_fmt(fmtbuf, sizeof(fmtbuf), raw, st);
                    vs_puts(vs, st, fmtbuf);
                    p = end + 1; continue;
                }
                /* No closing brace — fall through to literal output */
            }

            /* |XY — terminal control + info codes (two uppercase letters) */
            if (p[1] >= 'A' && p[1] <= 'Z' && p[2] >= 'A' && p[2] <= 'Z') {
                char a = p[1], b = p[2];

                /* Terminal controls */
                if (a == 'C' && b == 'L') {
                    mci_vs_clear(vs);
                    st->cx = 0; st->cy = 0; st->ca = 0x07;
                    p += 3; continue;
                }
                if (a == 'C' && b == 'R') { st->cy++; st->cx = 0; p += 3; continue; }
                if (a == 'C' && b == 'D') { st->ca = 0x07; p += 3; continue; }
                if (a == 'B' && b == 'S') {
                    if (st->cx > 0) {
                        st->cx--;
                        int off = st->cy * vs->cols + st->cx;
                        if (off >= 0 && off < vs->rows * vs->cols)
                            vs->ch[off] = ' ';
                    }
                    p += 3; continue;
                }
                /* SA/RA, SS/RS, LC/LF — no-op in preview */
                if ((a == 'S' && b == 'A') || (a == 'R' && b == 'A') ||
                    (a == 'S' && b == 'S') || (a == 'R' && b == 'S') ||
                    (a == 'L' && b == 'C') || (a == 'L' && b == 'F')) {
                    p += 3; continue;
                }

                /* Info code expansion — apply pending format */
                const char *expanded = expand_info(a, b, mock);
                if (expanded && expanded[0]) {
                    char fmtbuf[256];
                    apply_fmt(fmtbuf, sizeof(fmtbuf), expanded, st);
                    vs_puts(vs, st, fmtbuf);
                    p += 3; continue;
                }

                /* Unknown |XY — skip */
                p += 3; continue;
            }

            /* |U# — user number (uppercase + symbol) */
            if (p[1] == 'U' && p[2] == '#') {
                char fmtbuf[256];
                apply_fmt(fmtbuf, sizeof(fmtbuf), "1", st);
                vs_puts(vs, st, fmtbuf);
                p += 3; continue;
            }

            /* |&& — CPR, no-op in preview */
            if (p[1] == '&' && p[2] == '&') { p += 3; continue; }

            /* |PD — pad space before next expansion */
            if (p[1] == 'P' && p[2] == 'D') { st->pending_pad_space = 1; p += 3; continue; }
        }

        /* ---- Normal character ---- */
        vs_putc(vs, st, *p);
        p++;
    }
}
