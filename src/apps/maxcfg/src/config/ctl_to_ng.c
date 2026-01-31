/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ctl_to_ng.c - CTL file to MaxCfgNg* struct population (zero PRM dependency)
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "ctl_to_ng.h"
#include "libmaxcfg.h"

/* Helper to trim whitespace */
static char *trim_ws(char *s)
{
    if (!s) return NULL;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

/* Helper to check if line starts with keyword (case-insensitive) */
static bool line_starts_with_keyword(const char *line, const char *keyword)
{
    size_t kw_len = strlen(keyword);
    while (*line && isspace((unsigned char)*line)) line++;
    if (strncasecmp(line, keyword, kw_len) != 0) return false;
    const char *after = line + kw_len;
    return (*after == '\0' || isspace((unsigned char)*after));
}

/* Helper to extract value after keyword */
static char *extract_value_after_keyword(char *line, const char *keyword)
{
    size_t kw_len = strlen(keyword);
    while (*line && isspace((unsigned char)*line)) line++;
    line += kw_len;
    while (*line && isspace((unsigned char)*line)) line++;
    return trim_ws(line);
}

/* Parse privilege level from access level name using access.ctl */
static int parse_priv_level(const char *sys_path, const char *level_name)
{
    if (!sys_path || !level_name) return 0;
    
    char access_ctl[512];
    snprintf(access_ctl, sizeof(access_ctl), "%s/etc/access.ctl", sys_path);
    
    FILE *fp = fopen(access_ctl, "r");
    if (!fp) return 0;
    
    char line[1024];
    bool in_access = false;
    bool found_name = false;
    int level = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        char *s = trim_ws(line);
        if (!s || s[0] == '\0' || s[0] == '%' || s[0] == ';') continue;
        
        if (line_starts_with_keyword(s, "Access")) {
            char *v = extract_value_after_keyword(s, "Access");
            if (v && strcasecmp(v, level_name) == 0) {
                in_access = true;
                found_name = true;
            } else {
                in_access = false;
            }
        } else if (in_access && line_starts_with_keyword(s, "Level")) {
            char *v = extract_value_after_keyword(s, "Level");
            if (v) {
                level = atoi(v);
                break;
            }
        } else if (line_starts_with_keyword(s, "End Access")) {
            if (found_name) break;
            in_access = false;
        }
    }
    
    fclose(fp);
    return level;
}

/* Parse a keyword value from a CTL file */
bool ctl_to_ng_parse_keyword(const char *ctl_path, const char *keyword, char *value_buf, size_t value_sz)
{
    if (!ctl_path || !keyword || !value_buf || value_sz == 0) return false;

    FILE *fp = fopen(ctl_path, "r");
    if (!fp) return false;

    char line[1024];
    bool found = false;

    while (fgets(line, sizeof(line), fp)) {
        char *s = trim_ws(line);
        if (!s || s[0] == '\0' || s[0] == '%' || s[0] == ';') continue;
        
        if (line_starts_with_keyword(s, keyword)) {
            char *v = extract_value_after_keyword(s, keyword);
            strncpy(value_buf, v ? v : "", value_sz - 1);
            value_buf[value_sz - 1] = '\0';
            found = true;
            break;
        }
    }

    fclose(fp);
    return found;
}

/* Parse a boolean keyword from a CTL file */
bool ctl_to_ng_parse_boolean(const char *ctl_path, const char *keyword, bool *value)
{
    if (!ctl_path || !keyword || !value) return false;
    *value = false;

    FILE *fp = fopen(ctl_path, "r");
    if (!fp) return false;

    char line[1024];
    bool found = false;
    char neg[256];
    snprintf(neg, sizeof(neg), "No %s", keyword);

    while (fgets(line, sizeof(line), fp)) {
        char *s = trim_ws(line);
        if (!s || s[0] == '\0' || s[0] == '%' || s[0] == ';') continue;

        if (line_starts_with_keyword(s, keyword)) {
            *value = true;
            found = true;
            break;
        }
        if (line_starts_with_keyword(s, neg)) {
            *value = false;
            found = true;
            break;
        }
    }

    fclose(fp);
    return found;
}

