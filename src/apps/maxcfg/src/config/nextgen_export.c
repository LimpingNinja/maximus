#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>

#include "area_parse.h"
#include "ctl_to_ng.h"
#include "libmaxcfg.h"
#include "maxcfg.h"
#include "menu_data.h"
#include "nextgen_export.h"

#ifndef FLOW_TXOFF
#define FLOW_TXOFF 0x01
#endif
#ifndef FLOW_CTS
#define FLOW_CTS 0x02
#endif
#ifndef FLOW_DSR
#define FLOW_DSR 0x04
#endif

#ifndef VIDEO_IBM
#define VIDEO_IBM 0x02
#endif

#ifndef VIDEO_BIOS
#define VIDEO_BIOS 0x04
#endif

typedef struct ExportCtx {
    const char *sys_path;
    const char *config_dir;
    const char *maxctl_path;
} ExportCtx;

static bool dup_str(char **out, const char *in, char *err, size_t err_len);
static bool check_st(MaxCfgStatus st, char *err, size_t err_len);
static char *trim_ws(char *s);
static bool parse_access_ctl(const char *path, MaxCfgNgAccessLevelList *out, char *err, size_t err_len);

static bool line_starts_with_keyword(const char *line, const char *keyword)
{
    size_t kw_len = strlen(keyword);

    while (*line && isspace((unsigned char)*line)) {
        line++;
    }
    if (strncasecmp(line, keyword, kw_len) != 0) {
        return false;
    }
    const char *after = line + kw_len;
    return (*after == '\0' || isspace((unsigned char)*after));
}

static char *extract_value_after_keyword(char *line, const char *keyword)
{
    size_t kw_len = strlen(keyword);
    while (*line && isspace((unsigned char)*line)) {
        line++;
    }
    line += kw_len;
    while (*line && isspace((unsigned char)*line)) {
        line++;
    }
    return trim_ws(line);
}

static bool maxctl_find_value(const char *maxctl_path, const char *keyword, char *out, size_t out_sz)
{
    if (maxctl_path == NULL || keyword == NULL || out == NULL || out_sz == 0) {
        return false;
    }

    FILE *fp = fopen(maxctl_path, "r");
    if (!fp) {
        return false;
    }

    char line[1024];
    bool found = false;
    while (fgets(line, sizeof(line), fp)) {
        char *s = trim_ws(line);
        if (s == NULL || s[0] == '\0') continue;
        if (!isalnum((unsigned char)s[0])) continue;
        if (!line_starts_with_keyword(s, keyword)) continue;

        char *v = extract_value_after_keyword(s, keyword);
        if (v == NULL) v = (char *)"";
        strncpy(out, v, out_sz - 1);
        out[out_sz - 1] = '\0';
        found = true;
        break;
    }

    fclose(fp);
    return found;
}

static bool maxctl_has_keyword(const char *maxctl_path, const char *keyword, bool *out)
{
    if (maxctl_path == NULL || keyword == NULL || out == NULL) {
        return false;
    }
    *out = false;

    FILE *fp = fopen(maxctl_path, "r");
    if (!fp) {
        return false;
    }

    char line[1024];
    bool found = false;
    char neg[256];
    if (snprintf(neg, sizeof(neg), "No %s", keyword) >= (int)sizeof(neg)) {
        neg[0] = '\0';
    }

    while (fgets(line, sizeof(line), fp)) {
        char *s = trim_ws(line);
        if (s == NULL || s[0] == '\0') continue;
        if (!isalnum((unsigned char)s[0])) continue;

        if (line_starts_with_keyword(s, keyword)) {
            *out = true;
            found = true;
            break;
        }
        if (neg[0] && line_starts_with_keyword(s, neg)) {
            *out = false;
            found = true;
            break;
        }
    }

    fclose(fp);
    return found;
}

static bool join_sys_path(char *out, size_t out_len, const char *sys_path, const char *rel)
{
    if (out == NULL || out_len == 0) {
        return false;
    }
    if (sys_path == NULL) sys_path = "";
    if (rel == NULL) rel = "";

    if (rel[0] == '/' || rel[0] == '\\') {
        return snprintf(out, out_len, "%s", rel) < (int)out_len;
    }
    if (snprintf(out, out_len, "%s/%s", sys_path, rel) >= (int)out_len) {
        return false;
    }
    return true;
}

static bool derive_sys_path_from_maxctl(const char *maxctl_path, char *out, size_t out_sz)
{
    if (maxctl_path == NULL || out == NULL || out_sz == 0) {
        return false;
    }

    char tmp1[PATH_MAX];
    if (snprintf(tmp1, sizeof(tmp1), "%s", maxctl_path) >= (int)sizeof(tmp1)) {
        return false;
    }
    char *p1 = strrchr(tmp1, '/');
    if (p1 == NULL) {
        return false;
    }
    *p1 = '\0';

    char *p2 = strrchr(tmp1, '/');
    if (p2 == NULL) {
        return false;
    }
    *p2 = '\0';

    if (snprintf(out, out_sz, "%s", tmp1) >= (int)out_sz) {
        return false;
    }
    return true;
}

static bool str_eq_ci(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }
    return strcasecmp(a, b) == 0;
}

static int priv_name_to_level(const MaxCfgNgAccessLevelList *levels, const char *name)
{
    if (name == NULL) {
        return 0;
    }
    char tmp[64];
    if (snprintf(tmp, sizeof(tmp), "%s", name) >= (int)sizeof(tmp)) {
        return 0;
    }
    char *t = trim_ws(tmp);
    if (t == NULL || t[0] == '\0') {
        return 0;
    }

    /* Try numeric value first */
    char *end = NULL;
    long iv = strtol(t, &end, 10);
    if (end && *trim_ws(end) == '\0') {
        return (int)iv;
    }

    /* Lookup in access level list */
    if (levels) {
        for (size_t i = 0; i < levels->count; i++) {
            const MaxCfgNgAccessLevel *lvl = &levels->items[i];
            if (lvl->name && strcasecmp(lvl->name, t) == 0) {
                return lvl->level;
            }
        }
    }

    /* Fallback: if "hidden" but not in levels, return 65535 */
    if (strcasecmp(t, "hidden") == 0) {
        return 65535;
    }

    return 0;
}

static const char *matrix_attr_to_key(const char *attr)
{
    if (attr == NULL) return NULL;
    if (strcasecmp(attr, "private") == 0) return "private";
    if (strcasecmp(attr, "crash") == 0) return "crash";
    if (strcasecmp(attr, "fileattach") == 0) return "fileattach";
    if (strcasecmp(attr, "killsent") == 0) return "killsent";
    if (strcasecmp(attr, "hold") == 0) return "hold";
    if (strcasecmp(attr, "filerequest") == 0) return "filerequest";
    if (strcasecmp(attr, "updaterequest") == 0) return "updaterequest";
    if (strcasecmp(attr, "localattach") == 0) return "localattach";
    return NULL;
}

static bool strv_add(char ***items_io, size_t *count_io, const char *s)
{
    if (items_io == NULL || count_io == NULL) {
        return false;
    }

    char *dup = strdup(s ? s : "");
    if (dup == NULL) {
        return false;
    }

    char **p = (char **)realloc(*items_io, (*count_io + 1u) * sizeof((*items_io)[0]));
    if (p == NULL) {
        free(dup);
        return false;
    }

    p[*count_io] = dup;
    *items_io = p;
    (*count_io)++;
    return true;
}

static bool drives_to_save_to_strv(const char drives[(MAX_DRIVES/CHAR_BITS)+1], char ***out_items, size_t *out_count)
{
    if (out_items == NULL || out_count == NULL) {
        return false;
    }

    *out_items = NULL;
    *out_count = 0;

    for (int i = 0; i < MAX_DRIVES; i++) {
        unsigned int byte_index = (unsigned int)i / CHAR_BITS;
        unsigned int bit_index = (unsigned int)i % CHAR_BITS;
        unsigned char b = (unsigned char)drives[byte_index];
        if ((b & (unsigned char)(1u << bit_index)) != 0u) {
            char s[2];
            s[0] = (char)('A' + i);
            s[1] = '\0';
            if (!strv_add(out_items, out_count, s)) {
                return false;
            }
        }
    }

    return true;
}

static const char *charset_to_string(unsigned char charset)
{
    if (charset == CHARSET_SWEDISH) {
        return "swedish";
    }
    if (charset == CHARSET_CHINESE) {
        return "chinese";
    }
    return "";
}

static bool is_abs_path_like(const char *p)
{
    if (p == NULL || p[0] == '\0') {
        return false;
    }
    if (p[0] == '/' || p[0] == '\\') {
        return true;
    }
    return strchr(p, ':') != NULL;
}

static const char *system_video_to_string(unsigned char video)
{
    if (video == VIDEO_BIOS) {
        return "bios";
    }
    if (video == VIDEO_IBM) {
        return "ibm";
    }
    return "";
}

static const char *multitasker_to_string(int multitasker)
{
    switch (multitasker) {
        case MULTITASKER_NONE:
            return "none";
        case MULTITASKER_AUTO:
            return "auto";
        case MULTITASKER_DOUBLEDOS:
            return "doubledos";
        case MULTITASKER_DESQVIEW:
            return "desqview";
        case MULTITASKER_TOPVIEW:
            return "topview";
        case MULTITASKER_MLINK:
            return "multilink";
        case MULTITASKER_MSWINDOWS:
            return "mswindows";
        case MULTITASKER_PCMOS:
            return "pc-mos";
        case MULTITASKER_OS2:
            return "os/2";
        case MULTITASKER_UNIX:
            return "unix";
        default:
            return "";
    }
}

static const char *kill_attach_to_string(unsigned char v)
{
    if (v == 1u) {
        return "ask";
    }
    if (v == 2u) {
        return "always";
    }
    return "never";
}

static const char *nodelist_version_to_string(unsigned char nlver)
{
    if (nlver == NLVER_5) return "5";
    if (nlver == NLVER_6) return "6";
    if (nlver == NLVER_7) return "7";
    if (nlver == NLVER_FD) return "fd";
    return "";
}

static bool add_attr_priv(MaxCfgNgAttributePriv **arr, size_t *count_io, const char *attribute, int priv)
{
    if (arr == NULL || count_io == NULL || attribute == NULL) {
        return false;
    }

    MaxCfgNgAttributePriv *p = realloc(*arr, (*count_io + 1) * sizeof(**arr));
    if (p == NULL) {
        return false;
    }
    *arr = p;

    (*arr)[*count_io].attribute = strdup(attribute);
    if ((*arr)[*count_io].attribute == NULL) {
        return false;
    }
    (*arr)[*count_io].priv = priv;
    (*count_io)++;
    return true;
}

