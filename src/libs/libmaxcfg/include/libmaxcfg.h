/*
 * libmaxcfg.h — Public API header for config library
 *
 * Copyright 2026 by Kevin Morgan.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef LIBMAXCFG_H
#define LIBMAXCFG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIBMAXCFG_ABI_VERSION 3

typedef struct MaxCfg MaxCfg;

typedef struct MaxCfgToml MaxCfgToml;

typedef enum {
    MAXCFG_VAR_NULL = 0,
    MAXCFG_VAR_INT,
    MAXCFG_VAR_UINT,
    MAXCFG_VAR_BOOL,
    MAXCFG_VAR_STRING,
    MAXCFG_VAR_STRING_ARRAY,
    MAXCFG_VAR_TABLE,
    MAXCFG_VAR_TABLE_ARRAY,
    MAXCFG_VAR_INT_ARRAY
} MaxCfgVarType;

typedef struct {
    const char **items;
    size_t count;
} MaxCfgStrView;

typedef struct {
    const int *items;
    size_t count;
} MaxCfgIntView;

typedef struct {
    MaxCfgVarType type;
    union {
        int i;
        unsigned int u;
        bool b;
        const char *s;
        MaxCfgStrView strv;
        MaxCfgIntView intv;
        void *opaque;
    } v;
} MaxCfgVar;
typedef enum MaxCfgStatus {
    MAXCFG_OK = 0,
    MAXCFG_ERR_INVALID_ARGUMENT,
    MAXCFG_ERR_OOM,
    MAXCFG_ERR_NOT_FOUND,
    MAXCFG_ERR_NOT_DIR,
    MAXCFG_ERR_IO,
    MAXCFG_ERR_PATH_TOO_LONG,
    MAXCFG_ERR_DUPLICATE           /**< Key/namespace already exists */
} MaxCfgStatus;

/**
 * @brief Get the element count of an array-type config variable.
 *
 * @param var       Pointer to a MaxCfgVar (must be an array type).
 * @param out_count Receives the element count.
 * @return MAXCFG_OK on success, or an error status.
 */
MaxCfgStatus maxcfg_var_count(const MaxCfgVar *var, size_t *out_count);

/**
 * @brief Convert a DOS color name string to its numeric index (0–15).
 *
 * @param s  Color name (e.g. "blue", "light_green"). Case-insensitive.
 * @return Color index 0–15, or -1 if unrecognized.
 */
int maxcfg_dos_color_from_name(const char *s);

/**
 * @brief Convert a numeric DOS color index to its display name.
 *
 * @param color  Color index (0–15).
 * @return Human-readable color name, or "" if out of range.
 */
const char *maxcfg_dos_color_to_name(int color);

/**
 * @brief Build a DOS attribute byte from foreground and background colors.
 *
 * @param fg  Foreground color (0–15).
 * @param bg  Background color (0–15).
 * @return Combined attribute byte.
 */
unsigned char maxcfg_make_attr(int fg, int bg);

typedef struct {
    int config_version;
    char *system_name;
    char *sysop;
    int task_num;
    char *video;
    bool has_snow;
    char *multitasker;
    char *sys_path;
    char *config_path;
    char *display_path;      /**< Was misc_path — display/screens base */
    char *lang_path;
    char *temp_path;
    char *net_info_path;
    char *node_path;         /**< Was ipc_path — per-node runtime dirs */
    char *outbound_path;
    char *inbound_path;
    char *stage_path;
    char *mex_path;          /**< MEX scripts directory */
    char *data_path;         /**< Persistent mutable data root */
    char *run_path;          /**< Ephemeral runtime state root */
    char *doors_path;        /**< External door programs */
    char *msg_reader_menu;
    char *log_file;
    char *file_password;
    char *file_access;
    char *file_callers;
    char *message_data;
    char *file_data;
    char *log_mode;
    int mcp_sessions;
    bool snoop;
    bool no_password_encryption;
    bool no_share;
    bool reboot;
    bool swap;
    bool dos_close;
    bool local_input_timeout;
    bool status_line;
} MaxCfgNgSystem;

