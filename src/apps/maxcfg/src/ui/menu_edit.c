#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdio.h>
#include "menu_edit.h"

static char *safe_strdup(const char *s)
{
    return s ? strdup(s) : NULL;
}

static int bool_from_yesno(const char *s)
{
    if (s == NULL) {
        return 0;
    }
    return (strcasecmp(s, "Yes") == 0) ? 1 : 0;
}

static const char *yesno_from_bool(bool v)
{
    return v ? "Yes" : "No";
}

static int option_justify_from_string(const char *s)
{
    if (s == NULL || *s == '\0') {
        return 0;
    }
    if (strcasecmp(s, "Center") == 0) return 1;
    if (strcasecmp(s, "Right") == 0) return 2;
    return 0;
}

static const char *option_justify_to_string(int v)
{
    if (v == 1) return "Center";
    if (v == 2) return "Right";
    return "Left";
}

static int boundary_layout_from_string(const char *s)
{
    if (s == NULL || *s == '\0') {
        return 0;
    }
    if (strcasecmp(s, "Tight") == 0) return 1;
    if (strcasecmp(s, "Spread") == 0) return 2;
    if (strcasecmp(s, "Spread Width") == 0) return 3;
    if (strcasecmp(s, "Spread Height") == 0) return 4;
    return 0;
}

static const char *boundary_layout_to_string(int v)
{
    if (v == 1) return "Tight";
    if (v == 2) return "Spread";
    if (v == 3) return "Spread Width";
    if (v == 4) return "Spread Height";
    return "Grid";
}

static const char *boundary_justify_to_string(int hj, int vj)
{
    const char *h = (hj == 2) ? "Right" : (hj == 1) ? "Center" : "Left";
    const char *v = (vj == 2) ? "Bottom" : (vj == 1) ? "Center" : "Top";
    if (strcasecmp(h, "Left") == 0 && strcasecmp(v, "Top") == 0) return "Left Top";
    if (strcasecmp(h, "Left") == 0 && strcasecmp(v, "Center") == 0) return "Left Center";
    if (strcasecmp(h, "Left") == 0 && strcasecmp(v, "Bottom") == 0) return "Left Bottom";
    if (strcasecmp(h, "Center") == 0 && strcasecmp(v, "Top") == 0) return "Center Top";
    if (strcasecmp(h, "Center") == 0 && strcasecmp(v, "Center") == 0) return "Center Center";
    if (strcasecmp(h, "Center") == 0 && strcasecmp(v, "Bottom") == 0) return "Center Bottom";
    if (strcasecmp(h, "Right") == 0 && strcasecmp(v, "Top") == 0) return "Right Top";
    if (strcasecmp(h, "Right") == 0 && strcasecmp(v, "Center") == 0) return "Right Center";
    if (strcasecmp(h, "Right") == 0 && strcasecmp(v, "Bottom") == 0) return "Right Bottom";
    return "Left Top";
}

static void boundary_justify_from_string(const char *s, int *hj, int *vj)
{
    if (hj) *hj = 0;
    if (vj) *vj = 0;
    if (s == NULL || *s == '\0') {
        return;
    }

    char *copy = strdup(s);
    if (copy == NULL) {
        return;
    }

    int out_h = 0;
    int out_v = 0;

    char *h = strtok(copy, " \t");
    char *v = strtok(NULL, " \t");
    if (h) {
        if (strcasecmp(h, "right") == 0) out_h = 2;
        else if (strcasecmp(h, "center") == 0) out_h = 1;
        else out_h = 0;
    }
    if (v) {
        if (strcasecmp(v, "bottom") == 0) out_v = 2;
        else if (strcasecmp(v, "center") == 0) out_v = 1;
        else out_v = 0;
    }

    free(copy);
    if (hj) *hj = out_h;
    if (vj) *vj = out_v;
}

static bool is_default_value(const char *s)
{
    if (s == NULL) {
        return true;
    }
    if (*s == '\0') {
        return true;
    }
    return (strcasecmp(s, "(default)") == 0);
}

static void set_owned_string(char **dst, const char *src)
{
    if (dst == NULL) {
        return;
    }
    free(*dst);
    *dst = NULL;
    if (src && *src) {
        *dst = strdup(src);
    }
}

