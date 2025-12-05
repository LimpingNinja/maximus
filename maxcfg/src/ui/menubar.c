/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * menubar.c - Top menu bar for maxcfg
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "maxcfg.h"
#include "ui.h"
#include "fields.h"
#include "prm_data.h"
#include "treeview.h"

/* Forward declarations for menu actions */
static void action_placeholder(void);
static void action_bbs_sysop_info(void);
static void action_system_paths(void);
static void action_display_files(void);
static void action_logging_options(void);
static void action_global_toggles(void);
static void action_login_settings(void);
static void action_new_user_defaults(void);

/* Message area actions */
static void action_msg_tree_config(void);
static void action_msg_divisions_picklist(void);
static void action_msg_areas_picklist(void);

/* Security levels action */
static void action_security_levels(void);

/* ============================================================================
 * Menu Definitions
 * ============================================================================ */

/* Setup > Global submenu */
static MenuItem setup_global_items[] = {
    { "BBS and Sysop Information", "B", NULL, 0, action_bbs_sysop_info, true },
    { "System Paths",              "S", NULL, 0, action_system_paths, true },
    { "Logging Options",           "L", NULL, 0, action_logging_options, true },
    { "Global Toggles",            "G", NULL, 0, action_global_toggles, true },
    { "Login Settings",            "o", NULL, 0, action_login_settings, true },
    { "New User Defaults",         "N", NULL, 0, action_new_user_defaults, true },
    { "Default Colors",            "C", NULL, 0, action_default_colors, true },
};

/* Setup > Matrix submenu */
static MenuItem setup_matrix_items[] = {
    { "Network Addresses",  "N", NULL, 0, action_placeholder, true },
    { "Netmail Settings",   "e", NULL, 0, action_placeholder, true },
    { "Origin Lines",       "O", NULL, 0, action_placeholder, true },
};

/* Setup menu items */
static MenuItem setup_items[] = {
    { "Global",           "G", setup_global_items, 7, NULL, true },
    { "Security Levels",  "S", NULL, 0, action_security_levels, true },
    { "Archivers",        "A", NULL, 0, action_placeholder, true },
    { "Protocols",        "P", NULL, 0, action_placeholder, true },
    { "Events",           "E", NULL, 0, action_placeholder, true },
    { "Languages",        "L", NULL, 0, action_placeholder, true },
    { "Matrix/Echomail",  "M", setup_matrix_items, 3, NULL, true },
};

/* Content menu items */
static MenuItem content_items[] = {
    { "Menus",           "M", NULL, 0, action_placeholder, true },
    { "Display Files",   "D", NULL, 0, action_display_files, true },
    { "Help Files",      "H", NULL, 0, action_placeholder, true },
    { "Bulletins",       "B", NULL, 0, action_placeholder, true },
    { "Reader Settings", "R", NULL, 0, action_placeholder, true },
};

/* Messages > Setup Message Areas submenu */
static MenuItem msg_setup_items[] = {
    { "Tree Configuration",         "T", NULL, 0, action_msg_tree_config, true },
    { "Picklist: Message Divisions","D", NULL, 0, action_msg_divisions_picklist, true },
    { "Picklist: Message Areas",    "A", NULL, 0, action_msg_areas_picklist, true },
};

/* Messages menu items */
static MenuItem messages_items[] = {
    { "Setup Message Areas",   "S", msg_setup_items, 3, NULL, true },
    { "Netmail Aliases",       "N", NULL, 0, action_placeholder, true },
    { "Matrix and Echomail",   "M", NULL, 0, action_placeholder, true },
    { "Squish Configuration",  "q", NULL, 0, action_placeholder, true },
    { "QWK Mail and Networking","Q", NULL, 0, action_placeholder, true },
};

/* Forward declarations for file area actions */
static void action_file_tree_config(void);
static void action_file_divisions_picklist(void);
static void action_file_areas_picklist(void);

/* Files > Setup File Areas submenu */
static MenuItem file_setup_items[] = {
    { "Tree Configuration",       "T", NULL, 0, action_file_tree_config, true },
    { "Picklist: File Divisions", "D", NULL, 0, action_file_divisions_picklist, true },
    { "Picklist: File Areas",     "A", NULL, 0, action_file_areas_picklist, true },
};

/* Files menu items */
static MenuItem files_items[] = {
    { "Setup File Areas",  "S", file_setup_items, 3, NULL, true },
    { "Protocol Config",   "P", NULL, 0, action_placeholder, true },
    { "Archiver Config",   "A", NULL, 0, action_placeholder, true },
};

/* Users menu items */
static MenuItem users_items[] = {
    { "User Editor",     "U", NULL, 0, action_placeholder, true },
    { "Bad Users",       "B", NULL, 0, action_placeholder, true },
    { "Reserved Names",  "R", NULL, 0, action_placeholder, true },
};

/* Tools menu items */
static MenuItem tools_items[] = {
    { "Recompile All",      "R", NULL, 0, action_placeholder, true },
    { "Compile Config",     "C", NULL, 0, action_placeholder, true },
    { "Compile Language",   "L", NULL, 0, action_placeholder, true },
    { "View Log",           "V", NULL, 0, action_placeholder, true },
    { "System Information", "S", NULL, 0, action_placeholder, true },
};

/* Top-level menus */
static TopMenu top_menus[] = {
    { "Setup",       setup_items,       7 },
    { "Content",     content_items,     5 },
    { "Messages",    messages_items,    5 },
    { "Files",       files_items,       3 },
    { "Users",       users_items,       3 },
    { "Tools",       tools_items,       5 },
};

#define NUM_TOP_MENUS (sizeof(top_menus) / sizeof(top_menus[0]))

/* Menu positions (calculated on init) */
static int menu_positions[NUM_TOP_MENUS];

/* ============================================================================
 * Implementation
 * ============================================================================ */

static void action_placeholder(void)
{
    dialog_message("Not Implemented", 
                   "This feature is not yet implemented.\n\n"
                   "Coming soon!");
}

/* Current field values - loaded from PRM */
static char *bbs_sysop_values[7] = { NULL };
static char *system_paths_values[8] = { NULL };

static void action_bbs_sysop_info(void)
{
    if (!g_prm) return;
    
    /* Free old values */
    for (int i = 0; i < 7; i++) free(bbs_sysop_values[i]);
    
    /* Load from PRM */
    bbs_sysop_values[0] = strdup(PRMSTR(system_name));
    bbs_sysop_values[1] = strdup(PRMSTR(sysop));
    bbs_sysop_values[2] = strdup(prm_flag_get(FLAG_alias) ? "Yes" : "No");
    bbs_sysop_values[3] = strdup(prm_flag_get(FLAG_ask_name) ? "Yes" : "No");
    bbs_sysop_values[4] = strdup(prm_flag2_get(FLAG2_1NAME) ? "Yes" : "No");
    bbs_sysop_values[5] = strdup(prm_flag2_get(FLAG2_CHKANSI) ? "Yes" : "No");
    bbs_sysop_values[6] = strdup(prm_flag2_get(FLAG2_CHKRIP) ? "Yes" : "No");
    
    form_edit("BBS and Sysop Information", bbs_sysop_fields, bbs_sysop_field_count, bbs_sysop_values);
    
    /* Save changes back to PRM */
    if (bbs_sysop_values[0]) prm_set_string(&g_prm->prm.system_name, bbs_sysop_values[0]);
    if (bbs_sysop_values[1]) prm_set_string(&g_prm->prm.sysop, bbs_sysop_values[1]);
    prm_flag_set(FLAG_alias, bbs_sysop_values[2] && strcmp(bbs_sysop_values[2], "Yes") == 0);
    prm_flag_set(FLAG_ask_name, bbs_sysop_values[3] && strcmp(bbs_sysop_values[3], "Yes") == 0);
    prm_flag2_set(FLAG2_1NAME, bbs_sysop_values[4] && strcmp(bbs_sysop_values[4], "Yes") == 0);
    prm_flag2_set(FLAG2_CHKANSI, bbs_sysop_values[5] && strcmp(bbs_sysop_values[5], "Yes") == 0);
    prm_flag2_set(FLAG2_CHKRIP, bbs_sysop_values[6] && strcmp(bbs_sysop_values[6], "Yes") == 0);
}

