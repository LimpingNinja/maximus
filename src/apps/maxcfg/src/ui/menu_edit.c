#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "menu_edit.h"

static char *safe_strdup(const char *s)
{
    return s ? strdup(s) : NULL;
}

static bool token_has(const char *str, const char *token)
{
    if (!str || !*str || !token || !*token) return false;

    char *copy = strdup(str);
    if (!copy) return false;

    bool found = false;
    char *t = strtok(copy, " \t");
    while (t) {
        if (strcasecmp(t, token) == 0) {
            found = true;
            break;
        }
        t = strtok(NULL, " \t");
    }

    free(copy);
    return found;
}

static char *types_string_from_flags(word flags)
{
    char buf[64];
    buf[0] = '\0';

    if (flags == 0) return strdup("");

    if ((flags == MFLAG_MF_ALL) || (flags == MFLAG_HF_ALL)) {
        return strdup("");
    }

    bool first = true;
    if (flags & (word)0x0001 || flags & (word)0x0010) {
        strcat(buf, "Novice");
        first = false;
    }
    if (flags & (word)0x0002 || flags & (word)0x0020) {
        if (!first) strcat(buf, " ");
        strcat(buf, "Regular");
        first = false;
    }
    if (flags & (word)0x0004 || flags & (word)0x0040) {
        if (!first) strcat(buf, " ");
        strcat(buf, "Expert");
        first = false;
    }
    if (flags & (word)0x0400 || flags & (word)0x0800) {
        if (!first) strcat(buf, " ");
        strcat(buf, "RIP");
    }

    return strdup(buf);
}

static word types_flags_from_string(const char *str, bool is_header)
{
    word flags = 0;

    if (!str || !*str) {
        return is_header ? MFLAG_HF_ALL : MFLAG_MF_ALL;
    }

    if (token_has(str, "Novice")) flags |= is_header ? MFLAG_HF_NOVICE : MFLAG_MF_NOVICE;
    if (token_has(str, "Regular")) flags |= is_header ? MFLAG_HF_REGULAR : MFLAG_MF_REGULAR;
    if (token_has(str, "Expert")) flags |= is_header ? MFLAG_HF_EXPERT : MFLAG_MF_EXPERT;
    if (token_has(str, "RIP")) flags |= is_header ? MFLAG_HF_RIP : MFLAG_MF_RIP;

    if (flags == 0) {
        return is_header ? MFLAG_HF_ALL : MFLAG_MF_ALL;
    }

    return flags;
}

void menu_free_values(char **values, int count)
{
    if (!values) return;
    for (int i = 0; i < count; i++) {
        free(values[i]);
        values[i] = NULL;
    }
}

void menu_load_properties_form(MenuDefinition *menu, char **values)
{
    if (!menu || !values) return;

    values[0] = safe_strdup(menu->title ? menu->title : "");
    values[1] = safe_strdup(menu->header_file ? menu->header_file : "");
    values[2] = types_string_from_flags(menu->header_flags);
    values[3] = safe_strdup(menu->menu_file ? menu->menu_file : "");
    values[4] = types_string_from_flags(menu->menu_flags);

    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", menu->menu_length);
        values[5] = strdup(buf);
    }
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", menu->menu_color);
        values[6] = strdup(buf);
    }
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", menu->opt_width);
        values[7] = strdup(buf);
    }
}

bool menu_save_properties_form(MenuDefinition *menu, char **values)
{
    if (!menu || !values) return false;

    bool modified = false;

    const char *new_title = values[0] ? values[0] : "";
    const char *new_header_file = values[1] ? values[1] : "";
    const char *new_header_types = values[2] ? values[2] : "";
    const char *new_menu_file = values[3] ? values[3] : "";
    const char *new_menu_types = values[4] ? values[4] : "";

    int new_menu_length = values[5] ? atoi(values[5]) : 0;
    int new_menu_color = values[6] ? atoi(values[6]) : -1;
    int new_opt_width = values[7] ? atoi(values[7]) : 0;

    if (!menu->title || strcmp(menu->title, new_title) != 0) {
        free(menu->title);
        menu->title = strdup(new_title);
        modified = true;
    }

    if ((!menu->header_file && new_header_file[0]) || (menu->header_file && strcmp(menu->header_file, new_header_file) != 0)) {
        free(menu->header_file);
        menu->header_file = new_header_file[0] ? strdup(new_header_file) : NULL;
        modified = true;
    }

    word hf = types_flags_from_string(new_header_types, true);
    if (menu->header_flags != hf) {
        menu->header_flags = hf;
        modified = true;
    }

    if ((!menu->menu_file && new_menu_file[0]) || (menu->menu_file && strcmp(menu->menu_file, new_menu_file) != 0)) {
        free(menu->menu_file);
        menu->menu_file = new_menu_file[0] ? strdup(new_menu_file) : NULL;
        modified = true;
    }

    word mf = types_flags_from_string(new_menu_types, false);
    if (menu->menu_flags != mf) {
        menu->menu_flags = mf;
        modified = true;
    }

    if (menu->menu_length != new_menu_length) {
        menu->menu_length = new_menu_length;
        modified = true;
    }

    if (menu->menu_color != new_menu_color) {
        menu->menu_color = new_menu_color;
        modified = true;
    }

    if (menu->opt_width != new_opt_width) {
        menu->opt_width = new_opt_width;
        modified = true;
    }

    return modified;
}

