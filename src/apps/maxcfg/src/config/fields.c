/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * fields.c - Field definitions for maxcfg forms
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <stddef.h>
#include "fields.h"

/* Common toggle option sets */
const char *toggle_yes_no[] = { "Yes", "No", NULL };
const char *toggle_enabled_disabled[] = { "Enabled", "Disabled", NULL };
const char *toggle_on_off[] = { "On", "Off", NULL };

/* ============================================================================
 * BBS and Sysop Information (max.ctl System Section)
 * ============================================================================ */

const FieldDef bbs_sysop_fields[] = {
    {
        .keyword = "Name",
        .label = "BBS Name",
        .help = "The name of your BBS. Used as default for EchoMail origin "
                "lines unless a custom origin is specified. Do not include "
                "your FidoNet address - Maximus adds it automatically.",
        .type = FIELD_TEXT,
        .max_length = 60,
        .default_value = "My BBS",
        .toggle_options = NULL
    },
    {
        .keyword = "SysOp",
        .label = "SysOp Name",
        .help = "The SysOp's name for display purposes and the 'To:' field "
                "when users leave log-off comments. This does NOT grant any "
                "special privileges - use the User Editor to set privilege "
                "levels.",
        .type = FIELD_TEXT,
        .max_length = 35,
        .default_value = "SysOp",
        .toggle_options = NULL
    },
    {
        .keyword = "Alias System",
        .label = "Alias System",
        .help = "Enable system-wide alias support. Messages will use aliases "
                "by default. Users appear by alias in Who's Online.",
        .type = FIELD_TOGGLE,
        .max_length = 2,
        .default_value = "Yes",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "Ask Alias",
        .label = "Ask for Alias",
        .help = "Prompt new users for an alias at log-on. If Alias System is "
                "disabled, aliases are still stored but not used by default.",
        .type = FIELD_TOGGLE,
        .max_length = 2,
        .default_value = "Yes",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "Single Word Names",
        .label = "Single Word Names",
        .help = "Allow usernames with only a single word. Useful for alias-based "
                "systems. Suppresses 'What is your LAST name' prompt.",
        .type = FIELD_TOGGLE,
        .max_length = 2,
        .default_value = "Yes",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "Check ANSI",
        .label = "Check ANSI",
        .help = "Verify ANSI capability at login when a user has ANSI enabled. "
                "Prompts user to confirm if auto-detect fails.",
        .type = FIELD_TOGGLE,
        .max_length = 2,
        .default_value = "Yes",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "Check RIP",
        .label = "Check RIP",
        .help = "Verify RIP graphics capability at login. Prompts user to confirm "
                "if auto-detect fails.",
        .type = FIELD_TOGGLE,
        .max_length = 2,
        .default_value = "Yes",
        .toggle_options = toggle_yes_no
    },
};

const int bbs_sysop_field_count = sizeof(bbs_sysop_fields) / sizeof(bbs_sysop_fields[0]);

/* ============================================================================
 * System Paths (max.ctl System Section)
 * ============================================================================ */

const FieldDef system_paths_fields[] = {
    {
        .keyword = "Path System",
        .label = "System Path",
        .help = "The 'home base' directory for Maximus where executables are "
                "stored. All relative paths in this config are based from "
                "this directory. Use an absolute path.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "/var/max"
    },
    {
        .keyword = "Path Misc",
        .label = "Misc Path",
        .help = "Directory for miscellaneous text files displayed to users, "
                "including Fxx.BBS files shown when the SysOp presses local "
                "function keys.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "/var/max/etc/misc"
    },
    {
        .keyword = "Path Language",
        .label = "Language Path",
        .help = "Directory containing language files. Must contain at minimum "
                "an .LTF (Language Translation File) for each declared "
                "language. The .MAD, .LTH and .H files are not required.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "/var/max/etc/lang"
    },
    {
        .keyword = "Path Temp",
        .label = "Temp Path",
        .help = "Temporary directory for uploads and system operations. "
                "WARNING: Files in this directory may be deleted at any time. "
                "Do not use for permanent storage.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "/var/max/tmp"
    },
    {
        .keyword = "Path IPC",
        .label = "IPC Path",
        .help = "Inter-process communications directory for multi-node setups. "
                "Should point to a RAM drive for best performance. See "
                "documentation before enabling.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "ipc"
    },
    {
        .keyword = "File Password",
        .label = "User File",
        .help = "Location of the user database file containing all users, "
                "passwords, and user information. Relative to System Path.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "etc/user"
    },
    {
        .keyword = "File Access",
        .label = "Access File",
        .help = "Location of the privilege levels database. Levels are defined "
                "in access.ctl and describe attributes of user classes.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "etc/access"
    },
    {
        .keyword = "Log File",
        .label = "Log File",
        .help = "Location of the Maximus activity log file. Records system "
                "events, user activity, and errors.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "log/max.log"
    },
};

const int system_paths_field_count = sizeof(system_paths_fields) / sizeof(system_paths_fields[0]);

/* ============================================================================
 * Logging Options (max.ctl System Section)
 * ============================================================================ */

static const char *log_level_options[] = { "Terse", "Verbose", "Trace", NULL };

const FieldDef logging_options_fields[] = {
    {
        .keyword = "Log File",
        .label = "Log File",
        .help = "Path and filename for the main system log file. Maximus will record all caller activity, errors, and system events to this file based on the log level setting.",
        .type = FIELD_TEXT,
        .max_length = 80,
        .default_value = "etc/log/max.log"
    },
    {
        .keyword = "Log Mode",
        .label = "Log Level",
        .help = "Controls the amount of detail recorded in the log file. Terse: Basic call info only. Verbose: Detailed activity logging. Trace: Full debugging output including internal operations.",
        .type = FIELD_TOGGLE,
        .max_length = 10,
        .default_value = "Verbose",
        .toggle_options = log_level_options
    },
    {
        .keyword = "Uses Callers",
        .label = "Callers Log",
        .help = "Path to the caller information log file. This separate log records specific details about each caller session for statistical tracking and reporting purposes.",
        .type = FIELD_TEXT,
        .max_length = 80,
        .default_value = ""
    },
};

const int logging_options_field_count = sizeof(logging_options_fields) / sizeof(logging_options_fields[0]);

/* ============================================================================
 * Global Toggles (max.ctl System Section)
 * ============================================================================ */

const FieldDef global_toggles_fields[] = {
    {
        .keyword = "Snoop",
        .label = "Snoop",
        .help = "When enabled, allows the SysOp to view all caller activity on the local screen in real-time. Essential for monitoring user sessions and troubleshooting connection issues.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "Yes",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "No Password",
        .label = "Encrypt Passwords",
        .help = "When enabled (default), user passwords are stored using one-way encryption for security. Disable only if you need to recover forgotten passwords, but this is less secure.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "Yes",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "Reboot",
        .label = "Watchdog Reboot",
        .help = "When enabled, Maximus will automatically trigger a system reboot if a fatal error occurs or the system becomes unresponsive. Useful for unattended BBS operation.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "Swap",
        .label = "Swap to Disk",
        .help = "When enabled, Maximus swaps itself to disk/EMS when running external programs to free conventional memory. Essential for DOS systems with limited RAM. Not relevant for Unix.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "Local Input",
        .label = "Local Keyboard Timeout",
        .help = "When enabled, the local keyboard input will timeout after the configured period of inactivity, just like remote users. Prevents local sessions from running indefinitely.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "StatusLine",
        .label = "Status Line",
        .help = "When enabled, displays a status line at the bottom of the local screen showing current user info, time remaining, baud rate, and other session statistics.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "Yes",
        .toggle_options = toggle_yes_no
    },
};

const int global_toggles_field_count = sizeof(global_toggles_fields) / sizeof(global_toggles_fields[0]);

/* ============================================================================
 * Login Settings (max.ctl Session Section)
 * ============================================================================ */

