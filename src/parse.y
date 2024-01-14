%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#define YYDEBUG 1

#include "parse.h"
#include "scope.h"

void yyerror(YYLTYPE* loc, const char* err);

ast_node* target_program = NULL;

extern scope* parse_symbols;

int parse_error = 0;

extern int yylineno;

%}

%locations
%define parse.error verbose


%define api.value.type { void* }

%define api.pure full
%define api.push-pull push


%token IDENTIFIER TYPE
%token FLOAT INTEGER BOOLEAN
%token INDENT DEDENT NEWLINE
%token IF ELIF ELSE FOR BREAK WHILE RETURN
%token AND OR NOT BIT_AND BIT_NOT BIT_OR MOD INC DEC
%token ASSIGN EQ NEQ GT GTE LT LTE
%token ADD SUB MUL DIV
%token LPAR RPAR LCBRA RCBRA LSBRA RSBRA COMMA COLON

%start program

%left OR
%left AND
%left NOT
%left EQ NEQ GT GTE LT LTE MOD
%left ADD SUB
%left MUL DIV

%%
program
    : stmts { $$=target_program = $1; }
    ;

stmts
    :statement { $$=$1; }
    |  statement stmts{
        ast_node* n=$$=$1;
        n->NEXT=$2;
    }
    ;

symbol_decl
    : TYPE IDENTIFIER{
        if(!scope_get_top(parse_symbols,$2)){
            scope_insert(parse_symbols,$2,(void*)1);
            $$=create_ast_node(AST_DEFINITION,$1, $2);
        }else{
            printf("Line %d: ERROR: Variable \"%s\" already defined\n",yylineno,(char*)$2);
            parse_error=1;
        }
    }
    ;
/*todo fix accessing array on creation*/

symbol
//    : symbol_decl { $$=$1; }
    : array_access { $$=$1; }
    | IDENTIFIER{
        if(scope_get(parse_symbols,$1)){
            $$=create_ast_node(AST_IDENTIFIER,$1);
        }else{
            printf("Line %d: ERROR: Variable \"%s\" used before creation\n",yylineno,(char*)$1);
        }
    }

array_decl
    : symbol_decl LSBRA INTEGER RSBRA { ast_node* n=$$=$1;n->type=AST_ARRAY_DEFINITION;n->ival=atoi($3); }
    ;
array_access
    :  symbol LSBRA expression RSBRA { $$=create_ast_node(AST_ARRAY_ACCESS,$1,$3); }
    ;

var_list
    : symbol_decl { $$=$1; }
    | symbol_decl var_list {
        ast_node* n=$$=$1;
        n->NEXT=$2;
    }
call_list
    : expression { $$=$1; }
    | expression COMMA var_list{
        ast_node* n=$$=$1;
        n->NEXT=$2;
    }
    ;

fn_begin
    : symbol_decl LPAR { parse_symbols=scope_create(parse_symbols); $$=$1; }
    ;
if_begin
    : IF expression { parse_symbols=scope_create(parse_symbols); $$=$2; }
    ;
if_end
    : if_begin block { $$ = create_ast_node(AST_IF,$1,$2); scope_pop(&parse_symbols);}
    | elif_end { $$=$1; }
    ;
elif_begin
    : ELIF expression { parse_symbols=scope_create(parse_symbols); $$=$2; }
    ;
elif_end
    : if_end elif_begin block { ast_node* n=((ast_node*)$1);
    			while(n->ELIF){n=n->ELIF;}
    			n->ELIF=create_ast_node(AST_IF,$2,$3);
    			scope_pop(&parse_symbols);
    			}
    ;
else_begin
    : if_end ELSE { parse_symbols=scope_create(parse_symbols); $$=$1; }
    ;
else_end
    : else_begin block { $$=$1; ((ast_node*)$1)->ELSE=$2; scope_pop(&parse_symbols); }
    ;
while_begin
    : WHILE expression { parse_symbols=scope_create(parse_symbols); $$=$2; }
    ;

