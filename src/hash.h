#pragma once
#include <stdint.h>


typedef struct{
    uint32_t size;
    uint32_t used;
    void** data;
}hash_table;


hash_table* hash_table_init();

void* hash_get(hash_table *table, const char *key);
//returns true if newly inserted
int hash_insert(hash_table *table, const char *key, void* data);
void hash_free(hash_table* table);