typedef struct {
    bool alias_system;
    bool ask_alias;
    bool single_word_names;
    bool check_ansi;
    bool check_rip;
    bool ask_phone;
    bool no_real_name;

    bool disable_userlist;
    bool disable_magnet;

    char *edit_menu;

    bool autodate;
    int date_style;
    int filelist_margin;
    int exit_after_call;

    char *chat_program;
    char *local_editor;

    bool yell_enabled;
    bool compat_local_baud_9600;

    unsigned int min_free_kb;
    char *upload_log;
    char *virus_check;

    int mailchecker_reply_priv;
    int mailchecker_kill_priv;

    char *comment_area;
    char *highest_message_area;
    char *highest_file_area;
    char *area_change_keys;

    bool chat_capture;
    bool strict_xfer;
    bool gate_netmail;
    bool global_high_bit;
    bool upload_check_dupe;
    bool upload_check_dupe_extension;
    bool use_umsgids;
    int logon_priv;
    int logon_timelimit;
    int min_logon_baud;
    int min_graphics_baud;
    int min_rip_baud;
    int input_timeout;
    unsigned int max_msgsize;
    char *kill_private;
    char *charset;
    char **save_directories;
    size_t save_directory_count;
    char *track_privview;
    char *track_privmod;
    char *track_base;
    char *track_exclude;
    char *attach_base;
    char *attach_path;
    char *attach_archiver;
    char *kill_attach;
    int msg_localattach_priv;
    int kill_attach_priv;
    char *first_menu;
    char *first_file_area;
    char *first_message_area;
} MaxCfgNgGeneralSession;

typedef struct {
    char *attribute;
    int priv;
} MaxCfgNgAttributePriv;

typedef struct {
    int ctla_priv;
    int seenby_priv;
    int private_priv;
    int fromfile_priv;
    int unlisted_priv;
    int unlisted_cost;
    bool log_echomail;
    int after_edit_exit;
    int after_echomail_exit;
    int after_local_exit;
    char *nodelist_version;
    char *fidouser;
    char *echotoss_name;

    MaxCfgNgAttributePriv *message_edit_ask;
    size_t message_edit_ask_count;
    MaxCfgNgAttributePriv *message_edit_assume;
    size_t message_edit_assume_count;

    struct {
        int zone;
        int net;
        int node;
        int point;
    } *addresses;
    size_t address_count;
} MaxCfgNgMatrix;

typedef struct {
    int max_pack;
    char *archivers_ctl;
    char *packet_name;
    char *work_directory;
    char *phone;
} MaxCfgNgReader;

typedef struct {
    char *output;
    int com_port;
    int baud_maximum;
    char *busy;
    char *init;
    char *ring;
    char *answer;
    char *connect;
    int carrier_mask;
    char **handshaking;
    size_t handshaking_count;
    bool send_break;
    bool no_critical;
} MaxCfgNgEquipment;

typedef struct {
    int index;
    char *name;
    char *program;
    bool batch;
    bool exitlevel;

    char *log_file;
    char *control_file;
    char *download_cmd;
    char *upload_cmd;
    char *download_string;
    char *upload_string;
    char *download_keyword;
    char *upload_keyword;
    int filename_word;
    int descript_word;

    bool opus;
    bool bi;
} MaxCfgNgProtocol;

typedef struct {
    int protoexit;
    char *protocol_max_path;
    bool protocol_max_exists;
    char *protocol_ctl_path;
    bool protocol_ctl_exists;
    MaxCfgNgProtocol *items;
    size_t count;
    size_t capacity;
} MaxCfgNgProtocolList;

typedef struct {
    int max_lang;
    char **lang_files;
    size_t lang_file_count;
    int max_ptrs;
    int max_heap;
    int max_glh_ptrs;
    int max_glh_len;
    int max_syh_ptrs;
    int max_syh_len;
} MaxCfgNgLanguage;

typedef struct {
    char *logo;
    char *not_found;
    char *application;
    char *welcome;
    char *new_user1;
    char *new_user2;
    char *rookie;
    char *not_configured;
    char *quote;
    char *day_limit;
    char *time_warn;
    char *too_slow;
    char *bye_bye;
    char *bad_logon;
    char *barricade;
    char *no_space;
    char *no_mail;
    char *area_not_exist;
    char *chat_begin;
    char *chat_end;
    char *out_leaving;
    char *out_return;
    char *shell_to_dos;
    char *back_from_dos;
    char *locate;
    char *contents;
    char *oped_help;
    char *line_ed_help;
    char *replace_help;
    char *inquire_help;
    char *scan_help;
    char *list_help;
    char *header_help;
    char *entry_help;
    char *xfer_baud;
    char *file_area_list;
    char *file_header;
    char *file_format;
    char *file_footer;
    char *msg_area_list;
    char *msg_header;
    char *msg_format;
    char *msg_footer;
    char *protocol_dump;
    char *fname_format;
    char *time_format;
    char *date_format;
    char *tune;
} MaxCfgNgGeneralDisplayFiles;