/* Parse an integer keyword from a CTL file */
bool ctl_to_ng_parse_int(const char *ctl_path, const char *keyword, int *value)
{
    char buf[64];
    if (!ctl_to_ng_parse_keyword(ctl_path, keyword, buf, sizeof(buf))) return false;
    *value = atoi(buf);
    return true;
}

/* Helper to duplicate a string, returning NULL for empty strings */
static char *dup_str_or_null(const char *s)
{
    if (!s || s[0] == '\0') return NULL;
    return strdup(s);
}

/* Populate MaxCfgNgSystem from max.ctl */
bool ctl_to_ng_populate_system(const char *maxctl_path, const char *sys_path, const char *config_dir, MaxCfgNgSystem *sys)
{
    if (!maxctl_path || !sys) return false;

    char buf[512];

    /* Basic system info */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Name", buf, sizeof(buf))) {
        sys->system_name = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "SysOp", buf, sizeof(buf))) {
        sys->sysop = dup_str_or_null(buf);
    }

    /* Task number */
    ctl_to_ng_parse_int(maxctl_path, "Task", &sys->task_num);

    /* Video mode */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Video", buf, sizeof(buf))) {
        sys->video = dup_str_or_null(buf);
    }

    /* Multitasker */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Multitasker", buf, sizeof(buf))) {
        sys->multitasker = dup_str_or_null(buf);
    }

    /* Paths */
    sys->sys_path = sys_path ? strdup(sys_path) : NULL;
    sys->config_path = config_dir ? strdup(config_dir) : NULL;

    if (ctl_to_ng_parse_keyword(maxctl_path, "Path Misc", buf, sizeof(buf))) {
        sys->misc_path = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Path Language", buf, sizeof(buf))) {
        sys->lang_path = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Path Temp", buf, sizeof(buf))) {
        sys->temp_path = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Path NetInfo", buf, sizeof(buf))) {
        sys->net_info_path = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Path IPC", buf, sizeof(buf))) {
        sys->ipc_path = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Path Inbound", buf, sizeof(buf))) {
        sys->inbound_path = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Path Menu", buf, sizeof(buf))) {
        sys->menu_path = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Path RIP", buf, sizeof(buf))) {
        sys->rip_path = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Path Stage", buf, sizeof(buf))) {
        sys->stage_path = dup_str_or_null(buf);
    }

    /* Log settings */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Log File", buf, sizeof(buf))) {
        sys->log_file = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Log Mode", buf, sizeof(buf))) {
        sys->log_mode = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Callers", buf, sizeof(buf))) {
        sys->file_callers = dup_str_or_null(buf);
    }

    /* File paths */
    if (ctl_to_ng_parse_keyword(maxctl_path, "File Password", buf, sizeof(buf))) {
        sys->file_password = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "File Access", buf, sizeof(buf))) {
        sys->file_access = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Menu Path", buf, sizeof(buf))) {
        sys->menu_path = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "RIP Path", buf, sizeof(buf))) {
        sys->rip_path = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Stage Path", buf, sizeof(buf))) {
        sys->stage_path = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "File Callers", buf, sizeof(buf))) {
        sys->file_callers = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Protocol CTL", buf, sizeof(buf))) {
        sys->protocol_ctl = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "MessageData", buf, sizeof(buf))) {
        sys->message_data = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "FileData", buf, sizeof(buf))) {
        sys->file_data = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "MCP Pipe", buf, sizeof(buf))) {
        sys->mcp_pipe = dup_str_or_null(buf);
    }

    /* MCP sessions */
    ctl_to_ng_parse_int(maxctl_path, "MCP Sessions", &sys->mcp_sessions);

    /* Boolean flags */
    ctl_to_ng_parse_boolean(maxctl_path, "Snoop", &sys->snoop);
    ctl_to_ng_parse_boolean(maxctl_path, "No Password", &sys->no_password_encryption);
    ctl_to_ng_parse_boolean(maxctl_path, "No Share", &sys->no_share);
    ctl_to_ng_parse_boolean(maxctl_path, "Reboot", &sys->reboot);
    ctl_to_ng_parse_boolean(maxctl_path, "Swap", &sys->swap);
    ctl_to_ng_parse_boolean(maxctl_path, "DOS Close", &sys->dos_close);
    ctl_to_ng_parse_boolean(maxctl_path, "Local Input", &sys->local_input_timeout);
    ctl_to_ng_parse_boolean(maxctl_path, "StatusLine", &sys->status_line);
    ctl_to_ng_parse_boolean(maxctl_path, "Has Snow", &sys->has_snow);

    return true;
}

