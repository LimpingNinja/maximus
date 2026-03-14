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

/*# name=MaxEd editor: Basic spell checking support
*/

#define MAX_INCL_COMMS
#include "maxed.h"
#include <stdio.h>
#include <stdlib.h>
#include <hunspell/hunspell.h>
#include "mci.h"

#define MAGNET_SPELL_BACKEND_NONE      0
#define MAGNET_SPELL_BACKEND_HUNSPELL  1
#define MAGNET_SPELL_BACKEND_WORDLIST  2
#define MAGNET_SPELL_MAX_WORD          64
#define MAGNET_SPELL_MAX_SUGGESTIONS   5
#define MAGNET_SPELL_LIVE_MIN_LEN      4

static Hunhandle *near spell_handle = NULL;
static char **near spell_words = NULL;
static size_t near spell_word_count = 0;
static int near spell_backend = MAGNET_SPELL_BACKEND_NONE;
static int near spell_context_active = FALSE;
static word near spell_active_line = 0;
static word near spell_active_start_col = 0;
static word near spell_active_end_col = 0;
static word near spell_active_cursor_col = 0;
static int near spell_suggestion_count = 0;
static char near spell_suggestions[MAGNET_SPELL_MAX_SUGGESTIONS][MAGNET_SPELL_MAX_WORD];

/**
 * @brief Restore the editor cursor and active message text color.
 */
static void near MagnEt_SpellRestoreCursor(void)
{
  GOTO_TEXT(cursor_x, cursor_y);
  EMIT_MSG_TEXT_COL();
}

/**
 * @brief Return non-zero if the character ends a word for spell checks.
 *
 * The initial Phase 4 pass uses a conservative set of boundaries so the
 * checker only fires after obvious word completion characters.
 */
static int near MagnEt_SpellIsBoundary(int ch)
{
  return (ch==' ' || ch=='.' || ch==',' || ch=='!' || ch=='?' ||
          ch==';' || ch==':' || ch=='\n');
}

/**
 * @brief Reset the currently active spell warning and suggestion state.
 */
static void near MagnEt_SpellResetActive(void)
{
  int i;

  spell_active_line=0;
  spell_active_start_col=0;
  spell_active_end_col=0;
  spell_active_cursor_col=0;
  spell_suggestion_count=0;

  for (i=0; i < MAGNET_SPELL_MAX_SUGGESTIONS; i++)
    spell_suggestions[i][0]='\0';
}

/**
 * @brief Build a path below maximus.data_path.
 *
 * @param leaf     File name to append below the data directory.
 * @param out      Buffer to receive the combined path.
 * @param out_sz   Size of out.
 * @return TRUE if the path fit, FALSE otherwise.
 */
static int near MagnEt_SpellBuildDataPath(const char *leaf, char *out, size_t out_sz)
{
  const char *base;

  if (leaf==NULL || out==NULL || out_sz==0)
    return FALSE;

  base=ngcfg_get_path("maximus.data_path");
  if (base==NULL || *base=='\0')
    return FALSE;

  strnncpy(out, (char *)base, out_sz);
  Add_Trailing(out, PATH_DELIM);

  if (strlen(out) + strlen(leaf) + 1 >= out_sz)
    return FALSE;

  strcat(out, leaf);
  return TRUE;
}

/**
 * @brief Free the fallback word list loaded from wordlist.txt.
 */
static void near MagnEt_SpellFreeWordList(void)
{
  size_t i;

  if (spell_words==NULL)
    return;

  for (i=0; i < spell_word_count; i++)
    if (spell_words[i] != NULL)
      free(spell_words[i]);

  free(spell_words);
  spell_words=NULL;
  spell_word_count=0;
}

/**
 * @brief Load the fallback ASCII word list into memory.
 *
 * The downloaded fallback list is lowercase and already sorted, so the spell
 * checker can use a binary search once it is loaded.
 */
static int near MagnEt_SpellLoadWordList(void)
{
  FILE *fp;
  char path[PATHLEN];
  char linebuf[128];
  char **words;
  size_t count;
  size_t i;

  if (spell_words != NULL)
    return TRUE;

  if (!MagnEt_SpellBuildDataPath("wordlist.txt", path, sizeof(path)))
    return FALSE;

  fp=fopen(path, "r");
  if (fp==NULL)
    return FALSE;

  count=0;
  while (fgets(linebuf, sizeof(linebuf), fp) != NULL)
    count++;

  if (count==0)
  {
    fclose(fp);
    return FALSE;
  }

  words=(char **)calloc(count, sizeof(char *));
  if (words==NULL)
  {
    fclose(fp);
    return FALSE;
  }

  rewind(fp);
  i=0;
  while (i < count && fgets(linebuf, sizeof(linebuf), fp) != NULL)
  {
    size_t len;

    len=strlen(linebuf);
    while (len > 0 && (linebuf[len-1]=='\n' || linebuf[len-1]=='\r'))
      linebuf[--len]='\0';

    words[i]=(char *)malloc(len+1);
    if (words[i]==NULL)
      break;

    strcpy(words[i], linebuf);
    i++;
  }

  fclose(fp);

  if (i != count)
  {
    while (i-- > 0)
      free(words[i]);
    free(words);
    return FALSE;
  }

  spell_words=words;
  spell_word_count=count;
  return TRUE;
}