const FieldDef login_settings_fields[] = {
    {
        .keyword = "Logon Level",
        .label = "New User Access Level",
        .help = "The privilege level automatically assigned to new users when they first register on the BBS. Common values: Disgrace, Limited, Normal, Worthy, Privil, Favored, Extra, Clerk.",
        .type = FIELD_SELECT,
        .max_length = 60,
        .default_value = "Normal",
        .toggle_options = access_level_options
    },
    {
        .keyword = "Logon TimeLimit",
        .label = "Logon Time Limit",
        .help = "Maximum number of minutes allowed for the login process before the user is disconnected. This prevents callers from tying up the line during login. Typical value: 5-10 minutes.",
        .type = FIELD_TEXT,
        .max_length = 5,
        .default_value = "5"
    },
    {
        .keyword = "Min Logon Baud",
        .label = "Minimum Logon Baud",
        .help = "Minimum connection speed (in bps) required to access the BBS. Callers connecting at slower speeds will see the TooSlow display file and be disconnected. Set to 0 for no restriction.",
        .type = FIELD_TEXT,
        .max_length = 10,
        .default_value = "0"
    },
    {
        .keyword = "Min NonTTY Baud",
        .label = "Min Graphics Baud",
        .help = "Minimum connection speed required for ANSI/AVATAR graphics. Users below this speed are automatically switched to TTY (plain text) mode for better performance.",
        .type = FIELD_TEXT,
        .max_length = 10,
        .default_value = "0"
    },
    {
        .keyword = "Min RIP Baud",
        .label = "Minimum RIP Baud",
        .help = "Minimum connection speed required for RIP (Remote Imaging Protocol) graphics. RIP requires significant bandwidth, so this should be higher than the graphics baud setting.",
        .type = FIELD_TEXT,
        .max_length = 10,
        .default_value = "0"
    },
    {
        .keyword = "Input Timeout",
        .label = "Input Timeout",
        .help = "Minutes of keyboard inactivity before the user is automatically disconnected. Prevents idle users from tying up phone lines. Typical values: 3-10 minutes.",
        .type = FIELD_TEXT,
        .max_length = 5,
        .default_value = "5"
    },
    {
        .keyword = "Check ANSI",
        .label = "Check ANSI on Login",
        .help = "When enabled, Maximus will ask new users if their terminal supports ANSI graphics during login. This determines whether color and cursor positioning codes are sent.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "Yes",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "Check RIP",
        .label = "Check RIP on Login",
        .help = "When enabled, Maximus will query the user's terminal for RIP (Remote Imaging Protocol) support during login. RIP enables graphical menus and icons.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
};

const int login_settings_field_count = sizeof(login_settings_fields) / sizeof(login_settings_fields[0]);

/* ============================================================================
 * New User Defaults (max.ctl Session Section)
 * ============================================================================ */

const FieldDef new_user_defaults_fields[] = {
    {
        .keyword = "Ask Phone",
        .label = "Ask for Phone Number",
        .help = "When enabled, new users will be prompted to enter their voice and/or data phone numbers during registration. Useful for SysOp callback verification.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "Yes",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "Ask Alias",
        .label = "Ask for Alias",
        .help = "When enabled, users are asked to provide both a real name and an alias (handle) during registration. The alias can be used as their primary name on the BBS if Alias System is enabled.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "Alias System",
        .label = "Alias System",
        .help = "When enabled, users can use their alias (handle) instead of their real name throughout the BBS. Messages can be posted under aliases, and other users see the alias.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "Single",
        .label = "Single Word Names",
        .help = "When enabled, users can register with single-word names (just a first name or alias). When disabled, Maximus requires a first and last name (two words minimum).",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "No RealName",
        .label = "No Real Name Required",
        .help = "When enabled, users are not required to provide their real name during registration. They can use only an alias. Be aware this reduces accountability on your system.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "First Menu",
        .label = "First Menu",
        .help = "The name of the menu file (without path or extension) that is displayed immediately after a successful login. This is typically your main menu. Example: main, top, welcome.",
        .type = FIELD_TEXT,
        .max_length = 20,
        .default_value = "main"
    },
    {
        .keyword = "First File Area",
        .label = "First File Area",
        .help = "The tag name of the file area that new users start in by default. This should be a general-purpose download area accessible to new users. Example: general, newfiles, main.",
        .type = FIELD_TEXT,
        .max_length = 20,
        .default_value = ""
    },
    {
        .keyword = "First Message Area",
        .label = "First Message Area",
        .help = "The tag name of the message area that new users start in by default. This is typically a general discussion or welcome area. Example: general, welcome, main.",
        .type = FIELD_TEXT,
        .max_length = 20,
        .default_value = ""
    },
};

const int new_user_defaults_field_count = sizeof(new_user_defaults_fields) / sizeof(new_user_defaults_fields[0]);

/* ============================================================================
 * Display Files (max.ctl General Filenames Section)
 * All files support MECCA .bbs or MEX .vm (prefix with :)
 * ============================================================================ */

const FieldDef display_files_fields[] = {
    /* ---- Login/Welcome Files ---- */
    {
        .keyword = "Uses Logo",
        .label = "Logo",
        .help = "First file shown to a caller immediately after Maximus connects. Should contain a small amount of information describing your BBS such as the sysop name and system info. This file must NOT contain ANSI or AVATAR graphics since terminal type is unknown at this point.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/logo",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses NotFound",
        .label = "Not Found",
        .help = "Displayed to a new user after their name is entered but before the 'First Last [Y,n]?' confirmation prompt. Use this to welcome potential new users and explain what happens next in the registration process.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/notfound",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses Application",
        .label = "Application",
        .help = "New user questionnaire displayed after the user confirms their name with 'Y' to 'Firstname Lastname [Y,n]?' but before prompting for city and phone number. Use this to explain system rules or gather additional registration info.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/applic",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses Welcome",
        .label = "Welcome",
        .help = "Displayed to normal users who have called more than eight times. This file is shown immediately after the user enters their log-on password. This is your main welcome screen for regular callers.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/welcome",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses NewUser1",
        .label = "New User 1",
        .help = "Displayed to a new user right before Maximus asks them to enter a password. Use this to explain password requirements such as maximum length, no spaces allowed, and the importance of choosing a secure password.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/newuser1",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses NewUser2",
        .label = "New User 2",
        .help = "Displayed to a new user in lieu of the Welcome file. Often contains the same content as Welcome or similar to Application. This allows you to show different content to brand new users versus returning callers.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/newuser2",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses Rookie",
        .label = "Rookie",
        .help = "Displayed to users who have called between two and eight times, in lieu of the Welcome file. Use this to provide extra guidance to newer users who are still learning the system. F3=disable to use Welcome instead.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = true,
        .supports_mex = true
    },
    {
        .keyword = "Uses Configure",
        .label = "Configure",
        .help = "Displayed to new users after they log in but before standard user configuration questions are asked. If MEX sets the 'configured' bit in the user record, standard config questions are skipped, allowing custom new user setup. F3=disable.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = true,
        .supports_mex = true
    },
    /* ---- System Files ---- */
    {
        .keyword = "Uses Quote",
        .label = "Quotes",
        .help = "ASCII text file containing quotes and random pieces of wisdom. Each quote should be separated by a single blank line. Access quotes in your .bbs files using the MECCA [quote] token which displays a random selection.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/quotes",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses DayLimit",
        .label = "Day Limit",
        .help = "Displayed to users who try to log on after having exceeded their daily time limits. Should inform the user they have used all their time for today and when they can call back.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/daylimit",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses TimeWarn",
        .label = "Time Warning",
        .help = "Displayed to users just before the main menu as long as they have made more than one call on the current day. Use this to warn users about remaining time or upcoming system events.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/timewarn",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses TooSlow",
        .label = "Too Slow",
        .help = "Displayed to users whose connection speed is lower than the minimum required in Min Logon Baud, or if their speed is less than the LogonBaud keyword for their user class in the access control file.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/tooslow",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses ByeBye",
        .label = "Goodbye",
        .help = "Displayed to users after they select the Goodbye menu option. This is your farewell screen - use it to thank users for calling and remind them of upcoming events or new files.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/byebye",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses BadLogon",
        .label = "Bad Logon",
        .help = "Displayed to users who failed their last log-on attempt due to an invalid password. Use this to warn about security, explain password recovery options, or inform about lockout policies.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/badlogon",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses Barricade",
        .label = "Barricade",
        .help = "Displayed to users after they enter a barricaded message or file area but before they are prompted for the access password. Explain what the area contains and how to obtain access if needed.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/barricad",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses NoSpace",
        .label = "No Space",
        .help = "Displayed when the amount of free space on the upload drive is less than the value specified by the 'Upload Space Free' keyword. Informs users that uploads are temporarily disabled due to disk space.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/nospace",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses NoMail",
        .label = "No Mail",
        .help = "Displayed to users after the [msg_checkmail] MECCA token determines there is no mail waiting for them. Can suggest they check message areas or explain mail forwarding options.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/nomail",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses Cant_Enter_Area",
        .label = "Can't Enter Area",
        .help = "Displayed when users try to select an area that does not exist or they lack access to. Replaces the default 'That area does not exist!' message. Use to suggest valid areas. F3=disable for default.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = true,
        .supports_mex = true
    },
    /* ---- Chat/Shell Files ---- */
    {
        .keyword = "Uses BeginChat",
        .label = "Begin Chat",
        .help = "Displayed to the user when the SysOp enters CHAT mode. This is a good place for a greeting like 'Hi [user], this is the SysOp speaking.' Default message if not set is 'CHAT: start'.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/begchat",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses EndChat",
        .label = "End Chat",
        .help = "Displayed to the user when the SysOp exits chat mode. Use this to indicate the chat session has ended and normal BBS operation is resuming. Default message if not set is 'END CHAT'.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/endchat",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses Leaving",
        .label = "Leaving",
        .help = "Displayed just before Maximus exits to run an external program invoked from a menu option. Use this to inform users they are about to enter a door or external application.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/leaving",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses Returning",
        .label = "Returning",
        .help = "Displayed to the user upon returning from an external program invoked by a menu option. Welcome users back to the BBS and remind them where they were before the door.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/return",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses Shell_Leaving",
        .label = "Shell Leaving",
        .help = "Displayed to the user immediately after the SysOp presses Alt-J on the local console to shell to the operating system. Inform users the SysOp is temporarily away from the keyboard.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/shleave",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses Shell_Returning",
        .label = "Shell Returning",
        .help = "Displayed to the user after the SysOp returns from an Alt-J shell to the operating system. Let users know the SysOp is back and normal operation has resumed.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/shreturn",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    /* ---- Help Files ---- */
    {
        .keyword = "Uses LocateHelp",
        .label = "Locate Help",
        .help = "Displayed to users who request help using the File_Locate command. Explain how to search for files by name, date, or description and what wildcards are supported.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/lochelp",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses ContentsHelp",
        .label = "Contents Help",
        .help = "Displayed to users who request help for the File_Contents command. Explain how to view file descriptions, what information is shown, and how to navigate the listing.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/conthelp",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses MaxEdHelp",
        .label = "MaxEd Help",
        .help = "Displayed to users who ask for help by pressing Ctrl-K ? from within the MaxEd full-screen editor. Document all editor commands, cursor movement, and text manipulation keys.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/maxedhlp",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses BoredHelp",
        .label = "Line Editor Help",
        .help = "Displayed to first-time callers who enter the BORED line editor when their help level is set to novice. Provide a gentle introduction to the editor commands and how to save or abort.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/borehelp",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses ReplaceHelp",
        .label = "Replace Help",
        .help = "Displayed to users just after selecting the Edit_Edit option on the editor menu. Describe the search and replace feature of the line editor including pattern syntax.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/replhelp",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses InquireHelp",
        .label = "Inquire Help",
        .help = "Displayed to users requesting help with the Message Inquire command. Explain how to view and modify message attributes such as private, crash, file attach, and kill/sent flags.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/inqhelp",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses ScanHelp",
        .label = "Scan Help",
        .help = "Displayed to users requesting help with the message Scan command. Explain how to scan for new messages, personal mail, and how to set scan pointers and filters.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/scanhelp",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses ListHelp",
        .label = "List Help",
        .help = "Displayed to users requesting help with the file List command. Document listing options, sorting methods, and how to navigate through large file listings efficiently.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/listhelp",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses HeaderHelp",
        .label = "Header Help",
        .help = "Displayed to users just before the message header entry screen. Provide information regarding message attributes, using aliases, anonymous posting areas, and addressing options.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/hdrhelp",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses EntryHelp",
        .label = "Entry Help",
        .help = "Displayed to the user just before entering the message editor, for both full-screen and line editor. Can offer additional help or set up screen display for RIPscrip callers.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/enthelp",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    /* ---- Area/Protocol Files ---- */
    {
        .keyword = "Uses XferBaud",
        .label = "Transfer Baud",
        .help = "Displayed to users whose connection speed is less than the speed required for the XferBaud setting for their user class in the access control file. Explain why file transfers are restricted at lower speeds.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "etc/misc/xferbaud",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = false,
        .supports_mex = true
    },
    {
        .keyword = "Uses FileAreas",
        .label = "File Areas",
        .help = "Displayed when a user requests a file area listing. This custom display file replaces the automatically-generated file area list. Use MECCA tokens for dynamic content. F3=disable for auto-generated list.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = true,
        .supports_mex = true
    },
    {
        .keyword = "Uses MsgAreas",
        .label = "Msg Areas",
        .help = "Displayed when a user requests a message area listing. This custom display file replaces the automatically-generated message area list. Use MECCA tokens for dynamic content. F3=disable for auto-generated list.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = true,
        .supports_mex = true
    },
    {
        .keyword = "Uses ProtocolDump",
        .label = "Protocol Dump",
        .help = "Displayed to the user instead of the standard 'canned' list of protocol names. This file is shown for both File_Upload and File_Download menu options. Use to customize protocol presentation. F3=disable for built-in list.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = true,
        .supports_mex = true
    },
    {
        .keyword = "Uses Filename_Format",
        .label = "Filename Format",
        .help = "Displayed to users who try to upload files using an invalid filename. Use this to explain MS-DOS 8.3 filename restrictions, valid characters, and naming conventions for uploads. F3=disable for default message.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = true,
        .supports_mex = true
    },
    {
        .keyword = "Uses Tunes",
        .label = "Tunes",
        .help = "Specifies the Maximus tunes file for playing simple melodies on the PC speaker when a user yells for the SysOp. Format: '* TuneName' followed by frequency/duration pairs. See tunes.bbs for examples. F3=disable.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = true,
        .supports_mex = false
    },
};

const int display_files_field_count = sizeof(display_files_fields) / sizeof(display_files_fields[0]);

/* ============================================================================
 * Message Division (msgarea.ctl MsgDivisionBegin)
 * Syntax: MsgDivisionBegin <name> <acs> <display_file> <desc>
 * See: s_marea.c ParseMsgDivisionBegin() - only 4 parameters
 * ============================================================================ */

/* Access level options for F2 picklist */
const char *access_level_options[] = {
    "Transient", "Demoted", "Limited", "Normal", "Worthy", "Privil",
    "Favored", "Extra", "Clerk", "AsstSysop", "Sysop", NULL
};

const FieldDef msg_division_fields[] = {
    {
        .keyword = "Name",
        .label = "Division Name",
        .help = "Short tag for this division. Prefixed to all area names within. "
                "Example: 'cars' makes area 'lexus' become 'cars.lexus'. No dots allowed.",
        .type = FIELD_TEXT,
        .max_length = 32,
        .default_value = "",
        .toggle_options = NULL
    },
    {
        .keyword = "ParentDivision",
        .label = "Parent Division",
        .help = "Parent division for nesting. Divisions can be nested multiple levels deep. "
                "Press F2 to select from available divisions. (None) = top level.",
        .type = FIELD_SELECT,
        .max_length = 40,
        .default_value = "(None)",
        .toggle_options = msg_division_options
    },
    {
        .keyword = "Description",
        .label = "Description",
        .help = "Description shown on the message area menu when browsing divisions. "
                "Keep under 60 characters for proper display.",
        .type = FIELD_TEXT,
        .max_length = 60,
        .default_value = "",
        .toggle_options = NULL
    },
    {
        .keyword = "DisplayFile",
        .label = "Display File",
        .help = "Custom .bbs file shown when user requests area list of this division. "
                "Only used if 'Uses MsgAreas' is enabled. Specify '.' for none. F2=Browse",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = ".",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = true,
        .supports_mex = true
    },
    {
        .keyword = "ACS",
        .label = "Base Access Level",
        .help = "Access level required to see this division. Note: independent of ACS "
                "for contained areas. F2=Select from list, or type ACS expression.",
        .type = FIELD_SELECT,
        .max_length = 11,
        .default_value = "Demoted",
        .toggle_options = access_level_options
    },
};

const int msg_division_field_count = sizeof(msg_division_fields) / sizeof(msg_division_fields[0]);

/* ============================================================================
 * Message Area (msgarea.ctl MsgArea)
 * Keywords from s_marea.c ParseMsgArea(): acs, path, tag, desc, origin,
 *   menuname, override, style, renum, barricade, attachpath, owner
 * Style flags: Pvt, Pub, HiBit, Net, Echo, Conf, Anon, NoNameKludge,
 *   RealName, Alias, Audit, ReadOnly, Hidden, Attach, NoMailCheck, Squish, *.MSG
 * ============================================================================ */

/* Select options for message areas */
const char *msg_format_options[] = { "Squish", "*.MSG", NULL };
const char *msg_type_options[] = { "Local", "NetMail", "EchoMail", "Conference", NULL };
const char *msg_name_style_options[] = { "Real Name", "Alias", "Either", NULL };

/* Division options - populated dynamically at runtime */
const char *msg_division_options[16] = { "(None)", NULL };

const FieldDef msg_area_fields[] = {
    /* ---- Group 1: Basic identification ---- */
    {
        .keyword = "MsgArea",
        .label = "Area Name",
        .help = "Unique name for this area. If inside a division, division name is "
                "automatically prefixed (e.g., 'cars.lexus'). No dots in the name itself.",
        .type = FIELD_TEXT,
        .max_length = 40,
        .default_value = ""
    },
    {
        .keyword = "Division",
        .label = "Division",
        .help = "Parent division for this area. Areas inherit the division prefix in their "
                "name. Press F2 to select from available divisions. (None) = top level.",
        .type = FIELD_SELECT,
        .max_length = 40,
        .default_value = "(None)",
        .toggle_options = msg_division_options
    },
    {
        .keyword = "Tag",
        .label = "Short Name",
        .help = "EchoMail tag for ECHOTOSS.LOG. Match your tosser config (squish.cfg). "
                "Only needed for Echo/Conf areas. Example: CARS_LEXUS",
        .type = FIELD_TEXT,
        .max_length = 40,
        .default_value = ""
    },
    {
        .keyword = "Path",
        .label = "Mail path/file",
        .help = "Squish: path+basename (no .SQD). *.MSG: directory path with trailing slash. "
                "Example: spool/msgbase/public or /var/max/msg/cars.lexus",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = ""
    },
    {
        .keyword = "Desc",
        .label = "Description",
        .help = "Description shown on message area menu. Displayed when user browses areas. "
                "Keep under 60 characters for proper formatting.",
        .type = FIELD_TEXT,
        .max_length = 60,
        .default_value = ""
    },
    {
        .keyword = "Owner",
        .label = "Owner",
        .help = "Default owner for message tracking (MAX_TRACKER). Messages without explicit "
                "owner assigned will be owned by this user. Leave blank for none.",
        .type = FIELD_TEXT,
        .max_length = 35,
        .default_value = ""
    },

    /* Separator before Format/Type group */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 2: Format and Type (spacebar/F2 cycles) ---- */
    {
        .keyword = "Style_Format",
        .label = "Format",
        .help = "Squish: Modern indexed format with Audit/Attach support. "
                "*.MSG: FidoNet-compatible, one file per message. SPACE/F2 to change.",
        .type = FIELD_SELECT,
        .max_length = 10,
        .default_value = "Squish",
        .toggle_options = msg_format_options
    },
    {
        .keyword = "Style_Type",
        .label = "Type",
        .help = "Local: stays on BBS. NetMail: point-to-point FidoNet. "
                "EchoMail: broadcast with origin. Conference: broadcast with PID. SPACE/F2.",
        .type = FIELD_SELECT,
        .max_length = 10,
        .default_value = "Local",
        .toggle_options = msg_type_options
    },
    {
        .keyword = "Style_Name",
        .label = "Name style",
        .help = "Real Name: force real names (MA_REAL). Alias: force aliases (MA_ALIAS). "
                "Either: user chooses. Controls 'From:' field. SPACE/F2 to change.",
        .type = FIELD_SELECT,
        .max_length = 10,
        .default_value = "Real Name",
        .toggle_options = msg_name_style_options
    },

    /* Separator before Style toggles */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 3: Style toggles (2 columns, Yes/No) ---- */
    /* Row 1 */
    {
        .keyword = "Style_Pvt",
        .label = "Private allowed",
        .help = "Allow private messages (MA_PVT). Private msgs readable only by sender, "
                "recipient, and sysop. Can enable both Private and Public.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no,
        .pair_with_next = true
    },
    {
        .keyword = "Style_Pub",
        .label = "Public allowed",
        .help = "Allow public messages (MA_PUB). Public msgs readable by anyone with area "
                "access. Can enable both Private and Public. Default is Public only.",
        .type = FIELD_TOGGLE,
        .default_value = "Yes",
        .toggle_options = toggle_yes_no
    },
    /* Row 2 */
    {
        .keyword = "Style_HiBit",
        .label = "High-bit chars",
        .help = "Allow 8-bit extended ASCII (MA_HIBIT). Required for ANSI art, international "
                "characters, or CP437 graphics. Disable for 7-bit clean areas.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no,
        .pair_with_next = true
    },
    {
        .keyword = "Style_Anon",
        .label = "Anonymous OK",
        .help = "Allow anonymous posting (MA_ANON). User can modify From field. Real name "
                "still added as kludge unless NoNameKludge is also set.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    /* Row 3 */
    {
        .keyword = "Style_NoRNK",
        .label = "No name kludge",
        .help = "Don't add ^aREALNAME kludge (MA_NORNK). With Anonymous, truly hides identity. "
                "Without this, real name is embedded even in anonymous posts.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no,
        .pair_with_next = true
    },
    {
        .keyword = "Style_Audit",
        .label = "Audit trail",
        .help = "Enable message tracking/auditing (MA_AUDIT). Squish only. Tracks who read "
                "messages and allows ownership assignment. Useful for support areas.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    /* Row 4 */
    {
        .keyword = "Style_ReadOnly",
        .label = "Read only",
        .help = "Make area read-only (MA_READONLY). Only users with WriteRdOnly class flag "
                "can post. Useful for announcements or archived discussions.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no,
        .pair_with_next = true
    },
    {
        .keyword = "Style_Hidden",
        .label = "Hidden",
        .help = "Hide from area list (MA_HIDDN). Area not shown in normal listings, skipped "
                "by navigation. Can still be accessed directly by name.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    /* Row 5 */
    {
        .keyword = "Style_Attach",
        .label = "File attach",
        .help = "Allow local file attaches (MA_ATTACH). Squish only. Users can attach files "
                "to messages. Requires AttachPath to be set.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no,
        .pair_with_next = true
    },
    {
        .keyword = "Style_NoMailChk",
        .label = "Skip mail check",
        .help = "Skip in personal mail check (MA2_NOMCHK). High-volume areas that never "
                "contain personal mail. Speeds up login mail scan.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },

    /* Separator before Renum group */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 4: Renum/Purge settings ---- */
    {
        .keyword = "Renum_Max",
        .label = "Max messages",
        .help = "Maximum messages to keep (killbynum). MECCA or manual renumber purges "
                "oldest when exceeded. 0 = no limit. Typical: 100-500.",
        .type = FIELD_NUMBER,
        .max_length = 5,
        .default_value = "0",
        .toggle_options = NULL
    },
    {
        .keyword = "Renum_Days",
        .label = "Max age (days)",
        .help = "Maximum message age in days (killbyage). Messages older are purged. "
                "0 = no age limit. Works with Max messages. Typical: 30-180.",
        .type = FIELD_NUMBER,
        .max_length = 4,
        .default_value = "0",
        .toggle_options = NULL
    },
    {
        .keyword = "Renum_Skip",
        .label = "Skip first",
        .help = "Exempt first N messages from purging (killskip). Protects sticky posts "
                "or important announcements at top of area. 0 = none exempt.",
        .type = FIELD_NUMBER,
        .max_length = 4,
        .default_value = "0",
        .toggle_options = NULL
    },

    /* ---- Group 5: Access Control ---- */
    {
        .keyword = "ACS",
        .label = "Access (ACS)",
        .help = "Access Control String for this area. F2=pick level. Examples: 'Demoted', "
                "'Normal', 'Privil/K1' (Privil + key 1), 'Sysop'. Complex: 'Worthy/100'.",
        .type = FIELD_SELECT,
        .max_length = 60,
        .default_value = "Demoted",
        .toggle_options = access_level_options
    },

    /* Separator before Origin group */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 6: Origin (for Echo/Net areas) ---- */
    {
        .keyword = "Origin_Addr",
        .label = "Primary address",
        .help = "FidoNet address for origin line (zone:net/node.point). Used as source "
                "address for EchoMail/NetMail. Format: 1:234/567 or 1:234/567.0",
        .type = FIELD_TEXT,
        .max_length = 24,
        .default_value = ""
    },
    {
        .keyword = "Origin_SeenBy",
        .label = "SeenBy address",
        .help = "Address to use in SEEN-BY lines. Usually same as primary or your hub's "
                "address. Format: zone:net/node.point. Leave blank to use primary.",
        .type = FIELD_TEXT,
        .max_length = 24,
        .default_value = ""
    },
    {
        .keyword = "Origin_Line",
        .label = "Origin line",
        .help = "Custom origin text (max 60 chars). Appended after ' * Origin: ' in echomail. "
                "Your FidoNet address is added automatically. Leave blank for system default.",
        .type = FIELD_TEXT,
        .max_length = 60,
        .default_value = ""
    },

    /* Separator before Advanced group */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 7: Advanced/Barricade ---- */
    {
        .keyword = "Barricade_Menu",
        .label = "Barricade menu",
        .help = "Menu name where barricade priv applies. The barricade access level is only "
                "enforced while user is in this menu. Blank = all menus.",
        .type = FIELD_TEXT,
        .max_length = 13,
        .default_value = "",
        .toggle_options = NULL
    },
    {
        .keyword = "Barricade_File",
        .label = "Barricade file",
        .help = "Path to barricade file containing access overrides. Allows per-area privilege "
                "adjustments. Requires Barricade menu to be set.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "",
        .file_filter = "*",
        .file_base_path = "",
        .can_disable = true,
        .supports_mex = false
    },
    {
        .keyword = "MenuName",
        .label = "Custom menu",
        .help = "Use this menu file instead of default when in area. Press F2 to browse "
                "available menus. Path relative to menus directory.",
        .type = FIELD_FILE,
        .max_length = 60,
        .default_value = "",
        .file_filter = "*.mnu",
        .file_base_path = "m",
        .can_disable = true,
        .supports_mex = false
    },
    {
        .keyword = "MenuReplace",
        .label = "Replace menu",
        .help = "Replace this menu name with Custom menu above. Press F2 to browse. "
                "Only this specific menu is replaced.",
        .type = FIELD_FILE,
        .max_length = 60,
        .default_value = "",
        .file_filter = "*.mnu",
        .file_base_path = "m",
        .can_disable = true,
        .supports_mex = false
    },
    {
        .keyword = "AttachPath",
        .label = "Attach path",
        .help = "Directory for local file attaches. Required if File attach is enabled. "
                "Files attached to messages are stored/retrieved here.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "",
        .toggle_options = NULL
    },
};

const int msg_area_field_count = sizeof(msg_area_fields) / sizeof(msg_area_fields[0]);

/* ============================================================================
 * File Division (filearea.ctl FileDivisionBegin/End)
 * Similar structure to message divisions
 * ========================================================================== */

/* Options array for file division parent selection - populated at runtime */
const char *file_division_options[16] = { "(None)", NULL };

const FieldDef file_division_fields[] = {
    {
        .keyword = "Name",
        .label = "Division Name",
        .help = "Short tag for this file division. Prefixed to all area names within. "
                "Example: 'games' makes area 'doom' become 'games.doom'. No dots allowed.",
        .type = FIELD_TEXT,
        .max_length = 32,
        .default_value = "",
        .toggle_options = NULL
    },
    {
        .keyword = "ParentDivision",
        .label = "Parent Division",
        .help = "Parent division for nesting. Divisions can be nested multiple levels deep. "
                "Press F2 to select from available divisions. (None) = top level.",
        .type = FIELD_SELECT,
        .max_length = 40,
        .default_value = "(None)",
        .toggle_options = file_division_options
    },
    {
        .keyword = "Description",
        .label = "Description",
        .help = "Description shown on the file area menu when browsing divisions. "
                "Keep under 60 characters for proper display.",
        .type = FIELD_TEXT,
        .max_length = 60,
        .default_value = "",
        .toggle_options = NULL
    },
    {
        .keyword = "DisplayFile",
        .label = "Display File",
        .help = "Custom .bbs file shown when user requests area list of this division. "
                "Specify '.' for none. F2=Browse",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = ".",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = true,
        .supports_mex = true
    },
    {
        .keyword = "ACS",
        .label = "Base Access Level",
        .help = "Access level required to see this division. Note: independent of ACS "
                "for contained areas. F2=Select from list, or type ACS expression.",
        .type = FIELD_SELECT,
        .max_length = 11,
        .default_value = "Demoted",
        .toggle_options = access_level_options
    },
};

const int file_division_field_count = sizeof(file_division_fields) / sizeof(file_division_fields[0]);

/* ============================================================================
 * File Area (filearea.ctl FileArea)
 * Keywords from s_farea.c ParseFileArea(): acs, download, upload, filesbbs,
 *   desc, menuname, override, barricade
 * ========================================================================== */

/* Date style options for file areas */
static const char *file_date_style_options[] = {
    "Default", "Auto", "Manual", "List", NULL
};

const FieldDef file_area_fields[] = {
    /* ---- Group 1: Basic info ---- */
    {
        .keyword = "FileArea",
        .label = "Area tag",
        .help = "Unique tag for this file area. Used as directory reference and in logs. "
                "No spaces or dots. Example: 'GAMES_DOOM' or 'UTILS_ZIP'",
        .type = FIELD_TEXT,
        .max_length = 40,
        .default_value = ""
    },
    {
        .keyword = "Division",
        .label = "Division",
        .help = "Division this area belongs to. Press F2 to select from available divisions. "
                "(None) = top level area not in any division.",
        .type = FIELD_SELECT,
        .max_length = 40,
        .default_value = "(None)",
        .toggle_options = file_division_options
    },
    {
        .keyword = "Desc",
        .label = "Description",
        .help = "Description shown on file area menu. Displayed when user browses areas. "
                "Keep under 60 characters for proper formatting.",
        .type = FIELD_TEXT,
        .max_length = 60,
        .default_value = ""
    },

    /* Separator before paths */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 2: Paths ---- */
    {
        .keyword = "Download",
        .label = "Download path",
        .help = "Directory where downloadable files are stored. Users can download from here. "
                "Example: /var/max/files/games/doom",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = ""
    },
    {
        .keyword = "Upload",
        .label = "Upload path",
        .help = "Directory where uploaded files are placed. Can be same as download path. "
                "Leave blank to use download path. Example: /var/max/upload",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = ""
    },
    {
        .keyword = "FilesBbs",
        .label = "FILES.BBS path",
        .help = "Path to FILES.BBS catalog file for this area. Contains file descriptions. "
                "Leave blank to auto-generate from download path.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "",
        .file_filter = "*",
        .file_base_path = "",
        .can_disable = true,
        .supports_mex = false
    },

    /* Separator before flags */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 3: Flags ---- */
    {
        .keyword = "DateStyle",
        .label = "Date style",
        .help = "File date display: Default=system setting, Auto=file timestamp, "
                "Manual=FILES.BBS date, List=from file list. SPACE/F2 to change.",
        .type = FIELD_SELECT,
        .max_length = 10,
        .default_value = "Default",
        .toggle_options = file_date_style_options
    },
    {
        .keyword = "Slow",
        .label = "Slow media",
        .help = "Slow-access medium like CD-ROM (FA_SLOW). Skips file existence checks "
                "to improve performance. Combine with Staged for CD-ROM.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no,
        .pair_with_next = true
    },
    {
        .keyword = "Staged",
        .label = "Staged downloads",
        .help = "Use staged transfer area for downloads (FA_STAGED). Files copied to temp "
                "directory before sending. Useful for CD-ROM or slow media.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "NoNew",
        .label = "Skip new check",
        .help = "Permanent storage - skip for new file searches (FA_NONEW). Use for areas "
                "that don't get new files, like CD-ROM archives.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no,
        .pair_with_next = true
    },
    {
        .keyword = "Hidden",
        .label = "Hidden",
        .help = "Area does not display on normal area list (FA_HIDDN). Hidden areas can "
                "still be accessed directly by users who know the area name.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "FreeTime",
        .label = "Free time",
        .help = "Downloads don't count against daily time limits (FA_FREETIME). User's "
                "remaining time is not reduced while downloading from this area.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no,
        .pair_with_next = true
    },
    {
        .keyword = "FreeBytes",
        .label = "Free bytes",
        .help = "Downloads don't count against daily byte limits (FA_FREESIZE). User's "
                "remaining download quota is not reduced for this area.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "NoIndex",
        .label = "No index",
        .help = "Don't add this area to maxfiles.idx (FA_NOINDEX). Files won't appear "
                "in global file searches. Use for private or temp areas.",
        .type = FIELD_TOGGLE,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },

    /* Separator before access */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 4: Access ---- */
    {
        .keyword = "ACS",
        .label = "Base Access Level",
        .help = "Access level required to enter this file area. F2=Select from list, "
                "or type custom ACS expression.",
        .type = FIELD_SELECT,
        .max_length = 11,
        .default_value = "Demoted",
        .toggle_options = access_level_options
    },

    /* Separator before advanced */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 5: Advanced ---- */
    {
        .keyword = "Barricade_Menu",
        .label = "Barricade menu",
        .help = "Menu name where barricade priv applies. The barricade access level is only "
                "enforced while user is in this menu. Blank = all menus.",
        .type = FIELD_TEXT,
        .max_length = 13,
        .default_value = ""
    },
    {
        .keyword = "Barricade_File",
        .label = "Barricade file",
        .help = "Path to barricade file containing access overrides. Allows per-area privilege "
                "adjustments. Requires Barricade menu to be set.",
        .type = FIELD_FILE,
        .max_length = 80,
        .default_value = "",
        .file_filter = "*",
        .file_base_path = "",
        .can_disable = true,
        .supports_mex = false
    },
    {
        .keyword = "MenuName",
        .label = "Custom menu",
        .help = "Use this menu file instead of default when in area. Press F2 to browse "
                "available menus. Path relative to menus directory.",
        .type = FIELD_FILE,
        .max_length = 60,
        .default_value = "",
        .file_filter = "*.mnu",
        .file_base_path = "m",
        .can_disable = true,
        .supports_mex = false
    },
    {
        .keyword = "MenuReplace",
        .label = "Replace menu",
        .help = "Replace this menu name with Custom menu above. Press F2 to browse. "
                "Only this specific menu is replaced.",
        .type = FIELD_FILE,
        .max_length = 60,
        .default_value = "",
        .file_filter = "*.mnu",
        .file_base_path = "m",
        .can_disable = true,
        .supports_mex = false
    },
};

const int file_area_field_count = sizeof(file_area_fields) / sizeof(file_area_fields[0]);

/* ============================================================================
 * Access/Security Level (access.ctl Access)
 * Keywords from s_access.c: Level, Desc, Alias, Key, Time, Cume, Calls,
 *   LogonBaud, XferBaud, FileLimit, FileRatio, RatioFree, UploadReward,
 *   LoginFile, Flags, MailFlags, UserFlags, Oldpriv
 * ========================================================================== */

/* Flag options for access levels */
static const char *access_flags_options[] = {
    "Hangup", "Hide", "ShowHidden", "ShowAllFiles", "DloadHidden",
    "UploadAny", "NoFileLimit", "NoTimeLimit", "NoLimits", NULL
};

static const char *mail_flags_options[] = {
    "ShowPvt", "Editor", "LocalEditor", "NetFree", "MsgAttrAny",
    "WriteRdOnly", "NoRealName", NULL
};

const FieldDef access_level_fields[] = {
    /* ---- Group 1: Identity ---- */
    {
        .keyword = "Access",
        .label = "Access Name",
        .help = "Symbolic name for this access level. Used in ACS expressions. "
                "Must start with letter, no spaces. Example: 'Normal', 'Sysop'",
        .type = FIELD_TEXT,
        .max_length = 16,
        .default_value = ""
    },
    {
        .keyword = "Level",
        .label = "Level Number",
        .help = "Numeric priority (0-65535). Higher = more access. Used to compare "
                "privilege levels. Must be unique across all access levels.",
        .type = FIELD_NUMBER,
        .max_length = 5,
        .default_value = "0"
    },
    {
        .keyword = "Desc",
        .label = "Description",
        .help = "Human-readable description shown in user editor and status displays. "
                "Can contain spaces and punctuation.",
        .type = FIELD_TEXT,
        .max_length = 40,
        .default_value = ""
    },
    {
        .keyword = "Alias",
        .label = "Alias",
        .help = "Alternate symbolic name for this level. Can be used interchangeably "
                "with the access name in ACS expressions.",
        .type = FIELD_TEXT,
        .max_length = 16,
        .default_value = ""
    },
    {
        .keyword = "Key",
        .label = "Key Letter",
        .help = "Single character for MECCA compatibility tokens like [?below] [?above]. "
                "Defaults to first letter of access name if not specified.",
        .type = FIELD_TEXT,
        .max_length = 1,
        .default_value = ""
    },

    /* Separator before time limits */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 2: Time Limits ---- */
    {
        .keyword = "Time",
        .label = "Session time",
        .help = "Maximum minutes per session. User is warned and logged off when "
                "time expires. Leave blank or 0 for unlimited.",
        .type = FIELD_NUMBER,
        .max_length = 5,
        .default_value = "60",
        .pair_with_next = true
    },
    {
        .keyword = "Cume",
        .label = "Daily time",
        .help = "Maximum total minutes per day across all sessions. Cumulative "
                "time tracking. Leave blank or 0 for unlimited.",
        .type = FIELD_NUMBER,
        .max_length = 5,
        .default_value = "90"
    },
    {
        .keyword = "Calls",
        .label = "Daily calls",
        .help = "Maximum calls per day. Use -1 for unlimited calls. User cannot "
                "log in again after reaching limit until next day.",
        .type = FIELD_NUMBER,
        .max_length = 5,
        .default_value = "-1"
    },

    /* Separator before file limits */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 3: Download Limits ---- */
    {
        .keyword = "FileLimit",
        .label = "Download limit (KB)",
        .help = "Maximum kilobytes user can download per day. 0 = no downloads. "
                "Use NoFileLimit flag to bypass this for special users.",
        .type = FIELD_NUMBER,
        .max_length = 8,
        .default_value = "5000",
        .pair_with_next = true
    },
    {
        .keyword = "FileRatio",
        .label = "File ratio",
        .help = "Download:Upload ratio required. e.g. 5 means 5:1 ratio - user must "
                "upload 1KB for every 5KB downloaded. 0 = no ratio enforced.",
        .type = FIELD_NUMBER,
        .max_length = 5,
        .default_value = "0"
    },
    {
        .keyword = "RatioFree",
        .label = "Ratio-free (KB)",
        .help = "Kilobytes user can download before ratio is enforced. Allows new "
                "users to download some files before needing to upload.",
        .type = FIELD_NUMBER,
        .max_length = 8,
        .default_value = "1000",
        .pair_with_next = true
    },
    {
        .keyword = "UploadReward",
        .label = "Upload reward %",
        .help = "Percent of upload time credited back. 100% = time spent uploading "
                "is not deducted. 200% = earn 2x time back. 0% = no credit.",
        .type = FIELD_NUMBER,
        .max_length = 5,
        .default_value = "100"
    },

    /* Separator before baud limits */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 4: Baud Rate Limits ---- */
    {
        .keyword = "LogonBaud",
        .label = "Min logon baud",
        .help = "Minimum baud rate required to log in. Users connecting at lower "
                "speeds are denied access. 0 or 300 = any speed allowed.",
        .type = FIELD_NUMBER,
        .max_length = 6,
        .default_value = "300",
        .pair_with_next = true
    },
    {
        .keyword = "XferBaud",
        .label = "Min xfer baud",
        .help = "Minimum baud rate required for file transfers. Users at lower "
                "speeds cannot download files. 0 or 300 = any speed allowed.",
        .type = FIELD_NUMBER,
        .max_length = 6,
        .default_value = "300"
    },

    /* Separator before flags */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 5: Login File ---- */
    {
        .keyword = "LoginFile",
        .label = "Login display",
        .help = "File displayed immediately after user logs in. Path relative to "
                "Misc directory. Useful for level-specific announcements.",
        .type = FIELD_FILE,
        .max_length = 60,
        .default_value = "",
        .file_filter = "*.bbs",
        .file_base_path = "etc/misc",
        .can_disable = true,
        .supports_mex = true
    },

    /* Separator before flags */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 6: Permission Flags ---- */
    {
        .keyword = "Flags",
        .label = "System flags",
        .help = "Permission flags for this access level. Press ENTER or F2 to "
                "open the multi-select picker. SPACE to toggle individual flags.",
        .type = FIELD_MULTISELECT,
        .max_length = 80,
        .default_value = "",
        .toggle_options = access_flags_options
    },
    {
        .keyword = "MailFlags",
        .label = "Mail flags",
        .help = "Mail permission flags for this access level. Press ENTER or F2 "
                "to open the multi-select picker. SPACE to toggle individual flags.",
        .type = FIELD_MULTISELECT,
        .max_length = 80,
        .default_value = "",
        .toggle_options = mail_flags_options
    },
    {
        .keyword = "UserFlags",
        .label = "Custom flags",
        .help = "Custom 32-bit flags for MEX scripts. Each bit (0-31) can be "
                "tested in scripts. Enter as decimal or hex (0x prefix).",
        .type = FIELD_TEXT,
        .max_length = 12,
        .default_value = "0"
    },

    /* Separator before compatibility */
    { .type = FIELD_SEPARATOR },

    /* ---- Group 7: Compatibility ---- */
    {
        .keyword = "Oldpriv",
        .label = "Legacy priv",
        .help = "Maximus 2.x compatibility value. Copy from adjacent level if "
                "adding new levels. Not used by Maximus 3.x directly.",
        .type = FIELD_NUMBER,
        .max_length = 5,
        .default_value = "0"
    },
};

const int access_level_field_count = sizeof(access_level_fields) / sizeof(access_level_fields[0]);

/* ============================================================================
 * Menu Configuration (menus.ctl)
 * ============================================================================ */

/* MenuFile display type flags */
static const char *menufile_type_options[] = {
    "Novice", "Regular", "Expert", "RIP", NULL
};

/* HeaderFile display type flags */
static const char *headerfile_type_options[] = {
    "Novice", "Regular", "Expert", "RIP", NULL
};

static const char *menu_command_options[] = {
    "Display_Menu",
    "Display_File",
    "MEX",
    "Goodbye",
    "Userlist",
    "Press_Enter",
    "Key_Poke",
    "Return",
    "Msg_Area",
    "File_Area",
    "Msg_Change",
    "NewFiles",
    NULL
};

static const char *menu_modifier_options[] = {
    "(None)",
    "NoDsp",
    "Ctl",
    "NoCLS",
    "RIP",
    "NoRIP",
    "Then",
    "Else",
    "Stay",
    "UsrLocal",
    "UsrRemote",
    "ReRead",
    "Local",
    "Matrix",
    "Echo",
    "Conf",
    NULL
};

const FieldDef menu_properties_fields[] = {
    {
        .keyword = "Title",
        .label = "Menu title",
        .help = "Title displayed to users when entering this menu. Can include "
                "tokens like %t for time remaining.",
        .type = FIELD_TEXT,
        .max_length = 80,
        .default_value = "",
        .toggle_options = NULL
    },
    {
        .keyword = "HeaderFile",
        .label = "Header file",
        .help = "File or MEX script to display when entering menu. MEX scripts "
                "start with ':'. Leave blank for none.",
        .type = FIELD_TEXT,
        .max_length = 120,
        .default_value = "",
        .toggle_options = NULL
    },
    {
        .keyword = "HeaderFileTypes",
        .label = "Header types",
        .help = "User types that see the HeaderFile. Press ENTER or F2 to select. "
                "If none selected, all users see it.",
        .type = FIELD_MULTISELECT,
        .max_length = 80,
        .default_value = "",
        .toggle_options = headerfile_type_options
    },
    {
        .keyword = "MenuFile",
        .label = "Menu file",
        .help = "Custom .BBS file to display instead of auto-generated menu. "
                "Leave blank to use auto-generated menu.",
        .type = FIELD_TEXT,
        .max_length = 120,
        .default_value = "",
        .toggle_options = NULL
    },
    {
        .keyword = "MenuFileTypes",
        .label = "Menu types",
        .help = "User types that see the MenuFile. Press ENTER or F2 to select. "
                "If none selected, all users see it.",
        .type = FIELD_MULTISELECT,
        .max_length = 80,
        .default_value = "",
        .toggle_options = menufile_type_options
    },
    {
        .keyword = "MenuLength",
        .label = "Menu length",
        .help = "Number of lines the custom MenuFile occupies. Only needed if "
                "MenuFile is specified. Set to 0 for auto-generated menus.",
        .type = FIELD_NUMBER,
        .max_length = 3,
        .default_value = "0"
    },
    {
        .keyword = "MenuColor",
        .label = "Menu color",
        .help = "AVATAR color code (0-255) for hotkey display when using MenuFile. "
                "Set to -1 for no color override.",
        .type = FIELD_NUMBER,
        .max_length = 4,
        .default_value = "-1"
    },
    {
        .keyword = "OptionWidth",
        .label = "Option width",
        .help = "Width in characters for each menu option (6-80). Set to 0 to use "
                "system default (20).",
        .type = FIELD_NUMBER,
        .max_length = 2,
        .default_value = "0"
    },
};

const int menu_properties_field_count = sizeof(menu_properties_fields) / sizeof(menu_properties_fields[0]);

const FieldDef menu_option_fields[] = {
    {
        .keyword = "Command",
        .label = "Command",
        .help = "Command executed by this menu option. ENTER/F2 to pick from list. SPACE to type manually.",
        .type = FIELD_SELECT,
        .max_length = 30,
        .default_value = "",
        .toggle_options = menu_command_options
    },
    {
        .keyword = "Argument",
        .label = "Argument",
        .help = "Optional command argument. For Display_Menu/MEX/Display_File this will offer an F2 picker.",
        .type = FIELD_TEXT,
        .max_length = 120,
        .default_value = "",
        .toggle_options = NULL
    },
    {
        .keyword = "Priv",
        .label = "Priv level",
        .help = "Privilege level required. ENTER/F2 to pick from list. SPACE to type custom (e.g., Normal/1C).",
        .type = FIELD_SELECT,
        .max_length = 60,
        .default_value = "Demoted",
        .toggle_options = access_level_options
    },
    {
        .keyword = "Desc",
        .label = "Command display",
        .help = "Text shown to user for this option. First character is the hotkey. Use NoDsp options with scan codes for alternate key bindings.",
        .type = FIELD_TEXT,
        .max_length = 80,
        .default_value = "",
        .toggle_options = NULL
    },
    {
        .keyword = "Modifier",
        .label = "Modifier",
        .help = "Optional modifier flag. ENTER/F2 to pick from list. SPACE to type manually. Leave blank for none.",
        .type = FIELD_SELECT,
        .max_length = 40,
        .default_value = "",
        .toggle_options = menu_modifier_options
    },
    {
        .keyword = "KeyPoke",
        .label = "Key poke",
        .help = "Optional key-poke text (enclosed in quotes). When user selects this option, Maximus auto-inserts text into keyboard buffer.",
        .type = FIELD_TEXT,
        .max_length = 40,
        .default_value = "",
        .toggle_options = NULL
    },
};

const int menu_option_field_count = sizeof(menu_option_fields) / sizeof(menu_option_fields[0]);

const FieldDef matrix_netmail_fields[] = {
    {
        .keyword = "nodelist_version",
        .label = "Nodelist Version",
        .help = "Version number of the nodelist format (typically 7 for FidoNet standard nodelist format).",
        .type = FIELD_TEXT,
        .max_length = 10,
        .default_value = "7"
    },
    {
        .keyword = "echotoss_name",
        .label = "EchoToss Name",
        .help = "Filename used by mail tossers for EchoMail processing (typically the echotoss log/bundle base name).",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = ""
    },
    {
        .keyword = "fidouser",
        .label = "FidoUser",
        .help = "Path to the fidouser.lst file mapping FidoNet addresses to local usernames for NetMail routing.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "etc/fidouser.lst"
    },
    {
        .keyword = "ctla_priv",
        .label = "CTLA Privilege",
        .help = "Privilege level required to use ^A (CTRL-A) kludge lines in messages. Common values: Twit, Disgrace, Limited, Normal, Worthy, Privil, Favored, Extra, Clerk, AsstSysop, Sysop, Hidden.",
        .type = FIELD_SELECT,
        .max_length = 60,
        .default_value = "Sysop",
        .toggle_options = access_level_options
    },
    {
        .keyword = "seenby_priv",
        .label = "SEEN-BY Privilege",
        .help = "Privilege level required to view SEEN-BY lines in EchoMail messages. Common values: Twit, Disgrace, Limited, Normal, Worthy, Privil, Favored, Extra, Clerk, AsstSysop, Sysop, Hidden.",
        .type = FIELD_SELECT,
        .max_length = 60,
        .default_value = "Sysop",
        .toggle_options = access_level_options
    },
};

const int matrix_netmail_field_count = sizeof(matrix_netmail_fields) / sizeof(matrix_netmail_fields[0]);

const FieldDef matrix_privileges_fields[] = {
    {
        .keyword = "private_priv",
        .label = "Private Privilege",
        .help = "Privilege level required to view private messages.",
        .type = FIELD_SELECT,
        .max_length = 60,
        .default_value = "Demoted",
        .toggle_options = access_level_options
    },
    {
        .keyword = "fromfile_priv",
        .label = "FromFile Privilege",
        .help = "Privilege level required to use the FromFile message editing feature.",
        .type = FIELD_SELECT,
        .max_length = 60,
        .default_value = "Demoted",
        .toggle_options = access_level_options
    },
    {
        .keyword = "unlisted_priv",
        .label = "Unlisted Privilege",
        .help = "Privilege level required to send NetMail to unlisted nodes.",
        .type = FIELD_SELECT,
        .max_length = 60,
        .default_value = "Demoted",
        .toggle_options = access_level_options
    },
    {
        .keyword = "unlisted_cost",
        .label = "Unlisted Cost",
        .help = "Cost charged for sending NetMail to unlisted nodes.",
        .type = FIELD_TEXT,
        .max_length = 10,
        .default_value = "0"
    },
    {
        .keyword = "log_echomail",
        .label = "Log EchoMail",
        .help = "If enabled, EchoMail tossing/scanning is logged.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
};

const int matrix_privileges_field_count = sizeof(matrix_privileges_fields) / sizeof(matrix_privileges_fields[0]);

const FieldDef matrix_message_attr_priv_fields[] = {
    {
        .keyword = "attribute",
        .label = "Attribute",
        .help = "Message attribute key (private, crash, fileattach, killsent, hold, filerequest, updaterequest, localattach).",
        .type = FIELD_READONLY,
        .max_length = 20,
        .default_value = ""
    },
    {
        .keyword = "priv",
        .label = "Privilege",
        .help = "Privilege level for this attribute.",
        .type = FIELD_SELECT,
        .max_length = 60,
        .default_value = "Demoted",
        .toggle_options = access_level_options
    },
};

const int matrix_message_attr_priv_field_count = sizeof(matrix_message_attr_priv_fields) / sizeof(matrix_message_attr_priv_fields[0]);

const FieldDef matrix_address_fields[] = {
    {
        .keyword = "zone",
        .label = "Zone",
        .help = "FidoNet zone number (1-6 for standard FidoNet zones).",
        .type = FIELD_TEXT,
        .max_length = 5,
        .default_value = "1"
    },
    {
        .keyword = "net",
        .label = "Net",
        .help = "FidoNet network number within the zone.",
        .type = FIELD_TEXT,
        .max_length = 5,
        .default_value = "1"
    },
    {
        .keyword = "node",
        .label = "Node",
        .help = "FidoNet node number within the network.",
        .type = FIELD_TEXT,
        .max_length = 5,
        .default_value = "1"
    },
    {
        .keyword = "point",
        .label = "Point",
        .help = "FidoNet point number (0 for non-point systems).",
        .type = FIELD_TEXT,
        .max_length = 5,
        .default_value = "0"
    },
    {
        .keyword = "domain",
        .label = "Domain",
        .help = "FidoNet domain name (e.g., 'fidonet'). Leave blank for default domain.",
        .type = FIELD_TEXT,
        .max_length = 20,
        .default_value = ""
    },
};

const int matrix_address_field_count = sizeof(matrix_address_fields) / sizeof(matrix_address_fields[0]);

const FieldDef language_settings_fields[] = {
    {
        .keyword = "default_language",
        .label = "Default Language",
        .help = "Name of the default language file (without .LTF extension) used for new users and when no language is specified.",
        .type = FIELD_TEXT,
        .max_length = 20,
        .default_value = "english"
    },
    {
        .keyword = "lang_path",
        .label = "Language Path",
        .help = "Directory containing language files (.LTF, .MAD, .LTH). Must contain at minimum an .LTF file for each declared language.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "etc/lang"
    },
};

const int language_settings_field_count = sizeof(language_settings_fields) / sizeof(language_settings_fields[0]);

const FieldDef protocol_settings_fields[] = {
    {
        .keyword = "protoexit",
        .label = "Protocol Exit Level",
        .help = "Error level returned to batch files after external protocol transfer. Used for post-transfer processing and error handling.",
        .type = FIELD_TEXT,
        .max_length = 10,
        .default_value = "0"
    },
};

const int protocol_settings_field_count = sizeof(protocol_settings_fields) / sizeof(protocol_settings_fields[0]);

const FieldDef protocol_entry_fields[] = {
    {
        .keyword = "index",
        .label = "Index",
        .help = "Protocol slot/index number. This is controlled by list order.",
        .type = FIELD_READONLY,
        .max_length = 10,
        .default_value = "0"
    },
    {
        .keyword = "name",
        .label = "Name",
        .help = "Protocol name (from 'Protocol <name>' in protocol.ctl).",
        .type = FIELD_TEXT,
        .max_length = 40,
        .default_value = ""
    },
    {
        .keyword = "program",
        .label = "Program",
        .help = "Optional protocol program/path (if applicable).",
        .type = FIELD_TEXT,
        .max_length = 80,
        .default_value = ""
    },
    {
        .keyword = "batch",
        .label = "Type: Batch",
        .help = "Type flag from protocol.ctl: Batch.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "exitlevel",
        .label = "Type: Errorlevel",
        .help = "Type flag from protocol.ctl: Errorlevel.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "bi",
        .label = "Type: Bi",
        .help = "Type flag from protocol.ctl: Bi.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "opus",
        .label = "Type: Opus",
        .help = "Type flag from protocol.ctl: Opus.",
        .type = FIELD_TOGGLE,
        .max_length = 5,
        .default_value = "No",
        .toggle_options = toggle_yes_no
    },
    {
        .keyword = "log_file",
        .label = "LogFile",
        .help = "LogFile value from protocol.ctl.",
        .type = FIELD_TEXT,
        .max_length = 80,
        .default_value = ""
    },
    {
        .keyword = "control_file",
        .label = "ControlFile",
        .help = "ControlFile value from protocol.ctl.",
        .type = FIELD_TEXT,
        .max_length = 80,
        .default_value = ""
    },
    {
        .keyword = "download_cmd",
        .label = "DownloadCmd",
        .help = "DownloadCmd value from protocol.ctl.",
        .type = FIELD_TEXT,
        .max_length = 120,
        .default_value = ""
    },
    {
        .keyword = "upload_cmd",
        .label = "UploadCmd",
        .help = "UploadCmd value from protocol.ctl.",
        .type = FIELD_TEXT,
        .max_length = 120,
        .default_value = ""
    },
    {
        .keyword = "download_string",
        .label = "DownloadString",
        .help = "DownloadString value from protocol.ctl.",
        .type = FIELD_TEXT,
        .max_length = 80,
        .default_value = ""
    },
    {
        .keyword = "upload_string",
        .label = "UploadString",
        .help = "UploadString value from protocol.ctl.",
        .type = FIELD_TEXT,
        .max_length = 80,
        .default_value = ""
    },
    {
        .keyword = "download_keyword",
        .label = "DownloadKeyword",
        .help = "DownloadKeyword value from protocol.ctl.",
        .type = FIELD_TEXT,
        .max_length = 40,
        .default_value = ""
    },
    {
        .keyword = "upload_keyword",
        .label = "UploadKeyword",
        .help = "UploadKeyword value from protocol.ctl.",
        .type = FIELD_TEXT,
        .max_length = 40,
        .default_value = ""
    },
    {
        .keyword = "filename_word",
        .label = "FilenameWord",
        .help = "FilenameWord value from protocol.ctl.",
        .type = FIELD_NUMBER,
        .max_length = 10,
        .default_value = "0"
    },
    {
        .keyword = "descript_word",
        .label = "DescriptWord",
        .help = "DescriptWord value from protocol.ctl.",
        .type = FIELD_NUMBER,
        .max_length = 10,
        .default_value = "0"
    },
};

const int protocol_entry_field_count = sizeof(protocol_entry_fields) / sizeof(protocol_entry_fields[0]);

const FieldDef matrix_events_fields[] = {
    {
        .keyword = "after_edit_exit",
        .label = "After NetMail Exit",
        .help = "Errorlevel returned when user enters NetMail. Used for batch integration to trigger mail packer/exporter.",
        .type = FIELD_TEXT,
        .max_length = 10,
        .default_value = "0"
    },
    {
        .keyword = "after_echomail_exit",
        .label = "After EchoMail Exit",
        .help = "Errorlevel returned when user enters EchoMail. Supersedes After NetMail if both NetMail and EchoMail were entered.",
        .type = FIELD_TEXT,
        .max_length = 10,
        .default_value = "0"
    },
    {
        .keyword = "after_local_exit",
        .label = "After Local Exit",
        .help = "Errorlevel returned when user enters a local (non-network) message.",
        .type = FIELD_TEXT,
        .max_length = 10,
        .default_value = "0"
    },
};

const int matrix_events_field_count = sizeof(matrix_events_fields) / sizeof(matrix_events_fields[0]);

const FieldDef reader_settings_fields[] = {
    {
        .keyword = "archivers_ctl",
        .label = "Archivers Config",
        .help = "Path to compress.cfg which defines archiving/unarchiving programs for QWK bundles. Maximus and Squish use compatible formats.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "etc/compress.cfg"
    },
    {
        .keyword = "packet_name",
        .label = "Packet Name",
        .help = "Base filename for QWK packets. Keep to 8 characters, no spaces, DOS-safe characters only.",
        .type = FIELD_TEXT,
        .max_length = 8,
        .default_value = "MAXIMUS"
    },
    {
        .keyword = "work_directory",
        .label = "Work Directory",
        .help = "Blank work directory for offline reader operations. Maximus creates subdirectories here - do not modify manually while in use.",
        .type = FIELD_PATH,
        .max_length = 80,
        .default_value = "tmp/reader"
    },
    {
        .keyword = "phone",
        .label = "Phone Number",
        .help = "Phone number embedded into downloaded packets. Some readers expect format (xxx) yyy-zzzz.",
        .type = FIELD_TEXT,
        .max_length = 20,
        .default_value = ""
    },
    {
        .keyword = "max_pack",
        .label = "Max Messages",
        .help = "Maximum number of messages that can be downloaded in one browse/download session.",
        .type = FIELD_TEXT,
        .max_length = 10,
        .default_value = "500"
    },
};

const int reader_settings_field_count = sizeof(reader_settings_fields) / sizeof(reader_settings_fields[0]);
