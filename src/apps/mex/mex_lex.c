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
static char rcs_id[]="$Id: mex_lex.c,v 1.4 2004/01/27 20:57:25 paltas Exp $";
#pragma on(unreferenced)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "prog.h"
#include "mex.h"
#include "mex_lexp.h"

extern YYSTYPE yylval;
static MACRO macro={0};
static MACDEF macList=NULL;
static FILESTACK fstk[MAX_INCLUDE];
static int iStack=-1;
static XLTABLE xlt[]=
{
  {'n', '\n'},
  {'r', '\r'},
  {'a', '\a'},
  {'b', '\b'},
  {'f', '\f'},
  {'v', '\v'},
  {'t', '\t'},
  {0,   0}
};

static byte ifstatus[MAX_IFBLKS];
static byte ifdone[MAX_IFBLKS];
static int iBlock=-1;

/* ------------------------------------------------------------------ */
/* UTF-8 to CP437 reverse lookup table.                               */
/*                                                                    */
/* Sorted by Unicode codepoint for binary search.  Covers all non-    */
/* ASCII CP437 characters (0x01-0x1F graphic glyphs, 0x7F, 0x80-0xFF).*/
/* ------------------------------------------------------------------ */

static struct _utf8_to_cp437
{
  unsigned int codepoint;  /* Unicode codepoint */
  unsigned char cp437;     /* Corresponding CP437 byte */
} utf8_cp437_map[] =
{
  /* Latin-1 Supplement (U+00A0-U+00FF) */
  {0x00A0, 0xFF}, {0x00A1, 0xAD}, {0x00A2, 0x9B}, {0x00A3, 0x9C},
  {0x00A5, 0x9D}, {0x00A7, 0x15}, {0x00AA, 0xA6}, {0x00AB, 0xAE},
  {0x00AC, 0xAA}, {0x00B0, 0xF8}, {0x00B1, 0xF1}, {0x00B2, 0xFD},
  {0x00B5, 0xE6}, {0x00B6, 0x14}, {0x00B7, 0xFA}, {0x00BA, 0xA7},
  {0x00BB, 0xAF}, {0x00BC, 0xAC}, {0x00BD, 0xAB}, {0x00BF, 0xA8},
  {0x00C4, 0x8E}, {0x00C5, 0x8F}, {0x00C6, 0x92}, {0x00C7, 0x80},
  {0x00C9, 0x90}, {0x00D1, 0xA5}, {0x00D6, 0x99}, {0x00DC, 0x9A},
  {0x00DF, 0xE1}, {0x00E0, 0x85}, {0x00E1, 0xA0}, {0x00E2, 0x83},
  {0x00E4, 0x84}, {0x00E5, 0x86}, {0x00E6, 0x91}, {0x00E7, 0x87},
  {0x00E8, 0x8A}, {0x00E9, 0x82}, {0x00EA, 0x88}, {0x00EB, 0x89},
  {0x00EC, 0x8D}, {0x00ED, 0xA1}, {0x00EE, 0x8C}, {0x00EF, 0x8B},
  {0x00F1, 0xA4}, {0x00F2, 0x95}, {0x00F3, 0xA2}, {0x00F4, 0x93},
  {0x00F6, 0x94}, {0x00F7, 0xF6}, {0x00F9, 0x97}, {0x00FA, 0xA3},
  {0x00FB, 0x96}, {0x00FC, 0x81}, {0x00FF, 0x98},
  /* Latin Extended-B */
  {0x0192, 0x9F},
  /* Greek */
  {0x0393, 0xE2}, {0x0398, 0xE9}, {0x03A3, 0xE4}, {0x03A6, 0xE8},
  {0x03A9, 0xEA}, {0x03B1, 0xE0}, {0x03B4, 0xEB}, {0x03B5, 0xEE},
  {0x03C0, 0xE3}, {0x03C3, 0xE5}, {0x03C4, 0xE7}, {0x03C6, 0xED},
  /* General Punctuation / Superscripts / Currency */
  {0x2022, 0x07}, {0x203C, 0x13}, {0x207F, 0xFC}, {0x20A7, 0x9E},
  /* Arrows */
  {0x2190, 0x1B}, {0x2191, 0x18}, {0x2192, 0x1A}, {0x2193, 0x19},
  {0x2194, 0x1D}, {0x2195, 0x12}, {0x21A8, 0x17},
  /* Mathematical Operators */
  {0x2219, 0xF9}, {0x221A, 0xFB}, {0x221E, 0xEC}, {0x221F, 0x1C},
  {0x2229, 0xEF}, {0x2248, 0xF7}, {0x2261, 0xF0}, {0x2264, 0xF3},
  {0x2265, 0xF2},
  /* Miscellaneous Technical */
  {0x2302, 0x7F}, {0x2310, 0xA9}, {0x2320, 0xF4}, {0x2321, 0xF5},
  /* Box Drawing */
  {0x2500, 0xC4}, {0x2502, 0xB3}, {0x250C, 0xDA}, {0x2510, 0xBF},
  {0x2514, 0xC0}, {0x2518, 0xD9}, {0x251C, 0xC3}, {0x2524, 0xB4},
  {0x252C, 0xC2}, {0x2534, 0xC1}, {0x253C, 0xC5}, {0x2550, 0xCD},
  {0x2551, 0xBA}, {0x2552, 0xD5}, {0x2553, 0xD6}, {0x2554, 0xC9},
  {0x2555, 0xB8}, {0x2556, 0xB7}, {0x2557, 0xBB}, {0x2558, 0xD4},
  {0x2559, 0xD3}, {0x255A, 0xC8}, {0x255B, 0xBE}, {0x255C, 0xBD},
  {0x255D, 0xBC}, {0x255E, 0xC6}, {0x255F, 0xC7}, {0x2560, 0xCC},
  {0x2561, 0xB5}, {0x2562, 0xB6}, {0x2563, 0xB9}, {0x2564, 0xD1},
  {0x2565, 0xD2}, {0x2566, 0xCB}, {0x2567, 0xCF}, {0x2568, 0xD0},
  {0x2569, 0xCA}, {0x256A, 0xD8}, {0x256B, 0xD7}, {0x256C, 0xCE},
  /* Block Elements */
  {0x2580, 0xDF}, {0x2584, 0xDC}, {0x2588, 0xDB}, {0x258C, 0xDD},
  {0x2590, 0xDE}, {0x2591, 0xB0}, {0x2592, 0xB1}, {0x2593, 0xB2},
  /* Geometric Shapes */
  {0x25A0, 0xFE}, {0x25AC, 0x16}, {0x25B2, 0x1E}, {0x25BA, 0x10},
  {0x25BC, 0x1F}, {0x25C4, 0x11}, {0x25CB, 0x09}, {0x25D8, 0x08},
  {0x25D9, 0x0A},
  /* Miscellaneous Symbols */
  {0x263A, 0x01}, {0x263B, 0x02}, {0x263C, 0x0F}, {0x2640, 0x0C},
  {0x2642, 0x0B}, {0x2660, 0x06}, {0x2663, 0x05}, {0x2665, 0x03},
  {0x2666, 0x04}, {0x266A, 0x0D}, {0x266B, 0x0E}
};