static void action_system_paths(void)
{
    if (!g_prm) return;
    
    /* Load from PRM */
    free(system_paths_values[0]);
    free(system_paths_values[1]);
    free(system_paths_values[2]);
    free(system_paths_values[3]);
    free(system_paths_values[4]);
    free(system_paths_values[5]);
    free(system_paths_values[6]);
    free(system_paths_values[7]);
    
    system_paths_values[0] = strdup(PRMSTR(sys_path));
    system_paths_values[1] = strdup(PRMSTR(misc_path));
    system_paths_values[2] = strdup(PRMSTR(lang_path));
    system_paths_values[3] = strdup(PRMSTR(temppath));
    system_paths_values[4] = strdup(PRMSTR(ipc_path));
    system_paths_values[5] = strdup(PRMSTR(user_file));
    system_paths_values[6] = strdup(PRMSTR(access));
    system_paths_values[7] = strdup(PRMSTR(log_name));
    
    form_edit("System Paths", system_paths_fields, system_paths_field_count, system_paths_values);
    
    /* Save changes back to PRM */
    if (system_paths_values[0]) prm_set_string(&g_prm->prm.sys_path, system_paths_values[0]);
    if (system_paths_values[1]) prm_set_string(&g_prm->prm.misc_path, system_paths_values[1]);
    if (system_paths_values[2]) prm_set_string(&g_prm->prm.lang_path, system_paths_values[2]);
    if (system_paths_values[3]) prm_set_string(&g_prm->prm.temppath, system_paths_values[3]);
    if (system_paths_values[4]) prm_set_string(&g_prm->prm.ipc_path, system_paths_values[4]);
    if (system_paths_values[5]) prm_set_string(&g_prm->prm.user_file, system_paths_values[5]);
    if (system_paths_values[6]) prm_set_string(&g_prm->prm.access, system_paths_values[6]);
    if (system_paths_values[7]) prm_set_string(&g_prm->prm.log_name, system_paths_values[7]);
}

/* Display Files values (40 fields) */
static char *display_files_values[40] = { NULL };

static void action_display_files(void)
{
    if (!g_prm) return;
    
    /* Free old values */
    for (int i = 0; i < 40; i++) {
        free(display_files_values[i]);
        display_files_values[i] = NULL;
    }
    
    /* Load from PRM - map each field to its PRM offset */
    display_files_values[0] = strdup(PRMSTR(logo));           /* Logo */
    display_files_values[1] = strdup(PRMSTR(notfound));       /* Not Found */
    display_files_values[2] = strdup(PRMSTR(application));    /* Application */
    display_files_values[3] = strdup(PRMSTR(welcome));        /* Welcome */
    display_files_values[4] = strdup(PRMSTR(newuser1));       /* New User 1 */
    display_files_values[5] = strdup(PRMSTR(newuser2));       /* New User 2 */
    display_files_values[6] = strdup(PRMSTR(rookie));         /* Rookie */
    display_files_values[7] = strdup(PRMSTR(not_configured)); /* Configure */
    display_files_values[8] = strdup(PRMSTR(quote));          /* Quotes */
    display_files_values[9] = strdup(PRMSTR(daylimit));       /* Day Limit */
    display_files_values[10] = strdup(PRMSTR(timewarn));      /* Time Warning */
    display_files_values[11] = strdup(PRMSTR(tooslow));       /* Too Slow */
    display_files_values[12] = strdup(PRMSTR(byebye));        /* Goodbye */
    display_files_values[13] = strdup(PRMSTR(bad_logon));     /* Bad Logon */
    display_files_values[14] = strdup(PRMSTR(barricade));     /* Barricade */
    display_files_values[15] = strdup(PRMSTR(no_space));      /* No Space */
    display_files_values[16] = strdup(PRMSTR(nomail));        /* No Mail */
    display_files_values[17] = strdup(PRMSTR(areanotexist));  /* Can't Enter Area */
    display_files_values[18] = strdup(PRMSTR(chat_fbegin));   /* Begin Chat */
    display_files_values[19] = strdup(PRMSTR(chat_fend));     /* End Chat */
    display_files_values[20] = strdup(PRMSTR(out_leaving));   /* Leaving */
    display_files_values[21] = strdup(PRMSTR(out_return));    /* Returning */
    display_files_values[22] = strdup(PRMSTR(shelltodos));    /* Shell Leaving */
    display_files_values[23] = strdup(PRMSTR(backfromdos));   /* Shell Returning */
    display_files_values[24] = strdup(PRMSTR(hlp_locate));    /* Locate Help */
    display_files_values[25] = strdup(PRMSTR(hlp_contents));  /* Contents Help */
    display_files_values[26] = strdup(PRMSTR(oped_help));     /* MaxEd Help */
    display_files_values[27] = strdup(PRMSTR(hlp_editor));    /* Line Editor Help */
    display_files_values[28] = strdup(PRMSTR(hlp_replace));   /* Replace Help */
    display_files_values[29] = strdup(PRMSTR(msg_inquire));   /* Inquire Help */
    display_files_values[30] = strdup(PRMSTR(hlp_scan));      /* Scan Help */
    display_files_values[31] = strdup(PRMSTR(hlp_list));      /* List Help */
    display_files_values[32] = strdup(PRMSTR(hlp_hdrentry));  /* Header Help */
    display_files_values[33] = strdup(PRMSTR(hlp_msgentry));  /* Entry Help */
    display_files_values[34] = strdup(PRMSTR(xferbaud));      /* Transfer Baud */
    display_files_values[35] = strdup(PRMSTR(file_area_list));/* File Areas */
    display_files_values[36] = strdup(PRMSTR(msgarea_list));  /* Msg Areas */
    display_files_values[37] = strdup(PRMSTR(proto_dump));    /* Protocol Dump */
    display_files_values[38] = strdup(PRMSTR(fname_format));  /* Filename Format */
    display_files_values[39] = strdup(PRMSTR(tune_file));     /* Tunes */
    
    form_edit("Display Files", display_files_fields, display_files_field_count, display_files_values);
    
    /* Save changes back to PRM */
    if (display_files_values[0]) prm_set_string(&g_prm->prm.logo, display_files_values[0]);
    if (display_files_values[1]) prm_set_string(&g_prm->prm.notfound, display_files_values[1]);
    if (display_files_values[2]) prm_set_string(&g_prm->prm.application, display_files_values[2]);
    if (display_files_values[3]) prm_set_string(&g_prm->prm.welcome, display_files_values[3]);
    if (display_files_values[4]) prm_set_string(&g_prm->prm.newuser1, display_files_values[4]);
    if (display_files_values[5]) prm_set_string(&g_prm->prm.newuser2, display_files_values[5]);
    if (display_files_values[6]) prm_set_string(&g_prm->prm.rookie, display_files_values[6]);
    if (display_files_values[7]) prm_set_string(&g_prm->prm.not_configured, display_files_values[7]);
    if (display_files_values[8]) prm_set_string(&g_prm->prm.quote, display_files_values[8]);
    if (display_files_values[9]) prm_set_string(&g_prm->prm.daylimit, display_files_values[9]);
    if (display_files_values[10]) prm_set_string(&g_prm->prm.timewarn, display_files_values[10]);
    if (display_files_values[11]) prm_set_string(&g_prm->prm.tooslow, display_files_values[11]);
    if (display_files_values[12]) prm_set_string(&g_prm->prm.byebye, display_files_values[12]);
    if (display_files_values[13]) prm_set_string(&g_prm->prm.bad_logon, display_files_values[13]);
    if (display_files_values[14]) prm_set_string(&g_prm->prm.barricade, display_files_values[14]);
    if (display_files_values[15]) prm_set_string(&g_prm->prm.no_space, display_files_values[15]);
    if (display_files_values[16]) prm_set_string(&g_prm->prm.nomail, display_files_values[16]);
    if (display_files_values[17]) prm_set_string(&g_prm->prm.areanotexist, display_files_values[17]);
    if (display_files_values[18]) prm_set_string(&g_prm->prm.chat_fbegin, display_files_values[18]);
    if (display_files_values[19]) prm_set_string(&g_prm->prm.chat_fend, display_files_values[19]);
    if (display_files_values[20]) prm_set_string(&g_prm->prm.out_leaving, display_files_values[20]);
    if (display_files_values[21]) prm_set_string(&g_prm->prm.out_return, display_files_values[21]);
    if (display_files_values[22]) prm_set_string(&g_prm->prm.shelltodos, display_files_values[22]);
    if (display_files_values[23]) prm_set_string(&g_prm->prm.backfromdos, display_files_values[23]);
    if (display_files_values[24]) prm_set_string(&g_prm->prm.hlp_locate, display_files_values[24]);
    if (display_files_values[25]) prm_set_string(&g_prm->prm.hlp_contents, display_files_values[25]);
    if (display_files_values[26]) prm_set_string(&g_prm->prm.oped_help, display_files_values[26]);
    if (display_files_values[27]) prm_set_string(&g_prm->prm.hlp_editor, display_files_values[27]);
    if (display_files_values[28]) prm_set_string(&g_prm->prm.hlp_replace, display_files_values[28]);
    if (display_files_values[29]) prm_set_string(&g_prm->prm.msg_inquire, display_files_values[29]);
    if (display_files_values[30]) prm_set_string(&g_prm->prm.hlp_scan, display_files_values[30]);
    if (display_files_values[31]) prm_set_string(&g_prm->prm.hlp_list, display_files_values[31]);
    if (display_files_values[32]) prm_set_string(&g_prm->prm.hlp_hdrentry, display_files_values[32]);
    if (display_files_values[33]) prm_set_string(&g_prm->prm.hlp_msgentry, display_files_values[33]);
    if (display_files_values[34]) prm_set_string(&g_prm->prm.xferbaud, display_files_values[34]);
    if (display_files_values[35]) prm_set_string(&g_prm->prm.file_area_list, display_files_values[35]);
    if (display_files_values[36]) prm_set_string(&g_prm->prm.msgarea_list, display_files_values[36]);
    if (display_files_values[37]) prm_set_string(&g_prm->prm.proto_dump, display_files_values[37]);
    if (display_files_values[38]) prm_set_string(&g_prm->prm.fname_format, display_files_values[38]);
    if (display_files_values[39]) prm_set_string(&g_prm->prm.tune_file, display_files_values[39]);
}

