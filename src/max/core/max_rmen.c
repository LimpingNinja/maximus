/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
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

#ifndef __GNUC__
#pragma off(unreferenced)
static char rcs_id[]="$Id: max_rmen.c,v 1.4 2004/01/28 06:38:10 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=Routines to read *.MNU files (Overlaid)
*/

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>
#include <stdlib.h>
#include "libmaxcfg.h"
#include "prog.h"
#include "mm.h"
#include "max_menu.h"
#include "max_msg.h"

static int near Read_Menu_Toml(struct _amenu *menu, const char *mname);
static int near mnu_cmd_to_opt(const char *cmd, option *out);
static int near mnu_heap_add(char **pp, char *base, size_t cap, const char *s, zstr *out);
static void near mnu_apply_modifiers(MaxCfgStrView mods, word *pflag, byte *pareatype);
static word near mnu_menu_flag_from_types(MaxCfgStrView types, int is_header);

/* Count the number of menu options */

static void near CountMenuLines(PAMENU pam, char *mname)
{
  struct _opt *popt, *eopt;
  int num_opt=0;

  /* If we have a canned menu to display, use its length */

  if (*MNU(*pam, m.dspfile) && DoDspFile(menuhelp, pam->m.flag))
    menu_lines=pam->m.menu_length;
  else
  {
    /* Else count the number of valid menu options accordingly */

    for (popt=pam->opt, eopt=popt + pam->m.num_options; popt < eopt; popt++)
      if (popt->type && OptionOkay(pam, popt, TRUE, NULL, &mah, &fah, mname))
        num_opt++;

    if (!pam->m.opt_width)
      pam->m.opt_width=DEFAULT_OPT_WIDTH;

    if (usr.help==NOVICE)
    {
  /*  menu_lines = 3 + (3+num_opt)/4; */
      int opts_per_line = (TermWidth()+1) / pam->m.opt_width;
      if (opts_per_line <= 0)
        opts_per_line=1;
      menu_lines = 3 + (num_opt / opts_per_line)  + !!(num_opt % opts_per_line);
    }
    else menu_lines=2;
  }
}

sword Read_Menu(struct _amenu *menu, char *mname)
{
  long where, end;

  int menufile;
  word menu_items;
  size_t size, hlen;

  char mpath[PATHLEN];

  menu_items=1;

  if (Read_Menu_Toml(menu, mname) == 0)
  {
    CountMenuLines(menu, mname);
    return 0;
  }

  {
    char mname_ext[PATHLEN];
    snprintf(mname_ext, sizeof(mname_ext), "%s.mnu", mname);
    if (safe_path_join(menupath, mname_ext, mpath, sizeof(mpath)) != 0)
      return -2;
  }

  if ((menufile=shopen(mpath, O_RDONLY | O_BINARY))==-1)
    return -2;

  if (read(menufile, (char *)&menu->m, sizeof(menu->m)) != sizeof(menu->m))
  {
    logit(cantread, mpath);
    close(menufile);
    quit(2);
  }

  size=sizeof(struct _opt) * menu->m.num_options;

  if ((menu->opt=malloc(size))==NULL)
  {
    logit(mem_none);
    close(menufile);
    return -1;
  }

  if (read(menufile, (char *)menu->opt, size) != (signed)size)
  {
    logit(cantread, mpath);
    close(menufile);
    return -1;
  }
  

  /* Now read in the rest of the file as the variable-length heap */
  
  where=tell(menufile);
  
  lseek(menufile, 0L, SEEK_END);
  end=tell(menufile);

  hlen=(size_t)(end-where);
  
  if ((menu->menuheap=malloc(hlen))==NULL)
  {
    logit(mem_none);
    close(menufile);
    return -1;
  }
  
  lseek(menufile, where, SEEK_SET);

  if (read(menufile, menu->menuheap, hlen) != (signed)hlen)
  {
    logit(cantread, mpath);
    close(menufile);
    return -1;
  }

  close(menufile);

  CountMenuLines(menu, mname);

  return 0;
}