static bool msg_style_to_strings(unsigned int style, char ***out_styles, size_t *out_count)
{
    *out_styles = NULL;
    *out_count = 0;

    if ((style & MSGSTYLE_SQUISH) != 0u) { if (!strv_add(out_styles, out_count, "Squish")) return false; }
    else if ((style & MSGSTYLE_DOTMSG) != 0u) { if (!strv_add(out_styles, out_count, "*.MSG")) return false; }

    if ((style & MSGSTYLE_LOCAL) != 0u) { if (!strv_add(out_styles, out_count, "Local")) return false; }
    else if ((style & MSGSTYLE_NET) != 0u) { if (!strv_add(out_styles, out_count, "Net")) return false; }
    else if ((style & MSGSTYLE_ECHO) != 0u) { if (!strv_add(out_styles, out_count, "Echo")) return false; }
    else if ((style & MSGSTYLE_CONF) != 0u) { if (!strv_add(out_styles, out_count, "Conf")) return false; }

    if ((style & MSGSTYLE_PVT) != 0u) { if (!strv_add(out_styles, out_count, "Pvt")) return false; }
    if ((style & MSGSTYLE_PUB) != 0u) { if (!strv_add(out_styles, out_count, "Pub")) return false; }

    if ((style & MSGSTYLE_HIBIT) != 0u) { if (!strv_add(out_styles, out_count, "HiBit")) return false; }
    if ((style & MSGSTYLE_ANON) != 0u) { if (!strv_add(out_styles, out_count, "Anon")) return false; }
    if ((style & MSGSTYLE_NORNK) != 0u) { if (!strv_add(out_styles, out_count, "NoNameKludge")) return false; }
    if ((style & MSGSTYLE_REALNAME) != 0u) { if (!strv_add(out_styles, out_count, "RealName")) return false; }
    if ((style & MSGSTYLE_ALIAS) != 0u) { if (!strv_add(out_styles, out_count, "Alias")) return false; }
    if ((style & MSGSTYLE_AUDIT) != 0u) { if (!strv_add(out_styles, out_count, "Audit")) return false; }
    if ((style & MSGSTYLE_READONLY) != 0u) { if (!strv_add(out_styles, out_count, "ReadOnly")) return false; }
    if ((style & MSGSTYLE_HIDDEN) != 0u) { if (!strv_add(out_styles, out_count, "Hidden")) return false; }
    if ((style & MSGSTYLE_ATTACH) != 0u) { if (!strv_add(out_styles, out_count, "Attach")) return false; }
    if ((style & MSGSTYLE_NOMAILCHK) != 0u) { if (!strv_add(out_styles, out_count, "NoMailCheck")) return false; }

    return true;
}

static bool file_area_types_from_bits(const FileAreaData *a, char ***out_types, size_t *out_count)
{
    *out_types = NULL;
    *out_count = 0;

    if (a == NULL) {
        return true;
    }

    if (a->type_slow && a->type_staged && a->type_nonew) {
        return strv_add(out_types, out_count, "CD");
    }

    if (a->type_slow) { if (!strv_add(out_types, out_count, "Slow")) return false; }
    if (a->type_staged) { if (!strv_add(out_types, out_count, "Staged")) return false; }
    if (a->type_nonew) { if (!strv_add(out_types, out_count, "NoNew")) return false; }

    return true;
}

static void strv_free(char ***items_io, size_t *count_io)
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
    }
    free(*items_io);
    *items_io = NULL;
}

static bool strv_add_words(char ***items_io, size_t *count_io, const char *words)
{
    if (words == NULL) {
        return true;
    }

    char *copy = strdup(words);
    if (copy == NULL) {
        return false;
    }

    char *tok = strtok(copy, " \t\r\n");
    while (tok != NULL) {
        if (tok[0] != '\0') {
            if (!strv_add(items_io, count_io, tok)) {
                free(copy);
                return false;
            }
        }
        tok = strtok(NULL, " \t\r\n");
    }

    free(copy);
    return true;
}

static bool menu_types_from_flags(word flags, bool is_header, char ***out_types, size_t *out_count)
{
    (void)is_header;
    *out_types = NULL;
    *out_count = 0;

    if ((flags & (MFLAG_MF_NOVICE | MFLAG_HF_NOVICE)) != 0u) {
        if (!strv_add(out_types, out_count, "Novice")) return false;
    }
    if ((flags & (MFLAG_MF_REGULAR | MFLAG_HF_REGULAR)) != 0u) {
        if (!strv_add(out_types, out_count, "Regular")) return false;
    }
    if ((flags & (MFLAG_MF_EXPERT | MFLAG_HF_EXPERT)) != 0u) {
        if (!strv_add(out_types, out_count, "Expert")) return false;
    }
    if ((flags & (MFLAG_MF_RIP | MFLAG_HF_RIP)) != 0u) {
        if (!strv_add(out_types, out_count, "RIP")) return false;
    }

    return true;
}

static bool menu_option_modifiers_from_bits(const MenuOption *opt, char ***out_mods, size_t *out_count)
{
    *out_mods = NULL;
    *out_count = 0;

    if (!opt) {
        return true;
    }

    if ((opt->areatype & ATYPE_LOCAL) != 0u) { if (!strv_add(out_mods, out_count, "Local")) return false; }
    if ((opt->areatype & ATYPE_MATRIX) != 0u) { if (!strv_add(out_mods, out_count, "Matrix")) return false; }
    if ((opt->areatype & ATYPE_ECHO) != 0u) { if (!strv_add(out_mods, out_count, "Echo")) return false; }
    if ((opt->areatype & ATYPE_CONF) != 0u) { if (!strv_add(out_mods, out_count, "Conf")) return false; }

    if ((opt->flags & OFLAG_NODSP) != 0u) { if (!strv_add(out_mods, out_count, "NoDsp")) return false; }
    if ((opt->flags & OFLAG_CTL) != 0u) { if (!strv_add(out_mods, out_count, "Ctl")) return false; }
    if ((opt->flags & OFLAG_NOCLS) != 0u) { if (!strv_add(out_mods, out_count, "NoCLS")) return false; }
    if ((opt->flags & OFLAG_NORIP) != 0u) { if (!strv_add(out_mods, out_count, "NoRIP")) return false; }
    if ((opt->flags & OFLAG_RIP) != 0u) { if (!strv_add(out_mods, out_count, "RIP")) return false; }
    if ((opt->flags & OFLAG_THEN) != 0u) { if (!strv_add(out_mods, out_count, "Then")) return false; }
    if ((opt->flags & OFLAG_ELSE) != 0u) { if (!strv_add(out_mods, out_count, "Else")) return false; }
    if ((opt->flags & OFLAG_STAY) != 0u) { if (!strv_add(out_mods, out_count, "Stay")) return false; }
    if ((opt->flags & OFLAG_ULOCAL) != 0u) { if (!strv_add(out_mods, out_count, "UsrLocal")) return false; }
    if ((opt->flags & OFLAG_UREMOTE) != 0u) { if (!strv_add(out_mods, out_count, "UsrRemote")) return false; }
    if ((opt->flags & OFLAG_REREAD) != 0u) { if (!strv_add(out_mods, out_count, "ReRead")) return false; }

    return true;
}

static void set_err(char *err, size_t err_len, const char *fmt, ...)
{
    if (!err || err_len == 0) return;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(err, err_len, fmt ? fmt : "", ap);
    va_end(ap);
}

static bool ensure_dir(const char *path)
{
    struct stat st;
    if (!path || !path[0]) return false;

    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }

    if (mkdir(path) == 0) {
        return true;
    }

    return false;
}

static bool write_matrix_toml(FILE *fp, void *vctx, char *err, size_t err_len)
{
    ExportCtx *ctx = (ExportCtx *)vctx;

    MaxCfgNgMatrix matrix;
    if (!check_st(maxcfg_ng_matrix_init(&matrix), err, err_len)) {
        return false;
    }

    if (ctx && ctx->maxctl_path && ctx->maxctl_path[0]) {
        MaxCfgNgAccessLevelList levels;
        bool have_levels = false;
        if (ctx->sys_path && ctx->sys_path[0]) {
            char access_path[PATH_MAX];
            if (snprintf(access_path, sizeof(access_path), "%s/etc/access.ctl", ctx->sys_path) < (int)sizeof(access_path)) {
                have_levels = parse_access_ctl(access_path, &levels, err, err_len);
            }
        }

        char buf[512];
        if (maxctl_find_value(ctx->maxctl_path, "Message Show Ctl_A to", buf, sizeof(buf))) {
            matrix.ctla_priv = priv_name_to_level(have_levels ? &levels : NULL, buf);
        }
        if (maxctl_find_value(ctx->maxctl_path, "Message Show Seenby to", buf, sizeof(buf))) {
            matrix.seenby_priv = priv_name_to_level(have_levels ? &levels : NULL, buf);
        }
        if (maxctl_find_value(ctx->maxctl_path, "Message Show Private to", buf, sizeof(buf))) {
            matrix.private_priv = priv_name_to_level(have_levels ? &levels : NULL, buf);
        }
        if (maxctl_find_value(ctx->maxctl_path, "Message Edit Ask FromFile", buf, sizeof(buf))) {
            matrix.fromfile_priv = priv_name_to_level(have_levels ? &levels : NULL, buf);
        }

        if (maxctl_find_value(ctx->maxctl_path, "Message Send Unlisted", buf, sizeof(buf))) {
            char priv[128];
            int cost = 0;
            priv[0] = '\0';
            (void)sscanf(buf, "%127s %d", priv, &cost);
            if (priv[0]) {
                matrix.unlisted_priv = priv_name_to_level(have_levels ? &levels : NULL, priv);
            }
            matrix.unlisted_cost = cost;
        }

        /* Log EchoMail: presence of keyword = true */
        if (maxctl_find_value(ctx->maxctl_path, "Log EchoMail", buf, sizeof(buf))) {
            matrix.log_echomail = true;
        }
        
        /* After Edit/EchoMail/Local Exit keywords have "Exit" as second word */
        if (maxctl_find_value(ctx->maxctl_path, "After Edit", buf, sizeof(buf))) {
            /* Value is "Exit <number>" */
            char exit_word[32];
            int exit_val = 0;
            if (sscanf(buf, "%31s %d", exit_word, &exit_val) == 2) {
                matrix.after_edit_exit = exit_val;
            }
        }
        if (maxctl_find_value(ctx->maxctl_path, "After EchoMail", buf, sizeof(buf))) {
            /* Value is "Exit <number>" */
            char exit_word[32];
            int exit_val = 0;
            if (sscanf(buf, "%31s %d", exit_word, &exit_val) == 2) {
                matrix.after_echomail_exit = exit_val;
            }
        }
        if (maxctl_find_value(ctx->maxctl_path, "After Local", buf, sizeof(buf))) {
            /* Value is "Exit <number>" */
            char exit_word[32];
            int exit_val = 0;
            if (sscanf(buf, "%31s %d", exit_word, &exit_val) == 2) {
                matrix.after_local_exit = exit_val;
            }
        }

        if (maxctl_find_value(ctx->maxctl_path, "Nodelist", buf, sizeof(buf))) {
            const char *nv = "";
            if (strstr(buf, "FD") != NULL || strstr(buf, "fd") != NULL || strstr(buf, "FrontDoor") != NULL || strstr(buf, "frontdoor") != NULL) nv = "fd";
            else if (strstr(buf, "5") != NULL) nv = "5";
            else if (strstr(buf, "6") != NULL) nv = "6";
            else if (strstr(buf, "7") != NULL) nv = "7";
            if (!dup_str(&matrix.nodelist_version, nv, err, err_len)) goto fail;
        }
        if (maxctl_find_value(ctx->maxctl_path, "FidoUser", buf, sizeof(buf))) {
            if (!dup_str(&matrix.fidouser, buf, err, err_len)) goto fail;
        }
        if (maxctl_find_value(ctx->maxctl_path, "EchoToss Name", buf, sizeof(buf))) {
            if (!dup_str(&matrix.echotoss_name, buf, err, err_len)) goto fail;
        }

        FILE *fpctl = fopen(ctx->maxctl_path, "r");
        if (fpctl) {
            char line[1024];
            while (fgets(line, sizeof(line), fpctl)) {
                char *s = trim_ws(line);
                if (s == NULL || s[0] == '\0') continue;
                if (s[0] == '%' || s[0] == ';') continue;

                if (line_starts_with_keyword(s, "Address")) {
                    char *addr = extract_value_after_keyword(s, "Address");
                    if (addr && addr[0]) {
                        unsigned int zone = 0, net = 0, node = 0, point = 0;
                        int n = sscanf(addr, "%u:%u/%u.%u", &zone, &net, &node, &point);
                        if (n >= 3) {
                            if (n == 3) point = 0;
                            void *p = realloc(matrix.addresses, (matrix.address_count + 1u) * sizeof(*matrix.addresses));
                            if (p == NULL) {
                                fclose(fpctl);
                                set_err(err, err_len, "Out of memory");
                                goto fail;
                            }
                            matrix.addresses = p;
                            matrix.addresses[matrix.address_count].zone = (int)zone;
                            matrix.addresses[matrix.address_count].net = (int)net;
                            matrix.addresses[matrix.address_count].node = (int)node;
                            matrix.addresses[matrix.address_count].point = (int)point;
                            matrix.address_count++;
                        }
                    }
                    continue;
                }

                if (line_starts_with_keyword(s, "Message Edit Ask")) {
                    char attr[128];
                    char priv[128];
                    attr[0] = '\0';
                    priv[0] = '\0';
                    if (sscanf(s, "Message Edit Ask %127s %127s", attr, priv) == 2) {
                        const char *k = matrix_attr_to_key(attr);
                        if (k) {
                            int pv = priv_name_to_level(have_levels ? &levels : NULL, priv);
                            if (!add_attr_priv(&matrix.message_edit_ask, &matrix.message_edit_ask_count, k, pv)) {
                                fclose(fpctl);
                                set_err(err, err_len, "Out of memory");
                                goto fail;
                            }
                        }
                    }
                    continue;
                }

                if (line_starts_with_keyword(s, "Message Edit Assume")) {
                    char attr[128];
                    char priv[128];
                    attr[0] = '\0';
                    priv[0] = '\0';
                    if (sscanf(s, "Message Edit Assume %127s %127s", attr, priv) == 2) {
                        const char *k = matrix_attr_to_key(attr);
                        if (k) {
                            int pv = priv_name_to_level(have_levels ? &levels : NULL, priv);
                            if (!add_attr_priv(&matrix.message_edit_assume, &matrix.message_edit_assume_count, k, pv)) {
                                fclose(fpctl);
                                set_err(err, err_len, "Out of memory");
                                goto fail;
                            }
                        }
                    }
                    continue;
                }
            }
            fclose(fpctl);
        }

        if (have_levels) {
            maxcfg_ng_access_level_list_free(&levels);
        }
    }

    MaxCfgStatus st = maxcfg_ng_write_matrix_toml(fp, &matrix);
    maxcfg_ng_matrix_free(&matrix);
    return check_st(st, err, err_len);

fail:
    maxcfg_ng_matrix_free(&matrix);
    return false;
}

