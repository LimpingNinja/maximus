/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
 *
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * https://github.com/LimpingNinja
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
static char rcs_id[]="$Id: max_init.c,v 1.7 2004/06/06 21:48:51 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# tname=Initialization code
*/

#define MAX_INCL_COMMS

#define MAX_LANG_f_area
#define MAX_LANG_global
#define MAX_LANG_max_init
#define MAX_LANG_sysop
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>
#include <ctype.h>
#include <string.h>
#include <share.h>
#ifdef UNIX
#include <dirent.h>
#include <unistd.h>
#endif
#include "libmaxcfg.h"
#include "dr.h"
#include "prog.h"
#include "cfg_consts.h"
#include "mm.h"
#include "newarea.h"
#include "max_msg.h"
#include "max_file.h"
#include "max_edit.h"
#include "emsi.h"
#include "debug_log.h"

#ifdef KEY
#include "makekey.h"
#endif

MaxCfg *ng_cfg = NULL;

static struct
{
  int startup;
  int max;
} log_status = { 0, 0 };

static FILE *startup_log_fp = NULL;

extern unsigned char task_num;
extern char logformat[];
extern char nameabbr[];
extern byte debuglog;

/**
 * Guidance for future startup_logit() calls (so we don’t screw ourselves)
 *
 * - Use ! for “we’re about to die / can’t open file / fatal invariants”
 * - Use : for “state transitions / waiting / becoming / startup checkpoints”
 * - Use + for “major milestones” that you want when log_mode >= 1
 * - Use @ only when it’s genuinely debug noise you’re fine losing unless debuglog is enabled
 *
 * Args:
 *   fmt: printf-style format string. First character should be the log symbol.
 *   ...: printf-style varargs.
 *
 * Returns:
 *   None.
 *
 * Notes:
 *   Before LogOpen() succeeds, writes to ./max_startup.log (CWD).
 *   After LogOpen() succeeds (log_status.max==true), forwards to logit().
 */
static void near startup_logit(const char *fmt, ...)
{
  char msg[1024];
  char msg2[1200];
  char line[1600];
  va_list ap;

  if (log_status.max)
  {
    char esc[2048];
    size_t i, j;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    /* logit() treats its first character as the log level/symbol.
     * If msg contains '%', it would be interpreted as a format sequence,
     * so escape it before forwarding.
     */
    for (i = 0, j = 0; msg[i] && j + 1 < sizeof(esc); i++)
    {
      if (msg[i] == '%' && j + 2 < sizeof(esc))
      {
        esc[j++] = '%';
        esc[j++] = '%';
      }
      else
        esc[j++] = msg[i];
    }
    esc[j] = '\0';

    logit(esc);
    return;
  }

  log_status.startup = 1;

  if (startup_log_fp == NULL)
    startup_log_fp = fopen("max_startup.log", "a");

  if (startup_log_fp == NULL)
    return;

  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);

  /* Match logit(): first character is the log symbol, rest is message. */
  {
    char *p = msg;
    char sym;
    time_t t;
    struct tm *tmv;

    if (p[0] == '>')
      p++;

    if (p[0] == '\0')
      return;

    sym = *p;

    if (task_num && strstr(p + 1, "(task=") == NULL)
      snprintf(msg2, sizeof(msg2), "%s (task=%u)", p + 1, (unsigned)task_num);
    else
      strnncpy(msg2, p + 1, sizeof(msg2) - 1);

    t = time(NULL);
    tmv = localtime(&t);
    if (tmv)
      snprintf(line, sizeof(line), logformat,
               sym, tmv->tm_mday, months_ab[tmv->tm_mon],
               tmv->tm_hour, tmv->tm_min, tmv->tm_sec,
               nameabbr, msg2);
    else
      snprintf(line, sizeof(line), "%c %s %s\n", sym, nameabbr, msg2);
  }

  fputs(line, startup_log_fp);
  fflush(startup_log_fp);
}


static void near OpenAreas(void);
static void near install_handlers(void);
static void near StartUpVideo(void);
static void near Initialize_Colours(void);
static int near build_access_dat_from_toml(char *out_access_base, size_t out_access_base_sz);
static int near build_area_dats_from_toml(char *out_marea_base, size_t out_marea_base_sz,
                                         char *out_farea_base, size_t out_farea_base_sz);


const char *ngcfg_get_string_raw(const char *toml_path)
{
  MaxCfgVar v;

  if (toml_path && ng_cfg &&
      maxcfg_toml_get(ng_cfg, toml_path, &v) == MAXCFG_OK &&
      v.type == MAXCFG_VAR_STRING && v.v.s)
    return v.v.s;

  return "";
}

static zstr near access_heap_add(char **heap, size_t *heap_cap, size_t *heap_len, const char *s)
{
  size_t need;
  zstr ofs;
  char *new_heap;

  if (s == NULL)
    s = "";

  need = strlen(s) + 1;
  if (*heap == NULL)
    return (zstr)0;

  if (*heap_len + need > *heap_cap)
  {
    size_t new_cap = (*heap_cap == 0) ? 1024 : *heap_cap;
    while (*heap_len + need > new_cap)
      new_cap *= 2;
    new_heap = (char *)realloc(*heap, new_cap);
    if (new_heap == NULL)
      return (zstr)0;
    *heap = new_heap;
    *heap_cap = new_cap;
  }

  ofs = (zstr)(*heap_len);
  memcpy((*heap) + (*heap_len), s, need);
  *heap_len += need;
  return ofs;
}

static dword near access_flags_from_list(const MaxCfgVar *arr)
{
  MaxCfgVar it;
  dword flags = 0;
  size_t cnt = 0;

  if (arr == NULL || arr->type != MAXCFG_VAR_STRING_ARRAY)
    return 0;

  if (maxcfg_var_count(arr, &cnt) != MAXCFG_OK)
    return 0;

  for (size_t i = 0; i < cnt; i++)
  {
    if (maxcfg_toml_array_get(arr, i, &it) != MAXCFG_OK || it.type != MAXCFG_VAR_STRING || it.v.s == NULL)
      continue;

    if (stricmp(it.v.s, "UploadAny") == 0)
      flags |= CFLAGA_ULBBSOK;
    else if (stricmp(it.v.s, "DloadHidden") == 0)
      flags |= CFLAGA_FLIST;
    else if (stricmp(it.v.s, "ShowAllFiles") == 0)
      flags |= CFLAGA_FHIDDEN;
    else if (stricmp(it.v.s, "ShowHidden") == 0)
      flags |= CFLAGA_UHIDDEN;
    else if (stricmp(it.v.s, "Hide") == 0)
      flags |= CFLAGA_HIDDEN;
    else if (stricmp(it.v.s, "Hangup") == 0)
      flags |= CFLAGA_HANGUP;
    else if (stricmp(it.v.s, "NoFileLimit") == 0)
      flags |= CFLAGA_NOLIMIT;
    else if (stricmp(it.v.s, "NoTimeLimit") == 0)
      flags |= CFLAGA_NOTIME;
    else if (stricmp(it.v.s, "NoLimits") == 0)
      flags |= (CFLAGA_NOLIMIT | CFLAGA_NOTIME);
  }

  return flags;
}

static dword near mail_flags_from_list(const MaxCfgVar *arr)
{
  MaxCfgVar it;
  dword flags = 0;
  size_t cnt = 0;

  if (arr == NULL || arr->type != MAXCFG_VAR_STRING_ARRAY)
    return 0;

  if (maxcfg_var_count(arr, &cnt) != MAXCFG_OK)
    return 0;

  for (size_t i = 0; i < cnt; i++)
  {
    if (maxcfg_toml_array_get(arr, i, &it) != MAXCFG_OK || it.type != MAXCFG_VAR_STRING || it.v.s == NULL)
      continue;

    if (stricmp(it.v.s, "ShowPvt") == 0)
      flags |= CFLAGM_PVT;
    else if (stricmp(it.v.s, "Editor") == 0)
      flags |= CFLAGM_EDITOR;
    else if (stricmp(it.v.s, "LocalEditor") == 0)
      flags |= CFLAGM_LEDITOR;
    else if (stricmp(it.v.s, "NetFree") == 0)
      flags |= CFLAGM_NETFREE;
    else if (stricmp(it.v.s, "MsgAttrAny") == 0)
      flags |= CFLAGM_ATTRANY;
    else if (stricmp(it.v.s, "WriteRdOnly") == 0)
      flags |= CFLAGM_RDONLYOK;
    else if (stricmp(it.v.s, "NoRealName") == 0)
      flags |= CFLAGM_NOREALNM;
  }

  return flags;
}

static int near cmp_clsrec_level(const void *a, const void *b)
{
  const CLSREC *A = (const CLSREC *)a;
  const CLSREC *B = (const CLSREC *)b;
  if (A->usLevel < B->usLevel) return -1;
  if (A->usLevel > B->usLevel) return 1;
  return 0;
}

