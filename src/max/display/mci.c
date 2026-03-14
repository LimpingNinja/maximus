/*
 * mci.c — MCI expand/parse engine
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

#define MAX_LANG_global
 #include <ctype.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 
 #include "libmaxcfg.h"
 #include "prog.h"
 #include "mm.h"
 #include "max_area.h"
 #include "mci.h"
 #include "theme.h"

/**
 * @brief Global MCI parse flags.
 */
unsigned long g_mci_parse_flags = MCI_PARSE_PIPE_COLORS | MCI_PARSE_MCI_CODES | MCI_PARSE_FORMAT_OPS;

/** @brief Active positional parameter bindings for |!N expansion (NULL = no params). */
MciLangParams *g_lang_params = NULL;

/** @brief Active theme color table (set at startup from colors.toml).
 *  Typed as void* in mci.h; actually a MaxCfgThemeColors*. */
void *g_mci_theme = NULL;

enum { MCI_FLAG_STACK_MAX = 16 };

static unsigned long g_flag_stack[MCI_FLAG_STACK_MAX];
static int g_flag_sp = 0;

enum
{
  MCI_FMT_NONE = 0,
  MCI_FMT_LEFTPAD,
  MCI_FMT_RIGHTPAD,
  MCI_FMT_CENTER
};

/**
 * @brief Append a single character to the MCI output buffer.
 *
 * @param out      Output buffer.
 * @param out_size Total capacity of output buffer.
 * @param io_len   Pointer to current length (updated on append).
 * @param ch       Character to append.
 */
static void mci_out_append_ch(char *out, size_t out_size, size_t *io_len, char ch)
{
  if (out_size==0)
    return;

  if (*io_len + 1 >= out_size)
  {
    out[out_size-1]='\0';
    return;
  }

  out[(*io_len)++]=ch;
  out[*io_len]='\0';
}

/**
 * @brief Append a NUL-terminated string to the MCI output buffer.
 *
 * @param out      Output buffer.
 * @param out_size Total capacity of output buffer.
 * @param io_len   Pointer to current length (updated on append).
 * @param s        String to append.
 */
static void mci_out_append_str(char *out, size_t out_size, size_t *io_len, const char *s)
{
  if (out_size==0)
    return;

  while (*s)
  {
    if (*io_len + 1 >= out_size)
    {
      out[out_size-1]='\0';
      return;
    }

    out[(*io_len)++]=*s++;
  }

  out[*io_len]='\0';
}

/**
 * @brief Calculate the visible (non-color-code) length of an MCI string.
 *
 * Skips pipe color codes, AVATAR attribute sequences, MCI codes,
 * and cursor control sequences to count only printable characters.
 *
 * @param s Input string potentially containing MCI/pipe sequences.
 * @return  Visible character count.
 */
static int mci_visible_len(const char *s)
{
  int count=0;

  while (*s)
  {
    if (*s=='\x16')
    {
      if (s[1] && s[2])
        s += 3;
      else
        break;
      continue;
    }

    if (*s=='|' && isdigit((unsigned char)s[1]) && isdigit((unsigned char)s[2]))
    {
      int code=((int)(s[1]-'0') * 10) + (int)(s[2]-'0');
      if (code >= 0 && code <= 31)
      {
        s += 3;
        continue;
      }
    }

    if (*s=='|' && s[1]=='|')
    {
      ++count;
      s += 2;
      continue;
    }

    /* |DF{path} — display file embedding, zero visible width */
    if (*s=='|' && s[1]=='D' && s[2]=='F' && s[3]=='{')
    {
      const char *end = strchr(s + 4, '}');
      if (end) { s = end + 1; continue; }
    }

    /* |{string} — delimiters are invisible, content is visible */
    if (*s=='|' && s[1]=='{')
    {
      const char *end = strchr(s + 2, '}');
      if (end)
      {
        const char *p = s + 2;
        while (p < end) { ++count; ++p; }
        s = end + 1;
        continue;
      }
    }

    if (*s=='|' && s[1]=='U' && s[2]=='#')
    {
      s += 3;
      continue;
    }

    if (*s=='|' && s[1] >= 'A' && s[1] <= 'Z' && s[2] >= 'A' && s[2] <= 'Z')
    {
      s += 3;
      continue;
    }

    /* |!N and |#N positional parameter codes — zero visible width */
    if (*s=='|' && (s[1]=='!' || s[1]=='#') &&
        ((s[2] >= '1' && s[2] <= '9') || (s[2] >= 'A' && s[2] <= 'F')))
    {
      s += 3;
      continue;
    }

    /* MCI cursor codes — zero visible width */
    if (*s=='|' && s[1]=='[')
    {
      char cc = s[2];
      /* Parameterless: |[0, |[1, |[K, |[<, |[>, |[H */
      if (cc=='0' || cc=='1' || cc=='K' || cc=='<' || cc=='>' || cc=='H')
      {
        s += 3;
        continue;
      }
      /* Parametric: |[A##, |[X|TW, etc. — 2-digit or |XY width */
      if (cc=='A' || cc=='B' || cc=='C' || cc=='D' ||
          cc=='L' || cc=='X' || cc=='Y')
      {
        if (isdigit((unsigned char)s[3]) && isdigit((unsigned char)s[4]))
        {
          s += 5;
          continue;
        }
        if (s[3]=='|' && s[4] && s[5] &&
            ((s[4]>='A' && s[4]<='Z' && s[5]>='A' && s[5]<='Z') ||
             (s[4]=='U' && s[5]=='#')))
        {
          s += 6; /* |[ + cc + |XY */
          continue;
        }
      }
    }

    ++count;
    ++s;
  }

  return count;
}

