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
static char rcs_id[]="$Id: max_out.c,v 1.5 2004/01/28 06:38:10 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=Modem/local output routines
*/

#define MAX_LANG_global
#define MAX_LANG_m_area
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <stdarg.h>
#include "prog.h"
#include "mm.h"
#include "mci.h"

void MdmPipeFlush(void);
void LPipeFlush(void);

#if defined(NT) || defined(UNIX)
# include "ntcomm.h"
#else
# define COMMAPI_VER 0
#endif

int last_cc=-1;
char strng[20];
static char *szOutString = NULL;
static char *szMciString = NULL;
static size_t cbMciString = 0;

static int MciStringHasOps(const char *s)
{
  for (; *s; ++s)
  {
    if (*s=='$')
      return 1;

    if (*s=='|' && s[1]=='P' && s[2]=='D')
      return 1;

    if (*s=='|' && s[1] >= 'A' && s[1] <= 'Z' && s[2] >= 'A' && s[2] <= 'Z')
      return 1;

    if (*s=='|' && s[1]=='U' && s[2]=='#')
      return 1;

    /* |xx lowercase semantic theme color codes */
    if (*s=='|' && s[1] >= 'a' && s[1] <= 'z' && s[2] >= 'a' && s[2] <= 'z')
      return 1;

    /* |&& — Cursor Position Report */
    if (*s=='|' && s[1]=='&' && s[2]=='&')
      return 1;

    /* |!N positional parameter codes */
    if (*s=='|' && s[1]=='!' &&
        ((s[2] >= '1' && s[2] <= '9') || (s[2] >= 'A' && s[2] <= 'F')))
      return 1;

    /* MCI cursor codes: [0, [1, [K, [A##..[Y## */
    if (*s=='[')
    {
      char cc = s[1];
      if (cc=='0' || cc=='1' || cc=='K')
        return 1;
      if ((cc=='A' || cc=='B' || cc=='C' || cc=='D' ||
           cc=='L' || cc=='X' || cc=='Y') &&
          isdigit((unsigned char)s[2]) && isdigit((unsigned char)s[3]))
        return 1;
    }
  }

  return 0;
}

static const char *MciMaybeExpandString(const char *s)
{
  if (!(g_mci_parse_flags & (MCI_PARSE_MCI_CODES | MCI_PARSE_FORMAT_OPS)))
    return s;

  if (!MciStringHasOps(s))
    return s;

  size_t need=(strlen(s) * 4) + 512;

  if (need < 1024)
    need=1024;

  if (need > 1024 * 1024)
    need=1024 * 1024;

  if (need > cbMciString)
  {
    char *p=realloc(szMciString, need);
    if (!p)
      return s;

    szMciString=p;
    cbMciString=need;
  }

  MciExpand(s, szMciString, cbMciString);
  return szMciString;
}

void OutputAllocStr(void)
{
  if (szOutString==NULL && (szOutString=malloc(MAX_PRINTF))==NULL)
  {
    printf(printfstringtoolong,"P");
    quit(ERROR_CRITICAL);
  }
}

void OutputFreeStr(void)
{
  free(szOutString);
  szOutString = NULL;

  free(szMciString);
  szMciString = NULL;
  cbMciString = 0;
}

int _stdc Printf(char *format,...)   /* Sends AVATAR string to console/modem */
{
  va_list var_args;
  int x;

  va_start(var_args,format);
  x=vsprintf(szOutString, format, var_args);
  va_end(var_args);

  Puts(szOutString);
  return x;
}


/**
 * @brief Coerce a printf format string so every specifier becomes %s.
 *
 * This is used by LangPrintf's legacy fallback path.  After call-site
 * migration, all varargs are const char * strings, but the binary .ltf
 * fallback may still return format strings with %d/%c/%lu/etc.
 * Replacing them with %s makes vsprintf safe.
 *
 * @param src   Source format string.
 * @param dst   Destination buffer.
 * @param dstsz Size of destination buffer.
 */