/* Logging Options values */
static char *logging_options_values[3] = { NULL };

static void action_logging_options(void)
{
    const char *log_modes[] = { "", "", "Terse", "", "Verbose", "", "Trace" };
    
    if (!g_prm) return;
    
    free(logging_options_values[0]);
    free(logging_options_values[1]);
    free(logging_options_values[2]);
    
    logging_options_values[0] = strdup(PRMSTR(log_name));
    logging_options_values[1] = strdup(g_prm->prm.log_mode <= 6 ? log_modes[g_prm->prm.log_mode] : "Verbose");
    logging_options_values[2] = strdup(PRMSTR(caller_log));
    
    form_edit("Logging Options", logging_options_fields, logging_options_field_count, logging_options_values);
    
    /* Save changes back to PRM */
    if (logging_options_values[0]) prm_set_string(&g_prm->prm.log_name, logging_options_values[0]);
    if (logging_options_values[1]) {
        if (strcmp(logging_options_values[1], "Terse") == 0) g_prm->prm.log_mode = LOG_terse;
        else if (strcmp(logging_options_values[1], "Trace") == 0) g_prm->prm.log_mode = LOG_trace;
        else g_prm->prm.log_mode = LOG_verbose;
        g_prm->modified = true;
    }
    if (logging_options_values[2]) prm_set_string(&g_prm->prm.caller_log, logging_options_values[2]);
}

/* Global Toggles values */
static char *global_toggles_values[6] = { NULL };

static void action_global_toggles(void)
{
    if (!g_prm) return;
    
    free(global_toggles_values[0]);
    free(global_toggles_values[1]);
    free(global_toggles_values[2]);
    free(global_toggles_values[3]);
    free(global_toggles_values[4]);
    free(global_toggles_values[5]);
    
    global_toggles_values[0] = strdup(prm_flag_get(FLAG_snoop) ? "Yes" : "No");
    global_toggles_values[1] = strdup(prm_flag2_get(FLAG2_NOENCRYPT) ? "No" : "Yes");  /* Inverted */
    global_toggles_values[2] = strdup(prm_flag_get(FLAG_watchdog) ? "Yes" : "No");
    global_toggles_values[3] = strdup(prm_flag2_get(FLAG2_SWAPOUT) ? "Yes" : "No");
    global_toggles_values[4] = strdup(prm_flag2_get(FLAG2_ltimeout) ? "Yes" : "No");
    global_toggles_values[5] = strdup(prm_flag_get(FLAG_statusline) ? "Yes" : "No");
    
    form_edit("Global Toggles", global_toggles_fields, global_toggles_field_count, global_toggles_values);
    
    /* Save changes back to PRM */
    prm_flag_set(FLAG_snoop, global_toggles_values[0] && strcmp(global_toggles_values[0], "Yes") == 0);
    prm_flag2_set(FLAG2_NOENCRYPT, global_toggles_values[1] && strcmp(global_toggles_values[1], "No") == 0);  /* Inverted */
    prm_flag_set(FLAG_watchdog, global_toggles_values[2] && strcmp(global_toggles_values[2], "Yes") == 0);
    prm_flag2_set(FLAG2_SWAPOUT, global_toggles_values[3] && strcmp(global_toggles_values[3], "Yes") == 0);
    prm_flag2_set(FLAG2_ltimeout, global_toggles_values[4] && strcmp(global_toggles_values[4], "Yes") == 0);
    prm_flag_set(FLAG_statusline, global_toggles_values[5] && strcmp(global_toggles_values[5], "Yes") == 0);
}

