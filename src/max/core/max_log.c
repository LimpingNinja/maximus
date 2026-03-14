/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
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

/**
 * @file  max_log.c
 * @brief State-machine-based login flow for Maximus BBS.
 *
 * Replaces the former monolithic Login()/GetName()/NewUser() chain with
 * a 12+2 state dispatch loop.  Each login phase is a named state with
 * defined transitions; the context struct lives entirely on the stack.
 *
 * Architecture: see plans/max-log-rewrite.md for full design rationale.
 */

/*# name=Log-on routines and new-user junk — state machine rewrite
*/

#define MAX_LANG_max_init
#define MAX_LANG_max_chat
#define MAX_LANG_max_main
#define MAX_INCL_COMMS

#include <stdio.h>
#include <string.h>
#include <mem.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>
#include <ctype.h>
#include "prog.h"
#include "cfg_consts.h"
#include "ffind.h"
#include "alc.h"
#include "max_msg.h"
#include "display.h"
#include "userapi.h"
#include "trackm.h"
#include "ued.h"
#include "md5.h"
#include "ui_field.h"

#ifdef EMSI
#include "emsi.h"
#endif

/* -------------------------------------------------------------------- */
/*  Forward declarations — retained helper functions                     */
/* -------------------------------------------------------------------- */

static void near Logo(char *key_info);
static void near Banner(void);
static int  near Find_User(char *username);
static void near doublecheck_ansi(void);
static void near doublecheck_rip(void);
static int  near checkterm(char *prompt, char *helpfile);
static int  near InvalidPunctuation(char *string);
static void near Calc_Timeoff(void);
static void near Check_For_User_On_Other_Node(void);
static void near Set_OnOffTime(void);
static void near Write_Active(void);

/* -------------------------------------------------------------------- */
/*  Static string constants                                              */
/* -------------------------------------------------------------------- */

static char szBadUserName[] = "%sbad_user";

#define USR_BLOCK 9

/* ================================================================== */
/*  Enums and types — login state machine                              */
/* ================================================================== */

/**
 * @brief Login flow states.
 *
 * Each state maps to exactly one handler function.  COMPLETE and
 * DISCONNECT are terminal states that end the dispatch loop.
 */
typedef enum login_state {
  LOGIN_STATE_INIT,           /**< Pre-login setup: time, term caps, baud, flow ctrl */
  LOGIN_STATE_LOGO_BANNER,    /**< Logo display, baud check, banner, chat status     */
  LOGIN_STATE_NAME_ENTRY,     /**< Prompt for name, single/full search, retry logic  */
  LOGIN_STATE_CONFIRM_NAME,   /**< "Is this you?" Y/N prompt                         */
  LOGIN_STATE_EXISTING_AUTH,  /**< Language switch, password loop for existing users  */
  LOGIN_STATE_NEW_SETUP,      /**< Blank user, punctuation/bad-word, set priv/class  */
  LOGIN_STATE_NEW_REGISTER,   /**< Application, city, alias, phone, pwd, write rec   */
  LOGIN_STATE_TERM_SETUP,     /**< Terminal config (ANSI/RIP/FSED/hotkeys)           */
  LOGIN_STATE_VALIDATE,       /**< Validate_Runtime_Settings black box               */
  LOGIN_STATE_SET_TIME,       /**< Set_OnOffTime, log time given                     */
  LOGIN_STATE_POST_LOGIN,     /**< 14-step post-login housekeeping (order is sacred) */
  LOGIN_STATE_COMPLETE,       /**< Terminal — login succeeded                        */
  LOGIN_STATE_DISCONNECT,     /**< Terminal — hangup/quit                            */
  LOGIN_STATE_COUNT           /**< Sentinel — array sizing                           */
} login_state_t;

/** @brief Handler result codes. */
typedef enum login_result {
  LOGIN_NEXT,    /**< Proceed to returned next_state */
  LOGIN_ABORT,   /**< Clean abort — routes to DISCONNECT */
  LOGIN_HANGUP   /**< Remote gone after mdm_hangup() */
} login_result_t;

/** @brief Value returned by every state handler. */
typedef struct login_step {
  login_result_t result;
  login_state_t  next;
} login_step_t;

/**
 * @brief Stack-allocated login context — no malloc.
 *
 * All buffers use existing BUFLEN/PATHLEN constants.
 * Config fields are cached once during INIT so handlers
 * never repeat ngcfg_get_*() calls.
 */
typedef struct login_ctx {
  /* -- state machine -- */
  login_state_t state;

  /* -- stack buffers -- */
  char fname[BUFLEN];          /**< First name input buffer              */
  char lname[BUFLEN];          /**< Last name input buffer               */
  char username[BUFLEN * 3];   /**< Full assembled name                  */
  char pwd[BUFLEN];            /**< Password input buffer                */
  char quest[PATHLEN];         /**< Scratch buffer for prompt formatting */

  /* -- retry counters -- */
  unsigned name_tries;         /**< Failed name entry attempts (max 3)   */
  unsigned pwd_tries;          /**< Failed password attempts             */

  /* -- flow flags -- */
  int  found_user;             /**< True if user record found in DB      */
  int  is_newuser;             /**< True if new registration             */
  int  newuser_mex_ran;        /**< True if new-user questionnaire ran   */
  int  newuser_mex_saved;      /**< True if questionnaire returned data  */
  dword newuser_answered_mask; /**< Explicit handled-field bitmask       */

  /* -- saved state -- */
  byte saved_help;             /**< Original usr.help before confirm     */
  char saved_bits;             /**< Original usr.bits before confirm     */

  /* -- passed from Login() -- */
  const char *key_info;        /**< Key info string                      */

  /* -- config cache (read once at INIT) -- */
  int  cfg_alias_system;       /**< general.session.alias_system         */
  int  cfg_ask_alias;          /**< general.session.ask_alias            */
  int  cfg_single_word;        /**< general.session.single_word_names    */
  int  cfg_ask_phone;          /**< general.session.ask_phone            */
  int  cfg_check_ansi;         /**< general.session.check_ansi           */
  int  cfg_check_rip;          /**< general.session.check_rip            */
  int  cfg_bounded_login;      /**< general.display.bounded_input_login  */
  int  cfg_bounded_newuser;    /**< general.display.bounded_input_newuser*/
  int  cfg_logon_priv;         /**< general.session.logon_priv           */
  int  cfg_min_logon_baud;     /**< general.session.min_logon_baud       */
  int  cfg_min_graphics_baud;  /**< general.session.min_graphics_baud    */
  int  cfg_min_rip_baud;       /**< general.session.min_rip_baud         */
  int  cfg_disable_magnet;     /**< general.session.disable_magnet       */
} login_ctx_t;

#define NEWUSER_ANSWER_NAME        0x00000001UL
#define NEWUSER_ANSWER_CITY        0x00000002UL
#define NEWUSER_ANSWER_ALIAS       0x00000004UL
#define NEWUSER_ANSWER_PHONE       0x00000008UL
#define NEWUSER_ANSWER_DATAPHONE   0x00000010UL
#define NEWUSER_ANSWER_SEX         0x00000020UL
#define NEWUSER_ANSWER_DOB         0x00000040UL
#define NEWUSER_ANSWER_ANSI        0x00000080UL
#define NEWUSER_ANSWER_RIP         0x00000100UL
#define NEWUSER_ANSWER_FSR         0x00000200UL
#define NEWUSER_ANSWER_IBMCHARS    0x00000400UL
#define NEWUSER_ANSWER_HOTKEYS     0x00000800UL