/** @brief Lightbar area display settings for a single area type (file or msg). */
typedef struct {
    bool  lightbar_area;       /**< Enable lightbar navigation for area list */
    int   reduce_area;         /**< Rows to subtract from screen height for bottom boundary fallback */
    int   top_boundary_row;    /**< Top boundary row (1-based, 0 = not set) */
    int   top_boundary_col;    /**< Top boundary col (1-based, 0 = not set) */
    int   bottom_boundary_row; /**< Bottom boundary row (1-based, 0 = not set) */
    int   bottom_boundary_col; /**< Bottom boundary col (1-based, 0 = not set) */
    int   header_location_row; /**< Header anchor row (1-based, 0 = not set) */
    int   header_location_col; /**< Header anchor col (1-based, 0 = not set) */
    int   footer_location_row; /**< Footer anchor row (1-based, 0 = not set) */
    int   footer_location_col; /**< Footer anchor col (1-based, 0 = not set) */
    char *custom_screen;       /**< Optional additive display file path */
} MaxCfgNgDisplayAreaCfg;

/** @brief Top-level display/UI behavior configuration (general.display). */
typedef struct {
    bool                    lightbar_prompts; /**< Global lightbar prompt toggle */
    MaxCfgNgDisplayAreaCfg  file_areas;       /**< File area lightbar settings */
    MaxCfgNgDisplayAreaCfg  msg_areas;        /**< Message area lightbar settings */
} MaxCfgNgGeneralDisplay;

typedef struct {
    int fg;
    int bg;
    bool blink;
} MaxCfgNgColor;

typedef struct {
    MaxCfgNgColor menu_name;
    MaxCfgNgColor menu_highlight;
    MaxCfgNgColor menu_option;

    MaxCfgNgColor file_name;
    MaxCfgNgColor file_size;
    MaxCfgNgColor file_date;
    MaxCfgNgColor file_description;
    MaxCfgNgColor file_search_match;
    MaxCfgNgColor file_offline;
    MaxCfgNgColor file_new;

    MaxCfgNgColor msg_from_label;
    MaxCfgNgColor msg_from_text;
    MaxCfgNgColor msg_to_label;
    MaxCfgNgColor msg_to_text;
    MaxCfgNgColor msg_subject_label;
    MaxCfgNgColor msg_subject_text;
    MaxCfgNgColor msg_attributes;
    MaxCfgNgColor msg_date;
    MaxCfgNgColor msg_address;
    MaxCfgNgColor msg_locus;
    MaxCfgNgColor msg_body;
    MaxCfgNgColor msg_quote;
    MaxCfgNgColor msg_kludge;

    MaxCfgNgColor fsr_msgnum;
    MaxCfgNgColor fsr_links;
    MaxCfgNgColor fsr_attrib;
    MaxCfgNgColor fsr_msginfo;
    MaxCfgNgColor fsr_date;
    MaxCfgNgColor fsr_addr;
    MaxCfgNgColor fsr_static;
    MaxCfgNgColor fsr_border;
    MaxCfgNgColor fsr_locus;
} MaxCfgNgGeneralColors;

/* ============================================================================
 * Semantic theme color slots (|xx lowercase pipe codes)
 * ============================================================================ */

/** @brief Number of defined semantic theme color slots. */
#define MCI_THEME_SLOT_COUNT 25

/** @brief A single theme color slot mapping a 2-char code to an MCI pipe string. */
typedef struct {
    char code[4];        /**< 2-letter lowercase MCI code, e.g. "tx" */
    char key[32];        /**< TOML key name, e.g. "text" */
    char value[64];      /**< MCI pipe code string, e.g. "|07" or "|15|17" */
    char desc[64];       /**< Human-readable description */
} MaxCfgThemeSlot;

/** @brief Full theme color table with a name and all slots. */
typedef struct {
    char name[128];
    MaxCfgThemeSlot slots[MCI_THEME_SLOT_COUNT];
} MaxCfgThemeColors;

/** @brief Initialize a theme color table with built-in defaults. */
void maxcfg_theme_init(MaxCfgThemeColors *theme);

/**
 * @brief Load theme colors from a TOML handle.
 *
 * Reads @c <prefix>.theme.name and @c <prefix>.theme.colors.<slot> from the
 * given TOML handle.  For example, if @p prefix is @c "colors", the lookup
 * paths are @c "colors.theme.name" and @c "colors.theme.colors.text", etc.
 *
 * @param theme   Output theme table (initialized to defaults first).
 * @param toml    TOML handle to read from.
 * @param prefix  Dot-separated TOML path prefix (e.g. "colors" or
 *                "general.colors").
 */
MaxCfgStatus maxcfg_theme_load_from_toml(MaxCfgThemeColors *theme,
                                          const MaxCfgToml *toml,
                                          const char *prefix);

/**
 * @brief Look up a theme slot by its 2-char lowercase code.
 * @return Pointer to the MCI pipe string, or NULL if not found.
 */
const char *maxcfg_theme_lookup(const MaxCfgThemeColors *theme,
                                 char a, char b);