/* Login Settings values */
static char *login_settings_values[8] = { NULL };

static void action_login_settings(void)
{
    char buf[32];
    
    if (!g_prm) return;
    
    for (int i = 0; i < 8; i++) free(login_settings_values[i]);
    
    snprintf(buf, sizeof(buf), "%u", g_prm->prm.logon_priv);
    login_settings_values[0] = strdup(buf);
    snprintf(buf, sizeof(buf), "%u", g_prm->prm.logon_time);
    login_settings_values[1] = strdup(buf);
    snprintf(buf, sizeof(buf), "%u", g_prm->prm.min_baud);
    login_settings_values[2] = strdup(buf);
    snprintf(buf, sizeof(buf), "%u", g_prm->prm.speed_graphics);
    login_settings_values[3] = strdup(buf);
    snprintf(buf, sizeof(buf), "%u", g_prm->prm.speed_rip);
    login_settings_values[4] = strdup(buf);
    snprintf(buf, sizeof(buf), "%u", g_prm->prm.input_timeout);
    login_settings_values[5] = strdup(buf);
    login_settings_values[6] = strdup(prm_flag2_get(FLAG2_CHKANSI) ? "Yes" : "No");
    login_settings_values[7] = strdup(prm_flag2_get(FLAG2_CHKRIP) ? "Yes" : "No");
    
    form_edit("Login Settings", login_settings_fields, login_settings_field_count, login_settings_values);
    
    /* Save changes back to PRM */
    if (login_settings_values[0]) { g_prm->prm.logon_priv = (word)atoi(login_settings_values[0]); g_prm->modified = true; }
    if (login_settings_values[1]) { g_prm->prm.logon_time = (word)atoi(login_settings_values[1]); g_prm->modified = true; }
    if (login_settings_values[2]) { g_prm->prm.min_baud = (word)atoi(login_settings_values[2]); g_prm->modified = true; }
    if (login_settings_values[3]) { g_prm->prm.speed_graphics = (word)atoi(login_settings_values[3]); g_prm->modified = true; }
    if (login_settings_values[4]) { g_prm->prm.speed_rip = (word)atoi(login_settings_values[4]); g_prm->modified = true; }
    if (login_settings_values[5]) { g_prm->prm.input_timeout = (byte)atoi(login_settings_values[5]); g_prm->modified = true; }
    prm_flag2_set(FLAG2_CHKANSI, login_settings_values[6] && strcmp(login_settings_values[6], "Yes") == 0);
    prm_flag2_set(FLAG2_CHKRIP, login_settings_values[7] && strcmp(login_settings_values[7], "Yes") == 0);
}

/* New User Defaults values */
static char *new_user_defaults_values[8] = { NULL };

static void action_new_user_defaults(void)
{
    if (!g_prm) return;
    
    for (int i = 0; i < 8; i++) free(new_user_defaults_values[i]);
    
    new_user_defaults_values[0] = strdup(prm_flag_get(FLAG_ask_phone) ? "Yes" : "No");
    new_user_defaults_values[1] = strdup(prm_flag_get(FLAG_ask_name) ? "Yes" : "No");
    new_user_defaults_values[2] = strdup(prm_flag_get(FLAG_alias) ? "Yes" : "No");
    new_user_defaults_values[3] = strdup(prm_flag2_get(FLAG2_1NAME) ? "Yes" : "No");
    new_user_defaults_values[4] = strdup(prm_flag_get(FLAG_norname) ? "Yes" : "No");
    new_user_defaults_values[5] = strdup(PRMSTR(first_menu));
    new_user_defaults_values[6] = strdup(PRMSTR(begin_filearea));
    new_user_defaults_values[7] = strdup(PRMSTR(begin_msgarea));
    
    form_edit("New User Defaults", new_user_defaults_fields, new_user_defaults_field_count, new_user_defaults_values);
    
    /* Save changes back to PRM */
    prm_flag_set(FLAG_ask_phone, new_user_defaults_values[0] && strcmp(new_user_defaults_values[0], "Yes") == 0);
    prm_flag_set(FLAG_ask_name, new_user_defaults_values[1] && strcmp(new_user_defaults_values[1], "Yes") == 0);
    prm_flag_set(FLAG_alias, new_user_defaults_values[2] && strcmp(new_user_defaults_values[2], "Yes") == 0);
    prm_flag2_set(FLAG2_1NAME, new_user_defaults_values[3] && strcmp(new_user_defaults_values[3], "Yes") == 0);
    prm_flag_set(FLAG_norname, new_user_defaults_values[4] && strcmp(new_user_defaults_values[4], "Yes") == 0);
    if (new_user_defaults_values[5]) prm_set_string(&g_prm->prm.first_menu, new_user_defaults_values[5]);
    if (new_user_defaults_values[6]) prm_set_string(&g_prm->prm.begin_filearea, new_user_defaults_values[6]);
    if (new_user_defaults_values[7]) prm_set_string(&g_prm->prm.begin_msgarea, new_user_defaults_values[7]);
}

/* ============================================================================
 * Message Area Functions
 * ============================================================================ */

/* Sample data for demo - will be replaced with actual CTL parsing */
static ListItem sample_divisions[] = {
    { "Programming Languages", "5 areas", true, NULL },
    { "Gaming", "3 areas", true, NULL },
    { "General", "2 areas", true, NULL },
};

static ListItem sample_areas[] = {
    { "Main", "Main", true, NULL },
    { "Fidonet Netmail", "Fido Netmail", true, NULL },
    { "Trashcan Conference", "Lost mail", true, NULL },
    { "My Conference", NULL, true, NULL },
    { "Pascal", NULL, true, NULL },
    { "Fun Conference", NULL, true, NULL },
    { "Another Conference without a Div", NULL, true, NULL },
    { "C++", NULL, true, NULL },
    { "Ferrari", NULL, false, NULL },
    { "Mazda", NULL, true, NULL },
};