/**
 * @brief Try to initialize direct Hunspell access using bundled dictionaries.
 */
static int near MagnEt_SpellInitHunspell(void)
{
  char aff_path[PATHLEN];
  char dic_path[PATHLEN];

  if (!MagnEt_SpellBuildDataPath("en_US.aff", aff_path, sizeof(aff_path)))
    return FALSE;

  if (!MagnEt_SpellBuildDataPath("en_US.dic", dic_path, sizeof(dic_path)))
    return FALSE;

  spell_handle=Hunspell_create(aff_path, dic_path);
  if (spell_handle==NULL)
    return FALSE;

  spell_backend=MAGNET_SPELL_BACKEND_HUNSPELL;
  return TRUE;
}

/**
 * @brief Lowercase a word in-place using plain ASCII rules.
 */
static void near MagnEt_SpellLowercase(char *word)
{
  while (*word)
  {
    *word=(char)tolower((unsigned char)*word);
    word++;
  }
}

/**
 * @brief Trim leading and trailing spaces from a token in-place.
 */
static void near MagnEt_SpellTrim(char *text)
{
  char *start;
  char *end;

  if (text==NULL || *text=='\0')
    return;

  start=text;
  while (*start==' ' || *start=='\t')
    start++;

  if (start != text)
    memmove(text, start, strlen(start)+1);

  end=text + strlen(text);
  while (end > text && (end[-1]==' ' || end[-1]=='\t'))
    *--end='\0';
}

/**
 * @brief Split Hunspell's comma-separated suggestion text into display items.
 */
static void near MagnEt_SpellParseSuggestions(const char *raw)
{
  const char *cursor;
  int count;

  MagnEt_SpellResetActive();

  if (raw==NULL || *raw=='\0')
    return;

  cursor=raw;
  count=0;

  while (*cursor && count < MAGNET_SPELL_MAX_SUGGESTIONS)
  {
    const char *sep;
    size_t len;

    while (*cursor==' ' || *cursor=='\t' || *cursor==',')
      cursor++;

    if (*cursor=='\0')
      break;

    sep=strchr(cursor, ',');
    len=sep ? (size_t)(sep-cursor) : strlen(cursor);

    if (len >= MAGNET_SPELL_MAX_WORD)
      len=MAGNET_SPELL_MAX_WORD-1;

    memcpy(spell_suggestions[count], cursor, len);
    spell_suggestions[count][len]='\0';
    MagnEt_SpellTrim(spell_suggestions[count]);

    if (*spell_suggestions[count])
      count++;

    if (sep==NULL)
      break;

    cursor=sep+1;
  }

  spell_suggestion_count=count;
}

/**
 * @brief Locate the word immediately behind the cursor and report its bounds.
 *
 * @param line           Current editor buffer line.
 * @param visible_col    Cursor column after the triggering keystroke.
 * @param skip_boundary  Non-zero if the triggering boundary char is already in
 *                       the buffer and should be skipped before scanning.
 * @param word_out       Destination buffer for the extracted word.
 * @param max_len        Size of word_out.
 * @param start_col_out  Optional start column (visible coordinates).
 * @param end_col_out    Optional end column (visible coordinates).
 * @return Word length, or 0 if no spell-checkable word was found.
 */
static int near MagnEt_SpellLocateWord(const char *line, word visible_col, int skip_boundary,
                                       char *word_out, size_t max_len,
                                       word *start_col_out, word *end_col_out)
{
  word rawpos;
  int end;
  int start;
  int len;

  if (line==NULL || word_out==NULL || max_len < 2)
    return 0;

  rawpos=MagnEt_BufferPosFromVisibleCol(line, visible_col);
  end=(int)rawpos-1;

  if (skip_boundary)
    end--;

  while (end > 0 && !isalpha((unsigned char)line[end]))
    end--;

  if (end <= 0)
    return 0;

  start=end;
  while (start > 0 && isalpha((unsigned char)line[start]))
    start--;

  if (!isalpha((unsigned char)line[start]))
    start++;

  len=end-start+1;
  if (len <= 0 || (size_t)len >= max_len)
    return 0;

  memcpy(word_out, line+start, (size_t)len);
  word_out[len]='\0';
  MagnEt_SpellLowercase(word_out);

  if (start_col_out != NULL)
    *start_col_out=MagnEt_VisibleColFromBufferPos(line, (word)start)+1;

  if (end_col_out != NULL)
    *end_col_out=MagnEt_VisibleColFromBufferPos(line, (word)(end+1));

  return len;
}