/* ================================================================== */
/*  PromptInput style constants                                        */
/* ================================================================== */

/** @brief Style for bounded login name fields (white on blue). */
static const ui_prompt_field_style_t login_name_style = {
  .prompt_attr = (byte)-1,      /* preserve current color */
  .field_attr  = 0x1f,          /* white on blue          */
  .fill_ch     = ' ',
  .flags       = 0,
  .start_mode  = UI_PROMPT_START_HERE
};

/** @brief Style for bounded password fields (masked, white on blue). */
static const ui_prompt_field_style_t login_pwd_style = {
  .prompt_attr = (byte)-1,
  .field_attr  = 0x1f,
  .fill_ch     = '.',
  .flags       = UI_EDIT_FLAG_MASK,
  .start_mode  = UI_PROMPT_START_HERE
};


/* ================================================================== */
/*  Handler forward declarations                                       */
/* ================================================================== */

typedef login_step_t (*login_handler_fn)(login_ctx_t *ctx);

static login_step_t handle_init(login_ctx_t *ctx);
static login_step_t handle_logo_banner(login_ctx_t *ctx);
static login_step_t handle_name_entry(login_ctx_t *ctx);
static login_step_t handle_confirm_name(login_ctx_t *ctx);
static login_step_t handle_existing_auth(login_ctx_t *ctx);
static login_step_t handle_new_setup(login_ctx_t *ctx);
static login_step_t handle_new_register(login_ctx_t *ctx);
static login_step_t handle_term_setup(login_ctx_t *ctx);
static login_step_t handle_validate(login_ctx_t *ctx);
static login_step_t handle_set_time(login_ctx_t *ctx);
static login_step_t handle_post_login(login_ctx_t *ctx);
static login_step_t handle_complete(login_ctx_t *ctx);
static login_step_t handle_disconnect(login_ctx_t *ctx);


/* ================================================================== */
/*  Dispatch table                                                     */
/* ================================================================== */

static const login_handler_fn login_handlers[LOGIN_STATE_COUNT] = {
  [LOGIN_STATE_INIT]          = handle_init,
  [LOGIN_STATE_LOGO_BANNER]   = handle_logo_banner,
  [LOGIN_STATE_NAME_ENTRY]    = handle_name_entry,
  [LOGIN_STATE_CONFIRM_NAME]  = handle_confirm_name,
  [LOGIN_STATE_EXISTING_AUTH] = handle_existing_auth,
  [LOGIN_STATE_NEW_SETUP]     = handle_new_setup,
  [LOGIN_STATE_NEW_REGISTER]  = handle_new_register,
  [LOGIN_STATE_TERM_SETUP]    = handle_term_setup,
  [LOGIN_STATE_VALIDATE]      = handle_validate,
  [LOGIN_STATE_SET_TIME]      = handle_set_time,
  [LOGIN_STATE_POST_LOGIN]    = handle_post_login,
  [LOGIN_STATE_COMPLETE]      = handle_complete,
  [LOGIN_STATE_DISCONNECT]    = handle_disconnect,
};


/* ================================================================== */
/*  Login() — entry point (dispatch loop)                              */
/* ================================================================== */

/**
 * @brief Main login entry point — state machine dispatch loop.
 *
 * Replaces the former monolithic Login()/GetName()/NewUser() chain.
 * The context struct is stack-allocated; no malloc/free in the flow.
 *
 * @param key_info  Key information string passed from startup.
 */
void Login(char *key_info)
{
  login_ctx_t ctx;
  memset(&ctx, 0, sizeof ctx);
  ctx.state    = LOGIN_STATE_INIT;
  ctx.key_info = key_info;

  for (;;)
  {
    login_step_t step = login_handlers[ctx.state](&ctx);

    if (step.result == LOGIN_HANGUP)
      return;                             /* remote is gone */

    if (step.result == LOGIN_ABORT)
    {
      ctx.state = LOGIN_STATE_DISCONNECT;
      continue;
    }

    ctx.state = step.next;

    if (ctx.state == LOGIN_STATE_COMPLETE ||
        ctx.state == LOGIN_STATE_DISCONNECT)
      break;
  }

  /* Run terminal states if we broke out */
  if (ctx.state == LOGIN_STATE_DISCONNECT)
    login_handlers[LOGIN_STATE_DISCONNECT](&ctx);
}


/* ================================================================== */
/*  State handlers (in enum order)                                     */
/* ================================================================== */


/* ------------------------------------------------------------------ */
/*  handle_init — pre-login setup                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief One-time pre-login setup: time calc, term caps, baud, flow.
 *
 * Caches all TOML config into ctx->cfg_* fields so handlers never
 * repeat ngcfg_get_*() calls.  Also calls ci_init() early to fix
 * a pre-existing bug where ci_ejectuser() could fire before ci_init().
 */
static login_step_t handle_init(login_ctx_t *ctx)
{
  NW(ctx);

  Calc_Timeoff();

  if (!local && !waitforcaller)
    logit(log_caller_bps, baud);

  caller_online = TRUE;

  if (!local)
    mdm_baud(current_baud);

  Mdm_Flow_On();

  /* Fix pre-existing bug: ci_init() must run before any path that
   * may call ci_ejectuser() (e.g. name retry limit in NAME_ENTRY). */
  ci_init();

  /* Cache config values once — config is read-only during login */
  ctx->cfg_alias_system      = ngcfg_get_bool("general.session.alias_system");
  ctx->cfg_ask_alias         = ngcfg_get_bool("general.session.ask_alias");
  ctx->cfg_single_word       = ngcfg_get_bool("general.session.single_word_names");
  ctx->cfg_ask_phone         = ngcfg_get_bool("general.session.ask_phone");
  ctx->cfg_check_ansi        = ngcfg_get_bool("general.session.check_ansi");
  ctx->cfg_check_rip         = ngcfg_get_bool("general.session.check_rip");
  ctx->cfg_bounded_login     = ngcfg_get_bool("general.display.general.bounded_input_login");
  ctx->cfg_bounded_newuser   = ngcfg_get_bool("general.display.general.bounded_input_newuser");
  ctx->cfg_logon_priv        = ngcfg_get_int("general.session.logon_priv");
  ctx->cfg_min_logon_baud    = ngcfg_get_int("general.session.min_logon_baud");
  ctx->cfg_min_graphics_baud = ngcfg_get_int("general.session.min_graphics_baud");
  ctx->cfg_min_rip_baud      = ngcfg_get_int("general.session.min_rip_baud");
  ctx->cfg_disable_magnet    = ngcfg_get_bool("general.session.disable_magnet");

  return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_LOGO_BANNER };
}


/* ------------------------------------------------------------------ */
/*  handle_logo_banner — logo, min baud check, banner                 */
/* ------------------------------------------------------------------ */

/**
 * @brief Display system logo, enforce minimum baud, show banner file.
 *
 * Combines the former Logo(), baud check, and Banner() into one state.
 */