static int near build_access_dat_from_toml(char *out_access_base, size_t out_access_base_sz)
{
  MaxCfgVar levels;
  MaxCfgVar item;
  MaxCfgVar v;
  size_t count = 0;
  CLSREC *recs = NULL;
  char *heap = NULL;
  size_t heap_cap = 0;
  size_t heap_len = 0;
  char out_dir[PATHLEN];
  char out_path[PATHLEN];
  int fd = -1;
  CLSHDR hdr;

  if (out_access_base == NULL || out_access_base_sz == 0)
    return -1;

  out_access_base[0] = '\0';

  if (ng_cfg == NULL)
    return -1;

  if (maxcfg_toml_get(ng_cfg, "security.access_levels.access_level", &levels) != MAXCFG_OK || levels.type != MAXCFG_VAR_TABLE_ARRAY)
    return -1;

  if (maxcfg_var_count(&levels, &count) != MAXCFG_OK || count == 0)
    return -1;

  recs = (CLSREC *)calloc(count, sizeof(*recs));
  if (recs == NULL)
    goto fail;

  heap_cap = 4096;
  heap = (char *)malloc(heap_cap);
  if (heap == NULL)
    goto fail;
  heap[0] = '\0';
  heap_len = 1;

  for (size_t i = 0; i < count; i++)
  {
    CLSREC *r = &recs[i];
    const char *name = "";
    const char *desc = "";
    const char *alias = "";
    const char *login_file = "";
    const char *key = "";
    int level = 0;
    int time_call = 0;
    int time_day = 0;
    int calls_day = -1;
    int logon_baud = 0;
    int xfer_baud = 0;
    int file_ratio = 0;
    int ratio_free = 0;
    int upload_reward = 0;
    int oldpriv = -1;
    unsigned int file_limit = 0;
    dword acc_flags = 0;
    dword mail_flags = 0;
    dword user_flags = 0;

    memset(r, 0, sizeof(*r));

    if (maxcfg_toml_array_get(&levels, i, &item) != MAXCFG_OK || item.type != MAXCFG_VAR_TABLE)
      goto fail;

    if (maxcfg_toml_table_get(&item, "name", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
      name = v.v.s;
    if (maxcfg_toml_table_get(&item, "description", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
      desc = v.v.s;
    if (maxcfg_toml_table_get(&item, "alias", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
      alias = v.v.s;
    if (maxcfg_toml_table_get(&item, "key", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
      key = v.v.s;

    if (maxcfg_toml_table_get(&item, "level", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
      level = v.v.i;
    if (maxcfg_toml_table_get(&item, "time", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
      time_call = v.v.i;
    if (maxcfg_toml_table_get(&item, "cume", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
      time_day = v.v.i;
    if (maxcfg_toml_table_get(&item, "calls", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
      calls_day = v.v.i;
    if (maxcfg_toml_table_get(&item, "logon_baud", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
      logon_baud = v.v.i;
    if (maxcfg_toml_table_get(&item, "xfer_baud", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
      xfer_baud = v.v.i;
    if (maxcfg_toml_table_get(&item, "file_limit", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
      file_limit = (unsigned int)v.v.i;
    if (maxcfg_toml_table_get(&item, "file_ratio", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
      file_ratio = v.v.i;
    if (maxcfg_toml_table_get(&item, "ratio_free", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
      ratio_free = v.v.i;
    if (maxcfg_toml_table_get(&item, "upload_reward", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
      upload_reward = v.v.i;
    if (maxcfg_toml_table_get(&item, "user_flags", &v) == MAXCFG_OK)
    {
      if (v.type == MAXCFG_VAR_UINT)
        user_flags = (dword)v.v.u;
      else if (v.type == MAXCFG_VAR_INT)
        user_flags = (dword)v.v.i;
    }
    if (maxcfg_toml_table_get(&item, "oldpriv", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
      oldpriv = v.v.i;
    if (maxcfg_toml_table_get(&item, "login_file", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s)
      login_file = v.v.s;

    if (maxcfg_toml_table_get(&item, "flags", &v) == MAXCFG_OK)
      acc_flags = access_flags_from_list(&v);
    if (maxcfg_toml_table_get(&item, "mail_flags", &v) == MAXCFG_OK)
      mail_flags = mail_flags_from_list(&v);

    r->usLevel = (word)level;
    r->usKey = (word)(key[0] ? (unsigned char)toupper((unsigned char)key[0]) : 0);
    r->zAbbrev = access_heap_add(&heap, &heap_cap, &heap_len, name);
    r->zDesc = access_heap_add(&heap, &heap_cap, &heap_len, desc[0] ? desc : name);
    r->zAlias = access_heap_add(&heap, &heap_cap, &heap_len, alias);
    r->usTimeDay = (word)time_day;
    r->usTimeCall = (word)time_call;
    r->usCallsDay = (word)calls_day;
    r->usMinBaud = (word)(logon_baud / 100);
    r->usFileBaud = (word)(xfer_baud / 100);
    r->usFileRatio = (word)file_ratio;
    r->usFreeRatio = (word)ratio_free;
    r->ulFileLimit = (dword)file_limit;
    r->usUploadReward = (word)upload_reward;
    r->zLoginFile = access_heap_add(&heap, &heap_cap, &heap_len, login_file);
    r->ulAccFlags = acc_flags;
    r->ulMailFlags = mail_flags;
    r->ulUsrFlags = user_flags;
    r->usOldPriv = (word)oldpriv;
  }

  qsort(recs, count, sizeof(*recs), cmp_clsrec_level);

  strcpy(out_dir, ngcfg_get_path("maximus.temp_path"));
  if (out_dir[0] == '\0')
    goto fail;

  if (snprintf(out_access_base, out_access_base_sz, "%sng_access", out_dir) >= (int)out_access_base_sz)
    goto fail;

  if (snprintf(out_path, sizeof(out_path), "%s.dat", out_access_base) >= (int)sizeof(out_path))
    goto fail;

  fd = open(out_path, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IREAD | S_IWRITE);
  if (fd == -1)
    goto fail;

  memset(&hdr, 0, sizeof(hdr));
  hdr.ulclhid = CLS_ID;
  hdr.usclfirst = (word)sizeof(hdr);
  hdr.usn = (word)count;
  hdr.ussize = (word)sizeof(CLSREC);
  hdr.usstr = (word)heap_len;

  if (write(fd, (char *)&hdr, sizeof(hdr)) != sizeof(hdr))
    goto fail;
  if (write(fd, (char *)recs, (unsigned)(sizeof(CLSREC) * count)) != (int)(sizeof(CLSREC) * count))
    goto fail;
  if (write(fd, heap, (unsigned)heap_len) != (int)heap_len)
    goto fail;

  close(fd);
  fd = -1;

  free(recs);
  free(heap);
  return 0;

fail:
  if (fd != -1)
    close(fd);
  free(recs);
  free(heap);
  out_access_base[0] = '\0';
  return -1;
}

const char *ngcfg_get_string(const char *toml_path)
{
  const char *s = ngcfg_get_string_raw(toml_path);
  if (s && *s)
    return s;
  return "";
}

const char *ngcfg_get_path(const char *toml_path)
{
  const char *s;
  const char *sys_base;
  static char buf[PATHLEN];
  size_t len;

  s = ngcfg_get_string_raw(toml_path);
  if (s == NULL || *s == '\0')
  {
    if (getcwd(buf, sizeof(buf)) == NULL)
      buf[0] = '\0';
    return buf;
  }

  /* absolute paths */
  if (s[0] == '/' || s[0] == '\\' || (isalpha((unsigned char)s[0]) && s[1] == ':'))
  {
    /* Copy to buffer so we can potentially add trailing slash */
    strncpy(buf, s, sizeof(buf) - 2);
    buf[sizeof(buf) - 2] = '\0';
    len = strlen(buf);
    
    /* Ensure directory paths have trailing separator */
    if (len > 0 && buf[len-1] != '/' && buf[len-1] != '\\')
    {
      struct stat st;
      if (stat(buf, &st) == 0 && S_ISDIR(st.st_mode))
      {
        buf[len] = '/';
        buf[len+1] = '\0';
      }
    }
    return buf;
  }

  sys_base = ngcfg_get_string_raw("maximus.sys_path");

  if (*sys_base == '\0')
    return s;

  if (safe_path_join(sys_base, s, buf, sizeof(buf)) != 0)
    return s;

  len = strlen(buf);

  /* Ensure directory paths have trailing separator */
  if (len > 0 && buf[len-1] != '/' && buf[len-1] != '\\')
  {
    struct stat st;
    if (stat(buf, &st) == 0 && S_ISDIR(st.st_mode))
    {
      buf[len] = '/';
      buf[len+1] = '\0';
    }
  }
  return buf;
}

int ngcfg_get_int(const char *toml_path)
{
  MaxCfgVar v;

  if (toml_path && ng_cfg &&
      maxcfg_toml_get(ng_cfg, toml_path, &v) == MAXCFG_OK)
  {
    if (v.type == MAXCFG_VAR_INT)
      return v.v.i;
    if (v.type == MAXCFG_VAR_UINT)
      return (int)v.v.u;
  }

  return 0;
}

int ngcfg_get_bool(const char *toml_path)
{
  MaxCfgVar v;

  if (toml_path && ng_cfg &&
      maxcfg_toml_get(ng_cfg, toml_path, &v) == MAXCFG_OK &&
      v.type == MAXCFG_VAR_BOOL)
  {
    return v.v.b ? 1 : 0;
  }

  return 0;
}

int ngcfg_get_log_mode_int(void)
{
  int iv;
  if (ng_cfg && maxcfg_ng_get_log_mode(ng_cfg, &iv) == MAXCFG_OK)
    return iv;
  return 0;
}

int ngcfg_get_multitasker_int(void)
{
  int iv;
  if (ng_cfg && maxcfg_ng_get_multitasker(ng_cfg, &iv) == MAXCFG_OK)
    return iv;
  return 0;
}

int ngcfg_get_video_mode_int(int *out_has_snow)
{
  int iv;
  bool bv = false;
  if (out_has_snow)
    *out_has_snow = 0;
  if (ng_cfg && maxcfg_ng_get_video_mode(ng_cfg, &iv, &bv) == MAXCFG_OK)
  {
    if (out_has_snow)
      *out_has_snow = bv ? 1 : 0;
    return iv;
  }
  return 0;
}

int ngcfg_get_has_snow(void)
{
  MaxCfgVar v;
  if (ng_cfg &&
      maxcfg_toml_get(ng_cfg, "maximus.has_snow", &v) == MAXCFG_OK &&
      v.type == MAXCFG_VAR_BOOL)
    return v.v.b ? 1 : 0;
  return 0;
}

int ngcfg_get_handshake_mask_int(void)
{
  int iv;
  if (ng_cfg && maxcfg_ng_get_handshake_mask(ng_cfg, &iv) == MAXCFG_OK)
    return iv;
  return 0;
}

int ngcfg_get_charset_int(void)
{
  int iv;
  if (ng_cfg && maxcfg_ng_get_charset(ng_cfg, &iv) == MAXCFG_OK)
    return iv;
  return 0;
}

int ngcfg_get_nodelist_version_int(void)
{
  int iv;
  if (ng_cfg && maxcfg_ng_get_nodelist_version(ng_cfg, &iv) == MAXCFG_OK)
    return iv;
  return 0;
}

static int near toml_var_to_int(const MaxCfgVar *v, int *out)
{
  if (v == NULL || out == NULL)
    return 0;
  if (v->type == MAXCFG_VAR_INT)
  {
    *out = v->v.i;
    return 1;
  }
  if (v->type == MAXCFG_VAR_UINT)
  {
    *out = (int)v->v.u;
    return 1;
  }
  return 0;
}

size_t ngcfg_get_matrix_address_count(void)
{
  MaxCfgVar v;
  size_t cnt = 0;

  if (ng_cfg &&
      maxcfg_toml_get(ng_cfg, "matrix.address", &v) == MAXCFG_OK &&
      v.type == MAXCFG_VAR_TABLE_ARRAY &&
      maxcfg_var_count(&v, &cnt) == MAXCFG_OK)
  {
    return cnt;
  }

  return 0;
}

NETADDR ngcfg_get_matrix_address_at(size_t idx)
{
  MaxCfgVar arr;
  MaxCfgVar it;
  MaxCfgVar v;
  size_t cnt = 0;
  NETADDR out;
  memset(&out, 0, sizeof(out));

  if (ng_cfg &&
      maxcfg_toml_get(ng_cfg, "matrix.address", &arr) == MAXCFG_OK &&
      arr.type == MAXCFG_VAR_TABLE_ARRAY &&
      maxcfg_var_count(&arr, &cnt) == MAXCFG_OK &&
      idx < cnt &&
      maxcfg_toml_array_get(&arr, idx, &it) == MAXCFG_OK &&
      it.type == MAXCFG_VAR_TABLE)
  {
    int iv;

    if (maxcfg_toml_table_get(&it, "zone", &v) == MAXCFG_OK && toml_var_to_int(&v, &iv))
      out.zone = (word)iv;
    if (maxcfg_toml_table_get(&it, "net", &v) == MAXCFG_OK && toml_var_to_int(&v, &iv))
      out.net = (word)iv;
    if (maxcfg_toml_table_get(&it, "node", &v) == MAXCFG_OK && toml_var_to_int(&v, &iv))
      out.node = (word)iv;
    if (maxcfg_toml_table_get(&it, "point", &v) == MAXCFG_OK && toml_var_to_int(&v, &iv))
      out.point = (word)iv;
  }

  return out;
}

NETADDR ngcfg_get_matrix_address(size_t idx)
{
  return ngcfg_get_matrix_address_at(idx);
}

NETADDR ngcfg_get_matrix_primary_address(void)
{
  return ngcfg_get_matrix_address(0);
}

NETADDR ngcfg_get_matrix_seenby_address(void)
{
  NETADDR prim = ngcfg_get_matrix_address(0);
  NETADDR alias1 = ngcfg_get_matrix_address(1);

  if (prim.point && alias1.zone)
    return alias1;

  return prim;
}

static void near load_menu_tomls(void)
{
#ifdef UNIX
  DIR *d;
  struct dirent *de;

  d = opendir("config/menus");
  if (d == NULL)
    return;

  while ((de = readdir(d)) != NULL)
  {
    const char *name;
    const char *ext;
    char path[PATHLEN];
    char prefix[128];
    char base[96];
    size_t n;

    name = de->d_name;
    if (name == NULL || name[0] == '.')
      continue;

    ext = strrchr(name, '.');
    if (ext == NULL || stricmp(ext, ".toml") != 0)
      continue;

    n = (size_t)(ext - name);
    if (n == 0 || n >= sizeof(base))
      continue;

    memcpy(base, name, n);
    base[n] = '\0';

    if (snprintf(path, sizeof(path), "config/menus/%s", name) >= (int)sizeof(path))
      continue;

    if (snprintf(prefix, sizeof(prefix), "menus.%s", base) >= (int)sizeof(prefix))
      continue;

    (void)maxcfg_toml_load_file(ng_cfg, path, prefix);
  }

  closedir(d);
#endif
}

void Init_Variables(void)
{
  int x;
  char *s;

  install_handlers(); /* ISRs for ^c and int 24h */

  timestart=time(NULL);

  InputAllocStr();
  OutputAllocStr();

  /* Fix up global and initialization data */

  multitasker=-2;
  strcpy(log_name,c123);

  local_putc=(void (_stdc *)(int))fputchar;
  local_puts=(void (_stdc *)(char *))putss;

#ifndef ORACLE
  Lprintf(slogan,"\n",version,test);
  Lputs(copyright);
#endif

  *firstname=*linebuf=*searchfor=*fix_menupath=*last_readln=*arq_info='\0';

  display_line=display_col=current_line=current_col=1;

  isareply=isachange=FALSE;

  port=-1;
  cls=-1;
  orig_disk2=orig_disk3=-1;
  local=-2;
  task_num=255;
  fSetTask = FALSE;
  event_num=(byte)-2;

  baud=0;
  current_baud=0;
  steady_baud=0;
  steady_baud_l=0;
  ultoday=0;
  brk_trapped=0;
  erl=0;
  
  #ifndef ORACLE
  max_lines=0;
  #endif
    
  num_yells=0;
  rst_offset=-1L;
  getoff=0x7fffffffL;

  fFlow=FALSE;

  #ifdef UNIX
  memset(&CommApi, 0, sizeof(struct CommApi_));
  #endif

  menu_lines=1;

  if ((s=getenv("PROMPT")) != NULL)
  {
    /* extra parens for dmalloc() kludge - see max.c */

    if ((original_prompt=(malloc)(strlen(s)+1)) != NULL) 
      strcpy(original_prompt,s);
    else original_prompt=NULL;
  }
  else original_prompt=NULL;



  do_caller_vanished=TRUE;

  snoop=keyboard=caller_online=do_timecheck=fossil_initd=
    written_echomail=written_matrix=sent_time_5left=sent_time_almostup=
               wrap=inmagnet=restart_system=first_search=
                 barricade_ok=create_userbbs=this_logon_bad=inchat=
    locked=chatreq=do_useredit=fthru_yuhu=shut_up=debug_ovl=
    no_dcd_check=FALSE;
    fLoggedOn=FALSE;

#ifdef __MSDOS__
  port_is_device=TRUE;
#else
  port_is_device=FALSE; /* port is a file handle */
#endif

  nowrite_lastuser=in_file_xfer=written_local=mn_dirty=no_zmodem=
    in_mcheck=no_shell=dsp_set=in_node_chat=chkmail_reply=
    waitforcaller=in_wfc=log_wfc=in_msghibit=FALSE;

  direction=DIRECTION_NEXT;

  chatlog=NULL;
#ifndef ORACLE
  sq=NULL;
#endif
  dspwin=NULL;
  dspwin_time=0L;

  #ifndef ORACLE
  Init_File_Buffer();
  #endif

  max_time=0xffffL;
  last_bps=0;

  displaymode=VIDEO_DOS;  /* So that our output will work for Lprintf. */
                          /* This hopefully gets reset later! */
  Blank_User(&usr);

/*InitTag(&tfa);*/

  for (x=0;x < MAX_DRIVES;x++)
    orig_path2[x]=orig_path3[x]=NULL;

/*  memset(echo_written_in, '\x00', sizeof(echo_written_in));*/

#ifdef MCP
  *szMcpPipe = 0;
#endif


#ifdef EMSI
  EmsiInitHandshake();
#endif
}

static void near quitfile()
{
  vbuf_flush();
  Local_Beep(3);
  Delay(300);
  quit(ERROR_FILE);
}

char * Startup(void)
{
#ifdef KEY
  int key_fd, key_ok=TRUE;
  char *key_buf, *key_outbuf;
  int key_size, key_strip_ofs;
#endif

  union stamp_combo now;
  char temp[PATHLEN];

#ifndef ORACLE
  struct _usr user;
  struct _minf mi;
  int fd;
#endif



  if (getcwd(original_path, PATHLEN)==NULL)
  {
    Lputs(err_startup_tlong);
    Local_Beep(3);
    maximus_exit(ERROR_FILE);
  }
  else if (strlen(cfancy_fn(original_path)) > 3)
    strcat(cfancy_fn(original_path), PATH_DELIMS);

  /* Install the critical error handler */
  
  if (!ngcfg_get_bool("general.equipment.no_critical"))
  {
    install_24();
    maximus_atexit(uninstall_24);
  }

  Initialize_Languages();
  Initialize_Colours();

  /* Determine maximum length of string returned by MsgDte() */
  
  Get_Dos_Date(&now);
  MsgDte(&now, temp);
  datelen=strlen(temp);

  if (!dsp_set)
    displaymode=(byte)ngcfg_get_video_mode_int(NULL);
  
  /* Open a couple of files to prepare for the caller */

  if (!do_useredit)
    OpenAreas();

  Load_Archivers();

#ifdef KEY
  if ((key_fd=key_open("MAXIMUS.KEY"))==-1)
    key_ok=FALSE;
#endif

  /* Wait for the caller to come on-line, if using the os/2 version */

#if defined(OS_2) && !defined(__FLAT__) && !defined(ORACLE)
  MaxSemTriggerWait();
#endif

  /* Turn off the status line for local mode.  If restart_system is set,    *
   * then we won't know if the user is local or remote until we've read     *
   * in RESTARxx.BBS, so we'll take care of it in the System_Restart()      *
   * function in that case.                                                 */

  if (local && !restart_system)
    ;

  /* Open the video display */

  switch (displaymode)
  {
#if 0
    case VIDEO_FOSSIL:

#ifndef ORACLE       

      /* ORACLE doesn't use the fossil -- just fall thru to DOS video, if   *
       * that's the mode we've specified.                                   */

      local_putc=(void (_stdc *)(int))fossil_putc;
      local_puts=(void (_stdc *)(char *))fossil_puts;
      break;
#endif

    case VIDEO_DOS:
      local_putc=(void (_stdc *)(int))fputchar;
      local_puts=(void (_stdc *)(char *))putss;
      break;

    case VIDEO_FAST:
#ifdef __MSDOS__
      local_putc=(void (_stdc *)(int))xputch;
      local_puts=(void (_stdc *)(char *))xputs;
      break;
#endif  /* else fall through...*/
#endif /* TTYVIDEO */

    default:
      displaymode = VIDEO_IBM;

    case VIDEO_IBM:
    case VIDEO_BIOS:
      StartUpVideo();
      
#ifndef ORACLE
      /* Don't display copyright window if we're restarting from before */
      
      if (restart_system)
        break;

  #ifdef MCP_VIDEO
        if (no_video)
        {
          dspwin=NULL;
          dspwin_time=0L;
        }
        else
        {
  #endif
          dspwin=WinMsg(BORDER_DOUBLE, col.pop_text, col.pop_border,
                        logo1, logo2, NULL);

          dspwin_time=timerset(DSPWIN_TIME*100);
  #ifdef MCP_VIDEO
        }
  #endif
#endif
      break;
  }

#ifdef KEY
  if (key_ok && key_load(key_fd, &key_size, &key_buf) != 0)
    key_ok=FALSE;
#endif

  /* Use default, unless specified otherwise on command-line */

  if (!fSetTask)
    task_num=(byte)ngcfg_get_int("maximus.task_num");

  /* Unless specifically overridden, use the task num for the event file */

  if (event_num==(byte)-2)
    event_num=task_num;

#ifndef ORACLE
  Read_Events();
#endif

  if (!restart_system)
    ChatCleanUp();

  if (port==-1)
  {
    int p1 = ngcfg_get_int("general.equipment.com_port");
    if (p1 <= 0)
      p1 = 1;
    port = p1 - 1;
  }

  if (local==-2)
    local=TRUE;

  if (local)
    port=0xff;
  else if (port==-1 || port==0xff)
    port=0;

  snoop=(char)ngcfg_get_bool("maximus.snoop");

#ifdef __MSDOS__
  (void)zfree("");/* Calculate free disk space here, so we don't */
                  /* have a long pause later down the road. Not  */
                  /* needed for OS/2, unless we're doing the     */
                  /* fancy logo bit.                             */
#endif


  Lputs(GRAY);

  /* Derive menupath from config_path (menus are TOML under config/menus/) */
  snprintf(menupath, PATHLEN, "%s/menus",
           ngcfg_get_path("maximus.config_path"));

  /* Derive rippath from display_path: strip last component, append /rip.
   * e.g. display_path="display/screens" → rippath="<resolved>/display/rip" */
  {
    char disp_tmp[PATHLEN];
    strncpy(disp_tmp, ngcfg_get_path("maximus.display_path"), PATHLEN - 1);
    disp_tmp[PATHLEN - 1] = '\0';
    char *slash = strrchr(disp_tmp, '/');
    if (slash)
      *slash = '\0';  /* "…/display/screens" → "…/display" */
    snprintf(rippath, PATHLEN, "%s/rip", disp_tmp);
  }

  timeon=time(NULL);              /* Initalize time on/off counters */

#ifdef KEY
  if (key_ok)
    key_close(key_fd);
#endif


  /* Start up the input-processing threads */

  /*KickThreads();*/

#ifndef ORACLE

  if (do_useredit)
  {
    strcpy(usr.name, "\xff ");
    Config_Multitasker(FALSE);

    usr.video=GRAPH_ANSI;
    usr.bits |= BITS_TABS;
    usr.bits2 |= BITS2_CLS | BITS2_IBMCHARS;
    usr.width=80;
    usr.len=25;

    timeoff=timeon+(1440*60L);

    local=TRUE;

    OpenAreas();
    
    *log_name='\0';

    Fossil_Install(TRUE);
    User_Edit(NULL);

    AreaFileClose(ham);
    AreaFileClose(haf);

    ShutDownVideo();

    quit(0);
  }
  
#endif /* !ORACLE */

  timeoff=timestart+(unsigned long)(max_time*60L);
  /* Calc_Timeoff(); */

  do_timecheck=TRUE;


#ifndef ORACLE
  Blank_User(&usr);

  if (eqstr(log_name, c123)) /* No cmd-line param */
  {
    if (restart_system)
      *log_name='\0';
    else strnncpy(log_name, (char *)ngcfg_get_path("maximus.log_file"), LEN(80) - 1);
  }

  if (log_name && *log_name)  /* If we have a valid log name */
  {
    if (! LogOpen())
      quit(ERROR_CRITICAL);
    else
    {
      log_status.max = 1;
      if (!restart_system)
        LogWrite("\n");
    }
  }

#ifdef KEY
  if (key_ok && (key_outbuf=malloc(key_size))==NULL)
    key_ok=FALSE;
#endif

  if (!restart_system)
  {
    extern int _stdc main();

    if (task_num)
      logit(log_begin_mt, version, task_num);
    else
      logit(log_begin_1t, version);
  }
  
#ifdef KEY
  if (key_ok)
  {
    key_strip_ofs=key_unpack1(key_buf, key_outbuf, key_size);
    key_free(key_buf);
  }
#endif

  /* Only do a log msg if we're NOT restarting the system */

  Config_Multitasker(!restart_system);

  if (!restart_system)
  {
    char szUserFile[PATHLEN];

    {
      const char *system_name = ngcfg_get_string_raw("maximus.system_name");
      logit(" %s", system_name ? system_name : "");
    }

    strcpy(szUserFile, (char *)ngcfg_get_path("maximus.file_password"));
    strcat(szUserFile, ".db");

    if ((fd=shopen(szUserFile, O_RDONLY | O_BINARY)) == -1)
    {
      logit("!FATAL!  SQLite user database not found: %s", szUserFile);
      quit(ERROR_CRITICAL);
    }

    close(fd);

    node_file_path(task_num, "active.bbs", temp, sizeof(temp));

    if (fexist(temp))
    {
      unlink(temp);

      node_file_path(task_num, "lastus.bbs", temp, sizeof(temp));

      if ((fd=shopen(temp, O_RDONLY | O_BINARY)) != -1)
      {
        read(fd,(char *)&user, sizeof(struct _usr));
        close(fd);

        logit(log_syscrash1, task_num);
        logit(log_syscrash2, user.name);
      }
    }

    if (create_userbbs)
    {
      local=TRUE;
      waitforcaller=FALSE;
      port=0xff;
    }

    Fossil_Install(TRUE);
  }

#ifdef KEY
  if (key_ok && do_checksum(key_outbuf, key_size) != 0)
    key_ok=FALSE;
#endif

  mi.req_version=MSGAPI_VERSION;
  mi.def_zone=ngcfg_get_matrix_primary_address().zone;

  mi.palloc=max_palloc;
  mi.pfree=max_pfree;
  mi.repalloc=max_repalloc;

  mi.farpalloc=max_farpalloc;
  mi.farpfree=max_farpfree;
  mi.farrepalloc=max_farrepalloc;

  
  if (MsgOpenApi(&mi)==-1)
  {
    logit(log_err_msgapi);
    quit(ERROR_CRITICAL);
  }

#ifdef KEY
  if (!key_ok)
  {
    Lputs(err_no_key);
    quitfile();
  }
#endif


  /* Connect with the MCP server */

#ifdef MCP
  ChatOpenMCP();
#endif

  /* This must happen AFTER fossil_install and logopen! */

  OS2Init();

#ifdef KEY
  key_unpack2(key_outbuf, key_size, key_strip_ofs);
#endif

#endif /* !ORACLE */

#if defined(KEY) && !defined(ORACLE)
  return key_outbuf + CODE_1_SIZE;
#else
  return NULL;
#endif
}

void Read_Cfg(void)
{
  MaxCfgVar v;

#ifndef ORACLE
  startup_logit(": Read_Cfg: entry");
#endif

  if (maxcfg_abi_version() != LIBMAXCFG_ABI_VERSION)
  {
    startup_logit("!libmaxcfg ABI mismatch: compiled=%d runtime=%d", (int)LIBMAXCFG_ABI_VERSION, (int)maxcfg_abi_version());
    quitfile();
  }

  if (ng_cfg == NULL)
  {
    if (maxcfg_toml_init(&ng_cfg) == MAXCFG_OK)
    {
      (void)maxcfg_toml_load_file(ng_cfg, "config/maximus", "maximus");
      (void)maxcfg_toml_load_file(ng_cfg, "config/general/session", "general.session");
      (void)maxcfg_toml_load_file(ng_cfg, "config/general/display_files", "general.display_files");
      (void)maxcfg_toml_load_file(ng_cfg, "config/general/equipment", "general.equipment");
      (void)maxcfg_toml_load_file(ng_cfg, "config/general/colors", "general.colors");
      (void)maxcfg_toml_load_file(ng_cfg, "config/general/reader", "general.reader");
      (void)maxcfg_toml_load_file(ng_cfg, "config/general/protocol", "general.protocol");
      (void)maxcfg_toml_load_file(ng_cfg, "config/general/language", "general.language");
      (void)maxcfg_toml_load_file(ng_cfg, "config/security/access_levels", "security.access_levels");
      (void)maxcfg_toml_load_file(ng_cfg, "config/areas/msg/areas", "areas.msg");
      (void)maxcfg_toml_load_file(ng_cfg, "config/areas/file/areas", "areas.file");
      (void)maxcfg_toml_load_file(ng_cfg, "config/matrix", "matrix");

      load_menu_tomls();
    }
  }

  /* Now figure out which main menu to display */

  if (ng_cfg && maxcfg_toml_get(ng_cfg, "general.session.first_menu", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s && *v.v.s)
    strnncpy(main_menu, (char *)v.v.s, MAX_MENUNAME-1);
  else
  {
    /* Default already set in max_v.h */
  }

  /* Also grab the current MCP pipe, if not overridden on the command-line */

#ifdef MCP
  if (! *szMcpPipe)
  {
    if (ng_cfg && maxcfg_toml_get(ng_cfg, "maximus.mcp_pipe", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s && *v.v.s)
      strcpy(szMcpPipe, v.v.s);
    else
    {
      szMcpPipe[0] = '\0';
    }
  }
#endif

  /* Set the timeout counter... */

  {
    int input_timeout = ngcfg_get_int("general.session.input_timeout");
 
    if (input_timeout > 10) /* more than 60000 tics overflows a word */
      input_timeout = 10;

    timeout_tics=((word)input_timeout*60)*100;
  }
  
  /* If it's less than one minute, default to four mins */

  if (timeout_tics < 6000)
    timeout_tics=4*60*100;

}

void Read_Access()
{
  char temp[PATHLEN];
  char access_tmp_base[PATHLEN];
  const char *access_base;
  const char *sys_base;
  int have_toml_access = 0;
  MaxCfgVar v;
#ifndef ORACLE
  int plevels;
  extern PLIST *pl_privs;
#endif

  access_base = ngcfg_get_path("maximus.file_access");

  access_tmp_base[0] = '\0';
  if (build_access_dat_from_toml(access_tmp_base, sizeof(access_tmp_base)) == 0 && access_tmp_base[0] != '\0')
  {
    access_base = access_tmp_base;
    have_toml_access = 1;
  }

  if (ng_cfg && maxcfg_toml_get(ng_cfg, "maximus.file_access", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s && *v.v.s)
  {
    access_base = v.v.s;
    have_toml_access = 1;
  }

  sprintf(temp, ss, access_base, dotdat);
  if (!ClassReadFile(temp))
  {
    char temp2[PATHLEN];

    if (have_toml_access && temp[0] == '/')
      temp2[0] = '\0';
    else
    {
      sys_base = (char *)ngcfg_get_path("maximus.sys_path");

      strcpy(temp2, sys_base);
      strcat(temp2, temp);
    }

    if (temp2[0] == '\0' || !ClassReadFile(temp2))
    {
      Lprintf(cant_find_file, access_txt, temp);
      quitfile();
    }
  }

#ifndef ORACLE
  /* We allocate this here to avoid memory fragmentation */

  plevels=(int)ClassGetInfo(0,CIT_NUMCLASSES);
  if ((pl_privs=malloc((plevels+1)*sizeof(PLIST)))!=NULL)
  {
    int i;

    for (i=0; i < plevels; ++i)
    {
      pl_privs[i].name=ClassDesc(i);
      pl_privs[i].value=(int)ClassGetInfo(i,CIT_LEVEL);
    }
    pl_privs[i].name=NULL;
    pl_privs[i].value=-999;
  }
#endif
}

static void near Initialize_Colours(void)
{
  char temp[PATHLEN];
  int fd;
  const char *sys_base;
  const char *colors_dat;
  MaxCfgVar v;
  
  sys_base = (char *)ngcfg_get_path("maximus.sys_path");

  colors_dat = NULL;
  if (ng_cfg && maxcfg_toml_get(ng_cfg, "general.colors.colours_dat_path", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING && v.v.s && *v.v.s)
    colors_dat = v.v.s;

  if (colors_dat && *colors_dat)
    strnncpy(temp, (char *)colors_dat, sizeof(temp)-1);
  else
    sprintf(temp, "%s/config/colours.dat", sys_base);
  
  if ((fd=open(temp, O_RDONLY | O_BINARY))==-1)
  {
    cant_open(temp);
    quit(ERROR_CRITICAL);
  }
  
  if (read(fd, (char *)&col, sizeof(col)) != sizeof(col))
  {
    logit(cantread, temp);
    quit(ERROR_CRITICAL);
  }
  
  close(fd);
}

#ifndef ORACLE
  unsigned int Decimal_Baud_To_Mask(unsigned int bd)
  {
    static struct _baudxlt
    {
      unsigned int decimal;
      word mask;
    } baud[]={{  300u, BAUD300},
              {  600u, BAUD600},
              { 1200u, BAUD1200},
              { 2400u, BAUD2400},
              { 4800u, BAUD4800},
              { 9600u, BAUD9600},
              {19200u, BAUD19200},
              {38400u, BAUD38400},
  #ifndef __MSDOS__
              {57600u, BAUD57600},
              {115200u,BAUD115200},
  #endif
              {0,0}
             };
    struct _baudxlt *b;

    for (b=baud; b->decimal; b++)
      if (bd==b->decimal)
        return b->mask;

    /* Not found, so default to 38.4k */
    return BAUD38400;
  }

#endif /* !ORACLE */

static void near OpenAreas(void)
{
#ifndef ORACLE
  {
    char marea_base[PATHLEN];
    char farea_base[PATHLEN];
    int have_ng;

    if (debuglog)
      debug_log("OpenAreas: sys_path='%s' temp_path='%s' message_data='%s' file_data='%s'",
                ngcfg_get_string_raw("maximus.sys_path"),
                ngcfg_get_path("maximus.temp_path"),
                ngcfg_get_path("maximus.message_data"),
                ngcfg_get_path("maximus.file_data"));

    have_ng = build_area_dats_from_toml(marea_base, sizeof(marea_base),
                                        farea_base, sizeof(farea_base)) == 0;

    if (debuglog)
      debug_log("OpenAreas: build_area_dats_from_toml have_ng=%d marea_base='%s' farea_base='%s'",
                have_ng, marea_base, farea_base);

    if (have_ng)
      ham = AreaFileOpen(marea_base, TRUE);

    if (debuglog)
      debug_log("OpenAreas: AreaFileOpen(msg) have_ng=%d base='%s' ham=%p",
                have_ng, have_ng ? marea_base : "(n/a)", (void *)ham);

    if (ham == NULL)
      ham = AreaFileOpen((char *)ngcfg_get_path("maximus.message_data"), TRUE);

    if (debuglog)
      debug_log("OpenAreas: AreaFileOpen(msg) fallback base='%s' ham=%p",
                ngcfg_get_path("maximus.message_data"), (void *)ham);

    if (have_ng)
      haf = AreaFileOpen(farea_base, FALSE);

    if (debuglog)
      debug_log("OpenAreas: AreaFileOpen(file) have_ng=%d base='%s' haf=%p",
                have_ng, have_ng ? farea_base : "(n/a)", (void *)haf);

    if (haf == NULL)
      haf = AreaFileOpen((char *)ngcfg_get_path("maximus.file_data"), FALSE);

    if (debuglog)
      debug_log("OpenAreas: AreaFileOpen(file) fallback base='%s' haf=%p",
                ngcfg_get_path("maximus.file_data"), (void *)haf);
  }

  if (ham==NULL)
  {
    cant_open((char *)ngcfg_get_path("maximus.message_data"));
    vbuf_flush();
    Local_Beep(3);
    maximus_exit(ERROR_FILE);
  }

  if (haf==NULL)
  {
    cant_open((char *)ngcfg_get_path("maximus.file_data"));
    vbuf_flush();
    Local_Beep(3);
    maximus_exit(ERROR_FILE);
  }
#endif
}

static int near is_abs_path(const char *s)
{
  if (s == NULL || *s == '\0')
    return FALSE;

  if (s[0] == '/' || s[0] == '\\')
    return TRUE;

  if (isalpha((unsigned char)s[0]) && s[1] == ':')
    return TRUE;

  return FALSE;
}

static void near ensure_trailing_delim(char *s)
{
  size_t n;
  if (s == NULL)
    return;
  n = strlen(s);
  if (n == 0)
    return;
  if (n + 1 >= PATHLEN)
    return;
  if (s[n - 1] != PATH_DELIM)
  {
    s[n] = PATH_DELIM;
    s[n + 1] = '\0';
  }
}

/**
 * safe_path_join - Safely join two path components with validation
 * 
 * @base: Base directory path (may or may not have trailing delimiter)
 * @component: Path component to append (file or subdirectory)
 * @out: Output buffer for joined path
 * @out_sz: Size of output buffer
 * 
 * Returns: 0 on success, -1 on error
 * 
 * This function:
 * - Validates inputs (NULL checks, size checks)
 * - Detects if component is already absolute (returns it as-is)
 * - Ensures base has trailing delimiter before joining
 * - Prevents buffer overflows
 * 
 * Use this instead of raw snprintf("%s%s") or strcat() for path operations.
 */
int safe_path_join(const char *base, const char *component, char *out, size_t out_sz)
{
  char base_with_delim[PATHLEN];
  size_t base_len;
  size_t comp_len;

  if (out == NULL || out_sz == 0)
    return -1;

  out[0] = '\0';

  if (base == NULL)
    base = "";
  if (component == NULL)
    component = "";

  if (is_abs_path(component))
  {
    strnncpy(out, (char *)component, out_sz);
    return 0;
  }

  if (*base == '\0')
  {
    strnncpy(out, (char *)component, out_sz);
    return 0;
  }

  if (*component == '\0')
  {
    strnncpy(out, (char *)base, out_sz);
    return 0;
  }

  strnncpy(base_with_delim, (char *)base, sizeof(base_with_delim));
  ensure_trailing_delim(base_with_delim);

  base_len = strlen(base_with_delim);
  comp_len = strlen(component);

  if (base_len + comp_len + 1 > out_sz)
  {
    if (debuglog)
      debug_log("safe_path_join: overflow base='%s' component='%s' (need %lu, have %lu)",
                base, component, (unsigned long)(base_len + comp_len + 1), (unsigned long)out_sz);
    return -1;
  }

  snprintf(out, out_sz, "%s%s", base_with_delim, component);
  return 0;
}

static void near join_sys_path(const char *rel, char *out, size_t out_sz)
{
  const char *sys_base;

  if (out == NULL || out_sz == 0)
    return;

  sys_base = ngcfg_get_string_raw("maximus.sys_path");
  safe_path_join(sys_base, rel, out, out_sz);
}

static int near area_heap_add(char **hp, char *heap, size_t heap_sz, const char *s, zstr *out_off)
{
  size_t used;
  size_t n;

  if (hp == NULL || *hp == NULL || heap == NULL || heap_sz == 0 || out_off == NULL)
    return FALSE;

  if (s == NULL)
    s = "";

  used = (size_t)(*hp - heap);
  n = strlen(s) + 1;

  if (used >= heap_sz)
    return FALSE;
  if (n > heap_sz - used)
    return FALSE;

  *out_off = (zstr)used;
  memcpy(*hp, s, n);
  *hp += n;
  return TRUE;
}

static void near parse_msg_style(MaxCfgStrView style, word *attribs, word *attribs2, word *type)
{
  size_t i;

  if (attribs)
    *attribs = 0;
  if (attribs2)
    *attribs2 = 0;
  if (type)
    *type = 0;

  for (i = 0; i < style.count; i++)
  {
    const char *s = style.items[i];
    if (s == NULL || *s == '\0')
      continue;

    if (attribs)
    {
      if (stricmp(s, "Pvt") == 0 || stricmp(s, "Private") == 0)
        *attribs |= MA_PVT;
      else if (stricmp(s, "Pub") == 0 || stricmp(s, "Public") == 0)
        *attribs |= MA_PUB;
      else if (stricmp(s, "ReadOnly") == 0)
        *attribs |= MA_READONLY;
      else if (stricmp(s, "HiBit") == 0 || stricmp(s, "HighBit") == 0)
        *attribs |= MA_HIBIT;
      else if (stricmp(s, "Net") == 0 || stricmp(s, "Matrix") == 0)
        *attribs |= MA_NET;
      else if (stricmp(s, "Echo") == 0 || stricmp(s, "EchoMail") == 0)
        *attribs |= MA_ECHO;
      else if (stricmp(s, "Conf") == 0 || stricmp(s, "Conference") == 0)
        *attribs |= MA_CONF;
      else if (stricmp(s, "Anon") == 0 || stricmp(s, "Anonymous") == 0)
        *attribs |= MA_ANON;
      else if (stricmp(s, "NoNameKludge") == 0)
        *attribs |= MA_NORNK;
      else if (stricmp(s, "RealName") == 0)
        *attribs |= MA_REAL;
      else if (stricmp(s, "Alias") == 0)
        *attribs |= MA_ALIAS;
      else if (stricmp(s, "Audit") == 0)
        *attribs |= MA_AUDIT;
      else if (stricmp(s, "Hidden") == 0)
        *attribs |= MA_HIDDN;
      else if (stricmp(s, "Attach") == 0)
        *attribs |= MA_ATTACH;
    }

    if (attribs2)
    {
      if (stricmp(s, "NoMailCheck") == 0)
        *attribs2 |= MA2_NOMCHK;
    }

    if (type)
    {
      if (stricmp(s, "Squish") == 0)
        *type = MSGTYPE_SQUISH;
      else if (stricmp(s, "SDM") == 0 || stricmp(s, "*.MSG") == 0)
        *type = MSGTYPE_SDM;
    }
  }

  if (type && *type == 0)
    *type = MSGTYPE_SQUISH;
  if (attribs && ((*attribs & (MA_PUB | MA_PVT)) == 0))
    *attribs |= MA_PUB;
}

static word near parse_file_types(MaxCfgStrView types)
{
  size_t i;
  word attribs = 0;

  for (i = 0; i < types.count; i++)
  {
    const char *t = types.items[i];
    if (t == NULL || *t == '\0')
      continue;

    if (stricmp(t, "Slow") == 0)
      attribs |= FA_SLOW;
    else if (stricmp(t, "Staged") == 0)
      attribs |= FA_STAGED;
    else if (stricmp(t, "NoNew") == 0)
      attribs |= FA_NONEW;
    else if (stricmp(t, "CD") == 0)
      attribs |= FA_CDROM;
    else if (stricmp(t, "Hidden") == 0)
      attribs |= FA_HIDDN;
    else if (stricmp(t, "DateAuto") == 0)
      attribs |= FA_AUTODATE;
    else if (stricmp(t, "DateManual") == 0)
      attribs |= FA_MANDATE;
    else if (stricmp(t, "DateList") == 0)
      attribs |= FA_LISTDATE;
    else if (stricmp(t, "FreeTime") == 0)
      attribs |= FA_FREETIME;
    else if (stricmp(t, "FreeSize") == 0 || stricmp(t, "FreeBytes") == 0)
      attribs |= FA_FREESIZE;
    else if (stricmp(t, "Free") == 0)
      attribs |= FA_FREEALL;
    else if (stricmp(t, "NoIndex") == 0)
      attribs |= FA_NOINDEX;
  }

  return attribs;
}

static int near write_id(int fd, dword id)
{
  if (fd < 0)
    return -1;
  if (write(fd, (char *)&id, sizeof(id)) != sizeof(id))
    return -1;
  return 0;
}

static int near build_marea_from_toml(const char *base)
{
  int dat_fd = -1;
  int idx_fd = -1;
  char dat_path[PATHLEN];
  char idx_path[PATHLEN];
  MaxCfgVar divs;
  MaxCfgVar areas;
  size_t div_count = 0;
  size_t area_count = 0;
  size_t i;
  size_t j;
  word div_no = 0;
  long cbLast = 0;
  long last_rec_size = 0;
  const char *fail_reason = NULL;

  if (base == NULL || *base == '\0')
    return -1;

  snprintf(dat_path, sizeof(dat_path), "%s.dat", base);
  snprintf(idx_path, sizeof(idx_path), "%s.idx", base);

  if (debuglog)
    debug_log("build_marea_from_toml: base='%s' dat='%s' idx='%s'", base, dat_path, idx_path);

  dat_fd = sopen(dat_path, O_CREAT | O_TRUNC | O_RDWR | O_BINARY,
                 SH_DENYNO, S_IREAD | S_IWRITE);
  if (dat_fd == -1)
    return -1;

  idx_fd = sopen(idx_path, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
                 SH_DENYNO, S_IREAD | S_IWRITE);
  if (idx_fd == -1)
  {
    close(dat_fd);
    return -1;
  }

  if (write_id(dat_fd, MAREA_ID) != 0)
  {
    fail_reason = "write_id";
    goto fail;
  }

  if (maxcfg_toml_get(ng_cfg, "areas.msg.division", &divs) == MAXCFG_OK)
  {
    if (divs.type == MAXCFG_VAR_TABLE_ARRAY)
      (void)maxcfg_var_count(&divs, &div_count);
  }

  if (maxcfg_toml_get(ng_cfg, "areas.msg.area", &areas) != MAXCFG_OK ||
      areas.type != MAXCFG_VAR_TABLE_ARRAY ||
      maxcfg_var_count(&areas, &area_count) != MAXCFG_OK)
  {
    fail_reason = "areas.msg.area missing/invalid";
    goto fail;
  }

  if (debuglog)
    debug_log("build_marea_from_toml: div_count=%lu area_count=%lu", (unsigned long)div_count, (unsigned long)area_count);

  for (i = 0; i < div_count; i++)
  {
    MaxCfgVar dv;
    MaxCfgVar v;
    const char *key = "";
    const char *desc = "";
    const char *acs = "";
    const char *dsp = "";

    if (maxcfg_toml_array_get(&divs, i, &dv) != MAXCFG_OK || dv.type != MAXCFG_VAR_TABLE)
      continue;

    if (maxcfg_toml_table_get(&dv, "key", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
      key = v.v.s;
    if (maxcfg_toml_table_get(&dv, "description", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
      desc = v.v.s;
    if (maxcfg_toml_table_get(&dv, "acs", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
      acs = v.v.s;
    if (maxcfg_toml_table_get(&dv, "display_file", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
      dsp = v.v.s;

    {
      MAREA ma;
      char heap[PATHLEN * 4];
      char *hp = heap;
      size_t heap_sz;
      MFIDX mfi;
      zstr off_name;
      zstr off_acs;
      zstr off_path;
      zstr off_desc;

      memset(&ma, 0, sizeof(ma));
      memset(heap, 0, sizeof(heap));

      ma.cbArea = sizeof(ma);
      ma.num_override = 0;
      ma.cbPrior = cbLast;
      ma.attribs = MA_DIVBEGIN;
      ma.division = div_no;

      ma.primary = ngcfg_get_matrix_primary_address();
      ma.seenby = ngcfg_get_matrix_seenby_address();

      *hp++ = '\0';
      if (!area_heap_add(&hp, heap, sizeof(heap), key, &off_name) ||
          !area_heap_add(&hp, heap, sizeof(heap), acs, &off_acs) ||
          !area_heap_add(&hp, heap, sizeof(heap), dsp, &off_path) ||
          !area_heap_add(&hp, heap, sizeof(heap), desc, &off_desc))
      {
        fail_reason = "division heap overflow";
        goto fail;
      }

      ma.name = off_name;
      ma.acs = off_acs;
      ma.path = off_path;
      ma.descript = off_desc;

      heap_sz = (size_t)(hp - heap);
      ma.cbHeap = (word)heap_sz;

      memset(&mfi, 0, sizeof(mfi));
      strncpy(mfi.name, key ? key : "", sizeof(mfi.name) - 1);
      mfi.name_hash = SquishHash((byte *)(void *)(key ? key : ""));
      mfi.ofs = (dword)tell(dat_fd);

      if (write(idx_fd, (char *)&mfi, sizeof(mfi)) != sizeof(mfi))
      {
        fail_reason = "division write idx";
        goto fail;
      }
      if (write(dat_fd, (char *)&ma, sizeof(ma)) != sizeof(ma))
      {
        fail_reason = "division write dat";
        goto fail;
      }
      if (write(dat_fd, heap, heap_sz) != (signed)heap_sz)
      {
        fail_reason = "division write heap";
        goto fail;
      }

      last_rec_size = (long)sizeof(ma) + (long)heap_sz;
      cbLast = last_rec_size;
      div_no++;
    }

    for (j = 0; j < area_count; j++)
    {
      MaxCfgVar at;
      MaxCfgVar v;
      const char *division = "";
      const char *name = "";
      const char *desc2 = "";
      const char *acs2 = "";
      const char *menu = "";
      const char *tag = "";
      const char *path = "";
      const char *origin = "";
      const char *attach_path = "";
      const char *barricade = "";
      MaxCfgStrView style = {0};
      int renum_max = 0;
      int renum_days = 0;

      if (maxcfg_toml_array_get(&areas, j, &at) != MAXCFG_OK || at.type != MAXCFG_VAR_TABLE)
        continue;

      if (maxcfg_toml_table_get(&at, "division", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        division = v.v.s;
      if (division == NULL || stricmp(division, key) != 0)
        continue;

      if (maxcfg_toml_table_get(&at, "name", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        name = v.v.s;
      if (maxcfg_toml_table_get(&at, "description", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        desc2 = v.v.s;
      if (maxcfg_toml_table_get(&at, "acs", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        acs2 = v.v.s;
      if (maxcfg_toml_table_get(&at, "menu", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        menu = v.v.s;
      if (maxcfg_toml_table_get(&at, "tag", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        tag = v.v.s;
      if (maxcfg_toml_table_get(&at, "path", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        path = v.v.s;
      if (maxcfg_toml_table_get(&at, "origin", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        origin = v.v.s;
      if (maxcfg_toml_table_get(&at, "attach_path", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        attach_path = v.v.s;
      if (maxcfg_toml_table_get(&at, "barricade", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        barricade = v.v.s;
      if (maxcfg_toml_table_get(&at, "style", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING_ARRAY)
        style = v.v.strv;
      if (maxcfg_toml_table_get(&at, "renum_max", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
        renum_max = v.v.i;
      if (maxcfg_toml_table_get(&at, "renum_days", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
        renum_days = v.v.i;

      {
        MAREA ma;
        char heap[PATHLEN * 8];
        char *hp = heap;
        size_t heap_sz;
        MFIDX mfi;
        char full_name[PATHLEN];
        char full_path[PATHLEN];
        zstr off_name;
        zstr off_acs;
        zstr off_path;
        zstr off_tag;
        zstr off_desc;
        zstr off_origin;
        zstr off_menu;
        zstr off_barr;
        zstr off_attach;

        memset(&ma, 0, sizeof(ma));
        memset(heap, 0, sizeof(heap));
        memset(full_name, 0, sizeof(full_name));
        memset(full_path, 0, sizeof(full_path));

        if (key && *key)
          snprintf(full_name, sizeof(full_name), "%s.%s", key, name ? name : "");
        else
          strnncpy(full_name, (char *)(name ? name : ""), sizeof(full_name));

        join_sys_path(path, full_path, sizeof(full_path));

        if (debuglog)
          debug_log("build_marea_from_toml: area div='%s' name='%s' full_name='%s' path='%s' full_path='%s' acs='%s'",
                    division ? division : "", name ? name : "", full_name, path ? path : "", full_path, acs2 ? acs2 : "");

        ma.cbArea = sizeof(ma);
        ma.num_override = 0;
        ma.cbPrior = cbLast;
        ma.killbynum = (word)renum_max;
        ma.killbyage = (word)renum_days;
        ma.killskip = 0;

        ma.primary = ngcfg_get_matrix_primary_address();
        ma.seenby = ngcfg_get_matrix_seenby_address();

        {
          word attribs_tmp;
          word attribs2_tmp;
          word type_tmp;

          parse_msg_style(style, &attribs_tmp, &attribs2_tmp, &type_tmp);
          ma.attribs = attribs_tmp;
          ma.attribs_2 = attribs2_tmp;
          ma.type = type_tmp;

          if ((type_tmp & MSGTYPE_SQUISH) && full_path[0] != '\0')
          {
            size_t len = strlen(full_path);
            if (len > 0 && full_path[len - 1] == PATH_DELIM)
              full_path[len - 1] = '\0';
          }
        }

        if (full_path[0] == '\0')
        {
          fail_reason = "area full_path empty";
          goto fail;
        }

        *hp++ = '\0';
        if (!area_heap_add(&hp, heap, sizeof(heap), full_name, &off_name) ||
            !area_heap_add(&hp, heap, sizeof(heap), acs2, &off_acs) ||
            !area_heap_add(&hp, heap, sizeof(heap), full_path, &off_path) ||
            !area_heap_add(&hp, heap, sizeof(heap), tag, &off_tag) ||
            !area_heap_add(&hp, heap, sizeof(heap), desc2, &off_desc) ||
            !area_heap_add(&hp, heap, sizeof(heap), origin, &off_origin) ||
            !area_heap_add(&hp, heap, sizeof(heap), menu, &off_menu) ||
            !area_heap_add(&hp, heap, sizeof(heap), barricade, &off_barr) ||
            !area_heap_add(&hp, heap, sizeof(heap), attach_path, &off_attach))
        {
          fail_reason = "area heap overflow";
          goto fail;
        }

        ma.name = off_name;
        ma.acs = off_acs;
        ma.path = off_path;
        ma.echo_tag = off_tag;
        ma.descript = off_desc;
        ma.origin = off_origin;
        ma.menuname = off_menu;
        ma.barricade = off_barr;
        ma.attachpath = off_attach;

        heap_sz = (size_t)(hp - heap);
        ma.cbHeap = (word)heap_sz;

        memset(&mfi, 0, sizeof(mfi));
        strncpy(mfi.name, full_name, sizeof(mfi.name) - 1);
        mfi.name_hash = SquishHash((byte *)(void *)full_name);
        mfi.ofs = (dword)tell(dat_fd);

        if (write(idx_fd, (char *)&mfi, sizeof(mfi)) != sizeof(mfi))
        {
          fail_reason = "area write idx";
          goto fail;
        }
        if (write(dat_fd, (char *)&ma, sizeof(ma)) != sizeof(ma))
        {
          fail_reason = "area write dat";
          goto fail;
        }
        if (write(dat_fd, heap, heap_sz) != (signed)heap_sz)
        {
          fail_reason = "area write heap";
          goto fail;
        }

        last_rec_size = (long)sizeof(ma) + (long)heap_sz;
        cbLast = last_rec_size;
      }
    }

    {
      MAREA ma;
      char heap[2];
      char *hp = heap;
      size_t heap_sz;
      MFIDX mfi;

      memset(&ma, 0, sizeof(ma));
      memset(heap, 0, sizeof(heap));

      ma.cbArea = sizeof(ma);
      ma.num_override = 0;
      ma.cbPrior = cbLast;
      ma.attribs = MA_DIVEND;
      ma.division = div_no - 1;
      ma.primary = ngcfg_get_matrix_primary_address();
      ma.seenby = ngcfg_get_matrix_seenby_address();

      *hp++ = '\0';
      heap_sz = (size_t)(hp - heap);
      ma.cbHeap = (word)heap_sz;

      memset(&mfi, 0, sizeof(mfi));
      strncpy(mfi.name, key ? key : "", sizeof(mfi.name) - 1);
      mfi.name_hash = SquishHash((byte *)(void *)(key ? key : ""));
      mfi.ofs = (dword)tell(dat_fd);

      if (write(idx_fd, (char *)&mfi, sizeof(mfi)) != sizeof(mfi))
        goto fail;
      if (write(dat_fd, (char *)&ma, sizeof(ma)) != sizeof(ma))
        goto fail;
      if (write(dat_fd, heap, heap_sz) != (signed)heap_sz)
        goto fail;

      last_rec_size = (long)sizeof(ma) + (long)heap_sz;
      cbLast = last_rec_size;
    }
  }

  for (j = 0; j < area_count; j++)
  {
    MaxCfgVar at;
    MaxCfgVar v;
    const char *division = "";

    if (maxcfg_toml_array_get(&areas, j, &at) != MAXCFG_OK || at.type != MAXCFG_VAR_TABLE)
      continue;
    if (maxcfg_toml_table_get(&at, "division", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
      division = v.v.s;
    if (division && *division)
      continue;

    {
      const char *name = "";
      const char *desc2 = "";
      const char *acs2 = "";
      const char *menu = "";
      const char *tag = "";
      const char *path = "";
      const char *origin = "";
      const char *attach_path = "";
      const char *barricade = "";
      MaxCfgStrView style = {0};
      int renum_max = 0;
      int renum_days = 0;
      MAREA ma;
      char heap[PATHLEN * 8];
      char *hp = heap;
      size_t heap_sz;
      MFIDX mfi;
      char full_name[PATHLEN];
      char full_path[PATHLEN];
      zstr off_name;
      zstr off_acs;
      zstr off_path;
      zstr off_tag;
      zstr off_desc;
      zstr off_origin;
      zstr off_menu;
      zstr off_barr;
      zstr off_attach;

      if (maxcfg_toml_table_get(&at, "name", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        name = v.v.s;
      if (maxcfg_toml_table_get(&at, "description", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        desc2 = v.v.s;
      if (maxcfg_toml_table_get(&at, "acs", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        acs2 = v.v.s;
      if (maxcfg_toml_table_get(&at, "menu", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        menu = v.v.s;
      if (maxcfg_toml_table_get(&at, "tag", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        tag = v.v.s;
      if (maxcfg_toml_table_get(&at, "path", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        path = v.v.s;
      if (maxcfg_toml_table_get(&at, "origin", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        origin = v.v.s;
      if (maxcfg_toml_table_get(&at, "attach_path", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        attach_path = v.v.s;
      if (maxcfg_toml_table_get(&at, "barricade", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        barricade = v.v.s;
      if (maxcfg_toml_table_get(&at, "style", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING_ARRAY)
        style = v.v.strv;
      if (maxcfg_toml_table_get(&at, "renum_max", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
        renum_max = v.v.i;
      if (maxcfg_toml_table_get(&at, "renum_days", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
        renum_days = v.v.i;

      join_sys_path(path, full_path, sizeof(full_path));

      memset(&ma, 0, sizeof(ma));
      memset(heap, 0, sizeof(heap));
      ma.cbArea = sizeof(ma);
      ma.num_override = 0;
      ma.cbPrior = cbLast;
      ma.killbynum = (word)renum_max;
      ma.killbyage = (word)renum_days;
      ma.killskip = 0;
      ma.primary = ngcfg_get_matrix_primary_address();
      ma.seenby = ngcfg_get_matrix_seenby_address();
      {
        word attribs_tmp;
        word attribs2_tmp;
        word type_tmp;

        parse_msg_style(style, &attribs_tmp, &attribs2_tmp, &type_tmp);
        ma.attribs = attribs_tmp;
        ma.attribs_2 = attribs2_tmp;
        ma.type = type_tmp;

        if ((type_tmp & MSGTYPE_SQUISH) && full_path[0] != '\0')
        {
          size_t len = strlen(full_path);
          if (len > 0 && full_path[len - 1] == PATH_DELIM)
            full_path[len - 1] = '\0';
        }
      }

      if (full_path[0] == '\0')
        goto fail;

      *hp++ = '\0';
      if (!area_heap_add(&hp, heap, sizeof(heap), name, &off_name) ||
          !area_heap_add(&hp, heap, sizeof(heap), acs2, &off_acs) ||
          !area_heap_add(&hp, heap, sizeof(heap), full_path, &off_path) ||
          !area_heap_add(&hp, heap, sizeof(heap), tag, &off_tag) ||
          !area_heap_add(&hp, heap, sizeof(heap), desc2, &off_desc) ||
          !area_heap_add(&hp, heap, sizeof(heap), origin, &off_origin) ||
          !area_heap_add(&hp, heap, sizeof(heap), menu, &off_menu) ||
          !area_heap_add(&hp, heap, sizeof(heap), barricade, &off_barr) ||
          !area_heap_add(&hp, heap, sizeof(heap), attach_path, &off_attach))
        goto fail;

      ma.name = off_name;
      ma.acs = off_acs;
      ma.path = off_path;
      ma.echo_tag = off_tag;
      ma.descript = off_desc;
      ma.origin = off_origin;
      ma.menuname = off_menu;
      ma.barricade = off_barr;
      ma.attachpath = off_attach;

      heap_sz = (size_t)(hp - heap);
      ma.cbHeap = (word)heap_sz;

      memset(&mfi, 0, sizeof(mfi));
      strncpy(mfi.name, name ? name : "", sizeof(mfi.name) - 1);
      mfi.name_hash = SquishHash((byte *)(void *)(name ? name : ""));
      mfi.ofs = (dword)tell(dat_fd);

      if (write(idx_fd, (char *)&mfi, sizeof(mfi)) != sizeof(mfi))
        goto fail;
      if (write(dat_fd, (char *)&ma, sizeof(ma)) != sizeof(ma))
        goto fail;
      if (write(dat_fd, heap, heap_sz) != (signed)heap_sz)
        goto fail;

      last_rec_size = (long)sizeof(ma) + (long)heap_sz;
      cbLast = last_rec_size;
    }
  }

  if (last_rec_size > 0)
  {
    MAREA ma;
    long end_pos = lseek(dat_fd, 0L, SEEK_END);
    long pos = end_pos - last_rec_size - ADATA_START;

    lseek(dat_fd, ADATA_START, SEEK_SET);
    if (read(dat_fd, (char *)&ma, sizeof(ma)) != sizeof(ma))
      goto fail;

    ma.cbPrior = -pos;
    lseek(dat_fd, ADATA_START, SEEK_SET);
    if (write(dat_fd, (char *)&ma, sizeof(ma)) != sizeof(ma))
      goto fail;
  }

  close(dat_fd);
  close(idx_fd);
  return 0;

fail:
  if (debuglog)
    debug_log("build_marea_from_toml: FAIL reason='%s' base='%s'", fail_reason ? fail_reason : "(unknown)", base ? base : "(null)");
  if (dat_fd != -1)
    close(dat_fd);
  if (idx_fd != -1)
    close(idx_fd);
  return -1;
}

static int near build_farea_from_toml(const char *base)
{
  int dat_fd = -1;
  int idx_fd = -1;
  char dat_path[PATHLEN];
  char idx_path[PATHLEN];
  MaxCfgVar divs;
  MaxCfgVar areas;
  size_t div_count = 0;
  size_t area_count = 0;
  size_t i;
  size_t j;
  word div_no = 0;
  long cbLast = 0;
  long last_rec_size = 0;

  if (base == NULL || *base == '\0')
    return -1;

  snprintf(dat_path, sizeof(dat_path), "%s.dat", base);
  snprintf(idx_path, sizeof(idx_path), "%s.idx", base);

  dat_fd = sopen(dat_path, O_CREAT | O_TRUNC | O_RDWR | O_BINARY,
                 SH_DENYNO, S_IREAD | S_IWRITE);
  if (dat_fd == -1)
    return -1;

  idx_fd = sopen(idx_path, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
                 SH_DENYNO, S_IREAD | S_IWRITE);
  if (idx_fd == -1)
  {
    close(dat_fd);
    return -1;
  }

  if (write_id(dat_fd, FAREA_ID) != 0)
    goto fail;

  if (maxcfg_toml_get(ng_cfg, "areas.file.division", &divs) == MAXCFG_OK)
  {
    if (divs.type == MAXCFG_VAR_TABLE_ARRAY)
      (void)maxcfg_var_count(&divs, &div_count);
  }

  if (maxcfg_toml_get(ng_cfg, "areas.file.area", &areas) != MAXCFG_OK ||
      areas.type != MAXCFG_VAR_TABLE_ARRAY ||
      maxcfg_var_count(&areas, &area_count) != MAXCFG_OK)
    goto fail;

  for (i = 0; i < div_count; i++)
  {
    MaxCfgVar dv;
    MaxCfgVar v;
    const char *key = "";
    const char *desc = "";
    const char *acs = "";
    const char *dsp = "";

    if (maxcfg_toml_array_get(&divs, i, &dv) != MAXCFG_OK || dv.type != MAXCFG_VAR_TABLE)
      continue;

    if (maxcfg_toml_table_get(&dv, "key", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
      key = v.v.s;
    if (maxcfg_toml_table_get(&dv, "description", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
      desc = v.v.s;
    if (maxcfg_toml_table_get(&dv, "acs", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
      acs = v.v.s;
    if (maxcfg_toml_table_get(&dv, "display_file", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
      dsp = v.v.s;

    {
      FAREA fa;
      char heap[PATHLEN * 4];
      char *hp = heap;
      size_t heap_sz;
      MFIDX mfi;
      zstr off_acs;
      zstr off_name;
      zstr off_files;
      zstr off_desc;

      memset(&fa, 0, sizeof(fa));
      memset(heap, 0, sizeof(heap));

      fa.cbArea = sizeof(fa);
      fa.num_override = 0;
      fa.cbPrior = cbLast;
      fa.attribs = FA_DIVBEGIN;
      fa.division = div_no;

      *hp++ = '\0';
      if (!area_heap_add(&hp, heap, sizeof(heap), acs, &off_acs) ||
          !area_heap_add(&hp, heap, sizeof(heap), key, &off_name) ||
          !area_heap_add(&hp, heap, sizeof(heap), dsp, &off_files) ||
          !area_heap_add(&hp, heap, sizeof(heap), desc, &off_desc))
        goto fail;

      fa.acs = off_acs;
      fa.name = off_name;
      fa.filesbbs = off_files;
      fa.descript = off_desc;

      heap_sz = (size_t)(hp - heap);
      fa.cbHeap = (word)heap_sz;

      memset(&mfi, 0, sizeof(mfi));
      strncpy(mfi.name, key ? key : "", sizeof(mfi.name) - 1);
      mfi.name_hash = SquishHash((byte *)(void *)(key ? key : ""));
      mfi.ofs = (dword)tell(dat_fd);

      if (write(idx_fd, (char *)&mfi, sizeof(mfi)) != sizeof(mfi))
        goto fail;
      if (write(dat_fd, (char *)&fa, sizeof(fa)) != sizeof(fa))
        goto fail;
      if (write(dat_fd, heap, heap_sz) != (signed)heap_sz)
        goto fail;

      last_rec_size = (long)sizeof(fa) + (long)heap_sz;
      cbLast = last_rec_size;
      div_no++;
    }

    for (j = 0; j < area_count; j++)
    {
      MaxCfgVar at;
      MaxCfgVar v;
      const char *division = "";
      const char *name = "";
      const char *desc2 = "";
      const char *acs2 = "";
      const char *menu = "";
      const char *download = "";
      const char *upload = "";
      const char *filelist = "";
      const char *barricade = "";
      MaxCfgStrView types = {0};

      if (maxcfg_toml_array_get(&areas, j, &at) != MAXCFG_OK || at.type != MAXCFG_VAR_TABLE)
        continue;

      if (maxcfg_toml_table_get(&at, "division", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        division = v.v.s;
      if (division == NULL || stricmp(division, key) != 0)
        continue;

      if (maxcfg_toml_table_get(&at, "name", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        name = v.v.s;
      if (maxcfg_toml_table_get(&at, "description", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        desc2 = v.v.s;
      if (maxcfg_toml_table_get(&at, "acs", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        acs2 = v.v.s;
      if (maxcfg_toml_table_get(&at, "menu", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        menu = v.v.s;
      if (maxcfg_toml_table_get(&at, "download", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        download = v.v.s;
      if (maxcfg_toml_table_get(&at, "upload", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        upload = v.v.s;
      if (maxcfg_toml_table_get(&at, "filelist", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        filelist = v.v.s;
      if (maxcfg_toml_table_get(&at, "barricade", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        barricade = v.v.s;
      if (maxcfg_toml_table_get(&at, "types", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING_ARRAY)
        types = v.v.strv;

      {
        FAREA fa;
        char heap[PATHLEN * 8];
        char *hp = heap;
        size_t heap_sz;
        MFIDX mfi;
        char full_name[PATHLEN];
        char down_full[PATHLEN];
        char up_full[PATHLEN];
        char filelist_full[PATHLEN];
        zstr off_acs;
        zstr off_name;
        zstr off_down;
        zstr off_up;
        zstr off_files;
        zstr off_desc;
        zstr off_menu;
        zstr off_barr;

        memset(&fa, 0, sizeof(fa));
        memset(heap, 0, sizeof(heap));
        memset(full_name, 0, sizeof(full_name));

        if (key && *key)
          snprintf(full_name, sizeof(full_name), "%s.%s", key, name ? name : "");
        else
          strnncpy(full_name, (char *)(name ? name : ""), sizeof(full_name));

        join_sys_path(download, down_full, sizeof(down_full));
        join_sys_path(upload, up_full, sizeof(up_full));
        join_sys_path(filelist, filelist_full, sizeof(filelist_full));

        if (down_full[0] == '\0')
          goto fail;

        fa.cbArea = sizeof(fa);
        fa.num_override = 0;
        fa.cbPrior = cbLast;
        fa.attribs = parse_file_types(types);

        *hp++ = '\0';
        if (!area_heap_add(&hp, heap, sizeof(heap), acs2, &off_acs) ||
            !area_heap_add(&hp, heap, sizeof(heap), full_name, &off_name) ||
            !area_heap_add(&hp, heap, sizeof(heap), down_full, &off_down) ||
            !area_heap_add(&hp, heap, sizeof(heap), up_full, &off_up) ||
            !area_heap_add(&hp, heap, sizeof(heap), filelist_full, &off_files) ||
            !area_heap_add(&hp, heap, sizeof(heap), desc2, &off_desc) ||
            !area_heap_add(&hp, heap, sizeof(heap), menu, &off_menu) ||
            !area_heap_add(&hp, heap, sizeof(heap), barricade, &off_barr))
          goto fail;

        fa.acs = off_acs;
        fa.name = off_name;
        fa.downpath = off_down;
        fa.uppath = off_up;
        fa.filesbbs = off_files;
        fa.descript = off_desc;
        fa.menuname = off_menu;
        fa.barricade = off_barr;

        heap_sz = (size_t)(hp - heap);
        fa.cbHeap = (word)heap_sz;

        memset(&mfi, 0, sizeof(mfi));
        strncpy(mfi.name, full_name, sizeof(mfi.name) - 1);
        mfi.name_hash = SquishHash((byte *)(void *)full_name);
        mfi.ofs = (dword)tell(dat_fd);

        if (write(idx_fd, (char *)&mfi, sizeof(mfi)) != sizeof(mfi))
          goto fail;
        if (write(dat_fd, (char *)&fa, sizeof(fa)) != sizeof(fa))
          goto fail;
        if (write(dat_fd, heap, heap_sz) != (signed)heap_sz)
          goto fail;

        last_rec_size = (long)sizeof(fa) + (long)heap_sz;
        cbLast = last_rec_size;
      }
    }

    {
      FAREA fa;
      char heap[2];
      char *hp = heap;
      size_t heap_sz;
      MFIDX mfi;

      memset(&fa, 0, sizeof(fa));
      memset(heap, 0, sizeof(heap));

      fa.cbArea = sizeof(fa);
      fa.num_override = 0;
      fa.cbPrior = cbLast;
      fa.attribs = FA_DIVEND;
      fa.division = div_no - 1;

      *hp++ = '\0';
      heap_sz = (size_t)(hp - heap);
      fa.cbHeap = (word)heap_sz;

      memset(&mfi, 0, sizeof(mfi));
      strncpy(mfi.name, key ? key : "", sizeof(mfi.name) - 1);
      mfi.name_hash = SquishHash((byte *)(void *)(key ? key : ""));
      mfi.ofs = (dword)tell(dat_fd);

      if (write(idx_fd, (char *)&mfi, sizeof(mfi)) != sizeof(mfi))
        goto fail;
      if (write(dat_fd, (char *)&fa, sizeof(fa)) != sizeof(fa))
        goto fail;
      if (write(dat_fd, heap, heap_sz) != (signed)heap_sz)
        goto fail;

      last_rec_size = (long)sizeof(fa) + (long)heap_sz;
      cbLast = last_rec_size;
    }
  }

  for (j = 0; j < area_count; j++)
  {
    MaxCfgVar at;
    MaxCfgVar v;
    const char *division = "";

    if (maxcfg_toml_array_get(&areas, j, &at) != MAXCFG_OK || at.type != MAXCFG_VAR_TABLE)
      continue;
    if (maxcfg_toml_table_get(&at, "division", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
      division = v.v.s;
    if (division && *division)
      continue;

    {
      const char *name = "";
      const char *desc2 = "";
      const char *acs2 = "";
      const char *menu = "";
      const char *download = "";
      const char *upload = "";
      const char *filelist = "";
      const char *barricade = "";
      MaxCfgStrView types = {0};
      FAREA fa;
      char heap[PATHLEN * 8];
      char *hp = heap;
      size_t heap_sz;
      MFIDX mfi;
      char down_full[PATHLEN];
      char up_full[PATHLEN];
      char filelist_full[PATHLEN];
      zstr off_acs;
      zstr off_name;
      zstr off_down;
      zstr off_up;
      zstr off_files;
      zstr off_desc;
      zstr off_menu;
      zstr off_barr;

      if (maxcfg_toml_table_get(&at, "name", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        name = v.v.s;
      if (maxcfg_toml_table_get(&at, "description", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        desc2 = v.v.s;
      if (maxcfg_toml_table_get(&at, "acs", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        acs2 = v.v.s;
      if (maxcfg_toml_table_get(&at, "menu", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        menu = v.v.s;
      if (maxcfg_toml_table_get(&at, "download", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        download = v.v.s;
      if (maxcfg_toml_table_get(&at, "upload", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        upload = v.v.s;
      if (maxcfg_toml_table_get(&at, "filelist", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        filelist = v.v.s;
      if (maxcfg_toml_table_get(&at, "barricade", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING)
        barricade = v.v.s;
      if (maxcfg_toml_table_get(&at, "types", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING_ARRAY)
        types = v.v.strv;

      join_sys_path(download, down_full, sizeof(down_full));
      join_sys_path(upload, up_full, sizeof(up_full));
      join_sys_path(filelist, filelist_full, sizeof(filelist_full));

      if (down_full[0] == '\0')
        goto fail;

      memset(&fa, 0, sizeof(fa));
      memset(heap, 0, sizeof(heap));
      fa.cbArea = sizeof(fa);
      fa.num_override = 0;
      fa.cbPrior = cbLast;
      fa.attribs = parse_file_types(types);

      *hp++ = '\0';
      if (!area_heap_add(&hp, heap, sizeof(heap), acs2, &off_acs) ||
          !area_heap_add(&hp, heap, sizeof(heap), name, &off_name) ||
          !area_heap_add(&hp, heap, sizeof(heap), down_full, &off_down) ||
          !area_heap_add(&hp, heap, sizeof(heap), up_full, &off_up) ||
          !area_heap_add(&hp, heap, sizeof(heap), filelist_full, &off_files) ||
          !area_heap_add(&hp, heap, sizeof(heap), desc2, &off_desc) ||
          !area_heap_add(&hp, heap, sizeof(heap), menu, &off_menu) ||
          !area_heap_add(&hp, heap, sizeof(heap), barricade, &off_barr))
        goto fail;

      fa.acs = off_acs;
      fa.name = off_name;
      fa.downpath = off_down;
      fa.uppath = off_up;
      fa.filesbbs = off_files;
      fa.descript = off_desc;
      fa.menuname = off_menu;
      fa.barricade = off_barr;

      heap_sz = (size_t)(hp - heap);
      fa.cbHeap = (word)heap_sz;

      memset(&mfi, 0, sizeof(mfi));
      strncpy(mfi.name, name ? name : "", sizeof(mfi.name) - 1);
      mfi.name_hash = SquishHash((byte *)(void *)(name ? name : ""));
      mfi.ofs = (dword)tell(dat_fd);

      if (write(idx_fd, (char *)&mfi, sizeof(mfi)) != sizeof(mfi))
        goto fail;
      if (write(dat_fd, (char *)&fa, sizeof(fa)) != sizeof(fa))
        goto fail;
      if (write(dat_fd, heap, heap_sz) != (signed)heap_sz)
        goto fail;

      last_rec_size = (long)sizeof(fa) + (long)heap_sz;
      cbLast = last_rec_size;
    }
  }

  if (last_rec_size > 0)
  {
    FAREA fa;
    long end_pos = lseek(dat_fd, 0L, SEEK_END);
    long pos = end_pos - last_rec_size - ADATA_START;

    lseek(dat_fd, ADATA_START, SEEK_SET);
    if (read(dat_fd, (char *)&fa, sizeof(fa)) != sizeof(fa))
      goto fail;

    fa.cbPrior = -pos;
    lseek(dat_fd, ADATA_START, SEEK_SET);
    if (write(dat_fd, (char *)&fa, sizeof(fa)) != sizeof(fa))
      goto fail;
  }

  close(dat_fd);
  close(idx_fd);
  return 0;

fail:
  if (dat_fd != -1)
    close(dat_fd);
  if (idx_fd != -1)
    close(idx_fd);
  return -1;
}

static int near build_area_dats_from_toml(char *out_marea_base, size_t out_marea_base_sz,
                                         char *out_farea_base, size_t out_farea_base_sz)
{
  MaxCfgVar v;
  const char *tmp;
  char base_dir[PATHLEN];

  if (debuglog)
    debug_log("build_area_dats_from_toml: entry ng_cfg=%p", (void *)ng_cfg);

  if (out_marea_base == NULL || out_farea_base == NULL)
    return -1;
  if (out_marea_base_sz == 0 || out_farea_base_sz == 0)
    return -1;

  out_marea_base[0] = '\0';
  out_farea_base[0] = '\0';

  if (ng_cfg == NULL)
    return -1;

  memset(&v, 0, sizeof(v));
  if (maxcfg_toml_get(ng_cfg, "areas.msg.area", &v) != MAXCFG_OK || v.type != MAXCFG_VAR_TABLE_ARRAY)
  {
    if (debuglog)
      debug_log("build_area_dats_from_toml: missing/invalid areas.msg.area (type=%d)", (int)v.type);
    return -1;
  }

  memset(&v, 0, sizeof(v));
  if (maxcfg_toml_get(ng_cfg, "areas.file.area", &v) != MAXCFG_OK || v.type != MAXCFG_VAR_TABLE_ARRAY)
  {
    if (debuglog)
      debug_log("build_area_dats_from_toml: missing/invalid areas.file.area (type=%d)", (int)v.type);
    return -1;
  }

  tmp = ngcfg_get_path("maximus.temp_path");
  if (tmp == NULL)
    tmp = "";

  strnncpy(base_dir, (char *)tmp, sizeof(base_dir));
  ensure_trailing_delim(base_dir);

  snprintf(out_marea_base, out_marea_base_sz, "%sng_marea", base_dir);
  snprintf(out_farea_base, out_farea_base_sz, "%sng_farea", base_dir);

  if (debuglog)
    debug_log("build_area_dats_from_toml: temp_path='%s' base_dir='%s' marea_base='%s' farea_base='%s'",
              tmp, base_dir, out_marea_base, out_farea_base);

  if (build_marea_from_toml(out_marea_base) != 0)
    return -1;
  if (build_farea_from_toml(out_farea_base) != 0)
    return -1;

  return 0;
}







static void near install_handlers(void)
{
  /* Initialize the null-pointer check module */
  
  nullptrcheck();  
}



static void near StartUpVideo(void)
{
  VidOpen(ngcfg_get_has_snow(), multitasker==MULTITASKER_desqview,
          FALSE);

  if (!no_video)
    VidCls(CGREY);

  /* Turn on BIOS writes, but use same functions as the direct writes */

  if (displaymode==VIDEO_BIOS)
  {
#ifdef __MSDOS__
    VidBios(TRUE);
#endif
    displaymode=VIDEO_IBM;
  }

  WinApiOpen(FALSE);

  if ((win=WinOpen(0,
                   0,
                   VidNumRows()-((!local && ngcfg_get_bool("maximus.status_line")) ? 1 : 0),
                   VidNumCols(),
                   BORDER_NONE,
                   CGRAY,
                   CGRAY,
                   0))==NULL)
  {
    logit(mem_none);
    Local_Beep(3);
    maximus_exit(ERROR_CRITICAL);
  }

  local_putc=(void (_stdc *)(int))DoWinPutc;
  local_puts=(void (_stdc *)(char *))DoWinPuts;
}


void Load_Archivers(void)
{
  static int arcs_loaded=FALSE;
  
  if (!arcs_loaded)
  {
    #ifdef ORACLE
    ari=NULL;
    #else
    ari=Parse_Arc_Control_File(ngcfg_get_path("general.reader.archivers_ctl"));
    #endif

    arcs_loaded=TRUE;
  }
}



void Local_Beep(int n)
{
  int n_beep;

  for (n_beep=n; n_beep--; )
  {
    #ifdef OS_2
      DosBeep(300, 250);
      DosSleep(100);
    #elif UNIX
      beep();
      sleep(1);
    #else
      fputc('\a', stdout);
    
      /* wc buffers stdout */
      fflush(stdout);
    #endif
  }
}