/** @brief Write the [theme] section to a TOML file. */
MaxCfgStatus maxcfg_theme_write_toml(FILE *fp, const MaxCfgThemeColors *theme);

/**
 * @brief Convert a MaxCfgNgColor to an MCI pipe code string.
 *
 * Emits |fg, optionally |16+bg if bg > 0, and |24 if blink is set.
 * @param color  Source color.
 * @param out    Output buffer (must be >= 16 bytes).
 * @param out_sz Size of output buffer.
 */
void maxcfg_ng_color_to_mci(const MaxCfgNgColor *color, char *out, size_t out_sz);

typedef struct {
    char *command;
    char *arguments;
    char *priv_level;
    char *description;
    char *key_poke;
    char **modifiers;
    size_t modifier_count;
} MaxCfgNgMenuOption;

typedef struct {
    bool enabled;
    bool skip_canned_menu;
    bool show_title;

    bool lightbar_menu;
    int lightbar_margin;

    unsigned char lightbar_normal_attr;
    unsigned char lightbar_selected_attr;
    unsigned char lightbar_high_attr;
    unsigned char lightbar_high_selected_attr;

    bool has_lightbar_normal;
    bool has_lightbar_selected;
    bool has_lightbar_high;
    bool has_lightbar_high_selected;

    bool option_spacing;
    int option_justify;
    int boundary_justify;
    int boundary_vjustify;
    int boundary_layout;

    int top_boundary_row;
    int top_boundary_col;
    int bottom_boundary_row;
    int bottom_boundary_col;
    int title_location_row;
    int title_location_col;
    int prompt_location_row;
    int prompt_location_col;
} MaxCfgNgCustomMenu;

typedef struct {
    char *name;
    char *title;
    char *header_file;
    char **header_types;
    size_t header_type_count;
    char *footer_file;
    char **footer_types;
    size_t footer_type_count;
    char *menu_file;
    char **menu_types;
    size_t menu_type_count;
    int menu_length;
    int menu_color;
    int option_width;

    MaxCfgNgCustomMenu *custom_menu;

    MaxCfgNgMenuOption *options;
    size_t option_count;
    size_t option_capacity;
} MaxCfgNgMenu;

typedef struct {
    char *name;
    char *key;
    char *description;
    char *acs;
    char *display_file;
    int level;
} MaxCfgNgDivision;

typedef struct {
    MaxCfgNgDivision *items;
    size_t count;
    size_t capacity;
} MaxCfgNgDivisionList;

typedef struct {
    char *name;
    char *description;
    char *acs;
    char *menu;
    char *division;

    char *tag;
    char *path;
    char *owner;
    char *origin;
    char *attach_path;
    char *barricade;

    char **style;
    size_t style_count;
    int renum_max;
    int renum_days;
} MaxCfgNgMsgArea;

typedef struct {
    MaxCfgNgMsgArea *items;
    size_t count;
    size_t capacity;
} MaxCfgNgMsgAreaList;

typedef struct {
    char *name;
    char *description;
    char *acs;
    char *menu;
    char *division;

    char *download;
    char *upload;
    char *filelist;
    char *barricade;

    char **types;
    size_t type_count;
} MaxCfgNgFileArea;

typedef struct {
    MaxCfgNgFileArea *items;
    size_t count;
    size_t capacity;
} MaxCfgNgFileAreaList;

typedef struct {
    char *name;
    int level;
    char *description;
    char *alias;
    char *key;
    int time;
    int cume;
    int calls;
    int logon_baud;
    int xfer_baud;
    int file_limit;
    int file_ratio;
    int ratio_free;
    int upload_reward;
    char *login_file;
    char **flags;
    size_t flag_count;
    char **mail_flags;
    size_t mail_flag_count;
    unsigned int user_flags;
    int oldpriv;
} MaxCfgNgAccessLevel;

typedef struct {
    MaxCfgNgAccessLevel *items;
    size_t count;
    size_t capacity;
} MaxCfgNgAccessLevelList;

/**
 * @brief Open a config context rooted at the given base directory.
 *
 * @param out_cfg   Receives the allocated config handle.
 * @param base_dir  Root directory for config file resolution.
 * @return MAXCFG_OK on success, or an error status.
 */
MaxCfgStatus maxcfg_open(MaxCfg **out_cfg, const char *base_dir);

/**
 * @brief Close a config context and free all associated memory.
 *
 * @param cfg  Config handle to close (NULL-safe).
 */
void maxcfg_close(MaxCfg *cfg);

/**
 * @brief Return the base directory path for a config handle.
 *
 * @param cfg  Config handle.
 * @return Base directory string, or NULL if cfg is NULL.
 */