#define UTF8_CP437_MAP_SIZE \
  (sizeof(utf8_cp437_map) / sizeof(utf8_cp437_map[0]))

/**
 * @brief Binary search the utf8_cp437_map for a Unicode codepoint.
 * @param codepoint  Unicode codepoint to look up.
 * @return CP437 byte (0x00-0xFF) on match, or -1 if not found.
 */
static int near utf8_lookup_cp437(unsigned int codepoint)
{
  int lo = 0, hi = (int)UTF8_CP437_MAP_SIZE - 1;

  while (lo <= hi)
  {
    int mid = (lo + hi) / 2;
    unsigned int midval = utf8_cp437_map[mid].codepoint;

    if (codepoint == midval)
      return utf8_cp437_map[mid].cp437;
    else if (codepoint < midval)
      hi = mid - 1;
    else
      lo = mid + 1;
  }

  return -1;
}

/**
 * @brief Decode a UTF-8 lead byte + continuation bytes and map to CP437.
 *
 * Reads continuation bytes via pull_character().  On success, writes
 * the single CP437 byte to *out.  On failure (invalid sequence or no
 * CP437 mapping), the consumed continuation bytes are written to rawbuf
 * so the caller can emit them as-is.
 *
 * @param lead    First byte of the UTF-8 sequence (already pulled).
 * @param out     Receives the CP437 byte on success.
 * @param rawbuf  Receives consumed continuation bytes on failure (max 3).
 * @param rawlen  Receives number of continuation bytes stored in rawbuf.
 * @return 1 on success (CP437 byte in *out), 0 on failure.
 */