static login_step_t handle_logo_banner(login_ctx_t *ctx)
{
  /* Logo display for remote callers */
  if (!local)
    Logo((char *)ctx->key_info);

  /* Minimum baud rate enforcement */
  if (!local)
  {
    if (baud < (dword)ctx->cfg_min_logon_baud)
    {
      { char _a[16], _b[16];
        snprintf(_a, sizeof(_a), "%lu", (unsigned long)baud);
        snprintf(_b, sizeof(_b), "%lu", (unsigned long)ctx->cfg_min_logon_baud);
        logit(ltooslow, _a, _b); }
      Display_File(0, NULL, ngcfg_get_path("general.display_files.too_slow"));
      mdm_hangup();
      return (login_step_t){ LOGIN_HANGUP, LOGIN_STATE_DISCONNECT };
    }
  }

  /* Banner display */
  Banner();

  strcpy(usrname, us_short);
  ChatSetStatus(FALSE, cs_logging_on);

  return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_NAME_ENTRY };
}


/* ------------------------------------------------------------------ */
/*  handle_name_entry — name prompt, search, retry                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Name entry with internal retry loop.
 *
 * Handles first name prompt (alias in YES/YES mode), single-name
 * search, optional last name prompt, full-name search, and the
 * not-found display.  Self-loops on retry via the dispatch table.
 */
static login_step_t handle_name_entry(login_ctx_t *ctx)
{
  /* Retry limit check */
  if (!local && ctx->name_tries > 3)
  {
    ci_ejectuser();
    logit(too_many_attempts);
    Puts(too_many_attempts);
    mdm_hangup();
    return (login_step_t){ LOGIN_HANGUP, LOGIN_STATE_DISCONNECT };
  }

  ctx->name_tries++;

  /* Clear buffers */
  *ctx->fname = '\0';
  *ctx->lname = '\0';
  *ctx->username = '\0';

  ctx->found_user = FALSE;

  if (!*linebuf)
    Putc('\n');

  /* Prompt based on alias mode:
   *   YES/YES (alias_system && ask_alias): use enter_name
   *   Others: use what_first_name with blank_str param
   */
  if (ctx->cfg_alias_system && ctx->cfg_ask_alias)
  {
    PromptInput("general.display.general.bounded_input_login",
                enter_name, ctx->fname, 35, 35, &login_name_style);
  }
  else
  {
    LangSprintf(ctx->quest, sizeof ctx->quest, what_first_name, blank_str);
    PromptInput("general.display.general.bounded_input_login",
                ctx->quest, ctx->fname, 35, 35, &login_name_style);
  }

  /* Empty input: local quits, remote retries */
  if (!*ctx->fname)
  {
    if (local)
      return (login_step_t){ LOGIN_ABORT, LOGIN_STATE_DISCONNECT };
    else
      return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_NAME_ENTRY };
  }

  /* Check if linebuf has a stacked Y/N answer — if so, skip single-name find */
  if (!(((byte)toupper(*linebuf) == YES || (byte)toupper(*linebuf) == NO) &&
      (strpbrk(linebuf, cmd_delim) == linebuf + 1 || !linebuf[1])))
  {
    /* Try single-name search before asking for last name */
    if (!*linebuf)
    {
      strncpy(ctx->username, ctx->fname, 35);
      ctx->username[35] = '\0';
      fancier_str(ctx->username);

      ctx->found_user = Find_User(ctx->username);
    }

    /* If not found, ask for last name (unless single_word mode) */
    if (!ctx->found_user && !ctx->cfg_single_word)
    {
      LangSprintf(ctx->quest, sizeof ctx->quest, what_last_name,
                  ctx->cfg_alias_system ? s_alias : blank_str);

      PromptInput("general.display.general.bounded_input_login",
                  ctx->quest, ctx->lname, 35, 35, &login_name_style);
    }
  }

  /* Assemble full name */
  snprintf(ctx->username, sizeof ctx->username, "%s%s%s",
           ctx->fname, *ctx->lname ? " " : blank_str, ctx->lname);

  /* Consume additional name words from linebuf (multi-word names) */
  while (*linebuf &&
         !(((byte)toupper(*linebuf) == YES || (byte)toupper(*linebuf) == NO) &&
           (strpbrk(linebuf, cmd_delim) == linebuf + 1 || !linebuf[1])))
  {
    strcat(ctx->username, " ");
    InputGetsWNH(ctx->username + strlen(ctx->username), blank_str);
  }

  ctx->username[35] = '\0';
  fancier_str(ctx->username);

  /* Full-name search if not already found */
  if (!ctx->found_user)
    ctx->found_user = Find_User(ctx->username);

  /* Not found: show display file or self-loop if incomplete */
  if (!ctx->found_user)
  {
    *linebuf = '\0';

    /* If no last name given and multi-word required, restart */
    if (!*ctx->lname && !ctx->cfg_single_word)
      return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_NAME_ENTRY };

    /* Display not-found file and continue to confirm */
    strcpy(usr.name, ctx->username);
    SetUserName(&usr, usrname);

    Display_File(0, NULL, ngcfg_get_path("general.display_files.not_found"));
  }

  return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_CONFIRM_NAME };
}


/* ------------------------------------------------------------------ */
/*  handle_confirm_name — "Is this correct?" Y/N                      */
/* ------------------------------------------------------------------ */

/**
 * @brief Confirm the entered name with the user.
 *
 * Skipped for remote found users (they go straight to auth).
 * Always shown for local console and new users.
 */
static login_step_t handle_confirm_name(login_ctx_t *ctx)
{
  /* Save original help/bits for the confirm prompt */
  ctx->saved_help = usr.help;
  ctx->saved_bits = usr.bits;
  usr.help = NOVICE;
  usr.bits &= ~BITS_HOTKEYS;

  /* Remote + found: skip confirmation, go straight to auth */
  if (!local && ctx->found_user)
  {
    usr.help = ctx->saved_help;
    usr.bits = ctx->saved_bits;
    return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_EXISTING_AUTH };
  }

  if (!*linebuf)
    Putc('\n');

  if (GetYnAnswer(ctx->username, 0) == NO)
  {
    if (!local)
      logit(brain_lapse, ctx->username);

    Blank_User(&usr);

    usr.help = ctx->saved_help;
    usr.bits = ctx->saved_bits;

    return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_NAME_ENTRY };
  }

  /* Restore original help/bits */
  usr.help = ctx->saved_help;
  usr.bits = ctx->saved_bits;

  if (ctx->found_user)
    return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_EXISTING_AUTH };
  else
    return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_NEW_SETUP };
}


/* ------------------------------------------------------------------ */
/*  handle_existing_auth — language switch + password loop             */
/* ------------------------------------------------------------------ */

/**
 * @brief Authenticate an existing user: switch language, verify password.
 *
 * Re-initializes ci with the identified user, switches to the user's
 * language, then runs the password verification loop.
 */