/* Populate MaxCfgNgGeneralSession from max.ctl */
bool ctl_to_ng_populate_session(const char *maxctl_path, MaxCfgNgGeneralSession *session)
{
    if (!maxctl_path || !session) return false;

    char buf[512];

    /* Boolean flags */
    ctl_to_ng_parse_boolean(maxctl_path, "Alias System", &session->alias_system);
    ctl_to_ng_parse_boolean(maxctl_path, "Ask Alias", &session->ask_alias);
    ctl_to_ng_parse_boolean(maxctl_path, "Single Word Names", &session->single_word_names);
    ctl_to_ng_parse_boolean(maxctl_path, "Check ANSI", &session->check_ansi);
    ctl_to_ng_parse_boolean(maxctl_path, "Check RIP", &session->check_rip);
    ctl_to_ng_parse_boolean(maxctl_path, "Ask Phone", &session->ask_phone);
    ctl_to_ng_parse_boolean(maxctl_path, "No Real Name", &session->no_real_name);
    ctl_to_ng_parse_boolean(maxctl_path, "Disable Userlist", &session->disable_userlist);
    ctl_to_ng_parse_boolean(maxctl_path, "Disable Magnet", &session->disable_magnet);
    /* File Date: parse both autodate and date_style from "File Date Automatic mm-dd-yy" */
    if (ctl_to_ng_parse_keyword(maxctl_path, "File Date", buf, sizeof(buf))) {
        /* Check if Automatic or Manual */
        session->autodate = (strncasecmp(buf, "Automatic", 9) == 0);
        
        /* Parse the format string after the type */
        char *format = buf;
        while (*format && !isspace((unsigned char)*format)) format++;
        while (*format && isspace((unsigned char)*format)) format++;
        
        /* Map format string to date_style integer */
        if (strcasecmp(format, "mm-dd-yy") == 0) {
            session->date_style = 0;
        } else if (strcasecmp(format, "dd-mm-yy") == 0) {
            session->date_style = 1;
        } else if (strcasecmp(format, "yy-mm-dd") == 0) {
            session->date_style = 2;
        } else if (strcasecmp(format, "yymmdd") == 0) {
            session->date_style = 3;
        }
    }
    /* Yell: inverted boolean - "Yell Off" means false, absence means true */
    session->yell_enabled = true; /* Default to true */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Yell", buf, sizeof(buf))) {
        if (strcasecmp(buf, "Off") == 0) {
            session->yell_enabled = false;
        }
    }
    ctl_to_ng_parse_boolean(maxctl_path, "Chat Capture", &session->chat_capture);
    ctl_to_ng_parse_boolean(maxctl_path, "Strict Transfer", &session->strict_xfer);
    ctl_to_ng_parse_boolean(maxctl_path, "Gate Netmail", &session->gate_netmail);
    ctl_to_ng_parse_boolean(maxctl_path, "Global High Bit", &session->global_high_bit);
    /* Upload Check Dupe: presence of keyword = true, absence = false */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Upload Check Dupe", buf, sizeof(buf))) {
        session->upload_check_dupe = true;
    }
    ctl_to_ng_parse_boolean(maxctl_path, "Check Dupe Ext", &session->upload_check_dupe_extension);
    ctl_to_ng_parse_boolean(maxctl_path, "Use UMSGIDs", &session->use_umsgids);
    ctl_to_ng_parse_boolean(maxctl_path, "Local Baud 9600", &session->compat_local_baud_9600);

    /* String fields */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Edit Menu", buf, sizeof(buf))) {
        session->edit_menu = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Chat Program", buf, sizeof(buf))) {
        session->chat_program = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Local Editor", buf, sizeof(buf))) {
        session->local_editor = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Upload Log", buf, sizeof(buf))) {
        session->upload_log = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Virus Check", buf, sizeof(buf))) {
        session->virus_check = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Comment Area", buf, sizeof(buf))) {
        session->comment_area = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Highest MsgArea", buf, sizeof(buf))) {
        session->highest_message_area = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Highest FileArea", buf, sizeof(buf))) {
        session->highest_file_area = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Area Change Keys", buf, sizeof(buf))) {
        session->area_change_keys = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Charset", buf, sizeof(buf))) {
        session->charset = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Track PrivView", buf, sizeof(buf))) {
        session->track_privview = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Track PrivMod", buf, sizeof(buf))) {
        session->track_privmod = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Track Base", buf, sizeof(buf))) {
        session->track_base = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Track Exclude", buf, sizeof(buf))) {
        session->track_exclude = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Attach Base", buf, sizeof(buf))) {
        session->attach_base = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Attach Path", buf, sizeof(buf))) {
        session->attach_path = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Attach Archiver", buf, sizeof(buf))) {
        session->attach_archiver = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "First Menu", buf, sizeof(buf))) {
        session->first_menu = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "First File Area", buf, sizeof(buf))) {
        session->first_file_area = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "First Message Area", buf, sizeof(buf))) {
        session->first_message_area = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Kill Private", buf, sizeof(buf))) {
        session->kill_private = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Kill Attach", buf, sizeof(buf))) {
        session->kill_attach = dup_str_or_null(buf);
        
        /* Extract privilege level from "Ask <level>" format */
        if (strncasecmp(buf, "Ask", 3) == 0) {
            char *level_name = buf + 3;
            while (*level_name && isspace((unsigned char)*level_name)) level_name++;
            
            if (*level_name) {
                /* Check if it's numeric */
                bool is_numeric = true;
                for (const char *p = level_name; *p; p++) {
                    if (!isdigit((unsigned char)*p)) {
                        is_numeric = false;
                        break;
                    }
                }
                
                if (is_numeric) {
                    session->kill_attach_priv = atoi(level_name);
                } else {
                    /* Derive sys_path and lookup access level */
                    char sys_path[512];
                    if (snprintf(sys_path, sizeof(sys_path), "%s", maxctl_path) < (int)sizeof(sys_path)) {
                        char *p1 = strrchr(sys_path, '/');
                        if (p1) {
                            *p1 = '\0';
                            char *p2 = strrchr(sys_path, '/');
                            if (p2) {
                                *p2 = '\0';
                                session->kill_attach_priv = parse_priv_level(sys_path, level_name);
                            }
                        }
                    }
                }
            }
        }
    }

    /* Integer fields */
    ctl_to_ng_parse_int(maxctl_path, "Date Style", &session->date_style);
    ctl_to_ng_parse_int(maxctl_path, "Filelist Margin", &session->filelist_margin);
    ctl_to_ng_parse_int(maxctl_path, "After Call Exit", &session->exit_after_call);
    
    /* Logon Level: parse as access level name or integer */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Logon Level", buf, sizeof(buf))) {
        /* Check if it's a numeric value */
        bool is_numeric = true;
        for (const char *p = buf; *p; p++) {
            if (!isdigit((unsigned char)*p)) {
                is_numeric = false;
                break;
            }
        }
        
        if (is_numeric) {
            session->logon_priv = atoi(buf);
        } else {
            /* Derive sys_path from maxctl_path */
            char sys_path[512];
            if (snprintf(sys_path, sizeof(sys_path), "%s", maxctl_path) < (int)sizeof(sys_path)) {
                char *p1 = strrchr(sys_path, '/');
                if (p1) {
                    *p1 = '\0';
                    char *p2 = strrchr(sys_path, '/');
                    if (p2) {
                        *p2 = '\0';
                        session->logon_priv = parse_priv_level(sys_path, buf);
                    }
                }
            }
        }
    }
    ctl_to_ng_parse_int(maxctl_path, "Logon TimeLimit", &session->logon_timelimit);
    ctl_to_ng_parse_int(maxctl_path, "Min Logon Baud", &session->min_logon_baud);
    ctl_to_ng_parse_int(maxctl_path, "Min NonTTY Baud", &session->min_graphics_baud);
    ctl_to_ng_parse_int(maxctl_path, "Min RIP Baud", &session->min_rip_baud);
    ctl_to_ng_parse_int(maxctl_path, "Input Timeout", &session->input_timeout);
    ctl_to_ng_parse_int(maxctl_path, "Mailchecker Reply Priv", &session->mailchecker_reply_priv);
    ctl_to_ng_parse_int(maxctl_path, "Mailchecker Kill Priv", &session->mailchecker_kill_priv);
    
    /* Message Edit Ask LocalAttach privilege level */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Message Edit Ask LocalAttach", buf, sizeof(buf))) {
        /* Check if it's numeric */
        bool is_numeric = true;
        for (const char *p = buf; *p; p++) {
            if (!isdigit((unsigned char)*p)) {
                is_numeric = false;
                break;
            }
        }
        if (is_numeric) {
            session->msg_localattach_priv = atoi(buf);
        } else {
            /* Derive sys_path and lookup access level */
            char sys_path[512];
            if (snprintf(sys_path, sizeof(sys_path), "%s", maxctl_path) < (int)sizeof(sys_path)) {
                char *p1 = strrchr(sys_path, '/');
                if (p1) {
                    *p1 = '\0';
                    char *p2 = strrchr(sys_path, '/');
                    if (p2) {
                        *p2 = '\0';
                        session->msg_localattach_priv = parse_priv_level(sys_path, buf);
                    }
                }
            }
        }
    }
    
    ctl_to_ng_parse_int(maxctl_path, "Kill Attach Priv", &session->kill_attach_priv);

    /* Unsigned fields */
    int tmp;
    if (ctl_to_ng_parse_int(maxctl_path, "Upload Space Free", &tmp)) {
        session->min_free_kb = (unsigned int)tmp;
    }
    if (ctl_to_ng_parse_int(maxctl_path, "Min Free KB", &tmp)) {
        session->min_free_kb = (unsigned int)tmp;
    }
    
    /* MaxMsgSize */
    if (ctl_to_ng_parse_int(maxctl_path, "MaxMsgSize", &tmp)) {
        session->max_msgsize = (unsigned int)tmp;
    }
    if (ctl_to_ng_parse_int(maxctl_path, "Max MsgSize", &tmp)) {
        session->max_msgsize = (unsigned int)tmp;
    }
    if (ctl_to_ng_parse_int(maxctl_path, "Message Size", &tmp)) {
        session->max_msgsize = (unsigned int)tmp;
    }

    return true;
}