static bool write_general_reader_toml(FILE *fp, void *vctx, char *err, size_t err_len)
{
    ExportCtx *ctx = (ExportCtx *)vctx;

    MaxCfgNgReader reader;
    if (!check_st(maxcfg_ng_reader_init(&reader), err, err_len)) {
        return false;
    }

    if (ctx && ctx->sys_path && ctx->sys_path[0]) {
        char rpath[PATH_MAX];
        if (snprintf(rpath, sizeof(rpath), "%s/etc/reader.ctl", ctx->sys_path) < (int)sizeof(rpath)) {
            FILE *in = fopen(rpath, "r");
            if (in) {
                char line[1024];
                while (fgets(line, sizeof(line), in)) {
                    char *s = trim_ws(line);
                    if (s == NULL || s[0] == '\0') continue;
                    if (s[0] == '%' || s[0] == ';') continue;

                    if (line_starts_with_keyword(s, "Archivers")) {
                        if (!dup_str(&reader.archivers_ctl, extract_value_after_keyword(s, "Archivers"), err, err_len)) {
                            fclose(in);
                            goto fail;
                        }
                    } else if (line_starts_with_keyword(s, "Packet Name")) {
                        if (!dup_str(&reader.packet_name, extract_value_after_keyword(s, "Packet Name"), err, err_len)) {
                            fclose(in);
                            goto fail;
                        }
                    } else if (line_starts_with_keyword(s, "Work Directory")) {
                        if (!dup_str(&reader.work_directory, extract_value_after_keyword(s, "Work Directory"), err, err_len)) {
                            fclose(in);
                            goto fail;
                        }
                    } else if (line_starts_with_keyword(s, "Phone Number")) {
                        if (!dup_str(&reader.phone, extract_value_after_keyword(s, "Phone Number"), err, err_len)) {
                            fclose(in);
                            goto fail;
                        }
                    } else if (line_starts_with_keyword(s, "Max Messages")) {
                        reader.max_pack = atoi(extract_value_after_keyword(s, "Max Messages"));
                    }
                }
                fclose(in);
            }
        }
    }

    MaxCfgStatus st = maxcfg_ng_write_reader_toml(fp, &reader);
    maxcfg_ng_reader_free(&reader);
    return check_st(st, err, err_len);

fail:
    maxcfg_ng_reader_free(&reader);
    return false;
}

static bool handshake_mask_to_strv(unsigned int mask, char ***out, size_t *out_count)
{
    if (out == NULL || out_count == NULL) {
        return false;
    }

    *out = NULL;
    *out_count = 0;

    if ((mask & FLOW_TXOFF) != 0u) { if (!strv_add(out, out_count, "xon")) return false; }
    if ((mask & FLOW_CTS) != 0u) { if (!strv_add(out, out_count, "cts")) return false; }
    if ((mask & FLOW_DSR) != 0u) { if (!strv_add(out, out_count, "dsr")) return false; }
    return true;
}

static bool write_general_equipment_toml(FILE *fp, void *vctx, char *err, size_t err_len)
{
    ExportCtx *ctx = (ExportCtx *)vctx;

    MaxCfgNgEquipment equip;
    if (!check_st(maxcfg_ng_equipment_init(&equip), err, err_len)) {
        return false;
    }

    if (ctx && ctx->maxctl_path && ctx->maxctl_path[0]) {
        char buf[512];

        if (maxctl_find_value(ctx->maxctl_path, "Output", buf, sizeof(buf))) {
            if (strncasecmp(buf, "local", 5) == 0) {
                if (!dup_str(&equip.output, "local", err, err_len)) goto fail;
            } else {
                if (!dup_str(&equip.output, "com", err, err_len)) goto fail;
                int port = 0;
                if (sscanf(buf, "Com%d", &port) == 1 && port > 0) {
                    equip.com_port = port;
                }
            }
        }

        if (maxctl_find_value(ctx->maxctl_path, "Baud Maximum", buf, sizeof(buf))) {
            equip.baud_maximum = atoi(buf);
        }
        if (maxctl_find_value(ctx->maxctl_path, "Busy", buf, sizeof(buf))) {
            if (!dup_str(&equip.busy, buf, err, err_len)) goto fail;
        }
        if (maxctl_find_value(ctx->maxctl_path, "Init", buf, sizeof(buf))) {
            if (!dup_str(&equip.init, buf, err, err_len)) goto fail;
        }
        if (maxctl_find_value(ctx->maxctl_path, "Ring", buf, sizeof(buf))) {
            if (!dup_str(&equip.ring, buf, err, err_len)) goto fail;
        }
        if (maxctl_find_value(ctx->maxctl_path, "Answer", buf, sizeof(buf))) {
            if (!dup_str(&equip.answer, buf, err, err_len)) goto fail;
        }
        if (maxctl_find_value(ctx->maxctl_path, "Connect", buf, sizeof(buf))) {
            if (!dup_str(&equip.connect, buf, err, err_len)) goto fail;
        }
        if (maxctl_find_value(ctx->maxctl_path, "Mask Carrier", buf, sizeof(buf))) {
            equip.carrier_mask = atoi(buf);
        }

        FILE *fpctl = fopen(ctx->maxctl_path, "r");
        if (fpctl) {
            char line[1024];
            while (fgets(line, sizeof(line), fpctl)) {
                char *s = trim_ws(line);
                if (s == NULL || s[0] == '\0') continue;
                if (s[0] == '%' || s[0] == ';') continue;

                if (line_starts_with_keyword(s, "Mask Handshaking")) {
                    char *v = extract_value_after_keyword(s, "Mask Handshaking");
                    if (v && v[0]) {
                        if (strcasecmp(v, "XON") == 0 || strcasecmp(v, "Xon") == 0 || strcasecmp(v, "xon") == 0) {
                            (void)strv_add(&equip.handshaking, &equip.handshaking_count, "xon");
                        } else if (strcasecmp(v, "CTS") == 0 || strcasecmp(v, "cts") == 0) {
                            (void)strv_add(&equip.handshaking, &equip.handshaking_count, "cts");
                        } else if (strcasecmp(v, "DSR") == 0 || strcasecmp(v, "dsr") == 0) {
                            (void)strv_add(&equip.handshaking, &equip.handshaking_count, "dsr");
                        }
                    }
                }
            }
            fclose(fpctl);
        }

        bool b;
        if (maxctl_has_keyword(ctx->maxctl_path, "Send Break to Clear Buffer", &b)) {
            equip.send_break = b;
        }
        if (maxctl_has_keyword(ctx->maxctl_path, "No Critical Handler", &b)) {
            equip.no_critical = b;
        }

        if (equip.output == NULL) {
            if (!dup_str(&equip.output, "com", err, err_len)) goto fail;
        }
        if (equip.com_port == 0) equip.com_port = 1;
    }


    MaxCfgStatus st = maxcfg_ng_write_equipment_toml(fp, &equip);
    maxcfg_ng_equipment_free(&equip);
    return check_st(st, err, err_len);

fail:
    maxcfg_ng_equipment_free(&equip);
    return false;
}

typedef struct __attribute__((packed, aligned(2))) {
    word flag;
    char desc[40];
    char log[PATHLEN];
    char ctl[PATHLEN];
    char dlcmd[PATHLEN];
    char ulcmd[PATHLEN];
    char dlstr[40];
    char ulstr[40];
    char dlkey[40];
    char ulkey[40];
    word fnamword;
    word descword;
} NgProtoMaxRecord;

static bool read_protocol_max_record(const char *path, int index, NgProtoMaxRecord *out)
{
    if (path == NULL || out == NULL || index < 0) {
        return false;
    }

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return false;
    }

    long off = (long)sizeof(NgProtoMaxRecord) * (long)index;
    if (fseek(fp, off, SEEK_SET) != 0) {
        fclose(fp);
        return false;
    }

    size_t n = fread(out, 1, sizeof(*out), fp);
    fclose(fp);
    return n == sizeof(*out);
}

static bool protocol_max_path(char *out, size_t out_len, const char *sys_path)
{
    if (out == NULL || out_len == 0) {
        return false;
    }

    return join_sys_path(out, out_len, sys_path, "protocol.max");
}

static void protocol_unquote(const char *in, char *out, size_t out_sz)
{
    if (out == NULL || out_sz == 0) {
        return;
    }
    out[0] = '\0';
    if (in == NULL) {
        return;
    }

    const char *p = in;
    size_t w = 0;
    if (*p == '"') {
        p++;
        for (; *p && w + 1 < out_sz; p++) {
            if (*p == '"') {
                if (p[1] == '"') {
                    out[w++] = '"';
                    p++;
                    continue;
                }
                break;
            }
            out[w++] = *p;
        }
        out[w] = '\0';
        return;
    }

    snprintf(out, out_sz, "%s", in);
}