/* Build tree from sample_divisions and sample_areas */
static TreeNode **build_tree_from_samples(int *count)
{
    /* 
     * Map our sample data to a tree structure:
     * - Programming Languages (division, div=0)
     *   - Pascal (area, div=1)
     *   - C++ (area, div=1)
     * - Gaming (division, div=0)
     *   - Fun Conference (area, div=1)
     *   - Ferrari (area, div=1)
     *   - Mazda (area, div=1)
     * - General (division, div=0)
     *   - My Conference (area, div=1)
     *   - Another Conference without a Div (area, div=1)
     * - Main (area, div=0) - no division
     * - Fidonet Netmail (area, div=0)
     * - Trashcan Conference (area, div=0)
     */
    
    /* We'll have 3 divisions + 3 top-level areas = 6 root nodes */
    TreeNode **roots = malloc(6 * sizeof(TreeNode *));
    if (!roots) {
        *count = 0;
        return NULL;
    }
    
    int idx = 0;
    
    /* Division 0: Programming Languages */
    roots[idx] = treenode_create("Programming Languages", "Programming Languages",
                                 sample_divisions[0].extra,
                                 TREENODE_DIVISION, 0);
    /* Add Pascal and C++ under it */
    TreeNode *pascal_area = treenode_create("Pascal", "Programming Languages.Pascal",
                                            "Programming language area",
                                            TREENODE_AREA, 1);
    treenode_add_child(roots[idx], pascal_area);
    
    TreeNode *cpp = treenode_create("C++", "Programming Languages.C++",
                                    "C++ programming discussions",
                                    TREENODE_AREA, 1);
    treenode_add_child(roots[idx], cpp);
    idx++;
    
    /* Division 1: Gaming */
    roots[idx] = treenode_create("Gaming", "Gaming",
                                 sample_divisions[1].extra,
                                 TREENODE_DIVISION, 0);
    /* Add Fun Conference, Ferrari, Mazda under it */
    TreeNode *fun = treenode_create("Fun Conference", "Gaming.Fun Conference",
                                    "Fun gaming discussions",
                                    TREENODE_AREA, 1);
    treenode_add_child(roots[idx], fun);
    
    TreeNode *ferrari = treenode_create("Ferrari", "Gaming.Ferrari",
                                        "Racing games - Ferrari",
                                        TREENODE_AREA, 1);
    ferrari->enabled = false;  /* Disabled in sample data */
    treenode_add_child(roots[idx], ferrari);
    
    TreeNode *mazda = treenode_create("Mazda", "Gaming.Mazda",
                                      "Racing games - Mazda",
                                      TREENODE_AREA, 1);
    treenode_add_child(roots[idx], mazda);
    idx++;
    
    /* Division 2: General */
    roots[idx] = treenode_create("General", "General",
                                 sample_divisions[2].extra,
                                 TREENODE_DIVISION, 0);
    /* Add My Conference and Another Conference under it */
    TreeNode *myconf = treenode_create("My Conference", "General.My Conference",
                                       "General discussions",
                                       TREENODE_AREA, 1);
    treenode_add_child(roots[idx], myconf);
    
    TreeNode *another = treenode_create("Another Conference", "General.Another Conference",
                                        "Another conference area",
                                        TREENODE_AREA, 1);
    treenode_add_child(roots[idx], another);
    idx++;
    
    /* Top-level areas (no division) */
    roots[idx++] = treenode_create("Main", "Main",
                                   sample_areas[0].extra,
                                   TREENODE_AREA, 0);
    
    roots[idx++] = treenode_create("Fidonet Netmail", "Fidonet Netmail",
                                   sample_areas[1].extra,
                                   TREENODE_AREA, 0);
    
    roots[idx++] = treenode_create("Trashcan Conference", "Trashcan Conference",
                                   sample_areas[2].extra,
                                   TREENODE_AREA, 0);
    
    *count = idx;
    return roots;
}

static void action_msg_tree_config(void)
{
    /* Build tree from sample data */
    int root_count = 0;
    TreeNode **roots = build_tree_from_samples(&root_count);
    
    if (!roots) {
        dialog_message("Error", "Failed to build tree data.");
        return;
    }
    
    /* Show tree view */
    treeview_show("Conference Tree Editor", roots, root_count, NULL, TREE_CONTEXT_MESSAGE);
    
    /* Cleanup */
    treenode_array_free(roots, root_count);
    
    /* Redraw screen */
    touchwin(stdscr);
    wnoutrefresh(stdscr);
}

/* Helper to show division form with given values (for edit or insert) */
static void show_division_form(const char *title, char **div_values)
{
    form_edit(title, msg_division_fields, msg_division_field_count, div_values);
}

/* Helper to initialize default division values */
static void init_default_division_values(char **div_values)
{
    div_values[0] = strdup("");           /* Name */
    div_values[1] = strdup("(None)");     /* Parent Division */
    div_values[2] = strdup("");           /* Description */
    div_values[3] = strdup("");           /* Display file */
    div_values[4] = strdup("Demoted");    /* ACS */
}

/* Forward declaration */
static void populate_division_options(void);

static void action_msg_divisions_picklist(void)
{
    int selected = 0;
    ListPickResult result;
    
    /* Populate division options for Parent Division picker */
    populate_division_options();
    
    do {
        result = listpicker_show("Message Divisions", sample_divisions, 3, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < 3) {
            /* Edit division - show division form */
            char *div_values[8] = { NULL };
            
            /* Load sample data */
            div_values[0] = strdup(sample_divisions[selected].name);
            div_values[1] = strdup("(None)");  /* Parent Division - TODO: get from data */
            div_values[2] = strdup(sample_divisions[selected].extra ? sample_divisions[selected].extra : "");
            div_values[3] = strdup("");        /* Display file */
            div_values[4] = strdup("Demoted"); /* ACS */
            
            show_division_form("Edit Message Division", div_values);
            
            /* Free form values */
            for (int i = 0; i < 8; i++) free(div_values[i]);
        }
        else if (result == LISTPICK_INSERT) {
            /* Insert new division below current selection */
            char *div_values[8] = { NULL };
            init_default_division_values(div_values);
            
            show_division_form("New Message Division", div_values);
            
            /* TODO: Actually insert into data structure when CTL support is added */
            
            /* Free form values */
            for (int i = 0; i < 8; i++) free(div_values[i]);
        }
        
    } while (result != LISTPICK_EXIT);
}

/* Populate the global msg_division_options array from sample_divisions */
static void populate_division_options(void)
{
    /* Build options: (None) + all division names + NULL terminator */
    /* msg_division_options is declared in fields.c */
    extern const char *msg_division_options[16];
    
    int idx = 0;
    msg_division_options[idx++] = "(None)";
    for (int i = 0; i < 3 && idx < 15; i++) {  /* 3 = sample_divisions count */
        msg_division_options[idx++] = sample_divisions[i].name;
    }
    msg_division_options[idx] = NULL;
}

/* Helper to initialize default area values */
static void init_default_area_values(char **area_values)
{
    /* Group 1: Basic info (6 fields) */
    area_values[0] = strdup("");                /* MsgArea */
    area_values[1] = strdup("(None)");          /* Division */
    area_values[2] = strdup("");                /* Tag */
    area_values[3] = strdup("");                /* Path */
    area_values[4] = strdup("");                /* Desc */
    area_values[5] = strdup("");                /* Owner */
    /* 6 = separator */
    
    /* Group 2: Format/Type (3 fields) */
    area_values[7] = strdup("Squish");          /* Style_Format */
    area_values[8] = strdup("Local");           /* Style_Type */
    area_values[9] = strdup("Real Name");       /* Style_Name */
    /* 10 = separator */
    
    /* Group 3: Style toggles (10 fields, paired) */
    area_values[11] = strdup("No");             /* Style_Pvt */
    area_values[12] = strdup("Yes");            /* Style_Pub */
    area_values[13] = strdup("No");             /* Style_HiBit */
    area_values[14] = strdup("No");             /* Style_Anon */
    area_values[15] = strdup("No");             /* Style_NoRNK */
    area_values[16] = strdup("No");             /* Style_Audit */
    area_values[17] = strdup("No");             /* Style_ReadOnly */
    area_values[18] = strdup("No");             /* Style_Hidden */
    area_values[19] = strdup("No");             /* Style_Attach */
    area_values[20] = strdup("No");             /* Style_NoMailChk */
    /* 21 = separator */
    
    /* Group 4: Renum (3 fields) */
    area_values[22] = strdup("0");              /* Renum_Max */
    area_values[23] = strdup("0");              /* Renum_Days */
    area_values[24] = strdup("0");              /* Renum_Skip */
    
    /* Group 5: Access (1 field) */
    area_values[25] = strdup("Demoted");        /* ACS */
    /* 26 = separator */
    
    /* Group 6: Origin (3 fields) */
    area_values[27] = strdup("");               /* Origin_Addr */
    area_values[28] = strdup("");               /* Origin_SeenBy */
    area_values[29] = strdup("");               /* Origin_Line */
    /* 30 = separator */
    
    /* Group 7: Advanced (5 fields) */
    area_values[31] = strdup("");               /* Barricade_Menu */
    area_values[32] = strdup("");               /* Barricade_File */
    area_values[33] = strdup("");               /* MenuName */
    area_values[34] = strdup("");               /* MenuReplace */
    area_values[35] = strdup("");               /* AttachPath */
}