/* DOS color name to value (0-15) */
static int color_name_to_value(const char *name)
{
    if (!name || !*name) return -1;
    if (strcasecmp(name, "Black") == 0) return 0;
    if (strcasecmp(name, "Blue") == 0) return 1;
    if (strcasecmp(name, "Green") == 0) return 2;
    if (strcasecmp(name, "Cyan") == 0) return 3;
    if (strcasecmp(name, "Red") == 0) return 4;
    if (strcasecmp(name, "Magenta") == 0) return 5;
    if (strcasecmp(name, "Brown") == 0) return 6;
    if (strcasecmp(name, "Gray") == 0 || strcasecmp(name, "Grey") == 0) return 7;
    if (strcasecmp(name, "DarkGray") == 0 || strcasecmp(name, "DarkGrey") == 0) return 8;
    if (strcasecmp(name, "LightBlue") == 0) return 9;
    if (strcasecmp(name, "LightGreen") == 0) return 10;
    if (strcasecmp(name, "LightCyan") == 0) return 11;
    if (strcasecmp(name, "LightRed") == 0) return 12;
    if (strcasecmp(name, "LightMagenta") == 0) return 13;
    if (strcasecmp(name, "Yellow") == 0) return 14;
    if (strcasecmp(name, "White") == 0) return 15;
    return -1;
}

/* DOS color value (0-15) to name */
static const char *color_value_to_name(int val)
{
    static const char *names[] = {
        "Black", "Blue", "Green", "Cyan", "Red", "Magenta", "Brown", "Gray",
        "DarkGray", "LightBlue", "LightGreen", "LightCyan", "LightRed", "LightMagenta", "Yellow", "White"
    };
    if (val >= 0 && val < 16) return names[val];
    return "Gray";
}