/**
 * @brief Truncate a string to a visible character length.
 *
 * Walks the string counting visible characters and NUL-terminates
 * at the point where trim_len visible chars have been emitted.
 *
 * @param s        String to trim in-place.
 * @param trim_len Maximum visible characters to keep.
 */
static void mci_apply_trim(char *s, int trim_len)
{
  if (trim_len < 0)
    return;

  int visible=0;
  char *p=s;

  while (*p)
  {
    if (*p=='\x16')
    {
      if (p[1] && p[2])
      {
        p += 3;
        continue;
      }
      break;
    }

    if (*p=='|' && isdigit((unsigned char)p[1]) && isdigit((unsigned char)p[2]))
    {
      int code=((int)(p[1]-'0') * 10) + (int)(p[2]-'0');
      if (code >= 0 && code <= 31)
      {
        p += 3;
        continue;
      }
    }

    if (*p=='|' && p[1]=='|')
    {
      if (visible >= trim_len)
      {
        *p='\0';
        return;
      }

      visible += 1;
      p += 2;
      continue;
    }

    if (visible >= trim_len)
    {
      *p='\0';
      return;
    }

    ++visible;
    ++p;
  }
}

/**
 * @brief Return a short string describing the user's terminal emulation mode.
 *
 * @return "TTY", "ANSI", "AVATAR", or "?".
 */
static const char *mci_term_emul_str(void)
{
  switch (usr.video)
  {
    case GRAPH_TTY:
      return "TTY";
    case GRAPH_ANSI:
      return "ANSI";
    case GRAPH_AVATAR:
      return "AVATAR";
  }

  return "?";
}

/**
 * @brief Check if two characters are both uppercase letters.
 *
 * @param a First character.
 * @param b Second character.
 * @return  Non-zero if both are A-Z.
 */
static int mci_is_upper2(char a, char b)
{
  return (a >= 'A' && a <= 'Z') && (b >= 'A' && b <= 'Z');
}

/**
 * @brief Parse a two-digit decimal number from a string.
 *
 * @param s   Pointer to first digit character.
 * @param out Receives the parsed value (0-99).
 * @return    1 on success, 0 if either character is not a digit.
 */
static int mci_parse_2dig(const char *s, int *out)
{
  if (!isdigit((unsigned char)s[0]) || !isdigit((unsigned char)s[1]))
    return 0;

  *out=((int)(s[0]-'0') * 10) + (int)(s[1]-'0');
  return 1;
}

/* Forward declaration — defined below, needed by mci_parse_width */
static void mci_expand_code(char a, char b, char *out, size_t out_size);

/**
 * @brief Parse a format-op width: either a literal 2-digit number or a |XY
 *        info code that expands to a 2-digit number.
 *
 * Tries the literal path first (2 chars consumed).  If the next chars are
 * a pipe followed by a recognised info code whose expansion is a 1- or
 * 2-digit number, that value is used instead (3 chars consumed for |XY).
 *
 * @param s        Pointer into the input string (right after the op letter).
 * @param out_val  Receives the parsed integer (0-99).
 * @param out_len  Receives the number of input characters consumed.
 * @return         1 on success, 0 on failure.
 */
static int mci_parse_width(const char *s, int *out_val, int *out_len)
{
  /* Fast path: literal two digits */
  if (isdigit((unsigned char)s[0]) && isdigit((unsigned char)s[1]))
  {
    *out_val = ((int)(s[0] - '0') * 10) + (int)(s[1] - '0');
    *out_len = 2;
    return 1;
  }

  /* Slow path: |XY info code that resolves to a number */
  if (s[0] == '|' && s[1] && s[2] &&
      (mci_is_upper2(s[1], s[2]) || (s[1] == 'U' && s[2] == '#')))
  {
    char val[256];
    mci_expand_code(s[1], s[2], val, sizeof(val));
    if (val[0] != '\0')
    {
      char *end = NULL;
      long v = strtol(val, &end, 10);
      if (end != val && *end == '\0' && v >= 0 && v <= 99)
      {
        *out_val = (int)v;
        *out_len = 3; /* consumed |XY */
        return 1;
      }
    }
  }

  return 0;
}

/**
 * @brief Emit a repeated character into the output buffer.
 *
 * @param out      Output buffer.
 * @param out_size Total capacity of output buffer.
 * @param out_len  Current output length.
 * @param count    Number of repetitions.
 * @param ch       Character to repeat.
 * @return         Updated output length.
 */
