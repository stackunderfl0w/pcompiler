#pragma once

#include "hash.h"
typedef struct scope scope;
struct scope{
    scope* previous;
    hash_table* current;
};

scope* scope_create(scope* previous);

void* scope_get(scope* s, char* key);

void* scope_get_top(scope* s, char* key);

int scope_insert(scope *s, const char *key, void* data);

void scope_pop(scope **s);

void scope_push(scope **s);