static char *modifier_string_from_flags(word flags, byte areatype)
{
    char buf[128];
    buf[0] = '\0';

    if (flags & OFLAG_NODSP) strcat(buf, "NoDsp ");
    if (flags & OFLAG_CTL) strcat(buf, "Ctl ");
    if (flags & OFLAG_NOCLS) strcat(buf, "NoCLS ");
    if (flags & OFLAG_RIP) strcat(buf, "RIP ");
    if (flags & OFLAG_NORIP) strcat(buf, "NoRIP ");
    if (flags & OFLAG_THEN) strcat(buf, "Then ");
    if (flags & OFLAG_ELSE) strcat(buf, "Else ");
    if (flags & OFLAG_STAY) strcat(buf, "Stay ");
    if (flags & OFLAG_ULOCAL) strcat(buf, "UsrLocal ");
    if (flags & OFLAG_UREMOTE) strcat(buf, "UsrRemote ");
    if (flags & OFLAG_REREAD) strcat(buf, "ReRead ");
    if (areatype & ATYPE_LOCAL) strcat(buf, "Local ");
    if (areatype & ATYPE_MATRIX) strcat(buf, "Matrix ");
    if (areatype & ATYPE_ECHO) strcat(buf, "Echo ");
    if (areatype & ATYPE_CONF) strcat(buf, "Conf ");

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == ' ') buf[len - 1] = '\0';

    return strdup(buf);
}

static void modifier_flags_from_string(const char *str, word *flags, byte *areatype)
{
    *flags = 0;
    *areatype = ATYPE_NONE;

    if (!str || !*str) return;

    if (token_has(str, "NoDsp")) *flags |= OFLAG_NODSP;
    if (token_has(str, "Ctl")) *flags |= OFLAG_CTL;
    if (token_has(str, "NoCLS")) *flags |= OFLAG_NOCLS;
    if (token_has(str, "RIP")) *flags |= OFLAG_RIP;
    if (token_has(str, "NoRIP")) *flags |= OFLAG_NORIP;
    if (token_has(str, "Then")) *flags |= OFLAG_THEN;
    if (token_has(str, "Else")) *flags |= OFLAG_ELSE;
    if (token_has(str, "Stay")) *flags |= OFLAG_STAY;
    if (token_has(str, "UsrLocal")) *flags |= OFLAG_ULOCAL;
    if (token_has(str, "UsrRemote")) *flags |= OFLAG_UREMOTE;
    if (token_has(str, "ReRead")) *flags |= OFLAG_REREAD;
    if (token_has(str, "Local")) *areatype |= ATYPE_LOCAL;
    if (token_has(str, "Matrix")) *areatype |= ATYPE_MATRIX;
    if (token_has(str, "Echo")) *areatype |= ATYPE_ECHO;
    if (token_has(str, "Conf")) *areatype |= ATYPE_CONF;
}

void menu_load_option_form(MenuOption *opt, char **values)
{
    if (!opt || !values) return;

    values[0] = safe_strdup(opt->command ? opt->command : "");
    values[1] = safe_strdup(opt->arguments ? opt->arguments : "");
    values[2] = safe_strdup(opt->priv_level ? opt->priv_level : "Demoted");
    values[3] = safe_strdup(opt->description ? opt->description : "");
    values[4] = modifier_string_from_flags(opt->flags, opt->areatype);
    values[5] = safe_strdup(opt->key_poke ? opt->key_poke : "");
}

bool menu_save_option_form(MenuOption *opt, char **values)
{
    if (!opt || !values) return false;

    bool modified = false;

    const char *new_cmd = values[0] ? values[0] : "";
    const char *new_arg = values[1] ? values[1] : "";
    const char *new_priv = values[2] ? values[2] : "";
    const char *new_desc = values[3] ? values[3] : "";
    const char *new_modifier = values[4] ? values[4] : "";
    const char *new_keypoke = values[5] ? values[5] : "";

    if (!opt->command || strcmp(opt->command, new_cmd) != 0) {
        free(opt->command);
        opt->command = new_cmd[0] ? strdup(new_cmd) : NULL;
        modified = true;
    }

    if ((!opt->arguments && new_arg[0]) || (opt->arguments && strcmp(opt->arguments, new_arg) != 0)) {
        free(opt->arguments);
        opt->arguments = new_arg[0] ? strdup(new_arg) : NULL;
        modified = true;
    }

    if ((!opt->priv_level && new_priv[0]) || (opt->priv_level && strcmp(opt->priv_level, new_priv) != 0)) {
        free(opt->priv_level);
        opt->priv_level = new_priv[0] ? strdup(new_priv) : NULL;
        modified = true;
    }

    if (!opt->description || strcmp(opt->description, new_desc) != 0) {
        free(opt->description);
        opt->description = strdup(new_desc);
        modified = true;
    }

    word new_flags;
    byte new_areatype;
    modifier_flags_from_string(new_modifier, &new_flags, &new_areatype);
    if (opt->flags != new_flags || opt->areatype != new_areatype) {
        opt->flags = new_flags;
        opt->areatype = new_areatype;
        modified = true;
    }

    if ((!opt->key_poke && new_keypoke[0]) || (opt->key_poke && strcmp(opt->key_poke, new_keypoke) != 0)) {
        free(opt->key_poke);
        opt->key_poke = new_keypoke[0] ? strdup(new_keypoke) : NULL;
        modified = true;
    }

    return modified;
}