static size_t mci_emit_repeated(char *out, size_t out_size, size_t out_len, int count, char ch)
{
  for (int i=0; i < count; ++i)
  {
    if (out_size==0 || out_len + 1 >= out_size)
      break;
    out[out_len++]=ch;
  }

  if (out_size)
    out[(out_len < out_size) ? out_len : (out_size-1)]='\0';
  return out_len;
}

static void mci_expand_code(char a, char b, char *out, size_t out_size)
{
  if (out_size==0)
    return;

  out[0]='\0';

  if (a=='B' && b=='N')
    snprintf(out, out_size, "%s", ngcfg_get_string_raw("maximus.system_name"));
  else if (a=='S' && b=='N')
    snprintf(out, out_size, "%s", ngcfg_get_string_raw("maximus.sysop"));
  else if (a=='U' && b=='N')
    snprintf(out, out_size, "%s", usrname);
  else if (a=='U' && b=='H')
    snprintf(out, out_size, "%s", usr.alias);
  else if (a=='U' && b=='R')
    snprintf(out, out_size, "%s", usr.name);
  else if (a=='U' && b=='C')
    snprintf(out, out_size, "%s", usr.city);
  else if (a=='U' && b=='P')
    snprintf(out, out_size, "%s", usr.phone);
  else if (a=='U' && b=='D')
    snprintf(out, out_size, "%s", usr.dataphone);
  else if (a=='C' && b=='S')
    snprintf(out, out_size, "%lu", (unsigned long)usr.times);
  else if (a=='C' && b=='T')
    snprintf(out, out_size, "%lu", (unsigned long)usr.call);
  else if (a=='M' && b=='P')
    snprintf(out, out_size, "%lu", (unsigned long)usr.msgs_posted);
  else if (a=='D' && b=='K')
    snprintf(out, out_size, "%lu", (unsigned long)usr.down);
  else if (a=='F' && b=='K')
    snprintf(out, out_size, "%lu", (unsigned long)usr.up);
  else if (a=='D' && b=='L')
    snprintf(out, out_size, "%lu", (unsigned long)usr.ndown);
  else if (a=='F' && b=='U')
    snprintf(out, out_size, "%lu", (unsigned long)usr.nup);
  else if (a=='D' && b=='T')
    snprintf(out, out_size, "%ld", (long)usr.downtoday);
  else if (a=='T' && b=='L')
    snprintf(out, out_size, "%d", timeleft());
  else if (a=='U' && b=='S')
    snprintf(out, out_size, "%lu", (unsigned long)usr.len);
  else if (a=='T' && b=='E')
    snprintf(out, out_size, "%s", mci_term_emul_str());

  /* Terminal geometry codes — width clamped to 2 digits (max 99) */
  else if (a=='T' && b=='W')
    snprintf(out, out_size, "%02d", (int)(usr.width > 99 ? 99 : (usr.width ? usr.width : 80)));
  else if (a=='T' && b=='C')
    snprintf(out, out_size, "%02d", (int)(((usr.width ? usr.width : 80) + 1) / 2));
  else if (a=='T' && b=='H')
    snprintf(out, out_size, "%02d", (int)((usr.width ? usr.width : 80) / 2));

  /* Date/time codes */
  else if (a=='D' && b=='A')
  {
    time_t now = time(NULL);
    strftime(out, out_size, "%d %b %y", localtime(&now));
  }
  else if (a=='T' && b=='M')
  {
    time_t now = time(NULL);
    strftime(out, out_size, "%H:%M", localtime(&now));
  }
  else if (a=='T' && b=='S')
  {
    time_t now = time(NULL);
    strftime(out, out_size, "%H:%M:%S", localtime(&now));
  }

  /* User number (DB record id, set at login) */
  else if (a=='U' && b=='#')
    snprintf(out, out_size, "%ld", g_user_record_id);

  /* Message area codes */
  else if (a=='M' && b=='B')
    snprintf(out, out_size, "%s", mah.heap ? MAS(mah, name) : "");
  else if (a=='M' && b=='D')
    snprintf(out, out_size, "%s", mah.heap ? MAS(mah, descript) : "");

  /* File area codes */
  else if (a=='F' && b=='B')
    snprintf(out, out_size, "%s", fah.heap ? FAS(fah, name) : "");
  else if (a=='F' && b=='D')
    snprintf(out, out_size, "%s", fah.heap ? FAS(fah, descript) : "");

  /* Theme short_name: |TN */
  else if (a=='T' && b=='N')
  {
    const char *tsn = theme_get_current_shortname();
    snprintf(out, out_size, "%s", tsn ? tsn : "");
  }

  /* Positional parameter stub |!1..|!9, |!A..|!F (Round 1: no-op) */
  else if (a=='!' && ((b >= '1' && b <= '9') || (b >= 'A' && b <= 'F')))
    out[0]='\0'; /* Stub — substitution implemented in Round 2 */
}

