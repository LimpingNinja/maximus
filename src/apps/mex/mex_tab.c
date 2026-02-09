/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 166 "mex_tab.y"


  #define MEX_PARSER

  #include <stdio.h>
  #include <stdlib.h>
  #include <mem.h>
  #include "alc.h"
  #include "prog.h"
  #include "mex.h"

  #ifdef __TURBOC__
  #pragma warn -cln
  #endif
    
  ATTRIBUTES *curfn=NULL;
  
  #ifndef __GNUC__
  #pragma off(unreferenced)
  static char rcs_id[]="$Id: mex_tab.y,v 1.4 2004/01/27 20:57:25 paltas Exp $";
  #pragma on(unreferenced)
  #endif


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

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
/* Line 193 of yacc.c.  */
#line 252 "mex_tab.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 265 "mex_tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   532

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  58
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  53
/* YYNRULES -- Number of rules.  */
#define YYNRULES  124
/* YYNRULES -- Number of states.  */
#define YYNSTATES  217

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   312

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    11,    13,    14,    15,
      16,    26,    28,    30,    31,    37,    38,    40,    44,    46,
      50,    51,    53,    54,    60,    61,    64,    68,    69,    77,
      80,    82,    84,    86,    89,    92,    95,    98,   101,   104,
     106,   108,   115,   118,   122,   125,   129,   131,   132,   135,
     137,   140,   143,   144,   150,   154,   155,   160,   161,   162,
     168,   169,   176,   177,   178,   179,   192,   196,   199,   201,
     202,   203,   207,   208,   214,   215,   219,   221,   223,   228,
     233,   235,   237,   239,   240,   242,   246,   248,   250,   254,
     258,   262,   266,   270,   274,   278,   282,   286,   290,   294,
     298,   302,   306,   310,   314,   318,   321,   325,   327,   329,
     331,   333,   335,   337,   339,   341,   343,   345,   348,   350,
     355,   359,   361,   366,   370
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      59,     0,    -1,    60,    -1,    -1,    60,    61,    -1,    62,
      -1,    75,    -1,    -1,    -1,    -1,    78,   110,    63,    20,
      64,    69,    21,    65,    66,    -1,    67,    -1,    27,    -1,
      -1,     8,    68,    74,    81,     9,    -1,    -1,    19,    -1,
      70,    26,    69,    -1,    70,    -1,    71,    77,   110,    -1,
      -1,    24,    -1,    -1,     8,    73,    74,    81,     9,    -1,
      -1,    74,    75,    -1,    77,    80,    27,    -1,    -1,    17,
     110,     8,    76,    74,     9,    27,    -1,    78,    28,    -1,
       3,    -1,     4,    -1,     5,    -1,    33,     3,    -1,    33,
       4,    -1,    33,     5,    -1,    32,     3,    -1,    32,     4,
      -1,    32,     5,    -1,     7,    -1,     6,    -1,    29,    22,
      79,    23,    31,    78,    -1,    17,   110,    -1,    54,    30,
      54,    -1,    54,    30,    -1,    80,    26,   110,    -1,   110,
      -1,    -1,    81,    82,    -1,    72,    -1,   101,    27,    -1,
     100,    27,    -1,    -1,    10,    98,    83,    82,    91,    -1,
      13,   110,    27,    -1,    -1,   110,    28,    84,    82,    -1,
      -1,    -1,    14,    85,    98,    86,    82,    -1,    -1,    15,
      87,    82,    14,    98,    27,    -1,    -1,    -1,    -1,    16,
      20,    97,    27,    88,    97,    27,    89,    97,    90,    21,
      82,    -1,    25,    97,    27,    -1,     1,    27,    -1,    27,
      -1,    -1,    -1,    12,    92,    82,    -1,    -1,   110,    20,
      94,    95,    21,    -1,    -1,    99,    26,    95,    -1,    99,
      -1,    98,    -1,    20,    78,    21,    96,    -1,    34,    20,
      78,    21,    -1,    93,    -1,   106,    -1,   108,    -1,    -1,
      99,    -1,    20,    99,    21,    -1,   101,    -1,   100,    -1,
      99,    52,    99,    -1,    99,    51,    99,    -1,    99,    50,
      99,    -1,    99,    49,    99,    -1,    99,    48,    99,    -1,
      99,    43,    99,    -1,    99,    42,    99,    -1,    99,    46,
      99,    -1,    99,    47,    99,    -1,    99,    45,    99,    -1,
      99,    44,    99,    -1,    99,    37,    99,    -1,    99,    36,
      99,    -1,    99,    38,    99,    -1,    99,    39,    99,    -1,
      99,    41,    99,    -1,    99,    40,    99,    -1,    48,    96,
      -1,   109,    35,    99,    -1,    96,    -1,    53,    -1,    54,
      -1,    55,    -1,   107,    -1,   102,    -1,   103,    -1,   104,
      -1,   105,    -1,    56,    -1,   107,    56,    -1,    57,    -1,
     108,    22,    99,    23,    -1,   108,    18,   110,    -1,    57,
      -1,   108,    22,    99,    23,    -1,   108,    18,   110,    -1,
      57,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   265,   265,   268,   269,   272,   273,   278,   280,   282,
     277,   291,   293,   298,   297,   309,   310,   312,   316,   322,
     331,   332,   338,   337,   350,   351,   354,   357,   356,   362,
     366,   368,   370,   372,   374,   376,   378,   380,   382,   384,
     386,   388,   390,   395,   410,   423,   425,   430,   431,   438,
     440,   442,   448,   447,   451,   454,   453,   457,   459,   456,
     463,   462,   486,   490,   494,   485,   503,   505,   507,   512,
     516,   515,   522,   521,   528,   529,   541,   556,   558,   560,
     562,   564,   566,   572,   573,   578,   582,   584,   588,   590,
     592,   594,   596,   598,   600,   602,   604,   606,   608,   610,
     612,   614,   616,   618,   620,   622,   627,   636,   640,   644,
     648,   652,   656,   658,   660,   662,   667,   669,   672,   674,
     676,   680,   682,   684,   688
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_BYTE", "T_WORD", "T_DWORD",
  "T_STRING", "T_VOID", "T_BEGIN", "T_END", "T_IF", "T_THEN", "T_ELSE",
  "T_GOTO", "T_WHILE", "T_DO", "T_FOR", "T_STRUCT", "T_DOT", "T_ELLIPSIS",
  "T_LPAREN", "T_RPAREN", "T_LBRACKET", "T_RBRACKET", "T_REF", "T_RETURN",
  "T_COMMA", "T_SEMICOLON", "T_COLON", "T_ARRAY", "T_RANGE", "T_OF",
  "T_UNSIGNED", "T_SIGNED", "T_SIZEOF", "T_ASSIGN", "T_LOR", "T_LAND",
  "T_EQUAL", "T_NOTEQUAL", "T_GT", "T_GE", "T_LT", "T_LE", "T_BOR",
  "T_BAND", "T_SHR", "T_SHL", "T_MINUS", "T_BPLUS", "T_BMODULUS",
  "T_BDIVIDE", "T_BMULTIPLY", "T_CONSTBYTE", "T_CONSTWORD", "T_CONSTDWORD",
  "T_CONSTSTRING", "T_ID", "$accept", "program", "top_list",
  "func_or_decl", "function", "@1", "@2", "@3", "trailing_part",
  "function_block", "@4", "arg_list", "argument", "opt_ref", "block", "@5",
  "declarator_list", "declaration", "@6", "typename", "typedefn", "range",
  "id_list", "statement_list", "statement", "@7", "@8", "@9", "@10", "@11",
  "@12", "@13", "@14", "else_part", "@15", "function_call", "@16",
  "expr_list", "primary", "opt_expr", "paren_expr", "expr", "useless_expr",
  "useful_expr", "const_byte_p", "const_word_p", "const_dword_p",
  "const_string_p", "literal", "const_string", "ident", "lval_ident", "id", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    58,    59,    60,    60,    61,    61,    63,    64,    65,
      62,    66,    66,    68,    67,    69,    69,    69,    69,    70,
      71,    71,    73,    72,    74,    74,    75,    76,    75,    77,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    79,    79,    80,    80,    81,    81,    82,
      82,    82,    83,    82,    82,    84,    82,    85,    86,    82,
      87,    82,    88,    89,    90,    82,    82,    82,    82,    91,
      92,    91,    94,    93,    95,    95,    95,    96,    96,    96,
      96,    96,    96,    97,    97,    98,    99,    99,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   101,   101,   102,   103,
     104,   105,   106,   106,   106,   106,   107,   107,   108,   108,
     108,   109,   109,   109,   110
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     2,     1,     1,     0,     0,     0,
       9,     1,     1,     0,     5,     0,     1,     3,     1,     3,
       0,     1,     0,     5,     0,     2,     3,     0,     7,     2,
       1,     1,     1,     2,     2,     2,     2,     2,     2,     1,
       1,     6,     2,     3,     2,     3,     1,     0,     2,     1,
       2,     2,     0,     5,     3,     0,     4,     0,     0,     5,
       0,     6,     0,     0,     0,    12,     3,     2,     1,     0,
       0,     3,     0,     5,     0,     3,     1,     1,     4,     4,
       1,     1,     1,     0,     1,     3,     1,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     2,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     1,     4,
       3,     1,     4,     3,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     2,     1,    30,    31,    32,    40,    39,     0,
       0,     0,     0,     4,     5,     6,     0,     0,   124,    42,
       0,    36,    37,    38,    33,    34,    35,     0,    46,    29,
       7,    27,     0,     0,     0,    26,     0,    24,    44,     0,
      45,     8,     0,    43,     0,    20,     0,    25,     0,     0,
      41,    16,    21,     0,    18,     0,    28,    42,     9,    20,
       0,     0,    17,    19,    13,    12,    10,    11,    24,    47,
       0,     0,    22,    14,     0,     0,    57,    60,     0,     0,
      83,    68,     0,     0,   108,   109,   110,   116,   118,    49,
      48,    80,   107,    77,     0,    87,    86,   112,   113,   114,
     115,    81,   111,    82,     0,     0,    67,    24,     0,    52,
       0,     0,     0,    83,     0,     0,    87,    86,     0,     0,
      84,     0,   118,   105,    82,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    51,    50,   117,     0,     0,     0,    72,    55,
      47,     0,    54,    58,     0,     0,     0,    85,    66,     0,
       0,     0,   100,    99,   101,   102,   104,   103,    94,    93,
      98,    97,    95,    96,    92,    91,    90,    89,    88,   120,
       0,   106,    74,     0,     0,    69,     0,     0,    62,    78,
      79,   120,     0,   119,     0,    76,    56,    23,    70,    53,
      59,     0,    83,   119,    73,    74,     0,    61,     0,    75,
      71,    63,    83,    64,     0,     0,    65
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,    13,    14,    36,    45,    61,    66,    67,
      68,    53,    54,    55,    89,   107,    42,    47,    37,    16,
      48,    33,    27,    70,    90,   151,   183,   111,   186,   112,
     202,   212,   214,   199,   206,    91,   182,   194,    92,   119,
      93,    94,   116,   117,    97,    98,    99,   100,   101,   102,
     103,   104,   118
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -111
static const yytype_int16 yypact[] =
{
    -111,    14,   375,  -111,  -111,  -111,  -111,  -111,  -111,   -29,
      13,    59,    66,  -111,  -111,  -111,   -29,   -15,  -111,    18,
     -10,  -111,  -111,  -111,  -111,  -111,  -111,    26,  -111,  -111,
    -111,  -111,     9,    23,   -29,  -111,    40,  -111,    11,    36,
    -111,  -111,   309,  -111,   380,     8,    45,  -111,    46,   -29,
    -111,  -111,  -111,    54,    52,   380,  -111,  -111,  -111,     8,
     -29,     4,  -111,  -111,  -111,  -111,  -111,  -111,  -111,   375,
     175,    65,  -111,  -111,    61,   -29,  -111,  -111,    73,   303,
       2,  -111,    74,    34,  -111,  -111,  -111,  -111,    -5,  -111,
    -111,  -111,  -111,  -111,   452,    68,    70,  -111,  -111,  -111,
    -111,  -111,    43,    15,    67,    -4,  -111,  -111,     2,  -111,
      77,    61,   248,     2,    79,   325,  -111,  -111,    86,    80,
     452,   380,    89,  -111,    16,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,  -111,  -111,  -111,   -29,     2,     2,  -111,  -111,
     375,   248,  -111,  -111,    87,    83,    34,  -111,  -111,    92,
     -29,     2,   467,   467,   480,   480,    81,    81,    81,    81,
     -31,   -31,   -31,   -31,    33,    33,  -111,  -111,  -111,    85,
     378,   452,     2,   248,   232,   104,   248,    61,  -111,  -111,
    -111,  -111,   408,    88,   113,   435,  -111,  -111,  -111,  -111,
    -111,   108,     2,  -111,  -111,     2,   248,  -111,   111,  -111,
    -111,  -111,     2,  -111,   118,   248,  -111
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -111,  -111,  -111,  -111,  -111,  -111,  -111,  -111,  -111,  -111,
    -111,    84,  -111,  -111,  -111,  -111,   -58,   142,  -111,    93,
       1,  -111,  -111,    -3,  -110,  -111,  -111,  -111,  -111,  -111,
    -111,  -111,  -111,  -111,  -111,  -111,  -111,   -56,   -79,  -104,
     -63,    32,   -69,   -65,  -111,  -111,  -111,  -111,  -111,  -111,
     -77,  -111,    -9
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -125
static const yytype_int16 yytable[] =
{
      19,    95,   154,    17,   123,    96,   124,    28,    30,   155,
      69,   109,    64,    29,     3,  -124,   148,   137,   138,   139,
     140,   141,    79,  -124,   149,    40,    31,    51,    18,   -15,
    -121,    65,    52,   145,   160,    20,    82,   146,   161,    38,
      57,   185,    18,    95,    32,    50,    39,    96,   153,   150,
      83,    63,    34,    35,    79,    84,    85,    86,    87,    88,
      41,   105,    21,    22,    23,    43,   110,    44,    82,    24,
      25,    26,    56,   196,    29,    58,   200,   189,    59,   124,
     114,   108,    95,   139,   140,   141,    96,    84,    85,    86,
      87,   122,   106,   113,   121,   142,   210,   143,   208,   144,
     156,   187,   147,   105,   152,   216,   148,   158,   213,  -124,
     188,   115,   120,   190,    95,    95,   198,    95,    96,    96,
    -123,    96,   159,  -122,   201,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   204,   207,   179,    95,   211,   215,
     115,    96,   105,    62,    15,   120,    95,   184,    60,   209,
      96,   191,     0,     0,     0,     0,     0,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   105,   105,    71,   105,   180,   181,
       0,     0,     0,    72,    73,    74,     0,     0,    75,    76,
      77,    78,     0,   192,     0,    79,     0,   105,     0,     0,
      80,     0,    81,     0,     0,     0,   105,     0,     0,    82,
       0,     0,     0,     0,   195,     0,     0,     0,     0,     0,
       0,     0,     0,    83,     0,     0,     0,     0,    84,    85,
      86,    87,    88,    71,   120,     0,     0,   195,     0,     0,
      72,   197,    74,     0,   120,    75,    76,    77,    78,    71,
       0,     0,    79,     0,     0,     0,    72,    80,    74,    81,
       0,    75,    76,    77,    78,     0,    82,     0,    79,     0,
       0,     0,     0,    80,     0,    81,     0,     0,     0,     0,
      83,     0,    82,     0,     0,    84,    85,    86,    87,    88,
       0,     0,     0,     0,     0,     0,    83,     0,     0,     0,
       0,    84,    85,    86,    87,    88,     4,     5,     6,     7,
       8,     0,     4,     5,     6,     7,     8,     0,    46,     0,
      49,     0,     0,    79,     0,     0,     9,     0,     0,     0,
       0,     0,    10,     0,     0,    11,    12,    82,    10,     0,
       0,    11,    12,     0,     0,     0,   157,     0,     0,     0,
       0,    83,     0,     0,     0,     0,    84,    85,    86,    87,
      88,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,     4,     5,
       6,     7,     8,     4,     5,     6,     7,     8,     0,     0,
       0,     0,     9,     0,     0,     0,     0,    49,     0,     0,
       0,   193,     0,     0,    10,     0,     0,    11,    12,    10,
       0,     0,    11,    12,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   203,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   205,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141
};

static const yytype_int16 yycheck[] =
{
       9,    70,   112,     2,    83,    70,    83,    16,    17,   113,
      68,    74,     8,    28,     0,    20,    20,    48,    49,    50,
      51,    52,    20,    28,    28,    34,     8,    19,    57,    21,
      35,    27,    24,    18,    18,    22,    34,    22,    22,    30,
      49,   151,    57,   112,    54,    44,    23,   112,   111,   107,
      48,    60,    26,    27,    20,    53,    54,    55,    56,    57,
      20,    70,     3,     4,     5,    54,    75,    31,    34,     3,
       4,     5,    27,   183,    28,    21,   186,   156,    26,   156,
      79,    20,   151,    50,    51,    52,   151,    53,    54,    55,
      56,    57,    27,    20,    20,    27,   206,    27,   202,    56,
      21,    14,    35,   112,    27,   215,    20,    27,   212,    20,
      27,    79,    80,    21,   183,   184,    12,   186,   183,   184,
      35,   186,   121,    35,   187,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    21,    27,   145,   206,    27,    21,
     108,   206,   151,    59,     2,   113,   215,   150,    55,   205,
     215,   160,    -1,    -1,    -1,    -1,    -1,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   183,   184,     1,   186,   146,   147,
      -1,    -1,    -1,     8,     9,    10,    -1,    -1,    13,    14,
      15,    16,    -1,   161,    -1,    20,    -1,   206,    -1,    -1,
      25,    -1,    27,    -1,    -1,    -1,   215,    -1,    -1,    34,
      -1,    -1,    -1,    -1,   182,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    48,    -1,    -1,    -1,    -1,    53,    54,
      55,    56,    57,     1,   202,    -1,    -1,   205,    -1,    -1,
       8,     9,    10,    -1,   212,    13,    14,    15,    16,     1,
      -1,    -1,    20,    -1,    -1,    -1,     8,    25,    10,    27,
      -1,    13,    14,    15,    16,    -1,    34,    -1,    20,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    -1,
      48,    -1,    34,    -1,    -1,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    -1,    -1,    -1,    48,    -1,    -1,    -1,
      -1,    53,    54,    55,    56,    57,     3,     4,     5,     6,
       7,    -1,     3,     4,     5,     6,     7,    -1,     9,    -1,
      17,    -1,    -1,    20,    -1,    -1,    17,    -1,    -1,    -1,
      -1,    -1,    29,    -1,    -1,    32,    33,    34,    29,    -1,
      -1,    32,    33,    -1,    -1,    -1,    21,    -1,    -1,    -1,
      -1,    48,    -1,    -1,    -1,    -1,    53,    54,    55,    56,
      57,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,     3,     4,
       5,     6,     7,     3,     4,     5,     6,     7,    -1,    -1,
      -1,    -1,    17,    -1,    -1,    -1,    -1,    17,    -1,    -1,
      -1,    23,    -1,    -1,    29,    -1,    -1,    32,    33,    29,
      -1,    -1,    32,    33,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    26,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    59,    60,     0,     3,     4,     5,     6,     7,    17,
      29,    32,    33,    61,    62,    75,    77,    78,    57,   110,
      22,     3,     4,     5,     3,     4,     5,    80,   110,    28,
     110,     8,    54,    79,    26,    27,    63,    76,    30,    23,
     110,    20,    74,    54,    31,    64,     9,    75,    78,    17,
      78,    19,    24,    69,    70,    71,    27,   110,    21,    26,
      77,    65,    69,   110,     8,    27,    66,    67,    68,    74,
      81,     1,     8,     9,    10,    13,    14,    15,    16,    20,
      25,    27,    34,    48,    53,    54,    55,    56,    57,    72,
      82,    93,    96,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,    27,    73,    20,    98,
     110,    85,    87,    20,    78,    99,   100,   101,   110,    97,
      99,    20,    57,    96,   108,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    27,    27,    56,    18,    22,    35,    20,    28,
      74,    83,    27,    98,    82,    97,    21,    21,    27,    78,
      18,    22,    99,    99,    99,    99,    99,    99,    99,    99,
      99,    99,    99,    99,    99,    99,    99,    99,    99,   110,
      99,    99,    94,    84,    81,    82,    86,    14,    27,    96,
      21,   110,    99,    23,    95,    99,    82,     9,    12,    91,
      82,    98,    88,    23,    21,    26,    92,    27,    97,    95,
      82,    27,    89,    97,    90,    21,    82
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 6:
#line 274 "mex_tab.y"
    { /* nothing */ ;}
    break;

  case 7:
#line 278 "mex_tab.y"
    { (yyval.attrdesc)=curfn=function_begin((yyvsp[(1) - (2)].typedesc), (yyvsp[(2) - (2)].id)); ;}
    break;

  case 8:
#line 280 "mex_tab.y"
    { (yyval.size)=offset; scope_open(); ;}
    break;

  case 9:
#line 282 "mex_tab.y"
    { function_args((yyvsp[(3) - (7)].attrdesc), (yyvsp[(6) - (7)].arg)); ;}
    break;

  case 10:
#line 284 "mex_tab.y"
    { VMADDR end_quad=this_quad;
                                  scope_close();
                                  function_end((yyvsp[(3) - (9)].attrdesc), (yyvsp[(9) - (9)].size), end_quad);
                                  offset=(yyvsp[(5) - (9)].size);
                                ;}
    break;

  case 11:
#line 292 "mex_tab.y"
    { (yyval.size)=TRUE; ;}
    break;

  case 12:
#line 294 "mex_tab.y"
    { (yyval.size)=FALSE; ;}
    break;

  case 13:
#line 298 "mex_tab.y"
    { (yyval.size)=offset; GenFuncStartQuad(curfn); ;}
    break;

  case 14:
#line 300 "mex_tab.y"
    {
                                  /* Reset the value of the offset pointer  *
                                   * for the local activation record.       */

                                  offset=(yyvsp[(2) - (5)].size);
                                ;}
    break;

  case 15:
#line 309 "mex_tab.y"
    { (yyval.arg)=NULL; ;}
    break;

  case 16:
#line 311 "mex_tab.y"
    { (yyval.arg)=declare_ellipsis(); ;}
    break;

  case 17:
#line 313 "mex_tab.y"
    { if ((yyvsp[(1) - (3)].arg)) (yyvsp[(1) - (3)].arg)->next=(yyvsp[(3) - (3)].arg);
                                  (yyval.arg)=(yyvsp[(1) - (3)].arg);
                                ;}
    break;

  case 18:
#line 317 "mex_tab.y"
    { if ((yyvsp[(1) - (1)].arg)) (yyvsp[(1) - (1)].arg)->next=NULL;
                                  (yyval.arg)=(yyvsp[(1) - (1)].arg);
                                ;}
    break;

  case 19:
#line 323 "mex_tab.y"
    { (yyval.arg)=smalloc(sizeof(FUNCARGS));
                                  (yyval.arg)->type=(yyvsp[(2) - (3)].typedesc); (yyval.arg)->name=sstrdup((yyvsp[(3) - (3)].id));
                                  (yyval.arg)->next=NULL;
                                  (yyval.arg)->ref=(yyvsp[(1) - (3)].opt).boolval;
                                ;}
    break;

  case 20:
#line 331 "mex_tab.y"
    { (yyval.opt).boolval=FALSE; ;}
    break;

  case 21:
#line 333 "mex_tab.y"
    { (yyval.opt).boolval=TRUE; ;}
    break;

  case 22:
#line 338 "mex_tab.y"
    { scope_open(); (yyval.size)=offset; ;}
    break;

  case 23:
#line 340 "mex_tab.y"
    {
                                  /* Reset the value of the offset pointer  *
                                   * for the local activation record.       */

                                  offset=(yyvsp[(2) - (5)].size);
                                  scope_close();
                                ;}
    break;

  case 26:
#line 355 "mex_tab.y"
    { declare_vars((yyvsp[(1) - (3)].typedesc),(yyvsp[(2) - (3)].attrdesc)); ;}
    break;

  case 27:
#line 357 "mex_tab.y"
    { (yyval.typedesc)=define_struct_id((yyvsp[(2) - (3)].id)); ;}
    break;

  case 28:
#line 359 "mex_tab.y"
    { define_struct_body((yyvsp[(4) - (7)].typedesc)); ;}
    break;

  case 29:
#line 363 "mex_tab.y"
    { (yyval.typedesc)=(yyvsp[(1) - (2)].typedesc); /* default action */ ;}
    break;

  case 30:
#line 367 "mex_tab.y"
    { (yyval.typedesc)=&UnsignedByteType; ;}
    break;

  case 31:
#line 369 "mex_tab.y"
    { (yyval.typedesc)=&WordType; ;}
    break;

  case 32:
#line 371 "mex_tab.y"
    { (yyval.typedesc)=&DwordType; ;}
    break;

  case 33:
#line 373 "mex_tab.y"
    { (yyval.typedesc)=&ByteType; ;}
    break;

  case 34:
#line 375 "mex_tab.y"
    { (yyval.typedesc)=&WordType; ;}
    break;

  case 35:
#line 377 "mex_tab.y"
    { (yyval.typedesc)=&DwordType; ;}
    break;

  case 36:
#line 379 "mex_tab.y"
    { (yyval.typedesc)=&UnsignedByteType; ;}
    break;

  case 37:
#line 381 "mex_tab.y"
    { (yyval.typedesc)=&UnsignedWordType; ;}
    break;

  case 38:
#line 383 "mex_tab.y"
    { (yyval.typedesc)=&UnsignedDwordType; ;}
    break;

  case 39:
#line 385 "mex_tab.y"
    { (yyval.typedesc)=&VoidType; ;}
    break;

  case 40:
#line 387 "mex_tab.y"
    { (yyval.typedesc)=&StringType; ;}
    break;

  case 41:
#line 389 "mex_tab.y"
    { (yyval.typedesc)=array_descriptor(&(yyvsp[(3) - (6)].range),(yyvsp[(6) - (6)].typedesc)); ;}
    break;

  case 42:
#line 391 "mex_tab.y"
    { (yyval.typedesc)=declare_struct((yyvsp[(2) - (2)].id)); ;}
    break;

  case 43:
#line 396 "mex_tab.y"
    {
                                  (yyval.range).lo=(yyvsp[(1) - (3)].constant).val;
                                  (yyval.range).hi=(yyvsp[(3) - (3)].constant).val;

                                  if ((yyval.range).hi < (yyval.range).lo ||
                                      (yyval.range).hi > 0x7fff ||
                                      (yyval.range).lo > 0x7fff)
                                  {
                                    error(MEXERR_INVALIDRANGE,
                                          (yyval.range).lo,(yyval.range).hi);

                                    (yyval.range).hi=(yyval.range).lo;
                                  }
                                ;}
    break;

  case 44:
#line 411 "mex_tab.y"
    {
                                  (yyval.range).lo = (yyvsp[(1) - (2)].constant).val;
                                  (yyval.range).hi = (VMADDR)-1;

                                  if ((yyval.range).lo > 0x7fff)
                                  {
                                    error(MEXERR_INVALIDRANGE,
                                          (yyval.range).lo, -1);
                                  }
                                ;}
    break;

  case 45:
#line 424 "mex_tab.y"
    { (yyval.attrdesc)=var_list((yyvsp[(3) - (3)].id),(yyvsp[(1) - (3)].attrdesc)); ;}
    break;

  case 46:
#line 426 "mex_tab.y"
    { (yyval.attrdesc)=var_list((yyvsp[(1) - (1)].id),NULL); ;}
    break;

  case 49:
#line 439 "mex_tab.y"
    { ;}
    break;

  case 50:
#line 441 "mex_tab.y"
    { MaybeFreeTemporary((yyvsp[(1) - (2)].dataobj), TRUE); ;}
    break;

  case 51:
#line 443 "mex_tab.y"
    {
                                  warn(MEXERR_WARN_MEANINGLESSEXPR);
                                  MaybeFreeTemporary((yyvsp[(1) - (2)].dataobj), TRUE);
                                ;}
    break;

  case 52:
#line 448 "mex_tab.y"
    { (yyval.patch)=IfTest((yyvsp[(2) - (2)].dataobj)); ;}
    break;

  case 53:
#line 450 "mex_tab.y"
    { IfEnd(& (yyvsp[(3) - (5)].patch), & (yyvsp[(5) - (5)].elsetype)); ;}
    break;

  case 54:
#line 452 "mex_tab.y"
    { ProcessGoto((yyvsp[(2) - (3)].id)); ;}
    break;

  case 55:
#line 454 "mex_tab.y"
    { DeclareLabel((yyvsp[(1) - (2)].id)); ;}
    break;

  case 57:
#line 457 "mex_tab.y"
    { (yyval.whil).top_quad=this_quad; ;}
    break;

  case 58:
#line 459 "mex_tab.y"
    { WhileTest(&(yyvsp[(2) - (3)].whil), (yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 59:
#line 461 "mex_tab.y"
    { GenWhileOut(&(yyvsp[(2) - (5)].whil)); ;}
    break;

  case 60:
#line 463 "mex_tab.y"
    { (yyval.whil).top_quad=this_quad; ;}
    break;

  case 61:
#line 465 "mex_tab.y"
    { GenDoWhileOut(&(yyvsp[(2) - (6)].whil), (yyvsp[(5) - (6)].dataobj)); ;}
    break;

  case 62:
#line 486 "mex_tab.y"
    { (yyval.fr).vmTest = this_quad;
                                  MaybeFreeTemporary((yyvsp[(3) - (4)].dataobj), TRUE);
                                ;}
    break;

  case 63:
#line 490 "mex_tab.y"
    { GenForTest(&(yyvsp[(5) - (7)].fr), (yyvsp[(6) - (7)].dataobj));
                                  (yyvsp[(5) - (7)].fr).vmPost = this_quad;
                                ;}
    break;

  case 64:
#line 494 "mex_tab.y"
    {
                                  GenForJmpTest(&(yyvsp[(5) - (9)].fr));
                                  MaybeFreeTemporary((yyvsp[(9) - (9)].dataobj), TRUE);
                                  (yyvsp[(5) - (9)].fr).vmBody = this_quad;
                                ;}
    break;

  case 65:
#line 500 "mex_tab.y"
    {
                                  GenForJmpPostAndCleanup(&(yyvsp[(5) - (12)].fr));
                                ;}
    break;

  case 66:
#line 504 "mex_tab.y"
    { GenFuncRet((yyvsp[(2) - (3)].dataobj), curfn); ;}
    break;

  case 67:
#line 506 "mex_tab.y"
    { yyerrok; ;}
    break;

  case 68:
#line 508 "mex_tab.y"
    { /* null statement */ ;}
    break;

  case 69:
#line 512 "mex_tab.y"
    { (yyval.elsetype).patchout=NULL;
                                  (yyval.elsetype).else_label=this_quad;
                                ;}
    break;

  case 70:
#line 516 "mex_tab.y"
    { ElseHandler(&(yyval.elsetype)); ;}
    break;

  case 71:
#line 518 "mex_tab.y"
    { (yyval.elsetype)=(yyvsp[(2) - (3)].elsetype); ;}
    break;

  case 72:
#line 522 "mex_tab.y"
    { (yyval.fcall)=StartFuncCall((yyvsp[(1) - (2)].id)); ;}
    break;

  case 73:
#line 524 "mex_tab.y"
    { (yyval.dataobj)=EndFuncCall(&(yyvsp[(3) - (5)].fcall), (yyvsp[(4) - (5)].dataobj)); ;}
    break;

  case 74:
#line 528 "mex_tab.y"
    { (yyval.dataobj)=NULL; ;}
    break;

  case 75:
#line 530 "mex_tab.y"
    {
                                  if (!(yyvsp[(1) - (3)].dataobj))
                                  {
                                    (yyvsp[(1) - (3)].dataobj) = NewDataObj();
                                    (yyvsp[(1) - (3)].dataobj)->type = NULL;
                                    (yyvsp[(1) - (3)].dataobj)->argtype = NULL;
                                  }

                                  (yyvsp[(1) - (3)].dataobj)->next_arg=(yyvsp[(3) - (3)].dataobj);
                                  (yyval.dataobj)=(yyvsp[(1) - (3)].dataobj);
                                ;}
    break;

  case 76:
#line 542 "mex_tab.y"
    {
                                  if ((yyvsp[(1) - (1)].dataobj))
                                    (yyvsp[(1) - (1)].dataobj)->next_arg=NULL;
                                  else
                                  {
                                    (yyvsp[(1) - (1)].dataobj) = NewDataObj();
                                    (yyvsp[(1) - (1)].dataobj)->type = NULL;
                                    (yyvsp[(1) - (1)].dataobj)->argtype = NULL;
                                  }

                                  (yyval.dataobj)=(yyvsp[(1) - (1)].dataobj);
                                ;}
    break;

  case 77:
#line 557 "mex_tab.y"
    { (yyval.dataobj)=(yyvsp[(1) - (1)].dataobj); ;}
    break;

  case 78:
#line 559 "mex_tab.y"
    { (yyval.dataobj)=TypeCast((yyvsp[(4) - (4)].dataobj), (yyvsp[(2) - (4)].typedesc)); ;}
    break;

  case 79:
#line 561 "mex_tab.y"
    { (yyval.dataobj)=EvalSizeof((yyvsp[(3) - (4)].typedesc)); ;}
    break;

  case 80:
#line 563 "mex_tab.y"
    { (yyval.dataobj)=(yyvsp[(1) - (1)].dataobj); ;}
    break;

  case 81:
#line 565 "mex_tab.y"
    { (yyval.dataobj)=(yyvsp[(1) - (1)].dataobj); ;}
    break;

  case 82:
#line 567 "mex_tab.y"
    { (yyval.dataobj)=(yyvsp[(1) - (1)].dataobj); ;}
    break;

  case 83:
#line 572 "mex_tab.y"
    { (yyval.dataobj)=NULL; ;}
    break;

  case 84:
#line 574 "mex_tab.y"
    { (yyval.dataobj)=(yyvsp[(1) - (1)].dataobj); ;}
    break;

  case 85:
#line 579 "mex_tab.y"
    { (yyval.dataobj)=(yyvsp[(2) - (3)].dataobj); ;}
    break;

  case 86:
#line 583 "mex_tab.y"
    {(yyval.dataobj) = (yyvsp[(1) - (1)].dataobj); ;}
    break;

  case 87:
#line 585 "mex_tab.y"
    {(yyval.dataobj) = (yyvsp[(1) - (1)].dataobj); ;}
    break;

  case 88:
#line 589 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_BMULTIPLY,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 89:
#line 591 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_BDIVIDE,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 90:
#line 593 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_BMODULUS,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 91:
#line 595 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_BPLUS,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 92:
#line 597 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_MINUS,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 93:
#line 599 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_LE,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 94:
#line 601 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_LT,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 95:
#line 603 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_SHR,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 96:
#line 605 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_SHL,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 97:
#line 607 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_BAND,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 98:
#line 609 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_BOR,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 99:
#line 611 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_LAND,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 100:
#line 613 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_LOR,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 101:
#line 615 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_EQUAL,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 102:
#line 617 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_NOTEQUAL,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 103:
#line 619 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_GE,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 104:
#line 621 "mex_tab.y"
    { (yyval.dataobj)=EvalBinary((yyvsp[(1) - (3)].dataobj),T_GT,(yyvsp[(3) - (3)].dataobj)); ;}
    break;

  case 105:
#line 623 "mex_tab.y"
    { (yyval.dataobj)=EvalUnary((yyvsp[(2) - (2)].dataobj),T_MINUS); ;}
    break;

  case 106:
#line 628 "mex_tab.y"
    {
                                  /* The binary operator expects
                                   * assignments to be given
                                   * in the order "src -> dest";
                                   * hence the $3 $1 ordering.
                                   */
                                  (yyval.dataobj)=EvalBinary((yyvsp[(3) - (3)].dataobj),T_ASSIGN,(yyvsp[(1) - (3)].dataobj));
                                ;}
    break;

  case 107:
#line 637 "mex_tab.y"
    { (yyval.dataobj)=(yyvsp[(1) - (1)].dataobj); ;}
    break;

  case 108:
#line 641 "mex_tab.y"
    { (yyval.constant) = (yyvsp[(1) - (1)].constant); ;}
    break;

  case 109:
#line 645 "mex_tab.y"
    { (yyval.constant) = (yyvsp[(1) - (1)].constant); ;}
    break;

  case 110:
#line 649 "mex_tab.y"
    { (yyval.constant) = (yyvsp[(1) - (1)].constant); ;}
    break;

  case 111:
#line 653 "mex_tab.y"
    { (yyval.constant) = (yyvsp[(1) - (1)].constant); ;}
    break;

  case 112:
#line 657 "mex_tab.y"
    { (yyval.dataobj)=byteref(&(yyvsp[(1) - (1)].constant)); ;}
    break;

  case 113:
#line 659 "mex_tab.y"
    { (yyval.dataobj)=wordref(&(yyvsp[(1) - (1)].constant)); ;}
    break;

  case 114:
#line 661 "mex_tab.y"
    { (yyval.dataobj)=dwordref(&(yyvsp[(1) - (1)].constant)); ;}
    break;

  case 115:
#line 663 "mex_tab.y"
    { (yyval.dataobj)=stringref(&(yyvsp[(1) - (1)].constant)); ;}
    break;

  case 116:
#line 668 "mex_tab.y"
    { (yyval.constant)=(yyvsp[(1) - (1)].constant); ;}
    break;

  case 117:
#line 670 "mex_tab.y"
    { (yyval.constant)=string_merge((yyvsp[(1) - (2)].constant), (yyvsp[(2) - (2)].constant)); ;}
    break;

  case 118:
#line 673 "mex_tab.y"
    { (yyval.dataobj)=idref((yyvsp[(1) - (1)].id)); ;}
    break;

  case 119:
#line 675 "mex_tab.y"
    { (yyval.dataobj)=ProcessIndex((yyvsp[(1) - (4)].dataobj), (yyvsp[(3) - (4)].dataobj), FALSE); ;}
    break;

  case 120:
#line 677 "mex_tab.y"
    { (yyval.dataobj)=ProcessStruct((yyvsp[(1) - (3)].dataobj), (yyvsp[(3) - (3)].id)); ;}
    break;

  case 121:
#line 681 "mex_tab.y"
    { (yyval.dataobj)=idref((yyvsp[(1) - (1)].id)); ;}
    break;

  case 122:
#line 683 "mex_tab.y"
    { (yyval.dataobj)=ProcessIndex((yyvsp[(1) - (4)].dataobj), (yyvsp[(3) - (4)].dataobj), TRUE); ;}
    break;

  case 123:
#line 685 "mex_tab.y"
    { (yyval.dataobj)=ProcessStruct((yyvsp[(1) - (3)].dataobj), (yyvsp[(3) - (3)].id)); ;}
    break;

  case 124:
#line 689 "mex_tab.y"
    { (yyval.id)=(yyvsp[(1) - (1)].id); ;}
    break;


/* Line 1267 of yacc.c.  */
#line 2393 "mex_tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 692 "mex_tab.y"