static bool protocol_ctl_path(char *out, size_t out_len, const char *sys_path)
{
    if (out == NULL || out_len == 0) {
        return false;
    }
    return join_sys_path(out, out_len, sys_path, "etc/protocol.ctl");
}

static bool parse_protocol_ctl_into_list(const char *path, const char *pmax_path, bool have_pmax, MaxCfgNgProtocolList *list, char *err, size_t err_len)
{
    if (path == NULL || list == NULL) {
        set_err(err, err_len, "Invalid argument");
        return false;
    }

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return true;
    }

    bool in_proto = false;
    int cur_index = -1;

    char name[64];
    char log_file[PATHLEN];
    char control_file[PATHLEN];
    char download_cmd[PATHLEN];
    char upload_cmd[PATHLEN];
    char download_string[64];
    char upload_string[64];
    char download_keyword[64];
    char upload_keyword[64];
    int filename_word = 0;
    int descript_word = 0;
    bool batch = false;
    bool exitlevel = false;
    bool opus = false;
    bool bi = false;

    memset(name, 0, sizeof(name));
    memset(log_file, 0, sizeof(log_file));
    memset(control_file, 0, sizeof(control_file));
    memset(download_cmd, 0, sizeof(download_cmd));
    memset(upload_cmd, 0, sizeof(upload_cmd));
    memset(download_string, 0, sizeof(download_string));
    memset(upload_string, 0, sizeof(upload_string));
    memset(download_keyword, 0, sizeof(download_keyword));
    memset(upload_keyword, 0, sizeof(upload_keyword));

    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *s = trim_ws(line);
        if (s == NULL || !*s) continue;
        if (*s == ';' || *s == '%') continue;

        char *cmt = strchr(s, ';');
        if (cmt) {
            *cmt = '\0';
            s = trim_ws(s);
            if (s == NULL || !*s) continue;
        }

        char key[64];
        char rest[512];
        key[0] = '\0';
        rest[0] = '\0';

        int n = sscanf(s, "%63s %511[^\r\n]", key, rest);
        if (n <= 0) {
            continue;
        }

        if (strcasecmp(key, "protocol") == 0) {
            in_proto = true;
            cur_index = (int)list->count;
            strncpy(name, trim_ws(rest), sizeof(name) - 1);

            memset(log_file, 0, sizeof(log_file));
            memset(control_file, 0, sizeof(control_file));
            memset(download_cmd, 0, sizeof(download_cmd));
            memset(upload_cmd, 0, sizeof(upload_cmd));
            memset(download_string, 0, sizeof(download_string));
            memset(upload_string, 0, sizeof(upload_string));
            memset(download_keyword, 0, sizeof(download_keyword));
            memset(upload_keyword, 0, sizeof(upload_keyword));
            filename_word = 0;
            descript_word = 0;
            batch = false;
            exitlevel = false;
            opus = false;
            bi = false;
            continue;
        }

        if (!in_proto) {
            continue;
        }

        if (strcasecmp(key, "end") == 0) {
            MaxCfgNgProtocol p;
            memset(&p, 0, sizeof(p));
            p.index = cur_index;
            p.name = name;

            p.program = (char *)"";
            p.batch = batch;
            p.exitlevel = exitlevel;
            p.opus = opus;
            p.bi = bi;
            p.log_file = log_file;
            p.control_file = control_file;
            p.download_cmd = download_cmd;
            p.upload_cmd = upload_cmd;
            p.download_string = download_string;
            p.upload_string = upload_string;
            p.download_keyword = download_keyword;
            p.upload_keyword = upload_keyword;
            p.filename_word = filename_word;
            p.descript_word = descript_word;

            NgProtoMaxRecord rec;
            memset(&rec, 0, sizeof(rec));
            bool got_rec = (have_pmax && pmax_path) ? read_protocol_max_record(pmax_path, cur_index, &rec) : false;
            if (got_rec) {
                if (rec.desc[0] && (!p.name || !*p.name)) p.name = rec.desc;
                if ((rec.flag & 0x04u) != 0u) p.opus = true;
                if ((rec.flag & 0x10u) != 0u) p.bi = true;
                if ((rec.flag & 0x02u) != 0u) p.batch = true;
                if ((rec.flag & 0x08u) != 0u) p.exitlevel = true;
                if (p.program == NULL || p.program[0] == '\0') {
                    p.program = (char *)"";
                }
                p.log_file = rec.log;
                p.control_file = rec.ctl;
                p.download_cmd = rec.dlcmd;
                p.upload_cmd = rec.ulcmd;
                p.download_string = rec.dlstr;
                p.upload_string = rec.ulstr;
                p.download_keyword = rec.dlkey;
                p.upload_keyword = rec.ulkey;
                p.filename_word = (int)rec.fnamword;
                p.descript_word = (int)rec.descword;
            }

            if (!check_st(maxcfg_ng_protocol_list_add(list, &p), err, err_len)) {
                fclose(fp);
                return false;
            }

            in_proto = false;
            cur_index = -1;
            continue;
        }

        char word2[128];
        word2[0] = '\0';
        (void)sscanf(rest, "%127s", word2);

        if (strcasecmp(key, "logfile") == 0) {
            strncpy(log_file, word2, sizeof(log_file) - 1);
        } else if (strcasecmp(key, "controlfile") == 0) {
            strncpy(control_file, word2, sizeof(control_file) - 1);
        } else if (strcasecmp(key, "downloadcmd") == 0) {
            strncpy(download_cmd, trim_ws(rest), sizeof(download_cmd) - 1);
        } else if (strcasecmp(key, "uploadcmd") == 0) {
            strncpy(upload_cmd, trim_ws(rest), sizeof(upload_cmd) - 1);
        } else if (strcasecmp(key, "downloadstring") == 0) {
            strncpy(download_string, trim_ws(rest), sizeof(download_string) - 1);
        } else if (strcasecmp(key, "uploadstring") == 0) {
            strncpy(upload_string, trim_ws(rest), sizeof(upload_string) - 1);
        } else if (strcasecmp(key, "downloadkeyword") == 0) {
            protocol_unquote(trim_ws(rest), download_keyword, sizeof(download_keyword));
        } else if (strcasecmp(key, "uploadkeyword") == 0) {
            protocol_unquote(trim_ws(rest), upload_keyword, sizeof(upload_keyword));
        } else if (strcasecmp(key, "filenameword") == 0) {
            filename_word = atoi(word2);
        } else if (strcasecmp(key, "descriptword") == 0) {
            descript_word = atoi(word2);
        } else if (strcasecmp(key, "type") == 0) {
            if (strcasecmp(word2, "batch") == 0) batch = true;
            else if (strcasecmp(word2, "bi") == 0) bi = true;
            else if (strcasecmp(word2, "opus") == 0) opus = true;
            else if (strcasecmp(word2, "errorlevel") == 0) exitlevel = true;
        }
    }

    fclose(fp);
    return true;
}

static bool write_general_protocols_toml(FILE *fp, void *vctx, char *err, size_t err_len)
{
    ExportCtx *ctx = (ExportCtx *)vctx;

    MaxCfgNgProtocolList list;
    if (!check_st(maxcfg_ng_protocol_list_init(&list), err, err_len)) {
        return false;
    }

    if (ctx && ctx->maxctl_path && ctx->maxctl_path[0]) {
        char buf[256];
        if (maxctl_find_value(ctx->maxctl_path, "External Protocol Errorlevel", buf, sizeof(buf))) {
            list.protoexit = atoi(buf);
        }
    }
    char pmax_path[PATH_MAX];
    const char *sys = (ctx && ctx->sys_path) ? ctx->sys_path : ".";
    bool have_pmax = protocol_max_path(pmax_path, sizeof(pmax_path), sys);

    if (have_pmax) {
        list.protocol_max_path = strdup(pmax_path);
        if (list.protocol_max_path == NULL) {
            maxcfg_ng_protocol_list_free(&list);
            set_err(err, err_len, "Out of memory");
            return false;
        }
        struct stat st;
        list.protocol_max_exists = (stat(pmax_path, &st) == 0 && S_ISREG(st.st_mode));
    }

    char pctl_path[PATH_MAX];
    bool have_ctl = protocol_ctl_path(pctl_path, sizeof(pctl_path), sys);
    if (have_ctl) {
        list.protocol_ctl_path = strdup(pctl_path);
        if (list.protocol_ctl_path == NULL) {
            maxcfg_ng_protocol_list_free(&list);
            set_err(err, err_len, "Out of memory");
            return false;
        }
        struct stat st;
        list.protocol_ctl_exists = (stat(pctl_path, &st) == 0 && S_ISREG(st.st_mode));
    }
    if (have_ctl) {
        if (!parse_protocol_ctl_into_list(pctl_path, pmax_path, have_pmax, &list, err, err_len)) {
            maxcfg_ng_protocol_list_free(&list);
            return false;
        }
    }

    /* protocol_ctl_exists is now a semantic indicator: do we have any
     * protocol definitions in the exported TOML (table count > 0)?
     */
    list.protocol_ctl_exists = (list.count > 0);

    MaxCfgStatus st = maxcfg_ng_write_protocols_toml(fp, &list);
    maxcfg_ng_protocol_list_free(&list);
    return check_st(st, err, err_len);
}

static bool write_general_language_toml(FILE *fp, void *vctx, char *err, size_t err_len)
{
    ExportCtx *ctx = (ExportCtx *)vctx;

    if (!ctx || !ctx->sys_path) {
        set_err(err, err_len, "No sys_path provided");
        return false;
    }

    MaxCfgNgLanguage lang;
    if (!check_st(maxcfg_ng_language_init(&lang), err, err_len)) {
        return false;
    }

    if (!ctl_to_ng_populate_language(ctx->sys_path, &lang)) {
        set_err(err, err_len, "Failed to parse language.ctl");
        maxcfg_ng_language_free(&lang);
        return false;
    }

    /* Runtime-only sizing fields are NOT exported - they are computed at runtime */
    lang.max_ptrs = 0;
    lang.max_heap = 0;
    lang.max_glh_ptrs = 0;
    lang.max_glh_len = 0;
    lang.max_syh_ptrs = 0;
    lang.max_syh_len = 0;

    MaxCfgStatus st = maxcfg_ng_write_language_toml(fp, &lang);
    maxcfg_ng_language_free(&lang);
    return check_st(st, err, err_len);
}

static bool write_general_session_toml(FILE *fp, void *vctx, char *err, size_t err_len)
{
    ExportCtx *ctx = (ExportCtx *)vctx;

    if (!ctx || !ctx->maxctl_path) {
        set_err(err, err_len, "No max.ctl path provided");
        return false;
    }

    MaxCfgNgGeneralSession session;
    if (!check_st(maxcfg_ng_general_session_init(&session), err, err_len)) {
        return false;
    }

    if (!ctl_to_ng_populate_session(ctx->maxctl_path, &session)) {
        set_err(err, err_len, "Failed to parse max.ctl for session configuration");
        maxcfg_ng_general_session_free(&session);
        return false;
    }

    MaxCfgStatus st = maxcfg_ng_write_general_session_toml(fp, &session);
    maxcfg_ng_general_session_free(&session);
    return check_st(st, err, err_len);
}

static bool write_general_display_files_toml(FILE *fp, void *vctx, char *err, size_t err_len)
{
    ExportCtx *ctx = (ExportCtx *)vctx;

    if (!ctx || !ctx->maxctl_path) {
        set_err(err, err_len, "No max.ctl path provided");
        return false;
    }

    MaxCfgNgGeneralDisplayFiles files;
    if (!check_st(maxcfg_ng_general_display_files_init(&files), err, err_len)) {
        return false;
    }

    if (!ctl_to_ng_populate_display_files(ctx->maxctl_path, &files)) {
        set_err(err, err_len, "Failed to parse max.ctl for display files configuration");
        maxcfg_ng_general_display_files_free(&files);
        return false;
    }

    MaxCfgStatus st = maxcfg_ng_write_general_display_files_toml(fp, &files);
    maxcfg_ng_general_display_files_free(&files);
    return check_st(st, err, err_len);
}