/**
 * @brief Set the global MCI parse flags.
 *
 * @param flags New flags value (combination of MCI_PARSE_* constants).
 */
void MciSetParseFlags(unsigned long flags)
{
  g_mci_parse_flags = flags;
}

/**
 * @brief Get the current global MCI parse flags.
 *
 * @return Current flags value.
 */
unsigned long MciGetParseFlags(void)
{
  return g_mci_parse_flags;
}

/**
 * @brief Push modified parse flags onto the internal stack.
 *
 * Saves the current flags and applies the masked changes.
 *
 * @param mask   Bitmask of flags to change.
 * @param values New values for the masked flags.
 */
void MciPushParseFlags(unsigned long mask, unsigned long values)
{
  if (g_flag_sp >= MCI_FLAG_STACK_MAX)
    return;

  g_flag_stack[g_flag_sp++] = g_mci_parse_flags;
  g_mci_parse_flags = (g_mci_parse_flags & ~mask) | (values & mask);
}

/**
 * @brief Pop the most recently pushed parse flags from the stack.
 */
void MciPopParseFlags(void)
{
  if (g_flag_sp <= 0)
    return;

  g_mci_parse_flags = g_flag_stack[--g_flag_sp];
}

/**
 * @brief Expand MCI codes, pipe colors, and format operators in a string.
 *
 * @param in       Input string.
 * @param out      Output buffer.
 * @param out_size Size of output buffer in bytes.
 * @return         Number of bytes written (excluding NUL terminator).
 */
