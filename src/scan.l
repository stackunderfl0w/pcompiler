%{
#include <stdio.h>
#include <stdlib.h>
#include "parse.h"

YYSTYPE yylval;
YYLTYPE yylloc;
yypstate* pstate;

extern int handle_indent(char* s);

size_t prev;

//#define PUSH_TOKEN(category, lexeme) printf("%s\n",yytext);
#define PUSH_TOKEN(category, lexeme) do {                           \
    /*printf("TOKEN: (%s)\n",yytext);*/\
    pstate = pstate ? pstate : yypstate_new();                      \
    yylval = lexeme;                                                \
    yylloc.first_line = yylloc.last_line = yylineno;                \
    int status = yypush_parse(pstate, category, &yylval, &yylloc);  \
    if (status != YYPUSH_MORE) {                                    \
        yypstate_delete(pstate);                                    \
        return status;                                              \
    }                                                               \
} while (0)

%}

%option noyywrap
%option yylineno

%%

^[ \t]*\r?\n  /* Blank lines */

"//".*\n								;


^[ \t]+     handle_indent(yytext);

^[^ \t\r\n]+ { handle_indent(yytext); REJECT;}

\r?\n       ; PUSH_TOKEN(NEWLINE, NULL);

[ \t]  /* Whitespace*/


"if"        PUSH_TOKEN(IF, NULL);
"elif"      PUSH_TOKEN(ELIF, NULL);
"else"      PUSH_TOKEN(ELSE, NULL);
"for"       PUSH_TOKEN(FOR, NULL);
"break"     PUSH_TOKEN(BREAK, NULL);
"while"     PUSH_TOKEN(WHILE, NULL);
"return"    PUSH_TOKEN(RETURN, NULL);

"&&"        PUSH_TOKEN(AND, NULL);
"||"        PUSH_TOKEN(OR, NULL);
"!"         PUSH_TOKEN(NOT, NULL);

"&"         PUSH_TOKEN(BIT_AND, NULL);
"~"         PUSH_TOKEN(BIT_NOT, NULL);
"|"         PUSH_TOKEN(BIT_OR, NULL);

":="        PUSH_TOKEN(ASSIGN, NULL);
"=="        PUSH_TOKEN(EQ, NULL);
"!="        PUSH_TOKEN(NEQ, NULL);
">"         PUSH_TOKEN(GT, NULL);
">="        PUSH_TOKEN(GTE, NULL);
"<"         PUSH_TOKEN(LT, NULL);
"<="        PUSH_TOKEN(LTE, NULL);

"+"         PUSH_TOKEN(ADD, NULL);
"-"         PUSH_TOKEN(SUB, NULL);
"*"         PUSH_TOKEN(MUL, NULL);
"/"         PUSH_TOKEN(DIV, NULL);

"%"         PUSH_TOKEN(MOD, NULL);

"++"        PUSH_TOKEN(INC, NULL);
"--"        PUSH_TOKEN(DEC, NULL);

"("         PUSH_TOKEN(LPAR, NULL);
")"         PUSH_TOKEN(RPAR, NULL);
"{"         PUSH_TOKEN(LCBRA, NULL);
"}"         PUSH_TOKEN(RCBRA, NULL);
"["         PUSH_TOKEN(LSBRA, NULL);
"]"         PUSH_TOKEN(RSBRA, NULL);

","         PUSH_TOKEN(COMMA, NULL);
":"         PUSH_TOKEN(COLON, NULL);

"i32"       PUSH_TOKEN(TYPE, strdup(yytext));
"f32"       PUSH_TOKEN(TYPE, strdup(yytext));

[0-9]*"."[0-9]+ PUSH_TOKEN(FLOAT, strdup(yytext));

[0-9]+      PUSH_TOKEN(INTEGER, strdup(yytext));

0[xX][0-9a-fA-F]+ PUSH_TOKEN(INTEGER, strdup(yytext));

[_a-z0-9]+  PUSH_TOKEN(IDENTIFIER,strdup(yytext));

.           printf("Unknown token!(%s) (%d)\n",yytext,*yytext); yyterminate();

<<EOF>>    {
            //PUSH_TOKEN(NEWLINE,NULL);
            handle_indent("x");
            int s = yypush_parse(pstate, 0, NULL, NULL);
            yypstate_delete(pstate);
            return s;;
        }
%%


int indent_stack[128]={0};
int indent_index=0;

int handle_indent(char* s){
    int indent=0;
    //count whitespace
    for(;*s;s++){
        //whitespace
        if(*s==' ')
            indent++;
        else if(*s=='\t')
            indent++;
        else
            break;
    }
    //check for indent
    if(indent>indent_stack[indent_index]){
        //printf("INDENT\n");
        PUSH_TOKEN(INDENT,NULL);
        indent_stack[++indent_index]=indent;
    }
        //check for dedent
    else if(indent<indent_stack[indent_index]){
        while(indent<indent_stack[indent_index]){
            //printf("DEDENT\n");
            PUSH_TOKEN(DEDENT,NULL);
            indent_index--;
        }
    }
    return 0;
}