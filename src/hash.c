#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "hash.h"

#define KEYSIZE 2

hash_table* hash_table_init(){
    hash_table* h= malloc(sizeof(hash_table));
    *h=(hash_table){.size=32,.used=0,.data=calloc(32,(2*sizeof(void*)))};
    return h;
}
/*
 * The DJB hash function: http://www.cse.yorku.ca/~oz/hash.html.
 */
uint64_t hash_function(const char *key){
    unsigned long hash = 5381;
    int c;
    while ((c = (int)*key++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash;
}
void* hash_get(hash_table *table, const char *key){
    uint64_t hash = hash_function(key);
    int i=0, index = (int)(hash%table->size);
    while (table->data[KEYSIZE*index] != 0) {
        if (!strcmp((char*)table->data[KEYSIZE*index], key)) {
            return table->data[KEYSIZE*index+1];
        }
        i++;
        index = (int)((hash + i*i) % table->size);
    }
    return false;

}

int hash_resize(hash_table* table){
    uint32_t o_size=table->size;
    void** o_data=table->data;
    table->used=0;

    table->size*=2;
    table->data= calloc(table->size,(2*sizeof(void*)));

    for (int i = 0; i < o_size; ++i) {
        if(o_data[KEYSIZE*i]){
            hash_insert(table,o_data[KEYSIZE*i],o_data[KEYSIZE*i+1]);
        }
    }
    free(o_data);
    return (int)table->size;
}
//returns true if newly inserted
int hash_insert(hash_table *table, const char *key, void* data) {
    if(table->used>(table->size*3/4)){
        hash_resize(table);
    }
    int new=1;
    uint64_t hash = hash_function(key);
    int i=0, index = (int)(hash%table->size);
    while (table->data[KEYSIZE*index] != 0) {
        if (!strcmp((char*)table->data[KEYSIZE*index], key)) {
            new=0;
            break;
        }
        i++;
        index = (int)((hash + i*i) % table->size);
    }
    table->data[KEYSIZE*index]=(void*)key;
    table->data[KEYSIZE*index+1]=data;
    table->used++;

    return new;
}
void hash_free(hash_table* table){
    free(table->data);
    free(table);
}