/* Helper to load area values for editing */
static void load_area_values(char **area_values, int selected)
{
    /* Group 1: Basic info (6 fields) */
    area_values[0] = strdup(sample_areas[selected].name);                /* MsgArea */
    area_values[1] = strdup("(None)");                                   /* Division - TODO: get from data */
    area_values[2] = strdup(sample_areas[selected].extra ? sample_areas[selected].extra : ""); /* Tag */
    area_values[3] = strdup("spool/msgbase/area");                       /* Path */
    area_values[4] = strdup("Sample message area description");          /* Desc */
    area_values[5] = strdup("");                                         /* Owner */
    /* 6 = separator */
    
    /* Group 2: Format/Type (3 fields) */
    area_values[7] = strdup("Squish");                                   /* Style_Format */
    area_values[8] = strdup("Local");                                    /* Style_Type */
    area_values[9] = strdup("Real Name");                                /* Style_Name */
    /* 10 = separator */
    
    /* Group 3: Style toggles (10 fields, paired) */
    area_values[11] = strdup("No");                                      /* Style_Pvt */
    area_values[12] = strdup("Yes");                                     /* Style_Pub */
    area_values[13] = strdup("No");                                      /* Style_HiBit */
    area_values[14] = strdup("No");                                      /* Style_Anon */
    area_values[15] = strdup("No");                                      /* Style_NoRNK */
    area_values[16] = strdup("No");                                      /* Style_Audit */
    area_values[17] = strdup("No");                                      /* Style_ReadOnly */
    area_values[18] = strdup("No");                                      /* Style_Hidden */
    area_values[19] = strdup("No");                                      /* Style_Attach */
    area_values[20] = strdup("No");                                      /* Style_NoMailChk */
    /* 21 = separator */
    
    /* Group 4: Renum (3 fields) */
    area_values[22] = strdup("0");                                       /* Renum_Max */
    area_values[23] = strdup("0");                                       /* Renum_Days */
    area_values[24] = strdup("0");                                       /* Renum_Skip */
    
    /* Group 5: Access (1 field) */
    area_values[25] = strdup("Demoted");                                 /* ACS */
    /* 26 = separator */
    
    /* Group 6: Origin (3 fields) */
    area_values[27] = strdup("");                                        /* Origin_Addr */
    area_values[28] = strdup("");                                        /* Origin_SeenBy */
    area_values[29] = strdup("");                                        /* Origin_Line */
    /* 30 = separator */
    
    /* Group 7: Advanced (5 fields) */
    area_values[31] = strdup("");                                        /* Barricade_Menu */
    area_values[32] = strdup("");                                        /* Barricade_File */
    area_values[33] = strdup("");                                        /* MenuName */
    area_values[34] = strdup("");                                        /* MenuReplace */
    area_values[35] = strdup("");                                        /* AttachPath */
}

static void action_msg_areas_picklist(void)
{
    int selected = 0;
    ListPickResult result;
    
    /* Populate the global division options array with current divisions */
    populate_division_options();
    
    do {
        result = listpicker_show("Message Areas", sample_areas, 10, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < 10) {
            /* Edit area - show area form (36 fields with 6 separators) */
            char *area_values[45] = { NULL };
            load_area_values(area_values, selected);
            
            form_edit("Edit Message Area", msg_area_fields, msg_area_field_count, area_values);
            
            /* Free form values */
            for (int i = 0; i < 45; i++) free(area_values[i]);
        }
        else if (result == LISTPICK_INSERT) {
            /* Insert new area below current selection */
            char *area_values[45] = { NULL };
            init_default_area_values(area_values);
            
            form_edit("New Message Area", msg_area_fields, msg_area_field_count, area_values);
            
            /* TODO: Actually insert into data structure when CTL support is added */
            
            /* Free form values */
            for (int i = 0; i < 45; i++) free(area_values[i]);
        }
        
    } while (result != LISTPICK_EXIT);
}

/* ============================================================================
 * File Area Functions
 * ============================================================================ */

/* Sample data for demo - will be replaced with actual CTL parsing */
static ListItem sample_file_divisions[] = {
    { "Games", "Game files and patches", true, NULL },
    { "Utilities", "System utilities", true, NULL },
    { "Development", "Programming tools", true, NULL },
};

static ListItem sample_file_areas[] = {
    { "Uploads", "New uploads awaiting processing", true, NULL },
    { "DOS Games", "Classic DOS games", true, NULL },
    { "Windows Games", "Windows game files", true, NULL },
    { "Archivers", "ZIP, ARJ, RAR utilities", true, NULL },
    { "Disk Utils", "Disk management tools", true, NULL },
    { "Compilers", "C/C++/Pascal compilers", true, NULL },
    { "Editors", "Text and code editors", true, NULL },
    { "Sysop Tools", "BBS utilities", false, NULL },
};

/* Build file tree from sample_file_divisions and sample_file_areas */
static TreeNode **build_file_tree_from_samples(int *count)
{
    /* Map our sample data to a tree structure */
    TreeNode **roots = malloc(5 * sizeof(TreeNode *));
    if (!roots) {
        *count = 0;
        return NULL;
    }
    
    int idx = 0;
    
    /* Division 0: Games */
    roots[idx] = treenode_create("Games", "Games",
                                 sample_file_divisions[0].extra,
                                 TREENODE_DIVISION, 0);
    TreeNode *dos = treenode_create("DOS Games", "Games.DOS Games",
                                    "Classic DOS games",
                                    TREENODE_AREA, 1);
    treenode_add_child(roots[idx], dos);
    TreeNode *win = treenode_create("Windows Games", "Games.Windows Games",
                                    "Windows game files",
                                    TREENODE_AREA, 1);
    treenode_add_child(roots[idx], win);
    idx++;
    
    /* Division 1: Utilities */
    roots[idx] = treenode_create("Utilities", "Utilities",
                                 sample_file_divisions[1].extra,
                                 TREENODE_DIVISION, 0);
    TreeNode *arch = treenode_create("Archivers", "Utilities.Archivers",
                                     "ZIP, ARJ, RAR utilities",
                                     TREENODE_AREA, 1);
    treenode_add_child(roots[idx], arch);
    TreeNode *disk = treenode_create("Disk Utils", "Utilities.Disk Utils",
                                     "Disk management tools",
                                     TREENODE_AREA, 1);
    treenode_add_child(roots[idx], disk);
    idx++;
    
    /* Division 2: Development */
    roots[idx] = treenode_create("Development", "Development",
                                 sample_file_divisions[2].extra,
                                 TREENODE_DIVISION, 0);
    TreeNode *comp = treenode_create("Compilers", "Development.Compilers",
                                     "C/C++/Pascal compilers",
                                     TREENODE_AREA, 1);
    treenode_add_child(roots[idx], comp);
    TreeNode *edit = treenode_create("Editors", "Development.Editors",
                                     "Text and code editors",
                                     TREENODE_AREA, 1);
    treenode_add_child(roots[idx], edit);
    idx++;
    
    /* Top-level areas (no division) */
    roots[idx++] = treenode_create("Uploads", "Uploads",
                                   sample_file_areas[0].extra,
                                   TREENODE_AREA, 0);
    
    TreeNode *sysop = treenode_create("Sysop Tools", "Sysop Tools",
                                      "BBS utilities",
                                      TREENODE_AREA, 0);
    sysop->enabled = false;
    roots[idx++] = sysop;
    
    *count = idx;
    return roots;
}