static int near utf8_to_cp437(int lead, unsigned char *out,
                               unsigned char *rawbuf, int *rawlen)
{
  unsigned int codepoint;
  int expect;  /* number of continuation bytes expected */
  int i, cp437;

  *rawlen = 0;

  if ((lead & 0xE0) == 0xC0)      { expect = 1; codepoint = lead & 0x1F; }
  else if ((lead & 0xF0) == 0xE0) { expect = 2; codepoint = lead & 0x0F; }
  else if ((lead & 0xF8) == 0xF0) { expect = 3; codepoint = lead & 0x07; }
  else return 0;  /* not a valid UTF-8 lead byte */

  /* Read continuation bytes — buffer them for fallback */
  for (i = 0; i < expect; i++)
  {
    int ch = peek_character();
    if (ch == EOF || (ch & 0xC0) != 0x80)
      return 0;  /* broken sequence; rawbuf has bytes consumed so far */
    pull_character();
    rawbuf[(*rawlen)++] = (unsigned char)ch;
    codepoint = (codepoint << 6) | (ch & 0x3F);
  }

  cp437 = utf8_lookup_cp437(codepoint);
  if (cp437 >= 0)
  {
    *out = (unsigned char)cp437;
    return 1;
  }

  /* Valid UTF-8 but no CP437 mapping — warn, caller emits raw bytes */
  warn(MEXERR_WARN_UTF8UNMAPPED, codepoint);
  return 0;
}

/* Push a file onto the current file stack */

int push_fstk(char *fname)
{
  int i;

  if (iStack >= MAX_INCLUDE-1)
  {
    error(MEXERR_TOOMANYNESTEDINCL);
    return FALSE;
  }

  i=iStack+1;

  if ((fstk[i].fp=fopen(fname, "r"))==NULL)
  {
    error(MEXERR_CANTOPENREAD, fname);
    return FALSE;
  }

  /* Avoid overlapping strcpy when fname already points at global filename */
  if (fname != filename)
    strcpy(filename, fname);
  fstk[++iStack].name=strdup(fname);
  fstk[iStack].save_linenum=linenum;
  linenum=1;

  return TRUE;
}



/* Close off the file on the top of the stack */

int pop_fstk(void)
{
  if (iStack < 0)
    return FALSE;

  fclose(fstk[iStack].fp);

  if (fstk[iStack].name)
  {
    free(fstk[iStack].name);
    fstk[iStack].name=NULL;
  }

  /* Go back to the old line number */

  linenum=fstk[iStack].save_linenum;

  /* Change back to our original filename as well */

  if (--iStack >= 0)
    strcpy(filename, fstk[iStack].name);

  return TRUE;
}



/* Retrieve a character from the current file source -- whether that be     *
 * the main file, an include file, or a macro definition.                   */