static int near mnu_cmd_to_opt(const char *cmd, option *out)
{
  struct _cmdmap
  {
    const char *token;
    option opt;
  };

  static const struct _cmdmap map[] =
  {
    {"msg_reply_area", msg_reply_area},
    {"msg_download_attach", msg_dload_attach},
    {"msg_track", msg_track},
    {"link_menu", link_menu},
    {"return", o_return},
    {"mex", mex},
    {"msg_restrict", msg_restrict},
    {"climax", climax},
    {"msg_kludges", msg_toggle_kludges},
    {"msg_unreceive", msg_unreceive},
    {"msg_upload_qwk", msg_upload_qwk},
    {"chg_archiver", chg_archiver},
    {"msg_edit_user", msg_edit_user},
    {"chg_fsr", chg_fsr},
    {"msg_current", msg_current},
    {"msg_browse", msg_browse},
    {"chg_userlist", chg_userlist},
    {"chg_protocol", chg_protocol},
    {"msg_tag", msg_tag},
    {"chg_language", chg_language},
    {"file_tag", file_tag},
    {"chat_cb", o_chat_cb},
    {"chat_pvt", o_chat_pvt},
    {"chg_hotkeys", chg_hotkeys},
    {"msg_change", msg_change},
    {"chat_toggle", chat_toggle},
    {"chat_page", o_page},
    {"menupath", o_menupath},
    {"display_menu", display_menu},
    {"display_file", display_file},
    {"xtern_erlvl", xtern_erlvl},
    {"xtern_dos", xtern_dos},
    {"xtern_os2", xtern_dos},
    {"xtern_shell", xtern_dos},
    {"xtern_run", xtern_run},
    {"xtern_chain", xtern_chain},
    {"xtern_concur", xtern_concur},
    {"xtern_door32", xtern_door32},
    {"door32_run", xtern_door32},
    {"key_poke", key_poke},
    {"clear_stacked", clear_stacked},
    {"goodbye", goodbye},
    {"yell", o_yell},
    {"userlist", userlist},
    {"version", o_version},
    {"msg_area", msg_area},
    {"file_area", file_area},
    {"same_direction", same_direction},
    {"read_next", read_next},
    {"read_previous", read_previous},
    {"msg_enter", enter_message},
    {"msg_reply", msg_reply},
    {"read_nonstop", read_nonstop},
    {"read_original", read_original},
    {"read_reply", read_reply},
    {"msg_list", msg_list},
    {"msg_scan", msg_scan},
    {"msg_inquire", msg_inquir},
    {"msg_kill", msg_kill},
    {"msg_hurl", msg_hurl},
    {"msg_forward", forward},
    {"msg_upload", msg_upload},
    {"msg_xport", xport},
    {"read_individual", read_individual},
    {"msg_checkmail", msg_checkmail},
    {"file_locate", locate},
    {"file_titles", file_titles},
    {"file_view", file_type},
    {"file_upload", upload},
    {"file_download", download},
    {"file_raw", raw},
    {"file_kill", file_kill},
    {"file_contents", contents},
    {"file_hurl", file_hurl},
    {"file_override", override_path},
    {"file_newfiles", newfiles},
    {"chg_city", chg_city},
    {"chg_password", chg_password},
    {"chg_help", chg_help},
    {"chg_nulls", chg_nulls},
    {"chg_width", chg_width},
    {"chg_length", chg_length},
    {"chg_tabs", chg_tabs},
    {"chg_more", chg_more},
    {"chg_video", chg_video},
    {"chg_editor", chg_editor},
    {"chg_clear", chg_clear},
    {"chg_ibm", chg_ibm},
    {"chg_rip", chg_rip},
    {"edit_save", edit_save},
    {"edit_abort", edit_abort},
    {"edit_list", edit_list},
    {"edit_edit", edit_edit},
    {"edit_insert", edit_insert},
    {"edit_delete", edit_delete},
    {"edit_continue", edit_continue},
    {"edit_to", edit_to},
    {"edit_from", edit_from},
    {"edit_subj", edit_subj},
    {"edit_handling", edit_handling},
    {"read_diskfile", read_diskfile},
    {"edit_quote", edit_quote},
    {"cls", o_cls},
    {"user_editor", user_editor},
    {"chg_phone", chg_phone},
    {"chg_realname", chg_realname},
    {"chg_alias", chg_realname},
    {"leave_comment", leave_comment},
    {"message", message},
    {"file", file},
    {"other", other},
    {"if", o_if},
    {"press_enter", o_press_enter}
  };

  size_t i;
  char token[128];
  size_t n;

  if (cmd == NULL || out == NULL)
    return FALSE;

  n = strlen(cmd);
  if (n == 0 || n >= sizeof(token))
    return FALSE;

  for (i = 0; i < n; i++)
  {
    unsigned char c = (unsigned char)cmd[i];
    if (c >= 'A' && c <= 'Z')
      token[i] = (char)(c - 'A' + 'a');
    else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')
      token[i] = (char)c;
    else
      token[i] = '_';
  }
  token[n] = '\0';

  for (i = 0; i < (sizeof(map) / sizeof(map[0])); i++)
    if (stricmp(token, map[i].token) == 0)
    {
      *out = map[i].opt;
      return TRUE;
    }

  return FALSE;
}