static login_step_t handle_existing_auth(login_ctx_t *ctx)
{
  int fMatch;

  /* Re-init caller info with identified user */
  ci_init();

  logit(so_and_so_calling, ctx->username);

  if (*usr.alias && !eqstri(usr.name, usr.alias))
  {
    if (eqstri(ctx->username, usr.alias))
      logit(so_and_so_realname, usr.name);
    else
      logit(so_and_so_alias, usr.alias);
  }

  Switch_To_Language();
  Set_Lang_Alternate(hasRIP());

  /* Password verification loop */
  ctx->pwd_tries = 0;

  do
  {
    if (++ctx->pwd_tries != 1)
    {
      Clear_KBuffer();
      this_logon_bad = TRUE;

      logit(log_bad_pwd);
      { char _tb[16];
        snprintf(_tb, sizeof(_tb), "%u", ctx->pwd_tries - 1);
        LangPrintf(wrong_pwd, _tb); }

      if (ctx->pwd_tries == 6)
      {
        ci_ejectuser();
        logit(l_invalid_pwd);
        Puts(invalid_pwd);

        Display_File(0, NULL, "%sbad_pwd",
                     (char *)ngcfg_get_string_raw("maximus.display_path"));
        mdm_hangup();
        return (login_step_t){ LOGIN_HANGUP, LOGIN_STATE_DISCONNECT };
      }
    }

    *ctx->pwd = '\0';

    /* Check if password is required (non-guest account) */
#ifdef CANENCRYPT
    if (*usr.pwd || (usr.bits & BITS_ENCRYPT))
#else
    if (*usr.pwd)
#endif
    {
      if (!*linebuf)
        Putc('\n');

      PromptInput("general.display.general.bounded_input_login",
                  usr_pwd, ctx->pwd, 15, 15, &login_pwd_style);
    }

    /* Guest account handling: empty password matches trivially */
#ifdef CANENCRYPT
    if (*usr.pwd == 0 && (usr.bits & BITS_ENCRYPT) == 0)
#else
    if (*usr.pwd == 0)
#endif
    {
      /* Reset guest stats, preserve width/len */
      usr.bits  &= ~BITS_RIP;
      usr.bits2 |= BITS2_MORE | BITS2_CLS;
      usr.bits2 &= ~BITS2_CONFIGURED;
      usr.time   = 0;
      usr.call   = 0;
      usr.down   = 0L;
      usr.downtoday = 0L;
      usr.up     = ultoday = 0L;
      usr.nup    = usr.ndown = usr.ndowntoday = 0L;
      usr.width  = 80;
      usr.len    = 24;
    }

    /* Password comparison */
#ifdef CANENCRYPT
    if (usr.bits & BITS_ENCRYPT)
    {
      byte abMd5[MD5_SIZE];

      string_to_MD5(strlwr(ctx->pwd), abMd5);
      fMatch = (memcmp(abMd5, usr.pwd, MD5_SIZE) == 0);
    }
    else
#endif
      fMatch = (stricmp(cfancy_str(ctx->pwd), usr.pwd) == 0);
  }
  while (!fMatch);

  this_logon_bad = FALSE;

  /* Check configured status → route to appropriate term state */
  if ((usr.bits2 & BITS2_CONFIGURED) == 0)
  {
    if (*ngcfg_get_string_raw("general.display_files.not_configured"))
      Display_File(0, NULL, ngcfg_get_path("general.display_files.not_configured"));

    /* Test again — display file might have toggled it */
    if ((usr.bits2 & BITS2_CONFIGURED) == 0)
      return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_TERM_SETUP };
  }

  if (!local)
    return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_TERM_SETUP };

  return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_VALIDATE };
}


/* ------------------------------------------------------------------ */
/*  handle_new_setup — new user validation and initial record setup    */
/* ------------------------------------------------------------------ */

/**
 * @brief Validate the new username, check bad words, set up blank user.
 *
 * Performs punctuation check, bad word check, initializes the user
 * record with appropriate privilege and lastread pointer.
 */
static login_step_t handle_new_setup(login_ctx_t *ctx)
{
  HUF huf;

  /* Re-init caller info with new user identity */
  ci_init();

  /* Punctuation check */
  if (InvalidPunctuation(ctx->username))
  {
    Clear_KBuffer();
    Puts(invalid_punct);
    return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_NAME_ENTRY };
  }

  logit(so_and_so_calling, ctx->username);
  logit(log_not_in_ulist, ctx->username);

  /* Bad word check — may hangup internally and never return */
  Bad_Word_Check(ctx->username);

  Clear_KBuffer();

  /* Initialize blank user record */
  Blank_User(&usr);

  /* Set name/alias based on alias mode */
  if (ctx->cfg_alias_system && ctx->cfg_ask_alias)
  {
    /* YES/YES: login input is alias, real name asked in NEW_REGISTER */
    strncpy(usr.alias, ctx->username, sizeof(usr.alias) - 1);
    usr.alias[sizeof(usr.alias) - 1] = '\0';
    strcpy(usr.name, ctx->username);
  }
  else
  {
    /* YES/NO or NO/NO: login input is real name */
    strcpy(usr.name, ctx->username);
  }
  SetUserName(&usr, usrname);

  if (create_userbbs)
  {
    /* Sysop creation mode */
    usr.priv = (word)ClassGetInfo(ClassLevelIndex((word)-2), CIT_LEVEL);
    usr.lastread_ptr = 0;
    usr.credit = 65500u;
  }
  else
  {
    usr.priv = ctx->cfg_logon_priv;

    /* Get a preliminary lastread pointer */
    if ((huf = UserFileOpen((char *)ngcfg_get_path("maximus.file_password"), 0)) == NULL)
    {
      cant_open((char *)ngcfg_get_path("maximus.file_password"));
      quit(ERROR_FILE);
    }

    usr.lastread_ptr = Find_Next_Lastread(huf);
    UserFileClose(huf);
  }

  Find_Class_Number();

  /* Check preregistered system */
  logit(log_applic);

  if (ctx->cfg_logon_priv == PREREGISTERED && !create_userbbs)
  {
    if (*ngcfg_get_path("general.display_files.application"))
      Display_File(0, NULL, ngcfg_get_path("general.display_files.application"));
    else
      Puts(pvt_system);

    mdm_hangup();
    return (login_step_t){ LOGIN_HANGUP, LOGIN_STATE_DISCONNECT };
  }

  ctx->is_newuser = TRUE;

  return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_NEW_REGISTER };
}


/* ------------------------------------------------------------------ */
/*  handle_new_register — registration data, password, write record   */
/* ------------------------------------------------------------------ */

/**
 * @brief Full new-user registration flow.
 *
 * Displays application file, collects city/alias/phone, sets first-call
 * date, displays new_user1, creates password (at END per compatibility),
 * and writes the user record to disk.
 */