int pull_character(void)
{
  int ch = EOF;

  if (macro.fInMacro)
  {
    if (macro.iPos >= macro.md->iLength-1)
      macro.fInMacro=FALSE;

#ifdef DEBUGLEX
    printf("%c*", macro.md->szText[macro.iPos]);
#endif
    return macro.md->szText[macro.iPos++];
  }

  /* Get characters from the current stream */

  while (iStack >= 0 && (ch=fgetc(fstk[iStack].fp))==EOF)
    pop_fstk();

#ifdef DEBUGLEX
  printf("%c%d", ch, iStack);
#endif
  fflush(stdout);

  if (iStack >= 0)
    return ch;
  else
    return EOF;
}




/* Peek at a character from the current file source */

int peek_character(void)
{
  int ch, i;

  if (macro.fInMacro)
    return macro.md->szText[macro.iPos];

  i=iStack;

  /* Don't return an EOF unless we are at the end of ALL our include files */

  while (i >= 0 && (ch=fgetc(fstk[i].fp))==EOF)
    ungetc(EOF, fstk[i--].fp);

  if (i < 0)
    return EOF;

  ungetc(ch, fstk[i].fp);
  return ch;
}

/* Processing after we just read in a ":" symbol */

static int near process_colon(void)
{
  int c;

  if ((c=peek_character())=='=')
  {
    pull_character();
    return T_ASSIGN;
  }

  return T_COLON;
}


/* Processing after we just read in a ">" symbol: */

static int near process_morethan(void)
{
  int c;

  if ((c=peek_character())=='=')
  {
    pull_character();
    return T_GE;
  }

  return T_GT;
}


/* Processing after we just read in a "<" symbol */

static int near process_lessthan(void)
{
  int c;

  if ((c=peek_character())=='=')
  {
    pull_character();
    return T_LE;
  }

  if (c=='>')
  {
    pull_character();
    return T_NOTEQUAL;
  }

  return T_LT;
}


/* Process "/" or a double-slash comment */

static int near process_slash(void)
{
  int c;

  c=peek_character();

  if (c != '/')   /* if it's not a second '/', it must be divison */
    return TRUE;

  /* else it then must be a comment, so skip to EOL or EOF. */

  while ((c=peek_character()) != '\n' && c != EOF)
    pull_character();

  return FALSE;
}


/* Function to process numeric constants */

static int near process_digit(int c)
{
  word type=TYPE_DEC;
  int fLong = FALSE;
  int fUnsigned = FALSE;
  char *scan;
  char *p, *e;


  /* Set up pointers to beginning of identifier buffer */

  p=yytext;
  e=yytext+MAX_ID_LEN-1;


  /* Copy in the character we just found */

  *p++=(char)c;


  /* As long as we have space left in the buffer, get a character *
   * from the input file, and ensure that it's a digit.           */

  while (p < e && (c=peek_character()) != EOF &&
         (isxdigit(c) || tolower(c)=='x'))
  {
    pull_character();
    *p++=(char)c;
  }

  *p='\0';

  /* If we reached the end of the buffer, the constant was        *
   * too long                                                     */

  if (p==e)
    warn(MEXERR_WARN_CONSTANTTRUNCATED, yytext);

  /* Now check for trailing "u", "l", or "ul" specifiers to modify
   * the type of this constant.
   */

  do
  {
    c = peek_character();
    c = tolower(c);

    if (c=='l')
    {
      fLong = TRUE;
      pull_character();
    }

    /* No signed/unsigned support yet, but this paves the way for the
     * future.
     */

    if (c=='u')
    {
      fUnsigned = TRUE;
      pull_character();
    }
  }
  while (c=='l' || c=='u');

  scan=yytext;

  /* Numbers beginning with "0x" should be treated as hex */

  if (yytext[0]=='0' && tolower(yytext[1])=='x')
  {
    type=TYPE_HEX;
    scan += 2;
  }

  if (type==TYPE_DEC)
    yylval.constant.dwval=atol(scan);
  else
    sscanf(scan, "%" UINT32_XFORMAT, &yylval.constant.dwval);

  /* If the number will fit in a single integer, return it
   * as a word (assuming that we did not explicitly ask otherwise).
   * Otherwise, return it as a dword.
   */

  if (yylval.constant.dwval < 65536L && !fLong)
  {
    yylval.constant.val=(word)yylval.constant.dwval;
    return T_CONSTWORD;
  }

  return T_CONSTDWORD;
}