static int near mnu_heap_add(char **pp, char *base, size_t cap, const char *s, zstr *out)
{
  size_t len;
  size_t ofs;

  if (pp == NULL || base == NULL || out == NULL)
    return FALSE;

  if (s == NULL || *s == '\0')
  {
    *out = 0;
    return TRUE;
  }

  len = strlen(s) + 1;
  ofs = (size_t)(*pp - base);

  if (ofs + len > cap)
    return FALSE;
  if (ofs > 0xffff)
    return FALSE;

  *out = (zstr)ofs;
  memcpy(*pp, s, len);
  *pp += len;
  return TRUE;
}


static void near mnu_apply_modifiers(MaxCfgStrView mods, word *pflag, byte *pareatype)
{
  size_t i;
  int have_area = 0;

  if (pflag)
    *pflag = 0;
  if (pareatype)
    *pareatype = AREATYPE_ALL;

  for (i = 0; i < mods.count; i++)
  {
    const char *m = mods.items[i];
    if (m == NULL || *m == '\0')
      continue;

    if (pareatype)
    {
      if (stricmp(m, "Local") == 0)
      {
        if (!have_area)
          *pareatype = 0;
        have_area = 1;
        *pareatype |= AREATYPE_LOCAL;
      }
      else if (stricmp(m, "Matrix") == 0)
      {
        if (!have_area)
          *pareatype = 0;
        have_area = 1;
        *pareatype |= AREATYPE_MATRIX;
      }
      else if (stricmp(m, "Echo") == 0)
      {
        if (!have_area)
          *pareatype = 0;
        have_area = 1;
        *pareatype |= AREATYPE_ECHO;
      }
      else if (stricmp(m, "Conf") == 0)
      {
        if (!have_area)
          *pareatype = 0;
        have_area = 1;
        *pareatype |= AREATYPE_CONF;
      }
    }

    if (pflag)
    {
      if (stricmp(m, "NoDsp") == 0)
        *pflag |= OFLAG_NODSP;
      else if (stricmp(m, "Ctl") == 0)
        *pflag |= OFLAG_CTL;
      else if (stricmp(m, "NoCls") == 0)
        *pflag |= OFLAG_NOCLS;
      else if (stricmp(m, "Then") == 0)
        *pflag |= (OFLAG_THEN | OFLAG_NODSP);
      else if (stricmp(m, "Else") == 0)
        *pflag |= (OFLAG_ELSE | OFLAG_NODSP);
      else if (stricmp(m, "UsrLocal") == 0)
        *pflag |= OFLAG_ULOCAL;
      else if (stricmp(m, "UsrRemote") == 0)
        *pflag |= OFLAG_UREMOTE;
      else if (stricmp(m, "Reread") == 0)
        *pflag |= OFLAG_REREAD;
      else if (stricmp(m, "Stay") == 0)
        *pflag |= OFLAG_STAY;
      else if (stricmp(m, "RIP") == 0)
        *pflag |= OFLAG_RIP;
      else if (stricmp(m, "NoRIP") == 0)
        *pflag |= OFLAG_NORIP;
    }
  }
}