size_t MciExpand(const char *in, char *out, size_t out_size)
{
  if (out_size==0)
    return 0;

  out[0]='\0';

  int pending_pad_space=0;
  int pending_fmt=MCI_FMT_NONE;
  int pending_width=-1;
  char pending_padch=' ';
  int pending_trim=-1;

  size_t out_len=0;
  int cur_col=(int)current_col;

  for (size_t i=0; in[i] != '\0'; )
  {
    if (in[i]=='|' && in[i+1]=='|')
    {
      mci_out_append_ch(out, out_size, &out_len, '|');
      mci_out_append_ch(out, out_size, &out_len, '|');
      cur_col += 1;
      i += 2;
      continue;
    }

    if (in[i]=='$' && in[i+1]=='$')
    {
      mci_out_append_ch(out, out_size, &out_len, '$');
      cur_col += 1;
      i += 2;
      continue;
    }

    if ((g_mci_parse_flags & MCI_PARSE_FORMAT_OPS) && in[i]=='$')
    {
      char op=in[i+1];
      int n=0;
      int wlen=0; /* chars consumed by the width specifier (2 for ##, 3 for |XY) */
      if (op && mci_parse_width(in + i + 2, &n, &wlen))
      {
        /* base = $<op> (2) + width (wlen); fill char sits at i+2+wlen */
        char ch=in[i + 2 + wlen];

        if (op=='C' || op=='L' || op=='R' || op=='T')
        {
          if (op=='T')
            pending_trim=n;
          else
          {
            pending_width=n;
            pending_padch=' ';
            pending_fmt=(op=='C') ? MCI_FMT_CENTER : (op=='L' ? MCI_FMT_LEFTPAD : MCI_FMT_RIGHTPAD);
          }
          i += 2 + wlen;
          continue;
        }
        else if (op=='c' || op=='l' || op=='r')
        {
          if (ch=='\0')
            goto literal_dollar;

          pending_width=n;
          pending_padch=ch;
          pending_fmt=(op=='c') ? MCI_FMT_CENTER : (op=='l' ? MCI_FMT_LEFTPAD : MCI_FMT_RIGHTPAD);
          i += 2 + wlen + 1;
          continue;
        }
        else if (op=='D')
        {
          if (ch=='\0')
            goto literal_dollar;

          out_len=mci_emit_repeated(out, out_size, out_len, n, ch);
          cur_col += n;
          i += 2 + wlen + 1;
          continue;
        }
        else if (op=='X')
        {
          if (ch=='\0')
            goto literal_dollar;

          if (n > cur_col)
          {
            int count=n - cur_col;
            out_len=mci_emit_repeated(out, out_size, out_len, count, ch);
            cur_col += count;
          }

          i += 2 + wlen + 1;
          continue;
        }
      }

literal_dollar:
      mci_out_append_ch(out, out_size, &out_len, in[i]);
      if (in[i]=='\r' || in[i]=='\n')
        cur_col=1;
      else
        cur_col += 1;
      ++i;
      continue;
    }

    if ((g_mci_parse_flags & MCI_PARSE_FORMAT_OPS) && in[i]=='|' && in[i+1]=='P' && in[i+2]=='D')
    {
      pending_pad_space=1;
      i += 3;
      continue;
    }

    /* MCI cursor codes: |[0, |[1, |[K, |[A##, |[B##, |[C##, |[D##, |[L##, |[X##, |[Y## */
    if ((g_mci_parse_flags & MCI_PARSE_MCI_CODES) && in[i]=='|' && in[i+1]=='[')
    {
      char cc=in[i+2];

      /* |[0 — hide cursor */
      if (cc=='0')
      {
        mci_out_append_str(out, out_size, &out_len, "\x1b[?25l");
        i += 3;
        continue;
      }
      /* |[1 — show cursor */
      if (cc=='1')
      {
        mci_out_append_str(out, out_size, &out_len, "\x1b[?25h");
        i += 3;
        continue;
      }
      /* |[K — clear to end of line */
      if (cc=='K')
      {
        mci_out_append_str(out, out_size, &out_len, "\x1b[K");
        i += 3;
        continue;
      }

      /* |[< — move cursor to beginning of line (column 1) */
      if (cc=='<')
      {
        mci_out_append_str(out, out_size, &out_len, "\x1b[1G");
        cur_col=1;
        i += 3;
        continue;
      }
      /* |[> — move cursor to end of line (last column) */
      if (cc=='>')
      {
        int w = usr.width ? usr.width : 80;
        char csi[32];
        snprintf(csi, sizeof(csi), "\x1b[%dG", w);
        mci_out_append_str(out, out_size, &out_len, csi);
        cur_col=w;
        i += 3;
        continue;
      }
      /* |[H — move cursor to center column */
      if (cc=='H')
      {
        int w = usr.width ? usr.width : 80;
        int center = (w + 1) / 2;
        char csi[32];
        snprintf(csi, sizeof(csi), "\x1b[%dG", center);
        mci_out_append_str(out, out_size, &out_len, csi);
        cur_col=center;
        i += 3;
        continue;
      }

      /* |[A## through |[Y## — cursor movement with numeric parameter.
       * The ## may be a literal 2-digit number or a |XY info code. */
      int nn=0;
      int wlen=0;
      if ((cc=='A' || cc=='B' || cc=='C' || cc=='D' ||
           cc=='L' || cc=='X' || cc=='Y') &&
          mci_parse_width(in + i + 3, &nn, &wlen))
      {
        char csi[32];
        switch (cc)
        {
          case 'A': snprintf(csi, sizeof(csi), "\x1b[%dA", nn); break;            /* up */
          case 'B': snprintf(csi, sizeof(csi), "\x1b[%dB", nn); break;            /* down */
          case 'C': snprintf(csi, sizeof(csi), "\x1b[%dC", nn); break;            /* forward */
          case 'D': snprintf(csi, sizeof(csi), "\x1b[%dD", nn); break;            /* back */
          case 'X': snprintf(csi, sizeof(csi), "\x1b[%dG", nn); cur_col=nn; break; /* column absolute */
          case 'Y': snprintf(csi, sizeof(csi), "\x1b[%dd", nn);                    /* row absolute */
                    current_line=(byte)nn; display_line=(byte)nn;
                    break;
          case 'L': snprintf(csi, sizeof(csi), "\x1b[%dG\x1b[K", nn); cur_col=nn; break; /* move+erase */
          default: csi[0]='\0'; break;
        }
        mci_out_append_str(out, out_size, &out_len, csi);
        i += 3 + wlen;
        continue;
      }
    }

    /* |!N and |#N positional parameter expansion (|!1..|!9, |!A..|!F).
     * |!N is early-expanded by LangVsprintf before reaching here.
     * |#N is deferred — left intact by LangVsprintf so format ops
     * ($L, $R, $T, $C) can be applied to the resolved value here. */
    if ((g_mci_parse_flags & MCI_PARSE_MCI_CODES) && in[i]=='|' &&
        (in[i+1]=='!' || in[i+1]=='#') &&
        ((in[i+2] >= '1' && in[i+2] <= '9') || (in[i+2] >= 'A' && in[i+2] <= 'F')))
    {
      int idx;
      if (in[i+2] >= '1' && in[i+2] <= '9')
        idx = in[i+2] - '1';          /* |!1 → index 0 */
      else
        idx = in[i+2] - 'A' + 9;     /* |!A → index 9 */

      if (g_lang_params && idx < g_lang_params->count &&
          g_lang_params->values[idx])
      {
        char tmp[512];
        snprintf(tmp, sizeof(tmp), "%s", g_lang_params->values[idx]);

        if (pending_pad_space)
        {
          char with_pad[512];
          snprintf(with_pad, sizeof(with_pad), " %s", tmp);
          snprintf(tmp, sizeof(tmp), "%s", with_pad);
        }
        pending_pad_space=0;

        if (pending_trim >= 0)
        {
          mci_apply_trim(tmp, pending_trim);
          pending_trim=-1;
        }

        if (pending_fmt != MCI_FMT_NONE && pending_width >= 0)
        {
          int vlen=mci_visible_len(tmp);
          int pad=(pending_width > vlen) ? (pending_width - vlen) : 0;
          int left=0, right=0;

          if (pending_fmt==MCI_FMT_LEFTPAD)       left=pad;
          else if (pending_fmt==MCI_FMT_RIGHTPAD)  right=pad;
          else { left=pad/2; right=pad-left; }

          if (left)
          {
            out_len=mci_emit_repeated(out, out_size, out_len, left, pending_padch);
            cur_col += left;
          }

          mci_out_append_str(out, out_size, &out_len, tmp);
          cur_col += mci_visible_len(tmp);

          if (right)
          {
            out_len=mci_emit_repeated(out, out_size, out_len, right, pending_padch);
            cur_col += right;
          }

          pending_fmt=MCI_FMT_NONE;
          pending_width=-1;
          pending_padch=' ';
        }
        else
        {
          mci_out_append_str(out, out_size, &out_len, tmp);
          cur_col += mci_visible_len(tmp);
        }
      }
      /* If no params bound or index out of range, consume pending state and emit nothing */
      else
      {
        pending_trim=-1;
        pending_fmt=MCI_FMT_NONE;
        pending_width=-1;
        pending_pad_space=0;
      }

      i += 3;
      continue;
    }

    /* |&& — Cursor Position Report (DSR) → ESC[6n */
    if ((g_mci_parse_flags & MCI_PARSE_MCI_CODES) &&
        in[i]=='|' && in[i+1]=='&' && in[i+2]=='&')
    {
      mci_out_append_str(out, out_size, &out_len, "\x1b[6n");
      i += 3;
      continue;
    }

    /* |xx lowercase semantic theme color codes — expand to configured pipe string */
    if ((g_mci_parse_flags & MCI_PARSE_PIPE_COLORS) && g_mci_theme &&
        in[i]=='|' && in[i+1] >= 'a' && in[i+1] <= 'z' &&
        in[i+2] >= 'a' && in[i+2] <= 'z')
    {
      const char *expansion = maxcfg_theme_lookup((const MaxCfgThemeColors *)g_mci_theme, in[i+1], in[i+2]);
      if (expansion)
      {
        /* Emit the stored pipe code string (e.g. "|07" or "|15|17") */
        mci_out_append_str(out, out_size, &out_len, expansion);
        i += 3;
        continue;
      }
      /* Unknown slot — fall through to literal output */
    }

    /* |XY terminal control codes — emit ANSI/AVATAR sequences directly */
    if ((g_mci_parse_flags & MCI_PARSE_MCI_CODES) && in[i]=='|' &&
        in[i+1] >= 'A' && in[i+1] <= 'Z' &&
        in[i+2] >= 'A' && in[i+2] <= 'Z')
    {
      char a=in[i+1], b=in[i+2];
      const char *ctrl = NULL;

      if (a=='C' && b=='L')       ctrl = "\x0c";              /* CLS — display layer handles clear screen */
      else if (a=='B' && b=='S')  ctrl = "\x08 \x08";         /* destructive backspace */
      else if (a=='C' && b=='R')  ctrl = "\r\n";              /* carriage return + line feed */
      else if (a=='C' && b=='D')  ctrl = "\x1b[0m";           /* reset to default color */
      else if (a=='S' && b=='A')  ctrl = "\x1b""7";           /* DEC save cursor + attributes */
      else if (a=='R' && b=='A')  ctrl = "\x1b""8";           /* DEC restore cursor + attributes */
      else if (a=='S' && b=='S')  ctrl = "\x1b[?47h";         /* save screen (alt buffer) */
      else if (a=='R' && b=='S')  ctrl = "\x1b[?47l";         /* restore screen (main buffer) */
      else if (a=='L' && b=='C')  ctrl = "";                   /* load last color mode (stub) */
      else if (a=='L' && b=='F')  ctrl = "";                   /* load last font (stub) */

      if (ctrl)
      {
        mci_out_append_str(out, out_size, &out_len, ctrl);
        if (a=='C' && b=='L')
          cur_col = 1;  /* CLS resets column */
        else if (a=='C' && b=='R')
          cur_col = 1;  /* CRLF resets column */
        i += 3;
        continue;
      }
    }

    /* |{string} — inline string literal, usable as a format-op value.
     * Everything between '{' and '}' is the literal text.  If no closing
     * '}' is found, fall through to literal output. */
    if ((g_mci_parse_flags & MCI_PARSE_MCI_CODES) && in[i]=='|' && in[i+1]=='{')
    {
      /* Scan for the closing brace */
      const char *start = in + i + 2;
      const char *end   = strchr(start, '}');
      if (end)
      {
        size_t slen = (size_t)(end - start);
        char tmp[512];
        if (slen >= sizeof(tmp))
          slen = sizeof(tmp) - 1;
        memcpy(tmp, start, slen);
        tmp[slen] = '\0';

        if (pending_pad_space)
        {
          char with_pad[512];
          snprintf(with_pad, sizeof(with_pad), " %s", tmp);
          snprintf(tmp, sizeof(tmp), "%s", with_pad);
        }
        pending_pad_space=0;

        if (pending_trim >= 0)
        {
          mci_apply_trim(tmp, pending_trim);
          pending_trim=-1;
        }

        if (pending_fmt != MCI_FMT_NONE && pending_width >= 0)
        {
          int vlen=mci_visible_len(tmp);
          int pad=(pending_width > vlen) ? (pending_width - vlen) : 0;
          int left=0, right=0;

          if (pending_fmt==MCI_FMT_LEFTPAD)       left=pad;
          else if (pending_fmt==MCI_FMT_RIGHTPAD)  right=pad;
          else { left=pad/2; right=pad-left; }

          if (left)
          {
            out_len=mci_emit_repeated(out, out_size, out_len, left, pending_padch);
            cur_col += left;
          }

          mci_out_append_str(out, out_size, &out_len, tmp);
          cur_col += mci_visible_len(tmp);

          if (right)
          {
            out_len=mci_emit_repeated(out, out_size, out_len, right, pending_padch);
            cur_col += right;
          }

          pending_fmt=MCI_FMT_NONE;
          pending_width=-1;
          pending_padch=' ';
        }
        else
        {
          mci_out_append_str(out, out_size, &out_len, tmp);
          cur_col += mci_visible_len(tmp);
        }

        i += 2 + slen + 1; /* skip |{ ... } */
        continue;
      }
      /* No closing brace — fall through to literal output */
    }

    if ((g_mci_parse_flags & MCI_PARSE_MCI_CODES) && in[i]=='|' &&
        (mci_is_upper2(in[i+1], in[i+2]) || (in[i+1]=='U' && in[i+2]=='#')))
    {
      char a=in[i+1];
      char b=in[i+2];
      char val[256];
      char tmp[512];
      tmp[0]='\0';

      mci_expand_code(a, b, val, sizeof(val));

      if (val[0] != '\0')
      {
        snprintf(tmp, sizeof(tmp), "%s", val);

        if (pending_pad_space)
        {
          char with_pad[512];
          snprintf(with_pad, sizeof(with_pad), " %s", tmp);
          snprintf(tmp, sizeof(tmp), "%s", with_pad);
        }

        pending_pad_space=0;

        if (pending_trim >= 0)
        {
          mci_apply_trim(tmp, pending_trim);
          pending_trim=-1;
        }

        if (pending_fmt != MCI_FMT_NONE && pending_width >= 0)
        {
          int vlen=mci_visible_len(tmp);
          int pad=(pending_width > vlen) ? (pending_width - vlen) : 0;
          int left=0;
          int right=0;

          if (pending_fmt==MCI_FMT_LEFTPAD)
            left=pad;
          else if (pending_fmt==MCI_FMT_RIGHTPAD)
            right=pad;
          else
          {
            left=pad/2;
            right=pad-left;
          }

          if (left)
          {
            out_len=mci_emit_repeated(out, out_size, out_len, left, pending_padch);
            cur_col += left;
          }

          mci_out_append_str(out, out_size, &out_len, tmp);
          cur_col += mci_visible_len(tmp);

          if (right)
          {
            out_len=mci_emit_repeated(out, out_size, out_len, right, pending_padch);
            cur_col += right;
          }

          pending_fmt=MCI_FMT_NONE;
          pending_width=-1;
          pending_padch=' ';
        }
        else
        {
          mci_out_append_str(out, out_size, &out_len, tmp);
          cur_col += mci_visible_len(tmp);
        }

        i += 3;
        continue;
      }
    }

    mci_out_append_ch(out, out_size, &out_len, in[i]);
    if (in[i]=='\r' || in[i]=='\n')
      cur_col=1;
    else
      cur_col += 1;
    ++i;
  }

  return out_len;
}