/* Process a single character constant */

static int near process_character_constant(void)
{
  int c;

  /* a character constant */

  if ((c=pull_character())==EOF || c=='\n' || c=='\r')
  {
    error(MEXERR_UNTERMCHARCONST);
    if (c=='\n')
      linenum++;
    return FALSE;
  }

  /* Check for escape characters */

  if (c=='\\')
  {
    struct _xlt *x;

    if ((c=pull_character())==EOF)
      return FALSE;

    for (x=xlt; x->from; x++)
      if (c==x->from)
      {
        c=x->to;
        break;
      }
  }
  else if (utf8_convert && (c & 0xC0) == 0xC0 && c >= 0xC2)
  {
    /* UTF-8 lead byte in a char constant — decode and map to CP437 */
    unsigned char cp437_byte;
    unsigned char rawbuf[3];
    int rawlen;

    if (utf8_to_cp437(c, &cp437_byte, rawbuf, &rawlen))
      c = cp437_byte;
    else
    {
      error(MEXERR_INVALCHARCONST);
      return FALSE;
    }
  }

  yylval.constant.val=(word)c;

  /* Make sure that it ends with the appropriate punctuation */

  if (pull_character() != '\'')
  {
    error(MEXERR_INVALCHARCONST);
    return FALSE;
  }

  return TRUE;
}




/* Process a constant string */

static int near process_string(void)
{
  char *str, *sptr, *olds;
  int slen, c;

  /* Allocate memory on the heap to hold the new string */

  str=sptr=smalloc(slen=STR_BLOCK);

  /* Now scan 'till the end of the string is reached */

  while ((c=pull_character()) != EOF)
  {
    /* If string is larger than buffer (minus one for the NUL),   *
     * make the buffer a bit larger.                              */

    if (sptr-str >= slen-1)
    {
      olds=str;
      str=realloc(str, slen += STR_BLOCK);

      if (str==NULL)
        NoMem();

      sptr = str+(sptr-olds);
    }

    /* Check for the end-of-string quote */

    if (c=='"')
    {
      /* Skip over all whitespace and returns */

      while ((c=peek_character())==' ' || c=='\t' || c=='\n' || c=='\r')
      {
        if (pull_character()=='\n')
          linenum++;
      }

      /* If the next non-whitespace char is NOT a quote, then it's*
       * the end of the string.  If that's the case, exit loop.   */

      if (c=='"')
        pull_character();
      else
        break;
    }
    else if (c=='\\')  /* a backslash "escape character" */
    {
      struct _xlt *x;

      /* Now compare this character to those listed, and make     *
       * the appropriate translations.                            */

      if ((c=pull_character())==EOF)
        break;

      for (x=xlt; x->from; x++)
        if (c==x->from)
        {
          *sptr++=x->to;
          break;
        }

      /* If it wasn't found, just pass the second character thru  *
       * as a normal character.                                   */

      if (! x->from)
      {
        /* Handle "x34" hex-type codes */

        if (c != 'x')
          *sptr++=(char)c;
        else
        {
          char str[3];
          int hex;

          str[0]=pull_character();
          str[1]=pull_character();
          str[2]=0;

          if (sscanf(str, "%x", &hex) != 1)
            error(MEXERR_INVALIDHEX, str);
          else
            *sptr++=hex;
        }
      }
    }
    else /* 'c' is a normal character */
    {
      /* Try UTF-8 to CP437 conversion for multi-byte sequences */
      if (utf8_convert && (c & 0xC0) == 0xC0 && c >= 0xC2)
      {
        unsigned char cp437_byte;
        unsigned char rawbuf[3];
        int rawlen;

        if (utf8_to_cp437(c, &cp437_byte, rawbuf, &rawlen))
        {
          *sptr++ = (char)cp437_byte;
        }
        else
        {
          /* Conversion failed — emit lead byte + any consumed continuations */
          *sptr++ = (char)c;
          for (int j = 0; j < rawlen; j++)
            *sptr++ = (char)rawbuf[j];
        }
      }
      else
      {
        *sptr++=(char)c;
      }
    }
  }

  if (c==EOF)
    error(MEXERR_UNTERMSTRINGCONST);

  *sptr='\0';
  yylval.constant.lit=str;

  return T_CONSTSTRING;
}