static login_step_t handle_new_register(login_ctx_t *ctx)
{
  HUF huf;
  const char *newuser_mex;
  int mex_rc;

  NW(ctx);

  /* Display application file */
  if (*ngcfg_get_path("general.display_files.application"))
    Display_File(0, NULL, ngcfg_get_path("general.display_files.application"));

  ctx->newuser_mex_ran = FALSE;
  ctx->newuser_mex_saved = FALSE;
  ctx->newuser_answered_mask = 0;

  newuser_mex = ngcfg_get_path("general.display_files.newuser_mex");
  if (newuser_mex && *newuser_mex)
  {
    MexSetNewUserAnsweredMask(0);
    mex_rc = Mex((char *)newuser_mex);
    ctx->newuser_mex_ran = (mex_rc >= 0);
    ctx->newuser_answered_mask = MexGetNewUserAnsweredMask();
    ctx->newuser_mex_saved = (ctx->newuser_answered_mask != 0);
    logit("@newuser_mex path='%s' rc=%d ran=%d saved=%d mask=0x%08lx",
          newuser_mex,
          mex_rc,
          ctx->newuser_mex_ran,
          ctx->newuser_mex_saved,
          (unsigned long)ctx->newuser_answered_mask);
    SetUserName(&usr, usrname);
    Set_Lang_Alternate(hasRIP());
  }

  /* YES/YES mode: ask for real name (alias already set in NEW_SETUP) */
  if (ctx->cfg_alias_system && ctx->cfg_ask_alias &&
      !(ctx->newuser_answered_mask & NEWUSER_ANSWER_NAME))
  {
    char realname[PATHLEN];

    realname[0] = '\0';
    WhiteN();
    LangSprintf(ctx->quest, sizeof ctx->quest, what_first_name, s_alias);
    PromptInput("general.display.general.bounded_input_newuser",
                ctx->quest, realname, 35, sizeof(usr.name) - 1, &login_name_style);

    if (*realname)
    {
      fancier_str(realname);
      strcpy(usr.name, realname);
    }
    else
    {
      /* Use alias as real name if not provided */
      strcpy(usr.name, ctx->username);
    }
  }

  /* Collect city */
  if (!(ctx->newuser_answered_mask & NEWUSER_ANSWER_CITY))
    Chg_City();

  /* Alias handling by mode */
  if (!ctx->cfg_ask_alias)
  {
    *usr.alias = '\0';
  }
  else if (ctx->cfg_alias_system && ctx->cfg_ask_alias)
  {
    /* YES/YES: alias already set from login, just validate */
    Bad_Word_Check(usr.alias);
  }
  else
  {
    /* Other modes with Ask Alias: prompt for alias unless MEX already did it */
    if (!(ctx->newuser_answered_mask & NEWUSER_ANSWER_ALIAS))
      Chg_Alias();
    Bad_Word_Check(usr.alias);
  }

  /* Phone number */
  if (ctx->cfg_ask_phone && !(ctx->newuser_answered_mask & NEWUSER_ANSWER_PHONE))
    Chg_Phone();
  else if (!ctx->cfg_ask_phone)
    *usr.phone = '\0';

  /* First call date */
  Get_Dos_Date(&usr.date_1stcall);

  /* Display new_user1 informational screen */
  Display_File(0, NULL, ngcfg_get_path("general.display_files.new_user1"));

  /* Password creation — at END per compatibility-first decision */
  Get_Pwd();

  /* Write user record to disk */
  if ((huf = UserFileOpen((char *)ngcfg_get_path("maximus.file_password"), 0)) == NULL)
  {
    cant_open((char *)ngcfg_get_path("maximus.file_password"));
    quit(ERROR_FILE);
  }

  /* Get a good lastread pointer (second call for concurrency safety) */
  usr.lastread_ptr = Find_Next_Lastread(huf);

  if (!UserFileCreateRecord(huf, &usr, TRUE))
    logit(cantwrite, (char *)ngcfg_get_path("maximus.file_password"));

  origusr = usr;

  UserFileClose(huf);

  /* Re-init caller info after record creation */
  ci_init();

  return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_TERM_SETUP };
}


/* ------------------------------------------------------------------ */
/*  handle_term_setup — terminal config (ANSI/RIP/FSED/hotkeys)       */
/* ------------------------------------------------------------------ */

/**
 * @brief Terminal capability configuration.
 *
 * For unconfigured users: full config flow (ANSI, RIP, FSED, IBM PC,
 * hotkeys) using autodetect and GetListAnswer/GetYnhAnswer prompts.
 * For configured remote users: doublecheck_ansi() / doublecheck_rip().
 */
static login_step_t handle_term_setup(login_ctx_t *ctx)
{
  NW(ctx);

  if (!(usr.bits2 & BITS2_CONFIGURED))
  {
    /* Full terminal config flow — inlined from Get_AnsiMagnEt() */
    char string[PATHLEN];
    int x;

    Clear_KBuffer();

    if (local || baud >= (dword)ctx->cfg_min_graphics_baud)
    {
      /* --- ANSI detection and prompt --- */
      NoWhiteN();
      sprintf(string, "%swhy_ansi",
              (char *)ngcfg_get_path("maximus.display_path"));

      x = autodetect_ansi();

      if (!*linebuf)
        Puts(get_ansi1);

      if (!(ctx->newuser_answered_mask & NEWUSER_ANSWER_ANSI))
      {
        if (GetListAnswer(x ? CYnq : yCNq, string, useyforyes, 0,
                          get_ansi2) == YES)
        {
          usr.video = GRAPH_ANSI;
          usr.bits |= BITS_FSR;
        }
        else
        {
          usr.video = GRAPH_TTY;
          usr.bits &= ~BITS_FSR;
          usr.bits2 |= BITS2_BORED;
        }
      }

      if (!(ctx->newuser_answered_mask & NEWUSER_ANSWER_RIP))
        usr.bits &= ~BITS_RIP;

      /* --- RIP detection and prompt --- */
      if ((local || baud >= (dword)ctx->cfg_min_rip_baud) &&
          !(ctx->newuser_answered_mask & NEWUSER_ANSWER_RIP))
      {
        NoWhiteN();
        sprintf(string, "%swhy_rip",
                (char *)ngcfg_get_path("maximus.display_path"));

        x = autodetect_rip();

        if (GetListAnswer(x ? CYnq : yCNq, string, useyforyes, 0,
                          get_rip) == YES)
        {
          usr.bits  |= (BITS_RIP | BITS_FSR | BITS_HOTKEYS);
          usr.bits2 |= BITS2_CLS;

          if (!x)
          {
            /* User claims RIP but autodetect failed — double-check */
            logit(log_rip_enabled_ndt);
            doublecheck_rip();
          }
        }
      }

      /* --- Full-screen editor prompt (ANSI users only) --- */
      if (usr.video != GRAPH_TTY)
      {
        if (!(ctx->newuser_answered_mask & NEWUSER_ANSWER_FSR))
        {
          sprintf(string, "%swhy_fsed",
                  (char *)ngcfg_get_path("maximus.display_path"));

          NoWhiteN();

          if (GetYnhAnswer(string, get_fsed, 0) == YES)
          {
            usr.bits2 &= ~BITS2_BORED;
            usr.bits  |= BITS_FSR;
          }
          else
          {
            usr.bits2 |= BITS2_BORED;
            if (!(usr.bits & BITS_RIP))
              usr.bits &= ~BITS_FSR;
          }
        }
      }

      /* --- IBM PC character set prompt --- */
      if (!(ctx->newuser_answered_mask & NEWUSER_ANSWER_IBMCHARS))
      {
        sprintf(string, "%swhy_pc",
                (char *)ngcfg_get_path("maximus.display_path"));

        NoWhiteN();

        if (GetYnhAnswer(string, get_ibmpc, 0) == YES)
          usr.bits2 |= BITS2_IBMCHARS;
        else
          usr.bits2 &= ~BITS2_IBMCHARS;
      }

      /* --- Hotkeys prompt (non-RIP users) --- */
      if (usr.bits & BITS_RIP)
      {
        usr.bits  |= BITS_HOTKEYS | BITS_FSR;
        usr.bits2 |= BITS2_CLS;
      }
      else if (!(ctx->newuser_answered_mask & NEWUSER_ANSWER_HOTKEYS))
      {
        NoWhiteN();

        sprintf(string, "%swhy_hot",
                (char *)ngcfg_get_path("maximus.display_path"));

        if (GetYnhAnswer(string, get_hotkeys, 0) == YES)
          usr.bits |= BITS_HOTKEYS;
        else
          usr.bits &= ~BITS_HOTKEYS;
      }

      Set_Lang_Alternate(hasRIP());

      usr.bits2 |= BITS2_CONFIGURED;
    }
    else
    {
      /* Too slow for graphics — force TTY */
      usr.video  = GRAPH_TTY;
      usr.bits  &= ~BITS_RIP;
      usr.bits2 |= BITS2_BORED;
    }
  }
  else if (!local)
  {
    /* Already configured — just doublecheck autodetection */
    doublecheck_ansi();
    doublecheck_rip();
  }

  return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_VALIDATE };
}


