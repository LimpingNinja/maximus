#include "libmaxcfg.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

static int ng_stricmp(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;

    if (a == NULL) a = "";
    if (b == NULL) b = "";

    for (;;) {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        ca = (unsigned char)tolower(ca);
        cb = (unsigned char)tolower(cb);
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
        if (ca == '\0') {
            return 0;
        }
    }
}

int maxcfg_dos_color_from_name(const char *s)
{
    char buf[64];
    size_t i;
    size_t j;

    if (s == NULL) {
        return -1;
    }

    j = 0;
    for (i = 0; s[i] && j + 1 < sizeof(buf); i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == ' ' || c == '\t' || c == '_' || c == '-') {
            continue;
        }
        buf[j++] = (char)tolower(c);
    }
    buf[j] = '\0';

    if (strcmp(buf, "black") == 0) return 0;
    if (strcmp(buf, "blue") == 0) return 1;
    if (strcmp(buf, "green") == 0) return 2;
    if (strcmp(buf, "cyan") == 0) return 3;
    if (strcmp(buf, "red") == 0) return 4;
    if (strcmp(buf, "magenta") == 0) return 5;
    if (strcmp(buf, "brown") == 0) return 6;
    if (strcmp(buf, "lightgray") == 0 || strcmp(buf, "lightgrey") == 0) return 7;
    if (strcmp(buf, "darkgray") == 0 || strcmp(buf, "darkgrey") == 0) return 8;
    if (strcmp(buf, "lightblue") == 0) return 9;
    if (strcmp(buf, "lightgreen") == 0) return 10;
    if (strcmp(buf, "lightcyan") == 0) return 11;
    if (strcmp(buf, "lightred") == 0) return 12;
    if (strcmp(buf, "lightmagenta") == 0) return 13;
    if (strcmp(buf, "yellow") == 0) return 14;
    if (strcmp(buf, "white") == 0) return 15;

    return -1;
}

const char *maxcfg_dos_color_to_name(int color)
{
    static const char *names[16] = {
        "Black",
        "Blue",
        "Green",
        "Cyan",
        "Red",
        "Magenta",
        "Brown",
        "Light Gray",
        "Dark Gray",
        "Light Blue",
        "Light Green",
        "Light Cyan",
        "Light Red",
        "Light Magenta",
        "Yellow",
        "White",
    };

    if (color < 0 || color > 15) {
        return "";
    }
    return names[color];
}

unsigned char maxcfg_make_attr(int fg, int bg)
{
    return (unsigned char)((fg & 0x0f) | ((bg & 0x0f) << 4));
}

int maxcfg_abi_version(void)
{
    return LIBMAXCFG_ABI_VERSION;
}

static MaxCfgStatus ng_tbl_get_bool_default(const MaxCfgVar *tbl, const char *key, bool *out, bool def)
{
    if (out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    *out = def;
    if (tbl == NULL || key == NULL || key[0] == '\0') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    MaxCfgVar v;
    MaxCfgStatus st = maxcfg_toml_table_get(tbl, key, &v);
    if (st == MAXCFG_ERR_NOT_FOUND) {
        return MAXCFG_OK;
    }
    if (st != MAXCFG_OK) {
        return st;
    }
    if (v.type != MAXCFG_VAR_BOOL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    *out = v.v.b ? true : false;
    return MAXCFG_OK;
}

static MaxCfgStatus ng_tbl_get_int_array_view(const MaxCfgVar *tbl, const char *key, MaxCfgIntView *out)
{
    if (out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    out->items = NULL;
    out->count = 0;
    if (tbl == NULL || key == NULL || key[0] == '\0') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    MaxCfgVar v;
    MaxCfgStatus st = maxcfg_toml_table_get(tbl, key, &v);
    if (st == MAXCFG_ERR_NOT_FOUND) {
        return MAXCFG_OK;
    }
    if (st != MAXCFG_OK) {
        return st;
    }
    if (v.type != MAXCFG_VAR_INT_ARRAY) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    out->items = v.v.intv.items;
    out->count = v.v.intv.count;
    return MAXCFG_OK;
}

#ifndef FLOW_TXOFF
#define FLOW_TXOFF 0x01
#endif
#ifndef FLOW_CTS
#define FLOW_CTS 0x02
#endif
#ifndef FLOW_DSR
#define FLOW_DSR 0x04
#endif
#ifndef FLOW_RXOFF
#define FLOW_RXOFF 0x08
#endif

#ifndef VIDEO_DOS
#define VIDEO_DOS 0x00
#endif
#ifndef VIDEO_FOSSIL
#define VIDEO_FOSSIL 0x01
#endif
#ifndef VIDEO_IBM
#define VIDEO_IBM 0x02
#endif
#ifndef VIDEO_FAST
#define VIDEO_FAST 0x03
#endif
#ifndef VIDEO_BIOS
#define VIDEO_BIOS 0x04
#endif

#ifndef LOG_TERSE
#define LOG_TERSE 0x02
#endif
#ifndef LOG_VERBOSE
#define LOG_VERBOSE 0x04
#endif
#ifndef LOG_TRACE
#define LOG_TRACE 0x06
#endif

#ifndef MULTITASKER_AUTO
#define MULTITASKER_AUTO -1
#endif
#ifndef MULTITASKER_NONE
#define MULTITASKER_NONE 0
#endif
#ifndef MULTITASKER_DOUBLEDOS
#define MULTITASKER_DOUBLEDOS 1
#endif
#ifndef MULTITASKER_DESQVIEW
#define MULTITASKER_DESQVIEW 2
#endif
#ifndef MULTITASKER_TOPVIEW
#define MULTITASKER_TOPVIEW 3
#endif
#ifndef MULTITASKER_MLINK
#define MULTITASKER_MLINK 4
#endif
#ifndef MULTITASKER_MSWINDOWS
#define MULTITASKER_MSWINDOWS 5
#endif
#ifndef MULTITASKER_OS2
#define MULTITASKER_OS2 6
#endif
#ifndef MULTITASKER_PCMOS
#define MULTITASKER_PCMOS 7
#endif
#ifndef MULTITASKER_NT
#define MULTITASKER_NT 8
#endif
#ifndef MULTITASKER_UNIX
#define MULTITASKER_UNIX 9
#endif

#ifndef NLVER_5
#define NLVER_5 5
#endif
#ifndef NLVER_6
#define NLVER_6 6
#endif
#ifndef NLVER_7
#define NLVER_7 7
#endif
#ifndef NLVER_FD
#define NLVER_FD 32
#endif

#ifndef CHARSET_SWEDISH
#define CHARSET_SWEDISH 0x01
#endif
#ifndef CHARSET_CHINESE
#define CHARSET_CHINESE 0x02
#endif

#ifndef FLAG2_has_snow
#define FLAG2_has_snow 0x0002
#endif

struct MaxCfg {
    char *base_dir;
};

typedef struct TomlNode TomlNode;

typedef struct {
    char *key;
    TomlNode *value;
} TomlTableEntry;

typedef struct {
    TomlTableEntry *items;
    size_t count;
    size_t capacity;
} TomlTable;

typedef struct {
    TomlNode **items;
    size_t count;
    size_t capacity;
} TomlArray;

typedef struct {
    char *path;
    char *prefix;
} TomlLoadedFile;

struct TomlNode {
    MaxCfgVarType type;
    union {
        int i;
        unsigned int u;
        bool b;
        char *s;
        struct {
            char **items;
            size_t count;
        } strv;
        struct {
            int *items;
            size_t count;
        } intv;
        TomlTable *table;
        TomlArray *array;
    } v;
};

struct MaxCfgToml {
    TomlNode *root;
    TomlTable *overrides;
    TomlLoadedFile *loaded_files;
    size_t loaded_file_count;
    size_t loaded_file_capacity;
};

static MaxCfgStatus parse_segment(const char *seg, char *name_out, size_t name_out_sz, bool *has_index, size_t *index_out);
static TomlNode *toml_node_clone(const TomlNode *src);
static void toml_kv_string(FILE *fp, const char *key, const char *value);
static void toml_kv_int(FILE *fp, const char *key, int value);
static void toml_kv_bool(FILE *fp, const char *key, bool value);
static void toml_kv_uint(FILE *fp, const char *key, unsigned int value);
static void toml_kv_string_array(FILE *fp, const char *key, char **items, size_t count);
static void toml_kv_int_array(FILE *fp, const char *key, int *items, size_t count);

static void maxcfg_free_and_null(char **p)
{
    if (p == NULL || *p == NULL) {
        return;
    }

    free(*p);
    *p = NULL;
}

static void maxcfg_free_loaded_files(MaxCfgToml *toml)
{
    if (toml == NULL || toml->loaded_files == NULL) {
        if (toml) {
            toml->loaded_file_count = 0;
            toml->loaded_file_capacity = 0;
        }
        return;
    }

    for (size_t i = 0; i < toml->loaded_file_count; i++) {
        free(toml->loaded_files[i].path);
        free(toml->loaded_files[i].prefix);
    }
    free(toml->loaded_files);
    toml->loaded_files = NULL;
    toml->loaded_file_count = 0;
    toml->loaded_file_capacity = 0;
}

static MaxCfgStatus maxcfg_loaded_files_ensure_capacity(MaxCfgToml *toml, size_t want)
{
    if (toml == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    if (want <= toml->loaded_file_capacity) {
        return MAXCFG_OK;
    }

    size_t newcap = toml->loaded_file_capacity ? toml->loaded_file_capacity : 8u;
    while (newcap < want) {
        newcap *= 2u;
    }

    TomlLoadedFile *p = (TomlLoadedFile *)realloc(toml->loaded_files, newcap * sizeof(*p));
    if (p == NULL) {
        return MAXCFG_ERR_OOM;
    }
    toml->loaded_files = p;
    toml->loaded_file_capacity = newcap;
    return MAXCFG_OK;
}

static bool maxcfg_path_is_absolute(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return false;
    }

    if (path[0] == '/' || path[0] == '\\') {
        return true;
    }

    if (isalpha((unsigned char)path[0]) && path[1] == ':') {
        return true;
    }

    return false;
}

MaxCfgStatus maxcfg_resolve_path(const char *base_dir,
                                const char *path,
                                char *out_path,
                                size_t out_path_size)
{
    if (out_path == NULL || out_path_size == 0u || path == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    out_path[0] = '\0';

    if (maxcfg_path_is_absolute(path)) {
        size_t n = strlen(path);
        if (n + 1u > out_path_size) {
            return MAXCFG_ERR_PATH_TOO_LONG;
        }
        memcpy(out_path, path, n + 1u);
        return MAXCFG_OK;
    }

    if (base_dir == NULL || base_dir[0] == '\0') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    size_t b = strlen(base_dir);
    size_t p = strlen(path);
    bool need_sep = true;

    if (b > 0u) {
        char last = base_dir[b - 1u];
        if (last == '/' || last == '\\') {
            need_sep = false;
        }
    }

    size_t want = b + (need_sep ? 1u : 0u) + p + 1u;
    if (want > out_path_size) {
        return MAXCFG_ERR_PATH_TOO_LONG;
    }

    memcpy(out_path, base_dir, b);
    size_t o = b;
    if (need_sep) {
        out_path[o++] = '/';
    }
    memcpy(out_path + o, path, p);
    out_path[o + p] = '\0';
    return MAXCFG_OK;
}

static bool maxcfg_str_eq_ci(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }
    for (;; a++, b++) {
        unsigned char ca = (unsigned char)*a;
        unsigned char cb = (unsigned char)*b;
        ca = (unsigned char)tolower(ca);
        cb = (unsigned char)tolower(cb);
        if (ca != cb) {
            return false;
        }
        if (ca == '\0') {
            return true;
        }
    }
}

MaxCfgStatus maxcfg_ng_parse_video_mode(const char *s, int *out_video, bool *out_has_snow)
{
    bool snow = false;
    int video;

    if (s == NULL || out_video == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (maxcfg_str_eq_ci(s, "bios")) {
        video = VIDEO_BIOS;
    } else if (maxcfg_str_eq_ci(s, "ibm")) {
        video = VIDEO_IBM;
    } else if (maxcfg_str_eq_ci(s, "ibm/snow")) {
        video = VIDEO_IBM;
        snow = true;
    } else if (maxcfg_str_eq_ci(s, "dos")) {
        video = VIDEO_DOS;
    } else if (maxcfg_str_eq_ci(s, "fast")) {
        video = VIDEO_FAST;
    } else if (maxcfg_str_eq_ci(s, "fossil")) {
        video = VIDEO_FOSSIL;
    } else {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    *out_video = video;
    if (out_has_snow != NULL) {
        *out_has_snow = snow;
    }
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_parse_log_mode(const char *s, int *out_log_mode)
{
    if (s == NULL || out_log_mode == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (maxcfg_str_eq_ci(s, "terse")) {
        *out_log_mode = LOG_TERSE;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "verbose")) {
        *out_log_mode = LOG_VERBOSE;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "trace")) {
        *out_log_mode = LOG_TRACE;
        return MAXCFG_OK;
    }
    if (isdigit((unsigned char)s[0])) {
        *out_log_mode = atoi(s);
        return MAXCFG_OK;
    }

    return MAXCFG_ERR_INVALID_ARGUMENT;
}

MaxCfgStatus maxcfg_ng_parse_multitasker(const char *s, int *out_multitasker)
{
    if (s == NULL || out_multitasker == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (maxcfg_str_eq_ci(s, "none")) {
        *out_multitasker = MULTITASKER_NONE;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "auto")) {
        *out_multitasker = MULTITASKER_AUTO;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "doubledos")) {
        *out_multitasker = MULTITASKER_DOUBLEDOS;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "desqview")) {
        *out_multitasker = MULTITASKER_DESQVIEW;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "topview")) {
        *out_multitasker = MULTITASKER_TOPVIEW;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "multilink") || maxcfg_str_eq_ci(s, "mlink")) {
        *out_multitasker = MULTITASKER_MLINK;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "mswindows")) {
        *out_multitasker = MULTITASKER_MSWINDOWS;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "pc-mos") || maxcfg_str_eq_ci(s, "pcmos")) {
        *out_multitasker = MULTITASKER_PCMOS;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "os/2") || maxcfg_str_eq_ci(s, "os2")) {
        *out_multitasker = MULTITASKER_OS2;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "nt")) {
        *out_multitasker = MULTITASKER_NT;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "unix")) {
        *out_multitasker = MULTITASKER_UNIX;
        return MAXCFG_OK;
    }
    if (isdigit((unsigned char)s[0]) || (s[0] == '-' && isdigit((unsigned char)s[1]))) {
        *out_multitasker = atoi(s);
        return MAXCFG_OK;
    }

    return MAXCFG_ERR_INVALID_ARGUMENT;
}

MaxCfgStatus maxcfg_ng_parse_handshaking_token(const char *s, int *out_bits)
{
    if (s == NULL || out_bits == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (maxcfg_str_eq_ci(s, "xon")) {
        *out_bits = FLOW_TXOFF;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "cts")) {
        *out_bits = FLOW_CTS;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "dsr")) {
        *out_bits = FLOW_DSR;
        return MAXCFG_OK;
    }

    return MAXCFG_ERR_INVALID_ARGUMENT;
}

MaxCfgStatus maxcfg_ng_parse_charset(const char *s, int *out_charset, bool *out_global_high_bit)
{
    if (s == NULL || out_charset == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (maxcfg_str_eq_ci(s, "swedish")) {
        *out_charset = CHARSET_SWEDISH;
        if (out_global_high_bit != NULL) {
            *out_global_high_bit = false;
        }
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "chinese")) {
        *out_charset = CHARSET_CHINESE;
        if (out_global_high_bit != NULL) {
            *out_global_high_bit = true;
        }
        return MAXCFG_OK;
    }
    if (s[0] == '\0') {
        *out_charset = 0;
        if (out_global_high_bit != NULL) {
            *out_global_high_bit = false;
        }
        return MAXCFG_OK;
    }

    return MAXCFG_ERR_INVALID_ARGUMENT;
}