/* Populate MaxCfgNgEquipment from max.ctl */
bool ctl_to_ng_populate_equipment(const char *maxctl_path, MaxCfgNgEquipment *equip)
{
    if (!maxctl_path || !equip) return false;

    char buf[512];

    /* Output mode */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Output", buf, sizeof(buf))) {
        equip->output = dup_str_or_null(buf);
    }

    /* COM port */
    ctl_to_ng_parse_int(maxctl_path, "COM Port", &equip->com_port);

    /* Baud rate */
    ctl_to_ng_parse_int(maxctl_path, "Baud Maximum", &equip->baud_maximum);

    /* Modem strings */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Modem Busy", buf, sizeof(buf))) {
        equip->busy = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Modem Init", buf, sizeof(buf))) {
        equip->init = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Modem Ring", buf, sizeof(buf))) {
        equip->ring = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Modem Answer", buf, sizeof(buf))) {
        equip->answer = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Modem Connect", buf, sizeof(buf))) {
        equip->connect = dup_str_or_null(buf);
    }

    /* Carrier mask */
    ctl_to_ng_parse_int(maxctl_path, "Carrier Mask", &equip->carrier_mask);

    /* Handshaking - parse as string list */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Handshaking", buf, sizeof(buf))) {
        /* Parse comma-separated list */
        char *tok = strtok(buf, ",");
        while (tok) {
            char *t = trim_ws(tok);
            if (t && t[0]) {
                char **new_arr = realloc(equip->handshaking, (equip->handshaking_count + 1) * sizeof(char *));
                if (new_arr) {
                    equip->handshaking = new_arr;
                    equip->handshaking[equip->handshaking_count++] = strdup(t);
                }
            }
            tok = strtok(NULL, ",");
        }
    }

    /* Boolean flags */
    ctl_to_ng_parse_boolean(maxctl_path, "Send Break", &equip->send_break);
    ctl_to_ng_parse_boolean(maxctl_path, "No Critical", &equip->no_critical);

    return true;
}

