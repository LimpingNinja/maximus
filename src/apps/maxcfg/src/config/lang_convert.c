/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * lang_convert.c - Legacy .MAD language file to TOML converter
 *
 * Parses MAID-format .mad language source files, resolves #include/#define
 * directives, converts AVATAR color/cursor sequences to MCI codes, and
 * writes TOML output compatible with the new maxlang API.
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include "lang_convert.h"

/* ========================================================================== */
/* Constants                                                                   */
/* ========================================================================== */

#define LC_MAX_LINE       4096   /**< Maximum line length in .mad file */
#define LC_MAX_VARNAME    64     /**< Maximum symbol name length */
#define LC_MAX_CONTENT    8192   /**< Maximum expanded string content */
#define LC_MAX_DEFINES    512    /**< Maximum #define macros */
#define LC_MAX_INCLUDES   8      /**< Maximum nested #include depth */
#define LC_MAX_STRINGS    512    /**< Maximum strings per heap */
#define LC_MAX_HEAPS      32     /**< Maximum heap count */
#define LC_MAX_PATH       1024   /**< Maximum path length */

/* ========================================================================== */
/* Data structures                                                             */
/* ========================================================================== */

/** A #define macro: name → replacement text */
typedef struct {
    char *name;
    char *value;
} LcDefine;

/** Flags parsed from @ prefixes on a string line */
typedef struct {
    bool mex;       /**< @MEX — export to MEX */
    bool rip;       /**< @RIP or @ALT — alternate string */
} LcFlags;

/** A single language string entry */
typedef struct {
    char symbol[LC_MAX_VARNAME]; /**< Symbol name (e.g. "located") */
    char *text;                  /**< Converted text content */
    char *rip_text;              /**< RIP alternate text (NULL if none) */
    bool has_mex;                /**< Has @MEX flag */
    int  legacy_id;              /**< Legacy s_ret() numeric ID */
} LcString;

/** A language heap section */
typedef struct {
    char name[LC_MAX_VARNAME];   /**< Heap name (e.g. "global") */
    bool is_user_heap;           /**< True if defined with '=' prefix */
    LcString strings[LC_MAX_STRINGS];
    int count;                   /**< Number of strings in this heap */
    int base_id;                 /**< Legacy base ID for this heap */
} LcHeap;

/** Full converter state */
typedef struct {
    LcDefine defines[LC_MAX_DEFINES];
    int      num_defines;

    FILE    *include_stack[LC_MAX_INCLUDES];
    char    *include_names[LC_MAX_INCLUDES];
    int      include_lines[LC_MAX_INCLUDES];
    int      include_depth;

    LcHeap   heaps[LC_MAX_HEAPS];
    int      num_heaps;

    int      global_string_id;   /**< Running legacy string ID counter */
    char     base_dir[LC_MAX_PATH]; /**< Directory of the root .mad file */

    char     err[512];
} LcState;

/* ========================================================================== */
/* Forward declarations                                                        */
/* ========================================================================== */

static bool lc_parse_file(LcState *st, const char *path);
static bool lc_process_line(LcState *st, char *line);
static bool lc_write_toml(LcState *st, const char *out_path);
static void lc_expand_macros(LcState *st, const char *input, char *output, size_t out_sz);
static void lc_process_backslashes(const char *input, unsigned char *output, size_t out_sz, size_t *out_len);
static void lc_avatar_to_mci(const unsigned char *raw, size_t raw_len, char *output, size_t out_sz);
static void lc_cleanup(LcState *st);

/* ========================================================================== */
/* Utility helpers                                                             */
/* ========================================================================== */

static void set_err(char *err, size_t len, const char *fmt, ...)
{
    if (!err || !len) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(err, len, fmt, ap);
    va_end(ap);
}

/** Check if character is valid in a symbol name */
static bool is_id_char(int c)
{
    return (isalnum(c) || c == '_' || c == '-' || c == '\'');
}

/** Strip trailing whitespace / newline */
static void strip_trailing(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' ||
                        s[len - 1] == ' '  || s[len - 1] == '\t')) {
        s[--len] = '\0';
    }
}