static void langprintf_coerce_format(const char *src, char *dst, size_t dstsz)
{
  size_t di = 0;
  for (const char *p = src; *p && di + 1 < dstsz; p++)
  {
    if (p[0] == '%' && p[1] == '%')
    {
      /* Escaped %% — copy both */
      if (di + 2 < dstsz) { dst[di++] = '%'; dst[di++] = '%'; }
      p++;
      continue;
    }
    if (p[0] == '%')
    {
      /* Skip flags, width, precision, length modifiers */
      const char *spec = p + 1;
      while (*spec == '-' || *spec == '+' || *spec == ' ' ||
             *spec == '#' || *spec == '0')
        spec++;
      while (*spec >= '0' && *spec <= '9') spec++;
      if (*spec == '.') { spec++; while (*spec >= '0' && *spec <= '9') spec++; }
      while (*spec == 'h' || *spec == 'l' || *spec == 'L') spec++;
      if (*spec == 's' || *spec == 'd' || *spec == 'u' || *spec == 'c' ||
          *spec == 'x' || *spec == 'X' || *spec == 'o')
      {
        /* Replace entire specifier with %s */
        if (di + 2 < dstsz) { dst[di++] = '%'; dst[di++] = 's'; }
        p = spec; /* advance past the specifier */
        continue;
      }
    }
    dst[di++] = *p;
  }
  dst[di] = '\0';
}

/**
 * @brief Language-aware Printf with |!N positional parameter support.
 *
 * Two-pass architecture:
 *   Pass 1 — expand |!N positional parameters into an intermediate buffer.
 *            This resolves placeholders so that subsequent MCI format ops
 *            (e.g. $X|!1─ → $X80─) see literal values, not tokens.
 *   Pass 2 — feed the expanded buffer to Puts(), which triggers MCI
 *            format-op processing ($X, $D, $R, $L, $T, etc.), pipe
 *            colors, and AVATAR/ANSI output.
 *
 * Legacy fallback: if no |!N is found, coerces all %-specifiers to %s
 * (all varargs are const char * after call-site migration) and uses
 * vsprintf + Puts().
 *
 * @param format  Language string (may contain |!N codes or %specifiers).
 * @param ...     Pre-formatted const char * values, one per placeholder.
 * @return 0 (return value not meaningfully used).
 */
int _stdc LangPrintf(char *format,...)
{
  va_list var_args;
  va_start(var_args, format);

  /* Check if the format uses |!N or |#N positional parameters */
  int has_positional = 0;
  for (const char *p = format; *p; p++)
  {
    if (p[0] == '|' && (p[1] == '!' || p[1] == '#') &&
        ((p[2] >= '1' && p[2] <= '9') || (p[2] >= 'A' && p[2] <= 'F')))
    {
      has_positional = 1;
      break;
    }
  }

  if (has_positional)
  {
    /* Pass 1: LangVsprintf expands |!N (early) and passes |#N through.
     * Pass 2: Bind params to g_lang_params so MciExpand can resolve
     * deferred |#N codes with pending format ops applied. */
    char expanded[PATHLEN * 2];
    va_list ap_copy;
    va_copy(ap_copy, var_args);
    LangVsprintf(expanded, sizeof(expanded), format, var_args);

    /* Bind params for deferred |#N expansion in MciExpand */
    MciLangParams lp = {0};
    int max_slot = 0;
    for (const char *p = format; *p; p++)
    {
      if (p[0] == '|' && (p[1] == '!' || p[1] == '#') &&
          ((p[2] >= '1' && p[2] <= '9') || (p[2] >= 'A' && p[2] <= 'F')))
      {
        int slot = (p[2] >= '1' && p[2] <= '9')
                     ? p[2] - '1' + 1
                     : p[2] - 'A' + 10;
        if (slot > max_slot) max_slot = slot;
      }
    }
    lp.count = (max_slot > 15) ? 15 : max_slot;
    for (int i = 0; i < lp.count; i++)
      lp.values[i] = va_arg(ap_copy, const char *);
    va_end(ap_copy);

    g_lang_params = &lp;
    Puts(expanded);
    g_lang_params = NULL;
  }
  else
  {
    /* Legacy fallback: coerce all specifiers to %s since all args are
     * const char * strings after call-site migration. */
    char safe_fmt[PATHLEN];
    langprintf_coerce_format(format, safe_fmt, sizeof(safe_fmt));
    vsprintf(szOutString, safe_fmt, var_args);
    Puts(szOutString);
  }

  va_end(var_args);
  return 0;
}


