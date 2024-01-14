#include <stdlib.h>
#include "scope.h"

scope* scope_create(scope* previous){
    scope * s= malloc(sizeof(hash_table));
    s->current=hash_table_init();
    s->previous=previous;
    return s;
}

void* scope_get(scope* s, char* key){
    void* found=NULL;
    while(s&&!found){
        found=hash_get(s->current,key);
        s=s->previous;
    }
    return found;
}
void* scope_get_top(scope* s, char* key){
    return hash_get(s->current,key);
}

int scope_insert(scope *s, const char *key, void* data){
    return hash_insert(s->current,key,data);
}
//free current scope level
void scope_pop(scope **s){
    scope* prev=(*s)->previous;
    free((*s)->current);
    free(*s);
    *s=prev;
}

void scope_push(scope **s){
    *s= scope_create(*s);
}