const char *maxcfg_base_dir(const MaxCfg *cfg);

/**
 * @brief Resolve a path relative to a base directory.
 *
 * Absolute paths are returned unchanged; relative paths are joined
 * to base_dir.
 *
 * @param base_dir       Base directory for resolution.
 * @param path           Path to resolve (absolute or relative).
 * @param out_path       Output buffer for the resolved path.
 * @param out_path_size  Size of the output buffer.
 * @return MAXCFG_OK on success, or an error status.
 */
MaxCfgStatus maxcfg_resolve_path(const char *base_dir,
                                const char *path,
                                char *out_path,
                                size_t out_path_size);

/**
 * @brief Join a relative path to the config base directory.
 *
 * @param cfg            Config handle providing the base directory.
 * @param relative_path  Relative path to append.
 * @param out_path       Output buffer for the joined path.
 * @param out_path_size  Size of the output buffer.
 * @return MAXCFG_OK on success, or an error status.
 */
MaxCfgStatus maxcfg_join_path(const MaxCfg *cfg,
                             const char *relative_path,
                             char *out_path,
                             size_t out_path_size);

/**
 * @brief Return a human-readable string for a status code.
 *
 * @param st  Status code.
 * @return Static string describing the status.
 */
const char *maxcfg_status_string(MaxCfgStatus st);

/** @brief Allocate and initialize a TOML store handle. */
MaxCfgStatus maxcfg_toml_init(MaxCfgToml **out_toml);

/** @brief Free a TOML store handle and all loaded data. */
void maxcfg_toml_free(MaxCfgToml *toml);

/**
 * @brief Load a TOML file into the store under an optional dotted prefix.
 *
 * @param toml    TOML store handle.
 * @param path    Filesystem path to the .toml file.
 * @param prefix  Dotted prefix for all keys (e.g. "general"), or "" for root.
 * @return MAXCFG_OK on success, or an error status.
 */
MaxCfgStatus maxcfg_toml_load_file(MaxCfgToml *toml, const char *path, const char *prefix);

/**
 * @brief Retrieve a value from the TOML store by dotted path.
 *
 * @param toml  TOML store handle.
 * @param path  Dotted key path (e.g. "maximus.sys_path").
 * @param out   Receives the value.
 * @return MAXCFG_OK on success, MAXCFG_ERR_NOT_FOUND if absent.
 */
MaxCfgStatus maxcfg_toml_get(const MaxCfgToml *toml, const char *path, MaxCfgVar *out);

/** @brief Set an integer override in the TOML store. */
MaxCfgStatus maxcfg_toml_override_set_int(MaxCfgToml *toml, const char *path, int v);

/** @brief Set an unsigned integer override in the TOML store. */
MaxCfgStatus maxcfg_toml_override_set_uint(MaxCfgToml *toml, const char *path, unsigned int v);

/** @brief Set a boolean override in the TOML store. */
MaxCfgStatus maxcfg_toml_override_set_bool(MaxCfgToml *toml, const char *path, bool v);

/** @brief Set a string override in the TOML store. */
MaxCfgStatus maxcfg_toml_override_set_string(MaxCfgToml *toml, const char *path, const char *v);

/** @brief Set a string array override in the TOML store. */
MaxCfgStatus maxcfg_toml_override_set_string_array(MaxCfgToml *toml, const char *path, const char **items, size_t count);

/** @brief Set an empty table array override in the TOML store. */
MaxCfgStatus maxcfg_toml_override_set_table_array_empty(MaxCfgToml *toml, const char *path);

/** @brief Remove an override at the given path. */
MaxCfgStatus maxcfg_toml_override_unset(MaxCfgToml *toml, const char *path);

/** @brief Clear all overrides from the TOML store. */
void maxcfg_toml_override_clear(MaxCfgToml *toml);

/** @brief Save all loaded files back to disk (with overrides merged). */
MaxCfgStatus maxcfg_toml_save_loaded_files(const MaxCfgToml *toml);

/** @brief Save only the file(s) matching a given prefix. */
MaxCfgStatus maxcfg_toml_save_prefix(const MaxCfgToml *toml, const char *prefix);

/** @brief Persist a single override into the base TOML data. */
MaxCfgStatus maxcfg_toml_persist_override(MaxCfgToml *toml, const char *path);

/** @brief Persist a single override and immediately save its file. */
MaxCfgStatus maxcfg_toml_persist_override_and_save(MaxCfgToml *toml, const char *path);

/** @brief Persist all pending overrides into the base TOML data. */
MaxCfgStatus maxcfg_toml_persist_overrides(MaxCfgToml *toml);

/** @brief Persist all pending overrides and save all affected files. */
MaxCfgStatus maxcfg_toml_persist_overrides_and_save(MaxCfgToml *toml);