static word near mnu_menu_flag_from_types(MaxCfgStrView types, int is_header)
{
  size_t i;
  word flag = 0;

  for (i = 0; i < types.count; i++)
  {
    const char *t = types.items[i];
    if (t == NULL || *t == '\0')
      continue;

    if (stricmp(t, "Novice") == 0)
      flag |= (is_header ? MFLAG_HF_NOVICE : MFLAG_MF_NOVICE);
    else if (stricmp(t, "Regular") == 0)
      flag |= (is_header ? MFLAG_HF_REGULAR : MFLAG_MF_REGULAR);
    else if (stricmp(t, "Expert") == 0)
      flag |= (is_header ? MFLAG_HF_EXPERT : MFLAG_MF_EXPERT);
    else if (stricmp(t, "RIP") == 0)
      flag |= (is_header ? MFLAG_HF_RIP : MFLAG_MF_RIP);
  }

  if (flag == 0)
    flag = (is_header ? MFLAG_HF_ALL : MFLAG_MF_ALL);

  return flag;
}


static int near Read_Menu_Toml(struct _amenu *menu, const char *mname)
{
  MaxCfgVar mt;
  MaxCfgVar v;
  MaxCfgVar opts;
  size_t opt_count;
  size_t i;
  char path[128];
  char lower[96];
  size_t n;
  const char *title = "";
  const char *header_file = "";
  const char *menu_file = "";
  int menu_length = 0;
  int menu_color = -1;
  int option_width = 0;
  MaxCfgStrView header_types = {0};
  MaxCfgStrView menu_types = {0};
  size_t heap_cap;
  char *hp;
  word mf_flag;
  word hf_flag;

  if (menu == NULL || mname == NULL || *mname == '\0')
    return -2;
  if (ng_cfg == NULL)
    return -2;

  n = strlen(mname);
  if (n == 0 || n >= sizeof(lower))
    return -2;

  for (i = 0; i < n; i++)
  {
    unsigned char c = (unsigned char)mname[i];
    if (c >= 'A' && c <= 'Z')
      lower[i] = (char)(c - 'A' + 'a');
    else
      lower[i] = (char)c;
  }
  lower[n] = '\0';

  if (snprintf(path, sizeof(path), "menus.%s", lower) >= (int)sizeof(path))
    return -2;

  if (maxcfg_toml_get(ng_cfg, path, &mt) != MAXCFG_OK || mt.type != MAXCFG_VAR_TABLE)
    return -2;

  if (maxcfg_toml_table_get(&mt, "title", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
    title = v.v.s;
  if (maxcfg_toml_table_get(&mt, "header_file", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
    header_file = v.v.s;
  if (maxcfg_toml_table_get(&mt, "menu_file", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
    menu_file = v.v.s;
  if (maxcfg_toml_table_get(&mt, "menu_length", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
    menu_length = v.v.i;
  if (maxcfg_toml_table_get(&mt, "menu_color", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
    menu_color = v.v.i;
  if (maxcfg_toml_table_get(&mt, "option_width", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
    option_width = v.v.i;

  if (maxcfg_toml_table_get(&mt, "header_types", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING_ARRAY)
    header_types = v.v.strv;
  if (maxcfg_toml_table_get(&mt, "menu_types", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING_ARRAY)
    menu_types = v.v.strv;

  if (maxcfg_toml_table_get(&mt, "option", &opts) != MAXCFG_OK || opts.type != MAXCFG_VAR_TABLE_ARRAY)
    return -2;
  if (maxcfg_var_count(&opts, &opt_count) != MAXCFG_OK)
    return -2;

  menu->m.header = HEADER_NONE;
  menu->m.num_options = (word)opt_count;
  menu->m.menu_length = (word)menu_length;
  menu->m.opt_width = (word)option_width;
  menu->m.hot_colour = (sword)menu_color;
  menu->m.flag = 0;

  hf_flag = 0;
  if (header_file && *header_file)
    hf_flag = mnu_menu_flag_from_types(header_types, TRUE);
  mf_flag = 0;
  if (menu_file && *menu_file)
    mf_flag = mnu_menu_flag_from_types(menu_types, FALSE);
  menu->m.flag = (word)(hf_flag | mf_flag);

  if (opt_count > 0)
  {
    menu->opt = malloc(sizeof(struct _opt) * opt_count);
    if (menu->opt == NULL)
      goto fail;
    memset(menu->opt, 0, sizeof(struct _opt) * opt_count);
  }

  heap_cap = 1;
  heap_cap += (title && *title) ? strlen(title) + 1 : 0;
  heap_cap += (header_file && *header_file) ? strlen(header_file) + 1 : 0;
  heap_cap += (menu_file && *menu_file) ? strlen(menu_file) + 1 : 0;

  for (i = 0; i < opt_count; i++)
  {
    MaxCfgVar ot;
    if (maxcfg_toml_array_get(&opts, i, &ot) != MAXCFG_OK || ot.type != MAXCFG_VAR_TABLE)
      continue;

    if (maxcfg_toml_table_get(&ot, "priv_level", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s && *v.v.s)
      heap_cap += strlen(v.v.s) + 1;
    if (maxcfg_toml_table_get(&ot, "description", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s && *v.v.s)
      heap_cap += strlen(v.v.s) + 1;
    if (maxcfg_toml_table_get(&ot, "key_poke", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s && *v.v.s)
      heap_cap += strlen(v.v.s) + 1;
    if (maxcfg_toml_table_get(&ot, "arguments", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s && *v.v.s)
      heap_cap += strlen(v.v.s) + 1;
  }

  menu->menuheap = malloc(heap_cap);
  if (menu->menuheap == NULL)
    goto fail;
  memset(menu->menuheap, 0, heap_cap);

  hp = menu->menuheap;
  *hp++ = '\0';

  if (!mnu_heap_add(&hp, menu->menuheap, heap_cap, title, &menu->m.title) ||
      !mnu_heap_add(&hp, menu->menuheap, heap_cap, header_file, &menu->m.headfile) ||
      !mnu_heap_add(&hp, menu->menuheap, heap_cap, menu_file, &menu->m.dspfile))
    goto fail;

  for (i = 0; i < opt_count; i++)
  {
    MaxCfgVar ot;
    const char *cmd = NULL;
    const char *args = "";
    const char *priv = "";
    const char *desc = "";
    const char *kp = "";
    MaxCfgStrView mods = {0};
    option o;
    struct _opt *popt;

    if (maxcfg_toml_array_get(&opts, i, &ot) != MAXCFG_OK || ot.type != MAXCFG_VAR_TABLE)
      continue;

    if (maxcfg_toml_table_get(&ot, "command", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
      cmd = v.v.s;
    if (maxcfg_toml_table_get(&ot, "arguments", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
      args = v.v.s;
    if (maxcfg_toml_table_get(&ot, "priv_level", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
      priv = v.v.s;
    if (maxcfg_toml_table_get(&ot, "description", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
      desc = v.v.s;
    if (maxcfg_toml_table_get(&ot, "key_poke", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
      kp = v.v.s;
    if (maxcfg_toml_table_get(&ot, "modifiers", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING_ARRAY)
      mods = v.v.strv;

    o = nothing;
    if (cmd && *cmd)
      (void)mnu_cmd_to_opt(cmd, &o);

    popt = &menu->opt[i];
    memset(popt, 0, sizeof(*popt));
    popt->type = o;
    popt->areatype = AREATYPE_ALL;
    mnu_apply_modifiers(mods, &popt->flag, &popt->areatype);

    if (!mnu_heap_add(&hp, menu->menuheap, heap_cap, priv, &popt->priv) ||
        !mnu_heap_add(&hp, menu->menuheap, heap_cap, desc, &popt->name) ||
        !mnu_heap_add(&hp, menu->menuheap, heap_cap, kp, &popt->keypoke) ||
        !mnu_heap_add(&hp, menu->menuheap, heap_cap, args, &popt->arg))
      popt->type = nothing;
  }

  return 0;

fail:
  if (menu->menuheap)
  {
    free(menu->menuheap);
    menu->menuheap = NULL;
  }
  if (menu->opt)
  {
    free(menu->opt);
    menu->opt = NULL;
  }
  memset(&menu->m, 0, sizeof(menu->m));
  return -1;
}

void Initialize_Menu(struct _amenu *menu)
{
  memset(menu,'\0',sizeof(struct _amenu));
}





void Free_Menu(struct _amenu *menu)
{
  if (menu->menuheap)
    free(menu->menuheap);

  if (menu->opt)
    free(menu->opt);

  Initialize_Menu(menu);
}






#if 0
int Header_None(int entry, int silent)
{
  NW(entry);
  NW(silent);

  WhiteN();

  restart_system=FALSE;
  return TRUE;
}
#endif

