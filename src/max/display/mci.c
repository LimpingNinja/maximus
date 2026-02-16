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

    /* |!N positional parameter codes — zero visible width */
    if (*s=='|' && s[1]=='!' &&
        ((s[2] >= '1' && s[2] <= '9') || (s[2] >= 'A' && s[2] <= 'F')))
    {
      s += 3;
      continue;
    }

    /* MCI cursor codes [X##, [Y##, etc. — zero visible width */
    if (*s=='[')
    {
      char cc = s[1];
      if (cc=='0' || cc=='1' || cc=='K')
      {
        s += 2;
        continue;
      }
      if ((cc=='A' || cc=='B' || cc=='C' || cc=='D' ||
           cc=='L' || cc=='X' || cc=='Y') &&
          isdigit((unsigned char)s[2]) && isdigit((unsigned char)s[3]))
      {
        s += 4;
        continue;
      }
    }

    ++count;
    ++s;
  }

  return count;
}

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

static int mci_is_upper2(char a, char b)
{
  return (a >= 'A' && a <= 'Z') && (b >= 'A' && b <= 'Z');
}

static int mci_parse_2dig(const char *s, int *out)
{
  if (!isdigit((unsigned char)s[0]) || !isdigit((unsigned char)s[1]))
    return 0;

  *out=((int)(s[0]-'0') * 10) + (int)(s[1]-'0');
  return 1;
}

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

  /* Positional parameter stub |!1..|!9, |!A..|!F (Round 1: no-op) */
  else if (a=='!' && ((b >= '1' && b <= '9') || (b >= 'A' && b <= 'F')))
    out[0]='\0'; /* Stub — substitution implemented in Round 2 */
}

void MciSetParseFlags(unsigned long flags)
{
  g_mci_parse_flags = flags;
}

unsigned long MciGetParseFlags(void)
{
  return g_mci_parse_flags;
}

void MciPushParseFlags(unsigned long mask, unsigned long values)
{
  if (g_flag_sp >= MCI_FLAG_STACK_MAX)
    return;

  g_flag_stack[g_flag_sp++] = g_mci_parse_flags;
  g_mci_parse_flags = (g_mci_parse_flags & ~mask) | (values & mask);
}

void MciPopParseFlags(void)
{
  if (g_flag_sp <= 0)
    return;

  g_mci_parse_flags = g_flag_stack[--g_flag_sp];
}

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
      if (op && mci_parse_2dig(in + i + 2, &n))
      {
        char ch=in[i+4];

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
          i += 4;
          continue;
        }
        else if (op=='c' || op=='l' || op=='r')
        {
          if (ch=='\0')
            goto literal_dollar;

          pending_width=n;
          pending_padch=ch;
          pending_fmt=(op=='c') ? MCI_FMT_CENTER : (op=='l' ? MCI_FMT_LEFTPAD : MCI_FMT_RIGHTPAD);
          i += 5;
          continue;
        }
        else if (op=='D')
        {
          if (ch=='\0')
            goto literal_dollar;

          out_len=mci_emit_repeated(out, out_size, out_len, n, ch);
          cur_col += n;
          i += 5;
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

          i += 5;
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

    /* MCI cursor codes: [0, [1, [K, [A##, [B##, [C##, [D##, [L##, [X##, [Y## */
    if ((g_mci_parse_flags & MCI_PARSE_MCI_CODES) && in[i]=='[')
    {
      char cc=in[i+1];

      /* [0 — hide cursor */
      if (cc=='0')
      {
        mci_out_append_str(out, out_size, &out_len, "\x1b[?25l");
        i += 2;
        continue;
      }
      /* [1 — show cursor */
      if (cc=='1')
      {
        mci_out_append_str(out, out_size, &out_len, "\x1b[?25h");
        i += 2;
        continue;
      }
      /* [K — clear to end of line */
      if (cc=='K')
      {
        mci_out_append_str(out, out_size, &out_len, "\x1b[K");
        i += 2;
        continue;
      }

      /* [A## through [Y## — cursor movement with 2-digit parameter */
      int nn=0;
      if ((cc=='A' || cc=='B' || cc=='C' || cc=='D' ||
           cc=='L' || cc=='X' || cc=='Y') &&
          mci_parse_2dig(in + i + 2, &nn))
      {
        char csi[32];
        switch (cc)
        {
          case 'A': snprintf(csi, sizeof(csi), "\x1b[%dA", nn); break; /* up */
          case 'B': snprintf(csi, sizeof(csi), "\x1b[%dB", nn); break; /* down */
          case 'C': snprintf(csi, sizeof(csi), "\x1b[%dC", nn); break; /* forward */
          case 'D': snprintf(csi, sizeof(csi), "\x1b[%dD", nn); break; /* back */
          case 'X': snprintf(csi, sizeof(csi), "\x1b[%dG", nn); cur_col=nn; break; /* column absolute */
          case 'Y': snprintf(csi, sizeof(csi), "\x1b[%dd", nn); break; /* row absolute */
          case 'L': snprintf(csi, sizeof(csi), "\x1b[%dG\x1b[K", nn); cur_col=nn; break; /* move+erase */
          default: csi[0]='\0'; break;
        }
        mci_out_append_str(out, out_size, &out_len, csi);
        i += 4;
        continue;
      }
    }

    /* |!N positional parameter expansion (|!1..|!9, |!A..|!F) */
    if ((g_mci_parse_flags & MCI_PARSE_MCI_CODES) && in[i]=='|' && in[i+1]=='!' &&
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
        mci_out_append_str(out, out_size, &out_len, g_lang_params->values[idx]);
      }
      /* If no params bound or index out of range, emit nothing (strip) */

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

    /* Strip MCI cursor codes */
    if ((strip_flags & MCI_STRIP_INFO) && in[i]=='[')
    {
      char cc=in[i+1];
      if (cc=='0' || cc=='1' || cc=='K')
      {
        i += 2;
        continue;
      }
      int nn=0;
      if ((cc=='A' || cc=='B' || cc=='C' || cc=='D' ||
           cc=='L' || cc=='X' || cc=='Y') &&
          mci_parse_2dig(in + i + 2, &nn))
      {
        i += 4;
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