/* Process an "#include" direective */

static void near process_include(char *name)
{
  static char semicolon[]=";";
  char *mex_include=getenv("MEX_INCLUDE");
  char fname[PATHLEN];
  char *tok, *p;
  char incbuf[PATHLEN];

  if (name==NULL)
  {
    error(MEXERR_INVALIDINCLUDEDIR);
    return;
  }

  tok=strtok(name, "<>\"");

  if (tok==NULL)
  {
    error(MEXERR_INVALIDINCLUDEFILE);
    return;
  }

  if (!fexist(tok) && mex_include && *mex_include)
  {
    /* Work on a copy of MEX_INCLUDE so we don't modify the env buffer */
    strncpy(incbuf, mex_include, sizeof(incbuf)-1);
    incbuf[sizeof(incbuf)-1] = '\0';

    p=strtok(incbuf, semicolon);

    while (p)
    {
      strcpy(fname, p);
      Add_Trailing(fname, PATH_DELIM);
      strcat(fname, tok);

      if (fexist(fname))
      {
        tok=fname;
        break;
      }

      p=strtok(NULL, semicolon);
    }
  }

  push_fstk(tok);
}


static MACDEF near find_macro(char *szName)
{
  register char ch=*szName;
  MACDEF md;

  for (md=macList; md; md=md->next)
    if (ch==*md->szName && eqstr(md->szName,szName))
      return md;
  return NULL;
}

static void near process_undef(char *line)
{
  MACDEF md;

  md=find_macro(line);

  /* It was there, so remove it */

  if (md!=NULL)
  {
    if (md==macList)
      macList=md->next;
    else
    {
      MACDEF dd;

      for (dd=macList;dd->next!=md; dd=dd->next)
        ;
      dd->next=md->next;
    }
    free(md->szName);
    free(md->szText);
    free(md);
  }
}

/* Handle a macro definition */

static void near process_define(char *line)
{
  MACDEF md;
  char *m, *p;

  if ((m=strtok(line, " \t\n"))==NULL)
  {
    error(MEXERR_INVALIDDEFINE);
    return;
  }

  /* Use everything else on the line as the replacement */

  p = m + strlen(m)+1;
  strtok(p, "\n");

  if ((md=find_macro(m))!=NULL)
  {
    if (!eqstr(p,md->szText))  /* #defines are identical */
      error(MEXERR_MACROALREADYDEFINED, m);

    return;
  }

  md=smalloc(sizeof *md);

  /* Grab the name of the macro */

  md->szName=strdup(m);
  md->szText=strdup(p);
  md->iLength=strlen(md->szText);

  /* Append to the linked list of macros */

  md->next=macList;
  macList=md;
}

/* Test for a macro definition - include/exclude block on result */