/* Populate MaxCfgNgReader from reader.ctl or max.ctl */
bool ctl_to_ng_populate_reader(const char *sys_path, MaxCfgNgReader *reader)
{
    if (!sys_path || !reader) return false;

    char reader_ctl[512];
    snprintf(reader_ctl, sizeof(reader_ctl), "%s/etc/reader.ctl", sys_path);

    char buf[512];

    /* Try reader.ctl first, fall back to max.ctl */
    struct stat st;
    const char *ctl_path = (stat(reader_ctl, &st) == 0) ? reader_ctl : NULL;
    if (!ctl_path) {
        snprintf(reader_ctl, sizeof(reader_ctl), "%s/etc/max.ctl", sys_path);
        ctl_path = reader_ctl;
    }

    ctl_to_ng_parse_int(ctl_path, "Max Pack", &reader->max_pack);

    if (ctl_to_ng_parse_keyword(ctl_path, "Archivers CTL", buf, sizeof(buf))) {
        reader->archivers_ctl = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(ctl_path, "Packet Name", buf, sizeof(buf))) {
        reader->packet_name = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(ctl_path, "Work Directory", buf, sizeof(buf))) {
        reader->work_directory = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(ctl_path, "Phone", buf, sizeof(buf))) {
        reader->phone = dup_str_or_null(buf);
    }

    return true;
}

