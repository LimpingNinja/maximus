/**
 * @file mci_preview.h
 * @brief Shared MCI (Maximus Control Interface) interpreter for preview rendering.
 *
 * Provides a generic virtual-screen MCI expander used by both the language
 * string editor preview and the menu editor preview.  Handles pipe color
 * codes, format operators, info code expansion, cursor codes, AVATAR
 * attributes, backslash escapes, and legacy %t substitution.
 *
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef MCI_PREVIEW_H
#define MCI_PREVIEW_H

#include <stdint.h>

/* ======================================================================== */
/* Virtual screen                                                           */
/* ======================================================================== */

/**
 * @brief Generic virtual screen for MCI preview rendering.
 *
 * Both the language preview (6×80) and menu preview (25×80) are backed by
 * contiguous row-major char+attr grids.  This struct lets the interpreter
 * write into either without knowing the concrete type.
 */
typedef struct {
    char    *ch;    /**< Character grid  (row-major, rows × cols) */
    uint8_t *attr;  /**< Attribute grid  (row-major, rows × cols) */
    int      cols;  /**< Number of columns */
    int      rows;  /**< Number of rows */
} MciVScreen;

/** @brief Convenience macro: wrap a 2-D array pair into an MciVScreen. */
#define MCI_VS_WRAP(vs, ch_arr, attr_arr, c, r) do { \
    (vs)->ch   = &(ch_arr)[0][0];                     \
    (vs)->attr = &(attr_arr)[0][0];                    \
    (vs)->cols = (c);                                  \
    (vs)->rows = (r);                                  \
} while (0)

/* ======================================================================== */
/* Interpreter state                                                        */
/* ======================================================================== */

/** @brief Pending-format types (mirrors MciExpand in mci.c). */
enum {
    MCI_FMT_NONE = 0,
    MCI_FMT_LEFTPAD,   /**< $L — pad on LEFT  (right-align text) */
    MCI_FMT_RIGHTPAD,  /**< $R — pad on RIGHT (left-align text)  */
    MCI_FMT_CENTER     /**< $C — pad both sides                  */
};

/**
 * @brief MCI interpreter state carried between calls.
 *
 * Callers may chain multiple mci_preview_expand() calls on the same
 * state (e.g. title then prompt) without resetting.
 */
typedef struct {
    int      cx, cy;           /**< Cursor position (0-based) */
    uint8_t  ca;               /**< Current DOS attribute     */
    int      pending_fmt;      /**< Pending pad type          */
    int      pending_width;    /**< Pending pad width (-1 = none) */
    char     pending_padch;    /**< Pending pad character     */
    int      pending_trim;     /**< Pending trim width (-1 = none) */
    int      pending_pad_space;/**< |PD flag                  */
} MciState;

/* ======================================================================== */
/* Mock data for info-code expansion                                        */
/* ======================================================================== */

/**
 * @brief Mock data for MCI info code and %t expansion.
 *
 * Populated once from the userdb (first user) and TOML system config;
 * falls back to hardcoded defaults when the database is unavailable.
 */
typedef struct {
    char user_name[36];
    char user_alias[21];
    char user_city[36];
    char user_phone[15];
    char user_dataphone[19];
    char system_name[80];
    char sysop_name[80];
    unsigned long times_called;
    unsigned long calls_today;
    unsigned long msgs_posted;
    unsigned long kb_down;
    unsigned long kb_up;
    unsigned long files_down;
    unsigned long files_up;
    long  kb_down_today;
    int   time_left;
    int   screen_len;
    char  term_emul[16];
    char  msg_area[64];
    char  file_area[64];
} MciMockData;

/* ======================================================================== */
/* Public API                                                               */
/* ======================================================================== */

/** @brief Mock numeric values for |!1..|!F positional parameter substitution. */
extern const char *mci_pos_mocks[15];

/** @brief Initialize MCI interpreter state (cursor 0,0 — gray on black). */
void mci_state_init(MciState *st);

/** @brief Load mock data from userdb + TOML config (with hardcoded fallback). */
void mci_mock_load(MciMockData *m);

/** @brief Clear a virtual screen to spaces with attribute 0x07. */
void mci_vs_clear(MciVScreen *vs);

/**
 * @brief Expand an MCI string into a virtual screen.
 *
 * Processes the full set of MCI codes:
 *   - |00–|31 color codes
 *   - |CL, |CR, |CD, |BS  terminal controls
 *   - |!1–|!F             positional parameters
 *   - |XY                 info codes (mock data)
 *   - |PD                 pad-space modifier
 *   - $D##C / $D|!NC      repeat character
 *   - $X##C / $X|!NC      goto column with fill
 *   - $L/$R/$T/$C##       pending format (space pad)
 *   - $l/$r/$c##C         pending format (custom pad)
 *   - [X##/[Y##           cursor positioning
 *   - [A##–[D##           cursor movement
 *   - [K                  clear to EOL
 *   - ||, $$              escape sequences
 *   - \\n, \\t, \\xHH    backslash escapes
 *   - \\x16               AVATAR attribute byte
 *   - %t                  legacy time-left substitution
 *
 * @param vs   Target virtual screen
 * @param st   Interpreter state (may be reused across calls)
 * @param mock Mock data for info-code expansion (may be NULL)
 * @param text NUL-terminated MCI string to expand
 */
void mci_preview_expand(MciVScreen *vs, MciState *st,
                        const MciMockData *mock, const char *text);

#endif /* MCI_PREVIEW_H */