static void action_file_tree_config(void)
{
    /* Build tree from sample data */
    int root_count = 0;
    TreeNode **roots = build_file_tree_from_samples(&root_count);
    
    if (!roots) {
        dialog_message("Error", "Failed to build tree data.");
        return;
    }
    
    /* Show tree view */
    treeview_show("File Area Tree Editor", roots, root_count, NULL, TREE_CONTEXT_FILE);
    
    /* Cleanup */
    treenode_array_free(roots, root_count);
    
    /* Redraw screen */
    touchwin(stdscr);
    wnoutrefresh(stdscr);
}

/* Helper to show file division form */
static void show_file_division_form(const char *title, char **div_values)
{
    form_edit(title, file_division_fields, file_division_field_count, div_values);
}

/* Helper to initialize default file division values */
static void init_default_file_division_values(char **div_values)
{
    div_values[0] = strdup("");           /* Name */
    div_values[1] = strdup("(None)");     /* Parent Division */
    div_values[2] = strdup("");           /* Description */
    div_values[3] = strdup("");           /* Display file */
    div_values[4] = strdup("Demoted");    /* ACS */
}

/* Populate the file division options array */
static void populate_file_division_options(void)
{
    extern const char *file_division_options[16];
    
    int idx = 0;
    file_division_options[idx++] = "(None)";
    for (int i = 0; i < 3 && idx < 15; i++) {
        file_division_options[idx++] = sample_file_divisions[i].name;
    }
    file_division_options[idx] = NULL;
}

static void action_file_divisions_picklist(void)
{
    int selected = 0;
    ListPickResult result;
    
    populate_file_division_options();
    
    do {
        result = listpicker_show("File Divisions", sample_file_divisions, 3, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < 3) {
            char *div_values[8] = { NULL };
            
            div_values[0] = strdup(sample_file_divisions[selected].name);
            div_values[1] = strdup("(None)");
            div_values[2] = strdup(sample_file_divisions[selected].extra ? sample_file_divisions[selected].extra : "");
            div_values[3] = strdup("");
            div_values[4] = strdup("Demoted");
            
            show_file_division_form("Edit File Division", div_values);
            
            for (int i = 0; i < 8; i++) free(div_values[i]);
        }
        else if (result == LISTPICK_INSERT) {
            char *div_values[8] = { NULL };
            init_default_file_division_values(div_values);
            
            show_file_division_form("New File Division", div_values);
            
            for (int i = 0; i < 8; i++) free(div_values[i]);
        }
        
    } while (result != LISTPICK_EXIT);
}

/* Helper to initialize default file area values */
static void init_default_file_area_values(char **area_values)
{
    area_values[0] = strdup("");           /* Area tag */
    area_values[1] = strdup("(None)");     /* Division */
    area_values[2] = strdup("");           /* Description */
    /* separator at 3 */
    area_values[4] = strdup("");           /* Download path */
    area_values[5] = strdup("");           /* Upload path */
    area_values[6] = strdup("");           /* FILES.BBS path */
    /* separator at 7 */
    area_values[8] = strdup("Default");    /* Date style */
    area_values[9] = strdup("No");         /* Slow */
    area_values[10] = strdup("No");        /* Staged */
    area_values[11] = strdup("No");        /* NoNew */
    area_values[12] = strdup("No");        /* Hidden */
    area_values[13] = strdup("No");        /* FreeTime */
    area_values[14] = strdup("No");        /* FreeBytes */
    area_values[15] = strdup("No");        /* NoIndex */
    /* separator at 16 */
    area_values[17] = strdup("Demoted");   /* ACS */
    /* separator at 18 */
    area_values[19] = strdup("");          /* Barricade menu */
    area_values[20] = strdup("");          /* Barricade file */
    area_values[21] = strdup("");          /* Custom menu */
    area_values[22] = strdup("");          /* Replace menu */
}

/* Helper to load file area values from sample data */
static void load_file_area_values(char **area_values, int idx)
{
    area_values[0] = strdup(sample_file_areas[idx].name);
    area_values[1] = strdup("(None)");  /* TODO: get from data */
    area_values[2] = strdup(sample_file_areas[idx].extra ? sample_file_areas[idx].extra : "");
    area_values[4] = strdup("/var/max/files");
    area_values[5] = strdup("/var/max/upload");
    area_values[6] = strdup("");
    area_values[8] = strdup("Default");
    area_values[9] = strdup("No");         /* Slow */
    area_values[10] = strdup("No");        /* Staged */
    area_values[11] = strdup("No");        /* NoNew */
    area_values[12] = strdup(sample_file_areas[idx].enabled ? "No" : "Yes");  /* Hidden */
    area_values[13] = strdup("No");        /* FreeTime */
    area_values[14] = strdup("No");        /* FreeBytes */
    area_values[15] = strdup("No");        /* NoIndex */
    area_values[17] = strdup("Demoted");
    area_values[19] = strdup("");
    area_values[20] = strdup("");
    area_values[21] = strdup("");
    area_values[22] = strdup("");
}

static void action_file_areas_picklist(void)
{
    int selected = 0;
    ListPickResult result;
    
    populate_file_division_options();
    
    do {
        result = listpicker_show("File Areas", sample_file_areas, 8, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < 8) {
            char *area_values[25] = { NULL };
            load_file_area_values(area_values, selected);
            
            form_edit("Edit File Area", file_area_fields, file_area_field_count, area_values);
            
            for (int i = 0; i < 25; i++) free(area_values[i]);
        }
        else if (result == LISTPICK_INSERT) {
            char *area_values[25] = { NULL };
            init_default_file_area_values(area_values);
            
            form_edit("New File Area", file_area_fields, file_area_field_count, area_values);
            
            for (int i = 0; i < 25; i++) free(area_values[i]);
        }
        
    } while (result != LISTPICK_EXIT);
}

/* ============================================================================
 * Security/Access Levels Functions
 * ============================================================================ */

/* Sample data for demo - will be replaced with actual CTL parsing */
static ListItem sample_access_levels[] = {
    { "Transient",  "Level 0 - Twit/Banned users", true, NULL },
    { "Demoted",    "Level 10 - Restricted access", true, NULL },
    { "Limited",    "Level 20 - Limited user", true, NULL },
    { "Normal",     "Level 30 - Standard user", true, NULL },
    { "Worthy",     "Level 40 - Trusted user", true, NULL },
    { "Privil",     "Level 50 - Privileged user", true, NULL },
    { "Favored",    "Level 60 - Favored user", true, NULL },
    { "Extra",      "Level 70 - Extra privileges", true, NULL },
    { "AsstSysop",  "Level 80 - Assistant Sysop", true, NULL },
    { "Sysop",      "Level 100 - System Operator", true, NULL },
    { "Hidden",     "Level 65535 - Hidden/Internal", false, NULL },
};