/* Populate MaxCfgNgGeneralDisplayFiles from max.ctl */
bool ctl_to_ng_populate_display_files(const char *maxctl_path, MaxCfgNgGeneralDisplayFiles *files)
{
    if (!maxctl_path || !files) return false;

    char buf[512];

    /* Parse all display file paths from max.ctl using actual CTL keywords */
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Logo", buf, sizeof(buf))) {
        files->logo = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses NotFound", buf, sizeof(buf))) {
        files->not_found = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Application", buf, sizeof(buf))) {
        files->application = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Welcome", buf, sizeof(buf))) {
        files->welcome = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses NewUser1", buf, sizeof(buf))) {
        files->new_user1 = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses NewUser2", buf, sizeof(buf))) {
        files->new_user2 = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Rookie", buf, sizeof(buf))) {
        files->rookie = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Configure", buf, sizeof(buf))) {
        files->not_configured = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Quote", buf, sizeof(buf))) {
        files->quote = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses DayLimit", buf, sizeof(buf))) {
        files->day_limit = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses TimeWarn", buf, sizeof(buf))) {
        files->time_warn = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses TooSlow", buf, sizeof(buf))) {
        files->too_slow = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses ByeBye", buf, sizeof(buf))) {
        files->bye_bye = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses BadLogon", buf, sizeof(buf))) {
        files->bad_logon = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Barricade", buf, sizeof(buf))) {
        files->barricade = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses NoSpace", buf, sizeof(buf))) {
        files->no_space = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses NoMail", buf, sizeof(buf))) {
        files->no_mail = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Cant_Enter_Area", buf, sizeof(buf))) {
        files->area_not_exist = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses BeginChat", buf, sizeof(buf))) {
        files->chat_begin = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses EndChat", buf, sizeof(buf))) {
        files->chat_end = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Leaving", buf, sizeof(buf))) {
        files->out_leaving = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Returning", buf, sizeof(buf))) {
        files->out_return = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Shell_Leaving", buf, sizeof(buf))) {
        files->shell_to_dos = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Shell_Returning", buf, sizeof(buf))) {
        files->back_from_dos = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses LocateHelp", buf, sizeof(buf))) {
        files->locate = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses ContentsHelp", buf, sizeof(buf))) {
        files->contents = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses MaxEdHelp", buf, sizeof(buf))) {
        files->oped_help = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses BOREDhelp", buf, sizeof(buf))) {
        files->line_ed_help = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses ReplaceHelp", buf, sizeof(buf))) {
        files->replace_help = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses InquireHelp", buf, sizeof(buf))) {
        files->inquire_help = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses ScanHelp", buf, sizeof(buf))) {
        files->scan_help = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses ListHelp", buf, sizeof(buf))) {
        files->list_help = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses HeaderHelp", buf, sizeof(buf))) {
        files->header_help = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses EntryHelp", buf, sizeof(buf))) {
        files->entry_help = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses XferBaud", buf, sizeof(buf))) {
        files->xfer_baud = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses FileAreas", buf, sizeof(buf))) {
        files->file_area_list = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Format FileHeader", buf, sizeof(buf))) {
        files->file_header = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Format FileFormat", buf, sizeof(buf))) {
        files->file_format = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Format FileFooter", buf, sizeof(buf))) {
        files->file_footer = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses MsgAreas", buf, sizeof(buf))) {
        files->msg_area_list = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Format MsgHeader", buf, sizeof(buf))) {
        files->msg_header = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Format MsgFormat", buf, sizeof(buf))) {
        files->msg_format = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Format MsgFooter", buf, sizeof(buf))) {
        files->msg_footer = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses ProtocolDump", buf, sizeof(buf))) {
        files->protocol_dump = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Filename_Format", buf, sizeof(buf))) {
        files->fname_format = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Format Time", buf, sizeof(buf))) {
        files->time_format = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Format Date", buf, sizeof(buf))) {
        files->date_format = dup_str_or_null(buf);
    }
    if (ctl_to_ng_parse_keyword(maxctl_path, "Uses Tunes", buf, sizeof(buf))) {
        files->tune = dup_str_or_null(buf);
    }

    return true;
}