MaxCfgStatus maxcfg_ng_parse_nodelist_version(const char *s, int *out_nlver)
{
    if (s == NULL || out_nlver == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (strcmp(s, "5") == 0) {
        *out_nlver = NLVER_5;
        return MAXCFG_OK;
    }
    if (strcmp(s, "6") == 0) {
        *out_nlver = NLVER_6;
        return MAXCFG_OK;
    }
    if (strcmp(s, "7") == 0) {
        *out_nlver = NLVER_7;
        return MAXCFG_OK;
    }
    if (maxcfg_str_eq_ci(s, "fd")) {
        *out_nlver = NLVER_FD;
        return MAXCFG_OK;
    }
    if (s[0] == '\0') {
        *out_nlver = 0;
        return MAXCFG_OK;
    }

    return MAXCFG_ERR_INVALID_ARGUMENT;
}

MaxCfgStatus maxcfg_ng_get_video_mode(const MaxCfgToml *toml, int *out_video, bool *out_has_snow)
{
    MaxCfgVar v;
    bool got_any = false;

    if (toml == NULL || out_video == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (maxcfg_toml_get(toml, "maximus.video", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s) {
        MaxCfgStatus st = maxcfg_ng_parse_video_mode(v.v.s, out_video, out_has_snow);
        if (st != MAXCFG_OK) {
            return st;
        }
        got_any = true;
    }

    if (out_has_snow != NULL) {
        if (maxcfg_toml_get(toml, "maximus.has_snow", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_BOOL) {
            got_any = true;
            *out_has_snow = v.v.b;
        }
    }

    return got_any ? MAXCFG_OK : MAXCFG_ERR_NOT_FOUND;
}

MaxCfgStatus maxcfg_ng_get_log_mode(const MaxCfgToml *toml, int *out_log_mode)
{
    MaxCfgVar v;

    if (toml == NULL || out_log_mode == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (maxcfg_toml_get(toml, "maximus.log_mode", &v) != MAXCFG_OK) {
        return MAXCFG_ERR_NOT_FOUND;
    }

    if (v.type == MAXCFG_VAR_INT) {
        *out_log_mode = v.v.i;
        return MAXCFG_OK;
    }
    if (v.type != MAXCFG_VAR_STRING || v.v.s == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    return maxcfg_ng_parse_log_mode(v.v.s, out_log_mode);
}

MaxCfgStatus maxcfg_ng_get_multitasker(const MaxCfgToml *toml, int *out_multitasker)
{
    MaxCfgVar v;

    if (toml == NULL || out_multitasker == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (maxcfg_toml_get(toml, "maximus.multitasker", &v) != MAXCFG_OK) {
        return MAXCFG_ERR_NOT_FOUND;
    }

    if (v.type == MAXCFG_VAR_INT) {
        *out_multitasker = v.v.i;
        return MAXCFG_OK;
    }
    if (v.type != MAXCFG_VAR_STRING || v.v.s == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    return maxcfg_ng_parse_multitasker(v.v.s, out_multitasker);
}

MaxCfgStatus maxcfg_ng_get_handshake_mask(const MaxCfgToml *toml, int *out_mask)
{
    MaxCfgVar v;
    MaxCfgVar it;
    int mask = 0;
    size_t cnt = 0;

    if (toml == NULL || out_mask == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (maxcfg_toml_get(toml, "general.equipment.handshaking", &v) != MAXCFG_OK) {
        return MAXCFG_ERR_NOT_FOUND;
    }

    if (v.type != MAXCFG_VAR_STRING_ARRAY) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (maxcfg_var_count(&v, &cnt) != MAXCFG_OK) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < cnt; i++) {
        if (maxcfg_toml_array_get(&v, i, &it) != MAXCFG_OK || it.type != MAXCFG_VAR_STRING || it.v.s == NULL) {
            continue;
        }
        int bits = 0;
        if (maxcfg_ng_parse_handshaking_token(it.v.s, &bits) != MAXCFG_OK) {
            return MAXCFG_ERR_INVALID_ARGUMENT;
        }
        mask |= bits;
    }

    *out_mask = mask;
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_get_charset(const MaxCfgToml *toml, int *out_charset)
{
    MaxCfgVar v;

    if (toml == NULL || out_charset == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (maxcfg_toml_get(toml, "general.session.charset", &v) != MAXCFG_OK) {
        return MAXCFG_ERR_NOT_FOUND;
    }

    if (v.type == MAXCFG_VAR_INT) {
        *out_charset = v.v.i;
        return MAXCFG_OK;
    }
    if (v.type != MAXCFG_VAR_STRING || v.v.s == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    return maxcfg_ng_parse_charset(v.v.s, out_charset, NULL);
}

MaxCfgStatus maxcfg_ng_get_nodelist_version(const MaxCfgToml *toml, int *out_nlver)
{
    MaxCfgVar v;

    if (toml == NULL || out_nlver == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (maxcfg_toml_get(toml, "matrix.nodelist_version", &v) != MAXCFG_OK) {
        return MAXCFG_ERR_NOT_FOUND;
    }

    if (v.type == MAXCFG_VAR_INT) {
        *out_nlver = v.v.i;
        return MAXCFG_OK;
    }
    if (v.type != MAXCFG_VAR_STRING || v.v.s == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    return maxcfg_ng_parse_nodelist_version(v.v.s, out_nlver);
}

static MaxCfgStatus ng_tbl_get_string_default(const MaxCfgVar *tbl, const char *key, const char **out, const char *def)
{
    if (out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    *out = def ? def : "";
    if (tbl == NULL || key == NULL || key[0] == '\0') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    MaxCfgVar v;
    MaxCfgStatus st = maxcfg_toml_table_get(tbl, key, &v);
    if (st == MAXCFG_ERR_NOT_FOUND) {
        return MAXCFG_OK;
    }
    if (st != MAXCFG_OK) {
        return st;
    }
    if (v.type != MAXCFG_VAR_STRING) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    *out = v.v.s ? v.v.s : "";
    return MAXCFG_OK;
}

static MaxCfgStatus ng_tbl_get_int_default(const MaxCfgVar *tbl, const char *key, int *out, int def)
{
    if (out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    *out = def;
    if (tbl == NULL || key == NULL || key[0] == '\0') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    MaxCfgVar v;
    MaxCfgStatus st = maxcfg_toml_table_get(tbl, key, &v);
    if (st == MAXCFG_ERR_NOT_FOUND) {
        return MAXCFG_OK;
    }
    if (st != MAXCFG_OK) {
        return st;
    }
    if (v.type == MAXCFG_VAR_INT) {
        *out = v.v.i;
        return MAXCFG_OK;
    }
    if (v.type == MAXCFG_VAR_UINT) {
        *out = (int)v.v.u;
        return MAXCFG_OK;
    }
    return MAXCFG_ERR_INVALID_ARGUMENT;
}

static MaxCfgStatus ng_tbl_get_string_array_view(const MaxCfgVar *tbl, const char *key, MaxCfgStrView *out)
{
    if (out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    out->items = NULL;
    out->count = 0;
    if (tbl == NULL || key == NULL || key[0] == '\0') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    MaxCfgVar v;
    MaxCfgStatus st = maxcfg_toml_table_get(tbl, key, &v);
    if (st == MAXCFG_ERR_NOT_FOUND) {
        return MAXCFG_OK;
    }
    if (st != MAXCFG_OK) {
        return st;
    }
    if (v.type != MAXCFG_VAR_STRING_ARRAY) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    out->items = v.v.strv.items;
    out->count = v.v.strv.count;
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_get_msg_areas(const MaxCfgToml *toml, const char *prefix,
                                    MaxCfgNgDivisionList *divisions, MaxCfgNgMsgAreaList *areas)
{
    if (toml == NULL || divisions == NULL || areas == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    const char *pfx = (prefix != NULL) ? prefix : "";
    MaxCfgVar doc;
    MaxCfgStatus st = maxcfg_toml_get(toml, pfx, &doc);
    if (st != MAXCFG_OK) {
        return st;
    }
    if (doc.type != MAXCFG_VAR_TABLE) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    st = maxcfg_ng_division_list_init(divisions);
    if (st != MAXCFG_OK) {
        return st;
    }
    st = maxcfg_ng_msg_area_list_init(areas);
    if (st != MAXCFG_OK) {
        maxcfg_ng_division_list_free(divisions);
        return st;
    }

    MaxCfgVar div_arr;
    if (maxcfg_toml_table_get(&doc, "division", &div_arr) == MAXCFG_OK) {
        if (div_arr.type != MAXCFG_VAR_TABLE_ARRAY) {
            st = MAXCFG_ERR_INVALID_ARGUMENT;
            goto fail_msg_areas;
        }

        size_t n = 0;
        if (maxcfg_var_count(&div_arr, &n) != MAXCFG_OK) {
            st = MAXCFG_ERR_INVALID_ARGUMENT;
            goto fail_msg_areas;
        }

        for (size_t i = 0; i < n; i++) {
            MaxCfgVar it;
            if (maxcfg_toml_array_get(&div_arr, i, &it) != MAXCFG_OK) {
                continue;
            }
            if (it.type != MAXCFG_VAR_TABLE) {
                st = MAXCFG_ERR_INVALID_ARGUMENT;
                goto fail_msg_areas;
            }

            const char *name = "";
            const char *key = "";
            const char *description = "";
            const char *acs = "";
            const char *display_file = "";
            int level = 0;

            st = ng_tbl_get_string_default(&it, "name", &name, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "key", &key, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "description", &description, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "acs", &acs, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "display_file", &display_file, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_int_default(&it, "level", &level, 0);
            if (st != MAXCFG_OK) goto fail_msg_areas;

            MaxCfgNgDivision d = {
                .name = (char *)name,
                .key = (char *)key,
                .description = (char *)description,
                .acs = (char *)acs,
                .display_file = (char *)display_file,
                .level = level
            };
            st = maxcfg_ng_division_list_add(divisions, &d);
            if (st != MAXCFG_OK) goto fail_msg_areas;
        }
    }

    MaxCfgVar area_arr;
    if (maxcfg_toml_table_get(&doc, "area", &area_arr) == MAXCFG_OK) {
        if (area_arr.type != MAXCFG_VAR_TABLE_ARRAY) {
            st = MAXCFG_ERR_INVALID_ARGUMENT;
            goto fail_msg_areas;
        }

        size_t n = 0;
        if (maxcfg_var_count(&area_arr, &n) != MAXCFG_OK) {
            st = MAXCFG_ERR_INVALID_ARGUMENT;
            goto fail_msg_areas;
        }

        for (size_t i = 0; i < n; i++) {
            MaxCfgVar it;
            if (maxcfg_toml_array_get(&area_arr, i, &it) != MAXCFG_OK) {
                continue;
            }
            if (it.type != MAXCFG_VAR_TABLE) {
                st = MAXCFG_ERR_INVALID_ARGUMENT;
                goto fail_msg_areas;
            }

            const char *name = "";
            const char *description = "";
            const char *acs = "";
            const char *menu = "";
            const char *division = "";
            const char *tag = "";
            const char *path = "";
            const char *owner = "";
            const char *origin = "";
            const char *attach_path = "";
            const char *barricade = "";
            int renum_max = 0;
            int renum_days = 0;
            MaxCfgStrView style = {0};

            st = ng_tbl_get_string_default(&it, "name", &name, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "description", &description, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "acs", &acs, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "menu", &menu, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "division", &division, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "tag", &tag, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "path", &path, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "owner", &owner, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "origin", &origin, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "attach_path", &attach_path, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_default(&it, "barricade", &barricade, "");
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_string_array_view(&it, "style", &style);
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_int_default(&it, "renum_max", &renum_max, 0);
            if (st != MAXCFG_OK) goto fail_msg_areas;
            st = ng_tbl_get_int_default(&it, "renum_days", &renum_days, 0);
            if (st != MAXCFG_OK) goto fail_msg_areas;

            MaxCfgNgMsgArea a = {
                .name = (char *)name,
                .description = (char *)description,
                .acs = (char *)acs,
                .menu = (char *)menu,
                .division = (char *)division,
                .tag = (char *)tag,
                .path = (char *)path,
                .owner = (char *)owner,
                .origin = (char *)origin,
                .attach_path = (char *)attach_path,
                .barricade = (char *)barricade,
                .style = (char **)style.items,
                .style_count = style.count,
                .renum_max = renum_max,
                .renum_days = renum_days
            };
            st = maxcfg_ng_msg_area_list_add(areas, &a);
            if (st != MAXCFG_OK) goto fail_msg_areas;
        }
    }

    return MAXCFG_OK;

fail_msg_areas:
    maxcfg_ng_msg_area_list_free(areas);
    maxcfg_ng_division_list_free(divisions);
    return st;
}

MaxCfgStatus maxcfg_ng_get_file_areas(const MaxCfgToml *toml, const char *prefix,
                                     MaxCfgNgDivisionList *divisions, MaxCfgNgFileAreaList *areas)
{
    if (toml == NULL || divisions == NULL || areas == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    const char *pfx = (prefix != NULL) ? prefix : "";
    MaxCfgVar doc;
    MaxCfgStatus st = maxcfg_toml_get(toml, pfx, &doc);
    if (st != MAXCFG_OK) {
        return st;
    }
    if (doc.type != MAXCFG_VAR_TABLE) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    st = maxcfg_ng_division_list_init(divisions);
    if (st != MAXCFG_OK) {
        return st;
    }
    st = maxcfg_ng_file_area_list_init(areas);
    if (st != MAXCFG_OK) {
        maxcfg_ng_division_list_free(divisions);
        return st;
    }

    MaxCfgVar div_arr;
    if (maxcfg_toml_table_get(&doc, "division", &div_arr) == MAXCFG_OK) {
        if (div_arr.type != MAXCFG_VAR_TABLE_ARRAY) {
            st = MAXCFG_ERR_INVALID_ARGUMENT;
            goto fail_file_areas;
        }

        size_t n = 0;
        if (maxcfg_var_count(&div_arr, &n) != MAXCFG_OK) {
            st = MAXCFG_ERR_INVALID_ARGUMENT;
            goto fail_file_areas;
        }

        for (size_t i = 0; i < n; i++) {
            MaxCfgVar it;
            if (maxcfg_toml_array_get(&div_arr, i, &it) != MAXCFG_OK) {
                continue;
            }
            if (it.type != MAXCFG_VAR_TABLE) {
                st = MAXCFG_ERR_INVALID_ARGUMENT;
                goto fail_file_areas;
            }

            const char *name = "";
            const char *key = "";
            const char *description = "";
            const char *acs = "";
            const char *display_file = "";
            int level = 0;

            st = ng_tbl_get_string_default(&it, "name", &name, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_default(&it, "key", &key, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_default(&it, "description", &description, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_default(&it, "acs", &acs, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_default(&it, "display_file", &display_file, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_int_default(&it, "level", &level, 0);
            if (st != MAXCFG_OK) goto fail_file_areas;

            MaxCfgNgDivision d = {
                .name = (char *)name,
                .key = (char *)key,
                .description = (char *)description,
                .acs = (char *)acs,
                .display_file = (char *)display_file,
                .level = level
            };
            st = maxcfg_ng_division_list_add(divisions, &d);
            if (st != MAXCFG_OK) goto fail_file_areas;
        }
    }

    MaxCfgVar area_arr;
    if (maxcfg_toml_table_get(&doc, "area", &area_arr) == MAXCFG_OK) {
        if (area_arr.type != MAXCFG_VAR_TABLE_ARRAY) {
            st = MAXCFG_ERR_INVALID_ARGUMENT;
            goto fail_file_areas;
        }

        size_t n = 0;
        if (maxcfg_var_count(&area_arr, &n) != MAXCFG_OK) {
            st = MAXCFG_ERR_INVALID_ARGUMENT;
            goto fail_file_areas;
        }

        for (size_t i = 0; i < n; i++) {
            MaxCfgVar it;
            if (maxcfg_toml_array_get(&area_arr, i, &it) != MAXCFG_OK) {
                continue;
            }
            if (it.type != MAXCFG_VAR_TABLE) {
                st = MAXCFG_ERR_INVALID_ARGUMENT;
                goto fail_file_areas;
            }

            const char *name = "";
            const char *description = "";
            const char *acs = "";
            const char *menu = "";
            const char *division = "";
            const char *download = "";
            const char *upload = "";
            const char *filelist = "";
            const char *barricade = "";
            MaxCfgStrView types = {0};

            st = ng_tbl_get_string_default(&it, "name", &name, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_default(&it, "description", &description, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_default(&it, "acs", &acs, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_default(&it, "menu", &menu, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_default(&it, "division", &division, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_default(&it, "download", &download, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_default(&it, "upload", &upload, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_default(&it, "filelist", &filelist, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_default(&it, "barricade", &barricade, "");
            if (st != MAXCFG_OK) goto fail_file_areas;
            st = ng_tbl_get_string_array_view(&it, "types", &types);
            if (st != MAXCFG_OK) goto fail_file_areas;

            MaxCfgNgFileArea a = {
                .name = (char *)name,
                .description = (char *)description,
                .acs = (char *)acs,
                .menu = (char *)menu,
                .division = (char *)division,
                .download = (char *)download,
                .upload = (char *)upload,
                .filelist = (char *)filelist,
                .barricade = (char *)barricade,
                .types = (char **)types.items,
                .type_count = types.count
            };
            st = maxcfg_ng_file_area_list_add(areas, &a);
            if (st != MAXCFG_OK) goto fail_file_areas;
        }
    }

    return MAXCFG_OK;

fail_file_areas:
    maxcfg_ng_file_area_list_free(areas);
    maxcfg_ng_division_list_free(divisions);
    return st;
}

static MaxCfgStatus ng_strdup_safe(char **out, const char *in)
{
    if (out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    *out = NULL;
    if (in == NULL) {
        return MAXCFG_OK;
    }

    *out = strdup(in);
    if (*out == NULL) {
        return MAXCFG_ERR_OOM;
    }

    return MAXCFG_OK;
}

static MaxCfgStatus ng_copy_strv_from_view(char ***out_items, size_t *out_count, const MaxCfgStrView *in)
{
    if (out_items == NULL || out_count == NULL || in == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    *out_items = NULL;
    *out_count = 0;

    if (in->count == 0 || in->items == NULL) {
        return MAXCFG_OK;
    }

    char **dst = (char **)calloc(in->count, sizeof(dst[0]));
    if (dst == NULL) {
        return MAXCFG_ERR_OOM;
    }

    for (size_t i = 0; i < in->count; i++) {
        dst[i] = strdup(in->items[i] ? in->items[i] : "");
        if (dst[i] == NULL) {
            for (size_t j = 0; j < i; j++) {
                free(dst[j]);
            }
            free(dst);
            return MAXCFG_ERR_OOM;
        }
    }

    *out_items = dst;
    *out_count = in->count;
    return MAXCFG_OK;
}

static void ng_custom_menu_set_defaults(MaxCfgNgCustomMenu *cm);
static void ng_custom_menu_parse_justify(MaxCfgNgCustomMenu *cm, const char *s);
static void ng_custom_menu_parse_boundary_justify(MaxCfgNgCustomMenu *cm, const char *s);
static void ng_custom_menu_parse_boundary_layout(MaxCfgNgCustomMenu *cm, const char *s);
static void ng_custom_menu_set_location_from_view(int *out_row, int *out_col, const MaxCfgIntView *v);
static void ng_custom_menu_parse_lightbar_color_pair(const MaxCfgVar *tbl, const char *key,
                                                     unsigned char *out_attr, bool *out_has);

MaxCfgStatus maxcfg_ng_get_menu(const MaxCfgToml *toml, const char *prefix, MaxCfgNgMenu *menu)
{
    if (toml == NULL || menu == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    const char *pfx = (prefix != NULL) ? prefix : "";
    MaxCfgVar doc;
    MaxCfgStatus st = maxcfg_toml_get(toml, pfx, &doc);
    if (st != MAXCFG_OK) {
        return st;
    }
    if (doc.type != MAXCFG_VAR_TABLE) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    st = maxcfg_ng_menu_init(menu);
    if (st != MAXCFG_OK) {
        return st;
    }

    const char *name = "";
    const char *title = "";
    const char *header_file = "";
    const char *menu_file = "";
    int menu_length = 0;
    int menu_color = -1;
    int option_width = 0;
    MaxCfgStrView header_types = {0};
    MaxCfgStrView menu_types = {0};

    st = ng_tbl_get_string_default(&doc, "name", &name, "");
    if (st != MAXCFG_OK) goto fail_menu;
    st = ng_tbl_get_string_default(&doc, "title", &title, "");
    if (st != MAXCFG_OK) goto fail_menu;

    st = ng_tbl_get_string_default(&doc, "header_file", &header_file, "");
    if (st != MAXCFG_OK) goto fail_menu;
    st = ng_tbl_get_string_array_view(&doc, "header_types", &header_types);
    if (st != MAXCFG_OK) goto fail_menu;

    st = ng_tbl_get_string_default(&doc, "menu_file", &menu_file, "");
    if (st != MAXCFG_OK) goto fail_menu;
    st = ng_tbl_get_string_array_view(&doc, "menu_types", &menu_types);
    if (st != MAXCFG_OK) goto fail_menu;

    st = ng_tbl_get_int_default(&doc, "menu_length", &menu_length, 0);
    if (st != MAXCFG_OK) goto fail_menu;
    st = ng_tbl_get_int_default(&doc, "menu_color", &menu_color, -1);
    if (st != MAXCFG_OK) goto fail_menu;
    st = ng_tbl_get_int_default(&doc, "option_width", &option_width, 0);
    if (st != MAXCFG_OK) goto fail_menu;

    st = ng_strdup_safe(&menu->name, name);
    if (st != MAXCFG_OK) goto fail_menu;
    st = ng_strdup_safe(&menu->title, title);
    if (st != MAXCFG_OK) goto fail_menu;

    st = ng_strdup_safe(&menu->header_file, header_file);
    if (st != MAXCFG_OK) goto fail_menu;
    st = ng_copy_strv_from_view(&menu->header_types, &menu->header_type_count, &header_types);
    if (st != MAXCFG_OK) goto fail_menu;

    st = ng_strdup_safe(&menu->menu_file, menu_file);
    if (st != MAXCFG_OK) goto fail_menu;
    st = ng_copy_strv_from_view(&menu->menu_types, &menu->menu_type_count, &menu_types);
    if (st != MAXCFG_OK) goto fail_menu;

    menu->menu_length = menu_length;
    menu->menu_color = menu_color;
    menu->option_width = option_width;

    MaxCfgVar cm;
    if (maxcfg_toml_table_get(&doc, "custom_menu", &cm) == MAXCFG_OK && cm.type == MAXCFG_VAR_TABLE) {
        MaxCfgNgCustomMenu *dst = (MaxCfgNgCustomMenu *)calloc(1, sizeof(*dst));
        if (dst == NULL) {
            st = MAXCFG_ERR_OOM;
            goto fail_menu;
        }
        ng_custom_menu_set_defaults(dst);

        (void)ng_tbl_get_bool_default(&cm, "skip_canned_menu", &dst->skip_canned_menu, dst->skip_canned_menu);
        (void)ng_tbl_get_bool_default(&cm, "show_title", &dst->show_title, dst->show_title);
        (void)ng_tbl_get_bool_default(&cm, "lightbar_menu", &dst->lightbar_menu, dst->lightbar_menu);
        (void)ng_tbl_get_bool_default(&cm, "option_spacing", &dst->option_spacing, dst->option_spacing);

        int margin = dst->lightbar_margin;
        if (ng_tbl_get_int_default(&cm, "lightbar_margin", &margin, dst->lightbar_margin) == MAXCFG_OK) {
            if (margin < 0) margin = 0;
            if (margin > 255) margin = 255;
            dst->lightbar_margin = margin;
        }

        const char *justify = "";
        if (ng_tbl_get_string_default(&cm, "option_justify", &justify, "") == MAXCFG_OK) {
            ng_custom_menu_parse_justify(dst, justify);
        }

        const char *bjustify = "";
        if (ng_tbl_get_string_default(&cm, "boundary_justify", &bjustify, "") == MAXCFG_OK) {
            ng_custom_menu_parse_boundary_justify(dst, bjustify);
        }

        const char *layout = "";
        if (ng_tbl_get_string_default(&cm, "boundary_layout", &layout, "") == MAXCFG_OK) {
            ng_custom_menu_parse_boundary_layout(dst, layout);
        }

        MaxCfgIntView loc = {0};
        if (ng_tbl_get_int_array_view(&cm, "top_boundary", &loc) == MAXCFG_OK) {
            ng_custom_menu_set_location_from_view(&dst->top_boundary_row, &dst->top_boundary_col, &loc);
        }
        if (ng_tbl_get_int_array_view(&cm, "bottom_boundary", &loc) == MAXCFG_OK) {
            ng_custom_menu_set_location_from_view(&dst->bottom_boundary_row, &dst->bottom_boundary_col, &loc);
        }
        if (ng_tbl_get_int_array_view(&cm, "title_location", &loc) == MAXCFG_OK) {
            ng_custom_menu_set_location_from_view(&dst->title_location_row, &dst->title_location_col, &loc);
        }
        if (ng_tbl_get_int_array_view(&cm, "prompt_location", &loc) == MAXCFG_OK) {
            ng_custom_menu_set_location_from_view(&dst->prompt_location_row, &dst->prompt_location_col, &loc);
        }

        MaxCfgVar lc;
        if (maxcfg_toml_table_get(&cm, "lightbar_color", &lc) == MAXCFG_OK) {
            if (lc.type == MAXCFG_VAR_TABLE) {
                ng_custom_menu_parse_lightbar_color_pair(&lc, "normal", &dst->lightbar_normal_attr, &dst->has_lightbar_normal);
                ng_custom_menu_parse_lightbar_color_pair(&lc, "selected", &dst->lightbar_selected_attr, &dst->has_lightbar_selected);
                ng_custom_menu_parse_lightbar_color_pair(&lc, "high", &dst->lightbar_high_attr, &dst->has_lightbar_high);
                ng_custom_menu_parse_lightbar_color_pair(&lc, "high_selected", &dst->lightbar_high_selected_attr, &dst->has_lightbar_high_selected);
            } else if (lc.type == MAXCFG_VAR_STRING_ARRAY) {
                MaxCfgStrView sv = lc.v.strv;
                if (sv.count >= 2) {
                    int fg = maxcfg_dos_color_from_name(sv.items[0]);
                    int bg = maxcfg_dos_color_from_name(sv.items[1]);
                    if (fg >= 0 && bg >= 0) {
                        dst->lightbar_selected_attr = maxcfg_make_attr(fg, bg);
                        dst->has_lightbar_selected = true;
                    }
                }
            }
        }

        menu->custom_menu = dst;
    }

    MaxCfgVar opt_arr;
    if (maxcfg_toml_table_get(&doc, "option", &opt_arr) == MAXCFG_OK) {
        if (opt_arr.type != MAXCFG_VAR_TABLE_ARRAY) {
            st = MAXCFG_ERR_INVALID_ARGUMENT;
            goto fail_menu;
        }

        size_t n = 0;
        if (maxcfg_var_count(&opt_arr, &n) != MAXCFG_OK) {
            st = MAXCFG_ERR_INVALID_ARGUMENT;
            goto fail_menu;
        }

        for (size_t i = 0; i < n; i++) {
            MaxCfgVar it;
            if (maxcfg_toml_array_get(&opt_arr, i, &it) != MAXCFG_OK) {
                continue;
            }
            if (it.type != MAXCFG_VAR_TABLE) {
                st = MAXCFG_ERR_INVALID_ARGUMENT;
                goto fail_menu;
            }

            const char *command = "";
            const char *arguments = "";
            const char *priv_level = "";
            const char *description = "";
            const char *key_poke = "";
            MaxCfgStrView modifiers = {0};

            st = ng_tbl_get_string_default(&it, "command", &command, "");
            if (st != MAXCFG_OK) goto fail_menu;
            st = ng_tbl_get_string_default(&it, "arguments", &arguments, "");
            if (st != MAXCFG_OK) goto fail_menu;
            st = ng_tbl_get_string_default(&it, "priv_level", &priv_level, "");
            if (st != MAXCFG_OK) goto fail_menu;
            st = ng_tbl_get_string_default(&it, "description", &description, "");
            if (st != MAXCFG_OK) goto fail_menu;
            st = ng_tbl_get_string_default(&it, "key_poke", &key_poke, "");
            if (st != MAXCFG_OK) goto fail_menu;
            st = ng_tbl_get_string_array_view(&it, "modifiers", &modifiers);
            if (st != MAXCFG_OK) goto fail_menu;

            MaxCfgNgMenuOption opt = {
                .command = (char *)command,
                .arguments = (char *)arguments,
                .priv_level = (char *)priv_level,
                .description = (char *)description,
                .key_poke = (char *)key_poke,
                .modifiers = (char **)modifiers.items,
                .modifier_count = modifiers.count
            };
            st = maxcfg_ng_menu_add_option(menu, &opt);
            if (st != MAXCFG_OK) goto fail_menu;
        }
    }

    return MAXCFG_OK;

fail_menu:
    maxcfg_ng_menu_free(menu);
    return st;
}

static MaxCfgStatus ng_tbl_get_uint_default(const MaxCfgVar *tbl, const char *key, unsigned int *out, unsigned int def)
{
    if (out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    *out = def;
    if (tbl == NULL || key == NULL || key[0] == '\0') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    MaxCfgVar v;
    MaxCfgStatus st = maxcfg_toml_table_get(tbl, key, &v);
    if (st == MAXCFG_ERR_NOT_FOUND) {
        return MAXCFG_OK;
    }
    if (st != MAXCFG_OK) {
        return st;
    }
    if (v.type == MAXCFG_VAR_UINT) {
        *out = v.v.u;
        return MAXCFG_OK;
    }
    if (v.type == MAXCFG_VAR_INT && v.v.i >= 0) {
        *out = (unsigned int)v.v.i;
        return MAXCFG_OK;
    }
    return MAXCFG_ERR_INVALID_ARGUMENT;
}

MaxCfgStatus maxcfg_ng_get_access_levels(const MaxCfgToml *toml, const char *prefix, MaxCfgNgAccessLevelList *levels)
{
    if (toml == NULL || levels == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    const char *pfx = (prefix != NULL) ? prefix : "";
    MaxCfgVar doc;
    MaxCfgStatus st = maxcfg_toml_get(toml, pfx, &doc);
    if (st != MAXCFG_OK) {
        return st;
    }
    if (doc.type != MAXCFG_VAR_TABLE) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    st = maxcfg_ng_access_level_list_init(levels);
    if (st != MAXCFG_OK) {
        return st;
    }

    MaxCfgVar arr;
    if (maxcfg_toml_table_get(&doc, "access_level", &arr) != MAXCFG_OK) {
        return MAXCFG_OK;
    }
    if (arr.type != MAXCFG_VAR_TABLE_ARRAY) {
        maxcfg_ng_access_level_list_free(levels);
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    size_t n = 0;
    if (maxcfg_var_count(&arr, &n) != MAXCFG_OK) {
        maxcfg_ng_access_level_list_free(levels);
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < n; i++) {
        MaxCfgVar it;
        if (maxcfg_toml_array_get(&arr, i, &it) != MAXCFG_OK) {
            continue;
        }
        if (it.type != MAXCFG_VAR_TABLE) {
            maxcfg_ng_access_level_list_free(levels);
            return MAXCFG_ERR_INVALID_ARGUMENT;
        }

        const char *name = "";
        const char *description = "";
        const char *alias = "";
        const char *key = "";
        const char *login_file = "";
        int level = 0;
        int time = 0;
        int cume = 0;
        int calls = 0;
        int logon_baud = 0;
        int xfer_baud = 0;
        int file_limit = 0;
        int file_ratio = 0;
        int ratio_free = 0;
        int upload_reward = 0;
        unsigned int user_flags = 0;
        int oldpriv = 0;
        MaxCfgStrView flags = {0};
        MaxCfgStrView mail_flags = {0};

        st = ng_tbl_get_string_default(&it, "name", &name, "");
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_int_default(&it, "level", &level, 0);
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_string_default(&it, "description", &description, "");
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_string_default(&it, "alias", &alias, "");
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_string_default(&it, "key", &key, "");
        if (st != MAXCFG_OK) goto fail_access_levels;

        st = ng_tbl_get_int_default(&it, "time", &time, 0);
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_int_default(&it, "cume", &cume, 0);
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_int_default(&it, "calls", &calls, 0);
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_int_default(&it, "logon_baud", &logon_baud, 0);
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_int_default(&it, "xfer_baud", &xfer_baud, 0);
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_int_default(&it, "file_limit", &file_limit, 0);
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_int_default(&it, "file_ratio", &file_ratio, 0);
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_int_default(&it, "ratio_free", &ratio_free, 0);
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_int_default(&it, "upload_reward", &upload_reward, 0);
        if (st != MAXCFG_OK) goto fail_access_levels;

        st = ng_tbl_get_string_default(&it, "login_file", &login_file, "");
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_string_array_view(&it, "flags", &flags);
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_string_array_view(&it, "mail_flags", &mail_flags);
        if (st != MAXCFG_OK) goto fail_access_levels;

        st = ng_tbl_get_uint_default(&it, "user_flags", &user_flags, 0u);
        if (st != MAXCFG_OK) goto fail_access_levels;
        st = ng_tbl_get_int_default(&it, "oldpriv", &oldpriv, 0);
        if (st != MAXCFG_OK) goto fail_access_levels;

        MaxCfgNgAccessLevel lvl = {
            .name = (char *)name,
            .level = level,
            .description = (char *)description,
            .alias = (char *)alias,
            .key = (char *)key,
            .time = time,
            .cume = cume,
            .calls = calls,
            .logon_baud = logon_baud,
            .xfer_baud = xfer_baud,
            .file_limit = file_limit,
            .file_ratio = file_ratio,
            .ratio_free = ratio_free,
            .upload_reward = upload_reward,
            .login_file = (char *)login_file,
            .flags = (char **)flags.items,
            .flag_count = flags.count,
            .mail_flags = (char **)mail_flags.items,
            .mail_flag_count = mail_flags.count,
            .user_flags = user_flags,
            .oldpriv = oldpriv
        };
        st = maxcfg_ng_access_level_list_add(levels, &lvl);
        if (st != MAXCFG_OK) goto fail_access_levels;
    }

    return MAXCFG_OK;

fail_access_levels:
    maxcfg_ng_access_level_list_free(levels);
    return st;
}

static MaxCfgStatus maxcfg_strdup_safe(char **out, const char *in)
{
    if (out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    *out = NULL;
    if (in == NULL) {
        return MAXCFG_OK;
    }

    *out = strdup(in);
    if (*out == NULL) {
        return MAXCFG_ERR_OOM;
    }

    return MAXCFG_OK;
}

static TomlNode *toml_node_new(MaxCfgVarType type)
{
    TomlNode *n = (TomlNode *)calloc(1, sizeof(*n));
    if (n == NULL) {
        return NULL;
    }
    n->type = type;
    return n;
}

static void toml_table_free(TomlTable *t);
static void toml_array_free(TomlArray *a);
static void toml_table_clear(TomlTable *t);
static MaxCfgStatus toml_table_unset_node(TomlTable *t, const char *key);

static void toml_node_free(TomlNode *n)
{
    if (n == NULL) {
        return;
    }

    switch (n->type) {
    case MAXCFG_VAR_STRING:
        free(n->v.s);
        break;
    case MAXCFG_VAR_STRING_ARRAY:
        if (n->v.strv.items != NULL) {
            for (size_t i = 0; i < n->v.strv.count; i++) {
                free(n->v.strv.items[i]);
            }
            free(n->v.strv.items);
        }
        break;
    case MAXCFG_VAR_INT_ARRAY:
        free(n->v.intv.items);
        n->v.intv.items = NULL;
        n->v.intv.count = 0;
        break;
    case MAXCFG_VAR_TABLE:
        toml_table_free(n->v.table);
        break;
    case MAXCFG_VAR_TABLE_ARRAY:
        toml_array_free(n->v.array);
        break;
    default:
        break;
    }

    free(n);
}

static void toml_table_free(TomlTable *t)
{
    if (t == NULL) {
        return;
    }

    for (size_t i = 0; i < t->count; i++) {
        free(t->items[i].key);
        toml_node_free(t->items[i].value);
    }
    free(t->items);
    free(t);
}

static void toml_table_clear(TomlTable *t)
{
    if (t == NULL) {
        return;
    }

    for (size_t i = 0; i < t->count; i++) {
        free(t->items[i].key);
        toml_node_free(t->items[i].value);
    }
    free(t->items);
    t->items = NULL;
    t->count = 0;
    t->capacity = 0;
}

static MaxCfgStatus toml_table_unset_node(TomlTable *t, const char *key)
{
    if (t == NULL || key == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < t->count; i++) {
        if (t->items[i].key != NULL && strcmp(t->items[i].key, key) == 0) {
            free(t->items[i].key);
            t->items[i].key = NULL;
            toml_node_free(t->items[i].value);
            t->items[i].value = NULL;

            for (size_t j = i + 1; j < t->count; j++) {
                t->items[j - 1] = t->items[j];
            }
            t->count--;
            return MAXCFG_OK;
        }
    }

    return MAXCFG_ERR_NOT_FOUND;
}

static void toml_array_free(TomlArray *a)
{
    if (a == NULL) {
        return;
    }

    for (size_t i = 0; i < a->count; i++) {
        toml_node_free(a->items[i]);
    }
    free(a->items);
    free(a);
}

/* Forward declarations for internal TOML helpers */
static TomlTable *toml_table_new(void);
static MaxCfgStatus toml_array_ensure_capacity(TomlArray *a, size_t want);

static MaxCfgStatus toml_array_set_count_table(TomlArray *a, size_t count)
{
    if (a == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (toml_array_ensure_capacity(a, count) != MAXCFG_OK) {
        return MAXCFG_ERR_OOM;
    }

    while (a->count > count) {
        toml_node_free(a->items[a->count - 1u]);
        a->items[a->count - 1u] = NULL;
        a->count--;
    }

    while (a->count < count) {
        TomlNode *t = toml_node_new(MAXCFG_VAR_TABLE);
        if (t == NULL) {
            return MAXCFG_ERR_OOM;
        }
        t->v.table = toml_table_new();
        if (t->v.table == NULL) {
            toml_node_free(t);
            return MAXCFG_ERR_OOM;
        }
        a->items[a->count++] = t;
    }

    return MAXCFG_OK;
}

static MaxCfgStatus toml_string_array_set_count(TomlNode *n, size_t count)
{
    if (n == NULL || n->type != MAXCFG_VAR_STRING_ARRAY) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    char **items = n->v.strv.items;
    size_t cur = n->v.strv.count;

    if (count == cur) {
        return MAXCFG_OK;
    }

    if (count < cur) {
        for (size_t i = count; i < cur; i++) {
            free(items[i]);
            items[i] = NULL;
        }
        if (count == 0u) {
            free(items);
            n->v.strv.items = NULL;
            n->v.strv.count = 0u;
            return MAXCFG_OK;
        }
        char **ni = (char **)realloc(items, count * sizeof(*ni));
        if (ni == NULL) {
            return MAXCFG_ERR_OOM;
        }
        n->v.strv.items = ni;
        n->v.strv.count = count;
        return MAXCFG_OK;
    }

    char **ni = (char **)realloc(items, count * sizeof(*ni));
    if (ni == NULL) {
        return MAXCFG_ERR_OOM;
    }
    for (size_t i = cur; i < count; i++) {
        ni[i] = strdup("");
        if (ni[i] == NULL) {
            for (size_t j = cur; j < i; j++) {
                free(ni[j]);
                ni[j] = NULL;
            }
            /* best-effort rollback */
            (void)realloc(ni, cur * sizeof(*ni));
            return MAXCFG_ERR_OOM;
        }
    }
    n->v.strv.items = ni;
    n->v.strv.count = count;
    return MAXCFG_OK;
}

static TomlTable *toml_table_new(void)
{
    return (TomlTable *)calloc(1, sizeof(TomlTable));
}

static TomlArray *toml_array_new(void)
{
    return (TomlArray *)calloc(1, sizeof(TomlArray));
}

static MaxCfgStatus toml_table_ensure_capacity(TomlTable *t, size_t want)
{
    if (t == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (want <= t->capacity) {
        return MAXCFG_OK;
    }

    size_t newcap = t->capacity ? t->capacity : 8u;
    while (newcap < want) {
        newcap *= 2u;
    }

    TomlTableEntry *p = (TomlTableEntry *)realloc(t->items, newcap * sizeof(*p));
    if (p == NULL) {
        return MAXCFG_ERR_OOM;
    }

    t->items = p;
    t->capacity = newcap;
    return MAXCFG_OK;
}

static MaxCfgStatus toml_array_ensure_capacity(TomlArray *a, size_t want)
{
    if (a == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (want <= a->capacity) {
        return MAXCFG_OK;
    }

    size_t newcap = a->capacity ? a->capacity : 8u;
    while (newcap < want) {
        newcap *= 2u;
    }

    TomlNode **p = (TomlNode **)realloc(a->items, newcap * sizeof(*p));
    if (p == NULL) {
        return MAXCFG_ERR_OOM;
    }

    a->items = p;
    a->capacity = newcap;
    return MAXCFG_OK;
}

static TomlNode *toml_table_get_node(const TomlTable *t, const char *key)
{
    if (t == NULL || key == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < t->count; i++) {
        if (t->items[i].key != NULL && strcmp(t->items[i].key, key) == 0) {
            return t->items[i].value;
        }
    }

    return NULL;
}

static MaxCfgStatus toml_table_set_node(TomlTable *t, const char *key, TomlNode *value)
{
    if (t == NULL || key == NULL || value == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < t->count; i++) {
        if (t->items[i].key != NULL && strcmp(t->items[i].key, key) == 0) {
            toml_node_free(t->items[i].value);
            t->items[i].value = value;
            return MAXCFG_OK;
        }
    }

    MaxCfgStatus st = toml_table_ensure_capacity(t, t->count + 1u);
    if (st != MAXCFG_OK) {
        return st;
    }

    t->items[t->count].key = strdup(key);
    if (t->items[t->count].key == NULL) {
        return MAXCFG_ERR_OOM;
    }
    t->items[t->count].value = value;
    t->count++;
    return MAXCFG_OK;
}

static MaxCfgStatus toml_array_add_node(TomlArray *a, TomlNode *value)
{
    if (a == NULL || value == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    MaxCfgStatus st = toml_array_ensure_capacity(a, a->count + 1u);
    if (st != MAXCFG_OK) {
        return st;
    }

    a->items[a->count++] = value;
    return MAXCFG_OK;
}

static const char *skip_ws(const char *p)
{
    while (p && *p && isspace((unsigned char)*p)) {
        p++;
    }
    return p;
}

static char *str_trim_inplace(char *s)
{
    if (s == NULL) {
        return NULL;
    }

    while (*s && isspace((unsigned char)*s)) {
        s++;
    }

    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        end[-1] = '\0';
        end--;
    }

    return s;
}

static char *strip_comment(char *line)
{
    if (line == NULL) {
        return NULL;
    }

    bool in_str = false;
    bool esc = false;
    for (char *p = line; *p; p++) {
        if (in_str) {
            if (esc) {
                esc = false;
                continue;
            }
            if (*p == '\\') {
                esc = true;
                continue;
            }
            if (*p == '"') {
                in_str = false;
            }
            continue;
        }

        if (*p == '"') {
            in_str = true;
            continue;
        }

        if (*p == '#') {
            *p = '\0';
            break;
        }
    }

    return line;
}

static MaxCfgStatus parse_string(const char **p_io, char **out)
{
    const char *p = p_io ? *p_io : NULL;
    if (p == NULL || out == NULL || *p != '"') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    p++;
    size_t cap = 32u;
    size_t len = 0u;
    char *buf = (char *)malloc(cap);
    if (buf == NULL) {
        return MAXCFG_ERR_OOM;
    }

    while (*p) {
        if (*p == '"') {
            p++;
            break;
        }

        char ch = *p++;
        if (ch == '\\') {
            char e = *p++;
            if (e == 'n') {
                ch = '\n';
            } else if (e == 'r') {
                ch = '\r';
            } else if (e == 't') {
                ch = '\t';
            } else if (e == 'a') {
                ch = '\a';
            } else if (e == 'b') {
                ch = '\b';
            } else if (e == 'f') {
                ch = '\f';
            } else if (e == '\\') {
                ch = '\\';
            } else if (e == '"') {
                ch = '"';
            } else if (e == 'x' && isxdigit((unsigned char)p[0])
                                 && isxdigit((unsigned char)p[1])) {
                /* \xHH — two-digit hex byte */
                unsigned int hv = 0;
                for (int hi = 0; hi < 2; hi++) {
                    unsigned char hc = (unsigned char)*p++;
                    hv = (hv << 4) | (unsigned)((hc >= '0' && hc <= '9')
                        ? hc - '0'
                        : (hc >= 'a' && hc <= 'f')
                            ? hc - 'a' + 10
                            : hc - 'A' + 10);
                }
                ch = (char)hv;
            } else {
                ch = e;
            }
        }

        if (len + 1u >= cap) {
            cap *= 2u;
            char *nb = (char *)realloc(buf, cap);
            if (nb == NULL) {
                free(buf);
                return MAXCFG_ERR_OOM;
            }
            buf = nb;
        }
        buf[len++] = ch;
    }

    if (len + 1u >= cap) {
        char *nb = (char *)realloc(buf, len + 1u);
        if (nb == NULL) {
            free(buf);
            return MAXCFG_ERR_OOM;
        }
        buf = nb;
    }

    buf[len] = '\0';
    *out = buf;
    *p_io = p;
    return MAXCFG_OK;
}

static MaxCfgStatus parse_int_array(const char **p_io, TomlNode **out)
{
    /* Parse a TOML array of integers: [1, 2, 3]
     *
     * This is used for fields like custom_menu.top_boundary = [x, y].
     */
    const char *p = p_io ? *p_io : NULL;
    if (p == NULL || out == NULL || *p != '[') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    p++;
    int *items = NULL;
    size_t count = 0u;
    size_t cap = 0u;

    while (*p) {
        p = skip_ws(p);
        if (*p == ']') {
            p++;
            break;
        }

        char *endp = NULL;
        long v = strtol(p, &endp, 10);
        if (endp == p) {
            free(items);
            return MAXCFG_ERR_IO;
        }

        if (count + 1u > cap) {
            size_t ncap = cap ? cap * 2u : 8u;
            int *ni = (int *)realloc(items, ncap * sizeof(*ni));
            if (ni == NULL) {
                free(items);
                return MAXCFG_ERR_OOM;
            }
            items = ni;
            cap = ncap;
        }
        items[count++] = (int)v;
        p = endp;

        p = skip_ws(p);
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == ']') {
            p++;
            break;
        }
    }

    TomlNode *n = toml_node_new(MAXCFG_VAR_INT_ARRAY);
    if (n == NULL) {
        free(items);
        return MAXCFG_ERR_OOM;
    }

    n->v.intv.items = items;
    n->v.intv.count = count;
    *out = n;
    *p_io = p;
    return MAXCFG_OK;
}

static MaxCfgStatus parse_value(const char **p_io, TomlNode **out);

static MaxCfgStatus parse_inline_table(const char **p_io, TomlNode **out)
{
    const char *p = p_io ? *p_io : NULL;
    if (p == NULL || out == NULL || *p != '{') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    p++;
    TomlTable *t = toml_table_new();
    if (t == NULL) {
        return MAXCFG_ERR_OOM;
    }

    while (*p) {
        p = skip_ws(p);
        if (*p == '}') {
            p++;
            break;
        }

        const char *kstart = p;
        while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '-')) {
            p++;
        }
        if (p == kstart) {
            toml_table_free(t);
            return MAXCFG_ERR_IO;
        }

        char key[128];
        size_t klen = (size_t)(p - kstart);
        if (klen >= sizeof(key)) {
            toml_table_free(t);
            return MAXCFG_ERR_IO;
        }
        memcpy(key, kstart, klen);
        key[klen] = '\0';

        p = skip_ws(p);
        if (*p != '=') {
            toml_table_free(t);
            return MAXCFG_ERR_IO;
        }
        p++;
        p = skip_ws(p);

        TomlNode *val = NULL;
        MaxCfgStatus st = parse_value(&p, &val);
        if (st != MAXCFG_OK) {
            toml_table_free(t);
            return st;
        }

        st = toml_table_set_node(t, key, val);
        if (st != MAXCFG_OK) {
            toml_node_free(val);
            toml_table_free(t);
            return st;
        }

        p = skip_ws(p);
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == '}') {
            p++;
            break;
        }
    }

    TomlNode *n = toml_node_new(MAXCFG_VAR_TABLE);
    if (n == NULL) {
        toml_table_free(t);
        return MAXCFG_ERR_OOM;
    }
    n->v.table = t;
    *out = n;
    *p_io = p;
    return MAXCFG_OK;
}

static MaxCfgStatus parse_string_array(const char **p_io, TomlNode **out)
{
    const char *p = p_io ? *p_io : NULL;
    if (p == NULL || out == NULL || *p != '[') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    p++;
    char **items = NULL;
    size_t count = 0u;
    size_t cap = 0u;

    while (*p) {
        p = skip_ws(p);
        if (*p == ']') {
            p++;
            break;
        }

        if (*p != '"') {
            for (size_t i = 0; i < count; i++) {
                free(items[i]);
            }
            free(items);
            return MAXCFG_ERR_IO;
        }

        char *s = NULL;
        MaxCfgStatus st = parse_string(&p, &s);
        if (st != MAXCFG_OK) {
            for (size_t i = 0; i < count; i++) {
                free(items[i]);
            }
            free(items);
            return st;
        }

        if (count + 1u > cap) {
            size_t ncap = cap ? cap * 2u : 8u;
            char **ni = (char **)realloc(items, ncap * sizeof(*ni));
            if (ni == NULL) {
                free(s);
                for (size_t i = 0; i < count; i++) {
                    free(items[i]);
                }
                free(items);
                return MAXCFG_ERR_OOM;
            }
            items = ni;
            cap = ncap;
        }
        items[count++] = s;

        p = skip_ws(p);
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == ']') {
            p++;
            break;
        }
    }

    TomlNode *n = toml_node_new(MAXCFG_VAR_STRING_ARRAY);
    if (n == NULL) {
        for (size_t i = 0; i < count; i++) {
            free(items[i]);
        }
        free(items);
        return MAXCFG_ERR_OOM;
    }

    n->v.strv.items = items;
    n->v.strv.count = count;
    *out = n;
    *p_io = p;
    return MAXCFG_OK;
}

static MaxCfgStatus parse_value(const char **p_io, TomlNode **out)
{
    const char *p = p_io ? *p_io : NULL;
    if (p == NULL || out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    p = skip_ws(p);

    if (*p == '"') {
        char *s = NULL;
        MaxCfgStatus st = parse_string(&p, &s);
        if (st != MAXCFG_OK) {
            return st;
        }
        TomlNode *n = toml_node_new(MAXCFG_VAR_STRING);
        if (n == NULL) {
            free(s);
            return MAXCFG_ERR_OOM;
        }
        n->v.s = s;
        *out = n;
        *p_io = p;
        return MAXCFG_OK;
    }

    if (*p == '[') {
        const char *p2 = p + 1;
        p2 = skip_ws(p2);

        /* Arrays are type-detected from the first non-ws element:
         * - [] and ["..."] => string array
         * - [1,2] or [-1,2] => int array
         */
        if (*p2 == ']') {
            MaxCfgStatus st = parse_string_array(&p, out);
            if (st != MAXCFG_OK) {
                return st;
            }
            *p_io = p;
            return MAXCFG_OK;
        }

        if (*p2 == '"') {
            MaxCfgStatus st = parse_string_array(&p, out);
            if (st != MAXCFG_OK) {
                return st;
            }
            *p_io = p;
            return MAXCFG_OK;
        }

        if (*p2 == '-' || (*p2 >= '0' && *p2 <= '9')) {
            MaxCfgStatus st = parse_int_array(&p, out);
            if (st != MAXCFG_OK) {
                return st;
            }
            *p_io = p;
            return MAXCFG_OK;
        }

        if (*p2 == '{') {
            /* Array of inline tables: [{key = "val"}, {key = "val"}] */
            p++;  /* skip '[' */
            TomlArray *arr = toml_array_new();
            if (arr == NULL) return MAXCFG_ERR_OOM;

            while (*p) {
                p = skip_ws(p);
                if (*p == ']') { p++; break; }

                TomlNode *elem = NULL;
                MaxCfgStatus st = parse_inline_table(&p, &elem);
                if (st != MAXCFG_OK) {
                    toml_array_free(arr);
                    return st;
                }
                /* Re-tag as TABLE_ARRAY element (parse_inline_table returns TABLE) */
                st = toml_array_add_node(arr, elem);
                if (st != MAXCFG_OK) {
                    toml_node_free(elem);
                    toml_array_free(arr);
                    return st;
                }

                p = skip_ws(p);
                if (*p == ',') { p++; continue; }
                if (*p == ']') { p++; break; }
            }

            TomlNode *n = toml_node_new(MAXCFG_VAR_TABLE_ARRAY);
            if (n == NULL) {
                toml_array_free(arr);
                return MAXCFG_ERR_OOM;
            }
            n->v.array = arr;
            *out = n;
            *p_io = p;
            return MAXCFG_OK;
        }

        return MAXCFG_ERR_IO;
    }

    if (*p == '{') {
        MaxCfgStatus st = parse_inline_table(&p, out);
        if (st != MAXCFG_OK) {
            return st;
        }
        *p_io = p;
        return MAXCFG_OK;
    }

    if (strncmp(p, "true", 4) == 0 && !isalnum((unsigned char)p[4]) && p[4] != '_') {
        TomlNode *n = toml_node_new(MAXCFG_VAR_BOOL);
        if (n == NULL) {
            return MAXCFG_ERR_OOM;
        }
        n->v.b = true;
        p += 4;
        *out = n;
        *p_io = p;
        return MAXCFG_OK;
    }

    if (strncmp(p, "false", 5) == 0 && !isalnum((unsigned char)p[5]) && p[5] != '_') {
        TomlNode *n = toml_node_new(MAXCFG_VAR_BOOL);
        if (n == NULL) {
            return MAXCFG_ERR_OOM;
        }
        n->v.b = false;
        p += 5;
        *out = n;
        *p_io = p;
        return MAXCFG_OK;
    }

    char *endp = NULL;
    long v = strtol(p, &endp, 10);
    if (endp != p) {
        TomlNode *n = toml_node_new(MAXCFG_VAR_INT);
        if (n == NULL) {
            return MAXCFG_ERR_OOM;
        }
        n->v.i = (int)v;
        p = endp;
        *out = n;
        *p_io = p;
        return MAXCFG_OK;
    }

    return MAXCFG_ERR_IO;
}

static MaxCfgStatus table_get_or_create_table(TomlTable *root, const char *path, TomlTable **out)
{
    if (root == NULL || path == NULL || out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    char segbuf[256];

    *out = root;
    const char *p = path;
    while (*p) {
        const char *seg = p;
        while (*p && *p != '.') {
            p++;
        }
        size_t len = (size_t)(p - seg);
        if (len == 0u || len >= sizeof(segbuf)) {
            return MAXCFG_ERR_INVALID_ARGUMENT;
        }
        memcpy(segbuf, seg, len);
        segbuf[len] = '\0';

        char name[128];
        if (len >= sizeof(name)) {
            return MAXCFG_ERR_INVALID_ARGUMENT;
        }
        memcpy(name, segbuf, len);
        name[len] = '\0';

        TomlNode *child = toml_table_get_node(*out, name);
        if (child == NULL) {
            TomlTable *nt = toml_table_new();
            if (nt == NULL) {
                return MAXCFG_ERR_OOM;
            }

            TomlNode *nn = toml_node_new(MAXCFG_VAR_TABLE);
            if (nn == NULL) {
                toml_table_free(nt);
                return MAXCFG_ERR_OOM;
            }
            nn->v.table = nt;

            MaxCfgStatus st = toml_table_set_node(*out, name, nn);
            if (st != MAXCFG_OK) {
                toml_node_free(nn);
                return st;
            }
            child = nn;
        }

        if (child->type != MAXCFG_VAR_TABLE) {
            return MAXCFG_ERR_IO;
        }
        *out = child->v.table;

        if (*p == '.') {
            p++;
        }
    }

    return MAXCFG_OK;
}

static MaxCfgStatus file_parse_into_table(const char *path, TomlTable **out_root)
{
    if (path == NULL || out_root == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    *out_root = NULL;
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        if (errno == ENOENT) {
            return MAXCFG_ERR_NOT_FOUND;
        }
        return MAXCFG_ERR_IO;
    }

    TomlTable *root = toml_table_new();
    if (root == NULL) {
        fclose(fp);
        return MAXCFG_ERR_OOM;
    }

    TomlTable *cur = root;
    char linebuf[4096];
    while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
        strip_comment(linebuf);
        char *line = str_trim_inplace(linebuf);
        if (line == NULL || line[0] == '\0') {
            continue;
        }

        size_t n = strlen(line);
        if (n >= 2u && line[0] == '[' && line[1] == '[') {
            if (n < 4u || line[n - 2u] != ']' || line[n - 1u] != ']') {
                toml_table_free(root);
                fclose(fp);
                return MAXCFG_ERR_IO;
            }
            line[n - 2u] = '\0';
            char *name = str_trim_inplace(line + 2u);

            TomlNode *arrn = toml_table_get_node(root, name);
            TomlArray *arr = NULL;
            if (arrn == NULL) {
                arr = toml_array_new();
                if (arr == NULL) {
                    toml_table_free(root);
                    fclose(fp);
                    return MAXCFG_ERR_OOM;
                }
                arrn = toml_node_new(MAXCFG_VAR_TABLE_ARRAY);
                if (arrn == NULL) {
                    toml_array_free(arr);
                    toml_table_free(root);
                    fclose(fp);
                    return MAXCFG_ERR_OOM;
                }
                arrn->v.array = arr;

                MaxCfgStatus st = toml_table_set_node(root, name, arrn);
                if (st != MAXCFG_OK) {
                    toml_node_free(arrn);
                    toml_table_free(root);
                    fclose(fp);
                    return st;
                }
            } else {
                if (arrn->type != MAXCFG_VAR_TABLE_ARRAY) {
                    toml_table_free(root);
                    fclose(fp);
                    return MAXCFG_ERR_IO;
                }
                arr = arrn->v.array;
            }

            TomlTable *nt = toml_table_new();
            if (nt == NULL) {
                toml_table_free(root);
                fclose(fp);
                return MAXCFG_ERR_OOM;
            }

            TomlNode *tn = toml_node_new(MAXCFG_VAR_TABLE);
            if (tn == NULL) {
                toml_table_free(nt);
                toml_table_free(root);
                fclose(fp);
                return MAXCFG_ERR_OOM;
            }
            tn->v.table = nt;

            MaxCfgStatus st = toml_array_add_node(arr, tn);
            if (st != MAXCFG_OK) {
                toml_node_free(tn);
                toml_table_free(root);
                fclose(fp);
                return st;
            }
            cur = nt;
            continue;
        }

        if (line[0] == '[') {
            if (n < 2u || line[n - 1u] != ']') {
                toml_table_free(root);
                fclose(fp);
                return MAXCFG_ERR_IO;
            }
            line[n - 1u] = '\0';
            char *name = str_trim_inplace(line + 1u);
            TomlTable *t = NULL;
            MaxCfgStatus st = table_get_or_create_table(root, name, &t);
            if (st != MAXCFG_OK) {
                toml_table_free(root);
                fclose(fp);
                return st;
            }
            cur = t;
            continue;
        }

        char *eq = strchr(line, '=');
        if (eq == NULL) {
            toml_table_free(root);
            fclose(fp);
            return MAXCFG_ERR_IO;
        }
        *eq = '\0';
        char *key = str_trim_inplace(line);
        const char *p = skip_ws(eq + 1);

        if (key == NULL || key[0] == '\0') {
            toml_table_free(root);
            fclose(fp);
            return MAXCFG_ERR_IO;
        }

        TomlNode *val = NULL;
        MaxCfgStatus st = parse_value(&p, &val);
        if (st != MAXCFG_OK) {
            toml_table_free(root);
            fclose(fp);
            return st;
        }

        st = toml_table_set_node(cur, key, val);
        if (st != MAXCFG_OK) {
            toml_node_free(val);
            toml_table_free(root);
            fclose(fp);
            return st;
        }
    }

    fclose(fp);
    *out_root = root;
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_toml_init(MaxCfgToml **out_toml)
{
    if (out_toml == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    *out_toml = NULL;
    MaxCfgToml *t = (MaxCfgToml *)calloc(1, sizeof(*t));
    if (t == NULL) {
        return MAXCFG_ERR_OOM;
    }

    TomlTable *rt = toml_table_new();
    if (rt == NULL) {
        free(t);
        return MAXCFG_ERR_OOM;
    }

    t->root = toml_node_new(MAXCFG_VAR_TABLE);
    if (t->root == NULL) {
        toml_table_free(rt);
        free(t);
        return MAXCFG_ERR_OOM;
    }
    t->root->v.table = rt;

    t->overrides = toml_table_new();
    if (t->overrides == NULL) {
        toml_node_free(t->root);
        free(t);
        return MAXCFG_ERR_OOM;
    }

    t->loaded_files = NULL;
    t->loaded_file_count = 0;
    t->loaded_file_capacity = 0;

    *out_toml = t;
    return MAXCFG_OK;
}

void maxcfg_toml_free(MaxCfgToml *toml)
{
    if (toml == NULL) {
        return;
    }

    toml_node_free(toml->root);
    toml_table_free(toml->overrides);
    maxcfg_free_loaded_files(toml);
    free(toml);
}

MaxCfgStatus maxcfg_toml_load_file(MaxCfgToml *toml, const char *path, const char *prefix)
{
    if (toml == NULL || toml->root == NULL || toml->root->type != MAXCFG_VAR_TABLE || path == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    const char *used_path = path;
    char alt_path[1024];

    TomlTable *file_root = NULL;
    MaxCfgStatus st = file_parse_into_table(path, &file_root);
    if (st == MAXCFG_ERR_NOT_FOUND) {
        size_t n = strlen(path);
        bool has_toml_ext = false;

        if (n >= 5u) {
            const char *ext = path + (n - 5u);
            has_toml_ext = (tolower((unsigned char)ext[0]) == '.' &&
                            tolower((unsigned char)ext[1]) == 't' &&
                            tolower((unsigned char)ext[2]) == 'o' &&
                            tolower((unsigned char)ext[3]) == 'm' &&
                            tolower((unsigned char)ext[4]) == 'l');
        }

        if (!has_toml_ext) {
            if (n + 5u >= sizeof(alt_path)) {
                return MAXCFG_ERR_PATH_TOO_LONG;
            }
            memcpy(alt_path, path, n);
            memcpy(alt_path + n, ".toml", 6u);
            file_root = NULL;
            st = file_parse_into_table(alt_path, &file_root);
            if (st == MAXCFG_OK) {
                used_path = alt_path;
            }
        }
    }
    if (st != MAXCFG_OK) {
        return st;
    }

    TomlTable *dst = toml->root->v.table;
    if (prefix != NULL && prefix[0] != '\0') {
        st = table_get_or_create_table(dst, prefix, &dst);
        if (st != MAXCFG_OK) {
            toml_table_free(file_root);
            return st;
        }
    }

    for (size_t i = 0; i < file_root->count; i++) {
        TomlNode *v = file_root->items[i].value;
        file_root->items[i].value = NULL;
        st = toml_table_set_node(dst, file_root->items[i].key, v);
        if (st != MAXCFG_OK) {
            toml_node_free(v);
            toml_table_free(file_root);
            return st;
        }
    }

    toml_table_free(file_root);

    {
        MaxCfgStatus st2 = maxcfg_loaded_files_ensure_capacity(toml, toml->loaded_file_count + 1u);
        if (st2 != MAXCFG_OK) {
            return st2;
        }

        TomlLoadedFile *lf = &toml->loaded_files[toml->loaded_file_count];
        memset(lf, 0, sizeof(*lf));

        lf->path = strdup(used_path);
        if (lf->path == NULL) {
            return MAXCFG_ERR_OOM;
        }

        lf->prefix = strdup((prefix != NULL) ? prefix : "");
        if (lf->prefix == NULL) {
            free(lf->path);
            lf->path = NULL;
            return MAXCFG_ERR_OOM;
        }

        toml->loaded_file_count++;
    }

    return MAXCFG_OK;
}

static const TomlNode *toml_get_node_base(const TomlNode *root, const char *path)
{
    if (root == NULL || path == NULL) {
        return NULL;
    }

    if (path[0] == '\0') {
        return root;
    }

    const TomlNode *cur = root;
    const char *p = path;
    char segbuf[256];
    while (*p) {
        const char *start = p;
        while (*p && *p != '.') {
            p++;
        }
        size_t slen = (size_t)(p - start);
        if (slen == 0u || slen >= sizeof(segbuf)) {
            return NULL;
        }
        memcpy(segbuf, start, slen);
        segbuf[slen] = '\0';

        char name[256];
        bool has_idx = false;
        size_t idx = 0u;
        if (parse_segment(segbuf, name, sizeof(name), &has_idx, &idx) != MAXCFG_OK) {
            return NULL;
        }

        if (cur->type != MAXCFG_VAR_TABLE) {
            return NULL;
        }
        TomlNode *child = toml_table_get_node(cur->v.table, name);
        if (child == NULL) {
            return NULL;
        }
        cur = child;

        if (has_idx) {
            if (cur->type == MAXCFG_VAR_TABLE_ARRAY) {
                if (idx >= cur->v.array->count) {
                    return NULL;
                }
                cur = cur->v.array->items[idx];
            } else if (cur->type == MAXCFG_VAR_STRING_ARRAY) {
                return NULL;
            } else {
                return NULL;
            }
        }

        if (*p == '.') {
            p++;
        }
    }

    return cur;
}

static TomlNode *toml_node_clone(const TomlNode *src)
{
    if (src == NULL) {
        return NULL;
    }

    TomlNode *dst = toml_node_new(src->type);
    if (dst == NULL) {
        return NULL;
    }

    switch (src->type) {
    case MAXCFG_VAR_INT:
        dst->v.i = src->v.i;
        break;
    case MAXCFG_VAR_UINT:
        dst->v.u = src->v.u;
        break;
    case MAXCFG_VAR_BOOL:
        dst->v.b = src->v.b;
        break;
    case MAXCFG_VAR_STRING:
        dst->v.s = strdup(src->v.s ? src->v.s : "");
        if (dst->v.s == NULL) {
            toml_node_free(dst);
            return NULL;
        }
        break;
    case MAXCFG_VAR_STRING_ARRAY:
        if (src->v.strv.count > 0) {
            dst->v.strv.items = (char **)calloc(src->v.strv.count, sizeof(dst->v.strv.items[0]));
            if (dst->v.strv.items == NULL) {
                toml_node_free(dst);
                return NULL;
            }
            dst->v.strv.count = src->v.strv.count;
            for (size_t i = 0; i < src->v.strv.count; i++) {
                dst->v.strv.items[i] = strdup(src->v.strv.items[i] ? src->v.strv.items[i] : "");
                if (dst->v.strv.items[i] == NULL) {
                    toml_node_free(dst);
                    return NULL;
                }
            }
        }
        break;
    case MAXCFG_VAR_INT_ARRAY:
        if (src->v.intv.count > 0) {
            dst->v.intv.items = (int *)calloc(src->v.intv.count, sizeof(dst->v.intv.items[0]));
            if (dst->v.intv.items == NULL) {
                toml_node_free(dst);
                return NULL;
            }
            dst->v.intv.count = src->v.intv.count;
            memcpy(dst->v.intv.items, src->v.intv.items, src->v.intv.count * sizeof(dst->v.intv.items[0]));
        }
        break;
    case MAXCFG_VAR_TABLE:
        dst->v.table = toml_table_new();
        if (dst->v.table == NULL) {
            toml_node_free(dst);
            return NULL;
        }
        if (src->v.table != NULL) {
            for (size_t i = 0; i < src->v.table->count; i++) {
                TomlNode *child = toml_node_clone(src->v.table->items[i].value);
                if (child == NULL) {
                    toml_node_free(dst);
                    return NULL;
                }
                if (toml_table_set_node(dst->v.table, src->v.table->items[i].key, child) != MAXCFG_OK) {
                    toml_node_free(child);
                    toml_node_free(dst);
                    return NULL;
                }
            }
        }
        break;
    case MAXCFG_VAR_TABLE_ARRAY:
        dst->v.array = toml_array_new();
        if (dst->v.array == NULL) {
            toml_node_free(dst);
            return NULL;
        }
        if (src->v.array != NULL && src->v.array->count > 0) {
            if (toml_array_ensure_capacity(dst->v.array, src->v.array->count) != MAXCFG_OK) {
                toml_node_free(dst);
                return NULL;
            }
            for (size_t i = 0; i < src->v.array->count; i++) {
                TomlNode *it = toml_node_clone(src->v.array->items[i]);
                if (it == NULL) {
                    toml_node_free(dst);
                    return NULL;
                }
                dst->v.array->items[dst->v.array->count++] = it;
            }
        }
        break;
    default:
        toml_node_free(dst);
        return NULL;
    }

    return dst;
}

static MaxCfgStatus toml_set_path_node(TomlTable *root, const char *path, TomlNode *value)
{
    if (root == NULL || path == NULL || value == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    TomlNode *cur = NULL;
    TomlTable *cur_table = root;

    const char *p = path;
    char segbuf[256];

    while (*p) {
        const char *start = p;
        while (*p && *p != '.') {
            p++;
        }
        size_t slen = (size_t)(p - start);
        if (slen == 0u || slen >= sizeof(segbuf)) {
            return MAXCFG_ERR_INVALID_ARGUMENT;
        }
        memcpy(segbuf, start, slen);
        segbuf[slen] = '\0';

        char name[256];
        bool has_idx = false;
        size_t idx = 0u;
        MaxCfgStatus st = parse_segment(segbuf, name, sizeof(name), &has_idx, &idx);
        if (st != MAXCFG_OK) {
            return st;
        }

        bool is_last = (*p == '\0');

        if (!has_idx) {
            if (is_last) {
                return toml_table_set_node(cur_table, name, value);
            }

            TomlNode *child = toml_table_get_node(cur_table, name);
            if (child == NULL || child->type != MAXCFG_VAR_TABLE || child->v.table == NULL) {
                TomlNode *nn = toml_node_new(MAXCFG_VAR_TABLE);
                if (nn == NULL) {
                    return MAXCFG_ERR_OOM;
                }
                nn->v.table = toml_table_new();
                if (nn->v.table == NULL) {
                    toml_node_free(nn);
                    return MAXCFG_ERR_OOM;
                }
                st = toml_table_set_node(cur_table, name, nn);
                if (st != MAXCFG_OK) {
                    toml_node_free(nn);
                    return st;
                }
                child = nn;
            }

            cur = child;
            cur_table = child->v.table;
        } else {
            /* name[idx] */
            TomlNode *arr = toml_table_get_node(cur_table, name);
            if (arr == NULL) {
                TomlNode *arrn = toml_node_new((is_last && value->type == MAXCFG_VAR_STRING) ? MAXCFG_VAR_STRING_ARRAY : MAXCFG_VAR_TABLE_ARRAY);
                if (arrn == NULL) {
                    return MAXCFG_ERR_OOM;
                }
                if (arrn->type == MAXCFG_VAR_TABLE_ARRAY) {
                    arrn->v.array = toml_array_new();
                    if (arrn->v.array == NULL) {
                        toml_node_free(arrn);
                        return MAXCFG_ERR_OOM;
                    }
                }
                st = toml_table_set_node(cur_table, name, arrn);
                if (st != MAXCFG_OK) {
                    toml_node_free(arrn);
                    return st;
                }
                arr = arrn;
            }

            if (arr->type == MAXCFG_VAR_TABLE_ARRAY) {
                st = toml_array_set_count_table(arr->v.array, idx + 1u);
                if (st != MAXCFG_OK) {
                    return st;
                }

                TomlNode *elem = arr->v.array->items[idx];
                if (elem == NULL || elem->type != MAXCFG_VAR_TABLE || elem->v.table == NULL) {
                    return MAXCFG_ERR_INVALID_ARGUMENT;
                }

                if (is_last) {
                    /* Setting name[idx] itself to a table doesn't make sense for our use.
                     * Caller should use name[idx].leaf.
                     */
                    return MAXCFG_ERR_INVALID_ARGUMENT;
                }

                cur = elem;
                cur_table = elem->v.table;
            } else if (arr->type == MAXCFG_VAR_STRING_ARRAY) {
                if (is_last) {
                    if (value->type != MAXCFG_VAR_STRING) {
                        return MAXCFG_ERR_INVALID_ARGUMENT;
                    }
                    st = toml_string_array_set_count(arr, idx + 1u);
                    if (st != MAXCFG_OK) {
                        return st;
                    }
                    free(arr->v.strv.items[idx]);
                    arr->v.strv.items[idx] = strdup(value->v.s ? value->v.s : "");
                    if (arr->v.strv.items[idx] == NULL) {
                        return MAXCFG_ERR_OOM;
                    }
                    toml_node_free(value);
                    return MAXCFG_OK;
                }
                return MAXCFG_ERR_INVALID_ARGUMENT;
            } else {
                return MAXCFG_ERR_INVALID_ARGUMENT;
            }
        }

        if (*p == '.') {
            p++;
        }
    }

    return MAXCFG_ERR_INVALID_ARGUMENT;
}

static void toml_emit_key(FILE *fp, const char *key, const TomlNode *n)
{
    if (fp == NULL || key == NULL || n == NULL) {
        return;
    }

    switch (n->type) {
    case MAXCFG_VAR_INT:
        toml_kv_int(fp, key, n->v.i);
        break;
    case MAXCFG_VAR_UINT:
        toml_kv_uint(fp, key, n->v.u);
        break;
    case MAXCFG_VAR_BOOL:
        toml_kv_bool(fp, key, n->v.b);
        break;
    case MAXCFG_VAR_STRING:
        toml_kv_string(fp, key, n->v.s);
        break;
    case MAXCFG_VAR_STRING_ARRAY:
        toml_kv_string_array(fp, key, n->v.strv.items, n->v.strv.count);
        break;
    case MAXCFG_VAR_INT_ARRAY:
        toml_kv_int_array(fp, key, n->v.intv.items, n->v.intv.count);
        break;
    default:
        break;
    }
}

static void toml_emit_table(FILE *fp, const TomlTable *t, const char *section)
{
    if (fp == NULL || t == NULL) {
        return;
    }

    bool wrote_any = false;

    for (size_t i = 0; i < t->count; i++) {
        const char *key = t->items[i].key;
        const TomlNode *n = t->items[i].value;
        if (key == NULL || n == NULL) {
            continue;
        }

        if (n->type == MAXCFG_VAR_TABLE || n->type == MAXCFG_VAR_TABLE_ARRAY) {
            continue;
        }

        toml_emit_key(fp, key, n);
        wrote_any = true;
    }

    for (size_t i = 0; i < t->count; i++) {
        const char *key = t->items[i].key;
        const TomlNode *n = t->items[i].value;
        if (key == NULL || n == NULL || n->type != MAXCFG_VAR_TABLE_ARRAY || n->v.array == NULL) {
            continue;
        }

        for (size_t j = 0; j < n->v.array->count; j++) {
            const TomlNode *elem = n->v.array->items[j];
            if (elem == NULL || elem->type != MAXCFG_VAR_TABLE || elem->v.table == NULL) {
                continue;
            }

            char arr_section[512];
            if (section && *section) {
                snprintf(arr_section, sizeof(arr_section), "%s.%s", section, key);
                fprintf(fp, "[[%s]]\n", arr_section);
            } else {
                snprintf(arr_section, sizeof(arr_section), "%s", key);
                fprintf(fp, "[[%s]]\n", arr_section);
            }

            toml_emit_table(fp, elem->v.table, arr_section);

            if (wrote_any || (j + 1u) < n->v.array->count) {
                fputc('\n', fp);
            }
            wrote_any = true;
        }
    }

    for (size_t i = 0; i < t->count; i++) {
        const char *key = t->items[i].key;
        const TomlNode *n = t->items[i].value;
        if (key == NULL || n == NULL || n->type != MAXCFG_VAR_TABLE || n->v.table == NULL) {
            continue;
        }

        char child_section[512];
        if (section && *section) {
            snprintf(child_section, sizeof(child_section), "%s.%s", section, key);
        } else {
            snprintf(child_section, sizeof(child_section), "%s", key);
        }

        fputc('\n', fp);
        fprintf(fp, "[%s]\n", child_section);
        toml_emit_table(fp, n->v.table, child_section);
    }
}

static MaxCfgStatus maxcfg_toml_write_atomic_prefix_to_path(const MaxCfgToml *toml, const char *prefix, const char *path)
{
    if (toml == NULL || toml->root == NULL || toml->root->type != MAXCFG_VAR_TABLE || path == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    const TomlNode *n = toml_get_node_base(toml->root, (prefix != NULL) ? prefix : "");
    if (n == NULL || n->type != MAXCFG_VAR_TABLE || n->v.table == NULL) {
        return MAXCFG_ERR_NOT_FOUND;
    }

    size_t tmp_len = strlen(path) + 5u;
    char *tmp_path = (char *)malloc(tmp_len);
    if (tmp_path == NULL) {
        return MAXCFG_ERR_OOM;
    }
    snprintf(tmp_path, tmp_len, "%s.tmp", path);

    FILE *fp = fopen(tmp_path, "wb");
    if (fp == NULL) {
        free(tmp_path);
        return MAXCFG_ERR_IO;
    }

    toml_emit_table(fp, n->v.table, "");

    if (fclose(fp) != 0) {
        remove(tmp_path);
        free(tmp_path);
        return MAXCFG_ERR_IO;
    }

    if (rename(tmp_path, path) != 0) {
        remove(tmp_path);
        free(tmp_path);
        return MAXCFG_ERR_IO;
    }

    free(tmp_path);
    return MAXCFG_OK;
}

static const TomlLoadedFile *maxcfg_find_loaded_file_for_prefix(const MaxCfgToml *toml, const char *prefix)
{
    if (toml == NULL || toml->loaded_files == NULL || prefix == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < toml->loaded_file_count; i++) {
        if (toml->loaded_files[i].prefix != NULL && strcmp(toml->loaded_files[i].prefix, prefix) == 0) {
            return &toml->loaded_files[i];
        }
    }
    return NULL;
}

static const TomlLoadedFile *maxcfg_find_best_loaded_file_for_path(const MaxCfgToml *toml, const char *path)
{
    if (toml == NULL || toml->loaded_files == NULL || path == NULL) {
        return NULL;
    }

    const TomlLoadedFile *best = NULL;
    size_t best_len = 0;

    for (size_t i = 0; i < toml->loaded_file_count; i++) {
        const char *pfx = toml->loaded_files[i].prefix;
        if (pfx == NULL) {
            continue;
        }
        size_t plen = strlen(pfx);
        if (plen == 0) {
            continue;
        }
        if (strncmp(path, pfx, plen) != 0) {
            continue;
        }
        if (path[plen] != '\0' && path[plen] != '.') {
            continue;
        }
        if (plen > best_len) {
            best = &toml->loaded_files[i];
            best_len = plen;
        }
    }

    return best;
}

MaxCfgStatus maxcfg_toml_save_loaded_files(const MaxCfgToml *toml)
{
    if (toml == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    if (toml->loaded_files == NULL || toml->loaded_file_count == 0) {
        return MAXCFG_ERR_NOT_FOUND;
    }

    for (size_t i = 0; i < toml->loaded_file_count; i++) {
        const TomlLoadedFile *lf = &toml->loaded_files[i];
        if (lf->path == NULL || lf->prefix == NULL) {
            continue;
        }
        MaxCfgStatus st = maxcfg_toml_write_atomic_prefix_to_path(toml, lf->prefix, lf->path);
        if (st != MAXCFG_OK) {
            return st;
        }
    }

    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_toml_save_prefix(const MaxCfgToml *toml, const char *prefix)
{
    if (toml == NULL || prefix == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    const TomlLoadedFile *lf = maxcfg_find_loaded_file_for_prefix(toml, prefix);
    if (lf == NULL || lf->path == NULL) {
        return MAXCFG_ERR_NOT_FOUND;
    }

    return maxcfg_toml_write_atomic_prefix_to_path(toml, prefix, lf->path);
}

MaxCfgStatus maxcfg_toml_persist_override(MaxCfgToml *toml, const char *path)
{
    if (toml == NULL || toml->root == NULL || toml->root->type != MAXCFG_VAR_TABLE || toml->overrides == NULL || path == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    TomlNode *ov = toml_table_get_node(toml->overrides, path);
    if (ov == NULL) {
        return MAXCFG_ERR_NOT_FOUND;
    }

    TomlNode *cl = toml_node_clone(ov);
    if (cl == NULL) {
        return MAXCFG_ERR_OOM;
    }

    MaxCfgStatus st = toml_set_path_node(toml->root->v.table, path, cl);
    if (st != MAXCFG_OK) {
        toml_node_free(cl);
        return st;
    }

    (void)toml_table_unset_node(toml->overrides, path);
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_toml_persist_override_and_save(MaxCfgToml *toml, const char *path)
{
    if (toml == NULL || path == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    MaxCfgStatus st = maxcfg_toml_persist_override(toml, path);
    if (st != MAXCFG_OK) {
        return st;
    }

    const TomlLoadedFile *lf = maxcfg_find_best_loaded_file_for_path(toml, path);
    if (lf == NULL || lf->path == NULL || lf->prefix == NULL) {
        return MAXCFG_ERR_NOT_FOUND;
    }

    return maxcfg_toml_write_atomic_prefix_to_path(toml, lf->prefix, lf->path);
}

static int cmp_keys(const void *a, const void *b)
{
    const char *ka = *(const char * const *)a;
    const char *kb = *(const char * const *)b;
    if (ka == NULL && kb == NULL) return 0;
    if (ka == NULL) return -1;
    if (kb == NULL) return 1;
    return strcmp(ka, kb);
}

MaxCfgStatus maxcfg_toml_persist_overrides(MaxCfgToml *toml)
{
    if (toml == NULL || toml->overrides == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    while (toml->overrides->count > 0) {
        size_t n = toml->overrides->count;
        char **keys = (char **)calloc(n, sizeof(keys[0]));
        if (keys == NULL) {
            return MAXCFG_ERR_OOM;
        }
        for (size_t i = 0; i < n; i++) {
            keys[i] = toml->overrides->items[i].key;
        }
        qsort(keys, n, sizeof(keys[0]), cmp_keys);

        /* Persist in sorted order; if a key disappears due to earlier persistence, skip it. */
        for (size_t i = 0; i < n; i++) {
            const char *k = keys[i];
            if (k == NULL) {
                continue;
            }
            if (toml_table_get_node(toml->overrides, k) == NULL) {
                continue;
            }
            MaxCfgStatus st = maxcfg_toml_persist_override(toml, k);
            if (st != MAXCFG_OK) {
                free(keys);
                return st;
            }
        }

        free(keys);
    }

    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_toml_persist_overrides_and_save(MaxCfgToml *toml)
{
    if (toml == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    MaxCfgStatus st = maxcfg_toml_persist_overrides(toml);
    if (st != MAXCFG_OK) {
        return st;
    }

    return maxcfg_toml_save_loaded_files(toml);
}

static MaxCfgStatus var_from_node(const TomlNode *n, MaxCfgVar *out)
{
    if (out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(out, 0, sizeof(*out));
    if (n == NULL) {
        out->type = MAXCFG_VAR_NULL;
        return MAXCFG_OK;
    }

    out->type = n->type;
    switch (n->type) {
    case MAXCFG_VAR_INT:
        out->v.i = n->v.i;
        break;
    case MAXCFG_VAR_UINT:
        out->v.u = n->v.u;
        break;
    case MAXCFG_VAR_BOOL:
        out->v.b = n->v.b;
        break;
    case MAXCFG_VAR_STRING:
        out->v.s = n->v.s ? n->v.s : "";
        break;
    case MAXCFG_VAR_STRING_ARRAY:
        out->v.strv.items = (const char **)n->v.strv.items;
        out->v.strv.count = n->v.strv.count;
        break;
    case MAXCFG_VAR_INT_ARRAY:
        out->v.intv.items = (const int *)n->v.intv.items;
        out->v.intv.count = n->v.intv.count;
        break;
    case MAXCFG_VAR_TABLE:
        out->v.opaque = (void *)n->v.table;
        break;
    case MAXCFG_VAR_TABLE_ARRAY:
        out->v.opaque = (void *)n->v.array;
        break;
    default:
        out->type = MAXCFG_VAR_NULL;
        break;
    }

    return MAXCFG_OK;
}

static MaxCfgStatus parse_segment(const char *seg, char *name_out, size_t name_out_sz, bool *has_index, size_t *index_out)
{
    if (seg == NULL || name_out == NULL || name_out_sz == 0 || has_index == NULL || index_out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    *has_index = false;
    *index_out = 0u;

    const char *br = strchr(seg, '[');
    if (br == NULL) {
        size_t len = strlen(seg);
        if (len + 1u > name_out_sz) {
            return MAXCFG_ERR_INVALID_ARGUMENT;
        }
        memcpy(name_out, seg, len + 1u);
        return MAXCFG_OK;
    }

    size_t nlen = (size_t)(br - seg);
    if (nlen + 1u > name_out_sz) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    memcpy(name_out, seg, nlen);
    name_out[nlen] = '\0';

    const char *p = br + 1;
    char *endp = NULL;
    long idx = strtol(p, &endp, 10);
    if (endp == p || idx < 0 || endp == NULL || *endp != ']') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    *has_index = true;
    *index_out = (size_t)idx;
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_toml_get(const MaxCfgToml *toml, const char *path, MaxCfgVar *out)
{
    if (toml == NULL || toml->root == NULL || toml->root->type != MAXCFG_VAR_TABLE || path == NULL || out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (toml->overrides != NULL) {
        TomlNode *ov = toml_table_get_node(toml->overrides, path);
        if (ov != NULL) {
            return var_from_node(ov, out);
        }
    }

    const TomlNode *cur = toml->root;
    const char *p = path;
    char segbuf[256];
    while (*p) {
        const char *start = p;
        while (*p && *p != '.') {
            p++;
        }
        size_t slen = (size_t)(p - start);
        if (slen == 0u || slen >= sizeof(segbuf)) {
            return MAXCFG_ERR_INVALID_ARGUMENT;
        }
        memcpy(segbuf, start, slen);
        segbuf[slen] = '\0';

        char name[256];
        bool has_idx = false;
        size_t idx = 0u;
        MaxCfgStatus st = parse_segment(segbuf, name, sizeof(name), &has_idx, &idx);
        if (st != MAXCFG_OK) {
            return st;
        }

        if (cur->type != MAXCFG_VAR_TABLE) {
            return MAXCFG_ERR_NOT_FOUND;
        }
        TomlNode *child = toml_table_get_node(cur->v.table, name);
        if (child == NULL) {
            return MAXCFG_ERR_NOT_FOUND;
        }

        cur = child;
        if (has_idx) {
            if (cur->type == MAXCFG_VAR_TABLE_ARRAY) {
                if (idx >= cur->v.array->count) {
                    return MAXCFG_ERR_NOT_FOUND;
                }
                cur = cur->v.array->items[idx];
            } else if (cur->type == MAXCFG_VAR_STRING_ARRAY) {
                if (idx >= cur->v.strv.count) {
                    return MAXCFG_ERR_NOT_FOUND;
                }
                out->type = MAXCFG_VAR_STRING;
                out->v.s = cur->v.strv.items[idx] ? cur->v.strv.items[idx] : "";
                return MAXCFG_OK;
            } else if (cur->type == MAXCFG_VAR_INT_ARRAY) {
                /* Allow direct indexed reads like: custom_menu.top_boundary[0] */
                if (idx >= cur->v.intv.count) {
                    return MAXCFG_ERR_NOT_FOUND;
                }
                out->type = MAXCFG_VAR_INT;
                out->v.i = cur->v.intv.items[idx];
                return MAXCFG_OK;
            } else {
                return MAXCFG_ERR_NOT_FOUND;
            }
        }

        if (*p == '.') {
            p++;
        }
    }

    return var_from_node(cur, out);
}

MaxCfgStatus maxcfg_toml_override_set_int(MaxCfgToml *toml, const char *path, int v)
{
    if (toml == NULL || toml->overrides == NULL || path == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    TomlNode *n = toml_node_new(MAXCFG_VAR_INT);
    if (n == NULL) {
        return MAXCFG_ERR_OOM;
    }
    n->v.i = v;

    MaxCfgStatus st = toml_table_set_node(toml->overrides, path, n);
    if (st != MAXCFG_OK) {
        toml_node_free(n);
    }
    return st;
}

MaxCfgStatus maxcfg_toml_override_set_uint(MaxCfgToml *toml, const char *path, unsigned int v)
{
    if (toml == NULL || toml->overrides == NULL || path == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    TomlNode *n = toml_node_new(MAXCFG_VAR_UINT);
    if (n == NULL) {
        return MAXCFG_ERR_OOM;
    }
    n->v.u = v;

    MaxCfgStatus st = toml_table_set_node(toml->overrides, path, n);
    if (st != MAXCFG_OK) {
        toml_node_free(n);
    }
    return st;
}

MaxCfgStatus maxcfg_toml_override_set_bool(MaxCfgToml *toml, const char *path, bool v)
{
    if (toml == NULL || toml->overrides == NULL || path == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    TomlNode *n = toml_node_new(MAXCFG_VAR_BOOL);
    if (n == NULL) {
        return MAXCFG_ERR_OOM;
    }
    n->v.b = v;

    MaxCfgStatus st = toml_table_set_node(toml->overrides, path, n);
    if (st != MAXCFG_OK) {
        toml_node_free(n);
    }
    return st;
}

MaxCfgStatus maxcfg_toml_override_set_string(MaxCfgToml *toml, const char *path, const char *v)
{
    if (toml == NULL || toml->overrides == NULL || path == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    TomlNode *n = toml_node_new(MAXCFG_VAR_STRING);
    if (n == NULL) {
        return MAXCFG_ERR_OOM;
    }

    n->v.s = strdup(v ? v : "");
    if (n->v.s == NULL) {
        toml_node_free(n);
        return MAXCFG_ERR_OOM;
    }

    MaxCfgStatus st = toml_table_set_node(toml->overrides, path, n);
    if (st != MAXCFG_OK) {
        toml_node_free(n);
    }
    return st;
}

MaxCfgStatus maxcfg_toml_override_set_string_array(MaxCfgToml *toml, const char *path, const char **items, size_t count)
{
    if (toml == NULL || toml->overrides == NULL || path == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    TomlNode *n = toml_node_new(MAXCFG_VAR_STRING_ARRAY);
    if (n == NULL) {
        return MAXCFG_ERR_OOM;
    }

    if (count > 0) {
        n->v.strv.items = (char **)calloc(count, sizeof(n->v.strv.items[0]));
        if (n->v.strv.items == NULL) {
            toml_node_free(n);
            return MAXCFG_ERR_OOM;
        }
        n->v.strv.count = count;
        for (size_t i = 0; i < count; i++) {
            n->v.strv.items[i] = strdup((items != NULL && items[i] != NULL) ? items[i] : "");
            if (n->v.strv.items[i] == NULL) {
                toml_node_free(n);
                return MAXCFG_ERR_OOM;
            }
        }
    }

    MaxCfgStatus st = toml_table_set_node(toml->overrides, path, n);
    if (st != MAXCFG_OK) {
        toml_node_free(n);
    }
    return st;
}

MaxCfgStatus maxcfg_toml_override_set_table_array_empty(MaxCfgToml *toml, const char *path)
{
    if (toml == NULL || toml->overrides == NULL || path == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    TomlNode *n = toml_node_new(MAXCFG_VAR_TABLE_ARRAY);
    if (n == NULL) {
        return MAXCFG_ERR_OOM;
    }
    n->v.array = toml_array_new();
    if (n->v.array == NULL) {
        toml_node_free(n);
        return MAXCFG_ERR_OOM;
    }

    MaxCfgStatus st = toml_table_set_node(toml->overrides, path, n);
    if (st != MAXCFG_OK) {
        toml_node_free(n);
    }
    return st;
}

MaxCfgStatus maxcfg_toml_override_unset(MaxCfgToml *toml, const char *path)
{
    if (toml == NULL || toml->overrides == NULL || path == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    return toml_table_unset_node(toml->overrides, path);
}

void maxcfg_toml_override_clear(MaxCfgToml *toml)
{
    if (toml == NULL || toml->overrides == NULL) {
        return;
    }

    toml_table_clear(toml->overrides);
}

MaxCfgStatus maxcfg_toml_table_get(const MaxCfgVar *table, const char *key, MaxCfgVar *out)
{
    if (table == NULL || out == NULL || key == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    if (table->type != MAXCFG_VAR_TABLE || table->v.opaque == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    TomlTable *t = (TomlTable *)table->v.opaque;
    TomlNode *n = toml_table_get_node(t, key);
    if (n == NULL) {
        return MAXCFG_ERR_NOT_FOUND;
    }
    return var_from_node(n, out);
}

MaxCfgStatus maxcfg_toml_array_get(const MaxCfgVar *array, size_t index, MaxCfgVar *out)
{
    if (array == NULL || out == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (array->type == MAXCFG_VAR_TABLE_ARRAY) {
        TomlArray *a = (TomlArray *)array->v.opaque;
        if (a == NULL || index >= a->count) {
            return MAXCFG_ERR_NOT_FOUND;
        }
        return var_from_node(a->items[index], out);
    }

    if (array->type == MAXCFG_VAR_STRING_ARRAY) {
        if (index >= array->v.strv.count) {
            return MAXCFG_ERR_NOT_FOUND;
        }
        out->type = MAXCFG_VAR_STRING;
        out->v.s = array->v.strv.items[index] ? array->v.strv.items[index] : "";
        return MAXCFG_OK;
    }

    if (array->type == MAXCFG_VAR_INT_ARRAY) {
        if (index >= array->v.intv.count) {
            return MAXCFG_ERR_NOT_FOUND;
        }
        out->type = MAXCFG_VAR_INT;
        out->v.i = array->v.intv.items[index];
        return MAXCFG_OK;
    }

    return MAXCFG_ERR_INVALID_ARGUMENT;
}

MaxCfgStatus maxcfg_var_count(const MaxCfgVar *var, size_t *out_count)
{
    if (out_count == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
    *out_count = 0;
    if (var == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    switch (var->type) {
    case MAXCFG_VAR_STRING_ARRAY:
        *out_count = var->v.strv.count;
        return MAXCFG_OK;
    case MAXCFG_VAR_INT_ARRAY:
        *out_count = var->v.intv.count;
        return MAXCFG_OK;
    case MAXCFG_VAR_TABLE_ARRAY: {
        TomlArray *a = (TomlArray *)var->v.opaque;
        if (a == NULL) {
            return MAXCFG_ERR_INVALID_ARGUMENT;
        }
        *out_count = a->count;
        return MAXCFG_OK;
    }
    case MAXCFG_VAR_TABLE: {
        TomlTable *t = (TomlTable *)var->v.opaque;
        if (t == NULL) {
            return MAXCFG_ERR_INVALID_ARGUMENT;
        }
        *out_count = t->count;
        return MAXCFG_OK;
    }
    default:
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }
}

static void toml_write_escaped(FILE *fp, const char *s)
{
    fputc('"', fp);
    if (s) {
        for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
            if (*p == '\\') {
                fputs("\\\\", fp);
            } else if (*p == '"') {
                fputs("\\\"", fp);
            } else if (*p == '\n') {
                fputs("\\n", fp);
            } else if (*p == '\r') {
                fputs("\\r", fp);
            } else if (*p == '\t') {
                fputs("\\t", fp);
            } else {
                fputc((int)*p, fp);
            }
        }
    }
    fputc('"', fp);
}

static void toml_kv_string(FILE *fp, const char *key, const char *value)
{
    fprintf(fp, "%s = ", key);
    toml_write_escaped(fp, value ? value : "");
    fputc('\n', fp);
}

static void toml_kv_int(FILE *fp, const char *key, int value)
{
    fprintf(fp, "%s = %d\n", key, value);
}

static void toml_kv_bool(FILE *fp, const char *key, bool value)
{
    fprintf(fp, "%s = %s\n", key, value ? "true" : "false");
}

static void toml_kv_color(FILE *fp, const char *key, const MaxCfgNgColor *c)
{
    char mci[32];
    MaxCfgNgColor zero = {0, 0, false};
    maxcfg_ng_color_to_mci(c ? c : &zero, mci, sizeof(mci));
    fprintf(fp, "%s = \"%s\"\n", key, mci);
}

static void toml_kv_uint(FILE *fp, const char *key, unsigned int value)
{
    fprintf(fp, "%s = %u\n", key, value);
}

static void toml_kv_string_array(FILE *fp, const char *key, char **items, size_t count)
{
    fprintf(fp, "%s = [", key);
    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            fputs(", ", fp);
        }
        toml_write_escaped(fp, items[i] ? items[i] : "");
    }
    fputs("]\n", fp);
}

static void toml_kv_int_array(FILE *fp, const char *key, int *items, size_t count)
{
    fprintf(fp, "%s = [", key);
    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            fputs(", ", fp);
        }
        fprintf(fp, "%d", (items != NULL) ? items[i] : 0);
    }
    fprintf(fp, "]\n");
}

static void maxcfg_free_strv(char ***items_io, size_t *count_io)
{
    if (items_io == NULL || *items_io == NULL) {
        if (count_io) {
            *count_io = 0;
        }
        return;
    }

    if (count_io) {
        for (size_t i = 0; i < *count_io; i++) {
            free((*items_io)[i]);
        }
        *count_io = 0;
    } else {
        for (size_t i = 0; (*items_io)[i] != NULL; i++) {
            free((*items_io)[i]);
        }
    }

    free(*items_io);
    *items_io = NULL;
}

static MaxCfgStatus maxcfg_copy_strv(char ***out_items, size_t *out_count, char **in_items, size_t in_count)
{
    if (out_items == NULL || out_count == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    *out_items = NULL;
    *out_count = 0;

    if (in_items == NULL || in_count == 0) {
        return MAXCFG_OK;
    }

    char **dst = (char **)calloc(in_count, sizeof(dst[0]));
    if (dst == NULL) {
        return MAXCFG_ERR_OOM;
    }

    for (size_t i = 0; i < in_count; i++) {
        dst[i] = strdup(in_items[i] ? in_items[i] : "");
        if (dst[i] == NULL) {
            for (size_t j = 0; j < i; j++) {
                free(dst[j]);
            }
            free(dst);
            return MAXCFG_ERR_OOM;
        }
    }

    *out_items = dst;
    *out_count = in_count;
    return MAXCFG_OK;
}

static MaxCfgStatus ensure_base_dir_is_dir(const char *base_dir)
{
    struct stat st;

    if (base_dir == NULL || base_dir[0] == '\0') {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (stat(base_dir, &st) != 0) {
        if (errno == ENOENT) {
            return MAXCFG_ERR_NOT_FOUND;
        }
        return MAXCFG_ERR_IO;
    }

    if (!S_ISDIR(st.st_mode)) {
        return MAXCFG_ERR_NOT_DIR;
    }

    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_open(MaxCfg **out_cfg, const char *base_dir)
{
    MaxCfg *cfg;
    MaxCfgStatus st;

    if (out_cfg == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    *out_cfg = NULL;

    st = ensure_base_dir_is_dir(base_dir);
    if (st != MAXCFG_OK) {
        return st;
    }

    cfg = (MaxCfg *)calloc(1, sizeof(*cfg));
    if (cfg == NULL) {
        return MAXCFG_ERR_OOM;
    }

    cfg->base_dir = strdup(base_dir);
    if (cfg->base_dir == NULL) {
        free(cfg);
        return MAXCFG_ERR_OOM;
    }

    *out_cfg = cfg;
    return MAXCFG_OK;
}

void maxcfg_close(MaxCfg *cfg)
{
    if (cfg == NULL) {
        return;
    }

    free(cfg->base_dir);
    free(cfg);
}

const char *maxcfg_base_dir(const MaxCfg *cfg)
{
    if (cfg == NULL) {
        return NULL;
    }

    return cfg->base_dir;
}

const char *maxcfg_status_string(MaxCfgStatus st)
{
    switch (st) {
    case MAXCFG_OK:
        return "OK";
    case MAXCFG_ERR_INVALID_ARGUMENT:
        return "Invalid argument";
    case MAXCFG_ERR_OOM:
        return "Out of memory";
    case MAXCFG_ERR_NOT_FOUND:
        return "Not found";
    case MAXCFG_ERR_NOT_DIR:
        return "Not a directory";
    case MAXCFG_ERR_IO:
        return "I/O error";
    case MAXCFG_ERR_PATH_TOO_LONG:
        return "Path too long";
    default:
        return "Unknown error";
    }
}

MaxCfgStatus maxcfg_ng_system_init(MaxCfgNgSystem *sys)
{
    if (sys == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(sys, 0, sizeof(*sys));
    sys->config_version = 1;
    sys->msg_reader_menu = strdup("MSGREAD");
    if (sys->msg_reader_menu == NULL) {
        return MAXCFG_ERR_OOM;
    }
    return MAXCFG_OK;
}

void maxcfg_ng_system_free(MaxCfgNgSystem *sys)
{
    if (sys == NULL) {
        return;
    }

    maxcfg_free_and_null(&sys->system_name);
    maxcfg_free_and_null(&sys->sysop);
    maxcfg_free_and_null(&sys->video);
    maxcfg_free_and_null(&sys->multitasker);
    maxcfg_free_and_null(&sys->sys_path);
    maxcfg_free_and_null(&sys->config_path);
    maxcfg_free_and_null(&sys->display_path);
    maxcfg_free_and_null(&sys->lang_path);
    maxcfg_free_and_null(&sys->temp_path);
    maxcfg_free_and_null(&sys->net_info_path);
    maxcfg_free_and_null(&sys->node_path);
    maxcfg_free_and_null(&sys->outbound_path);
    maxcfg_free_and_null(&sys->inbound_path);
    maxcfg_free_and_null(&sys->stage_path);
    maxcfg_free_and_null(&sys->mex_path);
    maxcfg_free_and_null(&sys->data_path);
    maxcfg_free_and_null(&sys->run_path);
    maxcfg_free_and_null(&sys->doors_path);
    maxcfg_free_and_null(&sys->msg_reader_menu);
    maxcfg_free_and_null(&sys->log_file);
    maxcfg_free_and_null(&sys->file_password);
    maxcfg_free_and_null(&sys->file_access);
    maxcfg_free_and_null(&sys->file_callers);
    maxcfg_free_and_null(&sys->message_data);
    maxcfg_free_and_null(&sys->file_data);
    maxcfg_free_and_null(&sys->log_mode);

    memset(sys, 0, sizeof(*sys));
}

MaxCfgStatus maxcfg_ng_general_session_init(MaxCfgNgGeneralSession *session)
{
    if (session == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(session, 0, sizeof(*session));
    return MAXCFG_OK;
}

void maxcfg_ng_general_session_free(MaxCfgNgGeneralSession *session)
{
    if (session == NULL) {
        return;
    }

    maxcfg_free_and_null(&session->edit_menu);
    maxcfg_free_and_null(&session->chat_program);
    maxcfg_free_and_null(&session->local_editor);
    maxcfg_free_and_null(&session->upload_log);
    maxcfg_free_and_null(&session->virus_check);
    maxcfg_free_and_null(&session->comment_area);
    maxcfg_free_and_null(&session->highest_message_area);
    maxcfg_free_and_null(&session->highest_file_area);
    maxcfg_free_and_null(&session->area_change_keys);
    maxcfg_free_and_null(&session->kill_private);
    maxcfg_free_and_null(&session->charset);
    maxcfg_free_strv(&session->save_directories, &session->save_directory_count);
    maxcfg_free_and_null(&session->track_privview);
    maxcfg_free_and_null(&session->track_privmod);
    maxcfg_free_and_null(&session->track_base);
    maxcfg_free_and_null(&session->track_exclude);
    maxcfg_free_and_null(&session->attach_base);
    maxcfg_free_and_null(&session->attach_path);
    maxcfg_free_and_null(&session->attach_archiver);
    maxcfg_free_and_null(&session->kill_attach);
    maxcfg_free_and_null(&session->first_menu);
    maxcfg_free_and_null(&session->first_file_area);
    maxcfg_free_and_null(&session->first_message_area);
    memset(session, 0, sizeof(*session));
}

MaxCfgStatus maxcfg_ng_general_display_files_init(MaxCfgNgGeneralDisplayFiles *files)
{
    if (files == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(files, 0, sizeof(*files));
    return MAXCFG_OK;
}

void maxcfg_ng_general_display_files_free(MaxCfgNgGeneralDisplayFiles *files)
{
    if (files == NULL) {
        return;
    }

    maxcfg_free_and_null(&files->logo);
    maxcfg_free_and_null(&files->not_found);
    maxcfg_free_and_null(&files->application);
    maxcfg_free_and_null(&files->welcome);
    maxcfg_free_and_null(&files->new_user1);
    maxcfg_free_and_null(&files->new_user2);
    maxcfg_free_and_null(&files->rookie);
    maxcfg_free_and_null(&files->not_configured);
    maxcfg_free_and_null(&files->quote);
    maxcfg_free_and_null(&files->day_limit);
    maxcfg_free_and_null(&files->time_warn);
    maxcfg_free_and_null(&files->too_slow);
    maxcfg_free_and_null(&files->bye_bye);
    maxcfg_free_and_null(&files->bad_logon);
    maxcfg_free_and_null(&files->barricade);
    maxcfg_free_and_null(&files->no_space);
    maxcfg_free_and_null(&files->no_mail);
    maxcfg_free_and_null(&files->area_not_exist);
    maxcfg_free_and_null(&files->chat_begin);
    maxcfg_free_and_null(&files->chat_end);
    maxcfg_free_and_null(&files->out_leaving);
    maxcfg_free_and_null(&files->out_return);
    maxcfg_free_and_null(&files->shell_to_dos);
    maxcfg_free_and_null(&files->back_from_dos);
    maxcfg_free_and_null(&files->locate);
    maxcfg_free_and_null(&files->contents);
    maxcfg_free_and_null(&files->oped_help);
    maxcfg_free_and_null(&files->line_ed_help);
    maxcfg_free_and_null(&files->replace_help);
    maxcfg_free_and_null(&files->inquire_help);
    maxcfg_free_and_null(&files->scan_help);
    maxcfg_free_and_null(&files->list_help);
    maxcfg_free_and_null(&files->header_help);
    maxcfg_free_and_null(&files->entry_help);
    maxcfg_free_and_null(&files->xfer_baud);
    maxcfg_free_and_null(&files->file_area_list);
    maxcfg_free_and_null(&files->file_header);
    maxcfg_free_and_null(&files->file_format);
    maxcfg_free_and_null(&files->file_footer);
    maxcfg_free_and_null(&files->msg_area_list);
    maxcfg_free_and_null(&files->msg_header);
    maxcfg_free_and_null(&files->msg_format);
    maxcfg_free_and_null(&files->msg_footer);
    maxcfg_free_and_null(&files->protocol_dump);
    maxcfg_free_and_null(&files->fname_format);
    maxcfg_free_and_null(&files->time_format);
    maxcfg_free_and_null(&files->date_format);
    maxcfg_free_and_null(&files->tune);
    memset(files, 0, sizeof(*files));
}

MaxCfgStatus maxcfg_ng_general_colors_init(MaxCfgNgGeneralColors *colors)
{
    if (colors == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(colors, 0, sizeof(*colors));
    return MAXCFG_OK;
}

/* ============================================================================
 * Theme color API
 * ============================================================================ */

/** @brief Built-in default slot definitions (code, TOML key, default value, description). */
static const struct {
    const char *code;
    const char *key;
    const char *value;
    const char *desc;
} g_theme_defaults[MCI_THEME_SLOT_COUNT] = {
    { "tx", "text",        "|07",    "Normal body text"               },
    { "hi", "highlight",   "|15",    "Emphasized text"                },
    { "pr", "prompt",      "|14",    "User-facing prompts"            },
    { "in", "input",       "|15",    "User keystroke echo"            },
    { "tf", "textbox_fg",  "|15",    "Text input field foreground"    },
    { "tb", "textbox_bg",  "|17",    "Text input field background"    },
    { "hd", "heading",     "|11",    "Section headings"               },
    { "lf", "lightbar_fg", "|15",    "Lightbar selected foreground"   },
    { "lb", "lightbar_bg", "|17",    "Lightbar selected background"   },
    { "er", "error",       "|12",    "Error messages"                 },
    { "wn", "warning",     "|14",    "Warnings"                       },
    { "ok", "success",     "|10",    "Confirmations"                  },
    { "dm", "dim",         "|08",    "De-emphasized/help text"        },
    { "fi", "file_info",   "|03",    "File descriptions"              },
    { "sy", "sysop",       "|13",    "SysOp-only text"               },
    { "qt", "quote",       "|09",    "Quoted message text"            },
    { "br", "border",      "|01",    "Box borders, dividers"          },
    { "hk", "hotkey",      "|14",    "Hotkey characters"              },
    { "cd", "default",     "|16|07", "Reset to default theme color"   },
};

void maxcfg_theme_init(MaxCfgThemeColors *theme)
{
    if (!theme) return;
    memset(theme, 0, sizeof(*theme));
    snprintf(theme->name, sizeof(theme->name), "Classic Maximus");

    for (int i = 0; i < MCI_THEME_SLOT_COUNT; i++) {
        snprintf(theme->slots[i].code,  sizeof(theme->slots[i].code),  "%s", g_theme_defaults[i].code);
        snprintf(theme->slots[i].key,   sizeof(theme->slots[i].key),   "%s", g_theme_defaults[i].key);
        snprintf(theme->slots[i].value, sizeof(theme->slots[i].value), "%s", g_theme_defaults[i].value);
        snprintf(theme->slots[i].desc,  sizeof(theme->slots[i].desc),  "%s", g_theme_defaults[i].desc);
    }
}

MaxCfgStatus maxcfg_theme_load_from_toml(MaxCfgThemeColors *theme,
                                          const MaxCfgToml *toml,
                                          const char *prefix)
{
    if (!theme || !toml || !prefix) return MAXCFG_ERR_INVALID_ARGUMENT;

    /* Start with defaults */
    maxcfg_theme_init(theme);

    /* Override theme name if present */
    char path[128];
    MaxCfgVar v;
    snprintf(path, sizeof(path), "%s.theme.name", prefix);
    if (maxcfg_toml_get(toml, path, &v) == MAXCFG_OK
        && v.type == MAXCFG_VAR_STRING && v.v.s)
        snprintf(theme->name, sizeof(theme->name), "%s", v.v.s);

    /* Override each slot value from <prefix>.theme.colors.<key> */
    for (int i = 0; i < MCI_THEME_SLOT_COUNT; i++) {
        snprintf(path, sizeof(path), "%s.theme.colors.%s", prefix, theme->slots[i].key);
        if (maxcfg_toml_get(toml, path, &v) == MAXCFG_OK
            && v.type == MAXCFG_VAR_STRING && v.v.s) {
            snprintf(theme->slots[i].value, sizeof(theme->slots[i].value), "%s", v.v.s);
        }
    }

    return MAXCFG_OK;
}

const char *maxcfg_theme_lookup(const MaxCfgThemeColors *theme,
                                 char a, char b)
{
    if (!theme) return NULL;
    for (int i = 0; i < MCI_THEME_SLOT_COUNT; i++) {
        if (theme->slots[i].code[0] == a && theme->slots[i].code[1] == b)
            return theme->slots[i].value;
    }
    return NULL;
}

MaxCfgStatus maxcfg_theme_write_toml(FILE *fp, const MaxCfgThemeColors *theme)
{
    if (!fp || !theme) return MAXCFG_ERR_INVALID_ARGUMENT;

    fprintf(fp, "[theme]\n");
    fprintf(fp, "name = \"%s\"\n\n", theme->name);
    fprintf(fp, "[theme.colors]\n");
    for (int i = 0; i < MCI_THEME_SLOT_COUNT; i++) {
        fprintf(fp, "%-12s = \"%s\"%*s# |%s - %s\n",
                theme->slots[i].key,
                theme->slots[i].value,
                (int)(8 - strlen(theme->slots[i].value)), "",
                theme->slots[i].code,
                theme->slots[i].desc);
    }

    return MAXCFG_OK;
}

void maxcfg_ng_color_to_mci(const MaxCfgNgColor *color, char *out, size_t out_sz)
{
    if (!color || !out || out_sz < 4) { if (out && out_sz) out[0] = '\0'; return; }

    int fg = color->fg & 0x0f;
    int bg = color->bg & 0x07;
    int pos = snprintf(out, out_sz, "|%02d", fg);

    if (bg > 0 && pos < (int)out_sz)
        pos += snprintf(out + pos, out_sz - (size_t)pos, "|%02d", 16 + bg);

    if (color->blink && pos < (int)out_sz)
        snprintf(out + pos, out_sz - (size_t)pos, "|24");
}

MaxCfgStatus maxcfg_ng_menu_init(MaxCfgNgMenu *menu)
{
    if (menu == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(menu, 0, sizeof(*menu));
    return MAXCFG_OK;
}

void maxcfg_ng_menu_free(MaxCfgNgMenu *menu)
{
    if (menu == NULL) {
        return;
    }

    maxcfg_free_and_null(&menu->name);
    maxcfg_free_and_null(&menu->title);
    maxcfg_free_and_null(&menu->header_file);
    maxcfg_free_and_null(&menu->menu_file);

    maxcfg_free_strv(&menu->header_types, &menu->header_type_count);
    maxcfg_free_strv(&menu->menu_types, &menu->menu_type_count);

    if (menu->custom_menu != NULL) {
        free(menu->custom_menu);
        menu->custom_menu = NULL;
    }

    if (menu->options != NULL) {
        for (size_t i = 0; i < menu->option_count; i++) {
            MaxCfgNgMenuOption *opt = &menu->options[i];
            maxcfg_free_and_null(&opt->command);
            maxcfg_free_and_null(&opt->arguments);
            maxcfg_free_and_null(&opt->priv_level);
            maxcfg_free_and_null(&opt->description);
            maxcfg_free_and_null(&opt->key_poke);

            maxcfg_free_strv(&opt->modifiers, &opt->modifier_count);
        }

        free(menu->options);
    }

    memset(menu, 0, sizeof(*menu));
}

static void ng_custom_menu_set_defaults(MaxCfgNgCustomMenu *cm)
{
    if (cm == NULL) {
        return;
    }

    memset(cm, 0, sizeof(*cm));
    cm->enabled = true;
    cm->skip_canned_menu = false;
    cm->show_title = true;
    cm->lightbar_menu = false;
    cm->lightbar_margin = 1;

    cm->lightbar_normal_attr = 0x07;
    cm->lightbar_selected_attr = 0x1e;
    cm->lightbar_high_attr = 0;
    cm->lightbar_high_selected_attr = 0;

    cm->option_spacing = false;
    cm->option_justify = 0;
    cm->boundary_justify = 0;
    cm->boundary_vjustify = 0;
    cm->boundary_layout = 0;
}

static void ng_custom_menu_parse_justify(MaxCfgNgCustomMenu *cm, const char *s)
{
    if (cm == NULL || s == NULL || *s == '\0') {
        return;
    }
    if (ng_stricmp(s, "left") == 0) {
        cm->option_justify = 0;
    } else if (ng_stricmp(s, "center") == 0) {
        cm->option_justify = 1;
    } else if (ng_stricmp(s, "right") == 0) {
        cm->option_justify = 2;
    }
}

static void ng_custom_menu_parse_boundary_justify(MaxCfgNgCustomMenu *cm, const char *s)
{
    char buf[64];
    char *h;
    char *vert;

    if (cm == NULL || s == NULL || *s == '\0') {
        return;
    }

    strncpy(buf, s, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    h = buf;
    while (*h == ' ' || *h == '\t') {
        h++;
    }

    vert = h;
    while (*vert && *vert != ' ' && *vert != '\t') {
        vert++;
    }
    if (*vert) {
        *vert++ = '\0';
        while (*vert == ' ' || *vert == '\t') {
            vert++;
        }
        if (*vert == '\0') {
            vert = NULL;
        }
    } else {
        vert = NULL;
    }

    if (ng_stricmp(h, "left") == 0) {
        cm->boundary_justify = 0;
        cm->boundary_vjustify = 0;
    } else if (ng_stricmp(h, "center") == 0) {
        cm->boundary_justify = 1;
        cm->boundary_vjustify = 1;
    } else if (ng_stricmp(h, "right") == 0) {
        cm->boundary_justify = 2;
        cm->boundary_vjustify = 0;
    }

    if (vert) {
        if (ng_stricmp(vert, "top") == 0) {
            cm->boundary_vjustify = 0;
        } else if (ng_stricmp(vert, "center") == 0) {
            cm->boundary_vjustify = 1;
        } else if (ng_stricmp(vert, "bottom") == 0) {
            cm->boundary_vjustify = 2;
        }
    }
}

static void ng_custom_menu_parse_boundary_layout(MaxCfgNgCustomMenu *cm, const char *s)
{
    if (cm == NULL || s == NULL || *s == '\0') {
        return;
    }
    if (ng_stricmp(s, "grid") == 0) {
        cm->boundary_layout = 0;
    } else if (ng_stricmp(s, "tight") == 0) {
        cm->boundary_layout = 1;
    } else if (ng_stricmp(s, "spread") == 0) {
        cm->boundary_layout = 2;
    } else if (ng_stricmp(s, "spread_width") == 0) {
        cm->boundary_layout = 3;
    } else if (ng_stricmp(s, "spread_height") == 0) {
        cm->boundary_layout = 4;
    }
}

static void ng_custom_menu_set_location_from_view(int *out_row, int *out_col, const MaxCfgIntView *v)
{
    if (out_row == NULL || out_col == NULL || v == NULL) {
        return;
    }
    if (v->items != NULL && v->count >= 2) {
        int row = v->items[0];
        int col = v->items[1];
        if (row > 0 && col > 0) {
            *out_row = row;
            *out_col = col;
        }
    }
}

static void ng_custom_menu_parse_lightbar_color_pair(const MaxCfgVar *tbl, const char *key,
                                                     unsigned char *out_attr, bool *out_has)
{
    MaxCfgStrView vv = {0};
    if (out_has) {
        *out_has = false;
    }

    if (tbl == NULL || key == NULL || out_attr == NULL || out_has == NULL) {
        return;
    }

    if (ng_tbl_get_string_array_view(tbl, key, &vv) != MAXCFG_OK || vv.count < 2) {
        return;
    }

    int fg = maxcfg_dos_color_from_name(vv.items[0]);
    int bg = maxcfg_dos_color_from_name(vv.items[1]);
    if (fg < 0 || bg < 0) {
        return;
    }

    *out_attr = maxcfg_make_attr(fg, bg);
    *out_has = true;
}

static MaxCfgStatus maxcfg_ng_menu_ensure_capacity(MaxCfgNgMenu *menu, size_t want)
{
    if (menu == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (want <= menu->option_capacity) {
        return MAXCFG_OK;
    }

    size_t new_cap = (menu->option_capacity == 0) ? 8u : menu->option_capacity;
    while (new_cap < want) {
        new_cap *= 2u;
    }

    MaxCfgNgMenuOption *new_items = (MaxCfgNgMenuOption *)realloc(menu->options, new_cap * sizeof(*new_items));
    if (new_items == NULL) {
        return MAXCFG_ERR_OOM;
    }

    memset(new_items + menu->option_capacity, 0, (new_cap - menu->option_capacity) * sizeof(*new_items));
    menu->options = new_items;
    menu->option_capacity = new_cap;
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_menu_add_option(MaxCfgNgMenu *menu, const MaxCfgNgMenuOption *opt)
{
    MaxCfgStatus st;
    MaxCfgNgMenuOption *dst;

    if (menu == NULL || opt == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    st = maxcfg_ng_menu_ensure_capacity(menu, menu->option_count + 1u);
    if (st != MAXCFG_OK) {
        return st;
    }

    dst = &menu->options[menu->option_count];
    memset(dst, 0, sizeof(*dst));

    st = maxcfg_strdup_safe(&dst->command, opt->command);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->arguments, opt->arguments);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->priv_level, opt->priv_level);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->description, opt->description);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->key_poke, opt->key_poke);
    if (st != MAXCFG_OK) goto fail;

    if (opt->modifier_count > 0 && opt->modifiers != NULL) {
        dst->modifiers = (char **)calloc(opt->modifier_count, sizeof(dst->modifiers[0]));
        if (dst->modifiers == NULL) {
            st = MAXCFG_ERR_OOM;
            goto fail;
        }
        dst->modifier_count = opt->modifier_count;
        for (size_t i = 0; i < opt->modifier_count; i++) {
            if (opt->modifiers[i] == NULL) {
                dst->modifiers[i] = strdup("");
            } else {
                dst->modifiers[i] = strdup(opt->modifiers[i]);
            }
            if (dst->modifiers[i] == NULL) {
                st = MAXCFG_ERR_OOM;
                goto fail;
            }
        }
    }

    menu->option_count++;
    return MAXCFG_OK;

fail:
    maxcfg_free_and_null(&dst->command);
    maxcfg_free_and_null(&dst->arguments);
    maxcfg_free_and_null(&dst->priv_level);
    maxcfg_free_and_null(&dst->description);
    maxcfg_free_and_null(&dst->key_poke);
    maxcfg_free_strv(&dst->modifiers, &dst->modifier_count);
    return st;
}

MaxCfgStatus maxcfg_ng_division_list_init(MaxCfgNgDivisionList *list)
{
    if (list == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(list, 0, sizeof(*list));
    return MAXCFG_OK;
}

void maxcfg_ng_division_list_free(MaxCfgNgDivisionList *list)
{
    if (list == NULL) {
        return;
    }

    if (list->items != NULL) {
        for (size_t i = 0; i < list->count; i++) {
            MaxCfgNgDivision *d = &list->items[i];
            maxcfg_free_and_null(&d->name);
            maxcfg_free_and_null(&d->key);
            maxcfg_free_and_null(&d->description);
            maxcfg_free_and_null(&d->acs);
            maxcfg_free_and_null(&d->display_file);
        }
        free(list->items);
    }

    memset(list, 0, sizeof(*list));
}

static MaxCfgStatus maxcfg_ng_division_list_ensure_capacity(MaxCfgNgDivisionList *list, size_t want)
{
    if (list == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (want <= list->capacity) {
        return MAXCFG_OK;
    }

    size_t new_cap = (list->capacity == 0) ? 16u : list->capacity;
    while (new_cap < want) {
        new_cap *= 2u;
    }

    MaxCfgNgDivision *new_items = (MaxCfgNgDivision *)realloc(list->items, new_cap * sizeof(*new_items));
    if (new_items == NULL) {
        return MAXCFG_ERR_OOM;
    }

    memset(new_items + list->capacity, 0, (new_cap - list->capacity) * sizeof(*new_items));
    list->items = new_items;
    list->capacity = new_cap;
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_division_list_add(MaxCfgNgDivisionList *list, const MaxCfgNgDivision *div)
{
    MaxCfgStatus st;
    MaxCfgNgDivision *dst;

    if (list == NULL || div == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    st = maxcfg_ng_division_list_ensure_capacity(list, list->count + 1u);
    if (st != MAXCFG_OK) {
        return st;
    }

    dst = &list->items[list->count];
    memset(dst, 0, sizeof(*dst));

    st = maxcfg_strdup_safe(&dst->name, div->name);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->key, div->key);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->description, div->description);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->acs, div->acs);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->display_file, div->display_file);
    if (st != MAXCFG_OK) goto fail;
    dst->level = div->level;

    list->count++;
    return MAXCFG_OK;

fail:
    maxcfg_free_and_null(&dst->name);
    maxcfg_free_and_null(&dst->key);
    maxcfg_free_and_null(&dst->description);
    maxcfg_free_and_null(&dst->acs);
    maxcfg_free_and_null(&dst->display_file);
    return st;
}

MaxCfgStatus maxcfg_ng_protocol_list_init(MaxCfgNgProtocolList *list)
{
    if (list == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(list, 0, sizeof(*list));
    return MAXCFG_OK;
}

void maxcfg_ng_protocol_list_free(MaxCfgNgProtocolList *list)
{
    if (list == NULL) {
        return;
    }

    maxcfg_free_and_null(&list->protocol_max_path);
    maxcfg_free_and_null(&list->protocol_ctl_path);

    if (list->items != NULL) {
        for (size_t i = 0; i < list->count; i++) {
            MaxCfgNgProtocol *p = &list->items[i];
            maxcfg_free_and_null(&p->name);
            maxcfg_free_and_null(&p->program);
            maxcfg_free_and_null(&p->log_file);
            maxcfg_free_and_null(&p->control_file);
            maxcfg_free_and_null(&p->download_cmd);
            maxcfg_free_and_null(&p->upload_cmd);
            maxcfg_free_and_null(&p->download_string);
            maxcfg_free_and_null(&p->upload_string);
            maxcfg_free_and_null(&p->download_keyword);
            maxcfg_free_and_null(&p->upload_keyword);
        }
        free(list->items);
    }

    memset(list, 0, sizeof(*list));
}

static MaxCfgStatus maxcfg_ng_protocol_list_ensure_capacity(MaxCfgNgProtocolList *list, size_t want)
{
    if (list == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (want <= list->capacity) {
        return MAXCFG_OK;
    }

    size_t new_cap = (list->capacity == 0) ? 16u : list->capacity;
    while (new_cap < want) {
        new_cap *= 2u;
    }

    MaxCfgNgProtocol *new_items = (MaxCfgNgProtocol *)realloc(list->items, new_cap * sizeof(*new_items));
    if (new_items == NULL) {
        return MAXCFG_ERR_OOM;
    }

    memset(new_items + list->capacity, 0, (new_cap - list->capacity) * sizeof(*new_items));
    list->items = new_items;
    list->capacity = new_cap;
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_protocol_list_add(MaxCfgNgProtocolList *list, const MaxCfgNgProtocol *proto)
{
    MaxCfgStatus st;
    MaxCfgNgProtocol *dst;

    if (list == NULL || proto == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    st = maxcfg_ng_protocol_list_ensure_capacity(list, list->count + 1u);
    if (st != MAXCFG_OK) {
        return st;
    }

    dst = &list->items[list->count];
    memset(dst, 0, sizeof(*dst));

    st = maxcfg_strdup_safe(&dst->name, proto->name);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->program, proto->program);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->log_file, proto->log_file);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->control_file, proto->control_file);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->download_cmd, proto->download_cmd);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->upload_cmd, proto->upload_cmd);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->download_string, proto->download_string);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->upload_string, proto->upload_string);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->download_keyword, proto->download_keyword);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->upload_keyword, proto->upload_keyword);
    if (st != MAXCFG_OK) goto fail;

    dst->index = proto->index;
    dst->batch = proto->batch;
    dst->exitlevel = proto->exitlevel;
    dst->filename_word = proto->filename_word;
    dst->descript_word = proto->descript_word;
    dst->opus = proto->opus;
    dst->bi = proto->bi;

    list->count++;
    return MAXCFG_OK;

fail:
    maxcfg_free_and_null(&dst->name);
    maxcfg_free_and_null(&dst->program);
    maxcfg_free_and_null(&dst->log_file);
    maxcfg_free_and_null(&dst->control_file);
    maxcfg_free_and_null(&dst->download_cmd);
    maxcfg_free_and_null(&dst->upload_cmd);
    maxcfg_free_and_null(&dst->download_string);
    maxcfg_free_and_null(&dst->upload_string);
    maxcfg_free_and_null(&dst->download_keyword);
    maxcfg_free_and_null(&dst->upload_keyword);
    return st;
}

MaxCfgStatus maxcfg_ng_language_init(MaxCfgNgLanguage *lang)
{
    if (lang == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(lang, 0, sizeof(*lang));
    return MAXCFG_OK;
}

void maxcfg_ng_language_free(MaxCfgNgLanguage *lang)
{
    if (lang == NULL) {
        return;
    }

    maxcfg_free_strv(&lang->lang_files, &lang->lang_file_count);
    memset(lang, 0, sizeof(*lang));
}

MaxCfgStatus maxcfg_ng_msg_area_list_init(MaxCfgNgMsgAreaList *list)
{
    if (list == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(list, 0, sizeof(*list));
    return MAXCFG_OK;
}

void maxcfg_ng_msg_area_list_free(MaxCfgNgMsgAreaList *list)
{
    if (list == NULL) {
        return;
    }

    if (list->items != NULL) {
        for (size_t i = 0; i < list->count; i++) {
            MaxCfgNgMsgArea *a = &list->items[i];
            maxcfg_free_and_null(&a->name);
            maxcfg_free_and_null(&a->description);
            maxcfg_free_and_null(&a->acs);
            maxcfg_free_and_null(&a->menu);
            maxcfg_free_and_null(&a->division);
            maxcfg_free_and_null(&a->tag);
            maxcfg_free_and_null(&a->path);
            maxcfg_free_and_null(&a->owner);
            maxcfg_free_and_null(&a->origin);
            maxcfg_free_and_null(&a->attach_path);
            maxcfg_free_and_null(&a->barricade);
            maxcfg_free_strv(&a->style, &a->style_count);
        }
        free(list->items);
    }

    memset(list, 0, sizeof(*list));
}

static MaxCfgStatus maxcfg_ng_msg_area_list_ensure_capacity(MaxCfgNgMsgAreaList *list, size_t want)
{
    if (list == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (want <= list->capacity) {
        return MAXCFG_OK;
    }

    size_t new_cap = (list->capacity == 0) ? 32u : list->capacity;
    while (new_cap < want) {
        new_cap *= 2u;
    }

    MaxCfgNgMsgArea *new_items = (MaxCfgNgMsgArea *)realloc(list->items, new_cap * sizeof(*new_items));
    if (new_items == NULL) {
        return MAXCFG_ERR_OOM;
    }

    memset(new_items + list->capacity, 0, (new_cap - list->capacity) * sizeof(*new_items));
    list->items = new_items;
    list->capacity = new_cap;
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_msg_area_list_add(MaxCfgNgMsgAreaList *list, const MaxCfgNgMsgArea *area)
{
    MaxCfgStatus st;
    MaxCfgNgMsgArea *dst;

    if (list == NULL || area == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    st = maxcfg_ng_msg_area_list_ensure_capacity(list, list->count + 1u);
    if (st != MAXCFG_OK) {
        return st;
    }

    dst = &list->items[list->count];
    memset(dst, 0, sizeof(*dst));

    st = maxcfg_strdup_safe(&dst->name, area->name);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->description, area->description);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->acs, area->acs);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->menu, area->menu);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->division, area->division);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->tag, area->tag);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->path, area->path);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->owner, area->owner);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->origin, area->origin);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->attach_path, area->attach_path);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->barricade, area->barricade);
    if (st != MAXCFG_OK) goto fail;

    st = maxcfg_copy_strv(&dst->style, &dst->style_count, area->style, area->style_count);
    if (st != MAXCFG_OK) goto fail;
    dst->renum_max = area->renum_max;
    dst->renum_days = area->renum_days;

    list->count++;
    return MAXCFG_OK;

fail:
    maxcfg_free_and_null(&dst->name);
    maxcfg_free_and_null(&dst->description);
    maxcfg_free_and_null(&dst->acs);
    maxcfg_free_and_null(&dst->menu);
    maxcfg_free_and_null(&dst->division);
    maxcfg_free_and_null(&dst->tag);
    maxcfg_free_and_null(&dst->path);
    maxcfg_free_and_null(&dst->owner);
    maxcfg_free_and_null(&dst->origin);
    maxcfg_free_and_null(&dst->attach_path);
    maxcfg_free_and_null(&dst->barricade);
    maxcfg_free_strv(&dst->style, &dst->style_count);
    return st;
}

MaxCfgStatus maxcfg_ng_file_area_list_init(MaxCfgNgFileAreaList *list)
{
    if (list == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(list, 0, sizeof(*list));
    return MAXCFG_OK;
}

void maxcfg_ng_file_area_list_free(MaxCfgNgFileAreaList *list)
{
    if (list == NULL) {
        return;
    }

    if (list->items != NULL) {
        for (size_t i = 0; i < list->count; i++) {
            MaxCfgNgFileArea *a = &list->items[i];
            maxcfg_free_and_null(&a->name);
            maxcfg_free_and_null(&a->description);
            maxcfg_free_and_null(&a->acs);
            maxcfg_free_and_null(&a->menu);
            maxcfg_free_and_null(&a->division);
            maxcfg_free_and_null(&a->download);
            maxcfg_free_and_null(&a->upload);
            maxcfg_free_and_null(&a->filelist);
            maxcfg_free_and_null(&a->barricade);
            maxcfg_free_strv(&a->types, &a->type_count);
        }
        free(list->items);
    }

    memset(list, 0, sizeof(*list));
}

static MaxCfgStatus maxcfg_ng_file_area_list_ensure_capacity(MaxCfgNgFileAreaList *list, size_t want)
{
    if (list == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (want <= list->capacity) {
        return MAXCFG_OK;
    }

    size_t new_cap = (list->capacity == 0) ? 32u : list->capacity;
    while (new_cap < want) {
        new_cap *= 2u;
    }

    MaxCfgNgFileArea *new_items = (MaxCfgNgFileArea *)realloc(list->items, new_cap * sizeof(*new_items));
    if (new_items == NULL) {
        return MAXCFG_ERR_OOM;
    }

    memset(new_items + list->capacity, 0, (new_cap - list->capacity) * sizeof(*new_items));
    list->items = new_items;
    list->capacity = new_cap;
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_file_area_list_add(MaxCfgNgFileAreaList *list, const MaxCfgNgFileArea *area)
{
    MaxCfgStatus st;
    MaxCfgNgFileArea *dst;

    if (list == NULL || area == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    st = maxcfg_ng_file_area_list_ensure_capacity(list, list->count + 1u);
    if (st != MAXCFG_OK) {
        return st;
    }

    dst = &list->items[list->count];
    memset(dst, 0, sizeof(*dst));

    st = maxcfg_strdup_safe(&dst->name, area->name);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->description, area->description);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->acs, area->acs);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->menu, area->menu);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->division, area->division);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->download, area->download);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->upload, area->upload);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->filelist, area->filelist);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->barricade, area->barricade);
    if (st != MAXCFG_OK) goto fail;

    st = maxcfg_copy_strv(&dst->types, &dst->type_count, area->types, area->type_count);
    if (st != MAXCFG_OK) goto fail;

    list->count++;
    return MAXCFG_OK;

fail:
    maxcfg_free_and_null(&dst->name);
    maxcfg_free_and_null(&dst->description);
    maxcfg_free_and_null(&dst->acs);
    maxcfg_free_and_null(&dst->menu);
    maxcfg_free_and_null(&dst->division);
    maxcfg_free_and_null(&dst->download);
    maxcfg_free_and_null(&dst->upload);
    maxcfg_free_and_null(&dst->filelist);
    maxcfg_free_and_null(&dst->barricade);
    maxcfg_free_strv(&dst->types, &dst->type_count);
    return st;
}

MaxCfgStatus maxcfg_ng_access_level_list_init(MaxCfgNgAccessLevelList *list)
{
    if (list == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(list, 0, sizeof(*list));
    return MAXCFG_OK;
}

void maxcfg_ng_access_level_list_free(MaxCfgNgAccessLevelList *list)
{
    if (list == NULL) {
        return;
    }

    if (list->items != NULL) {
        for (size_t i = 0; i < list->count; i++) {
            MaxCfgNgAccessLevel *lvl = &list->items[i];
            maxcfg_free_and_null(&lvl->name);
            maxcfg_free_and_null(&lvl->description);
            maxcfg_free_and_null(&lvl->alias);
            maxcfg_free_and_null(&lvl->key);
            maxcfg_free_and_null(&lvl->login_file);
            maxcfg_free_strv(&lvl->flags, &lvl->flag_count);
            maxcfg_free_strv(&lvl->mail_flags, &lvl->mail_flag_count);
        }
        free(list->items);
    }

    memset(list, 0, sizeof(*list));
}

static MaxCfgStatus maxcfg_ng_access_level_list_ensure_capacity(MaxCfgNgAccessLevelList *list, size_t want)
{
    if (list == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    if (want <= list->capacity) {
        return MAXCFG_OK;
    }

    size_t new_cap = (list->capacity == 0) ? 16u : list->capacity;
    while (new_cap < want) {
        new_cap *= 2u;
    }

    MaxCfgNgAccessLevel *new_items = (MaxCfgNgAccessLevel *)realloc(list->items, new_cap * sizeof(*new_items));
    if (new_items == NULL) {
        return MAXCFG_ERR_OOM;
    }

    memset(new_items + list->capacity, 0, (new_cap - list->capacity) * sizeof(*new_items));
    list->items = new_items;
    list->capacity = new_cap;
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_access_level_list_add(MaxCfgNgAccessLevelList *list, const MaxCfgNgAccessLevel *lvl)
{
    MaxCfgStatus st;
    MaxCfgNgAccessLevel *dst;

    if (list == NULL || lvl == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    st = maxcfg_ng_access_level_list_ensure_capacity(list, list->count + 1u);
    if (st != MAXCFG_OK) {
        return st;
    }

    dst = &list->items[list->count];
    memset(dst, 0, sizeof(*dst));

    st = maxcfg_strdup_safe(&dst->name, lvl->name);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->description, lvl->description);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->alias, lvl->alias);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->key, lvl->key);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_strdup_safe(&dst->login_file, lvl->login_file);
    if (st != MAXCFG_OK) goto fail;

    st = maxcfg_copy_strv(&dst->flags, &dst->flag_count, lvl->flags, lvl->flag_count);
    if (st != MAXCFG_OK) goto fail;
    st = maxcfg_copy_strv(&dst->mail_flags, &dst->mail_flag_count, lvl->mail_flags, lvl->mail_flag_count);
    if (st != MAXCFG_OK) goto fail;

    dst->level = lvl->level;
    dst->time = lvl->time;
    dst->cume = lvl->cume;
    dst->calls = lvl->calls;
    dst->logon_baud = lvl->logon_baud;
    dst->xfer_baud = lvl->xfer_baud;
    dst->file_limit = lvl->file_limit;
    dst->file_ratio = lvl->file_ratio;
    dst->ratio_free = lvl->ratio_free;
    dst->upload_reward = lvl->upload_reward;
    dst->user_flags = lvl->user_flags;
    dst->oldpriv = lvl->oldpriv;

    list->count++;
    return MAXCFG_OK;

fail:
    maxcfg_free_and_null(&dst->name);
    maxcfg_free_and_null(&dst->description);
    maxcfg_free_and_null(&dst->alias);
    maxcfg_free_and_null(&dst->key);
    maxcfg_free_and_null(&dst->login_file);
    maxcfg_free_strv(&dst->flags, &dst->flag_count);
    maxcfg_free_strv(&dst->mail_flags, &dst->mail_flag_count);
    return st;
}

MaxCfgStatus maxcfg_ng_write_maximus_toml(FILE *fp, const MaxCfgNgSystem *sys)
{
    if (fp == NULL || sys == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    toml_kv_int(fp, "config_version", sys->config_version);
    toml_kv_string(fp, "system_name", sys->system_name);
    toml_kv_string(fp, "sysop", sys->sysop);
    toml_kv_int(fp, "task_num", sys->task_num);
    toml_kv_string(fp, "video", sys->video);
    toml_kv_bool(fp, "has_snow", sys->has_snow);
    toml_kv_string(fp, "multitasker", sys->multitasker);

    fprintf(fp, "\n# === Core Paths ===\n");
    fprintf(fp, "# sys_path is the ONLY absolute path — all others are relative to it\n");
    toml_kv_string(fp, "sys_path", sys->sys_path);
    toml_kv_string(fp, "config_path", sys->config_path);

    fprintf(fp, "\n# === Display ===\n");
    toml_kv_string(fp, "display_path", sys->display_path);

    fprintf(fp, "\n# === Scripts ===\n");
    toml_kv_string(fp, "mex_path", sys->mex_path);

    fprintf(fp, "\n# === Language ===\n");
    toml_kv_string(fp, "lang_path", sys->lang_path);

    fprintf(fp, "\n# === Data ===\n");
    toml_kv_string(fp, "data_path", sys->data_path);
    toml_kv_string(fp, "file_password", sys->file_password);
    toml_kv_string(fp, "file_callers", sys->file_callers);
    toml_kv_string(fp, "file_access", sys->file_access);
    toml_kv_string(fp, "message_data", sys->message_data);
    toml_kv_string(fp, "file_data", sys->file_data);
    toml_kv_string(fp, "net_info_path", sys->net_info_path);
    toml_kv_string(fp, "outbound_path", sys->outbound_path);
    toml_kv_string(fp, "inbound_path", sys->inbound_path);

    fprintf(fp, "\n# === Runtime ===\n");
    toml_kv_string(fp, "run_path", sys->run_path);
    toml_kv_string(fp, "node_path", sys->node_path);
    toml_kv_string(fp, "temp_path", sys->temp_path);
    toml_kv_string(fp, "stage_path", sys->stage_path);
    toml_kv_string(fp, "doors_path", sys->doors_path);

    fprintf(fp, "\n# === Logging ===\n");
    toml_kv_string(fp, "log_file", sys->log_file);
    toml_kv_string(fp, "log_mode", sys->log_mode);

    fprintf(fp, "\n# === System Settings ===\n");
    toml_kv_string(fp, "msg_reader_menu", sys->msg_reader_menu);
    toml_kv_int(fp, "mcp_sessions", sys->mcp_sessions);
    toml_kv_bool(fp, "snoop", sys->snoop);
    toml_kv_bool(fp, "no_password_encryption", sys->no_password_encryption);
    toml_kv_bool(fp, "no_share", sys->no_share);
    toml_kv_bool(fp, "reboot", sys->reboot);
    toml_kv_bool(fp, "swap", sys->swap);
    toml_kv_bool(fp, "dos_close", sys->dos_close);
    toml_kv_bool(fp, "local_input_timeout", sys->local_input_timeout);
    toml_kv_bool(fp, "status_line", sys->status_line);
    fprintf(fp, "\n");
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_write_general_session_toml(FILE *fp, const MaxCfgNgGeneralSession *session)
{
    if (fp == NULL || session == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    toml_kv_bool(fp, "alias_system", session->alias_system);
    toml_kv_bool(fp, "ask_alias", session->ask_alias);
    toml_kv_bool(fp, "single_word_names", session->single_word_names);
    toml_kv_bool(fp, "check_ansi", session->check_ansi);
    toml_kv_bool(fp, "check_rip", session->check_rip);
    toml_kv_bool(fp, "ask_phone", session->ask_phone);
    toml_kv_bool(fp, "no_real_name", session->no_real_name);

    toml_kv_bool(fp, "disable_userlist", session->disable_userlist);
    toml_kv_bool(fp, "disable_magnet", session->disable_magnet);
    toml_kv_string(fp, "edit_menu", session->edit_menu);

    toml_kv_bool(fp, "autodate", session->autodate);
    toml_kv_int(fp, "date_style", session->date_style);
    toml_kv_int(fp, "filelist_margin", session->filelist_margin);
    toml_kv_int(fp, "exit_after_call", session->exit_after_call);

    toml_kv_string(fp, "chat_program", session->chat_program);
    toml_kv_string(fp, "local_editor", session->local_editor);
    toml_kv_bool(fp, "yell_enabled", session->yell_enabled);
    toml_kv_bool(fp, "compat_local_baud_9600", session->compat_local_baud_9600);
    toml_kv_uint(fp, "min_free_kb", session->min_free_kb);
    toml_kv_string(fp, "upload_log", session->upload_log);
    toml_kv_string(fp, "virus_check", session->virus_check);
    toml_kv_int(fp, "mailchecker_reply_priv", session->mailchecker_reply_priv);
    toml_kv_int(fp, "mailchecker_kill_priv", session->mailchecker_kill_priv);
    toml_kv_string(fp, "comment_area", session->comment_area);
    toml_kv_string(fp, "highest_message_area", session->highest_message_area);
    toml_kv_string(fp, "highest_file_area", session->highest_file_area);
    toml_kv_string(fp, "area_change_keys", session->area_change_keys);

    toml_kv_bool(fp, "chat_capture", session->chat_capture);
    toml_kv_bool(fp, "strict_xfer", session->strict_xfer);
    toml_kv_bool(fp, "gate_netmail", session->gate_netmail);
    toml_kv_bool(fp, "global_high_bit", session->global_high_bit);
    toml_kv_bool(fp, "upload_check_dupe", session->upload_check_dupe);
    toml_kv_bool(fp, "upload_check_dupe_extension", session->upload_check_dupe_extension);
    toml_kv_bool(fp, "use_umsgids", session->use_umsgids);

    toml_kv_int(fp, "logon_priv", session->logon_priv);
    toml_kv_int(fp, "logon_timelimit", session->logon_timelimit);
    toml_kv_int(fp, "min_logon_baud", session->min_logon_baud);
    toml_kv_int(fp, "min_graphics_baud", session->min_graphics_baud);
    toml_kv_int(fp, "min_rip_baud", session->min_rip_baud);
    toml_kv_int(fp, "input_timeout", session->input_timeout);

    toml_kv_uint(fp, "max_msgsize", session->max_msgsize);
    toml_kv_string(fp, "kill_private", session->kill_private);
    toml_kv_string(fp, "charset", session->charset);
    toml_kv_string_array(fp, "save_directories", session->save_directories, session->save_directory_count);

    toml_kv_string(fp, "track_privview", session->track_privview);
    toml_kv_string(fp, "track_privmod", session->track_privmod);
    toml_kv_string(fp, "track_exclude", session->track_exclude);
    toml_kv_string(fp, "attach_base", session->attach_base);
    toml_kv_string(fp, "attach_path", session->attach_path);
    toml_kv_string(fp, "attach_archiver", session->attach_archiver);
    toml_kv_string(fp, "kill_attach", session->kill_attach);
    toml_kv_int(fp, "msg_localattach_priv", session->msg_localattach_priv);
    toml_kv_int(fp, "kill_attach_priv", session->kill_attach_priv);

    toml_kv_string(fp, "first_menu", session->first_menu);
    toml_kv_string(fp, "first_file_area", session->first_file_area);
    toml_kv_string(fp, "first_message_area", session->first_message_area);
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_matrix_init(MaxCfgNgMatrix *matrix)
{
    if (matrix == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(matrix, 0, sizeof(*matrix));
    return MAXCFG_OK;
}

void maxcfg_ng_matrix_free(MaxCfgNgMatrix *matrix)
{
    if (matrix == NULL) {
        return;
    }

    maxcfg_free_and_null(&matrix->nodelist_version);
    maxcfg_free_and_null(&matrix->fidouser);
    maxcfg_free_and_null(&matrix->echotoss_name);

    if (matrix->message_edit_ask) {
        for (size_t i = 0; i < matrix->message_edit_ask_count; i++) {
            maxcfg_free_and_null(&matrix->message_edit_ask[i].attribute);
        }
        free(matrix->message_edit_ask);
        matrix->message_edit_ask = NULL;
        matrix->message_edit_ask_count = 0;
    }

    if (matrix->message_edit_assume) {
        for (size_t i = 0; i < matrix->message_edit_assume_count; i++) {
            maxcfg_free_and_null(&matrix->message_edit_assume[i].attribute);
        }
        free(matrix->message_edit_assume);
        matrix->message_edit_assume = NULL;
        matrix->message_edit_assume_count = 0;
    }

    free(matrix->addresses);
    matrix->addresses = NULL;
    matrix->address_count = 0;
    memset(matrix, 0, sizeof(*matrix));
}

MaxCfgStatus maxcfg_ng_reader_init(MaxCfgNgReader *reader)
{
    if (reader == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(reader, 0, sizeof(*reader));
    return MAXCFG_OK;
}

void maxcfg_ng_reader_free(MaxCfgNgReader *reader)
{
    if (reader == NULL) {
        return;
    }

    maxcfg_free_and_null(&reader->archivers_ctl);
    maxcfg_free_and_null(&reader->packet_name);
    maxcfg_free_and_null(&reader->work_directory);
    maxcfg_free_and_null(&reader->phone);
    memset(reader, 0, sizeof(*reader));
}

MaxCfgStatus maxcfg_ng_equipment_init(MaxCfgNgEquipment *equip)
{
    if (equip == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    memset(equip, 0, sizeof(*equip));
    return MAXCFG_OK;
}

void maxcfg_ng_equipment_free(MaxCfgNgEquipment *equip)
{
    if (equip == NULL) {
        return;
    }

    maxcfg_free_and_null(&equip->output);
    maxcfg_free_and_null(&equip->busy);
    maxcfg_free_and_null(&equip->init);
    maxcfg_free_and_null(&equip->ring);
    maxcfg_free_and_null(&equip->answer);
    maxcfg_free_and_null(&equip->connect);
    maxcfg_free_strv(&equip->handshaking, &equip->handshaking_count);
    memset(equip, 0, sizeof(*equip));
}

MaxCfgStatus maxcfg_ng_write_matrix_toml(FILE *fp, const MaxCfgNgMatrix *matrix)
{
    if (fp == NULL || matrix == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    toml_kv_int(fp, "ctla_priv", matrix->ctla_priv);
    toml_kv_int(fp, "seenby_priv", matrix->seenby_priv);
    toml_kv_int(fp, "private_priv", matrix->private_priv);
    toml_kv_int(fp, "fromfile_priv", matrix->fromfile_priv);

    toml_kv_int(fp, "unlisted_priv", matrix->unlisted_priv);
    toml_kv_int(fp, "unlisted_cost", matrix->unlisted_cost);

    toml_kv_bool(fp, "log_echomail", matrix->log_echomail);

    toml_kv_int(fp, "after_edit_exit", matrix->after_edit_exit);
    toml_kv_int(fp, "after_echomail_exit", matrix->after_echomail_exit);
    toml_kv_int(fp, "after_local_exit", matrix->after_local_exit);

    toml_kv_string(fp, "nodelist_version", matrix->nodelist_version);
    toml_kv_string(fp, "fidouser", matrix->fidouser);
    toml_kv_string(fp, "echotoss_name", matrix->echotoss_name);

    if (matrix->message_edit_ask_count > 0) {
        fprintf(fp, "\n[message_edit.ask]\n");
        for (size_t i = 0; i < matrix->message_edit_ask_count; i++) {
            if (matrix->message_edit_ask[i].attribute) {
                toml_kv_int(fp, matrix->message_edit_ask[i].attribute, matrix->message_edit_ask[i].priv);
            }
        }
    }

    if (matrix->message_edit_assume_count > 0) {
        fprintf(fp, "\n[message_edit.assume]\n");
        for (size_t i = 0; i < matrix->message_edit_assume_count; i++) {
            if (matrix->message_edit_assume[i].attribute) {
                toml_kv_int(fp, matrix->message_edit_assume[i].attribute, matrix->message_edit_assume[i].priv);
            }
        }
    }

    for (size_t i = 0; i < matrix->address_count; i++) {
        fprintf(fp, "\n[[address]]\n");
        toml_kv_int(fp, "zone", matrix->addresses[i].zone);
        toml_kv_int(fp, "net", matrix->addresses[i].net);
        toml_kv_int(fp, "node", matrix->addresses[i].node);
        toml_kv_int(fp, "point", matrix->addresses[i].point);
    }
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_write_language_toml(FILE *fp, const MaxCfgNgLanguage *lang)
{
    if (fp == NULL || lang == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    toml_kv_int(fp, "max_lang", lang->max_lang);
    toml_kv_string_array(fp, "lang_file", lang->lang_files, lang->lang_file_count);

    if (lang->max_ptrs != 0) toml_kv_int(fp, "max_ptrs", lang->max_ptrs);
    if (lang->max_heap != 0) toml_kv_int(fp, "max_heap", lang->max_heap);
    if (lang->max_glh_ptrs != 0) toml_kv_int(fp, "max_glh_ptrs", lang->max_glh_ptrs);
    if (lang->max_glh_len != 0) toml_kv_int(fp, "max_glh_len", lang->max_glh_len);
    if (lang->max_syh_ptrs != 0) toml_kv_int(fp, "max_syh_ptrs", lang->max_syh_ptrs);
    if (lang->max_syh_len != 0) toml_kv_int(fp, "max_syh_len", lang->max_syh_len);
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_write_reader_toml(FILE *fp, const MaxCfgNgReader *reader)
{
    if (fp == NULL || reader == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    toml_kv_int(fp, "max_pack", reader->max_pack);
    toml_kv_string(fp, "archivers_ctl", reader->archivers_ctl);
    toml_kv_string(fp, "packet_name", reader->packet_name);
    toml_kv_string(fp, "work_directory", reader->work_directory);
    toml_kv_string(fp, "phone", reader->phone);
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_write_equipment_toml(FILE *fp, const MaxCfgNgEquipment *equip)
{
    if (fp == NULL || equip == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    toml_kv_string(fp, "output", equip->output);
    toml_kv_int(fp, "com_port", equip->com_port);
    toml_kv_int(fp, "baud_maximum", equip->baud_maximum);

    toml_kv_string(fp, "busy", equip->busy);
    toml_kv_string(fp, "init", equip->init);
    toml_kv_string(fp, "ring", equip->ring);
    toml_kv_string(fp, "answer", equip->answer);
    toml_kv_string(fp, "connect", equip->connect);

    toml_kv_int(fp, "carrier_mask", equip->carrier_mask);
    toml_kv_string_array(fp, "handshaking", equip->handshaking, equip->handshaking_count);

    toml_kv_bool(fp, "send_break", equip->send_break);
    toml_kv_bool(fp, "no_critical", equip->no_critical);
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_write_protocols_toml(FILE *fp, const MaxCfgNgProtocolList *list)
{
    if (fp == NULL || list == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    toml_kv_int(fp, "protoexit", list->protoexit);
    toml_kv_string(fp, "protocol_max_path", list->protocol_max_path);
    toml_kv_bool(fp, "protocol_max_exists", list->protocol_max_exists);
    toml_kv_string(fp, "protocol_ctl_path", list->protocol_ctl_path);
    toml_kv_bool(fp, "protocol_ctl_exists", list->protocol_ctl_exists);

    for (size_t i = 0; i < list->count; i++) {
        const MaxCfgNgProtocol *p = &list->items[i];
        fprintf(fp, "\n[[protocol]]\n");
        toml_kv_int(fp, "index", p->index);
        toml_kv_string(fp, "name", p->name);
        toml_kv_string(fp, "program", p->program);
        toml_kv_bool(fp, "batch", p->batch);
        toml_kv_bool(fp, "exitlevel", p->exitlevel);
        toml_kv_bool(fp, "opus", p->opus);
        toml_kv_bool(fp, "bi", p->bi);
        toml_kv_string(fp, "log_file", p->log_file);
        toml_kv_string(fp, "control_file", p->control_file);
        toml_kv_string(fp, "download_cmd", p->download_cmd);
        toml_kv_string(fp, "upload_cmd", p->upload_cmd);
        toml_kv_string(fp, "download_string", p->download_string);
        toml_kv_string(fp, "upload_string", p->upload_string);
        toml_kv_string(fp, "download_keyword", p->download_keyword);
        toml_kv_string(fp, "upload_keyword", p->upload_keyword);
        toml_kv_int(fp, "filename_word", p->filename_word);
        toml_kv_int(fp, "descript_word", p->descript_word);
    }
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_write_general_display_files_toml(FILE *fp, const MaxCfgNgGeneralDisplayFiles *files)
{
    if (fp == NULL || files == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    toml_kv_string(fp, "logo", files->logo);
    toml_kv_string(fp, "not_found", files->not_found);
    toml_kv_string(fp, "application", files->application);
    toml_kv_string(fp, "welcome", files->welcome);
    toml_kv_string(fp, "new_user1", files->new_user1);
    toml_kv_string(fp, "new_user2", files->new_user2);
    toml_kv_string(fp, "rookie", files->rookie);
    toml_kv_string(fp, "not_configured", files->not_configured);
    toml_kv_string(fp, "quote", files->quote);
    toml_kv_string(fp, "day_limit", files->day_limit);
    toml_kv_string(fp, "time_warn", files->time_warn);
    toml_kv_string(fp, "too_slow", files->too_slow);
    toml_kv_string(fp, "bye_bye", files->bye_bye);
    toml_kv_string(fp, "bad_logon", files->bad_logon);
    toml_kv_string(fp, "barricade", files->barricade);
    toml_kv_string(fp, "no_space", files->no_space);
    toml_kv_string(fp, "no_mail", files->no_mail);
    toml_kv_string(fp, "area_not_exist", files->area_not_exist);
    toml_kv_string(fp, "chat_begin", files->chat_begin);
    toml_kv_string(fp, "chat_end", files->chat_end);
    toml_kv_string(fp, "out_leaving", files->out_leaving);
    toml_kv_string(fp, "out_return", files->out_return);
    toml_kv_string(fp, "shell_to_dos", files->shell_to_dos);
    toml_kv_string(fp, "back_from_dos", files->back_from_dos);
    toml_kv_string(fp, "locate", files->locate);
    toml_kv_string(fp, "contents", files->contents);
    toml_kv_string(fp, "oped_help", files->oped_help);
    toml_kv_string(fp, "line_ed_help", files->line_ed_help);
    toml_kv_string(fp, "replace_help", files->replace_help);
    toml_kv_string(fp, "inquire_help", files->inquire_help);
    toml_kv_string(fp, "scan_help", files->scan_help);
    toml_kv_string(fp, "list_help", files->list_help);
    toml_kv_string(fp, "header_help", files->header_help);
    toml_kv_string(fp, "entry_help", files->entry_help);
    toml_kv_string(fp, "xfer_baud", files->xfer_baud);
    toml_kv_string(fp, "file_area_list", files->file_area_list);
    toml_kv_string(fp, "file_header", files->file_header);
    toml_kv_string(fp, "file_format", files->file_format);
    toml_kv_string(fp, "file_footer", files->file_footer);
    toml_kv_string(fp, "msg_area_list", files->msg_area_list);
    toml_kv_string(fp, "msg_header", files->msg_header);
    toml_kv_string(fp, "msg_format", files->msg_format);
    toml_kv_string(fp, "msg_footer", files->msg_footer);
    toml_kv_string(fp, "protocol_dump", files->protocol_dump);
    toml_kv_string(fp, "fname_format", files->fname_format);
    toml_kv_string(fp, "time_format", files->time_format);
    toml_kv_string(fp, "date_format", files->date_format);
    toml_kv_string(fp, "tune", files->tune);
    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_write_general_colors_toml(FILE *fp, const MaxCfgNgGeneralColors *colors)
{
    if (fp == NULL || colors == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    fputs("[menu]\n", fp);
    toml_kv_color(fp, "name", &colors->menu_name);
    toml_kv_color(fp, "highlight", &colors->menu_highlight);
    toml_kv_color(fp, "option", &colors->menu_option);

    fputs("\n[file]\n", fp);
    toml_kv_color(fp, "name", &colors->file_name);
    toml_kv_color(fp, "size", &colors->file_size);
    toml_kv_color(fp, "date", &colors->file_date);
    toml_kv_color(fp, "description", &colors->file_description);
    toml_kv_color(fp, "search_match", &colors->file_search_match);
    toml_kv_color(fp, "offline", &colors->file_offline);
    toml_kv_color(fp, "new", &colors->file_new);

    fputs("\n[msg]\n", fp);
    toml_kv_color(fp, "from_label", &colors->msg_from_label);
    toml_kv_color(fp, "from_text", &colors->msg_from_text);
    toml_kv_color(fp, "to_label", &colors->msg_to_label);
    toml_kv_color(fp, "to_text", &colors->msg_to_text);
    toml_kv_color(fp, "subject_label", &colors->msg_subject_label);
    toml_kv_color(fp, "subject_text", &colors->msg_subject_text);
    toml_kv_color(fp, "attributes", &colors->msg_attributes);
    toml_kv_color(fp, "date", &colors->msg_date);
    toml_kv_color(fp, "address", &colors->msg_address);
    toml_kv_color(fp, "locus", &colors->msg_locus);
    toml_kv_color(fp, "body", &colors->msg_body);
    toml_kv_color(fp, "quote", &colors->msg_quote);
    toml_kv_color(fp, "kludge", &colors->msg_kludge);

    fputs("\n[fsr]\n", fp);
    toml_kv_color(fp, "msgnum", &colors->fsr_msgnum);
    toml_kv_color(fp, "links", &colors->fsr_links);
    toml_kv_color(fp, "attrib", &colors->fsr_attrib);
    toml_kv_color(fp, "msginfo", &colors->fsr_msginfo);
    toml_kv_color(fp, "date", &colors->fsr_date);
    toml_kv_color(fp, "addr", &colors->fsr_addr);
    toml_kv_color(fp, "static", &colors->fsr_static);
    toml_kv_color(fp, "border", &colors->fsr_border);
    toml_kv_color(fp, "locus", &colors->fsr_locus);

    /* Append default theme color section */
    fputc('\n', fp);
    MaxCfgThemeColors theme;
    maxcfg_theme_init(&theme);
    maxcfg_theme_write_toml(fp, &theme);

    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_write_access_levels_toml(FILE *fp, const MaxCfgNgAccessLevelList *levels)
{
    if (fp == NULL || levels == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < levels->count; i++) {
        const MaxCfgNgAccessLevel *lvl = &levels->items[i];

        if (i > 0) {
            fputc('\n', fp);
        }

        fputs("[[access_level]]\n", fp);
        toml_kv_string(fp, "name", lvl->name);
        toml_kv_int(fp, "level", lvl->level);
        toml_kv_string(fp, "description", lvl->description);
        toml_kv_string(fp, "alias", lvl->alias);
        toml_kv_string(fp, "key", lvl->key);

        toml_kv_int(fp, "time", lvl->time);
        toml_kv_int(fp, "cume", lvl->cume);
        toml_kv_int(fp, "calls", lvl->calls);

        toml_kv_int(fp, "logon_baud", lvl->logon_baud);
        toml_kv_int(fp, "xfer_baud", lvl->xfer_baud);

        toml_kv_int(fp, "file_limit", lvl->file_limit);
        toml_kv_int(fp, "file_ratio", lvl->file_ratio);
        toml_kv_int(fp, "ratio_free", lvl->ratio_free);
        toml_kv_int(fp, "upload_reward", lvl->upload_reward);

        toml_kv_string(fp, "login_file", lvl->login_file);
        toml_kv_string_array(fp, "flags", lvl->flags, lvl->flag_count);
        toml_kv_string_array(fp, "mail_flags", lvl->mail_flags, lvl->mail_flag_count);
        toml_kv_uint(fp, "user_flags", lvl->user_flags);
        toml_kv_int(fp, "oldpriv", lvl->oldpriv);
    }

    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_write_menu_toml(FILE *fp, const MaxCfgNgMenu *menu)
{
    if (fp == NULL || menu == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    toml_kv_string(fp, "name", menu->name);
    toml_kv_string(fp, "title", menu->title);

    toml_kv_string(fp, "header_file", menu->header_file);
    toml_kv_string_array(fp, "header_types", menu->header_types, menu->header_type_count);

    toml_kv_string(fp, "menu_file", menu->menu_file);
    toml_kv_string_array(fp, "menu_types", menu->menu_types, menu->menu_type_count);

    toml_kv_int(fp, "menu_length", menu->menu_length);
    toml_kv_int(fp, "menu_color", menu->menu_color);
    toml_kv_int(fp, "option_width", menu->option_width);

    if (menu->custom_menu != NULL && menu->custom_menu->enabled) {
        const MaxCfgNgCustomMenu *cm = menu->custom_menu;

        fputs("\n[custom_menu]\n", fp);
        toml_kv_bool(fp, "skip_canned_menu", cm->skip_canned_menu);
        toml_kv_bool(fp, "show_title", cm->show_title);
        toml_kv_bool(fp, "lightbar_menu", cm->lightbar_menu);
        toml_kv_int(fp, "lightbar_margin", cm->lightbar_margin);

        if (cm->top_boundary_row > 0 && cm->top_boundary_col > 0) {
            int items[2] = { cm->top_boundary_row, cm->top_boundary_col };
            toml_kv_int_array(fp, "top_boundary", items, 2);
        }
        if (cm->bottom_boundary_row > 0 && cm->bottom_boundary_col > 0) {
            int items[2] = { cm->bottom_boundary_row, cm->bottom_boundary_col };
            toml_kv_int_array(fp, "bottom_boundary", items, 2);
        }
        if (cm->title_location_row > 0 && cm->title_location_col > 0) {
            int items[2] = { cm->title_location_row, cm->title_location_col };
            toml_kv_int_array(fp, "title_location", items, 2);
        }
        if (cm->prompt_location_row > 0 && cm->prompt_location_col > 0) {
            int items[2] = { cm->prompt_location_row, cm->prompt_location_col };
            toml_kv_int_array(fp, "prompt_location", items, 2);
        }

        if (cm->has_lightbar_normal || cm->has_lightbar_selected || cm->has_lightbar_high || cm->has_lightbar_high_selected) {
            fputs("lightbar_color = { ", fp);
            bool first = true;

            unsigned char attr;
            const char *fg;
            const char *bg;

            if (cm->has_lightbar_normal) {
                attr = cm->lightbar_normal_attr;
                fg = maxcfg_dos_color_to_name((int)(attr & 0x0f));
                bg = maxcfg_dos_color_to_name((int)((attr >> 4) & 0x0f));
                if (!first) fputs(", ", fp);
                fputs("normal = [", fp);
                toml_write_escaped(fp, fg);
                fputs(", ", fp);
                toml_write_escaped(fp, bg);
                fputs("]", fp);
                first = false;
            }
            if (cm->has_lightbar_high) {
                attr = cm->lightbar_high_attr;
                fg = maxcfg_dos_color_to_name((int)(attr & 0x0f));
                bg = maxcfg_dos_color_to_name((int)((attr >> 4) & 0x0f));
                if (!first) fputs(", ", fp);
                fputs("high = [", fp);
                toml_write_escaped(fp, fg);
                fputs(", ", fp);
                toml_write_escaped(fp, bg);
                fputs("]", fp);
                first = false;
            }
            if (cm->has_lightbar_selected) {
                attr = cm->lightbar_selected_attr;
                fg = maxcfg_dos_color_to_name((int)(attr & 0x0f));
                bg = maxcfg_dos_color_to_name((int)((attr >> 4) & 0x0f));
                if (!first) fputs(", ", fp);
                fputs("selected = [", fp);
                toml_write_escaped(fp, fg);
                fputs(", ", fp);
                toml_write_escaped(fp, bg);
                fputs("]", fp);
                first = false;
            }
            if (cm->has_lightbar_high_selected) {
                attr = cm->lightbar_high_selected_attr;
                fg = maxcfg_dos_color_to_name((int)(attr & 0x0f));
                bg = maxcfg_dos_color_to_name((int)((attr >> 4) & 0x0f));
                if (!first) fputs(", ", fp);
                fputs("high_selected = [", fp);
                toml_write_escaped(fp, fg);
                fputs(", ", fp);
                toml_write_escaped(fp, bg);
                fputs("]", fp);
                first = false;
            }

            fputs(" }\n", fp);
        }

        toml_kv_bool(fp, "option_spacing", cm->option_spacing);
        toml_kv_string(fp, "option_justify", (cm->option_justify == 1) ? "center" : (cm->option_justify == 2) ? "right" : "left");

        {
            const char *hj = (cm->boundary_justify == 2) ? "right" : (cm->boundary_justify == 1) ? "center" : "left";
            const char *vj = (cm->boundary_vjustify == 2) ? "bottom" : (cm->boundary_vjustify == 1) ? "center" : "top";
            char buf[32];
            if (snprintf(buf, sizeof(buf), "%s %s", hj, vj) < (int)sizeof(buf)) {
                toml_kv_string(fp, "boundary_justify", buf);
            }
        }

        toml_kv_string(fp, "boundary_layout",
                       (cm->boundary_layout == 1) ? "tight" :
                       (cm->boundary_layout == 2) ? "spread" :
                       (cm->boundary_layout == 3) ? "spread_width" :
                       (cm->boundary_layout == 4) ? "spread_height" : "grid");
    }

    for (size_t i = 0; i < menu->option_count; i++) {
        const MaxCfgNgMenuOption *opt = &menu->options[i];
        fputs("\n[[option]]\n", fp);
        toml_kv_string(fp, "command", opt->command);
        toml_kv_string(fp, "arguments", opt->arguments);
        toml_kv_string(fp, "priv_level", opt->priv_level);
        toml_kv_string(fp, "description", opt->description);
        toml_kv_string(fp, "key_poke", opt->key_poke);
        toml_kv_string_array(fp, "modifiers", opt->modifiers, opt->modifier_count);
    }

    return MAXCFG_OK;
}

static void toml_write_area_common(FILE *fp, const char *name, const char *desc, const char *acs, const char *menuname)
{
    toml_kv_string(fp, "name", name);
    toml_kv_string(fp, "description", desc);
    toml_kv_string(fp, "acs", acs);
    toml_kv_string(fp, "menu", menuname);
}

MaxCfgStatus maxcfg_ng_write_msg_areas_toml(FILE *fp, const MaxCfgNgDivisionList *divisions, const MaxCfgNgMsgAreaList *areas)
{
    if (fp == NULL || divisions == NULL || areas == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < divisions->count; i++) {
        const MaxCfgNgDivision *d = &divisions->items[i];
        fputs("\n[[division]]\n", fp);
        toml_kv_string(fp, "name", d->name);
        toml_kv_string(fp, "key", d->key);
        toml_kv_string(fp, "description", d->description);
        toml_kv_string(fp, "acs", d->acs);
        toml_kv_string(fp, "display_file", d->display_file);
        toml_kv_int(fp, "level", d->level);
    }

    for (size_t i = 0; i < areas->count; i++) {
        const MaxCfgNgMsgArea *a = &areas->items[i];
        fputs("\n[[area]]\n", fp);
        toml_write_area_common(fp, a->name, a->description, a->acs, a->menu);
        toml_kv_string(fp, "division", a->division);
        toml_kv_string(fp, "tag", a->tag);
        toml_kv_string(fp, "path", a->path);
        toml_kv_string(fp, "owner", a->owner);
        toml_kv_string(fp, "origin", a->origin);
        toml_kv_string(fp, "attach_path", a->attach_path);
        toml_kv_string(fp, "barricade", a->barricade);
        toml_kv_string_array(fp, "style", a->style, a->style_count);
        toml_kv_int(fp, "renum_max", a->renum_max);
        toml_kv_int(fp, "renum_days", a->renum_days);
    }

    return MAXCFG_OK;
}

MaxCfgStatus maxcfg_ng_write_file_areas_toml(FILE *fp, const MaxCfgNgDivisionList *divisions, const MaxCfgNgFileAreaList *areas)
{
    if (fp == NULL || divisions == NULL || areas == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < divisions->count; i++) {
        const MaxCfgNgDivision *d = &divisions->items[i];
        fputs("\n[[division]]\n", fp);
        toml_kv_string(fp, "name", d->name);
        toml_kv_string(fp, "key", d->key);
        toml_kv_string(fp, "description", d->description);
        toml_kv_string(fp, "acs", d->acs);
        toml_kv_string(fp, "display_file", d->display_file);
        toml_kv_int(fp, "level", d->level);
    }

    for (size_t i = 0; i < areas->count; i++) {
        const MaxCfgNgFileArea *a = &areas->items[i];
        fputs("\n[[area]]\n", fp);
        toml_write_area_common(fp, a->name, a->description, a->acs, a->menu);
        toml_kv_string(fp, "division", a->division);
        toml_kv_string(fp, "download", a->download);
        toml_kv_string(fp, "upload", a->upload);
        toml_kv_string(fp, "filelist", a->filelist);
        toml_kv_string(fp, "barricade", a->barricade);
        toml_kv_string_array(fp, "types", a->types, a->type_count);
    }

    return MAXCFG_OK;
}
