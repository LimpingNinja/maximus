/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_BYTE = 258,
     T_WORD = 259,
     T_DWORD = 260,
     T_STRING = 261,
     T_VOID = 262,
     T_BEGIN = 263,
     T_END = 264,
     T_IF = 265,
     T_THEN = 266,
     T_ELSE = 267,
     T_GOTO = 268,
     T_WHILE = 269,
     T_DO = 270,
     T_FOR = 271,
     T_STRUCT = 272,
     T_DOT = 273,
     T_ELLIPSIS = 274,
     T_LPAREN = 275,
     T_RPAREN = 276,
     T_LBRACKET = 277,
     T_RBRACKET = 278,
     T_REF = 279,
     T_RETURN = 280,
     T_COMMA = 281,
     T_SEMICOLON = 282,
     T_COLON = 283,
     T_ARRAY = 284,
     T_RANGE = 285,
     T_OF = 286,
     T_UNSIGNED = 287,
     T_SIGNED = 288,
     T_SIZEOF = 289,
     T_ASSIGN = 290,
     T_LOR = 291,
     T_LAND = 292,
     T_EQUAL = 293,
     T_NOTEQUAL = 294,
     T_GT = 295,
     T_GE = 296,
     T_LT = 297,
     T_LE = 298,
     T_BOR = 299,
     T_BAND = 300,
     T_SHR = 301,
     T_SHL = 302,
     T_MINUS = 303,
     T_BPLUS = 304,
     T_BMODULUS = 305,
     T_BDIVIDE = 306,
     T_BMULTIPLY = 307,
     T_CONSTBYTE = 308,
     T_CONSTWORD = 309,
     T_CONSTDWORD = 310,
     T_CONSTSTRING = 311,
     T_ID = 312
   };
#endif
/* Tokens.  */
#define T_BYTE 258
#define T_WORD 259
#define T_DWORD 260
#define T_STRING 261
#define T_VOID 262
#define T_BEGIN 263
#define T_END 264
#define T_IF 265
#define T_THEN 266
#define T_ELSE 267
#define T_GOTO 268
#define T_WHILE 269
#define T_DO 270
#define T_FOR 271
#define T_STRUCT 272
#define T_DOT 273
#define T_ELLIPSIS 274
#define T_LPAREN 275
#define T_RPAREN 276
#define T_LBRACKET 277
#define T_RBRACKET 278
#define T_REF 279
#define T_RETURN 280
#define T_COMMA 281
#define T_SEMICOLON 282
#define T_COLON 283
#define T_ARRAY 284
#define T_RANGE 285
#define T_OF 286
#define T_UNSIGNED 287
#define T_SIGNED 288
#define T_SIZEOF 289
#define T_ASSIGN 290
#define T_LOR 291
#define T_LAND 292
#define T_EQUAL 293
#define T_NOTEQUAL 294
#define T_GT 295
#define T_GE 296
#define T_LT 297
#define T_LE 298
#define T_BOR 299
#define T_BAND 300
#define T_SHR 301
#define T_SHL 302
#define T_MINUS 303
#define T_BPLUS 304
#define T_BMODULUS 305
#define T_BDIVIDE 306
#define T_BMULTIPLY 307
#define T_CONSTBYTE 308
#define T_CONSTWORD 309
#define T_CONSTDWORD 310
#define T_CONSTSTRING 311
#define T_ID 312




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 193 "mex_tab.y"
{
          IDTYPE *id;
          TYPEDESC *typedesc;
          ATTRIBUTES *attrdesc;
          DATAOBJ *dataobj;
          RANGE range;
          CONSTTYPE constant;
          TOKEN token;
          PATCH patch;
          ELSETYPE elsetype;
          FUNCARGS *arg;
          FUNCCALL fcall;
          WHILETYPE whil;
          OPTTYPE opt;
          FORTYPE fr;
          word size;
        }
/* Line 1529 of yacc.c.  */
#line 181 "mex_tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