#define NUM_SAMPLE_ACCESS_LEVELS 11

/* Helper to initialize default access level values */
static void init_default_access_values(char **values)
{
    values[0] = strdup("");           /* Access name */
    values[1] = strdup("0");          /* Level */
    values[2] = strdup("");           /* Description */
    values[3] = strdup("");           /* Alias */
    values[4] = strdup("");           /* Key */
    /* separator at 5 */
    values[6] = strdup("60");         /* Session time */
    values[7] = strdup("90");         /* Daily time */
    values[8] = strdup("-1");         /* Daily calls */
    /* separator at 9 */
    values[10] = strdup("5000");      /* Download limit */
    values[11] = strdup("0");         /* File ratio */
    values[12] = strdup("1000");      /* Ratio-free */
    values[13] = strdup("100");       /* Upload reward */
    /* separator at 14 */
    values[15] = strdup("300");       /* Logon baud */
    values[16] = strdup("300");       /* Xfer baud */
    /* separator at 17 */
    values[18] = strdup("");          /* Login file */
    /* separator at 19 */
    values[20] = strdup("");          /* Flags */
    values[21] = strdup("");          /* Mail flags */
    values[22] = strdup("0");         /* User flags */
    /* separator at 23 */
    values[24] = strdup("0");         /* Oldpriv */
}

/* Helper to load access level values from sample data */
static void load_access_level_values(char **values, int idx)
{
    /* Sample level numbers matching access.ctl */
    static const int level_numbers[] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 100, 65535 };
    static const int oldpriv_values[] = { -2, 0, 1, 2, 3, 4, 5, 6, 7, 10, 11 };
    char buf[16];
    
    values[0] = strdup(sample_access_levels[idx].name);
    snprintf(buf, sizeof(buf), "%d", level_numbers[idx]);
    values[1] = strdup(buf);
    values[2] = strdup(sample_access_levels[idx].extra ? sample_access_levels[idx].extra : "");
    values[3] = strdup("");           /* Alias */
    values[4] = strdup("");           /* Key - will use first letter */
    values[6] = strdup("60");
    values[7] = strdup("90");
    values[8] = strdup("-1");
    values[10] = strdup("5000");
    values[11] = strdup("0");
    values[12] = strdup("1000");
    values[13] = strdup("100");
    values[15] = strdup("300");
    values[16] = strdup("300");
    values[18] = strdup("");
    values[20] = strdup(idx >= 9 ? "NoLimits" : "");  /* Sysop/Hidden get NoLimits */
    values[21] = strdup(idx >= 9 ? "ShowPvt MsgAttrAny" : "");
    values[22] = strdup("0");
    snprintf(buf, sizeof(buf), "%d", oldpriv_values[idx]);
    values[24] = strdup(buf);
}

static void action_security_levels(void)
{
    int selected = 0;
    ListPickResult result;
    
    do {
        result = listpicker_show("Security Levels", sample_access_levels, 
                                 NUM_SAMPLE_ACCESS_LEVELS, &selected);
        
        if (result == LISTPICK_EDIT && selected >= 0 && selected < NUM_SAMPLE_ACCESS_LEVELS) {
            char *values[30] = { NULL };
            load_access_level_values(values, selected);
            
            form_edit("Edit Access Level", access_level_fields, 
                      access_level_field_count, values);
            
            /* TODO: Save changes when CTL support is added */
            
            for (int i = 0; i < 30; i++) free(values[i]);
        }
        else if (result == LISTPICK_INSERT) {
            char *values[30] = { NULL };
            init_default_access_values(values);
            
            form_edit("New Access Level", access_level_fields,
                      access_level_field_count, values);
            
            /* TODO: Insert into data structure when CTL support is added */
            
            for (int i = 0; i < 30; i++) free(values[i]);
        }
        else if (result == LISTPICK_DELETE && selected >= 0 && selected < NUM_SAMPLE_ACCESS_LEVELS) {
            /* Toggle enabled state - actual deletion would need confirmation */
            sample_access_levels[selected].enabled = !sample_access_levels[selected].enabled;
        }
        
    } while (result != LISTPICK_EXIT);
}

void menubar_init(void)
{
    /* Calculate menu positions */
    int x = 2;  /* Start with some padding */
    
    for (size_t i = 0; i < NUM_TOP_MENUS; i++) {
        menu_positions[i] = x;
        x += strlen(top_menus[i].label) + 3;  /* label + spacing */
    }
}

void draw_menubar(void)
{
    attron(COLOR_PAIR(CP_MENU_BAR));
    
    /* Clear the menu bar line with black background */
    move(MENUBAR_ROW, 0);
    for (int i = 0; i < COLS; i++) {
        addch(' ');
    }
    
    /* Draw each menu item */
    for (size_t i = 0; i < NUM_TOP_MENUS; i++) {
        const char *label = top_menus[i].label;
        int x = menu_positions[i];
        
        if ((int)i == g_state.current_menu) {
            /* Highlighted: bold yellow on blue for entire item */
            attron(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
            mvprintw(MENUBAR_ROW, x, " %s ", label);
            attroff(COLOR_PAIR(CP_MENU_HIGHLIGHT) | A_BOLD);
        } else {
            /* Normal: bold yellow hotkey, grey rest */
            move(MENUBAR_ROW, x + 1);
            
            /* First char is hotkey - bold yellow */
            attron(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
            addch(label[0]);
            attroff(COLOR_PAIR(CP_MENU_HOTKEY) | A_BOLD);
            
            /* Rest of label - grey */
            attron(COLOR_PAIR(CP_MENU_BAR));
            printw("%s", label + 1);
            attroff(COLOR_PAIR(CP_MENU_BAR));
        }
    }
    
    wnoutrefresh(stdscr);
}

bool menubar_handle_key(int ch)
{
    switch (ch) {
        case KEY_LEFT:
            if (g_state.current_menu > 0) {
                g_state.current_menu--;
                if (dropdown_is_open()) {
                    dropdown_open(g_state.current_menu);
                }
            }
            return true;
            
        case KEY_RIGHT:
            if (g_state.current_menu < (int)NUM_TOP_MENUS - 1) {
                g_state.current_menu++;
                if (dropdown_is_open()) {
                    dropdown_open(g_state.current_menu);
                }
            }
            return true;
            
        case KEY_DOWN:
        case '\n':
        case '\r':
            dropdown_open(g_state.current_menu);
            return true;
            
        default:
            /* Check for hotkey (first letter of menu) */
            for (size_t i = 0; i < NUM_TOP_MENUS; i++) {
                if (toupper(ch) == toupper(top_menus[i].label[0])) {
                    g_state.current_menu = i;
                    dropdown_open(g_state.current_menu);
                    return true;
                }
            }
            break;
    }
    
    return false;
}

int menubar_get_current(void)
{
    return g_state.current_menu;
}

void menubar_set_current(int index)
{
    if (index >= 0 && index < (int)NUM_TOP_MENUS) {
        g_state.current_menu = index;
    }
}

/* Get top menu data (used by dropdown) */
TopMenu* menubar_get_menu(int index)
{
    if (index >= 0 && index < (int)NUM_TOP_MENUS) {
        return &top_menus[index];
    }
    return NULL;
}

int menubar_get_position(int index)
{
    if (index >= 0 && index < (int)NUM_TOP_MENUS) {
        return menu_positions[index];
    }
    return 0;
}