/**
 * @brief Strip MCI-related sequences from a string.
 *
 * @param in          Input string.
 * @param out         Output buffer.
 * @param out_size    Size of output buffer in bytes.
 * @param strip_flags Bitmask of MCI_STRIP_COLORS, MCI_STRIP_INFO, MCI_STRIP_FORMAT.
 * @return            Number of bytes written (excluding NUL terminator).
 */
size_t MciStrip(const char *in, char *out, size_t out_size, unsigned long strip_flags)
{
  if (out_size==0)
    return 0;

  out[0]='\0';
  size_t out_len=0;

  for (size_t i=0; in[i] != '\0'; )
  {
    if (in[i]=='|' && in[i+1]=='|')
    {
      mci_out_append_ch(out, out_size, &out_len, '|');
      i += 2;
      continue;
    }

    if (in[i]=='|' && isdigit((unsigned char)in[i+1]) && isdigit((unsigned char)in[i+2]))
    {
      int code=((int)(in[i+1]-'0') * 10) + (int)(in[i+2]-'0');
      if ((strip_flags & MCI_STRIP_COLORS) && code >= 0 && code <= 31)
      {
        i += 3;
        continue;
      }
    }

    if (in[i]=='|' && in[i+1]=='P' && in[i+2]=='D')
    {
      if (strip_flags & MCI_STRIP_FORMAT)
      {
        i += 3;
        continue;
      }
    }

    /* |DF{path} — strip entire display-file embedding */
    if ((strip_flags & MCI_STRIP_INFO) && in[i]=='|' &&
        in[i+1]=='D' && in[i+2]=='F' && in[i+3]=='{')
    {
      const char *end = strchr(in + i + 4, '}');
      if (end)
      {
        i += 4 + (size_t)(end - (in + i + 4)) + 1;
        continue;
      }
    }

    /* |{string} — strip delimiters, keep content */
    if ((strip_flags & MCI_STRIP_INFO) && in[i]=='|' && in[i+1]=='{')
    {
      const char *end = strchr(in + i + 2, '}');
      if (end)
      {
        const char *p = in + i + 2;
        while (p < end)
          mci_out_append_ch(out, out_size, &out_len, *p++);
        i += 2 + (size_t)(end - (in + i + 2)) + 1;
        continue;
      }
    }

    if (in[i]=='|' && (mci_is_upper2(in[i+1], in[i+2]) || (in[i+1]=='U' && in[i+2]=='#')))
    {
      if (strip_flags & MCI_STRIP_INFO)
      {
        i += 3;
        continue;
      }
    }

    /* Strip |&& (CPR) */
    if ((strip_flags & MCI_STRIP_INFO) && in[i]=='|' && in[i+1]=='&' && in[i+2]=='&')
    {
      i += 3;
      continue;
    }

    /* Strip |!N positional parameter codes */
    if ((strip_flags & MCI_STRIP_INFO) && in[i]=='|' && in[i+1]=='!' &&
        ((in[i+2] >= '1' && in[i+2] <= '9') || (in[i+2] >= 'A' && in[i+2] <= 'F')))
    {
      i += 3;
      continue;
    }

    /* Strip MCI cursor codes |[X##, |[Y##, |[K, |[0, |[1, |[<, |[>, |[H, etc. */
    if ((strip_flags & MCI_STRIP_INFO) && in[i]=='|' && in[i+1]=='[')
    {
      char cc=in[i+2];
      if (cc=='0' || cc=='1' || cc=='K' || cc=='<' || cc=='>' || cc=='H')
      {
        i += 3;
        continue;
      }
      int nn=0;
      int wlen=0;
      if ((cc=='A' || cc=='B' || cc=='C' || cc=='D' ||
           cc=='L' || cc=='X' || cc=='Y') &&
          mci_parse_width(in + i + 3, &nn, &wlen))
      {
        i += 3 + wlen;
        continue;
      }
    }

    if (in[i]=='$' && (strip_flags & MCI_STRIP_FORMAT))
    {
      char op=in[i+1];
      int n=0;
      if (op && mci_parse_2dig(in + i + 2, &n))
      {
        if (op=='C' || op=='L' || op=='R' || op=='T')
        {
          i += 4;
          continue;
        }

        if ((op=='c' || op=='l' || op=='r' || op=='D' || op=='X') && in[i+4] != '\0')
        {
          i += 5;
          continue;
        }
      }
    }

    mci_out_append_ch(out, out_size, &out_len, in[i]);
    ++i;
  }

  return out_len;
}