static void near process_ifdef(char *line, byte inblock,byte which)
{
  char *p;

  if ((p=strtok(line, " \t\n"))==NULL)
  {
    error(MEXERR_INVALIDIFDEF);
    return;
  }

  if (++iBlock >= MAX_IFBLKS)
  {
    error(MEXERR_TOOMANYNESTEDIFDEF);
    iBlock--;
  }

  if (!inblock)
    ifstatus[iBlock]=0;
  else
    ifstatus[iBlock]=find_macro(p) ? which : !which;

  ifdone[iBlock]=ifstatus[iBlock];
}

static void near process_endif()
{
  if (iBlock-- == -1)
  {
    error(MEXERR_UNMATCHEDENDIF);
    iBlock = 0;
  }
}

static void near process_else()
{
  if (iBlock == -1)
    error(MEXERR_UNMATCHEDELSE);
  else
  {
    /* Reverse previous state of this block */

    ifstatus[iBlock]=!ifstatus[iBlock];


    /* If we've already done this block via a
     * previous ifdef or elifdef, don't do it again
     */

    if (ifdone[iBlock])
      ifstatus[iBlock]=0;

    /* Now, if we've just switched this block on, check all previously
     * nested blocks to make sure we're ok on all levels.
     *
     */

    if (ifstatus[iBlock])
    {
      int i;

      for (i=iBlock; i-- > 0; )
        if (!ifstatus[i])     /* Still prevented by at least one level */
          ifstatus[iBlock]=0;

      ifdone[iBlock]=ifstatus[iBlock];
    }
  }
}


static void near process_elseifdef(char *line)
{
  if (iBlock == -1)
    error(MEXERR_UNMATCHEDELIFDEF);
  else
  {
    process_else();
    if (ifstatus[iBlock])
    {
      --iBlock;   /* Step back one level and assert new define */
      process_ifdef(line,TRUE,TRUE);
    }
  }
}



/* Process a preprocessor directive */

static int near process_preprocessor(byte inblock)
{
  char *ppbuf, *s, *tok;

  ppbuf=smalloc(PPLEN);

  /* Read till the end of line */

  for (s=ppbuf;
         s < ppbuf+PPLEN-1 && (*s=(char)peek_character()) != EOF &&
         *s != '\n';
       s++)
  {
    pull_character();
  }

  /* Since we read an EOL, increment the line counter too */

  if (*s=='\n')
  {
    pull_character();
    linenum++;
  }

  /* Cap the string */

  *++s='\0';

  /* Now parse the preprocessor directive from the line */

  tok=strtok(ppbuf, " \t\n");

  if ((s=strtok(NULL, "\r\n"))==NULL)
    s="";
  else
  {
    char *p;

    /* Chop off any '//' comments */

    if ((p=strstr(s, "//")) != NULL)
      *p=0;

    /* And remove trailing whitespace */

    p=s+strlen(s);
    while ( --p >= s && (*p == ' ' || *p == '\t'))
      *p = '\0';
  }

  if (eqstr(tok, "endif"))
    process_endif();
  else if (eqstr(tok, "else"))
    process_else();
  else if (eqstr(tok, "elifdef"))
    process_elseifdef(s);
  else if (eqstr(tok, "elseifdef"))
    process_elseifdef(s);
  else if (eqstr(tok, "ifdef"))
    process_ifdef(s,inblock,TRUE);
  else if (eqstr(tok, "ifndef"))
    process_ifdef(s,inblock,FALSE);
  else if (inblock)
  {
    if (eqstr(tok, "error"))
      error(MEXERR_ERRORDIRECTIVE, s);
    else if (eqstr(tok, "include"))
    {
      tok=strtok(s, " \t\n");
      process_include(tok);
    }
    else if (eqstr(tok, "define"))
      process_define(s);
    else if (eqstr(tok, "undef"))
      process_undef(s);
    else
    {
      error(MEXERR_UNKNOWNDIRECTIVE, tok);
    }
  }

  free(ppbuf);
  return 0;
}




/* Handle structure operator, range operator, and ellipsis operator */