/* ------------------------------------------------------------------ */
/*  handle_validate — runtime settings validation                     */
/* ------------------------------------------------------------------ */

/**
 * @brief Validate_Runtime_Settings black-box call.
 *
 * Includes class baud check, concurrent-login check, setting clamps,
 * and password encryption enforcement.
 */
static login_step_t handle_validate(login_ctx_t *ctx)
{
  NW(ctx);

  Validate_Runtime_Settings();

  return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_SET_TIME };
}


/* ------------------------------------------------------------------ */
/*  handle_set_time — session time limits                             */
/* ------------------------------------------------------------------ */

/**
 * @brief Calculate session time and check daily limits.
 *
 * Set_OnOffTime() may hangup internally if daily limit exceeded.
 */
static login_step_t handle_set_time(login_ctx_t *ctx)
{
  signed int left;

  NW(ctx);

  Set_OnOffTime();
  left = timeleft();
  { char _ib[16];
    snprintf(_ib, sizeof(_ib), "%d", left);
    logit(log_given, _ib); }

  NW(left);

  return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_POST_LOGIN };
}


/* ------------------------------------------------------------------ */
/*  handle_post_login — 14-step housekeeping (ORDER IS SACRED)        */
/* ------------------------------------------------------------------ */

/**
 * @brief Post-login housekeeping — the order of these 14 steps is sacred.
 *
 * InitTracker → fLoggedOn → Write_LastUser/Active → ci_login/loggedon →
 * tags → chat → attr → class file → bad logon → newfile date → welcome →
 * stats → BITS_TABS → time_warn.
 */
static login_step_t handle_post_login(login_ctx_t *ctx)
{
  /* Step 1: Initialize message tracker */
#ifdef MAX_TRACKER
  InitTracker();
#endif

  /* Step 2: Mark user as logged on */
  fLoggedOn = TRUE;

  /* Step 3: Write presence files */
  Write_LastUser();
  Write_Active();

  /* Step 4: Caller info log */
  ci_login();
  ci_loggedon();

  /* Step 5: Read tag file */
  TagReadTagFile(&mtm);

  /* Step 6: Chat availability */
  if (usr.bits & BITS_NOTAVAIL)
    ChatSetStatus(FALSE, cs_notavail);
  else
    ChatSetStatus(TRUE, cs_avail);

  /* Step 7: Reset display attributes */
  mdm_attr = curattr = -1;
  Puts(CYAN);

  /* Step 8: Class-specific login file */
  if (!ctx->is_newuser && *ClassFile(cls))
    Display_File(0, NULL, ClassFile(cls));

  /* Step 9: Bad logon flag from previous session */
  if (usr.bits2 & BITS2_BADLOGON)
  {
    Display_File(0, NULL, ngcfg_get_path("general.display_files.bad_logon"));
    usr.bits2 &= ~BITS2_BADLOGON;
  }

  /* Step 10: New-files date */
  date_newfile = usr.date_newfile;

  /* Step 11: Welcome file based on call count */
  switch (usr.times + 1)
  {
    case 1:
      Display_File(DISPLAY_PCALL, NULL,
                   ngcfg_get_path("general.display_files.new_user2"));
      break;

    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      Display_File(DISPLAY_PCALL, NULL,
                   ngcfg_get_path("general.display_files.rookie"));
      break;

    default:
      Display_File(DISPLAY_PCALL, NULL,
                   ngcfg_get_path("general.display_files.welcome"));
      break;
  }

  /* Step 12: Update call statistics */
  bstats.today_callers++;
  bstats.num_callers++;
  usr.times++;

  /* Step 13: Clear tab expansion bit */
  usr.bits &= ~BITS_TABS;

  /* Step 14: Time warning display */
  Display_File(0, NULL, ngcfg_get_path("general.display_files.time_warn"));

  return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_COMPLETE };
}


/* ------------------------------------------------------------------ */
/*  handle_complete — terminal state (login succeeded)                */
/* ------------------------------------------------------------------ */

/**
 * @brief Terminal state — dispatch loop breaks on COMPLETE.
 */
static login_step_t handle_complete(login_ctx_t *ctx)
{
  NW(ctx);
  return (login_step_t){ LOGIN_NEXT, LOGIN_STATE_COMPLETE };
}


/* ------------------------------------------------------------------ */
/*  handle_disconnect — terminal state (hangup/quit)                  */
/* ------------------------------------------------------------------ */

/**
 * @brief Clean disconnect: quit(0) for local, mdm_hangup() for remote.
 */
static login_step_t handle_disconnect(login_ctx_t *ctx)
{
  NW(ctx);

  if (local)
    quit(0);
  else
    mdm_hangup();

  return (login_step_t){ LOGIN_HANGUP, LOGIN_STATE_DISCONNECT };
}


/* ================================================================== */
/*  Retained helper functions (verbatim from baseline)                 */
/* ================================================================== */


/* ------------------------------------------------------------------ */
/*  Logo() — display system identification string                     */
/* ------------------------------------------------------------------ */

static void near Logo(char *key_info)
{
#ifndef KEY
  NW(key_info);
#endif

  mdm_dump(DUMP_INPUT);

  /* Only output newline for modem/serial - socket connections are clean */
  if (!tcpip)
    Putc('\n');

#ifdef EMSI
  EmsiTxFrame(EMSI_IRQ);
#endif

  Printf("%s v%s ", name, version);

#ifdef KEY
  Printf("\nSystem: %s\n"
           " SysOp: %s",
         key_info + strlen(key_info) + 1,
         key_info);
#endif
}


/* ------------------------------------------------------------------ */
/*  Banner() — display banner/logo file                               */
/* ------------------------------------------------------------------ */

static void near Banner(void)
{
  if ((!*linebuf && !local) || eqstri(linebuf, "-"))
  {
    *linebuf = '\0';
    Display_File(0, NULL, ngcfg_get_path("general.display_files.logo"));
  }
  else if (!*linebuf)
    strcpy(linebuf, ngcfg_get_string("maximus.sysop"));
}


/* ------------------------------------------------------------------ */
/*  Find_User() — search user database by name and alias              */
/* ------------------------------------------------------------------ */

static int near Find_User(char *username)
{
  HUF huf;
  int mode;
  int ret;

  mode = create_userbbs ? O_CREAT : 0;

  if ((huf = UserFileOpen((char *)ngcfg_get_path("maximus.file_password"), mode)) == NULL)
  {
    cant_open((char *)ngcfg_get_path("maximus.file_password"));
    Local_Beep(3);
    quit(ERROR_FILE);
  }

  if (!UserFileFind(huf, username, NULL, &usr) &&
      !UserFileFind(huf, NULL, username, &usr))
  {
    ret = FALSE;
  }
  else
  {
    if (usr.delflag & UFLAG_DEL)
      ret = FALSE;
    else
    {
      /* Found the user successfully */
      origusr = usr;
      SetUserName(&usr, usrname);
      ret = TRUE;
    }
  }

  UserFileClose(huf);
  return ret;
}


/* ------------------------------------------------------------------ */
/*  doublecheck_ansi() — verify ANSI after autodetect                 */
/* ------------------------------------------------------------------ */

/**
 * @brief Ensure that the user really does have ANSI graphics by trying to
 *        autodetect, and then warning the user if graphics are not found.
 */