/**
 * @brief Check a word against the in-memory fallback word list.
 */
static int near MagnEt_SpellCheckWordList(const char *word)
{
  size_t lo;
  size_t hi;

  if (spell_words==NULL || spell_word_count==0)
    return FALSE;

  lo=0;
  hi=spell_word_count;

  while (lo < hi)
  {
    size_t mid;
    int cmp;

    mid=lo + ((hi-lo) / 2);
    cmp=strcmp(word, spell_words[mid]);

    if (cmp==0)
      return TRUE;

    if (cmp < 0)
      hi=mid;
    else lo=mid+1;
  }

  return FALSE;
}

/**
 * @brief Check a word through the direct Hunspell library API.
 *
 * @param word         Lowercase ASCII word to test.
 * @param suggestions  Optional buffer for Hunspell suggestions.
 * @param sug_sz       Size of suggestions buffer.
 * @return TRUE if the word is accepted, FALSE otherwise.
 */
static int near MagnEt_SpellCheckHunspell(const char *word, char *suggestions, size_t sug_sz)
{
  char **list;
  int count;
  int i;
  size_t used;

  if (spell_handle==NULL)
    return FALSE;

  if (suggestions != NULL && sug_sz)
    *suggestions='\0';

  if (Hunspell_spell(spell_handle, word) != 0)
    return TRUE;

  if (suggestions==NULL || sug_sz==0)
    return FALSE;

  list=NULL;
  count=Hunspell_suggest(spell_handle, &list, word);
  if (count <= 0 || list==NULL)
    return FALSE;

  used=0;

  for (i=0; i < count; i++)
  {
    size_t len;

    if (list[i]==NULL || *list[i]=='\0')
      continue;

    len=strlen(list[i]);

    if (used != 0)
    {
      if (used + 2 >= sug_sz)
        break;

      suggestions[used++]=',';
      suggestions[used++]=' ';
    }

    if (used + len >= sug_sz)
      len=sug_sz-used-1;

    if (len==0)
      break;

    memcpy(suggestions+used, list[i], len);
    used += len;
    suggestions[used]='\0';
  }

  Hunspell_free_list(spell_handle, &list, count);

  return FALSE;
}

/**
 * @brief Render the current spell warning on the context line.
 */
static void near MagnEt_SpellShowUnknown(const char *word)
{
  const char *fmt;
  int i;

  fmt=maxlang_get(g_current_lang, "editor.spell_unknown_word");

  Goto(MAGNET_CONTEXT_ROW, 1);
  Puts(CLEOL);

  if (fmt && *fmt)
    LangPrintfForce((char *)fmt, word);
  else
  {
    PutsForce("|erUnknown: |hi\"");
    Puts((char *)word);
    PutsForce("\"|cd");
  }

  for (i=0; i < spell_suggestion_count && i < 3; i++)
  {
    PutsForce(" |hk");
    Putc('1'+i);
    PutsForce("|hi=");
    Puts((char *)spell_suggestions[i]);
    PutsForce("|cd");
  }

  spell_context_active=TRUE;
  MagnEt_SpellRestoreCursor();
}

/**
 * @brief Clear spell-owned context content and restore the text cursor.
 */
void MagnEt_SpellClear(void)
{
  if (!spell_context_active)
  {
    MagnEt_SpellResetActive();
    return;
  }

  MagnEt_ClearContextLine();
  spell_context_active=FALSE;
  MagnEt_SpellResetActive();
  MagnEt_SpellRestoreCursor();
}

/**
 * @brief Extract the word immediately behind the cursor.
 *
 * @param line           Current editor buffer line.
 * @param visible_col    Cursor column after the triggering keystroke.
 * @param skip_boundary  Non-zero if the triggering boundary char is already in
 *                       the buffer and should be skipped before scanning.
 * @param word_out       Destination buffer.
 * @param max_len        Size of word_out.
 * @return Word length, or 0 if no spell-checkable word was found.
 */
int MagnEt_SpellExtractWord(const char *line, word visible_col, int skip_boundary,
                            char *word_out, size_t max_len)
{
  return MagnEt_SpellLocateWord(line, visible_col, skip_boundary,
                                word_out, max_len, NULL, NULL);
}

/**
 * @brief Initialize the spell checker backend for the current editor session.
 */
void MagnEt_SpellInit(void)
{
  spell_backend=MAGNET_SPELL_BACKEND_NONE;
  spell_context_active=FALSE;
  MagnEt_SpellResetActive();

  if (MagnEt_SpellInitHunspell())
    return;

  if (MagnEt_SpellLoadWordList())
    spell_backend=MAGNET_SPELL_BACKEND_WORDLIST;
}