static char *trim_ws(char *s)
{
    if (s == NULL) {
        return NULL;
    }

    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') {
        s++;
    }

    size_t n = strlen(s);
    while (n > 0) {
        char c = s[n - 1];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            s[n - 1] = '\0';
            n--;
        } else {
            break;
        }
    }

    return s;
}

static void free_access_level_tmp(MaxCfgNgAccessLevel *lvl)
{
    if (lvl == NULL) {
        return;
    }

    free(lvl->name);
    free(lvl->description);
    free(lvl->alias);
    free(lvl->key);
    free(lvl->login_file);
    strv_free(&lvl->flags, &lvl->flag_count);
    strv_free(&lvl->mail_flags, &lvl->mail_flag_count);
    memset(lvl, 0, sizeof(*lvl));
}

static void init_access_level_defaults(MaxCfgNgAccessLevel *lvl)
{
    memset(lvl, 0, sizeof(*lvl));
    lvl->calls = -1;
    lvl->logon_baud = 300;
    lvl->xfer_baud = 300;
}

static bool parse_access_ctl(const char *path, MaxCfgNgAccessLevelList *out, char *err, size_t err_len)
{
    if (path == NULL || out == NULL) {
        set_err(err, err_len, "Invalid argument");
        return false;
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        set_err(err, err_len, "Failed to open %s (%s)", path, strerror(errno));
        return false;
    }

    if (!check_st(maxcfg_ng_access_level_list_init(out), err, err_len)) {
        fclose(fp);
        return false;
    }

    MaxCfgNgAccessLevel cur;
    bool in_block = false;
    init_access_level_defaults(&cur);

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        char *s = trim_ws(line);
        if (s == NULL || s[0] == '\0') {
            continue;
        }
        if (s[0] == '%' || s[0] == ';') {
            continue;
        }

        if (strncmp(s, "Access ", 7) == 0) {
            if (in_block) {
                /* Implicit close */
                MaxCfgStatus st_add = maxcfg_ng_access_level_list_add(out, &cur);
                free_access_level_tmp(&cur);
                init_access_level_defaults(&cur);
                if (!check_st(st_add, err, err_len)) {
                    fclose(fp);
                    maxcfg_ng_access_level_list_free(out);
                    return false;
                }
            }

            in_block = true;
            char *name = trim_ws(s + 7);
            if (name && name[0]) {
                cur.name = strdup(name);
                if (!cur.name) {
                    set_err(err, err_len, "Out of memory");
                    fclose(fp);
                    maxcfg_ng_access_level_list_free(out);
                    return false;
                }
            }
            continue;
        }

        if (strcmp(s, "End Access") == 0) {
            if (in_block) {
                MaxCfgStatus st_add = maxcfg_ng_access_level_list_add(out, &cur);
                free_access_level_tmp(&cur);
                init_access_level_defaults(&cur);
                in_block = false;
                if (!check_st(st_add, err, err_len)) {
                    fclose(fp);
                    maxcfg_ng_access_level_list_free(out);
                    return false;
                }
            }
            continue;
        }

        if (!in_block) {
            continue;
        }

        char key[64];
        const char *rest = "";
        {
            char *sp = s;
            size_t k = 0;
            while (*sp && *sp != ' ' && *sp != '\t' && k + 1 < sizeof(key)) {
                key[k++] = *sp++;
            }
            key[k] = '\0';
            rest = trim_ws(sp);
        }

        if (strcmp(key, "Level") == 0) {
            cur.level = (int)strtol(rest, NULL, 10);
        } else if (strcmp(key, "Desc") == 0) {
            free(cur.description);
            cur.description = strdup(rest);
            if (rest[0] && !cur.description) goto oom;
        } else if (strcmp(key, "Alias") == 0) {
            free(cur.alias);
            cur.alias = strdup(rest);
            if (rest[0] && !cur.alias) goto oom;
        } else if (strcmp(key, "Key") == 0) {
            free(cur.key);
            cur.key = strdup(rest);
            if (rest[0] && !cur.key) goto oom;
        } else if (strcmp(key, "Time") == 0) {
            cur.time = (int)strtol(rest, NULL, 10);
        } else if (strcmp(key, "Cume") == 0) {
            cur.cume = (int)strtol(rest, NULL, 10);
        } else if (strcmp(key, "Calls") == 0) {
            cur.calls = (int)strtol(rest, NULL, 10);
        } else if (strcmp(key, "LogonBaud") == 0) {
            cur.logon_baud = (int)strtol(rest, NULL, 10);
        } else if (strcmp(key, "XferBaud") == 0) {
            cur.xfer_baud = (int)strtol(rest, NULL, 10);
        } else if (strcmp(key, "FileLimit") == 0) {
            cur.file_limit = (int)strtol(rest, NULL, 10);
        } else if (strcmp(key, "FileRatio") == 0) {
            cur.file_ratio = (int)strtol(rest, NULL, 10);
        } else if (strcmp(key, "RatioFree") == 0) {
            cur.ratio_free = (int)strtol(rest, NULL, 10);
        } else if (strcmp(key, "UploadReward") == 0) {
            cur.upload_reward = (int)strtol(rest, NULL, 10);
        } else if (strcmp(key, "LoginFile") == 0) {
            free(cur.login_file);
            cur.login_file = strdup(rest);
            if (rest[0] && !cur.login_file) goto oom;
        } else if (strcmp(key, "Flags") == 0) {
            if (!strv_add_words(&cur.flags, &cur.flag_count, rest)) goto oom;
        } else if (strcmp(key, "MailFlags") == 0) {
            if (!strv_add_words(&cur.mail_flags, &cur.mail_flag_count, rest)) goto oom;
        } else if (strcmp(key, "UserFlags") == 0) {
            const char *p = rest;
            int base = 0;
            if (*p == '$') {
                p++;
                base = 16;
            }
            cur.user_flags = (unsigned int)strtoul(p, NULL, base);
        } else if (strcmp(key, "Oldpriv") == 0) {
            cur.oldpriv = (int)strtol(rest, NULL, 10);
        }
    }

    fclose(fp);
    return true;

oom:
    set_err(err, err_len, "Out of memory");
    fclose(fp);
    free_access_level_tmp(&cur);
    maxcfg_ng_access_level_list_free(out);
    return false;
}

static bool write_security_access_levels_toml(FILE *fp, void *vctx, char *err, size_t err_len)
{
    ExportCtx *ctx = (ExportCtx *)vctx;
    if (!ctx || !ctx->sys_path) {
        return true;
    }

    char in_path[PATH_MAX];
    if (snprintf(in_path, sizeof(in_path), "%s/etc/access.ctl", ctx->sys_path) >= (int)sizeof(in_path)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    MaxCfgNgAccessLevelList levels;
    if (!parse_access_ctl(in_path, &levels, err, err_len)) {
        return false;
    }

    MaxCfgStatus st = maxcfg_ng_write_access_levels_toml(fp, &levels);
    maxcfg_ng_access_level_list_free(&levels);
    return check_st(st, err, err_len);
}

static bool parse_avatar_color_byte(int byte, MaxCfgNgColor *out)
{
    if (out == NULL) {
        return false;
    }

    if (byte < 0 || byte > 255) {
        return false;
    }

    out->fg = byte & 0x0f;
    out->bg = (byte >> 4) & 0x07;
    out->blink = ((byte >> 7) & 0x01) != 0;
    return true;
}

static bool parse_colors_lh_for_define(const char *path, const char *define_name, MaxCfgNgColor *out)
{
    if (path == NULL || define_name == NULL || out == NULL) {
        return false;
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        return false;
    }

    char line[512];
    bool ok = false;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "#define", 7) != 0) {
            continue;
        }

        char name[128];
        if (sscanf(line, "#define %127s", name) != 1) {
            continue;
        }

        if (strcmp(name, define_name) != 0) {
            continue;
        }

        const char *p = strstr(line, "\\x16\\x01\\x");
        if (!p) {
            break;
        }
        p += strlen("\\x16\\x01\\x");

        unsigned int b = 0;
        if (sscanf(p, "%2x", &b) != 1) {
            break;
        }

        (void)parse_avatar_color_byte((int)b, out);

        /* Blink is encoded as a trailing \x16\x02 sequence (optional) */
        out->blink = strstr(line, "\\x16\\x02") != NULL;
        ok = true;
        break;
    }

    fclose(fp);
    return ok;
}

static bool write_general_colors_toml(FILE *fp, void *vctx, char *err, size_t err_len)
{
    ExportCtx *ctx = (ExportCtx *)vctx;

    if (!ctx || !ctx->sys_path) {
        return true;
    }

    MaxCfgNgGeneralColors colors;
    if (!check_st(maxcfg_ng_general_colors_init(&colors), err, err_len)) {
        return false;
    }

    char colors_lh_path[PATH_MAX];
    if (snprintf(colors_lh_path, sizeof(colors_lh_path), "%s/etc/lang/colors.lh", ctx->sys_path) >= (int)sizeof(colors_lh_path)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    /* Best-effort parse; missing entries simply stay 0/0/false */
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MNU_NAME", &colors.menu_name);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MNU_HILITE", &colors.menu_highlight);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MNU_OPTION", &colors.menu_option);

    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FILE_NAME", &colors.file_name);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FILE_SIZE", &colors.file_size);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FILE_DATE", &colors.file_date);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FILE_DESC", &colors.file_description);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FILE_FIND", &colors.file_search_match);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FILE_OFFLN", &colors.file_offline);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FILE_NEW", &colors.file_new);

    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_FROM", &colors.msg_from_label);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_FROMTXT", &colors.msg_from_text);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_TO", &colors.msg_to_label);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_TOTXT", &colors.msg_to_text);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_SUBJ", &colors.msg_subject_label);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_SUBJTXT", &colors.msg_subject_text);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_ATTR", &colors.msg_attributes);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_DATE", &colors.msg_date);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_ADDR", &colors.msg_address);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_LOCUS", &colors.msg_locus);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_BODY", &colors.msg_body);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_QUOTE", &colors.msg_quote);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_MSG_KLUDGE", &colors.msg_kludge);

    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FSR_MSGNUM", &colors.fsr_msgnum);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FSR_LINKS", &colors.fsr_links);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FSR_ATTRIB", &colors.fsr_attrib);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FSR_MSGINFO", &colors.fsr_msginfo);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FSR_DATE", &colors.fsr_date);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FSR_ADDR", &colors.fsr_addr);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FSR_STATIC", &colors.fsr_static);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FSR_BORDER", &colors.fsr_border);
    (void)parse_colors_lh_for_define(colors_lh_path, "COL_FSR_LOCUS", &colors.fsr_locus);

    MaxCfgStatus st = maxcfg_ng_write_general_colors_toml(fp, &colors);
    return check_st(st, err, err_len);
}

static bool mkdir_p(const char *path)
{
    if (!path || !path[0]) return false;

    char tmp[PATH_MAX];
    size_t len = strlen(path);
    if (len >= sizeof(tmp)) return false;

    memcpy(tmp, path, len + 1);

    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (!ensure_dir(tmp)) return false;
            *p = '/';
        }
    }

    if (!ensure_dir(tmp)) return false;
    return true;
}