break_statement
    : BREAK NEWLINE { $$ = create_ast_node(AST_BREAK); }
    ;

statement
    : symbol_decl NEWLINE { $$ = $1; /*create_ast_node(AST_ASSIGNMENT,$1,create_ast_node(AST_INTEGER,"0"));*/ }
    | array_decl NEWLINE { $$=$1; }
    | symbol ASSIGN expression NEWLINE { $$=create_ast_node(AST_ASSIGNMENT,$1,$3); }
    | symbol_decl ASSIGN expression NEWLINE { $$=create_ast_node(AST_ASSIGNMENT,$1,$3); }
    | fn_begin var_list RPAR block { $$=create_ast_node(AST_FUNCTION,$1,$2,$4); scope_pop(&parse_symbols);}
    | fn_begin RPAR block { $$=create_ast_node(AST_FUNCTION,$1,NULL,$3); scope_pop(&parse_symbols);}
    | RETURN expression NEWLINE{ $$=create_ast_node(AST_RETURN, $2); }
    | if_end { $$=$1; }
    | else_end { $$=$1; }
    | while_begin block { $$ = create_ast_node(AST_WHILE,$1,$2); scope_pop(&parse_symbols); }
    | break_statement { $$ = $1; }
    | error NEWLINE { parse_error = 1; }
    | expression NEWLINE { $$=$1; }
    ;

block
    : INDENT stmts DEDENT {$$=$2;}
    | NEWLINE INDENT stmts DEDENT {$$=$3;}
    ;
function_call
    : symbol LPAR call_list RPAR { $$=create_ast_node(AST_FUNCTION_CALL,$1,$3); }
    | symbol LPAR RPAR { $$=create_ast_node(AST_FUNCTION_CALL,$1,NULL); }
    ;
expression
    : symbol { $$ = create_ast_node(AST_DEREFERENCE,$1); }
    | TYPE LPAR expression RPAR { $$ = create_ast_node(AST_CAST, $1, $3); }
    | INTEGER { $$ = create_ast_node(AST_INTEGER,$1); }
    | FLOAT { $$ = create_ast_node(AST_FLOAT,$1); }
    | function_call { $$=$1; }
    | symbol INC { $$ = create_ast_node(AST_POST_INC,$1); }
    | symbol DEC { $$ = create_ast_node(AST_POST_DEC,$1); }
    | INC symbol { $$ = create_ast_node(AST_PRE_INC,$2); }
    | DEC symbol { $$ = create_ast_node(AST_PRE_DEC,$2); }
    | expression ADD expression { $$ = create_ast_node(AST_ADD,$1,$3); }
    | expression SUB expression { $$ = create_ast_node(AST_SUB,$1,$3); }
    | expression MUL expression { $$ = create_ast_node(AST_MUL,$1,$3); }
    | expression DIV expression { $$ = create_ast_node(AST_DIV,$1,$3); }
    | expression MOD expression { $$ = create_ast_node(AST_MOD,$1,$3); }
    | expression EQ expression { $$ = create_ast_node(AST_EQUAL,$1,$3); }
    | expression NEQ expression { $$ = create_ast_node(AST_NEQUAL,$1,$3); }
    | expression GT expression { $$ = create_ast_node(AST_GREATER_THAN,$1,$3); }
    | expression GTE expression { $$ = create_ast_node(AST_GREATER_THAN_EQ,$1,$3); }
    | expression LT expression { $$ = create_ast_node(AST_LESS_THAN,$1,$3); }
    | expression LTE expression { $$ = create_ast_node(AST_LESS_THAN_EQ,$1,$3); }
    | LPAR expression RPAR { $$=$2; }
    ;

%%

//void yyerror(YYLTYPE* loc, const char* err) {
//    fprintf(stderr, "Error (line %d): %s\n", loc->first_line, err);
//}
void yyerror(YYLTYPE* loc, const char* err)
{
    printf("Line %d: (%s)\n",yylineno,err);
}
