#ifndef LIBMAXCFG_H
#define LIBMAXCFG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIBMAXCFG_ABI_VERSION 1

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
    MAXCFG_VAR_TABLE_ARRAY
} MaxCfgVarType;

typedef struct {
    const char **items;
    size_t count;
} MaxCfgStrView;

typedef struct {
    MaxCfgVarType type;
    union {
        int i;
        unsigned int u;
        bool b;
        const char *s;
        MaxCfgStrView strv;
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
    MAXCFG_ERR_PATH_TOO_LONG
} MaxCfgStatus;

MaxCfgStatus maxcfg_var_count(const MaxCfgVar *var, size_t *out_count);

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
    char *misc_path;
    char *lang_path;
    char *temp_path;
    char *net_info_path;
    char *ipc_path;
    char *outbound_path;
    char *inbound_path;
    char *menu_path;
    char *rip_path;
    char *stage_path;
    char *log_file;
    char *file_password;
    char *file_access;
    char *file_callers;
    char *protocol_ctl;
    char *message_data;
    char *file_data;
    char *log_mode;
    char *mcp_pipe;
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
    char *name;
    char *title;
    char *header_file;
    char **header_types;
    size_t header_type_count;
    char *menu_file;
    char **menu_types;
    size_t menu_type_count;
    int menu_length;
    int menu_color;
    int option_width;

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

MaxCfgStatus maxcfg_open(MaxCfg **out_cfg, const char *base_dir);
void maxcfg_close(MaxCfg *cfg);

const char *maxcfg_base_dir(const MaxCfg *cfg);

MaxCfgStatus maxcfg_resolve_path(const char *base_dir,
                                const char *path,
                                char *out_path,
                                size_t out_path_size);

MaxCfgStatus maxcfg_join_path(const MaxCfg *cfg,
                             const char *relative_path,
                             char *out_path,
                             size_t out_path_size);

const char *maxcfg_status_string(MaxCfgStatus st);

MaxCfgStatus maxcfg_toml_init(MaxCfgToml **out_toml);
void maxcfg_toml_free(MaxCfgToml *toml);
MaxCfgStatus maxcfg_toml_load_file(MaxCfgToml *toml, const char *path, const char *prefix);
MaxCfgStatus maxcfg_toml_get(const MaxCfgToml *toml, const char *path, MaxCfgVar *out);
MaxCfgStatus maxcfg_toml_override_set_int(MaxCfgToml *toml, const char *path, int v);
MaxCfgStatus maxcfg_toml_override_set_uint(MaxCfgToml *toml, const char *path, unsigned int v);
MaxCfgStatus maxcfg_toml_override_set_bool(MaxCfgToml *toml, const char *path, bool v);
MaxCfgStatus maxcfg_toml_override_set_string(MaxCfgToml *toml, const char *path, const char *v);
MaxCfgStatus maxcfg_toml_override_set_string_array(MaxCfgToml *toml, const char *path, const char **items, size_t count);
MaxCfgStatus maxcfg_toml_override_set_table_array_empty(MaxCfgToml *toml, const char *path);
MaxCfgStatus maxcfg_toml_override_unset(MaxCfgToml *toml, const char *path);
void maxcfg_toml_override_clear(MaxCfgToml *toml);
MaxCfgStatus maxcfg_toml_save_loaded_files(const MaxCfgToml *toml);
MaxCfgStatus maxcfg_toml_save_prefix(const MaxCfgToml *toml, const char *prefix);
MaxCfgStatus maxcfg_toml_persist_override(MaxCfgToml *toml, const char *path);
MaxCfgStatus maxcfg_toml_persist_override_and_save(MaxCfgToml *toml, const char *path);
MaxCfgStatus maxcfg_toml_persist_overrides(MaxCfgToml *toml);
MaxCfgStatus maxcfg_toml_persist_overrides_and_save(MaxCfgToml *toml);
MaxCfgStatus maxcfg_toml_table_get(const MaxCfgVar *table, const char *key, MaxCfgVar *out);
MaxCfgStatus maxcfg_toml_array_get(const MaxCfgVar *array, size_t index, MaxCfgVar *out);

MaxCfgStatus maxcfg_ng_parse_video_mode(const char *s, int *out_video, bool *out_has_snow);
MaxCfgStatus maxcfg_ng_parse_log_mode(const char *s, int *out_log_mode);
MaxCfgStatus maxcfg_ng_parse_multitasker(const char *s, int *out_multitasker);
MaxCfgStatus maxcfg_ng_parse_handshaking_token(const char *s, int *out_bits);
MaxCfgStatus maxcfg_ng_parse_charset(const char *s, int *out_charset, bool *out_global_high_bit);
MaxCfgStatus maxcfg_ng_parse_nodelist_version(const char *s, int *out_nlver);

MaxCfgStatus maxcfg_ng_get_video_mode(const MaxCfgToml *toml, int *out_video, bool *out_has_snow);
MaxCfgStatus maxcfg_ng_get_log_mode(const MaxCfgToml *toml, int *out_log_mode);
MaxCfgStatus maxcfg_ng_get_multitasker(const MaxCfgToml *toml, int *out_multitasker);
MaxCfgStatus maxcfg_ng_get_handshake_mask(const MaxCfgToml *toml, int *out_mask);
MaxCfgStatus maxcfg_ng_get_charset(const MaxCfgToml *toml, int *out_charset);
MaxCfgStatus maxcfg_ng_get_nodelist_version(const MaxCfgToml *toml, int *out_nlver);

int maxcfg_abi_version(void);

MaxCfgStatus maxcfg_ng_system_init(MaxCfgNgSystem *sys);
void maxcfg_ng_system_free(MaxCfgNgSystem *sys);

MaxCfgStatus maxcfg_ng_general_session_init(MaxCfgNgGeneralSession *session);
void maxcfg_ng_general_session_free(MaxCfgNgGeneralSession *session);

MaxCfgStatus maxcfg_ng_matrix_init(MaxCfgNgMatrix *matrix);
void maxcfg_ng_matrix_free(MaxCfgNgMatrix *matrix);

MaxCfgStatus maxcfg_ng_reader_init(MaxCfgNgReader *reader);
void maxcfg_ng_reader_free(MaxCfgNgReader *reader);

MaxCfgStatus maxcfg_ng_equipment_init(MaxCfgNgEquipment *equip);
void maxcfg_ng_equipment_free(MaxCfgNgEquipment *equip);

MaxCfgStatus maxcfg_ng_protocol_list_init(MaxCfgNgProtocolList *list);
void maxcfg_ng_protocol_list_free(MaxCfgNgProtocolList *list);
MaxCfgStatus maxcfg_ng_protocol_list_add(MaxCfgNgProtocolList *list, const MaxCfgNgProtocol *proto);

MaxCfgStatus maxcfg_ng_language_init(MaxCfgNgLanguage *lang);
void maxcfg_ng_language_free(MaxCfgNgLanguage *lang);

MaxCfgStatus maxcfg_ng_general_display_files_init(MaxCfgNgGeneralDisplayFiles *files);
void maxcfg_ng_general_display_files_free(MaxCfgNgGeneralDisplayFiles *files);

MaxCfgStatus maxcfg_ng_general_colors_init(MaxCfgNgGeneralColors *colors);

MaxCfgStatus maxcfg_ng_menu_init(MaxCfgNgMenu *menu);
void maxcfg_ng_menu_free(MaxCfgNgMenu *menu);
MaxCfgStatus maxcfg_ng_menu_add_option(MaxCfgNgMenu *menu, const MaxCfgNgMenuOption *opt);

MaxCfgStatus maxcfg_ng_division_list_init(MaxCfgNgDivisionList *list);
void maxcfg_ng_division_list_free(MaxCfgNgDivisionList *list);
MaxCfgStatus maxcfg_ng_division_list_add(MaxCfgNgDivisionList *list, const MaxCfgNgDivision *div);

MaxCfgStatus maxcfg_ng_msg_area_list_init(MaxCfgNgMsgAreaList *list);
void maxcfg_ng_msg_area_list_free(MaxCfgNgMsgAreaList *list);
MaxCfgStatus maxcfg_ng_msg_area_list_add(MaxCfgNgMsgAreaList *list, const MaxCfgNgMsgArea *area);

MaxCfgStatus maxcfg_ng_file_area_list_init(MaxCfgNgFileAreaList *list);
void maxcfg_ng_file_area_list_free(MaxCfgNgFileAreaList *list);
MaxCfgStatus maxcfg_ng_file_area_list_add(MaxCfgNgFileAreaList *list, const MaxCfgNgFileArea *area);

MaxCfgStatus maxcfg_ng_access_level_list_init(MaxCfgNgAccessLevelList *list);
void maxcfg_ng_access_level_list_free(MaxCfgNgAccessLevelList *list);
MaxCfgStatus maxcfg_ng_access_level_list_add(MaxCfgNgAccessLevelList *list, const MaxCfgNgAccessLevel *lvl);

MaxCfgStatus maxcfg_ng_get_msg_areas(const MaxCfgToml *toml, const char *prefix,
                                    MaxCfgNgDivisionList *divisions, MaxCfgNgMsgAreaList *areas);
MaxCfgStatus maxcfg_ng_get_file_areas(const MaxCfgToml *toml, const char *prefix,
                                     MaxCfgNgDivisionList *divisions, MaxCfgNgFileAreaList *areas);
MaxCfgStatus maxcfg_ng_get_menu(const MaxCfgToml *toml, const char *prefix, MaxCfgNgMenu *menu);
MaxCfgStatus maxcfg_ng_get_access_levels(const MaxCfgToml *toml, const char *prefix, MaxCfgNgAccessLevelList *levels);

MaxCfgStatus maxcfg_ng_write_maximus_toml(FILE *fp, const MaxCfgNgSystem *sys);
MaxCfgStatus maxcfg_ng_write_general_session_toml(FILE *fp, const MaxCfgNgGeneralSession *session);
MaxCfgStatus maxcfg_ng_write_general_display_files_toml(FILE *fp, const MaxCfgNgGeneralDisplayFiles *files);
MaxCfgStatus maxcfg_ng_write_general_colors_toml(FILE *fp, const MaxCfgNgGeneralColors *colors);
MaxCfgStatus maxcfg_ng_write_matrix_toml(FILE *fp, const MaxCfgNgMatrix *matrix);
MaxCfgStatus maxcfg_ng_write_reader_toml(FILE *fp, const MaxCfgNgReader *reader);
MaxCfgStatus maxcfg_ng_write_equipment_toml(FILE *fp, const MaxCfgNgEquipment *equip);
MaxCfgStatus maxcfg_ng_write_protocols_toml(FILE *fp, const MaxCfgNgProtocolList *list);
MaxCfgStatus maxcfg_ng_write_language_toml(FILE *fp, const MaxCfgNgLanguage *lang);
MaxCfgStatus maxcfg_ng_write_menu_toml(FILE *fp, const MaxCfgNgMenu *menu);
MaxCfgStatus maxcfg_ng_write_msg_areas_toml(FILE *fp, const MaxCfgNgDivisionList *divisions, const MaxCfgNgMsgAreaList *areas);
MaxCfgStatus maxcfg_ng_write_file_areas_toml(FILE *fp, const MaxCfgNgDivisionList *divisions, const MaxCfgNgFileAreaList *areas);
MaxCfgStatus maxcfg_ng_write_access_levels_toml(FILE *fp, const MaxCfgNgAccessLevelList *levels);

#ifdef __cplusplus
}
#endif

#endif