static bool ensure_parent_dir(const char *path)
{
    if (!path || !path[0]) return false;

    const char *slash = strrchr(path, '/');
    if (!slash) {
        return true;
    }

    size_t parent_len = (size_t)(slash - path);
    if (parent_len == 0) {
        return true;
    }

    char parent[PATH_MAX];
    if (parent_len >= sizeof(parent)) {
        return false;
    }

    memcpy(parent, path, parent_len);
    parent[parent_len] = '\0';
    return mkdir_p(parent);
}

static void normalize_filename_component(const char *in, char *out, size_t out_sz)
{
    if (!out || out_sz == 0) return;

    size_t w = 0;
    if (!in) {
        out[0] = '\0';
        return;
    }

    for (const unsigned char *p = (const unsigned char *)in; *p && w + 1 < out_sz; p++) {
        unsigned char c = *p;
        if (c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');

        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.') {
            out[w++] = (char)c;
        } else {
            out[w++] = '_';
        }
    }

    out[w] = '\0';
}

typedef struct {
    char final_path[PATH_MAX];
    char backup_path[PATH_MAX];
    bool had_backup;
    bool committed;
} ExportOp;

static bool dup_str(char **out, const char *in, char *err, size_t err_len)
{
    if (out == NULL) {
        set_err(err, err_len, "Invalid argument");
        return false;
    }

    *out = NULL;
    if (in == NULL) {
        return true;
    }

    *out = strdup(in);
    if (*out == NULL) {
        set_err(err, err_len, "Out of memory");
        return false;
    }

    return true;
}

static bool check_st(MaxCfgStatus st, char *err, size_t err_len)
{
    if (st == MAXCFG_OK) {
        return true;
    }
    set_err(err, err_len, "%s", maxcfg_status_string(st));
    return false;
}