/* Populate MaxCfgNgLanguage from language.ctl */
bool ctl_to_ng_populate_language(const char *sys_path, MaxCfgNgLanguage *lang)
{
    if (!sys_path || !lang) return false;

    char lang_ctl[512];
    snprintf(lang_ctl, sizeof(lang_ctl), "%s/etc/language.ctl", sys_path);

    /* Parse max_lang */
    ctl_to_ng_parse_int(lang_ctl, "Max Languages", &lang->max_lang);

    /* Parse language files */
    FILE *fp = fopen(lang_ctl, "r");
    if (fp) {
        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            char *s = trim_ws(line);
            if (!s || s[0] == '\0' || s[0] == '%' || s[0] == ';') continue;

            if (line_starts_with_keyword(s, "Language")) {
                char *v = extract_value_after_keyword(s, "Language");
                if (v && v[0]) {
                    /* Skip "Language Section" header line */
                    if (strcmp(v, "Section") == 0) {
                        continue;
                    }
                    char **new_arr = realloc(lang->lang_files, (lang->lang_file_count + 1) * sizeof(char *));
                    if (new_arr) {
                        lang->lang_files = new_arr;
                        lang->lang_files[lang->lang_file_count++] = strdup(v);
                    }
                }
            }
        }
        fclose(fp);
    }

    /* Note: max_ptrs, max_heap, etc. are runtime-only and should NOT be exported to TOML */
    /* They are computed dynamically at runtime based on loaded language files */

    return true;
}