/**
 * @brief Expand |!N positional parameters into a buffer (no MCI output).
 *
 * Thin wrapper around LangVsprintf.  Supports type suffixes (d/l/u).
 *
 * @param buf    Destination buffer.
 * @param bufsz  Size of destination buffer.
 * @param format Language string containing |!N placeholders.
 * @param ...    Values, one per placeholder (types per suffix).
 * @return Number of characters written (excluding NUL).
 */
int LangSprintf(char *buf, size_t bufsz, const char *format, ...)
{
  va_list var_args;
  va_start(var_args, format);
  int ret = LangVsprintf(buf, bufsz, format, var_args);
  va_end(var_args);
  return ret;
}


/**
 * @brief Expand |!N positional parameters into a buffer (va_list variant).
 *
 * Core expansion engine used by LangSprintf, LangPrintf, and logit().
 * All varargs are const char * — callers pre-format any numeric values
 * before passing them.
 *
 * Format tokens:
 *   |!1..|!9  — positional parameter 1–9  (early expansion)
 *   |!A..|!F  — positional parameter 10–15 (early expansion)
 *   |#1..|#9  — deferred parameter 1–9  (passed through for MciExpand)
 *   |#A..|#F  — deferred parameter 10–15 (passed through for MciExpand)
 *
 * Both |!N and |#N consume the same vararg slot.  |#N codes are left
 * intact in the output so MciExpand can apply pending format ops ($L,
 * $R, $T, $C) to the resolved value.
 *
 * @param buf    Destination buffer.
 * @param bufsz  Size of destination buffer.
 * @param format Language string containing |!N placeholders.
 * @param ap     va_list of const char * values, one per slot.
 * @return Number of characters written (excluding NUL).
 */
int LangVsprintf(char *buf, size_t bufsz, const char *format, va_list ap)
{
  const char *vals[15] = {0};

  /* Pass 1: scan for highest slot index (both |!N and |#N) */
  int max_slot = 0;
  for (const char *p = format; *p; p++)
  {
    if (p[0] == '|' && (p[1] == '!' || p[1] == '#') &&
        ((p[2] >= '1' && p[2] <= '9') || (p[2] >= 'A' && p[2] <= 'F')))
    {
      int slot = (p[2] >= '1' && p[2] <= '9')
                   ? p[2] - '1' + 1
                   : p[2] - 'A' + 10;
      if (slot > max_slot)
        max_slot = slot;
    }
  }

  /* Pass 2: bind varargs into slots (all const char *) */
  int count = (max_slot > 15) ? 15 : max_slot;
  for (int i = 0; i < count; i++)
    vals[i] = va_arg(ap, const char *);

  /* Pass 3: walk format, expanding |!N inline, passing |#N through */
  size_t di = 0;
  for (const char *p = format; *p && di + 1 < bufsz; p++)
  {
    if (p[0] == '|' && p[1] == '!' &&
        ((p[2] >= '1' && p[2] <= '9') || (p[2] >= 'A' && p[2] <= 'F')))
    {
      /* |!N — early expansion: substitute param value inline */
      int slot = (p[2] >= '1' && p[2] <= '9')
                   ? p[2] - '1'
                   : p[2] - 'A' + 9;
      p += 2; /* skip past |!N */
      if (slot < count && vals[slot])
      {
        for (const char *v = vals[slot]; *v && di + 1 < bufsz; v++)
          buf[di++] = *v;
      }
    }
    else if (p[0] == '|' && p[1] == '#' &&
             ((p[2] >= '1' && p[2] <= '9') || (p[2] >= 'A' && p[2] <= 'F')))
    {
      /* |#N — deferred expansion: copy through for MciExpand */
      if (di + 3 < bufsz) { buf[di++] = p[0]; buf[di++] = p[1]; buf[di++] = p[2]; }
      p += 2;
    }
    else
    {
      buf[di++] = *p;
    }
  }
  buf[di] = '\0';
  return (int)di;
}