/** Strip leading whitespace, shifting in-place */
static void strip_leading(char *s)
{
    char *p = s;
    while (*p == ' ' || *p == '\t') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

/** Case-insensitive prefix match */
static bool starts_with_ci(const char *s, const char *prefix, size_t prefix_len)
{
    return strncasecmp(s, prefix, prefix_len) == 0;
}

/* ========================================================================== */
/* Preprocessor: #define handling                                               */
/* ========================================================================== */

static bool lc_add_define(LcState *st, const char *line)
{
    if (st->num_defines >= LC_MAX_DEFINES) {
        snprintf(st->err, sizeof(st->err), "Too many #define macros (max %d)", LC_MAX_DEFINES);
        return false;
    }

    /* Skip "#define" and whitespace */
    const char *p = line;
    while (*p && !isspace(*p)) p++;  /* skip "#define" */
    while (*p && isspace(*p))  p++;  /* skip whitespace */

    /* Extract name */
    const char *name_start = p;
    while (*p && !isspace(*p)) p++;
    size_t name_len = (size_t)(p - name_start);
    if (name_len == 0) return true; /* empty define, ignore */

    /* Extract value (rest of line) */
    while (*p && isspace(*p)) p++;

    LcDefine *d = &st->defines[st->num_defines++];
    d->name = strndup(name_start, name_len);
    d->value = strdup(*p ? p : "");
    return true;
}

/**
 * @brief Expand all known macros in input, writing result to output.
 *
 * Scans for identifier tokens and replaces them with their #define values.
 * Does NOT recurse (single-pass expansion, same as MAID).
 */
static void lc_expand_macros(LcState *st, const char *input, char *output, size_t out_sz)
{
    const char *s = input;
    char *d = output;
    char *end = output + out_sz - 1;

    while (*s && d < end) {
        /* Inside a quoted string, copy verbatim (handle backslash escapes) */
        if (*s == '"') {
            *d++ = *s++;
            while (*s && *s != '"' && d < end) {
                if (*s == '\\' && s[1]) {
                    *d++ = *s++;
                    if (d < end) *d++ = *s++;
                } else {
                    *d++ = *s++;
                }
            }
            if (*s == '"' && d < end) *d++ = *s++;
            continue;
        }

        /* Check for identifier token to expand */
        if (is_id_char(*s) && !isdigit(*s)) {
            const char *tok_start = s;
            while (is_id_char(*s)) s++;
            size_t tok_len = (size_t)(s - tok_start);

            /* Search defines */
            bool found = false;
            for (int i = 0; i < st->num_defines; i++) {
                if (strlen(st->defines[i].name) == tok_len &&
                    strncasecmp(st->defines[i].name, tok_start, tok_len) == 0) {
                    size_t vlen = strlen(st->defines[i].value);
                    if (d + vlen < end) {
                        memcpy(d, st->defines[i].value, vlen);
                        d += vlen;
                    }
                    found = true;
                    break;
                }
            }
            if (!found) {
                /* Not a macro — copy token as-is */
                size_t copy = (tok_len < (size_t)(end - d)) ? tok_len : (size_t)(end - d);
                memcpy(d, tok_start, copy);
                d += copy;
            }
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';
}

/* ========================================================================== */
/* Preprocessor: #include handling                                             */
/* ========================================================================== */

static bool lc_push_include(LcState *st, const char *filename)
{
    if (st->include_depth + 1 >= LC_MAX_INCLUDES) {
        snprintf(st->err, sizeof(st->err), "Too many nested includes (max %d)", LC_MAX_INCLUDES);
        return false;
    }

    char path[LC_MAX_PATH];

    /* Try relative to base_dir first */
    snprintf(path, sizeof(path), "%s/%s", st->base_dir, filename);
    FILE *f = fopen(path, "r");

    if (!f) {
        /* Try the filename as-is (absolute or cwd-relative) */
        f = fopen(filename, "r");
    }

    if (!f) {
        snprintf(st->err, sizeof(st->err), "Cannot open include file: %s", filename);
        return false;
    }

    st->include_depth++;
    st->include_stack[st->include_depth] = f;
    st->include_names[st->include_depth] = strdup(path);
    st->include_lines[st->include_depth] = 0;
    return true;
}

static bool lc_handle_include(LcState *st, const char *line)
{
    /* Skip "#include" and whitespace */
    const char *p = line;
    while (*p && !isspace(*p)) p++;
    while (*p && isspace(*p))  p++;

    char delim = 0;
    if (*p == '"') delim = '"';
    else if (*p == '<') delim = '>';
    else {
        snprintf(st->err, sizeof(st->err), "Invalid #include syntax: %s", line);
        return false;
    }

    p++; /* skip opening delimiter */
    const char *start = p;
    while (*p && *p != delim) p++;

    char filename[LC_MAX_PATH];
    size_t len = (size_t)(p - start);
    if (len >= sizeof(filename)) len = sizeof(filename) - 1;
    memcpy(filename, start, len);
    filename[len] = '\0';

    return lc_push_include(st, filename);
}

/* ========================================================================== */
/* String content extraction (mirrors MAID's Get_Var_Contents)                 */
/* ========================================================================== */

/**
 * @brief Extract the string content portion after "symbol= ".
 *
 * Handles quoted strings with macro expansion between quotes,
 * semicolon termination, and multi-line continuation (lines not
 * ending with ';').  The raw result still contains backslash escapes.
 */
static void lc_get_var_contents(LcState *st, const char *s, char *out, size_t out_sz)
{
    char *d = out;
    char *end = out + out_sz - 1;

    while (*s && isspace(*s)) s++;

    /* Walk through the content, handling "quoted" sections and bare macros */
    while (*s && *s != ';' && d < end) {
        if (*s == '"') {
            s++; /* skip opening quote */
            while (*s && *s != '"' && d < end) {
                if (*s == '\\' && s[1]) {
                    /* Preserve backslash escapes for later processing */
                    *d++ = *s++;
                    if (d < end) *d++ = *s++;
                } else {
                    *d++ = *s++;
                }
            }
            if (*s == '"') s++; /* skip closing quote */
        } else if (isspace(*s)) {
            s++;
        } else if (*s == ';') {
            break;
        } else {
            /* Bare token — should be a macro name, expand it */
            const char *tok_start = s;
            while (*s && !isspace(*s) && *s != '"' && *s != ';') s++;
            size_t tok_len = (size_t)(s - tok_start);

            bool found = false;
            for (int i = 0; i < st->num_defines; i++) {
                if (strlen(st->defines[i].name) == tok_len &&
                    strncasecmp(st->defines[i].name, tok_start, tok_len) == 0) {
                    size_t vlen = strlen(st->defines[i].value);
                    if (d + vlen < end) {
                        memcpy(d, st->defines[i].value, vlen);
                        d += vlen;
                    }
                    found = true;
                    break;
                }
            }
            if (!found) {
                /* Copy unknown token as-is */
                size_t copy = (tok_len < (size_t)(end - d)) ? tok_len : (size_t)(end - d);
                memcpy(d, tok_start, copy);
                d += copy;
            }
        }
    }
    *d = '\0';
}

/* ========================================================================== */
/* Backslash escape processing (mirrors MAID's Process_Backslashes)            */
/* ========================================================================== */

/**
 * @brief Convert backslash escape sequences to raw bytes.
 *
 * Handles \\xHH (hex), \\r, \\n, \\a, \\\\, and literal pass-through.
 * Output is raw bytes (may contain NUL-adjacent bytes like 0x16).
 * out_len receives the number of output bytes (not NUL-terminated for
 * binary safety, but a NUL is appended for convenience).
 */
static void lc_process_backslashes(const char *input, unsigned char *output,
                                   size_t out_sz, size_t *out_len)
{
    const char *s = input;
    unsigned char *d = output;
    unsigned char *end = output + out_sz - 1;

    while (*s && d < end) {
        if (*s != '\\') {
            *d++ = (unsigned char)*s++;
        } else {
            s++; /* skip backslash */
            if (*s == 'x' || *s == 'X') {
                s++;
                unsigned int val = 0;
                if (sscanf(s, "%02x", &val) == 1) {
                    *d++ = (unsigned char)val;
                    s += 2;
                }
            } else if (*s == 'r') { *d++ = '\r'; s++; }
            else if (*s == 'n')   { *d++ = '\n'; s++; }
            else if (*s == 'a')   { *d++ = '\a'; s++; }
            else if (*s == '\\')  { *d++ = '\\'; s++; }
            else if (*s == '"')   { *d++ = '"';  s++; }
            else { *d++ = (unsigned char)*s++; }
        }
    }
    *out_len = (size_t)(d - output);
    *d = '\0';
}

/* ========================================================================== */
/* Printf → |!N positional parameter conversion (Round 2, spec §3.3)           */
/* ========================================================================== */

/**
 * @brief Convert C printf format specifiers to |!N positional parameters.
 *
 * Scans the string for printf-style format specifiers and replaces each
 * with the next sequential |!N code (|!1..|!9, |!A..|!F).
 * Literal %% is preserved as %%.
 *
 * A format specifier is: %[flags][width][.precision][length]type
 *   flags:     - + 0 space #
 *   width:     digits or *
 *   precision: . followed by digits or *
 *   length:    h hh l ll L z j t
 *   type:      d i o u x X e E f g G a A c s n p
 *
 * @param input   NUL-terminated MCI string (output of lc_avatar_to_mci).
 * @param output  Buffer for result with |!N placeholders.
 * @param out_sz  Size of the output buffer.
 */
static void lc_printf_to_positional(const char *input, char *output, size_t out_sz)
{
    char *d = output;
    char *end = output + out_sz - 1;
    const char *s = input;
    int param_idx = 0; /* 0-based: 0→|!1, 8→|!9, 9→|!A, 14→|!F */

    while (*s && d < end) {
        if (*s != '%') {
            *d++ = *s++;
            continue;
        }

        /* Found '%' — check what follows */
        const char *spec_start = s;
        s++; /* skip '%' */

        /* %% → literal %% (keep escaped for TOML) */
        if (*s == '%') {
            if (d + 1 < end) { *d++ = '%'; *d++ = '%'; }
            s++;
            continue;
        }

        /* Parse flags: - + 0 space # */
        bool flag_minus = false;
        bool flag_zero = false;
        while (*s == '-' || *s == '+' || *s == '0' || *s == ' ' || *s == '#') {
            if (*s == '-') flag_minus = true;
            if (*s == '0') flag_zero = true;
            s++;
        }

        /* Parse width: digits or * */
        int width = 0;
        bool has_width = false;
        if (*s == '*') { s++; }
        else if (*s >= '1' && *s <= '9') {
            has_width = true;
            while (*s >= '0' && *s <= '9') { width = width * 10 + (*s - '0'); s++; }
        }

        /* Parse precision: .digits or .* */
        int precision = 0;
        bool has_precision = false;
        if (*s == '.') {
            s++;
            has_precision = true;
            if (*s == '*') { s++; }
            else while (*s >= '0' && *s <= '9') { precision = precision * 10 + (*s - '0'); s++; }
        }

        /* Parse length modifier: hh h ll l L z j t */
        bool len_long = false;
        if (*s == 'h') { s++; if (*s == 'h') s++; }
        else if (*s == 'l') { len_long = true; s++; if (*s == 'l') s++; }
        else if (*s == 'L' || *s == 'z' || *s == 'j' || *s == 't') s++;

        /* Parse conversion type */
        if (*s && strchr("diouxXeEfgGaAcsnp", *s)) {
            char type_ch = *s;
            s++; /* consume the type character */

            /* Emit MCI format operations + |!N for this parameter.
             * Mapping:
             *   %-Ns       → $R<N>|!X    (left-align = right-pad)
             *   %Ns        → $L<N>|!X    (right-align = left-pad)
             *   %-N.Ms     → $T<M>$R<N>|!X (truncate + right-pad)
             *   %N.Ms      → $T<M>$L<N>|!X (truncate + left-pad)
             *   %0Nd       → $l<N>0|!X   (zero-pad left)
             */
            if (param_idx < 15 && d + 2 < end) {
                /* Emit truncation if precision specified on string types */
                if (has_precision && precision > 0 &&
                    (type_ch == 's' || type_ch == 'c'))
                    d += snprintf(d, (size_t)(end - d), "$T%02d", precision);

                /* Emit padding if width specified */
                if (has_width && width > 0) {
                    if (flag_zero && !flag_minus &&
                        strchr("diouxX", type_ch))
                        d += snprintf(d, (size_t)(end - d), "$l%02d0", width);
                    else if (flag_minus)
                        d += snprintf(d, (size_t)(end - d), "$R%02d", width);
                    else
                        d += snprintf(d, (size_t)(end - d), "$L%02d", width);
                }

                char slot;
                if (param_idx < 9)
                    slot = (char)('1' + param_idx);
                else
                    slot = (char)('A' + param_idx - 9);
                *d++ = '|';
                *d++ = '!';
                *d++ = slot;

                /* No type suffix emitted — all callers standardized on
                 * pre-formatted const char * strings.  LangVsprintf
                 * defaults to va_arg(ap, const char *) when no suffix
                 * is present, which is the correct behavior. */

                param_idx++;
            }
        } else {
            /* Not a recognized format specifier — copy literally */
            while (spec_start < s && d < end)
                *d++ = *spec_start++;
        }
    }
    *d = '\0';
}

/* ========================================================================== */
/* AVATAR → MCI conversion                                                     */
/* ========================================================================== */

/**
 * @brief Convert raw bytes containing AVATAR sequences to MCI string.
 *
 * Scans for AVATAR control sequences (0x16 prefix) and converts:
 * - Color attrs (0x16 0x01 NN) → |00–|23 pipe color codes
 * - Blink       (0x16 0x02)    → |24
 * - Cursor up   (0x16 0x03)    → |[A01
 * - Cursor down (0x16 0x04)    → |[B01
 * - Cursor left (0x16 0x05)    → |[D01
 * - Cursor right(0x16 0x06)    → |[C01
 * - Clear EOL   (0x16 0x07)    → |[K
 * - Goto(r,c)   (0x16 0x08 R C)→ |[Y##|[X##
 * - CLS         (0x0c)         → |CL
 * - Repeat char (0x19 ch count) → $D##C (MCI duplicate)
 *
 * All other bytes are passed through, with non-printable bytes
 * escaped as \\xHH (now parsed by libmaxcfg).
 */
static void lc_avatar_to_mci(const unsigned char *raw, size_t raw_len,
                              char *output, size_t out_sz)
{
    char *d = output;
    char *end = output + out_sz - 1;
    const unsigned char *s = raw;
    const unsigned char *s_end = raw + raw_len;

    while (s < s_end && d < end) {
        if (*s == 0x16 && s + 1 < s_end) {
            unsigned char cmd = s[1];
            switch (cmd) {
            case 0x01: /* Color attribute */
                if (s + 2 < s_end) {
                    if (s[2] == '%') {
                        /* Dynamic color: attribute byte is a printf format
                         * specifier (e.g. %c).  Emit it as-is so the
                         * printf-to-positional pass converts it to |!N.
                         * At runtime: LangPrintf Pass 1 expands |!N to a
                         * pipe color string (e.g. "|05"), then MCI Pass 2
                         * interprets it as a color code. */
                        s += 2; /* skip \x16\x01, leave %spec for passthrough */
                        const unsigned char *fs = s + 1; /* past '%' */
                        while (*fs == '-' || *fs == '+' || *fs == '0' ||
                               *fs == ' ' || *fs == '#') fs++;
                        while (*fs >= '0' && *fs <= '9') fs++;
                        if (*fs == '.') { fs++; while (*fs >= '0' && *fs <= '9') fs++; }
                        if (*fs == 'h') { fs++; if (*fs == 'h') fs++; }
                        else if (*fs == 'l') { fs++; if (*fs == 'l') fs++; }
                        else if (*fs == 'L' || *fs == 'z') fs++;
                        if (*fs && strchr("diouxXcsnp", *fs)) {
                            fs++; /* include type char */
                            while (s < fs && d < end) *d++ = *s++;
                        }
                    } else {
                        unsigned char attr = s[2];
                        int fg = attr & 0x0F;
                        int bg = (attr >> 4) & 0x07;
                        bool blink = (attr & 0x80) != 0;

                        /* Emit foreground color code */
                        d += snprintf(d, (size_t)(end - d), "|%02d", fg);

                        /* Emit background if non-zero */
                        if (bg > 0 && d < end) {
                            d += snprintf(d, (size_t)(end - d), "|%02d", 16 + bg);
                        }

                        /* Emit blink if set */
                        if (blink && d < end) {
                            d += snprintf(d, (size_t)(end - d), "|24");
                        }

                        s += 3;
                    }
                } else {
                    s++;
                }
                break;

            case 0x02: /* Blink standalone */
                d += snprintf(d, (size_t)(end - d), "|24");
                s += 2;
                break;

            case 0x03: /* Cursor up */
                d += snprintf(d, (size_t)(end - d), "|[A01");
                s += 2;
                break;

            case 0x04: /* Cursor down */
                d += snprintf(d, (size_t)(end - d), "|[B01");
                s += 2;
                break;

            case 0x05: /* Cursor left */
                d += snprintf(d, (size_t)(end - d), "|[D01");
                s += 2;
                break;

            case 0x06: /* Cursor right */
                d += snprintf(d, (size_t)(end - d), "|[C01");
                s += 2;
                break;

            case 0x07: /* Clear to EOL */
                d += snprintf(d, (size_t)(end - d), "|[K");
                s += 2;
                break;

            case 0x08: /* Goto(row, col) */
                if (s + 3 < s_end) {
                    unsigned char row = s[2];
                    unsigned char col = s[3];
                    d += snprintf(d, (size_t)(end - d), "|[Y%02d|[X%02d",
                                  (int)row, (int)col);
                    s += 4;
                } else {
                    s += 2;
                }
                break;

            default:
                /* Unknown AVATAR command — escape both bytes */
                d += snprintf(d, (size_t)(end - d), "\\x%02x\\x%02x",
                              (unsigned)*s, (unsigned)cmd);
                s += 2;
                break;
            }
        } else if (*s == 0x0c) {
            /* CLS → |CL MCI code (emits ANSI ESC[2J ESC[H at runtime) */
            d += snprintf(d, (size_t)(end - d), "|CL");
            s++;
        } else if (*s == 0x19 && s + 2 < s_end) {
            /* Standalone AVATAR RLE: 0x19 <char> <count>
             * If the count byte is '%' (0x25), the count is a printf format
             * specifier — emit $D%s<char> so the subsequent printf-to-
             * positional pass converts %s to |!N (string type).
             *
             * The legacy .mad source used %c here (raw byte count), but the
             * MCI $D##C operator expects two ASCII decimal digits.  All C
             * callers already snprintf the count as "%02d" into a char[]
             * buffer and pass a char* — so %s is the correct type.
             *
             * At runtime: LangPrintf Pass 1 expands |!N→"37", then
             * MCI Pass 2 processes $D37─ correctly. */
            unsigned char rle_ch = s[1];
            if (s[2] == '%') {
                /* Emit $D prefix then %s (always string — the caller
                 * pre-formats the count as a zero-padded 2-digit string) */
                d += snprintf(d, (size_t)(end - d), "$D");
                s += 2; /* skip 0x19 + char; leave %spec for passthrough */
                /* Scan past the original format spec so we can skip it,
                 * then emit %s in its place. */
                const unsigned char *fs = s; /* points at '%' */
                fs++; /* skip '%' */
                /* Skip flags */
                while (*fs == '-' || *fs == '+' || *fs == '0' ||
                       *fs == ' ' || *fs == '#') fs++;
                /* Skip width */
                while (*fs >= '0' && *fs <= '9') fs++;
                /* Skip precision */
                if (*fs == '.') { fs++; while (*fs >= '0' && *fs <= '9') fs++; }
                /* Skip length modifier */
                if (*fs == 'h') { fs++; if (*fs == 'h') fs++; }
                else if (*fs == 'l') { fs++; if (*fs == 'l') fs++; }
                else if (*fs == 'L' || *fs == 'z') fs++;
                /* Replace the original format spec (e.g. "%c") with "%s" */
                if (*fs && strchr("diouxXcsnp", *fs)) {
                    fs++; /* skip past type char */
                    s = fs; /* advance past the original spec */
                    if (d + 2 <= end) { *d++ = '%'; *d++ = 's'; }
                }
                /* Now emit the RLE character (TOML-safe) */
                if (rle_ch >= 0x20 && rle_ch != '"' && rle_ch != '\\')
                    *d++ = (char)rle_ch;
                else
                    d += snprintf(d, (size_t)(end - d), "\\x%02x", (unsigned)rle_ch);
            } else {
                /* Static count — emit $D##C directly */
                int rle_count = (int)s[2];
                d += snprintf(d, (size_t)(end - d), "$D%02d%c",
                              rle_count, (char)rle_ch);
                s += 3;
            }
        } else if (*s == '\n') {
            /* Newline → TOML escape */
            if (d + 1 < end) { *d++ = '\\'; *d++ = 'n'; }
            s++;
        } else if (*s == '\r') {
            if (d + 1 < end) { *d++ = '\\'; *d++ = 'r'; }
            s++;
        } else if (*s == '\a') {
            if (d + 1 < end) { *d++ = '\\'; *d++ = 'a'; }
            s++;
        } else if (*s == '\t') {
            if (d + 1 < end) { *d++ = '\\'; *d++ = 't'; }
            s++;
        } else if (*s == '"') {
            if (d + 1 < end) { *d++ = '\\'; *d++ = '"'; }
            s++;
        } else if (*s == '\\') {
            if (d + 1 < end) { *d++ = '\\'; *d++ = '\\'; }
            s++;
        } else if (*s < 0x20 && *s != '\n' && *s != '\r' && *s != '\t') {
            /* Non-printable control char → hex escape */
            d += snprintf(d, (size_t)(end - d), "\\x%02x", (unsigned)*s);
            s++;
        } else {
            *d++ = (char)*s++;
        }
    }
    *d = '\0';
}

/* ========================================================================== */
/* Main line parser                                                            */
/* ========================================================================== */

/**
 * @brief Parse a single logical line from the .mad file.
 *
 * Handles: comments, #define, #include, heap sections (:name / =name),
 * @ flags, and string definitions (symbol= "text";).
 */
static bool lc_process_line(LcState *st, char *line)
{
    strip_trailing(line);
    strip_leading(line);

    /* Empty or comment */
    if (!*line || *line == ';')
        return true;

    /* #define */
    if (line[0] == '#') {
        char keyword[32] = {0};
        sscanf(line + 1, "%31s", keyword);

        if (strcasecmp(keyword, "define") == 0) {
            /* Expand macros in the define value first */
            char expanded[LC_MAX_CONTENT];
            lc_expand_macros(st, line, expanded, sizeof(expanded));
            return lc_add_define(st, expanded);
        }
        if (strcasecmp(keyword, "include") == 0) {
            return lc_handle_include(st, line);
        }
        /* Other directives ignored */
        return true;
    }

    /* Heap section: ":heapname" or "=heapname" (user heap) */
    if (*line == ':' || *line == '=') {
        bool is_user = (*line == '=');

        if (st->num_heaps >= LC_MAX_HEAPS) {
            snprintf(st->err, sizeof(st->err), "Too many heaps (max %d)", LC_MAX_HEAPS);
            return false;
        }

        LcHeap *h = &st->heaps[st->num_heaps];
        memset(h, 0, sizeof(*h));
        strncpy(h->name, line + 1, LC_MAX_VARNAME - 1);
        h->is_user_heap = is_user;
        h->base_id = st->global_string_id;
        st->num_heaps++;
        return true;
    }

    /* Parse @ flags before the string definition */
    LcFlags flags = {0};
    bool skip_platform = false;

    while (*line == '@') {
        if (starts_with_ci(line + 1, "MEX", 3)) {
            flags.mex = true;
            line += 4;
        } else if (starts_with_ci(line + 1, "RIP", 3) ||
                   starts_with_ci(line + 1, "ALT", 3)) {
            flags.rip = true;
            line += 4;
        } else if (starts_with_ci(line + 1, "UNIX", 4)) {
            /* Our platform — accept */
            line += 5;
        } else if (starts_with_ci(line + 1, "DOS", 3) ||
                   starts_with_ci(line + 1, "OS2", 3)) {
            skip_platform = true;
            line += 4;
        } else if (starts_with_ci(line + 1, "NT", 2)) {
            skip_platform = true;
            line += 3;
        } else {
            /* Unknown @ flag, skip past it */
            line++;
            while (*line && !isspace(*line) && *line != '@') line++;
        }
        while (*line && isspace(*line)) line++;
    }

    /* If this string is for a platform we don't support, skip it */
    if (skip_platform)
        return true;

    /* Must be a string definition: symbol= "content"; */
    if (!*line)
        return true;

    /* Extract symbol name */
    char symbol[LC_MAX_VARNAME] = {0};
    const char *p = line;
    char *sd = symbol;
    while (*p && (is_id_char(*p) || *p == '$' || *p == '#')) {
        if (sd < symbol + LC_MAX_VARNAME - 1)
            *sd++ = *p;
        p++;
    }
    *sd = '\0';

    if (!*symbol)
        return true;

    /* Strip trailing $ and # modifiers from symbol name */
    size_t slen = strlen(symbol);
    if (slen > 0 && (symbol[slen - 1] == '$' || symbol[slen - 1] == '#'))
        symbol[slen - 1] = '\0';

    /* Skip whitespace and '=' */
    while (*p && (isspace(*p) || *p == '=')) p++;

    /* Extract string content (with macro expansion) */
    char raw_content[LC_MAX_CONTENT];
    lc_get_var_contents(st, p, raw_content, sizeof(raw_content));

    /* Process backslash escapes to raw bytes */
    unsigned char raw_bytes[LC_MAX_CONTENT];
    size_t raw_len = 0;
    lc_process_backslashes(raw_content, raw_bytes, sizeof(raw_bytes), &raw_len);

    /* Convert AVATAR sequences to MCI codes */
    char mci_raw[LC_MAX_CONTENT];
    lc_avatar_to_mci(raw_bytes, raw_len, mci_raw, sizeof(mci_raw));

    /* Convert printf format specifiers to |!N positional params (Round 2) */
    char mci_text[LC_MAX_CONTENT];
    lc_printf_to_positional(mci_raw, mci_text, sizeof(mci_text));

    /* Ensure we have at least one heap */
    if (st->num_heaps == 0) {
        snprintf(st->err, sizeof(st->err),
                 "String '%s' defined before any heap section", symbol);
        return false;
    }

    LcHeap *h = &st->heaps[st->num_heaps - 1];

    if (flags.rip) {
        /* RIP alternate — attach to the most recent string with this symbol */
        for (int i = h->count - 1; i >= 0; i--) {
            if (strcmp(h->strings[i].symbol, symbol) == 0) {
                free(h->strings[i].rip_text);
                h->strings[i].rip_text = strdup(mci_text);
                break;
            }
        }
    } else {
        /* Regular string */
        if (h->count >= LC_MAX_STRINGS) {
            snprintf(st->err, sizeof(st->err),
                     "Too many strings in heap '%s' (max %d)", h->name, LC_MAX_STRINGS);
            return false;
        }

        LcString *ls = &h->strings[h->count++];
        strncpy(ls->symbol, symbol, LC_MAX_VARNAME - 1);
        ls->text = strdup(mci_text);
        ls->rip_text = NULL;
        ls->has_mex = flags.mex || h->is_user_heap;
        ls->legacy_id = st->global_string_id++;
    }

    return true;
}

/* ========================================================================== */
/* File parser                                                                 */
/* ========================================================================== */

/**
 * @brief Read and parse a .mad file, handling includes recursively.
 */
static bool lc_parse_file(LcState *st, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(st->err, sizeof(st->err), "Cannot open: %s", path);
        return false;
    }

    /* Set base directory from the root file */
    if (st->include_depth < 0) {
        char tmp[LC_MAX_PATH];
        strncpy(tmp, path, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';
        char *dir = dirname(tmp);
        strncpy(st->base_dir, dir, sizeof(st->base_dir) - 1);
    }

    st->include_depth = 0;
    st->include_stack[0] = f;
    st->include_names[0] = strdup(path);
    st->include_lines[0] = 0;

    char line[LC_MAX_LINE];

    while (st->include_depth >= 0) {
        FILE *cur = st->include_stack[st->include_depth];

        if (!fgets(line, sizeof(line), cur)) {
            /* EOF on current file — pop include stack */
            fclose(cur);
            free(st->include_names[st->include_depth]);
            st->include_names[st->include_depth] = NULL;
            st->include_depth--;
            continue;
        }

        st->include_lines[st->include_depth]++;
        strip_trailing(line);
        strip_leading(line);

        /* Skip empty / comment lines early */
        if (!*line || *line == ';')
            continue;

        /* Handle multi-line string continuation (lines not ending with ';'
         * and not starting with '#', ':', '=') */
        if (*line != '#' && *line != ':' && *line != '=') {
            /* Check if this is a string definition that needs continuation */
            size_t len = strlen(line);
            while (len > 0 && line[len - 1] != ';' && *line != ':' && *line != '=') {
                /* Append next line */
                if (len + 1 < sizeof(line)) {
                    line[len++] = ' ';
                }
                if (!fgets(line + len, (int)(sizeof(line) - len), cur)) {
                    break;
                }
                st->include_lines[st->include_depth]++;
                strip_trailing(line);
                len = strlen(line);
            }
        }

        if (!lc_process_line(st, line)) {
            return false;
        }
    }

    return true;
}

/* ========================================================================== */
/* TOML writer                                                                 */
/* ========================================================================== */

/**
 * @brief Write the parsed language data as a TOML file.
 */
static bool lc_write_toml(LcState *st, const char *out_path)
{
    FILE *f = fopen(out_path, "w");
    if (!f) {
        snprintf(st->err, sizeof(st->err), "Cannot create output file: %s", out_path);
        return false;
    }

    /* Meta header */
    fprintf(f, "# Language file converted from .MAD format by maxcfg\n");
    fprintf(f, "# Do not edit the [_legacy_map] section manually.\n\n");
    fprintf(f, "[meta]\n");

    /* Derive language name from output filename */
    char tmp[LC_MAX_PATH];
    strncpy(tmp, out_path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    char *base = basename(tmp);
    char *dot = strrchr(base, '.');
    if (dot) *dot = '\0';
    fprintf(f, "name = \"%s\"\n", base);
    fprintf(f, "version = 1\n");

    /* Write each heap */
    for (int hi = 0; hi < st->num_heaps; hi++) {
        LcHeap *h = &st->heaps[hi];

        /* Skip the sentinel "end" heap if it has no strings */
        if (strcasecmp(h->name, "end") == 0 && h->count == 0)
            continue;

        fprintf(f, "\n[%s]\n", h->name);

        if (h->is_user_heap) {
            fprintf(f, "_user_heap = true\n");
        }

        for (int si = 0; si < h->count; si++) {
            LcString *ls = &h->strings[si];
            bool needs_table = (ls->has_mex || ls->rip_text);

            if (needs_table) {
                /* Inline table form for strings with flags/rip */
                fprintf(f, "%s = { text = \"%s\"", ls->symbol, ls->text);

                if (ls->has_mex) {
                    fprintf(f, ", flags = [\"mex\"]");
                }

                if (ls->rip_text) {
                    fprintf(f, ", rip = \"%s\"", ls->rip_text);
                }

                fprintf(f, " }\n");
            } else {
                /* Simple key = "value" form */
                fprintf(f, "%s = \"%s\"\n", ls->symbol, ls->text);
            }
        }
    }

    /* Write legacy map */
    fprintf(f, "\n[_legacy_map]\n");
    fprintf(f, "# Maps old s_ret() numeric IDs to heap.symbol keys\n");

    for (int hi = 0; hi < st->num_heaps; hi++) {
        LcHeap *h = &st->heaps[hi];
        if (strcasecmp(h->name, "end") == 0 && h->count == 0)
            continue;

        for (int si = 0; si < h->count; si++) {
            LcString *ls = &h->strings[si];
            fprintf(f, "\"0x%04x\" = \"%s.%s\"\n", ls->legacy_id, h->name, ls->symbol);
        }
    }

    fclose(f);
    return true;
}

/* ========================================================================== */
/* Cleanup                                                                     */
/* ========================================================================== */

static void lc_cleanup(LcState *st)
{
    /* Free defines */
    for (int i = 0; i < st->num_defines; i++) {
        free(st->defines[i].name);
        free(st->defines[i].value);
    }

    /* Free string data */
    for (int hi = 0; hi < st->num_heaps; hi++) {
        for (int si = 0; si < st->heaps[hi].count; si++) {
            free(st->heaps[hi].strings[si].text);
            free(st->heaps[hi].strings[si].rip_text);
        }
    }

    /* Close any open include files */
    for (int i = 0; i <= st->include_depth && i < LC_MAX_INCLUDES; i++) {
        if (st->include_stack[i]) {
            fclose(st->include_stack[i]);
            st->include_stack[i] = NULL;
        }
        free(st->include_names[i]);
        st->include_names[i] = NULL;
    }
}

/* ========================================================================== */
/* Delta merge helpers                                                         */
/* ========================================================================== */

/**
 * @brief Skip past a TOML value (string, array, inline table) respecting nesting.
 *
 * @param s  Pointer to start of value.
 * @return Pointer to character after the value.
 */
static const char *lc_skip_toml_value(const char *s)
{
    while (*s == ' ' || *s == '\t') s++;

    if (*s == '"') {
        /* Quoted string — skip to closing quote, handling escapes */
        s++;
        while (*s) {
            if (*s == '\\') { s++; if (*s) s++; continue; }
            if (*s == '"') { s++; return s; }
            s++;
        }
    } else if (*s == '[' || *s == '{') {
        /* Array or inline table — track nesting */
        char open = *s, close = (*s == '[') ? ']' : '}';
        int depth = 1;
        s++;
        while (*s && depth > 0) {
            if (*s == '"') {
                s++;
                while (*s && *s != '"') {
                    if (*s == '\\') { s++; if (*s) s++; continue; }
                    s++;
                }
                if (*s == '"') s++;
            } else {
                if (*s == open) depth++;
                else if (*s == close) depth--;
                if (depth > 0) s++;
                else { s++; return s; }
            }
        }
    } else {
        /* Number, boolean, etc — skip to comma, }, or end */
        while (*s && *s != ',' && *s != '}' && *s != ']') s++;
    }
    return s;
}

/**
 * @brief Merge a delta inline table into a base TOML line (shallow field merge).
 *
 * When the delta value is an inline table `{ ... }`, merges its fields into
 * the base line rather than replacing it entirely:
 *   - Base is `"..."` → wraps as `{ text = <string>, <delta_fields> }`
 *   - Base is `{ ... }` → appends new delta fields, replaces existing ones
 *
 * @param base_line   The existing base TOML line (e.g. `key = "hello"`).
 * @param delta_line  The delta TOML line (e.g. `key = { params = [...] }`).
 * @return malloc'd merged line, or NULL if merge not possible.
 */
static char *lc_delta_merge_line(const char *base_line, const char *delta_line)
{
    /* Locate value portion of delta line — must be inline table */
    const char *deq = strchr(delta_line, '=');
    if (!deq) return NULL;
    const char *dv = deq + 1;
    while (*dv == ' ' || *dv == '\t') dv++;
    if (*dv != '{') return NULL;

    /* Extract delta inner content (between { and last }) */
    const char *d_open = dv + 1;
    while (*d_open == ' ' || *d_open == '\t') d_open++;
    const char *d_close = strrchr(dv, '}');
    if (!d_close || d_close <= d_open) return NULL;
    const char *d_end = d_close;
    while (d_end > d_open && (d_end[-1] == ' ' || d_end[-1] == '\t' || d_end[-1] == ','))
        d_end--;
    size_t d_inner_len = (size_t)(d_end - d_open);

    /* Extract key from base line */
    const char *bp = base_line;
    while (*bp == ' ' || *bp == '\t') bp++;
    const char *beq = strchr(bp, '=');
    if (!beq) return NULL;
    const char *key_end = beq;
    while (key_end > bp && (key_end[-1] == ' ' || key_end[-1] == '\t')) key_end--;
    size_t key_len = (size_t)(key_end - bp);

    /* Locate base value */
    const char *bv = beq + 1;
    while (*bv == ' ' || *bv == '\t') bv++;

    char result[LC_MAX_CONTENT];

    if (*bv == '{') {
        /* Base is already an inline table.
         * Extract base inner, then for each delta field check if it exists
         * in base: replace if so, append if not. */
        const char *b_close = strrchr(bv, '}');
        if (!b_close) return NULL;

        /* Copy base inner into working buffer */
        const char *b_inner = bv + 1;
        while (*b_inner == ' ' || *b_inner == '\t') b_inner++;
        size_t b_inner_len = (size_t)(b_close - b_inner);
        char base_inner[LC_MAX_CONTENT];
        if (b_inner_len >= sizeof(base_inner)) return NULL;
        memcpy(base_inner, b_inner, b_inner_len);
        base_inner[b_inner_len] = '\0';
        /* Trim trailing whitespace/commas */
        while (b_inner_len > 0 &&
               (base_inner[b_inner_len-1] == ' ' || base_inner[b_inner_len-1] == '\t' ||
                base_inner[b_inner_len-1] == ','))
            base_inner[--b_inner_len] = '\0';

        /* Walk delta fields; collect fields to append */
        char append_buf[LC_MAX_CONTENT] = "";
        size_t append_len = 0;
        const char *dp = d_open;

        while (dp < d_end) {
            while (dp < d_end && (*dp == ' ' || *dp == '\t' || *dp == ',')) dp++;
            if (dp >= d_end) break;

            /* Extract field name */
            const char *fname = dp;
            while (dp < d_end && *dp != '=' && *dp != ' ' && *dp != '\t') dp++;
            size_t fname_len = (size_t)(dp - fname);

            /* Skip to value */
            while (dp < d_end && (*dp == ' ' || *dp == '\t')) dp++;
            if (dp < d_end && *dp == '=') dp++;
            while (dp < d_end && (*dp == ' ' || *dp == '\t')) dp++;

            /* Capture the full value */
            const char *fval = dp;
            dp = lc_skip_toml_value(dp);
            size_t fval_len = (size_t)(dp - fval);

            /* Build "name = value" string for this field */
            char field_str[2048];
            int fs_len = snprintf(field_str, sizeof(field_str), "%.*s = %.*s",
                                  (int)fname_len, fname, (int)fval_len, fval);
            if (fs_len < 0 || (size_t)fs_len >= sizeof(field_str)) continue;

            /* Check if this field name exists in base inner.
             * Simple check: find "fname =" or "fname=" in base_inner. */
            bool found_in_base = false;
            char needle[256];
            snprintf(needle, sizeof(needle), "%.*s", (int)fname_len, fname);
            const char *bp2 = base_inner;
            while (*bp2) {
                while (*bp2 == ' ' || *bp2 == '\t' || *bp2 == ',') bp2++;
                if (!*bp2) break;
                /* Check if current field matches */
                if (strncmp(bp2, needle, fname_len) == 0) {
                    const char *after = bp2 + fname_len;
                    while (*after == ' ' || *after == '\t') after++;
                    if (*after == '=') { found_in_base = true; break; }
                }
                /* Skip this field's value to move to next */
                while (*bp2 && *bp2 != '=') bp2++;
                if (*bp2 == '=') bp2++;
                while (*bp2 == ' ' || *bp2 == '\t') bp2++;
                bp2 = lc_skip_toml_value(bp2);
            }

            if (!found_in_base) {
                /* Append this field */
                if (append_len > 0 && append_len + 2 < sizeof(append_buf)) {
                    append_buf[append_len++] = ',';
                    append_buf[append_len++] = ' ';
                }
                if (append_len + (size_t)fs_len < sizeof(append_buf) - 1) {
                    memcpy(append_buf + append_len, field_str, (size_t)fs_len);
                    append_len += (size_t)fs_len;
                    append_buf[append_len] = '\0';
                }
            }
            /* Note: field replacement in base_inner is not implemented here.
             * For now, delta fields that already exist in base are skipped
             * (base value preserved). Full field replacement can be added if needed. */
        }

        if (append_len > 0)
            snprintf(result, sizeof(result), "%.*s = { %s, %s }",
                     (int)key_len, bp, base_inner, append_buf);
        else
            snprintf(result, sizeof(result), "%.*s = { %s }",
                     (int)key_len, bp, base_inner);

    } else if (*bv == '"') {
        /* Base is a simple string — wrap as { text = <string>, <delta_fields> } */
        const char *str_end = lc_skip_toml_value(bv);
        size_t str_len = (size_t)(str_end - bv);

        snprintf(result, sizeof(result), "%.*s = { text = %.*s, %.*s }",
                 (int)key_len, bp,
                 (int)str_len, bv,
                 (int)d_inner_len, d_open);
    } else {
        return NULL;
    }

    return strdup(result);
}

/* ========================================================================== */
/* Public API                                                                  */
/* ========================================================================== */

/**
 * @brief Apply a delta overlay to an existing TOML language file.
 *
 * Reads the base .toml and applies changes from the delta file according to
 * the specified mode.  Delta lines before any [maximusng-*] section are Tier 1
 * (@merge param metadata).  Lines inside [maximusng-*] sections are Tier 2
 * (theme color overrides).
 *
 * - LANG_DELTA_FULL:       Apply both tiers.
 * - LANG_DELTA_MERGE_ONLY: Apply Tier 1 only (preserves user colors).
 * - LANG_DELTA_NG_ONLY:    Apply Tier 2 only (theme on enriched file).
 */
bool lang_apply_delta(const char *toml_path,
                      const char *delta_path,
                      LangDeltaMode mode,
                      char *err,
                      size_t err_len)
{
    if (!toml_path || !*toml_path) {
        set_err(err, err_len, "No TOML file path specified for delta apply");
        return false;
    }

    /* Resolve delta path if not provided explicitly */
    char resolved_delta[LC_MAX_PATH];
    if (delta_path && *delta_path) {
        strncpy(resolved_delta, delta_path, sizeof(resolved_delta) - 1);
        resolved_delta[sizeof(resolved_delta) - 1] = '\0';
    } else {
        /* Auto-detect: delta_<basename>.toml in same directory as toml_path */
        char tmp[LC_MAX_PATH];
        strncpy(tmp, toml_path, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';

        char *base = basename(tmp);
        char name_only[LC_MAX_VARNAME];
        char *dot = strrchr(base, '.');
        if (dot) {
            size_t nlen = (size_t)(dot - base);
            if (nlen >= sizeof(name_only)) nlen = sizeof(name_only) - 1;
            memcpy(name_only, base, nlen);
            name_only[nlen] = '\0';
        } else {
            strncpy(name_only, base, sizeof(name_only) - 1);
            name_only[sizeof(name_only) - 1] = '\0';
        }

        char dir_buf[LC_MAX_PATH];
        strncpy(dir_buf, toml_path, sizeof(dir_buf) - 1);
        dir_buf[sizeof(dir_buf) - 1] = '\0';
        char *dir = dirname(dir_buf);
        snprintf(resolved_delta, sizeof(resolved_delta),
                 "%s/delta_%s.toml", dir, name_only);
    }

    FILE *df = fopen(resolved_delta, "r");
    if (!df) {
        /* No delta file is not an error — nothing to apply */
        return true;
    }

    /* Read the base TOML into memory */
    FILE *bf = fopen(toml_path, "r");
    if (!bf) {
        fclose(df);
        set_err(err, err_len, "Cannot open base file: %s", toml_path);
        return false;
    }

    char **lines = NULL;
    int line_count = 0, line_cap = 0;
    char lbuf[4096];

    while (fgets(lbuf, (int)sizeof(lbuf), bf)) {
        size_t ln = strlen(lbuf);
        if (ln > 0 && lbuf[ln - 1] == '\n') lbuf[--ln] = '\0';
        if (ln > 0 && lbuf[ln - 1] == '\r') lbuf[--ln] = '\0';

        if (line_count >= line_cap) {
            line_cap = line_cap ? line_cap * 2 : 256;
            lines = realloc(lines, (size_t)line_cap * sizeof(char *));
        }
        lines[line_count++] = strdup(lbuf);
    }
    fclose(bf);

    /* Process each delta line with section-aware tier tracking.
     *   - Lines before any [maximusng-*] section = Tier 1
     *   - Lines inside [maximusng-*] sections    = Tier 2 */
    bool merge_next = false;
    bool in_ng_section = false;

    while (fgets(lbuf, (int)sizeof(lbuf), df)) {
        size_t ln = strlen(lbuf);
        if (ln > 0 && lbuf[ln - 1] == '\n') lbuf[--ln] = '\0';
        if (ln > 0 && lbuf[ln - 1] == '\r') lbuf[--ln] = '\0';

        const char *p = lbuf;
        while (*p == ' ' || *p == '\t') p++;

        /* Blank lines reset nothing, just skip */
        if (*p == '\0')
            continue;

        /* Section headers: track whether we're in a [maximusng-*] tier */
        if (*p == '[') {
            in_ng_section = (strncmp(p, "[maximusng-", 11) == 0);
            merge_next = false;
            continue;
        }

        /* Comments: check for @merge directive */
        if (*p == '#') {
            if (strstr(p, "@merge"))
                merge_next = true;
            continue;
        }

        /* Tier filtering based on mode */
        if (in_ng_section && mode == LANG_DELTA_MERGE_ONLY) {
            merge_next = false;
            continue;   /* Skip Tier 2 entries in merge-only mode */
        }
        if (!in_ng_section && mode == LANG_DELTA_NG_ONLY) {
            merge_next = false;
            continue;   /* Skip Tier 1 entries in ng-only mode */
        }

        /* Extract key: everything before '=' */
        const char *eq = strstr(p, " =");
        if (!eq) eq = strchr(p, '=');
        if (!eq) { merge_next = false; continue; }

        size_t key_len = (size_t)(eq - p);
        while (key_len > 0 && (p[key_len - 1] == ' ' || p[key_len - 1] == '\t'))
            key_len--;

        /* Search for existing key in base lines */
        int found = -1;
        for (int i = 0; i < line_count; i++) {
            const char *bl = lines[i];
            while (*bl == ' ' || *bl == '\t') bl++;
            if (strncmp(bl, p, key_len) == 0) {
                const char *after = bl + key_len;
                while (*after == ' ' || *after == '\t') after++;
                if (*after == '=') {
                    found = i;
                    break;
                }
            }
        }

        if (found >= 0) {
            if (merge_next) {
                /* Shallow merge: append delta fields to base */
                char *merged = lc_delta_merge_line(lines[found], lbuf);
                if (merged) {
                    free(lines[found]);
                    lines[found] = merged;
                }
                /* If merge fails, base line is preserved unchanged */
            } else {
                /* Full replacement */
                free(lines[found]);
                lines[found] = strdup(lbuf);
            }
        } else {
            /* Insert before [_legacy_map] or at end */
            int insert_at = line_count;
            for (int i = 0; i < line_count; i++) {
                if (strncmp(lines[i], "[_legacy_map]", 13) == 0) {
                    insert_at = i;
                    break;
                }
            }
            if (line_count >= line_cap) {
                line_cap = line_cap ? line_cap * 2 : 256;
                lines = realloc(lines, (size_t)line_cap * sizeof(char *));
            }
            memmove(&lines[insert_at + 1], &lines[insert_at],
                    (size_t)(line_count - insert_at) * sizeof(char *));
            lines[insert_at] = strdup(lbuf);
            line_count++;
        }
        merge_next = false;
    }
    fclose(df);

    /* Write merged result back */
    FILE *of = fopen(toml_path, "w");
    if (!of) {
        for (int i = 0; i < line_count; i++) free(lines[i]);
        free(lines);
        set_err(err, err_len, "Cannot write to: %s", toml_path);
        return false;
    }
    for (int i = 0; i < line_count; i++)
        fprintf(of, "%s\n", lines[i]);
    fclose(of);

    for (int i = 0; i < line_count; i++) free(lines[i]);
    free(lines);
    return true;
}

bool lang_convert_mad_to_toml(const char *mad_path,
                              const char *out_dir,
                              LangDeltaMode mode,
                              char *err,
                              size_t err_len)
{
    if (!mad_path || !*mad_path) {
        set_err(err, err_len, "No .MAD file path specified");
        return false;
    }

    LcState *st = calloc(1, sizeof(LcState));
    if (!st) {
        set_err(err, err_len, "Out of memory");
        return false;
    }
    st->include_depth = -1;

    /* Parse the .mad file */
    if (!lc_parse_file(st, mad_path)) {
        set_err(err, err_len, "%s", st->err);
        lc_cleanup(st);
        free(st);
        return false;
    }

    /* Build output path */
    char out_path[LC_MAX_PATH];
    char tmp[LC_MAX_PATH];
    strncpy(tmp, mad_path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    char *base = basename(tmp);
    char *dot = strrchr(base, '.');
    char name_only[LC_MAX_VARNAME];
    if (dot) {
        size_t nlen = (size_t)(dot - base);
        if (nlen >= sizeof(name_only)) nlen = sizeof(name_only) - 1;
        memcpy(name_only, base, nlen);
        name_only[nlen] = '\0';
    } else {
        strncpy(name_only, base, sizeof(name_only) - 1);
        name_only[sizeof(name_only) - 1] = '\0';
    }

    if (out_dir && *out_dir) {
        snprintf(out_path, sizeof(out_path), "%s/%s.toml", out_dir, name_only);
    } else {
        /* Same directory as input file */
        char dir_tmp[LC_MAX_PATH];
        strncpy(dir_tmp, mad_path, sizeof(dir_tmp) - 1);
        dir_tmp[sizeof(dir_tmp) - 1] = '\0';
        char *dir = dirname(dir_tmp);
        snprintf(out_path, sizeof(out_path), "%s/%s.toml", dir, name_only);
    }

    /* Write TOML */
    bool ok = lc_write_toml(st, out_path);
    if (!ok) {
        set_err(err, err_len, "%s", st->err);
        lc_cleanup(st);
        free(st);
        return false;
    }

    /* Apply delta overlay if delta_<name>.toml exists.
     * Search order: output directory first, then .mad input directory.
     * The delta apply function handles all tier filtering via mode. */
    {
        char delta_path[LC_MAX_PATH];
        bool found_delta = false;

        /* Try output directory first (co-located with the .toml result) */
        char out_dir_buf[LC_MAX_PATH];
        strncpy(out_dir_buf, out_path, sizeof(out_dir_buf) - 1);
        out_dir_buf[sizeof(out_dir_buf) - 1] = '\0';
        char *odir = dirname(out_dir_buf);
        snprintf(delta_path, sizeof(delta_path), "%s/delta_%s.toml", odir, name_only);

        FILE *test = fopen(delta_path, "r");
        if (test) {
            fclose(test);
            found_delta = true;
        } else {
            /* Fall back to .mad input directory */
            char dir_buf[LC_MAX_PATH];
            strncpy(dir_buf, mad_path, sizeof(dir_buf) - 1);
            dir_buf[sizeof(dir_buf) - 1] = '\0';
            char *dir = dirname(dir_buf);
            snprintf(delta_path, sizeof(delta_path), "%s/delta_%s.toml", dir, name_only);
            test = fopen(delta_path, "r");
            if (test) {
                fclose(test);
                found_delta = true;
            }
        }

        if (found_delta) {
            char delta_err[512] = {0};
            if (!lang_apply_delta(out_path, delta_path, mode,
                                  delta_err, sizeof(delta_err))) {
                /* Delta failure is non-fatal for conversion — warn but continue */
                if (err && err_len > 0)
                    set_err(err, err_len, "Warning: delta apply failed: %s",
                            delta_err);
            }
        }
    }

    lc_cleanup(st);
    free(st);
    return ok;
}

int lang_convert_all_mad(const char *lang_dir,
                         const char *out_dir,
                         LangDeltaMode mode,
                         char *err,
                         size_t err_len)
{
    if (!lang_dir || !*lang_dir) {
        set_err(err, err_len, "No language directory specified");
        return -1;
    }

    DIR *d = opendir(lang_dir);
    if (!d) {
        set_err(err, err_len, "Cannot open directory: %s", lang_dir);
        return -1;
    }

    int converted = 0;
    struct dirent *ent;

    while ((ent = readdir(d)) != NULL) {
        /* Look for *.mad files */
        const char *name = ent->d_name;
        size_t nlen = strlen(name);
        if (nlen < 5) continue;
        if (strcasecmp(name + nlen - 4, ".mad") != 0) continue;

        char full_path[LC_MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", lang_dir, name);

        char file_err[512] = {0};
        if (lang_convert_mad_to_toml(full_path,
                                     out_dir ? out_dir : lang_dir,
                                     mode, file_err, sizeof(file_err))) {
            converted++;
        } else {
            /* Report the first error but continue with other files */
            if (err && err_len > 0 && err[0] == '\0') {
                snprintf(err, err_len, "%s: %s", name, file_err);
            }
        }
    }

    closedir(d);
    return converted;
}