/**
 * @brief Convert an MCI pipe color string to a single DOS attribute byte.
 *
 * Walks the string looking for |## numeric color codes and |xx theme codes.
 * Each code modifies the running attribute:
 *   |00..|15 set foreground (low nibble)
 *   |16..|23 set background (bits 4-6)
 *   |24..|31 set background + blink (bit 7)
 *   |xx      resolve via g_mci_theme, then parse the expansion recursively
 *
 * Non-pipe characters are silently skipped.
 */
byte Mci2Attr(const char *mci, byte base)
{
  byte attr = base;
  const char *p;

  if (!mci)
    return attr;

  for (p = mci; *p; )
  {
    if (p[0] == '|' && p[1])
    {
      /* |## numeric color code */
      if (p[1] >= '0' && p[1] <= '9' && p[2] >= '0' && p[2] <= '9')
      {
        int code = (p[1] - '0') * 10 + (p[2] - '0');

        if (code <= 15)
        {
          /* Set foreground, preserve background + blink */
          attr = (byte)((attr & 0xf0) | (code & 0x0f));
        }
        else if (code <= 23)
        {
          /* Set background, preserve foreground + blink */
          attr = (byte)((attr & 0x8f) | (((code - 16) & 0x07) << 4));
        }
        else if (code <= 31)
        {
          /* Set background + blink, preserve foreground */
          attr = (byte)((attr & 0x0f) | (((code - 24) & 0x07) << 4) | 0x80);
        }
        /* codes 32+ ignored */

        p += 3;
        continue;
      }

      /* |xx lowercase theme code */
      if (g_mci_theme &&
          p[1] >= 'a' && p[1] <= 'z' &&
          p[2] >= 'a' && p[2] <= 'z')
      {
        const char *expansion = maxcfg_theme_lookup(
            (const MaxCfgThemeColors *)g_mci_theme, p[1], p[2]);
        if (expansion)
        {
          /* Recurse into the expansion string */
          attr = Mci2Attr(expansion, attr);
          p += 3;
          continue;
        }
      }

      /* Unknown pipe sequence — skip the pipe and move on */
      p++;
    }
    else
    {
      /* Non-pipe character — skip */
      p++;
    }
  }

  return attr;
}