static void near doublecheck_ansi(void)
{
  if (ngcfg_get_bool("general.session.check_ansi") &&
      (usr.video == GRAPH_ANSI) &&
      !autodetect_ansi() &&
      checkterm(check_ansi, "why_ansi"))
  {
    usr.video = GRAPH_TTY;
    usr.bits &= ~BITS_RIP;
    SetTermSize(0, 0);
  }
}


/* ------------------------------------------------------------------ */
/*  doublecheck_rip() — verify RIP after autodetect                   */
/* ------------------------------------------------------------------ */

/**
 * @brief Ensure that the user really does have RIP graphics by trying to
 *        autodetect, and then warning the user if graphics are not found.
 */
static void near doublecheck_rip(void)
{
  if (ngcfg_get_bool("general.session.check_rip") &&
      (usr.bits & BITS_RIP) &&
      !autodetect_rip() &&
      checkterm(check_rip, "why_rip"))
  {
    usr.bits &= ~BITS_RIP;
    SetTermSize(0, 0);
  }

  Set_Lang_Alternate(hasRIP());
}


/* ------------------------------------------------------------------ */
/*  checkterm() — helper for doublecheck_ansi/rip                     */
/* ------------------------------------------------------------------ */

static int near checkterm(char *prompt, char *helpfile)
{
  char string[PATHLEN];

  sprintf(string, ss, (char *)ngcfg_get_string_raw("maximus.display_path"), helpfile);

  NoWhiteN();
  return (GetYnhAnswer(string, prompt, 0) == YES);
}


/* ------------------------------------------------------------------ */
/*  InvalidPunctuation() — check logon name for bad characters        */
/* ------------------------------------------------------------------ */

static int near InvalidPunctuation(char *string)
{
  char *badstring;
  char *badalias;

  if (ngcfg_get_charset_int() == CHARSET_SWEDISH)
  {
    badstring = ",/=@#$%^&()";
    badalias  = "!*+:<>?~_";
  }
  else
  {
    badstring = "\",/\\[]=@#$%^&()";
    badalias  = "!*+:<>?{|}~_";
  }

  if (strpbrk(string, badstring) != NULL)
    return TRUE;
  else
  {
    if (ngcfg_get_bool("general.session.alias_system"))
      return FALSE;
    else
      return ((strpbrk(string, badalias) != NULL));
  }
}


/* ------------------------------------------------------------------ */
/*  Bad_Word_Check() — check name against baduser.bbs                 */
/* ------------------------------------------------------------------ */

int Bad_Word_Check(char *username)
{
  FILE *baduser;

  char usrword[PATHLEN];
  char fname[PATHLEN];
  char line[PATHLEN];
  char *p;

  sprintf(fname, "%sbaduser.bbs", original_path);

  /* If it's not there and can't be opened, don't worry about it */

  if ((baduser = shfopen(fname, fopen_read, O_RDONLY)) == NULL)
    return FALSE;

  while (fgets(line, PATHLEN, baduser))
  {
    Trim_Line(line);

    if (*line == '\0' || *line == ';')
      continue;

    strcpy(usrword, username);

    p = strtok(usrword, cmd_delim);

    while (p)
    {
      /* '~' means 'name contains' rather than finding a word match */

      if ((*line == '~' && (stristr(p, line + 1) || stristr(username, line + 1))) ||
          (*line != '~' && (eqstri(p, line) || eqstri(username, line))))
      {
        fclose(baduser);

        /* Log the problem and hang up */

        logit(bad_uword);
        ci_ejectuser();
        Display_File(0, NULL, szBadUserName,
                     (char *)ngcfg_get_string_raw("maximus.display_path"));
        mdm_hangup();

        /* This should never return, but... */

        return TRUE;
      }

      p = strtok(NULL, cmd_delim);
    }
  }

  fclose(baduser);

  return FALSE;
}


/* ------------------------------------------------------------------ */
/*  Get_Pwd() — password creation for new users                       */
/* ------------------------------------------------------------------ */

void Get_Pwd(void)
{
  char got[PATHLEN];
  char check[PATHLEN];

  do
  {
    Clear_KBuffer();
    got[0] = '\0';

    PromptInput("general.display.general.bounded_input_newuser",
                get_pwd1, got, 15, BUFLEN, &login_pwd_style);

    if (strlen(got) < 4 || strlen(got) > 15 || strpbrk(got, cmd_delim))
    {
      Puts(bad_pwd1);
      *got = 0;
      continue;
    }

    Clear_KBuffer();
    check[0] = '\0';

    PromptInput("general.display.general.bounded_input_newuser",
                check_pwd2, check, 15, BUFLEN, &login_pwd_style);

    if (!eqstri(got, check))
    {
      LangPrintf(bad_pwd2, got, check);
      *got = 0;
    }
    else
    {
#if defined(CHANGEENCRYPT) || defined(MUSTENCRYPT)
      usr.bits &= ~BITS_ENCRYPT;
      if (!ngcfg_get_bool("maximus.no_password_encryption"))
      {
        byte abMd5[MD5_SIZE];

        string_to_MD5(strlwr(got), abMd5);

        memcpy(usr.pwd, abMd5, sizeof usr.pwd);
        usr.bits |= BITS_ENCRYPT;
      }
      else
#endif
        strcpy(usr.pwd, cfancy_str(got));

      Get_Dos_Date(&usr.date_pwd_chg);
    }
  }
  while (*got == '\0');
}


/* ------------------------------------------------------------------ */
/*  Calc_Timeoff() — compute session time limit                       */
/* ------------------------------------------------------------------ */

static void near Calc_Timeoff(void)
{
  word mins;

  dword min_1;
  dword min_2;
  dword min_3;

  /* Our time limit is the SMALLEST out of the following three numbers:
   *
   * Our priv level's maximum time limit for each call
   * Our priv level's maximum time limit for each day, minus the amount
   * of time we've been on previously today,
   * The -t parameter specified on the command line
   */

  if (caller_online)
  {
    mins = (word)ClassGetInfo(cls, CIT_DAY_TIME) - usr.time + usr.time_added;
    mins = min(mins, (word)ClassGetInfo(cls, CIT_CALL_TIME));
  }
  else
    mins = (word)ngcfg_get_int("general.session.logon_timelimit");

  min_1 = timeon + (dword)(mins * 60L);
  min_2 = timestart + (dword)(max_time * 60L);

  if (usr.xp_flag & XFLAG_EXPMINS)
    min_3 = timeon + ((long)(usr.xp_mins + 1) * 60L);
  else
    min_3 = min_2;

  timeoff = min(min_1, min_2);
  timeoff = min(timeoff, min_3);
}


/* ------------------------------------------------------------------ */
/*  Validate_Runtime_Settings() — black-box validation                */
/* ------------------------------------------------------------------ */

