#include "libmaxcfg.h"

#include <string.h>

MaxCfgStatus maxcfg_join_path(const MaxCfg *cfg,
                             const char *relative_path,
                             char *out_path,
                             size_t out_path_size)
{
    const char *base;
    size_t base_len;
    size_t rel_len;
    int need_slash;
    size_t total_len;

    if (cfg == NULL || relative_path == NULL || out_path == NULL || out_path_size == 0) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    base = maxcfg_base_dir(cfg);
    if (base == NULL) {
        return MAXCFG_ERR_INVALID_ARGUMENT;
    }

    base_len = strlen(base);
    rel_len = strlen(relative_path);

    need_slash = (base_len > 0 && base[base_len - 1] != '/');

    total_len = base_len + (need_slash ? 1u : 0u) + rel_len;
    if (total_len + 1u > out_path_size) {
        return MAXCFG_ERR_PATH_TOO_LONG;
    }

    memcpy(out_path, base, base_len);

    if (need_slash) {
        out_path[base_len] = '/';
        memcpy(out_path + base_len + 1u, relative_path, rel_len);
        out_path[base_len + 1u + rel_len] = '\0';
    } else {
        memcpy(out_path + base_len, relative_path, rel_len);
        out_path[base_len + rel_len] = '\0';
    }

    return MAXCFG_OK;
}
