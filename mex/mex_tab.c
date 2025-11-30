/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
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

#line 95 "mex_tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "mex_tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_T_BYTE = 3,                     /* T_BYTE  */
  YYSYMBOL_T_WORD = 4,                     /* T_WORD  */
  YYSYMBOL_T_DWORD = 5,                    /* T_DWORD  */
  YYSYMBOL_T_STRING = 6,                   /* T_STRING  */
  YYSYMBOL_T_VOID = 7,                     /* T_VOID  */
  YYSYMBOL_T_BEGIN = 8,                    /* T_BEGIN  */
  YYSYMBOL_T_END = 9,                      /* T_END  */
  YYSYMBOL_T_IF = 10,                      /* T_IF  */
  YYSYMBOL_T_THEN = 11,                    /* T_THEN  */
  YYSYMBOL_T_ELSE = 12,                    /* T_ELSE  */
  YYSYMBOL_T_GOTO = 13,                    /* T_GOTO  */
  YYSYMBOL_T_WHILE = 14,                   /* T_WHILE  */
  YYSYMBOL_T_DO = 15,                      /* T_DO  */
  YYSYMBOL_T_FOR = 16,                     /* T_FOR  */
  YYSYMBOL_T_STRUCT = 17,                  /* T_STRUCT  */
  YYSYMBOL_T_DOT = 18,                     /* T_DOT  */
  YYSYMBOL_T_ELLIPSIS = 19,                /* T_ELLIPSIS  */
  YYSYMBOL_T_LPAREN = 20,                  /* T_LPAREN  */
  YYSYMBOL_T_RPAREN = 21,                  /* T_RPAREN  */
  YYSYMBOL_T_LBRACKET = 22,                /* T_LBRACKET  */
  YYSYMBOL_T_RBRACKET = 23,                /* T_RBRACKET  */
  YYSYMBOL_T_REF = 24,                     /* T_REF  */
  YYSYMBOL_T_RETURN = 25,                  /* T_RETURN  */
  YYSYMBOL_T_COMMA = 26,                   /* T_COMMA  */
  YYSYMBOL_T_SEMICOLON = 27,               /* T_SEMICOLON  */
  YYSYMBOL_T_COLON = 28,                   /* T_COLON  */
  YYSYMBOL_T_ARRAY = 29,                   /* T_ARRAY  */
  YYSYMBOL_T_RANGE = 30,                   /* T_RANGE  */
  YYSYMBOL_T_OF = 31,                      /* T_OF  */
  YYSYMBOL_T_UNSIGNED = 32,                /* T_UNSIGNED  */
  YYSYMBOL_T_SIGNED = 33,                  /* T_SIGNED  */
  YYSYMBOL_T_SIZEOF = 34,                  /* T_SIZEOF  */
  YYSYMBOL_T_ASSIGN = 35,                  /* T_ASSIGN  */
  YYSYMBOL_T_LAND = 36,                    /* T_LAND  */
  YYSYMBOL_T_LOR = 37,                     /* T_LOR  */
  YYSYMBOL_T_NOTEQUAL = 38,                /* T_NOTEQUAL  */
  YYSYMBOL_T_EQUAL = 39,                   /* T_EQUAL  */
  YYSYMBOL_T_LE = 40,                      /* T_LE  */
  YYSYMBOL_T_LT = 41,                      /* T_LT  */
  YYSYMBOL_T_GE = 42,                      /* T_GE  */
  YYSYMBOL_T_GT = 43,                      /* T_GT  */
  YYSYMBOL_T_SHL = 44,                     /* T_SHL  */
  YYSYMBOL_T_SHR = 45,                     /* T_SHR  */
  YYSYMBOL_T_BAND = 46,                    /* T_BAND  */
  YYSYMBOL_T_BOR = 47,                     /* T_BOR  */
  YYSYMBOL_T_BPLUS = 48,                   /* T_BPLUS  */
  YYSYMBOL_T_MINUS = 49,                   /* T_MINUS  */
  YYSYMBOL_T_BMULTIPLY = 50,               /* T_BMULTIPLY  */
  YYSYMBOL_T_BDIVIDE = 51,                 /* T_BDIVIDE  */
  YYSYMBOL_T_BMODULUS = 52,                /* T_BMODULUS  */
  YYSYMBOL_T_CONSTBYTE = 53,               /* T_CONSTBYTE  */
  YYSYMBOL_T_CONSTWORD = 54,               /* T_CONSTWORD  */
  YYSYMBOL_T_CONSTDWORD = 55,              /* T_CONSTDWORD  */
  YYSYMBOL_T_CONSTSTRING = 56,             /* T_CONSTSTRING  */
  YYSYMBOL_T_ID = 57,                      /* T_ID  */
  YYSYMBOL_YYACCEPT = 58,                  /* $accept  */
  YYSYMBOL_program = 59,                   /* program  */
  YYSYMBOL_top_list = 60,                  /* top_list  */
  YYSYMBOL_func_or_decl = 61,              /* func_or_decl  */
  YYSYMBOL_function = 62,                  /* function  */
  YYSYMBOL_63_1 = 63,                      /* @1  */
  YYSYMBOL_64_2 = 64,                      /* @2  */
  YYSYMBOL_65_3 = 65,                      /* $@3  */
  YYSYMBOL_trailing_part = 66,             /* trailing_part  */
  YYSYMBOL_function_block = 67,            /* function_block  */
  YYSYMBOL_68_4 = 68,                      /* @4  */
  YYSYMBOL_arg_list = 69,                  /* arg_list  */
  YYSYMBOL_argument = 70,                  /* argument  */
  YYSYMBOL_opt_ref = 71,                   /* opt_ref  */
  YYSYMBOL_block = 72,                     /* block  */
  YYSYMBOL_73_5 = 73,                      /* @5  */
  YYSYMBOL_declarator_list = 74,           /* declarator_list  */
  YYSYMBOL_declaration = 75,               /* declaration  */
  YYSYMBOL_76_6 = 76,                      /* @6  */
  YYSYMBOL_typename = 77,                  /* typename  */
  YYSYMBOL_typedefn = 78,                  /* typedefn  */
  YYSYMBOL_range = 79,                     /* range  */
  YYSYMBOL_id_list = 80,                   /* id_list  */
  YYSYMBOL_statement_list = 81,            /* statement_list  */
  YYSYMBOL_statement = 82,                 /* statement  */
  YYSYMBOL_83_7 = 83,                      /* @7  */
  YYSYMBOL_84_8 = 84,                      /* $@8  */
  YYSYMBOL_85_9 = 85,                      /* @9  */
  YYSYMBOL_86_10 = 86,                     /* $@10  */
  YYSYMBOL_87_11 = 87,                     /* @11  */
  YYSYMBOL_88_12 = 88,                     /* @12  */
  YYSYMBOL_89_13 = 89,                     /* $@13  */
  YYSYMBOL_90_14 = 90,                     /* $@14  */
  YYSYMBOL_else_part = 91,                 /* else_part  */
  YYSYMBOL_92_15 = 92,                     /* @15  */
  YYSYMBOL_function_call = 93,             /* function_call  */
  YYSYMBOL_94_16 = 94,                     /* @16  */
  YYSYMBOL_expr_list = 95,                 /* expr_list  */
  YYSYMBOL_primary = 96,                   /* primary  */
  YYSYMBOL_opt_expr = 97,                  /* opt_expr  */
  YYSYMBOL_paren_expr = 98,                /* paren_expr  */
  YYSYMBOL_expr = 99,                      /* expr  */
  YYSYMBOL_useless_expr = 100,             /* useless_expr  */
  YYSYMBOL_useful_expr = 101,              /* useful_expr  */
  YYSYMBOL_const_byte_p = 102,             /* const_byte_p  */
  YYSYMBOL_const_word_p = 103,             /* const_word_p  */
  YYSYMBOL_const_dword_p = 104,            /* const_dword_p  */
  YYSYMBOL_const_string_p = 105,           /* const_string_p  */
  YYSYMBOL_literal = 106,                  /* literal  */
  YYSYMBOL_const_string = 107,             /* const_string  */
  YYSYMBOL_ident = 108,                    /* ident  */
  YYSYMBOL_lval_ident = 109,               /* lval_ident  */
  YYSYMBOL_id = 110                        /* id  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   518

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  58
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  53
/* YYNRULES -- Number of rules.  */
#define YYNRULES  124
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  217

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   312


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
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
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
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

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "T_BYTE", "T_WORD",
  "T_DWORD", "T_STRING", "T_VOID", "T_BEGIN", "T_END", "T_IF", "T_THEN",
  "T_ELSE", "T_GOTO", "T_WHILE", "T_DO", "T_FOR", "T_STRUCT", "T_DOT",
  "T_ELLIPSIS", "T_LPAREN", "T_RPAREN", "T_LBRACKET", "T_RBRACKET",
  "T_REF", "T_RETURN", "T_COMMA", "T_SEMICOLON", "T_COLON", "T_ARRAY",
  "T_RANGE", "T_OF", "T_UNSIGNED", "T_SIGNED", "T_SIZEOF", "T_ASSIGN",
  "T_LAND", "T_LOR", "T_NOTEQUAL", "T_EQUAL", "T_LE", "T_LT", "T_GE",
  "T_GT", "T_SHL", "T_SHR", "T_BAND", "T_BOR", "T_BPLUS", "T_MINUS",
  "T_BMULTIPLY", "T_BDIVIDE", "T_BMODULUS", "T_CONSTBYTE", "T_CONSTWORD",
  "T_CONSTDWORD", "T_CONSTSTRING", "T_ID", "$accept", "program",
  "top_list", "func_or_decl", "function", "@1", "@2", "$@3",
  "trailing_part", "function_block", "@4", "arg_list", "argument",
  "opt_ref", "block", "@5", "declarator_list", "declaration", "@6",
  "typename", "typedefn", "range", "id_list", "statement_list",
  "statement", "@7", "$@8", "@9", "$@10", "@11", "@12", "$@13", "$@14",
  "else_part", "@15", "function_call", "@16", "expr_list", "primary",
  "opt_expr", "paren_expr", "expr", "useless_expr", "useful_expr",
  "const_byte_p", "const_word_p", "const_dword_p", "const_string_p",
  "literal", "const_string", "ident", "lval_ident", "id", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-111)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-125)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -111,    15,   361,  -111,  -111,  -111,  -111,  -111,  -111,   -22,
      17,    59,    66,  -111,  -111,  -111,   -22,   -15,  -111,    36,
      -8,  -111,  -111,  -111,  -111,  -111,  -111,     0,  -111,  -111,
    -111,  -111,    20,    30,   -22,  -111,    35,  -111,    11,    41,
    -111,  -111,   307,  -111,   366,     9,    40,  -111,    46,   -22,
    -111,  -111,  -111,    54,    52,   366,  -111,  -111,  -111,     9,
     -22,     4,  -111,  -111,  -111,  -111,  -111,  -111,  -111,   361,
     175,    65,  -111,  -111,    61,   -22,  -111,  -111,    73,   289,
       3,  -111,    74,    34,  -111,  -111,  -111,  -111,    -6,  -111,
    -111,  -111,  -111,  -111,   438,    68,    70,  -111,  -111,  -111,
    -111,  -111,    43,    14,    67,    -4,  -111,  -111,     3,  -111,
      77,    61,   248,     3,    79,   311,  -111,  -111,    86,    80,
     438,   366,    89,  -111,    16,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,  -111,  -111,  -111,   -22,     3,     3,  -111,  -111,
     361,   248,  -111,  -111,    87,    83,    34,  -111,  -111,    92,
     -22,     3,   453,   453,   466,   466,    81,    81,    81,    81,
     -31,   -31,   -31,   -31,    33,    33,  -111,  -111,  -111,    85,
     364,   438,     3,   248,   232,   104,   248,    61,  -111,  -111,
    -111,  -111,   394,    88,   113,   421,  -111,  -111,  -111,  -111,
    -111,   108,     3,  -111,  -111,     3,   248,  -111,   111,  -111,
    -111,  -111,     3,  -111,   118,   248,  -111
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
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
       0,     0,    99,   100,   102,   101,    93,    94,   103,   104,
      96,    95,    97,    98,    91,    92,    88,    89,    90,   120,
       0,   106,    74,     0,     0,    69,     0,     0,    62,    78,
      79,   120,     0,   119,     0,    76,    56,    23,    70,    53,
      59,     0,    83,   119,    73,    74,     0,    61,     0,    75,
      71,    63,    83,    64,     0,     0,    65
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

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,     2,    13,    14,    36,    45,    61,    66,    67,
      68,    53,    54,    55,    89,   107,    42,    47,    37,    16,
      48,    33,    27,    70,    90,   151,   183,   111,   186,   112,
     202,   212,   214,   199,   206,    91,   182,   194,    92,   119,
      93,    94,   116,   117,    97,    98,    99,   100,   101,   102,
     103,   104,   118
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      19,    95,   154,    17,   123,    96,   124,    28,    30,   155,
      69,   109,    64,    29,  -124,     3,   148,   137,   138,   139,
     140,   141,  -124,    79,   149,    40,    34,    35,    51,  -121,
     -15,    65,   145,    52,   160,    18,   146,    82,   161,    20,
      57,   185,    18,    95,    31,    50,    32,    96,   153,   150,
      38,    63,    83,    39,    79,    41,    84,    85,    86,    87,
      88,   105,    21,    22,    23,    43,   110,    56,    82,    24,
      25,    26,    44,   196,    29,    58,   200,   189,    59,   124,
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
       0,     0,     0,     0,    83,     0,     0,     0,    84,    85,
      86,    87,    88,    71,   120,     0,     0,   195,     0,     0,
      72,   197,    74,     0,   120,    75,    76,    77,    78,    71,
       0,     0,    79,     0,     0,     0,    72,    80,    74,    81,
       0,    75,    76,    77,    78,     0,    82,     0,    79,     0,
       0,     0,     0,    80,     0,    81,     0,     0,     0,     0,
       0,    83,    82,     0,     0,    84,    85,    86,    87,    88,
       0,     0,     4,     5,     6,     7,     8,    83,     0,     0,
       0,    84,    85,    86,    87,    88,    49,     0,     0,    79,
       4,     5,     6,     7,     8,     0,    46,     0,    10,     0,
       0,    11,    12,    82,     9,     0,     0,     0,     0,     0,
       0,     0,   157,     0,     0,     0,    10,     0,    83,    11,
      12,     0,    84,    85,    86,    87,    88,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,     4,     5,     6,     7,     8,     4,
       5,     6,     7,     8,     0,     0,     0,     0,     9,     0,
       0,     0,     0,    49,     0,     0,     0,   193,     0,     0,
      10,     0,     0,    11,    12,    10,     0,     0,    11,    12,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   203,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   205,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141
};