/**
 * @brief Look up a key within a table-type MaxCfgVar.
 *
 * @param table  Table variable to search.
 * @param key    Key name to look up.
 * @param out    Receives the value.
 * @return MAXCFG_OK on success, MAXCFG_ERR_NOT_FOUND if absent.
 */
MaxCfgStatus maxcfg_toml_table_get(const MaxCfgVar *table, const char *key, MaxCfgVar *out);

/**
 * @brief Access an element by index within an array-type MaxCfgVar.
 *
 * @param array  Array variable to index.
 * @param index  Zero-based element index.
 * @param out    Receives the value.
 * @return MAXCFG_OK on success, or an error status.
 */
MaxCfgStatus maxcfg_toml_array_get(const MaxCfgVar *array, size_t index, MaxCfgVar *out);

/** @brief Parse a video mode string into numeric constants. */
MaxCfgStatus maxcfg_ng_parse_video_mode(const char *s, int *out_video, bool *out_has_snow);

/** @brief Parse a log mode string ("terse", "verbose", "trace"). */
MaxCfgStatus maxcfg_ng_parse_log_mode(const char *s, int *out_log_mode);

/** @brief Parse a multitasker name string into its numeric constant. */
MaxCfgStatus maxcfg_ng_parse_multitasker(const char *s, int *out_multitasker);

/** @brief Parse a single handshaking token ("xon", "cts", "dsr") to bit flags. */
MaxCfgStatus maxcfg_ng_parse_handshaking_token(const char *s, int *out_bits);

/** @brief Parse a charset name into its numeric constant and high-bit flag. */
MaxCfgStatus maxcfg_ng_parse_charset(const char *s, int *out_charset, bool *out_global_high_bit);

/** @brief Parse a nodelist version string ("5", "6", "7", "fd"). */
MaxCfgStatus maxcfg_ng_parse_nodelist_version(const char *s, int *out_nlver);

/** @brief Read and parse the video mode from the TOML config. */
MaxCfgStatus maxcfg_ng_get_video_mode(const MaxCfgToml *toml, int *out_video, bool *out_has_snow);

/** @brief Read and parse the log mode from the TOML config. */
MaxCfgStatus maxcfg_ng_get_log_mode(const MaxCfgToml *toml, int *out_log_mode);

/** @brief Read and parse the multitasker setting from the TOML config. */
MaxCfgStatus maxcfg_ng_get_multitasker(const MaxCfgToml *toml, int *out_multitasker);

/** @brief Read and combine handshaking flags from the TOML config. */
MaxCfgStatus maxcfg_ng_get_handshake_mask(const MaxCfgToml *toml, int *out_mask);

/** @brief Read and parse the charset from the TOML config. */
MaxCfgStatus maxcfg_ng_get_charset(const MaxCfgToml *toml, int *out_charset);

/** @brief Read and parse the nodelist version from the TOML config. */
MaxCfgStatus maxcfg_ng_get_nodelist_version(const MaxCfgToml *toml, int *out_nlver);

/**
 * @brief Return the library ABI version number.
 *
 * @return Current ABI version (LIBMAXCFG_ABI_VERSION).
 */
int maxcfg_abi_version(void);

/** @brief Initialize an NgSystem struct to safe defaults. */
MaxCfgStatus maxcfg_ng_system_init(MaxCfgNgSystem *sys);

/** @brief Free all heap memory owned by an NgSystem struct. */
void maxcfg_ng_system_free(MaxCfgNgSystem *sys);

/** @brief Initialize an NgGeneralSession struct to safe defaults. */
MaxCfgStatus maxcfg_ng_general_session_init(MaxCfgNgGeneralSession *session);

/** @brief Free all heap memory owned by an NgGeneralSession struct. */
void maxcfg_ng_general_session_free(MaxCfgNgGeneralSession *session);

/** @brief Initialize an NgMatrix struct to safe defaults. */
MaxCfgStatus maxcfg_ng_matrix_init(MaxCfgNgMatrix *matrix);

/** @brief Free all heap memory owned by an NgMatrix struct. */
void maxcfg_ng_matrix_free(MaxCfgNgMatrix *matrix);

/** @brief Initialize an NgReader struct to safe defaults. */
MaxCfgStatus maxcfg_ng_reader_init(MaxCfgNgReader *reader);

/** @brief Free all heap memory owned by an NgReader struct. */
void maxcfg_ng_reader_free(MaxCfgNgReader *reader);

/** @brief Initialize an NgEquipment struct to safe defaults. */
MaxCfgStatus maxcfg_ng_equipment_init(MaxCfgNgEquipment *equip);

