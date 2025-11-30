/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_MEX_TAB_H_INCLUDED
# define YY_YY_MEX_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    T_BYTE = 258,                  /* T_BYTE  */
    T_WORD = 259,                  /* T_WORD  */
    T_DWORD = 260,                 /* T_DWORD  */
    T_STRING = 261,                /* T_STRING  */
    T_VOID = 262,                  /* T_VOID  */
    T_BEGIN = 263,                 /* T_BEGIN  */
    T_END = 264,                   /* T_END  */
    T_IF = 265,                    /* T_IF  */
    T_THEN = 266,                  /* T_THEN  */
    T_ELSE = 267,                  /* T_ELSE  */
    T_GOTO = 268,                  /* T_GOTO  */
    T_WHILE = 269,                 /* T_WHILE  */
    T_DO = 270,                    /* T_DO  */
    T_FOR = 271,                   /* T_FOR  */
    T_STRUCT = 272,                /* T_STRUCT  */
    T_DOT = 273,                   /* T_DOT  */
    T_ELLIPSIS = 274,              /* T_ELLIPSIS  */
    T_LPAREN = 275,                /* T_LPAREN  */
    T_RPAREN = 276,                /* T_RPAREN  */
    T_LBRACKET = 277,              /* T_LBRACKET  */
    T_RBRACKET = 278,              /* T_RBRACKET  */
    T_REF = 279,                   /* T_REF  */
    T_RETURN = 280,                /* T_RETURN  */
    T_COMMA = 281,                 /* T_COMMA  */
    T_SEMICOLON = 282,             /* T_SEMICOLON  */
    T_COLON = 283,                 /* T_COLON  */
    T_ARRAY = 284,                 /* T_ARRAY  */
    T_RANGE = 285,                 /* T_RANGE  */
    T_OF = 286,                    /* T_OF  */
    T_UNSIGNED = 287,              /* T_UNSIGNED  */
    T_SIGNED = 288,                /* T_SIGNED  */
    T_SIZEOF = 289,                /* T_SIZEOF  */
    T_ASSIGN = 290,                /* T_ASSIGN  */
    T_LAND = 291,                  /* T_LAND  */
    T_LOR = 292,                   /* T_LOR  */
    T_NOTEQUAL = 293,              /* T_NOTEQUAL  */
    T_EQUAL = 294,                 /* T_EQUAL  */
    T_LE = 295,                    /* T_LE  */
    T_LT = 296,                    /* T_LT  */
    T_GE = 297,                    /* T_GE  */
    T_GT = 298,                    /* T_GT  */
    T_SHL = 299,                   /* T_SHL  */
    T_SHR = 300,                   /* T_SHR  */
    T_BAND = 301,                  /* T_BAND  */
    T_BOR = 302,                   /* T_BOR  */
    T_BPLUS = 303,                 /* T_BPLUS  */
    T_MINUS = 304,                 /* T_MINUS  */
    T_BMULTIPLY = 305,             /* T_BMULTIPLY  */
    T_BDIVIDE = 306,               /* T_BDIVIDE  */
    T_BMODULUS = 307,              /* T_BMODULUS  */
    T_CONSTBYTE = 308,             /* T_CONSTBYTE  */
    T_CONSTWORD = 309,             /* T_CONSTWORD  */
    T_CONSTDWORD = 310,            /* T_CONSTDWORD  */
    T_CONSTSTRING = 311,           /* T_CONSTSTRING  */
    T_ID = 312                     /* T_ID  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 193 "mex_tab.y"

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
        

#line 140 "mex_tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_MEX_TAB_H_INCLUDED  */
