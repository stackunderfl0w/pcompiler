#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 1000

typedef struct Node {
    char* key;
    void* value;
} Node;

typedef struct hashmap {
    Node table[TABLE_SIZE];
} hashmap;


unsigned int hash(const char* key, int size) {
    unsigned int hashval = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        hashval = key[i] + 31 * hashval;
    }
    hashval += size;
    return hashval % TABLE_SIZE;
}

void hashmap_insert(hashmap* table, const char* key, int size, void* value) {
    unsigned int index = hash(key, size);
    unsigned int offset = 0;
    while (table->table[index].key != NULL) {
        offset++;
        index = (index + offset*offset) % TABLE_SIZE;
    }
    table->table[index].key = strndup(key,256);
    table->table[index].value = value;
}

void* hashmap_get(hashmap* table, const char* key, int size) {
    unsigned int index = hash(key, size);
    unsigned int offset = 0;
    while (table->table[index].key != NULL) {
        if (strcmp(table->table[index].key, key) == 0) {
            return table->table[index].value;
        }
        offset++;
        index = (index + offset*offset) % TABLE_SIZE;
    }
    return NULL;
}

void hashmap_free(hashmap* table) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        if(table->table[i].key){
            free(table->table[i].key);
        }
    }
    free(table);
}

hashmap* hashmap_init(){
    return calloc(sizeof(hashmap),1);
}