/* Sends AVATAR string to local console */

int _stdc Lprintf(char *format,...)   
{
  va_list var_args;
  int x;

  char string[MAX_PRINTF];

  if (strlen(format) >= MAX_PRINTF)
  {
    printf(printfstringtoolong,"Lp");
    return -1;
  }

  va_start(var_args,format);

  /* Detect |!N positional parameters from TOML lang strings.
   * When found, expand via LangVsprintf (all args are const char *).
   * Otherwise, fall through to legacy vsprintf for hardcoded formats. */
  {
    int has_positional = 0;
    for (const char *p = format; *p; p++)
    {
      if (p[0] == '|' && p[1] == '!' &&
          ((p[2] >= '1' && p[2] <= '9') || (p[2] >= 'A' && p[2] <= 'F')))
      {
        has_positional = 1;
        break;
      }
    }

    if (has_positional)
      x = LangVsprintf(string, sizeof(string), format, var_args);
    else
      x = vsprintf(string, format, var_args);
  }

  va_end(var_args);

  Lputs(string);
  return x;
}


/* Displays AVATAR string to modem */

int _stdc Mdm_printf(char *format,...)   
{
  va_list var_args;
  int x;

  char string[MAX_PRINTF];

  if (strlen(format) >= MAX_PRINTF)
  {
    printf(printfstringtoolong,"Mdm_p");
    return -1;
  }

  va_start(var_args,format);
  x=vsprintf(string,format,var_args);
  va_end(var_args);

  Mdm_puts(string);
  return x;
}


void Putc(int ch)
{
  if (!no_remote_output)
    Mdm_putc(ch);

  if ((snoop || local) && !no_local_output)
    Lputc(ch);
}

void Puts(char *s)
{
  const char *expanded=MciMaybeExpandString(s);

  if (!no_remote_output)
    Mdm_puts((char *)expanded);

  if ((snoop || local) && !no_local_output)
    Lputs((char *)expanded);
}

void Mdm_puts(char *s)
{
#if (COMMAPI_VER > 1)
  extern HCOMM hcModem;
  BOOL lastState = ComBurstMode(hcModem, TRUE);
#endif

  while (*s)
    Mdm_putc(*s++);

  MdmPipeFlush();

#if (COMMAPI_VER > 1)
  ComBurstMode(hcModem, lastState);
#endif
}


void _stdc DoWinPutc(int ch)
{
  WinPutc(win, (byte)ch);
}

void _stdc DoWinPuts(char *s)
{
  while (*s)
    WinPutc(win,*s++);
}


void vbuf_flush(void)
{
  if (no_video)
    return;

#ifdef TTYVIDEO
  if (displaymode==VIDEO_IBM)
#endif
    WinSync(win, in_wfc ? FALSE : TRUE);
#ifdef TTYVIDEO
  else fflush(stdout);
#endif
}



void Lputs(char *s)
{
  while (*s)
    Lputc(*s++);

  LPipeFlush();

  /* Otherwise, only flush video buffer if in file transfer protocol */

  if (in_file_xfer)
    vbuf_flush();

}