static const yytype_int16 yycheck[] =
{
       9,    70,   112,     2,    83,    70,    83,    16,    17,   113,
      68,    74,     8,    28,    20,     0,    20,    48,    49,    50,
      51,    52,    28,    20,    28,    34,    26,    27,    19,    35,
      21,    27,    18,    24,    18,    57,    22,    34,    22,    22,
      49,   151,    57,   112,     8,    44,    54,   112,   111,   107,
      30,    60,    49,    23,    20,    20,    53,    54,    55,    56,
      57,    70,     3,     4,     5,    54,    75,    27,    34,     3,
       4,     5,    31,   183,    28,    21,   186,   156,    26,   156,
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
      -1,    -1,    -1,    -1,    49,    -1,    -1,    -1,    53,    54,
      55,    56,    57,     1,   202,    -1,    -1,   205,    -1,    -1,
       8,     9,    10,    -1,   212,    13,    14,    15,    16,     1,
      -1,    -1,    20,    -1,    -1,    -1,     8,    25,    10,    27,
      -1,    13,    14,    15,    16,    -1,    34,    -1,    20,    -1,
      -1,    -1,    -1,    25,    -1,    27,    -1,    -1,    -1,    -1,
      -1,    49,    34,    -1,    -1,    53,    54,    55,    56,    57,
      -1,    -1,     3,     4,     5,     6,     7,    49,    -1,    -1,
      -1,    53,    54,    55,    56,    57,    17,    -1,    -1,    20,
       3,     4,     5,     6,     7,    -1,     9,    -1,    29,    -1,
      -1,    32,    33,    34,    17,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    -1,    -1,    -1,    29,    -1,    49,    32,
      33,    -1,    53,    54,    55,    56,    57,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,     3,     4,     5,     6,     7,     3,
       4,     5,     6,     7,    -1,    -1,    -1,    -1,    17,    -1,
      -1,    -1,    -1,    17,    -1,    -1,    -1,    23,    -1,    -1,
      29,    -1,    -1,    32,    33,    29,    -1,    -1,    32,    33,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    23,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    26,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    59,    60,     0,     3,     4,     5,     6,     7,    17,
      29,    32,    33,    61,    62,    75,    77,    78,    57,   110,
      22,     3,     4,     5,     3,     4,     5,    80,   110,    28,
     110,     8,    54,    79,    26,    27,    63,    76,    30,    23,
     110,    20,    74,    54,    31,    64,     9,    75,    78,    17,
      78,    19,    24,    69,    70,    71,    27,   110,    21,    26,
      77,    65,    69,   110,     8,    27,    66,    67,    68,    74,
      81,     1,     8,     9,    10,    13,    14,    15,    16,    20,
      25,    27,    34,    49,    53,    54,    55,    56,    57,    72,
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

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
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

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
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


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
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
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 6: /* func_or_decl: declaration  */
#line 274 "mex_tab.y"
                                { /* nothing */ }
#line 1412 "mex_tab.c"
    break;

  case 7: /* @1: %empty  */
#line 278 "mex_tab.y"
                                { (yyval.attrdesc)=curfn=function_begin((yyvsp[-1].typedesc), (yyvsp[0].id)); }
#line 1418 "mex_tab.c"
    break;

  case 8: /* @2: %empty  */
#line 280 "mex_tab.y"
                                { (yyval.size)=offset; scope_open(); }
#line 1424 "mex_tab.c"
    break;

  case 9: /* $@3: %empty  */
#line 282 "mex_tab.y"
                                { function_args((yyvsp[-4].attrdesc), (yyvsp[-1].arg)); }
#line 1430 "mex_tab.c"
    break;

  case 10: /* function: typedefn id @1 T_LPAREN @2 arg_list T_RPAREN $@3 trailing_part  */
#line 284 "mex_tab.y"
                                { VMADDR end_quad=this_quad;
                                  scope_close();
                                  function_end((yyvsp[-6].attrdesc), (yyvsp[0].size), end_quad);
                                  offset=(yyvsp[-4].size);
                                }
#line 1440 "mex_tab.c"
    break;

  case 11: /* trailing_part: function_block  */
#line 292 "mex_tab.y"
                                { (yyval.size)=TRUE; }
#line 1446 "mex_tab.c"
    break;

  case 12: /* trailing_part: T_SEMICOLON  */
#line 294 "mex_tab.y"
                                { (yyval.size)=FALSE; }
#line 1452 "mex_tab.c"
    break;

  case 13: /* @4: %empty  */
#line 298 "mex_tab.y"
                                { (yyval.size)=offset; GenFuncStartQuad(curfn); }
#line 1458 "mex_tab.c"
    break;

  case 14: /* function_block: T_BEGIN @4 declarator_list statement_list T_END  */
#line 300 "mex_tab.y"
                                {
                                  /* Reset the value of the offset pointer  *
                                   * for the local activation record.       */

                                  offset=(yyvsp[-3].size);
                                }
#line 1469 "mex_tab.c"
    break;

  case 15: /* arg_list: %empty  */
#line 309 "mex_tab.y"
                                { (yyval.arg)=NULL; }
#line 1475 "mex_tab.c"
    break;

  case 16: /* arg_list: T_ELLIPSIS  */
#line 311 "mex_tab.y"
                                { (yyval.arg)=declare_ellipsis(); }
#line 1481 "mex_tab.c"
    break;

  case 17: /* arg_list: argument T_COMMA arg_list  */
#line 313 "mex_tab.y"
                                { if ((yyvsp[-2].arg)) (yyvsp[-2].arg)->next=(yyvsp[0].arg);
                                  (yyval.arg)=(yyvsp[-2].arg);
                                }
#line 1489 "mex_tab.c"
    break;

  case 18: /* arg_list: argument  */
#line 317 "mex_tab.y"
                                { if ((yyvsp[0].arg)) (yyvsp[0].arg)->next=NULL;
                                  (yyval.arg)=(yyvsp[0].arg);
                                }
#line 1497 "mex_tab.c"
    break;

  case 19: /* argument: opt_ref typename id  */
#line 323 "mex_tab.y"
                                { (yyval.arg)=smalloc(sizeof(FUNCARGS));
                                  (yyval.arg)->type=(yyvsp[-1].typedesc); (yyval.arg)->name=sstrdup((yyvsp[0].id));
                                  (yyval.arg)->next=NULL;
                                  (yyval.arg)->ref=(yyvsp[-2].opt).boolval;
                                }
#line 1507 "mex_tab.c"
    break;

  case 20: /* opt_ref: %empty  */
#line 331 "mex_tab.y"
                                { (yyval.opt).boolval=FALSE; }
#line 1513 "mex_tab.c"
    break;

  case 21: /* opt_ref: T_REF  */
#line 333 "mex_tab.y"
                                { (yyval.opt).boolval=TRUE; }
#line 1519 "mex_tab.c"
    break;

  case 22: /* @5: %empty  */
#line 338 "mex_tab.y"
                                { scope_open(); (yyval.size)=offset; }
#line 1525 "mex_tab.c"
    break;

  case 23: /* block: T_BEGIN @5 declarator_list statement_list T_END  */
#line 340 "mex_tab.y"
                                {
                                  /* Reset the value of the offset pointer  *
                                   * for the local activation record.       */

                                  offset=(yyvsp[-3].size);
                                  scope_close();
                                }
#line 1537 "mex_tab.c"
    break;

  case 26: /* declaration: typename id_list T_SEMICOLON  */
#line 355 "mex_tab.y"
                                { declare_vars((yyvsp[-2].typedesc),(yyvsp[-1].attrdesc)); }
#line 1543 "mex_tab.c"
    break;

  case 27: /* @6: %empty  */
#line 357 "mex_tab.y"
                                { (yyval.typedesc)=define_struct_id((yyvsp[-1].id)); }
#line 1549 "mex_tab.c"
    break;

  case 28: /* declaration: T_STRUCT id T_BEGIN @6 declarator_list T_END T_SEMICOLON  */
#line 359 "mex_tab.y"
                                { define_struct_body((yyvsp[-3].typedesc)); }
#line 1555 "mex_tab.c"
    break;

  case 29: /* typename: typedefn T_COLON  */
#line 363 "mex_tab.y"
                                { (yyval.typedesc)=(yyvsp[-1].typedesc); /* default action */ }
#line 1561 "mex_tab.c"
    break;

  case 30: /* typedefn: T_BYTE  */
#line 367 "mex_tab.y"
                                { (yyval.typedesc)=&UnsignedByteType; }
#line 1567 "mex_tab.c"
    break;

  case 31: /* typedefn: T_WORD  */
#line 369 "mex_tab.y"
                                { (yyval.typedesc)=&WordType; }
#line 1573 "mex_tab.c"
    break;

  case 32: /* typedefn: T_DWORD  */
#line 371 "mex_tab.y"
                                { (yyval.typedesc)=&DwordType; }
#line 1579 "mex_tab.c"
    break;

  case 33: /* typedefn: T_SIGNED T_BYTE  */
#line 373 "mex_tab.y"
                                { (yyval.typedesc)=&ByteType; }
#line 1585 "mex_tab.c"
    break;

  case 34: /* typedefn: T_SIGNED T_WORD  */
#line 375 "mex_tab.y"
                                { (yyval.typedesc)=&WordType; }
#line 1591 "mex_tab.c"
    break;

  case 35: /* typedefn: T_SIGNED T_DWORD  */
#line 377 "mex_tab.y"
                                { (yyval.typedesc)=&DwordType; }
#line 1597 "mex_tab.c"
    break;

  case 36: /* typedefn: T_UNSIGNED T_BYTE  */
#line 379 "mex_tab.y"
                                { (yyval.typedesc)=&UnsignedByteType; }
#line 1603 "mex_tab.c"
    break;

  case 37: /* typedefn: T_UNSIGNED T_WORD  */
#line 381 "mex_tab.y"
                                { (yyval.typedesc)=&UnsignedWordType; }
#line 1609 "mex_tab.c"
    break;

  case 38: /* typedefn: T_UNSIGNED T_DWORD  */
#line 383 "mex_tab.y"
                                { (yyval.typedesc)=&UnsignedDwordType; }
#line 1615 "mex_tab.c"
    break;

  case 39: /* typedefn: T_VOID  */
#line 385 "mex_tab.y"
                                { (yyval.typedesc)=&VoidType; }
#line 1621 "mex_tab.c"
    break;

  case 40: /* typedefn: T_STRING  */
#line 387 "mex_tab.y"
                                { (yyval.typedesc)=&StringType; }
#line 1627 "mex_tab.c"
    break;

  case 41: /* typedefn: T_ARRAY T_LBRACKET range T_RBRACKET T_OF typedefn  */
#line 389 "mex_tab.y"
                                { (yyval.typedesc)=array_descriptor(&(yyvsp[-3].range),(yyvsp[0].typedesc)); }
#line 1633 "mex_tab.c"
    break;

  case 42: /* typedefn: T_STRUCT id  */
#line 391 "mex_tab.y"
                                { (yyval.typedesc)=declare_struct((yyvsp[0].id)); }
#line 1639 "mex_tab.c"
    break;

  case 43: /* range: T_CONSTWORD T_RANGE T_CONSTWORD  */
#line 396 "mex_tab.y"
                                {
                                  (yyval.range).lo=(yyvsp[-2].constant).val;
                                  (yyval.range).hi=(yyvsp[0].constant).val;

                                  if ((yyval.range).hi < (yyval.range).lo ||
                                      (yyval.range).hi > 0x7fff ||
                                      (yyval.range).lo > 0x7fff)
                                  {
                                    error(MEXERR_INVALIDRANGE,
                                          (yyval.range).lo,(yyval.range).hi);

                                    (yyval.range).hi=(yyval.range).lo;
                                  }
                                }
#line 1658 "mex_tab.c"
    break;

  case 44: /* range: T_CONSTWORD T_RANGE  */
#line 411 "mex_tab.y"
                                {
                                  (yyval.range).lo = (yyvsp[-1].constant).val;
                                  (yyval.range).hi = (VMADDR)-1;

                                  if ((yyval.range).lo > 0x7fff)
                                  {
                                    error(MEXERR_INVALIDRANGE,
                                          (yyval.range).lo, -1);
                                  }
                                }
#line 1673 "mex_tab.c"
    break;

  case 45: /* id_list: id_list T_COMMA id  */
#line 424 "mex_tab.y"
                                { (yyval.attrdesc)=var_list((yyvsp[0].id),(yyvsp[-2].attrdesc)); }
#line 1679 "mex_tab.c"
    break;

  case 46: /* id_list: id  */
#line 426 "mex_tab.y"
                                { (yyval.attrdesc)=var_list((yyvsp[0].id),NULL); }
#line 1685 "mex_tab.c"
    break;

  case 49: /* statement: block  */
#line 439 "mex_tab.y"
                                { }
#line 1691 "mex_tab.c"
    break;

  case 50: /* statement: useful_expr T_SEMICOLON  */
#line 441 "mex_tab.y"
                                { MaybeFreeTemporary((yyvsp[-1].dataobj), TRUE); }
#line 1697 "mex_tab.c"
    break;

  case 51: /* statement: useless_expr T_SEMICOLON  */
#line 443 "mex_tab.y"
                                {
                                  warn(MEXERR_WARN_MEANINGLESSEXPR);
                                  MaybeFreeTemporary((yyvsp[-1].dataobj), TRUE);
                                }
#line 1706 "mex_tab.c"
    break;

  case 52: /* @7: %empty  */
#line 448 "mex_tab.y"
                                { (yyval.patch)=IfTest((yyvsp[0].dataobj)); }
#line 1712 "mex_tab.c"
    break;

  case 53: /* statement: T_IF paren_expr @7 statement else_part  */
#line 450 "mex_tab.y"
                                { IfEnd(& (yyvsp[-2].patch), & (yyvsp[0].elsetype)); }
#line 1718 "mex_tab.c"
    break;

  case 54: /* statement: T_GOTO id T_SEMICOLON  */
#line 452 "mex_tab.y"
                                { ProcessGoto((yyvsp[-1].id)); }
#line 1724 "mex_tab.c"
    break;

  case 55: /* $@8: %empty  */
#line 454 "mex_tab.y"
                                { DeclareLabel((yyvsp[-1].id)); }
#line 1730 "mex_tab.c"
    break;

  case 57: /* @9: %empty  */
#line 457 "mex_tab.y"
                                { (yyval.whil).top_quad=this_quad; }
#line 1736 "mex_tab.c"
    break;

  case 58: /* $@10: %empty  */
#line 459 "mex_tab.y"
                                { WhileTest(&(yyvsp[-1].whil), (yyvsp[0].dataobj)); }
#line 1742 "mex_tab.c"
    break;

  case 59: /* statement: T_WHILE @9 paren_expr $@10 statement  */
#line 461 "mex_tab.y"
                                { GenWhileOut(&(yyvsp[-3].whil)); }
#line 1748 "mex_tab.c"
    break;

  case 60: /* @11: %empty  */
#line 463 "mex_tab.y"
                                { (yyval.whil).top_quad=this_quad; }
#line 1754 "mex_tab.c"
    break;

  case 61: /* statement: T_DO @11 statement T_WHILE paren_expr T_SEMICOLON  */
#line 465 "mex_tab.y"
                                { GenDoWhileOut(&(yyvsp[-4].whil), (yyvsp[-1].dataobj)); }
#line 1760 "mex_tab.c"
    break;

  case 62: /* @12: %empty  */
#line 486 "mex_tab.y"
                                { (yyval.fr).vmTest = this_quad;
                                  MaybeFreeTemporary((yyvsp[-1].dataobj), TRUE);
                                }
#line 1768 "mex_tab.c"
    break;

  case 63: /* $@13: %empty  */
#line 490 "mex_tab.y"
                                { GenForTest(&(yyvsp[-2].fr), (yyvsp[-1].dataobj));
                                  (yyvsp[-2].fr).vmPost = this_quad;
                                }
#line 1776 "mex_tab.c"
    break;

  case 64: /* $@14: %empty  */
#line 494 "mex_tab.y"
                                {
                                  GenForJmpTest(&(yyvsp[-4].fr));
                                  MaybeFreeTemporary((yyvsp[0].dataobj), TRUE);
                                  (yyvsp[-4].fr).vmBody = this_quad;
                                }
#line 1786 "mex_tab.c"
    break;

  case 65: /* statement: T_FOR T_LPAREN opt_expr T_SEMICOLON @12 opt_expr T_SEMICOLON $@13 opt_expr $@14 T_RPAREN statement  */
#line 500 "mex_tab.y"
                                {
                                  GenForJmpPostAndCleanup(&(yyvsp[-7].fr));
                                }
#line 1794 "mex_tab.c"
    break;

  case 66: /* statement: T_RETURN opt_expr T_SEMICOLON  */
#line 504 "mex_tab.y"
                                { GenFuncRet((yyvsp[-1].dataobj), curfn); }
#line 1800 "mex_tab.c"
    break;

  case 67: /* statement: error T_SEMICOLON  */
#line 506 "mex_tab.y"
                                { yyerrok; }
#line 1806 "mex_tab.c"
    break;

  case 68: /* statement: T_SEMICOLON  */
#line 508 "mex_tab.y"
                                { /* null statement */ }
#line 1812 "mex_tab.c"
    break;

  case 69: /* else_part: %empty  */
#line 512 "mex_tab.y"
                                { (yyval.elsetype).patchout=NULL;
                                  (yyval.elsetype).else_label=this_quad;
                                }
#line 1820 "mex_tab.c"
    break;

  case 70: /* @15: %empty  */
#line 516 "mex_tab.y"
                                { ElseHandler(&(yyval.elsetype)); }
#line 1826 "mex_tab.c"
    break;

  case 71: /* else_part: T_ELSE @15 statement  */
#line 518 "mex_tab.y"
                                { (yyval.elsetype)=(yyvsp[-1].elsetype); }
#line 1832 "mex_tab.c"
    break;

  case 72: /* @16: %empty  */
#line 522 "mex_tab.y"
                                { (yyval.fcall)=StartFuncCall((yyvsp[-1].id)); }
#line 1838 "mex_tab.c"
    break;

  case 73: /* function_call: id T_LPAREN @16 expr_list T_RPAREN  */
#line 524 "mex_tab.y"
                                { (yyval.dataobj)=EndFuncCall(&(yyvsp[-2].fcall), (yyvsp[-1].dataobj)); }
#line 1844 "mex_tab.c"
    break;

  case 74: /* expr_list: %empty  */
#line 528 "mex_tab.y"
                                { (yyval.dataobj)=NULL; }
#line 1850 "mex_tab.c"
    break;

  case 75: /* expr_list: expr T_COMMA expr_list  */
#line 530 "mex_tab.y"
                                {
                                  if (!(yyvsp[-2].dataobj))
                                  {
                                    (yyvsp[-2].dataobj) = NewDataObj();
                                    (yyvsp[-2].dataobj)->type = NULL;
                                    (yyvsp[-2].dataobj)->argtype = NULL;
                                  }

                                  (yyvsp[-2].dataobj)->next_arg=(yyvsp[0].dataobj);
                                  (yyval.dataobj)=(yyvsp[-2].dataobj);
                                }
#line 1866 "mex_tab.c"
    break;

  case 76: /* expr_list: expr  */
#line 542 "mex_tab.y"
                                {
                                  if ((yyvsp[0].dataobj))
                                    (yyvsp[0].dataobj)->next_arg=NULL;
                                  else
                                  {
                                    (yyvsp[0].dataobj) = NewDataObj();
                                    (yyvsp[0].dataobj)->type = NULL;
                                    (yyvsp[0].dataobj)->argtype = NULL;
                                  }

                                  (yyval.dataobj)=(yyvsp[0].dataobj);
                                }
#line 1883 "mex_tab.c"
    break;

  case 77: /* primary: paren_expr  */
#line 557 "mex_tab.y"
                                { (yyval.dataobj)=(yyvsp[0].dataobj); }
#line 1889 "mex_tab.c"
    break;

  case 78: /* primary: T_LPAREN typedefn T_RPAREN primary  */
#line 559 "mex_tab.y"
                                { (yyval.dataobj)=TypeCast((yyvsp[0].dataobj), (yyvsp[-2].typedesc)); }
#line 1895 "mex_tab.c"
    break;

  case 79: /* primary: T_SIZEOF T_LPAREN typedefn T_RPAREN  */
#line 561 "mex_tab.y"
                                { (yyval.dataobj)=EvalSizeof((yyvsp[-1].typedesc)); }
#line 1901 "mex_tab.c"
    break;

  case 80: /* primary: function_call  */
#line 563 "mex_tab.y"
                                { (yyval.dataobj)=(yyvsp[0].dataobj); }
#line 1907 "mex_tab.c"
    break;

  case 81: /* primary: literal  */
#line 565 "mex_tab.y"
                                { (yyval.dataobj)=(yyvsp[0].dataobj); }
#line 1913 "mex_tab.c"
    break;

  case 82: /* primary: ident  */
#line 567 "mex_tab.y"
                                { (yyval.dataobj)=(yyvsp[0].dataobj); }
#line 1919 "mex_tab.c"
    break;

  case 83: /* opt_expr: %empty  */
#line 572 "mex_tab.y"
                                { (yyval.dataobj)=NULL; }
#line 1925 "mex_tab.c"
    break;

  case 84: /* opt_expr: expr  */
#line 574 "mex_tab.y"
                                { (yyval.dataobj)=(yyvsp[0].dataobj); }
#line 1931 "mex_tab.c"
    break;

  case 85: /* paren_expr: T_LPAREN expr T_RPAREN  */
#line 579 "mex_tab.y"
                                { (yyval.dataobj)=(yyvsp[-1].dataobj); }
#line 1937 "mex_tab.c"
    break;

  case 86: /* expr: useful_expr  */
#line 583 "mex_tab.y"
                                {(yyval.dataobj) = (yyvsp[0].dataobj); }
#line 1943 "mex_tab.c"
    break;

  case 87: /* expr: useless_expr  */
#line 585 "mex_tab.y"
                                {(yyval.dataobj) = (yyvsp[0].dataobj); }
#line 1949 "mex_tab.c"
    break;

  case 88: /* useless_expr: expr T_BMULTIPLY expr  */
#line 589 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_BMULTIPLY,(yyvsp[0].dataobj)); }
#line 1955 "mex_tab.c"
    break;

  case 89: /* useless_expr: expr T_BDIVIDE expr  */
#line 591 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_BDIVIDE,(yyvsp[0].dataobj)); }
#line 1961 "mex_tab.c"
    break;

  case 90: /* useless_expr: expr T_BMODULUS expr  */
#line 593 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_BMODULUS,(yyvsp[0].dataobj)); }
#line 1967 "mex_tab.c"
    break;

  case 91: /* useless_expr: expr T_BPLUS expr  */
#line 595 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_BPLUS,(yyvsp[0].dataobj)); }
#line 1973 "mex_tab.c"
    break;

  case 92: /* useless_expr: expr T_MINUS expr  */
#line 597 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_MINUS,(yyvsp[0].dataobj)); }
#line 1979 "mex_tab.c"
    break;

  case 93: /* useless_expr: expr T_LE expr  */
#line 599 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_LE,(yyvsp[0].dataobj)); }
#line 1985 "mex_tab.c"
    break;

  case 94: /* useless_expr: expr T_LT expr  */
#line 601 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_LT,(yyvsp[0].dataobj)); }
#line 1991 "mex_tab.c"
    break;

  case 95: /* useless_expr: expr T_SHR expr  */
#line 603 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_SHR,(yyvsp[0].dataobj)); }
#line 1997 "mex_tab.c"
    break;

  case 96: /* useless_expr: expr T_SHL expr  */
#line 605 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_SHL,(yyvsp[0].dataobj)); }
#line 2003 "mex_tab.c"
    break;

  case 97: /* useless_expr: expr T_BAND expr  */
#line 607 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_BAND,(yyvsp[0].dataobj)); }
#line 2009 "mex_tab.c"
    break;

  case 98: /* useless_expr: expr T_BOR expr  */
#line 609 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_BOR,(yyvsp[0].dataobj)); }
#line 2015 "mex_tab.c"
    break;

  case 99: /* useless_expr: expr T_LAND expr  */
#line 611 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_LAND,(yyvsp[0].dataobj)); }
#line 2021 "mex_tab.c"
    break;

  case 100: /* useless_expr: expr T_LOR expr  */
#line 613 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_LOR,(yyvsp[0].dataobj)); }
#line 2027 "mex_tab.c"
    break;

  case 101: /* useless_expr: expr T_EQUAL expr  */
#line 615 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_EQUAL,(yyvsp[0].dataobj)); }
#line 2033 "mex_tab.c"
    break;

  case 102: /* useless_expr: expr T_NOTEQUAL expr  */
#line 617 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_NOTEQUAL,(yyvsp[0].dataobj)); }
#line 2039 "mex_tab.c"
    break;

  case 103: /* useless_expr: expr T_GE expr  */
#line 619 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_GE,(yyvsp[0].dataobj)); }
#line 2045 "mex_tab.c"
    break;

  case 104: /* useless_expr: expr T_GT expr  */
#line 621 "mex_tab.y"
                                { (yyval.dataobj)=EvalBinary((yyvsp[-2].dataobj),T_GT,(yyvsp[0].dataobj)); }
#line 2051 "mex_tab.c"
    break;

  case 105: /* useless_expr: T_MINUS primary  */
#line 623 "mex_tab.y"
                                { (yyval.dataobj)=EvalUnary((yyvsp[0].dataobj),T_MINUS); }
#line 2057 "mex_tab.c"
    break;

  case 106: /* useful_expr: lval_ident T_ASSIGN expr  */
#line 628 "mex_tab.y"
                                {
                                  /* The binary operator expects
                                   * assignments to be given
                                   * in the order "src -> dest";
                                   * hence the $3 $1 ordering.
                                   */
                                  (yyval.dataobj)=EvalBinary((yyvsp[0].dataobj),T_ASSIGN,(yyvsp[-2].dataobj));
                                }
#line 2070 "mex_tab.c"
    break;

  case 107: /* useful_expr: primary  */
#line 637 "mex_tab.y"
                                { (yyval.dataobj)=(yyvsp[0].dataobj); }
#line 2076 "mex_tab.c"
    break;

  case 108: /* const_byte_p: T_CONSTBYTE  */
#line 641 "mex_tab.y"
                                { (yyval.constant) = (yyvsp[0].constant); }
#line 2082 "mex_tab.c"
    break;

  case 109: /* const_word_p: T_CONSTWORD  */
#line 645 "mex_tab.y"
                                { (yyval.constant) = (yyvsp[0].constant); }
#line 2088 "mex_tab.c"
    break;

  case 110: /* const_dword_p: T_CONSTDWORD  */
#line 649 "mex_tab.y"
                                { (yyval.constant) = (yyvsp[0].constant); }
#line 2094 "mex_tab.c"
    break;

  case 111: /* const_string_p: const_string  */
#line 653 "mex_tab.y"
                                { (yyval.constant) = (yyvsp[0].constant); }
#line 2100 "mex_tab.c"
    break;

  case 112: /* literal: const_byte_p  */
#line 657 "mex_tab.y"
                                { (yyval.dataobj)=byteref(&(yyvsp[0].constant)); }
#line 2106 "mex_tab.c"
    break;

  case 113: /* literal: const_word_p  */
#line 659 "mex_tab.y"
                                { (yyval.dataobj)=wordref(&(yyvsp[0].constant)); }
#line 2112 "mex_tab.c"
    break;

  case 114: /* literal: const_dword_p  */
#line 661 "mex_tab.y"
                                { (yyval.dataobj)=dwordref(&(yyvsp[0].constant)); }
#line 2118 "mex_tab.c"
    break;

  case 115: /* literal: const_string_p  */
#line 663 "mex_tab.y"
                                { (yyval.dataobj)=stringref(&(yyvsp[0].constant)); }
#line 2124 "mex_tab.c"
    break;

  case 116: /* const_string: T_CONSTSTRING  */
#line 668 "mex_tab.y"
                                { (yyval.constant)=(yyvsp[0].constant); }
#line 2130 "mex_tab.c"
    break;

  case 117: /* const_string: const_string T_CONSTSTRING  */
#line 670 "mex_tab.y"
                                { (yyval.constant)=string_merge((yyvsp[-1].constant), (yyvsp[0].constant)); }
#line 2136 "mex_tab.c"
    break;

  case 118: /* ident: T_ID  */
#line 673 "mex_tab.y"
                                { (yyval.dataobj)=idref((yyvsp[0].id)); }
#line 2142 "mex_tab.c"
    break;

  case 119: /* ident: ident T_LBRACKET expr T_RBRACKET  */
#line 675 "mex_tab.y"
                                { (yyval.dataobj)=ProcessIndex((yyvsp[-3].dataobj), (yyvsp[-1].dataobj), FALSE); }
#line 2148 "mex_tab.c"
    break;

  case 120: /* ident: ident T_DOT id  */
#line 677 "mex_tab.y"
                                { (yyval.dataobj)=ProcessStruct((yyvsp[-2].dataobj), (yyvsp[0].id)); }
#line 2154 "mex_tab.c"
    break;

  case 121: /* lval_ident: T_ID  */
#line 681 "mex_tab.y"
                                { (yyval.dataobj)=idref((yyvsp[0].id)); }
#line 2160 "mex_tab.c"
    break;

  case 122: /* lval_ident: ident T_LBRACKET expr T_RBRACKET  */
#line 683 "mex_tab.y"
                                { (yyval.dataobj)=ProcessIndex((yyvsp[-3].dataobj), (yyvsp[-1].dataobj), TRUE); }
#line 2166 "mex_tab.c"
    break;

  case 123: /* lval_ident: ident T_DOT id  */
#line 685 "mex_tab.y"
                                { (yyval.dataobj)=ProcessStruct((yyvsp[-2].dataobj), (yyvsp[0].id)); }
#line 2172 "mex_tab.c"
    break;

  case 124: /* id: T_ID  */
#line 689 "mex_tab.y"
                                { (yyval.id)=(yyvsp[0].id); }
#line 2178 "mex_tab.c"
    break;


#line 2182 "mex_tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
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

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 692 "mex_tab.y"