static bool begin_backup_into_txn(ExportOp *op, const char *config_dir, const char *txn_dir, const char *final_path, int unique_id, char *err, size_t err_len)
{
    struct stat st;

    memset(op, 0, sizeof(*op));

    if (!final_path || !final_path[0]) {
        set_err(err, err_len, "Invalid export path");
        return false;
    }

    if (snprintf(op->final_path, sizeof(op->final_path), "%s", final_path) >= (int)sizeof(op->final_path)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    if (stat(final_path, &st) == 0 && S_ISREG(st.st_mode)) {
        const char *rel = final_path;
        size_t cfg_len = strlen(config_dir);
        if (strncmp(final_path, config_dir, cfg_len) == 0) {
            rel = final_path + cfg_len;
            while (*rel == '/') rel++;
        }

        char rel_norm[PATH_MAX];
        normalize_filename_component(rel, rel_norm, sizeof(rel_norm));

        if (snprintf(op->backup_path, sizeof(op->backup_path), "%s/%s.%d.bak", txn_dir, rel_norm, unique_id) >= (int)sizeof(op->backup_path)) {
            set_err(err, err_len, "Backup path too long");
            return false;
        }

        if (!ensure_parent_dir(op->backup_path)) {
            set_err(err, err_len, "Failed to create backup directory");
            return false;
        }

        if (rename(final_path, op->backup_path) != 0) {
            set_err(err, err_len, "Failed to backup %s (%s)", final_path, strerror(errno));
            return false;
        }

        op->had_backup = true;
    }

    return true;
}

static bool write_file_atomic_and_commit(ExportOp *op,
                                        bool (*write_fn)(FILE *fp, void *ctx, char *err, size_t err_len),
                                        void *ctx,
                                        char *err,
                                        size_t err_len)
{
    char new_path[PATH_MAX];

    if (!ensure_parent_dir(op->final_path)) {
        set_err(err, err_len, "Failed to create output directory");
        return false;
    }

    if (snprintf(new_path, sizeof(new_path), "%s.new", op->final_path) >= (int)sizeof(new_path)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    FILE *fp = fopen(new_path, "w");
    if (!fp) {
        set_err(err, err_len, "Failed to open %s (%s)", new_path, strerror(errno));
        return false;
    }

    bool ok = write_fn(fp, ctx, err, err_len);

    if (fclose(fp) != 0) {
        ok = false;
        if (!err || !err[0]) {
            set_err(err, err_len, "Failed to close %s (%s)", new_path, strerror(errno));
        }
    }

    if (!ok) {
        unlink(new_path);
        return false;
    }

    if (rename(new_path, op->final_path) != 0) {
        unlink(new_path);
        set_err(err, err_len, "Failed to commit %s (%s)", op->final_path, strerror(errno));
        return false;
    }

    op->committed = true;
    return true;
}

static void rollback_ops(ExportOp *ops, int op_count)
{
    for (int i = op_count - 1; i >= 0; i--) {
        ExportOp *op = &ops[i];
        if (op->committed) {
            if (op->had_backup) {
                unlink(op->final_path);
                (void)rename(op->backup_path, op->final_path);
            } else {
                unlink(op->final_path);
            }
        } else {
            if (op->had_backup) {
                (void)rename(op->backup_path, op->final_path);
            }
        }
    }
}

static void cleanup_ops_and_txn(ExportOp *ops, int op_count, const char *txn_dir)
{
    for (int i = 0; i < op_count; i++) {
        ExportOp *op = &ops[i];
        if (op->had_backup && op->backup_path[0]) {
            unlink(op->backup_path);
        }
    }

    if (txn_dir && txn_dir[0]) {
        (void)rmdir(txn_dir);
    }
}

static bool write_maximus_toml(FILE *fp, void *vctx, char *err, size_t err_len)
{
    ExportCtx *ctx = (ExportCtx *)vctx;

    if (!ctx || !ctx->maxctl_path) {
        set_err(err, err_len, "No max.ctl path provided");
        return false;
    }

    MaxCfgNgSystem sys;
    if (!check_st(maxcfg_ng_system_init(&sys), err, err_len)) {
        return false;
    }

    sys.config_version = 1;

    if (!ctl_to_ng_populate_system(ctx->maxctl_path, ctx->sys_path, ctx->config_dir, &sys)) {
        set_err(err, err_len, "Failed to parse max.ctl for system configuration");
        maxcfg_ng_system_free(&sys);
        return false;
    }

    MaxCfgStatus st = maxcfg_ng_write_maximus_toml(fp, &sys);
    maxcfg_ng_system_free(&sys);
    return check_st(st, err, err_len);
}

typedef struct {
    const char *sys_path;
    MenuDefinition *menu;
} MenuWriteCtx;

static bool write_menu_toml(FILE *fp, void *vctx, char *err, size_t err_len)
{
    MenuWriteCtx *ctx = (MenuWriteCtx *)vctx;

    if (!ctx || !ctx->menu) {
        return true;
    }

    MaxCfgNgMenu menu;
    if (!check_st(maxcfg_ng_menu_init(&menu), err, err_len)) {
        return false;
    }

    if (!dup_str(&menu.name, ctx->menu->name, err, err_len)) goto fail;
    if (!dup_str(&menu.title, ctx->menu->title, err, err_len)) goto fail;
    if (!dup_str(&menu.header_file, ctx->menu->header_file, err, err_len)) goto fail;
    if (!dup_str(&menu.menu_file, ctx->menu->menu_file, err, err_len)) goto fail;

    if (!menu_types_from_flags(ctx->menu->header_flags, true, &menu.header_types, &menu.header_type_count)) {
        set_err(err, err_len, "Out of memory");
        goto fail;
    }
    if (!menu_types_from_flags(ctx->menu->menu_flags, false, &menu.menu_types, &menu.menu_type_count)) {
        set_err(err, err_len, "Out of memory");
        goto fail;
    }

    menu.menu_length = ctx->menu->menu_length;
    menu.menu_color = ctx->menu->menu_color;
    menu.option_width = ctx->menu->opt_width;

    for (int i = 0; i < ctx->menu->option_count; i++) {
        MenuOption *opt = ctx->menu->options[i];
        if (!opt) continue;

        char **mods = NULL;
        size_t mod_count = 0;
        if (!menu_option_modifiers_from_bits(opt, &mods, &mod_count)) {
            strv_free(&mods, &mod_count);
            set_err(err, err_len, "Out of memory");
            goto fail;
        }

        MaxCfgNgMenuOption ngopt = {
            .command = opt->command,
            .arguments = opt->arguments,
            .priv_level = opt->priv_level,
            .description = opt->description,
            .key_poke = opt->key_poke,
            .modifiers = mods,
            .modifier_count = mod_count
        };

        if (!check_st(maxcfg_ng_menu_add_option(&menu, &ngopt), err, err_len)) {
            strv_free(&mods, &mod_count);
            goto fail;
        }

        strv_free(&mods, &mod_count);
    }

    MaxCfgStatus st = maxcfg_ng_write_menu_toml(fp, &menu);
    maxcfg_ng_menu_free(&menu);
    return check_st(st, err, err_len);

fail:
    maxcfg_ng_menu_free(&menu);
    return false;
}

typedef struct {
    const char *sys_path;
    bool is_msg;
} AreasWriteCtx;

static bool build_divisions_recursive(MaxCfgNgDivisionList *divs, TreeNode *node, char *err, size_t err_len)
{
    if (!node) return true;

    if (node->type == TREENODE_DIVISION) {
        DivisionData *d = (DivisionData *)node->data;

        MaxCfgNgDivision div = {
            .name = node->name,
            .key = node->full_name,
            .description = node->description,
            .acs = d ? d->acs : NULL,
            .display_file = d ? d->display_file : NULL,
            .level = node->division_level
        };

        if (!check_st(maxcfg_ng_division_list_add(divs, &div), err, err_len)) {
            return false;
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        if (!build_divisions_recursive(divs, node->children[i], err, err_len)) {
            return false;
        }
    }

    return true;
}

static bool build_msg_areas_recursive(MaxCfgNgMsgAreaList *areas, TreeNode *node, const char *division_key, char *err, size_t err_len)
{
    if (!node) return true;

    const char *div_for_children = division_key;
    if (node->type == TREENODE_DIVISION) {
        div_for_children = node->full_name;
    } else if (node->type == TREENODE_AREA) {
        MsgAreaData *a = (MsgAreaData *)node->data;
        if (a) {
            char **styles = NULL;
            size_t style_count = 0;
            if (!msg_style_to_strings(a->style, &styles, &style_count)) {
                set_err(err, err_len, "Out of memory");
                return false;
            }

            MaxCfgNgMsgArea area = {
                .name = a->name,
                .description = a->desc,
                .acs = a->acs,
                .menu = a->menuname,
                .division = (char *)division_key,
                .tag = a->tag,
                .path = a->path,
                .owner = a->owner,
                .origin = a->origin,
                .attach_path = a->attachpath,
                .barricade = a->barricade,
                .style = styles,
                .style_count = style_count,
                .renum_max = a->renum_max,
                .renum_days = a->renum_days
            };

            if (!check_st(maxcfg_ng_msg_area_list_add(areas, &area), err, err_len)) {
                strv_free(&styles, &style_count);
                return false;
            }

            strv_free(&styles, &style_count);
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        if (!build_msg_areas_recursive(areas, node->children[i], div_for_children, err, err_len)) {
            return false;
        }
    }

    return true;
}

static bool build_file_areas_recursive(MaxCfgNgFileAreaList *areas, TreeNode *node, const char *division_key, char *err, size_t err_len)
{
    if (!node) return true;

    const char *div_for_children = division_key;
    if (node->type == TREENODE_DIVISION) {
        div_for_children = node->full_name;
    } else if (node->type == TREENODE_AREA) {
        FileAreaData *a = (FileAreaData *)node->data;
        if (a) {
            char **types = NULL;
            size_t type_count = 0;
            if (!file_area_types_from_bits(a, &types, &type_count)) {
                set_err(err, err_len, "Out of memory");
                return false;
            }

            MaxCfgNgFileArea area = {
                .name = a->name,
                .description = a->desc,
                .acs = a->acs,
                .menu = a->menuname,
                .division = (char *)division_key,
                .download = a->download,
                .upload = a->upload,
                .filelist = a->filelist,
                .barricade = a->barricade,
                .types = types,
                .type_count = type_count
            };

            if (!check_st(maxcfg_ng_file_area_list_add(areas, &area), err, err_len)) {
                strv_free(&types, &type_count);
                return false;
            }

            strv_free(&types, &type_count);
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        if (!build_file_areas_recursive(areas, node->children[i], div_for_children, err, err_len)) {
            return false;
        }
    }

    return true;
}

static bool write_areas_toml(FILE *fp, void *vctx, char *err, size_t err_len)
{
    AreasWriteCtx *ctx = (AreasWriteCtx *)vctx;

    if (!ctx || !ctx->sys_path) {
        return true;
    }

    /* TOML-first: if config/areas/<msg|file>/areas.toml exists, export from it.
     * Fallback to CTL parsing only when TOML sources are unavailable.
     */

    char src_path[PATH_MAX];
    if (snprintf(src_path, sizeof(src_path), "%s/config/areas/%s/areas.toml", ctx->sys_path, ctx->is_msg ? "msg" : "file") < (int)sizeof(src_path)) {
        struct stat st;
        if (stat(src_path, &st) == 0 && S_ISREG(st.st_mode)) {
            MaxCfgToml *toml = NULL;
            MaxCfgStatus ts = maxcfg_toml_init(&toml);
            if (ts == MAXCFG_OK) {
                const char *prefix = ctx->is_msg ? "areas.msg" : "areas.file";
                ts = maxcfg_toml_load_file(toml, src_path, prefix);
                if (ts == MAXCFG_OK) {
                    if (ctx->is_msg) {
                        MaxCfgNgDivisionList divs;
                        MaxCfgNgMsgAreaList areas;
                        if (!check_st(maxcfg_ng_division_list_init(&divs), err, err_len) ||
                            !check_st(maxcfg_ng_msg_area_list_init(&areas), err, err_len)) {
                            maxcfg_toml_free(toml);
                            return false;
                        }

                        ts = maxcfg_ng_get_msg_areas(toml, prefix, &divs, &areas);
                        if (ts == MAXCFG_OK) {
                            ts = maxcfg_ng_write_msg_areas_toml(fp, &divs, &areas);
                        }

                        maxcfg_ng_msg_area_list_free(&areas);
                        maxcfg_ng_division_list_free(&divs);
                        maxcfg_toml_free(toml);
                        return check_st(ts, err, err_len);
                    } else {
                        MaxCfgNgDivisionList divs;
                        MaxCfgNgFileAreaList areas;
                        if (!check_st(maxcfg_ng_division_list_init(&divs), err, err_len) ||
                            !check_st(maxcfg_ng_file_area_list_init(&areas), err, err_len)) {
                            maxcfg_toml_free(toml);
                            return false;
                        }

                        ts = maxcfg_ng_get_file_areas(toml, prefix, &divs, &areas);
                        if (ts == MAXCFG_OK) {
                            ts = maxcfg_ng_write_file_areas_toml(fp, &divs, &areas);
                        }

                        maxcfg_ng_file_area_list_free(&areas);
                        maxcfg_ng_division_list_free(&divs);
                        maxcfg_toml_free(toml);
                        return check_st(ts, err, err_len);
                    }
                }
                maxcfg_toml_free(toml);
            }
        }
    }

    /* Legacy fallback: export from CTL */
    char parse_err[512];
    parse_err[0] = '\0';

    if (ctx->is_msg) {
        int count = 0;
        TreeNode **roots = parse_msgarea_ctl(ctx->sys_path, &count, parse_err, sizeof(parse_err));
        if (!roots && parse_err[0]) {
            set_err(err, err_len, "%s", parse_err);
            return false;
        }

        MaxCfgNgDivisionList divs;
        MaxCfgNgMsgAreaList areas;
        if (!check_st(maxcfg_ng_division_list_init(&divs), err, err_len)) {
            free_msg_tree(roots, count);
            return false;
        }
        if (!check_st(maxcfg_ng_msg_area_list_init(&areas), err, err_len)) {
            maxcfg_ng_division_list_free(&divs);
            free_msg_tree(roots, count);
            return false;
        }

        bool ok = true;
        for (int i = 0; i < count && ok; i++) {
            ok = build_divisions_recursive(&divs, roots[i], err, err_len);
        }
        for (int i = 0; i < count && ok; i++) {
            ok = build_msg_areas_recursive(&areas, roots[i], NULL, err, err_len);
        }

        MaxCfgStatus stw = ok ? maxcfg_ng_write_msg_areas_toml(fp, &divs, &areas) : MAXCFG_ERR_IO;
        if (ok) ok = check_st(stw, err, err_len);

        maxcfg_ng_msg_area_list_free(&areas);
        maxcfg_ng_division_list_free(&divs);
        free_msg_tree(roots, count);
        return ok;
    } else {
        int count = 0;
        TreeNode **roots = parse_filearea_ctl(ctx->sys_path, &count, parse_err, sizeof(parse_err));
        if (!roots && parse_err[0]) {
            set_err(err, err_len, "%s", parse_err);
            return false;
        }

        MaxCfgNgDivisionList divs;
        MaxCfgNgFileAreaList areas;
        if (!check_st(maxcfg_ng_division_list_init(&divs), err, err_len)) {
            free_file_tree(roots, count);
            return false;
        }
        if (!check_st(maxcfg_ng_file_area_list_init(&areas), err, err_len)) {
            maxcfg_ng_division_list_free(&divs);
            free_file_tree(roots, count);
            return false;
        }

        bool ok = true;
        for (int i = 0; i < count && ok; i++) {
            ok = build_divisions_recursive(&divs, roots[i], err, err_len);
        }
        for (int i = 0; i < count && ok; i++) {
            ok = build_file_areas_recursive(&areas, roots[i], NULL, err, err_len);
        }

        MaxCfgStatus stw = ok ? maxcfg_ng_write_file_areas_toml(fp, &divs, &areas) : MAXCFG_ERR_IO;
        if (ok) ok = check_st(stw, err, err_len);

        maxcfg_ng_file_area_list_free(&areas);
        maxcfg_ng_division_list_free(&divs);
        free_file_tree(roots, count);
        return ok;
    }
}

static bool export_menus(const char *sys_path, const char *config_dir, const char *txn_dir, ExportOp *ops, int *op_count_io, int max_ops, char *err, size_t err_len)
{
    int menu_count = 0;
    MenuDefinition **menus = NULL;
    char **menu_paths = NULL;
    char **menu_prefixes = NULL;

    /* TOML-first: export from config/menus/<name>.toml when present */
    MaxCfgToml *toml = NULL;
    if (maxcfg_toml_init(&toml) == MAXCFG_OK) {
        if (!load_menus_toml(toml, sys_path, &menus, &menu_paths, &menu_prefixes, &menu_count, err, err_len)) {
            /* ignore and fall back */
            menus = NULL;
            menu_count = 0;
        }
        maxcfg_toml_free(toml);
    }

    char parse_err[512];
    parse_err[0] = '\0';
    if (menus == NULL || menu_count == 0) {
        menus = parse_menus_ctl(sys_path, &menu_count, parse_err, sizeof(parse_err));
        if (!menus && parse_err[0]) {
            set_err(err, err_len, "%s", parse_err);
            return false;
        }
    }

    for (int i = 0; i < menu_count; i++) {
        MenuDefinition *menu = menus[i];
        if (!menu || !menu->name || !menu->name[0]) continue;

        if (*op_count_io >= max_ops) {
            free_menu_definitions(menus, menu_count);
            set_err(err, err_len, "Too many export files");
            return false;
        }

        char name_norm[128];
        normalize_filename_component(menu->name, name_norm, sizeof(name_norm));

        char out_path[PATH_MAX];
        if (snprintf(out_path, sizeof(out_path), "%s/menus/%s.toml", config_dir, name_norm) >= (int)sizeof(out_path)) {
            free_menu_definitions(menus, menu_count);
            set_err(err, err_len, "Path too long");
            return false;
        }

        ExportOp *op = &ops[*op_count_io];
        if (!begin_backup_into_txn(op, config_dir, txn_dir, out_path, *op_count_io, err, err_len)) {
            free_menu_definitions(menus, menu_count);
            return false;
        }

        MenuWriteCtx mctx = {
            .sys_path = sys_path,
            .menu = menu
        };

        if (!write_file_atomic_and_commit(op, write_menu_toml, &mctx, err, err_len)) {
            free_menu_definitions(menus, menu_count);
            if (menu_paths && menu_prefixes) {
                for (int j = 0; j < menu_count; j++) {
                    free(menu_paths[j]);
                    free(menu_prefixes[j]);
                }
                free(menu_paths);
                free(menu_prefixes);
            }
            return false;
        }

        (*op_count_io)++;
    }

    free_menu_definitions(menus, menu_count);
    if (menu_paths && menu_prefixes) {
        for (int j = 0; j < menu_count; j++) {
            free(menu_paths[j]);
            free(menu_prefixes[j]);
        }
        free(menu_paths);
        free(menu_prefixes);
    }
    return true;
}

static bool export_system_commit_last(const char *sys_path, const char *config_dir, const char *txn_dir, ExportOp *ops, int *op_count_io, int max_ops, char *err, size_t err_len)
{
    if (*op_count_io >= max_ops) {
        set_err(err, err_len, "Too many export files");
        return false;
    }

    char out_path[PATH_MAX];
    if (snprintf(out_path, sizeof(out_path), "%s/maximus.toml", config_dir) >= (int)sizeof(out_path)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    ExportOp *op = &ops[*op_count_io];
    if (!begin_backup_into_txn(op, config_dir, txn_dir, out_path, *op_count_io, err, err_len)) {
        return false;
    }

    ExportCtx ctx = {
        .sys_path = sys_path,
        .config_dir = config_dir,
        .maxctl_path = NULL
    };

    char maxctl_path[PATH_MAX];
    struct stat mst;
    if (snprintf(maxctl_path, sizeof(maxctl_path), "%s/etc/max.ctl", sys_path) < (int)sizeof(maxctl_path) &&
        stat(maxctl_path, &mst) == 0 && S_ISREG(mst.st_mode)) {
        ctx.maxctl_path = maxctl_path;
    }

    if (!write_file_atomic_and_commit(op, write_maximus_toml, &ctx, err, err_len)) {
        return false;
    }

    (*op_count_io)++;
    return true;
}

static bool export_security_files(const char *sys_path, const char *config_dir, const char *txn_dir, ExportOp *ops, int *op_count_io, int max_ops, char *err, size_t err_len)
{
    if (*op_count_io >= max_ops) {
        set_err(err, err_len, "Too many export files");
        return false;
    }

    char out_path[PATH_MAX];
    if (snprintf(out_path, sizeof(out_path), "%s/security/access_levels.toml", config_dir) >= (int)sizeof(out_path)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    ExportOp *op = &ops[*op_count_io];
    if (!begin_backup_into_txn(op, config_dir, txn_dir, out_path, *op_count_io, err, err_len)) {
        return false;
    }

    ExportCtx ctx = {
        .sys_path = sys_path,
        .config_dir = config_dir,
        .maxctl_path = NULL
    };

    char maxctl_path[PATH_MAX];
    struct stat mst;
    if (snprintf(maxctl_path, sizeof(maxctl_path), "%s/etc/max.ctl", sys_path) < (int)sizeof(maxctl_path) &&
        stat(maxctl_path, &mst) == 0 && S_ISREG(mst.st_mode)) {
        ctx.maxctl_path = maxctl_path;
    }

    if (!write_file_atomic_and_commit(op, write_security_access_levels_toml, &ctx, err, err_len)) {
        return false;
    }

    (*op_count_io)++;
    return true;
}

static bool export_general_files(const char *sys_path, const char *config_dir, const char *txn_dir, ExportOp *ops, int *op_count_io, int max_ops, char *err, size_t err_len)
{
    if (*op_count_io + 7 >= max_ops) {
        set_err(err, err_len, "Too many export files");
        return false;
    }

    ExportCtx ctx = {
        .sys_path = sys_path,
        .config_dir = config_dir,
        .maxctl_path = NULL
    };

    char maxctl_path[PATH_MAX];
    struct stat mst;
    if (snprintf(maxctl_path, sizeof(maxctl_path), "%s/etc/max.ctl", sys_path) < (int)sizeof(maxctl_path) &&
        stat(maxctl_path, &mst) == 0 && S_ISREG(mst.st_mode)) {
        ctx.maxctl_path = maxctl_path;
    }

    char out_path_session[PATH_MAX];
    if (snprintf(out_path_session, sizeof(out_path_session), "%s/general/session.toml", config_dir) >= (int)sizeof(out_path_session)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    ExportOp *op_session = &ops[*op_count_io];
    if (!begin_backup_into_txn(op_session, config_dir, txn_dir, out_path_session, *op_count_io, err, err_len)) {
        return false;
    }

    if (!write_file_atomic_and_commit(op_session, write_general_session_toml, &ctx, err, err_len)) {
        return false;
    }

    (*op_count_io)++;

    char out_path_display[PATH_MAX];
    if (snprintf(out_path_display, sizeof(out_path_display), "%s/general/display_files.toml", config_dir) >= (int)sizeof(out_path_display)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    ExportOp *op_display = &ops[*op_count_io];
    if (!begin_backup_into_txn(op_display, config_dir, txn_dir, out_path_display, *op_count_io, err, err_len)) {
        return false;
    }

    if (!write_file_atomic_and_commit(op_display, write_general_display_files_toml, &ctx, err, err_len)) {
        return false;
    }

    (*op_count_io)++;

    char out_path_colors[PATH_MAX];
    if (snprintf(out_path_colors, sizeof(out_path_colors), "%s/general/colors.toml", config_dir) >= (int)sizeof(out_path_colors)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    ExportOp *op_colors = &ops[*op_count_io];
    if (!begin_backup_into_txn(op_colors, config_dir, txn_dir, out_path_colors, *op_count_io, err, err_len)) {
        return false;
    }

    if (!write_file_atomic_and_commit(op_colors, write_general_colors_toml, &ctx, err, err_len)) {
        return false;
    }

    (*op_count_io)++;

    char out_path_reader[PATH_MAX];
    if (snprintf(out_path_reader, sizeof(out_path_reader), "%s/general/reader.toml", config_dir) >= (int)sizeof(out_path_reader)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    ExportOp *op_reader = &ops[*op_count_io];
    if (!begin_backup_into_txn(op_reader, config_dir, txn_dir, out_path_reader, *op_count_io, err, err_len)) {
        return false;
    }

    if (!write_file_atomic_and_commit(op_reader, write_general_reader_toml, &ctx, err, err_len)) {
        return false;
    }

    (*op_count_io)++;

    char out_path_equipment[PATH_MAX];
    if (snprintf(out_path_equipment, sizeof(out_path_equipment), "%s/general/equipment.toml", config_dir) >= (int)sizeof(out_path_equipment)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    ExportOp *op_equipment = &ops[*op_count_io];
    if (!begin_backup_into_txn(op_equipment, config_dir, txn_dir, out_path_equipment, *op_count_io, err, err_len)) {
        return false;
    }

    if (!write_file_atomic_and_commit(op_equipment, write_general_equipment_toml, &ctx, err, err_len)) {
        return false;
    }

    (*op_count_io)++;

    char out_path_protocol[PATH_MAX];
    if (snprintf(out_path_protocol, sizeof(out_path_protocol), "%s/general/protocol.toml", config_dir) >= (int)sizeof(out_path_protocol)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    ExportOp *op_protocol = &ops[*op_count_io];
    if (!begin_backup_into_txn(op_protocol, config_dir, txn_dir, out_path_protocol, *op_count_io, err, err_len)) {
        return false;
    }

    if (!write_file_atomic_and_commit(op_protocol, write_general_protocols_toml, &ctx, err, err_len)) {
        return false;
    }

    (*op_count_io)++;

    char out_path_language[PATH_MAX];
    if (snprintf(out_path_language, sizeof(out_path_language), "%s/general/language.toml", config_dir) >= (int)sizeof(out_path_language)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    ExportOp *op_language = &ops[*op_count_io];
    if (!begin_backup_into_txn(op_language, config_dir, txn_dir, out_path_language, *op_count_io, err, err_len)) {
        return false;
    }

    if (!write_file_atomic_and_commit(op_language, write_general_language_toml, &ctx, err, err_len)) {
        return false;
    }

    (*op_count_io)++;
    return true;
}

static bool export_matrix_file(const char *sys_path, const char *config_dir, const char *txn_dir, ExportOp *ops, int *op_count_io, int max_ops, char *err, size_t err_len)
{
    if (*op_count_io >= max_ops) {
        set_err(err, err_len, "Too many export files");
        return false;
    }

    char out_path[PATH_MAX];
    if (snprintf(out_path, sizeof(out_path), "%s/matrix.toml", config_dir) >= (int)sizeof(out_path)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    ExportOp *op = &ops[*op_count_io];
    if (!begin_backup_into_txn(op, config_dir, txn_dir, out_path, *op_count_io, err, err_len)) {
        return false;
    }

    ExportCtx ctx = {
        .sys_path = sys_path,
        .config_dir = config_dir,
        .maxctl_path = NULL
    };

    char maxctl_path[PATH_MAX];
    struct stat mst;
    if (snprintf(maxctl_path, sizeof(maxctl_path), "%s/etc/max.ctl", sys_path) < (int)sizeof(maxctl_path) &&
        stat(maxctl_path, &mst) == 0 && S_ISREG(mst.st_mode)) {
        ctx.maxctl_path = maxctl_path;
    }

    if (!write_file_atomic_and_commit(op, write_matrix_toml, &ctx, err, err_len)) {
        return false;
    }

    (*op_count_io)++;
    return true;
}

static bool export_areas_file(const char *sys_path, const char *config_dir, const char *txn_dir, bool is_msg, ExportOp *ops, int *op_count_io, int max_ops, char *err, size_t err_len)
{
    if (*op_count_io >= max_ops) {
        set_err(err, err_len, "Too many export files");
        return false;
    }

    const char *subdir = is_msg ? "areas/msg" : "areas/file";
    const char *fname = is_msg ? "areas.toml" : "areas.toml";

    char out_path[PATH_MAX];
    if (snprintf(out_path, sizeof(out_path), "%s/%s/%s", config_dir, subdir, fname) >= (int)sizeof(out_path)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    ExportOp *op = &ops[*op_count_io];
    if (!begin_backup_into_txn(op, config_dir, txn_dir, out_path, *op_count_io, err, err_len)) {
        return false;
    }

    AreasWriteCtx actx = {
        .sys_path = sys_path,
        .is_msg = is_msg
    };

    if (!write_file_atomic_and_commit(op, write_areas_toml, &actx, err, err_len)) {
        return false;
    }

    (*op_count_io)++;
    return true;
}

bool nextgen_export_config(const char *sys_path,
                           const char *config_dir,
                           unsigned int flags,
                           char *err,
                           size_t err_len)
{
    if (!sys_path || !sys_path[0] || !config_dir || !config_dir[0]) {
        set_err(err, err_len, "Invalid sys_path or config_dir");
        return false;
    }

    if (!mkdir_p(config_dir)) {
        set_err(err, err_len, "Failed to create %s", config_dir);
        return false;
    }

    char txn_dir[PATH_MAX];
    time_t now = time(NULL);

    if (snprintf(txn_dir, sizeof(txn_dir), "%s/.txn-%ld-%ld", config_dir, (long)getpid(), (long)now) >= (int)sizeof(txn_dir)) {
        set_err(err, err_len, "Path too long");
        return false;
    }

    if (!mkdir_p(txn_dir)) {
        set_err(err, err_len, "Failed to create %s", txn_dir);
        return false;
    }

    ExportOp ops[256];
    int op_count = 0;

    bool ok = true;

    if ((flags & NG_EXPORT_MENUS) != 0u) {
        ok = export_menus(sys_path, config_dir, txn_dir, ops, &op_count, (int)(sizeof(ops) / sizeof(ops[0])), err, err_len);
    }

    if (ok && (flags & NG_EXPORT_MSG_AREAS) != 0u) {
        ok = export_areas_file(sys_path, config_dir, txn_dir, true, ops, &op_count, (int)(sizeof(ops) / sizeof(ops[0])), err, err_len);
    }

    if (ok && (flags & NG_EXPORT_FILE_AREAS) != 0u) {
        ok = export_areas_file(sys_path, config_dir, txn_dir, false, ops, &op_count, (int)(sizeof(ops) / sizeof(ops[0])), err, err_len);
    }

    if (ok && (flags & NG_EXPORT_SYSTEM) != 0u) {
        ok = export_general_files(sys_path, config_dir, txn_dir, ops, &op_count, (int)(sizeof(ops) / sizeof(ops[0])), err, err_len);
    }

    if (ok && (flags & NG_EXPORT_SYSTEM) != 0u) {
        ok = export_matrix_file(sys_path, config_dir, txn_dir, ops, &op_count, (int)(sizeof(ops) / sizeof(ops[0])), err, err_len);
    }

    if (ok && (flags & NG_EXPORT_SYSTEM) != 0u) {
        ok = export_security_files(sys_path, config_dir, txn_dir, ops, &op_count, (int)(sizeof(ops) / sizeof(ops[0])), err, err_len);
    }

    if (ok && (flags & NG_EXPORT_SYSTEM) != 0u) {
        ok = export_system_commit_last(sys_path, config_dir, txn_dir, ops, &op_count, (int)(sizeof(ops) / sizeof(ops[0])), err, err_len);
    }

    if (!ok) {
        rollback_ops(ops, op_count);
        return false;
    }

    cleanup_ops_and_txn(ops, op_count, txn_dir);
    return true;
}

bool nextgen_export_config_from_maxctl(const char *maxctl_path,
                                       const char *config_dir,
                                       unsigned int flags,
                                       char *err,
                                       size_t err_len)
{
    if (maxctl_path == NULL || maxctl_path[0] == '\0') {
        set_err(err, err_len, "Invalid max.ctl path");
        return false;
    }
    if (config_dir == NULL || config_dir[0] == '\0') {
        set_err(err, err_len, "Invalid config_dir");
        return false;
    }

    char sys_path[PATH_MAX];
    if (!derive_sys_path_from_maxctl(maxctl_path, sys_path, sizeof(sys_path))) {
        set_err(err, err_len, "Failed to derive sys_path from %s", maxctl_path);
        return false;
    }

    return nextgen_export_config(sys_path, config_dir, flags, err, err_len);
}