/* Format a human-readable color pair string like "Gray on Black" */
static char *format_color_pair(const char *fg, const char *bg)
{
    char buf[64];
    if (!fg && !bg) {
        snprintf(buf, sizeof(buf), "(default)");
    } else if (!fg) {
        snprintf(buf, sizeof(buf), "(default FG) on %s", bg);
    } else if (!bg) {
        snprintf(buf, sizeof(buf), "%s on (default BG)", fg);
    } else {
        snprintf(buf, sizeof(buf), "%s on %s", fg, bg);
    }
    return strdup(buf);
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

/* Menu type context for flags conversion helpers. */
#define MENU_TYPE_HEADER 0
#define MENU_TYPE_FOOTER 1
#define MENU_TYPE_BODY   2

static char *types_string_from_flags(word flags, int kind)
{
    char buf[64];
    buf[0] = '\0';

    if (flags == 0) return strdup("");

    if ((kind == MENU_TYPE_HEADER && flags == MFLAG_HF_ALL) ||
        (kind == MENU_TYPE_FOOTER && flags == MFLAG_FF_ALL) ||
        (kind == MENU_TYPE_BODY && flags == MFLAG_MF_ALL)) {
        return strdup("");
    }

    bool first = true;
    if (flags & (word)((kind == MENU_TYPE_HEADER) ? MFLAG_HF_NOVICE :
                       (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_NOVICE : MFLAG_MF_NOVICE)) {
        strcat(buf, "Novice");
        first = false;
    }
    if (flags & (word)((kind == MENU_TYPE_HEADER) ? MFLAG_HF_REGULAR :
                       (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_REGULAR : MFLAG_MF_REGULAR)) {
        if (!first) strcat(buf, " ");
        strcat(buf, "Regular");
        first = false;
    }
    if (flags & (word)((kind == MENU_TYPE_HEADER) ? MFLAG_HF_EXPERT :
                       (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_EXPERT : MFLAG_MF_EXPERT)) {
        if (!first) strcat(buf, " ");
        strcat(buf, "Expert");
        first = false;
    }
    if (kind != MENU_TYPE_FOOTER &&
        (flags & (word)((kind == MENU_TYPE_HEADER) ? MFLAG_HF_RIP : MFLAG_MF_RIP))) {
        if (!first) strcat(buf, " ");
        strcat(buf, "RIP");
    }

    return strdup(buf);
}

static word types_flags_from_string(const char *str, int kind)
{
    word flags = 0;

    if (!str || !*str) {
        return (kind == MENU_TYPE_HEADER) ? MFLAG_HF_ALL :
               (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_ALL : MFLAG_MF_ALL;
    }

    if (token_has(str, "Novice")) flags |= (kind == MENU_TYPE_HEADER) ? MFLAG_HF_NOVICE :
                                              (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_NOVICE : MFLAG_MF_NOVICE;
    if (token_has(str, "Regular")) flags |= (kind == MENU_TYPE_HEADER) ? MFLAG_HF_REGULAR :
                                               (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_REGULAR : MFLAG_MF_REGULAR;
    if (token_has(str, "Expert")) flags |= (kind == MENU_TYPE_HEADER) ? MFLAG_HF_EXPERT :
                                              (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_EXPERT : MFLAG_MF_EXPERT;
    if (kind != MENU_TYPE_FOOTER && token_has(str, "RIP"))
        flags |= (kind == MENU_TYPE_HEADER) ? MFLAG_HF_RIP : MFLAG_MF_RIP;

    if (flags == 0) {
        return (kind == MENU_TYPE_HEADER) ? MFLAG_HF_ALL :
               (kind == MENU_TYPE_FOOTER) ? MFLAG_FF_ALL : MFLAG_MF_ALL;
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
    values[2] = types_string_from_flags(menu->header_flags, MENU_TYPE_HEADER);
    values[3] = safe_strdup(menu->footer_file ? menu->footer_file : "");
    values[4] = types_string_from_flags(menu->footer_flags, MENU_TYPE_FOOTER);
    values[5] = safe_strdup(menu->menu_file ? menu->menu_file : "");
    values[6] = types_string_from_flags(menu->menu_flags, MENU_TYPE_BODY);

    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", menu->menu_length);
        values[7] = strdup(buf);
    }
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", menu->menu_color);
        values[8] = strdup(buf);
    }
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", menu->opt_width);
        values[9] = strdup(buf);
    }
}

bool menu_save_properties_form(MenuDefinition *menu, char **values)
{
    if (!menu || !values) return false;

    bool modified = false;

    const char *new_title = values[0] ? values[0] : "";
    const char *new_header_file = values[1] ? values[1] : "";
    const char *new_header_types = values[2] ? values[2] : "";
    const char *new_footer_file = values[3] ? values[3] : "";
    const char *new_footer_types = values[4] ? values[4] : "";
    const char *new_menu_file = values[5] ? values[5] : "";
    const char *new_menu_types = values[6] ? values[6] : "";

    int new_menu_length = values[7] ? atoi(values[7]) : 0;
    int new_menu_color = values[8] ? atoi(values[8]) : -1;
    int new_opt_width = values[9] ? atoi(values[9]) : 0;

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

    word hf = types_flags_from_string(new_header_types, MENU_TYPE_HEADER);
    if (menu->header_flags != hf) {
        menu->header_flags = hf;
        modified = true;
    }

    if ((!menu->footer_file && new_footer_file[0]) || (menu->footer_file && strcmp(menu->footer_file, new_footer_file) != 0)) {
        free(menu->footer_file);
        menu->footer_file = new_footer_file[0] ? strdup(new_footer_file) : NULL;
        modified = true;
    }

    word ff = types_flags_from_string(new_footer_types, MENU_TYPE_FOOTER);
    if (menu->footer_flags != ff) {
        menu->footer_flags = ff;
        modified = true;
    }

    if ((!menu->menu_file && new_menu_file[0]) || (menu->menu_file && strcmp(menu->menu_file, new_menu_file) != 0)) {
        free(menu->menu_file);
        menu->menu_file = new_menu_file[0] ? strdup(new_menu_file) : NULL;
        modified = true;
    }

    word mf = types_flags_from_string(new_menu_types, MENU_TYPE_BODY);
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

void menu_load_customization_form(MenuDefinition *menu, char **values)
{
    if (!menu || !values) return;

    values[0] = safe_strdup(yesno_from_bool(menu->cm_enabled));
    values[1] = safe_strdup(yesno_from_bool(menu->cm_skip_canned));
    values[2] = safe_strdup(yesno_from_bool(menu->cm_show_title));
    values[3] = safe_strdup(yesno_from_bool(menu->cm_lightbar));

    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", menu->cm_lightbar_margin);
        values[4] = strdup(buf);
    }

    /* values[5] is a separator */

    values[6] = format_color_pair(menu->cm_lb_normal_fg, menu->cm_lb_normal_bg);
    values[7] = format_color_pair(menu->cm_lb_selected_fg, menu->cm_lb_selected_bg);
    values[8] = format_color_pair(menu->cm_lb_high_fg, menu->cm_lb_high_bg);
    values[9] = format_color_pair(menu->cm_lb_high_sel_fg, menu->cm_lb_high_sel_bg);

    /* values[10] is a separator */

    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", menu->cm_top_row);
        values[11] = strdup(buf);
        snprintf(buf, sizeof(buf), "%d", menu->cm_top_col);
        values[12] = strdup(buf);
        snprintf(buf, sizeof(buf), "%d", menu->cm_bottom_row);
        values[13] = strdup(buf);
        snprintf(buf, sizeof(buf), "%d", menu->cm_bottom_col);
        values[14] = strdup(buf);

        snprintf(buf, sizeof(buf), "%d", menu->cm_title_row);
        values[15] = strdup(buf);
        snprintf(buf, sizeof(buf), "%d", menu->cm_title_col);
        values[16] = strdup(buf);
        snprintf(buf, sizeof(buf), "%d", menu->cm_prompt_row);
        values[17] = strdup(buf);
        snprintf(buf, sizeof(buf), "%d", menu->cm_prompt_col);
        values[18] = strdup(buf);
    }

    /* values[19] is a separator */

    values[20] = safe_strdup(yesno_from_bool(menu->cm_option_spacing));
    values[21] = safe_strdup(option_justify_to_string(menu->cm_option_justify));
    values[22] = safe_strdup(boundary_justify_to_string(menu->cm_boundary_justify, menu->cm_boundary_vjustify));
    values[23] = safe_strdup(boundary_layout_to_string(menu->cm_boundary_layout));
}

bool menu_save_customization_form(MenuDefinition *menu, char **values)
{
    if (!menu || !values) return false;

    bool modified = false;

    bool new_enabled = bool_from_yesno(values[0]) ? true : false;
    bool new_skip = bool_from_yesno(values[1]) ? true : false;
    bool new_show_title = bool_from_yesno(values[2]) ? true : false;
    bool new_lightbar = bool_from_yesno(values[3]) ? true : false;
    int new_margin = values[4] ? atoi(values[4]) : 0;

    if (menu->cm_enabled != new_enabled) { menu->cm_enabled = new_enabled; modified = true; }
    if (menu->cm_skip_canned != new_skip) { menu->cm_skip_canned = new_skip; modified = true; }
    if (menu->cm_show_title != new_show_title) { menu->cm_show_title = new_show_title; modified = true; }
    if (menu->cm_lightbar != new_lightbar) { menu->cm_lightbar = new_lightbar; modified = true; }
    if (menu->cm_lightbar_margin != new_margin) { menu->cm_lightbar_margin = new_margin; modified = true; }

    /* Color fields are stored internally as "<internal fg>|<internal bg>" by the color picker action */
    /* We just need to extract and update the MenuDefinition strings */
    {
        /* Normal colors */
        const char *normal_val = values[6];
        /* Selected colors */
        const char *selected_val = values[7];
        /* High colors */
        const char *high_val = values[8];
        /* High+Selected colors */
        const char *high_sel_val = values[9];

        /* Parse and update if changed (values are already in the right format from the picker) */
        /* The picker will set values[6-9] to internal format like "Gray|Black" */
        /* We need to split and compare */
        
        /* For now, just mark as modified if the string representation changed */
        /* The actual parsing will be done by the color picker action */
        (void)normal_val;
        (void)selected_val;
        (void)high_val;
        (void)high_sel_val;
    }

    {
        int tr = values[11] ? atoi(values[11]) : 0;
        int tc = values[12] ? atoi(values[12]) : 0;
        int br = values[13] ? atoi(values[13]) : 0;
        int bc = values[14] ? atoi(values[14]) : 0;
        int ttr = values[15] ? atoi(values[15]) : 0;
        int ttc = values[16] ? atoi(values[16]) : 0;
        int pr = values[17] ? atoi(values[17]) : 0;
        int pc = values[18] ? atoi(values[18]) : 0;

        if (menu->cm_top_row != tr) { menu->cm_top_row = tr; modified = true; }
        if (menu->cm_top_col != tc) { menu->cm_top_col = tc; modified = true; }
        if (menu->cm_bottom_row != br) { menu->cm_bottom_row = br; modified = true; }
        if (menu->cm_bottom_col != bc) { menu->cm_bottom_col = bc; modified = true; }
        if (menu->cm_title_row != ttr) { menu->cm_title_row = ttr; modified = true; }
        if (menu->cm_title_col != ttc) { menu->cm_title_col = ttc; modified = true; }
        if (menu->cm_prompt_row != pr) { menu->cm_prompt_row = pr; modified = true; }
        if (menu->cm_prompt_col != pc) { menu->cm_prompt_col = pc; modified = true; }
    }

    {
        bool sp = bool_from_yesno(values[20]) ? true : false;
        int oj = option_justify_from_string(values[21]);
        int hj = 0;
        int vj = 0;
        boundary_justify_from_string(values[22], &hj, &vj);
        int bl = boundary_layout_from_string(values[23]);

        if (menu->cm_option_spacing != sp) { menu->cm_option_spacing = sp; modified = true; }
        if (menu->cm_option_justify != oj) { menu->cm_option_justify = oj; modified = true; }
        if (menu->cm_boundary_justify != hj) { menu->cm_boundary_justify = hj; modified = true; }
        if (menu->cm_boundary_vjustify != vj) { menu->cm_boundary_vjustify = vj; modified = true; }
        if (menu->cm_boundary_layout != bl) { menu->cm_boundary_layout = bl; modified = true; }
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
