#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <llvm-c/Core.h>

enum ast_node_type{
    AST_GREATER_THAN,
    AST_LESS_THAN,
    AST_GREATER_THAN_EQ,
    AST_LESS_THAN_EQ,
    AST_EQUAL,
    AST_NEQUAL,
    AST_ADD,
    AST_SUB,
    AST_MUL,
    AST_DIV,
    AST_MOD,
    AST_POST_INC,
    AST_POST_DEC,
    AST_PRE_INC,
    AST_PRE_DEC,
    //	AST_AND,
    //	AST_OR,
    AST_INTEGER,
    AST_FLOAT,
    AST_BOOLEAN,
    AST_IDENTIFIER,
    AST_ASSIGNMENT,
    AST_DEFINITION,
    AST_ARRAY_DEFINITION,
    AST_ARRAY_ACCESS,
    AST_IF,
    AST_ELIF,
    AST_ELSE,
    AST_WHILE,
    AST_BREAK,
    AST_FUNCTION,
    AST_FUNCTION_CALL,
    AST_RETURN,
    AST_DEREFERENCE,
    AST_CAST,
};

typedef struct ast_node ast_node;

struct ast_node{
    enum ast_node_type type;
    char* name;
    union{
        size_t ival;
        double fval;
        char* vtype;
    };
    ast_node* L,*R,*NEXT,*ELIF,*ELSE;
};


ast_node* create_ast_node(int node_type, ...);

LLVMModuleRef generate_code(ast_node* root, const char* obj);