/** @brief Free all heap memory owned by an NgEquipment struct. */
void maxcfg_ng_equipment_free(MaxCfgNgEquipment *equip);

/** @brief Initialize a protocol list to empty. */
MaxCfgStatus maxcfg_ng_protocol_list_init(MaxCfgNgProtocolList *list);

/** @brief Free a protocol list and all its entries. */
void maxcfg_ng_protocol_list_free(MaxCfgNgProtocolList *list);

/** @brief Append a protocol entry to the list (deep-copies fields). */
MaxCfgStatus maxcfg_ng_protocol_list_add(MaxCfgNgProtocolList *list, const MaxCfgNgProtocol *proto);

/** @brief Initialize an NgLanguage struct to safe defaults. */
MaxCfgStatus maxcfg_ng_language_init(MaxCfgNgLanguage *lang);

/** @brief Free all heap memory owned by an NgLanguage struct. */
void maxcfg_ng_language_free(MaxCfgNgLanguage *lang);

/** @brief Initialize an NgGeneralDisplayFiles struct to safe defaults. */
MaxCfgStatus maxcfg_ng_general_display_files_init(MaxCfgNgGeneralDisplayFiles *files);

/** @brief Free all heap memory owned by an NgGeneralDisplayFiles struct. */
void maxcfg_ng_general_display_files_free(MaxCfgNgGeneralDisplayFiles *files);

/** @brief Initialize an NgGeneralDisplay struct to safe defaults. */
MaxCfgStatus maxcfg_ng_general_display_init(MaxCfgNgGeneralDisplay *disp);

/** @brief Free all heap memory owned by an NgGeneralDisplay struct. */
void maxcfg_ng_general_display_free(MaxCfgNgGeneralDisplay *disp);

/** @brief Parse the general.display section from TOML into the struct. */
MaxCfgStatus maxcfg_ng_get_general_display(const MaxCfgToml *toml, const char *prefix, MaxCfgNgGeneralDisplay *disp);

/** @brief Initialize an NgGeneralColors struct to safe defaults. */
MaxCfgStatus maxcfg_ng_general_colors_init(MaxCfgNgGeneralColors *colors);

/** @brief Initialize an NgMenu struct to empty. */
MaxCfgStatus maxcfg_ng_menu_init(MaxCfgNgMenu *menu);

/** @brief Free a menu struct and all its option entries. */
void maxcfg_ng_menu_free(MaxCfgNgMenu *menu);

/** @brief Append a menu option to the menu (deep-copies fields). */
MaxCfgStatus maxcfg_ng_menu_add_option(MaxCfgNgMenu *menu, const MaxCfgNgMenuOption *opt);

/** @brief Initialize a division list to empty. */
MaxCfgStatus maxcfg_ng_division_list_init(MaxCfgNgDivisionList *list);

/** @brief Free a division list and all its entries. */
void maxcfg_ng_division_list_free(MaxCfgNgDivisionList *list);

/** @brief Append a division to the list (deep-copies fields). */
MaxCfgStatus maxcfg_ng_division_list_add(MaxCfgNgDivisionList *list, const MaxCfgNgDivision *div);

/** @brief Initialize a message area list to empty. */
MaxCfgStatus maxcfg_ng_msg_area_list_init(MaxCfgNgMsgAreaList *list);

/** @brief Free a message area list and all its entries. */
void maxcfg_ng_msg_area_list_free(MaxCfgNgMsgAreaList *list);

/** @brief Append a message area to the list (deep-copies fields). */
MaxCfgStatus maxcfg_ng_msg_area_list_add(MaxCfgNgMsgAreaList *list, const MaxCfgNgMsgArea *area);

/** @brief Initialize a file area list to empty. */
MaxCfgStatus maxcfg_ng_file_area_list_init(MaxCfgNgFileAreaList *list);

/** @brief Free a file area list and all its entries. */
void maxcfg_ng_file_area_list_free(MaxCfgNgFileAreaList *list);

/** @brief Append a file area to the list (deep-copies fields). */
MaxCfgStatus maxcfg_ng_file_area_list_add(MaxCfgNgFileAreaList *list, const MaxCfgNgFileArea *area);

/** @brief Initialize an access level list to empty. */
MaxCfgStatus maxcfg_ng_access_level_list_init(MaxCfgNgAccessLevelList *list);

/** @brief Free an access level list and all its entries. */
void maxcfg_ng_access_level_list_free(MaxCfgNgAccessLevelList *list);

/** @brief Append an access level to the list (deep-copies fields). */
MaxCfgStatus maxcfg_ng_access_level_list_add(MaxCfgNgAccessLevelList *list, const MaxCfgNgAccessLevel *lvl);