void Validate_Runtime_Settings(void)
{
  BARINFO bi;
  int min_graphics;

  Find_Class_Number();

  /* Check overall lowest baud rate */

  if (!local && baud < (dword)ClassGetInfo(cls, CIT_MIN_BAUD))
  {
    { char _a[16], _b[16];
      snprintf(_a, sizeof(_a), "%lu", (unsigned long)baud);
      snprintf(_b, sizeof(_b), "%lu", (unsigned long)ClassGetInfo(cls, CIT_MIN_BAUD));
      logit(ltooslow, _a, _b); }

    Display_File(0, NULL, ngcfg_get_path("general.display_files.too_slow"));
    mdm_hangup();
  }

  Check_For_User_On_Other_Node();

  /* Validate run-time settings in case external program modified user file */

  min_graphics = ngcfg_get_int("general.session.min_graphics_baud");

  if (usr.video && baud < (dword)min_graphics && !local)
  {
    usr.video = GRAPH_TTY;
    usr.bits2 |= BITS2_BORED;
  }

  if (ngcfg_get_bool("general.session.disable_magnet"))
    usr.bits2 |= BITS2_BORED;

  if (usr.help != NOVICE && usr.help != REGULAR && usr.help != EXPERT)
    usr.help = NOVICE;

  if (usr.width < 20)
    usr.width = 20;

  if (usr.width > 132)
    usr.width = 132;

  if (usr.len < 8)
    usr.len = 8;

  if (usr.len > 200)
    usr.len = 200;

  if (usr.lang > ngcfg_get_int("general.language.max_lang"))
    usr.lang = 0;

  if (usr.bits & BITS_RIP)
  {
    usr.bits  |= (BITS_HOTKEYS | BITS_FSR);
    usr.bits2 |= BITS2_CLS;
  }

  if (usr.bits & BITS_FSR)
    usr.bits2 |= BITS2_MORE;

#ifdef MUSTENCRYPT
  /* Enforce password encryption if required */

  if (!ngcfg_get_bool("maximus.no_password_encryption") &&
      *usr.pwd && (usr.bits & BITS_ENCRYPT) == 0)
  {
    byte abMd5[MD5_SIZE];

    string_to_MD5(strlwr(usr.pwd), abMd5);

    memcpy(usr.pwd, abMd5, sizeof usr.pwd);
    usr.bits |= BITS_ENCRYPT;
  }
#endif

  /* Ensure valid file area */

  if (!ValidFileArea(usr.files, NULL, VA_VAL | VA_PWD | VA_EXTONLY, &bi))
  {
    char temp[PATHLEN];

    Parse_Outside_Cmd((char *)ngcfg_get_string("general.session.first_file_area"), temp);
    SetAreaName(usr.files, temp);
  }

  /* Ensure valid message area */

  if (!ValidMsgArea(usr.msg, NULL, VA_VAL | VA_PWD | VA_EXTONLY, &bi))
  {
    char temp[PATHLEN];

    Parse_Outside_Cmd((char *)ngcfg_get_string("general.session.first_message_area"), temp);
    SetAreaName(usr.msg, temp);
  }

  ForceGetFileArea();
  ForceGetMsgArea();
}


/* ------------------------------------------------------------------ */
/*  Check_For_User_On_Other_Node() — concurrent login check           */
/* ------------------------------------------------------------------ */

static void near Check_For_User_On_Other_Node(void)
{
  int lastuser;
  sword ret;

  struct _usr user;

  char fname[PATHLEN];

  unsigned int their_task;

  FFIND *ff;

  sprintf(fname, active_star, original_path);

  for (ff = FindOpen(fname, 0), ret = 0; ff && ret == 0; ret = FindNext(ff))
  {
    if (sscanf(cstrlwr(ff->szName), active_x_bbs, &their_task) != 1)
      continue;

    /* Don't process our own task number */

    if ((byte)their_task == task_num)
      continue;

    sprintf(fname,
            their_task ? lastusxx_bbs : lastuser_bbs,
            original_path, their_task);

    if ((lastuser = shopen(fname, O_RDONLY | O_BINARY)) == -1)
      continue;

    read(lastuser, (char *)&user, sizeof(struct _usr));
    close(lastuser);

    /* If we found this turkey on another node... */

    if (eqstri(user.name, usr.name))
    {
      Display_File(0, NULL, "%sACTIVE_2",
                   (char *)ngcfg_get_string_raw("maximus.display_path"));
      ci_ejectuser();
      mdm_hangup();
    }
  }

  FindClose(ff);
}


/* ------------------------------------------------------------------ */
/*  Set_OnOffTime() — daily limits and time calculation                */
/* ------------------------------------------------------------------ */

static void near Set_OnOffTime(void)
{
  union stamp_combo today;

  Get_Dos_Date(&today);

  /* If the user has an expiry date, and that date has passed... */

  if ((usr.xp_flag & XFLAG_EXPDATE) && GEdate(&today, &usr.xp_date))
    Xpired(REASON_DATE);

  timeon = time(NULL);

  /* Get today's date for next_ludate */

  next_ludate = today;

  /* If different day, reset daily quotas */

  if (usr.ludate.dos_st.date != next_ludate.dos_st.date)
  {
    usr.time       = 0;
    usr.call       = 0;
    usr.time_added = 0;
    usr.downtoday  = 0;
    usr.ndowntoday = 0;
  }

  if (usr.time >= (word)ClassGetInfo(cls, CIT_DAY_TIME) ||
      usr.call >= (word)ClassGetInfo(cls, CIT_MAX_CALLS))
  {
    do_timecheck = FALSE;

    logit(log_exc_daylimit);
    Display_File(0, NULL, ngcfg_get_path("general.display_files.day_limit"));

    { char _t1[16], _t2[16];
      snprintf(_t1, sizeof(_t1), "%d", ClassGetInfo(cls, CIT_CALL_TIME));
      snprintf(_t2, sizeof(_t2), "%d", usr.time);
      LangPrintf(tlimit1, _t1);
      LangPrintf(tlimit2, _t2); }

    do_timecheck = TRUE;
    ci_ejectuser();

    mdm_hangup();    /* Bye! */
  }

  scRestrict.ldate = 0; /* default to no restriction */

  Calc_Timeoff();
}


/* ------------------------------------------------------------------ */
/*  Write_Active() — write ACTIVExx.BBS presence file                 */
/* ------------------------------------------------------------------ */

static void near Write_Active(void)
{
  int file;
  char fname[PATHLEN];

  sprintf(fname, activexx_bbs, original_path, task_num);

  if ((file = sopen(fname, O_CREAT | O_WRONLY | O_BINARY,
                    SH_DENYWR, S_IREAD | S_IWRITE)) == -1)
    cant_open(fname);
  else
    close(file);
}


/* ------------------------------------------------------------------ */
/*  autodetect_ansi() — ANSI terminal autodetection                   */
/* ------------------------------------------------------------------ */

#ifndef ORACLE

int autodetect_ansi(void)
{
  int x;

  if (local || !ComIsAModem(hcModem))
    return TRUE;

  mdm_dump(DUMP_INPUT);
  Mdm_puts(ansi_autodetect);
  Mdm_flush();

  /* If user's term reports s/he has ANSI */

  if ((x = Mdm_kpeek_tic(200)) == 27)
    x = TRUE;
  else
    x = FALSE;

  while (Mdm_kpeek_tic(50) != -1)
    Mdm_getcw();
  mdm_dump(DUMP_INPUT);

  return x;
}


/* ------------------------------------------------------------------ */
/*  autodetect_rip() — RIP terminal autodetection                     */
/* ------------------------------------------------------------------ */

int autodetect_rip(void)
{
  int x;

  if (local || !ComIsAModem(hcModem))
    return FALSE;

  /* RIP autodetect */

  mdm_dump(DUMP_INPUT);
  Mdm_puts(rip_autodetect);
  Mdm_flush();

  /* If user's term reports s/he has RIP */

  if ((x = Mdm_kpeek_tic(200)) == '0' || x == '1')
    x = TRUE;
  else
    x = FALSE;

  while (Mdm_kpeek_tic(50) != -1)
    Mdm_getcw();

  return x;
}

#endif