static int near process_dot(void)
{
  int c;

  /* Handle '.' (structure operator), '..' (range operator), and    *
   * '...' (ellipsis).                                              */

  if ((c=peek_character())=='.')
  {
    pull_character();

    if ((c=peek_character())=='.')
    {
      pull_character();
      return T_ELLIPSIS;
    }

    return T_RANGE;
  }

  /* Otherwise it's just a single dot */

  return T_DOT;
}



/* Process a normal identifier */

static int near process_id(int c, int *prc)
{
  MACDEF md;
  struct _id *i;
  char *p, *e;

  /* Probably a reserved word or identifier */

  if (! isidchar(c))
  {
    error(MEXERR_INVALIDCHAR, (byte)c < ' ' ? ' ' : c, c);
    return FALSE;
  }

  /* Set up pointers to identifier buffer */

  p=yytext;
  e=yytext+MAX_ID_LEN-1;

  /* Now copy the characters into the ID buffer 'till we find       *
   * end of file, or a non-identifier character.                    */

  for (*p++=(char)c;
       (*p=(char)peek_character()) != EOF && isidchar(*p) && p < e;
       p++)
    pull_character();

  /* Now cap the string */

  *p='\0';

  if (p==e)
    warn(MEXERR_WARN_IDENTTRUNCATED, yytext);

  /* Scan table of reserved words, and if found, return appropriate *
   * value.                                                         */

  for (i=idlist;i->name;i++)
    if (eqstri(yytext,i->name))
    {
      *prc=i->value;
      return TRUE;
    }


  /* It wasn't found, so make a copy of the name, for insertion in  *
   * the symbol table.                                              */

  yylval.id=strdup(yytext);

  if (!macro.fInMacro && (md=find_macro(yytext))!=NULL)
  {
    macro.fInMacro=TRUE;
    macro.iPos=0;
    macro.md=md;

    return FALSE;
  }

  *prc=T_ID;
  return TRUE;
}



/* yylex() - the lexical analysis function.  */

int yylex(void)
{
  int c, rc;

  /* Get a character until we hit end-of-file */

  while ((c=pull_character()) != EOF)
  {

    /* Non-active #ifdef/#ifndef block handler */

    if (iBlock >= 0 && !ifstatus[iBlock])
    {
      switch (c)
      {
        case '\n':  linenum++;  break;
        case ' ':
        case '\t':
        case '\r':  break;
        case '#':   process_preprocessor(FALSE); break;
        default:    while ((c=peek_character()) != '\n' && c != EOF)
                      pull_character(); /* Waste the rest */
                    break;
      }
    }
    else
    {
      switch (c)
      {
        case '\n':      linenum++;  break;
        case ' ':
        case '\t':
        case '\r':      break;
        case '{':       return T_BEGIN;
        case '}':       return T_END;
        case ':':       return process_colon();
        case '=':       return T_EQUAL;
        case '>':       return process_morethan();
        case '<':       return process_lessthan();
        case '*':       return T_BMULTIPLY;
        case '&':       return T_BAND;
        case '|':       return T_BOR;
        case '/':       if (process_slash()) return T_BDIVIDE; break;
        case '%':       return T_BMODULUS;
        case '+':       return T_BPLUS;
        case '-':       return T_MINUS;
        case '(':       return T_LPAREN;
        case ')':       return T_RPAREN;
        case '[':       return T_LBRACKET;
        case ']':       return T_RBRACKET;
        case ',':       return T_COMMA;
        case ';':       return T_SEMICOLON;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':       return process_digit(c);
        case '\'':      if (process_character_constant()) return T_CONSTBYTE; break;
        case '\"':      return process_string();
        case '#':       process_preprocessor(TRUE); break;
        case '.':       return process_dot(); break;
        default:        if (process_id(c, &rc)) return rc; break;
      } /* switch */
    }
  } /* while */

  if (iBlock != -1)
    error(MEXERR_UNMATCHEDIFDEF);

  /* Return a negative value to indicate EOF */

  return -1;
}

