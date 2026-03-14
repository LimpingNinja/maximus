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

/**
 * @brief Emit msg_text_col as a full Avatar attribute set.
 *
 * Converts the pipe color string to a PC attribute byte via Mci2Attr,
 * then emits it as \x16\x01<attr>.  This sets fg AND bg together,
 * matching the original .mad COL_MSG_BODY behavior (\x16\x01\x03).
 * Pipe codes like |03 only touch the fg nibble — this avoids that
 * pitfall and properly resets background after blue status bars.
 */
#define EMIT_MSG_TEXT_COL() do { \
  byte _a = Mci2Attr(msg_text_col, 0x07); \
  Putc('\x16'); Putc('\x01'); Putc((int)_a); \
} while (0)

#define MAGNET_ESC_NONE    0
#define MAGNET_ESC_SAVE    1
#define MAGNET_ESC_ABORT   2
#define MAGNET_ESC_HELP    3
#define MAGNET_ESC_QUOTE   4
#define MAGNET_ESC_COLOR   5
#define MAGNET_ESC_REDRAW  6

#define MAGNET_TEXT_ROW(row) ((row) + text_start_row - 1)
#define GOTO_TEXT(row, col)  Goto(MAGNET_TEXT_ROW((row)), (col))
#define MAGNET_DIVIDER_ROW   (text_start_row + usrlen)
#define MAGNET_CONTEXT_ROW   (text_start_row + usrlen + 1)
#define MAGNET_STATUS_ROW    (text_start_row + usrlen + 2)

void BackSpace(void);
void Delete_Char(void);
void Delete_Line(int cx);
void Delete_Word(void)   /*ABK 1990-09-02 14:25:34 */;
void EdMemOvfl(void);
void Add_Character(int ch);
void New_Line(int col);
word Carriage_Return(int hard);
int Insert_Line_Before(int cx);
void MagnEt_Help(void);
void MagnEt_Menu(void);
int MagnEt_ContextYesNo(const char *prompt);
int MagnEt_EscMenu(struct _replyp *pr);
int MagnEt_InsertColor(void);
void MagnEt_ClearContextLine(void);
void MagnEt_DrawFooterDivider(void);
byte MagnEt_CalcHeaderHeight(void);
void MagnEt_DrawHeader(void);
void MagnEt_SpellInit(void);
void MagnEt_SpellDone(void);
void MagnEt_SpellClear(void);
void MagnEt_SpellCheckCurrentWord(int skip_boundary);
void MagnEt_SpellHandleTypedChar(int ch);
int MagnEt_SpellApplySuggestion(int index);
int MagnEt_SpellExtractWord(const char *line, word visible_col, int skip_boundary,
                            char *word_out, size_t max_len);
void Piggy(void);
void MagnEt_Bad_Keystroke(void);
void Cursor_Left(void);
void Cursor_Right(void);
void Cursor_Up(void);
void Cursor_Down(int update_pos);
void Cursor_BeginLine(void);
void Cursor_EndLine(void);
void Word_Left(void);
void Word_Right(void);
void Scroll_Up(int n,int location);
void Scroll_Down(int n,int location);
void Page_Up(void);
void Page_Down(void);
void Quote_OnOff(struct _replyp *pr);
void Quote_Up(void);
void Quote_Down(void);
void Quote_Copy(void);
int  Quote_Popup(struct _replyp *pr);
word MagnEt_LineVisibleLen(const char *line);
word MagnEt_BufferPosFromVisibleCol(const char *line, word col);
word MagnEt_VisibleColFromBufferPos(const char *line, word pos);
char MagnEt_VisibleCharAt(const char *line, word col);
int MagnEt_LineHasColorCodes(const char *line);
void MagnEt_ClampCursor(void);
void Read_DiskFile(void);
void Load_Message(HMSG msgh);
void Update_Line(word cx, word cy, word inc, word update_cursor);
int Word_Wrap(word mode);
void Toggle_Insert(void);
void Update_Position(void);
void NoFF_CLS(void);
void Fix_MagnEt(void);
void Do_Update(void);
int Mdm_getcwcc(void);