/**
 * @brief Release spell checker resources for the current editor session.
 */
void MagnEt_SpellDone(void)
{
  if (spell_handle != NULL)
  {
    Hunspell_destroy(spell_handle);
    spell_handle=NULL;
  }

  MagnEt_SpellFreeWordList();
  spell_backend=MAGNET_SPELL_BACKEND_NONE;
  spell_context_active=FALSE;
  MagnEt_SpellResetActive();
}

/**
 * @brief Spell-check the word that was just completed on the current line.
 *
 * @param skip_boundary Non-zero if the boundary character is already present in
 *                      the line buffer and should be skipped.
 */
void MagnEt_SpellCheckCurrentWord(int skip_boundary)
{
  char word[MAGNET_SPELL_MAX_WORD];
  char suggestions[256];
  const char *line;
  int ok;
  unsigned short spell_start_col;
  unsigned short spell_end_col;

  line=(char *)screen[offset+cursor_x];
  if (line==NULL)
    return;

  if (MagnEt_SpellLocateWord(line, cursor_y, skip_boundary, word, sizeof(word),
                             &spell_start_col, &spell_end_col) <= 0)
  {
    MagnEt_SpellClear();
    return;
  }

  ok=FALSE;
  suggestions[0]='\0';

  if (spell_backend==MAGNET_SPELL_BACKEND_HUNSPELL)
    ok=MagnEt_SpellCheckHunspell(word, suggestions, sizeof(suggestions));
  else if (spell_backend==MAGNET_SPELL_BACKEND_WORDLIST)
    ok=MagnEt_SpellCheckWordList(word);

  if (ok)
    MagnEt_SpellClear();
  else
  {
    MagnEt_SpellParseSuggestions(suggestions);
    spell_active_line=offset+cursor_x;
    spell_active_start_col=spell_start_col;
    spell_active_end_col=spell_end_col;
    spell_active_cursor_col=cursor_y;
    MagnEt_SpellShowUnknown(word);
  }
}

/**
 * @brief React to a typed character from the main editor loop.
 *
 * Alphabetic characters clear a previous warning when the user starts fixing a
 * word. Boundary characters trigger a spell check for the word that ended.
 */
void MagnEt_SpellHandleTypedChar(int ch)
{
  const char *line;
  char word[MAGNET_SPELL_MAX_WORD];

  if (spell_backend==MAGNET_SPELL_BACKEND_NONE)
    return;

  if (MagnEt_SpellIsBoundary(ch))
  {
    MagnEt_SpellCheckCurrentWord(TRUE);
    return;
  }

  if (isalpha((unsigned char)ch))
  {
    line=(char *)screen[offset+cursor_x];

    if (line != NULL &&
        MagnEt_SpellExtractWord(line, cursor_y, FALSE, word, sizeof(word)) >= MAGNET_SPELL_LIVE_MIN_LEN)
    {
      MagnEt_SpellCheckCurrentWord(FALSE);
      return;
    }
  }

  MagnEt_SpellClear();
}

/**
 * @brief Apply a numbered suggestion for the active misspelled word.
 *
 * The first pass only applies corrections while the cursor remains on the same
 * editor line. This keeps the replacement path inside the existing line edit
 * machinery so insert-mode, redraw, and wrapping rules stay intact.
 *
 * @param index Zero-based suggestion index.
 * @return TRUE if a suggestion was applied, FALSE otherwise.
 */
int MagnEt_SpellApplySuggestion(int index)
{
  int old_insert;
  int old_len;
  int new_len;
  int i;
  word abs_line;
  word restore_col;

  if (index < 0 || index >= spell_suggestion_count)
    return FALSE;

  abs_line=offset+cursor_x;
  if (spell_active_line==0 || spell_active_line != abs_line)
    return FALSE;

  if (spell_active_start_col < 1 || spell_active_end_col < spell_active_start_col)
    return FALSE;

  old_len=(int)(spell_active_end_col-spell_active_start_col)+1;
  new_len=(int)strlen(spell_suggestions[index]);
  restore_col=(word)(spell_active_cursor_col + (new_len-old_len));

  if (new_len <= 0)
    return FALSE;

  GOTO_TEXT(cursor_x, cursor_y=spell_active_start_col);

  for (i=0; i < old_len; i++)
    Delete_Char();

  old_insert=insert;
  insert=TRUE;

  for (i=0; i < new_len; i++)
    Add_Character(spell_suggestions[index][i]);

  insert=old_insert;

  cursor_y=restore_col;
  MagnEt_ClampCursor();
  GOTO_TEXT(cursor_x, cursor_y);
  Update_Position();
  MagnEt_SpellClear();
  return TRUE;
}
