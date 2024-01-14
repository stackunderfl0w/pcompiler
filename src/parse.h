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

#ifndef YY_YY_HOME_PAT_CLIONPROJECTS_COMPILER_SRC_PARSE_H_INCLUDED
# define YY_YY_HOME_PAT_CLIONPROJECTS_COMPILER_SRC_PARSE_H_INCLUDED
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
    IDENTIFIER = 258,              /* IDENTIFIER  */
    TYPE = 259,                    /* TYPE  */
    FLOAT = 260,                   /* FLOAT  */
    INTEGER = 261,                 /* INTEGER  */
    BOOLEAN = 262,                 /* BOOLEAN  */
    INDENT = 263,                  /* INDENT  */
    DEDENT = 264,                  /* DEDENT  */
    NEWLINE = 265,                 /* NEWLINE  */
    IF = 266,                      /* IF  */
    ELIF = 267,                    /* ELIF  */
    ELSE = 268,                    /* ELSE  */
    FOR = 269,                     /* FOR  */
    BREAK = 270,                   /* BREAK  */
    WHILE = 271,                   /* WHILE  */
    RETURN = 272,                  /* RETURN  */
    AND = 273,                     /* AND  */
    OR = 274,                      /* OR  */
    NOT = 275,                     /* NOT  */
    BIT_AND = 276,                 /* BIT_AND  */
    BIT_NOT = 277,                 /* BIT_NOT  */
    BIT_OR = 278,                  /* BIT_OR  */
    MOD = 279,                     /* MOD  */
    INC = 280,                     /* INC  */
    DEC = 281,                     /* DEC  */
    ASSIGN = 282,                  /* ASSIGN  */
    EQ = 283,                      /* EQ  */
    NEQ = 284,                     /* NEQ  */
    GT = 285,                      /* GT  */
    GTE = 286,                     /* GTE  */
    LT = 287,                      /* LT  */
    LTE = 288,                     /* LTE  */
    ADD = 289,                     /* ADD  */
    SUB = 290,                     /* SUB  */
    MUL = 291,                     /* MUL  */
    DIV = 292,                     /* DIV  */
    LPAR = 293,                    /* LPAR  */
    RPAR = 294,                    /* RPAR  */
    LCBRA = 295,                   /* LCBRA  */
    RCBRA = 296,                   /* RCBRA  */
    LSBRA = 297,                   /* LSBRA  */
    RSBRA = 298,                   /* RSBRA  */
    COMMA = 299,                   /* COMMA  */
    COLON = 300                    /* COLON  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef  void*  YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif




#ifndef YYPUSH_MORE_DEFINED
# define YYPUSH_MORE_DEFINED
enum { YYPUSH_MORE = 4 };
#endif

typedef struct yypstate yypstate;


int yypush_parse (yypstate *ps,
                  int pushed_char, YYSTYPE const *pushed_val, YYLTYPE *pushed_loc);

yypstate *yypstate_new (void);
void yypstate_delete (yypstate *ps);


#endif /* !YY_YY_HOME_PAT_CLIONPROJECTS_COMPILER_SRC_PARSE_H_INCLUDED  */