/**
 * @brief Parse message area definitions from TOML.
 *
 * @param toml       TOML store handle.
 * @param prefix     Dotted prefix to the message area config section.
 * @param divisions  Receives parsed division list.
 * @param areas      Receives parsed message area list.
 * @return MAXCFG_OK on success, or an error status.
 */
MaxCfgStatus maxcfg_ng_get_msg_areas(const MaxCfgToml *toml, const char *prefix,
                                    MaxCfgNgDivisionList *divisions, MaxCfgNgMsgAreaList *areas);

/**
 * @brief Parse file area definitions from TOML.
 *
 * @param toml       TOML store handle.
 * @param prefix     Dotted prefix to the file area config section.
 * @param divisions  Receives parsed division list.
 * @param areas      Receives parsed file area list.
 * @return MAXCFG_OK on success, or an error status.
 */
MaxCfgStatus maxcfg_ng_get_file_areas(const MaxCfgToml *toml, const char *prefix,
                                     MaxCfgNgDivisionList *divisions, MaxCfgNgFileAreaList *areas);

/**
 * @brief Parse a menu definition from TOML.
 *
 * @param toml    TOML store handle.
 * @param prefix  Dotted prefix to the menu config section.
 * @param menu    Receives the parsed menu.
 * @return MAXCFG_OK on success, or an error status.
 */
MaxCfgStatus maxcfg_ng_get_menu(const MaxCfgToml *toml, const char *prefix, MaxCfgNgMenu *menu);

/**
 * @brief Parse access level definitions from TOML.
 *
 * @param toml    TOML store handle.
 * @param prefix  Dotted prefix to the access level config section.
 * @param levels  Receives the parsed access level list.
 * @return MAXCFG_OK on success, or an error status.
 */
MaxCfgStatus maxcfg_ng_get_access_levels(const MaxCfgToml *toml, const char *prefix, MaxCfgNgAccessLevelList *levels);

/** @brief Write the [maximus] system section to a TOML file. */
MaxCfgStatus maxcfg_ng_write_maximus_toml(FILE *fp, const MaxCfgNgSystem *sys);

/** @brief Write the [general.session] section to a TOML file. */
MaxCfgStatus maxcfg_ng_write_general_session_toml(FILE *fp, const MaxCfgNgGeneralSession *session);

/** @brief Write the [general.display_files] section to a TOML file. */
MaxCfgStatus maxcfg_ng_write_general_display_files_toml(FILE *fp, const MaxCfgNgGeneralDisplayFiles *files);

/** @brief Write the [general.display] section to a TOML file. */
MaxCfgStatus maxcfg_ng_write_general_display_toml(FILE *fp, const MaxCfgNgGeneralDisplay *disp);

/** @brief Write the [general.colors] section to a TOML file. */
MaxCfgStatus maxcfg_ng_write_general_colors_toml(FILE *fp, const MaxCfgNgGeneralColors *colors);

/** @brief Write the [matrix] section to a TOML file. */
MaxCfgStatus maxcfg_ng_write_matrix_toml(FILE *fp, const MaxCfgNgMatrix *matrix);

/** @brief Write the [reader] section to a TOML file. */
MaxCfgStatus maxcfg_ng_write_reader_toml(FILE *fp, const MaxCfgNgReader *reader);

/** @brief Write the [general.equipment] section to a TOML file. */
MaxCfgStatus maxcfg_ng_write_equipment_toml(FILE *fp, const MaxCfgNgEquipment *equip);

/** @brief Write the [[protocol]] array section to a TOML file. */
MaxCfgStatus maxcfg_ng_write_protocols_toml(FILE *fp, const MaxCfgNgProtocolList *list);

/** @brief Write the [language] section to a TOML file. */
MaxCfgStatus maxcfg_ng_write_language_toml(FILE *fp, const MaxCfgNgLanguage *lang);

/** @brief Write a menu definition to a TOML file. */
MaxCfgStatus maxcfg_ng_write_menu_toml(FILE *fp, const MaxCfgNgMenu *menu);

/** @brief Write message area definitions to a TOML file. */
MaxCfgStatus maxcfg_ng_write_msg_areas_toml(FILE *fp, const MaxCfgNgDivisionList *divisions, const MaxCfgNgMsgAreaList *areas);

/** @brief Write file area definitions to a TOML file. */
MaxCfgStatus maxcfg_ng_write_file_areas_toml(FILE *fp, const MaxCfgNgDivisionList *divisions, const MaxCfgNgFileAreaList *areas);

/** @brief Write access level definitions to a TOML file. */
MaxCfgStatus maxcfg_ng_write_access_levels_toml(FILE *fp, const MaxCfgNgAccessLevelList *levels);

#ifdef __cplusplus
}
#endif

#endif
