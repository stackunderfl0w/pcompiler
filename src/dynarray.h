#pragma once
#include <stdlib.h>
#include <string.h>
#define INITIAL_CAPACITY 8

typedef struct{
    void **data;
    int size;
    int capacity;
}dynarray;

dynarray* dynarray_init(){
    dynarray* arr=malloc(sizeof(dynarray));
    arr->size=0;
    arr->capacity=INITIAL_CAPACITY;
    arr->data=malloc(arr->capacity*sizeof(void*));
    return arr;
}

void dynarray_insert(dynarray *arr, void *elem, int index){
    if(arr->size==arr->capacity){
        arr->capacity*=2;
        arr->data=realloc(arr->data, arr->capacity*sizeof(void*));
    }
    if(index==-1){
        arr->data[arr->size++] = elem;
    }
    else{
        memmove(arr->data+index+1,arr->data+index,(arr->size-index)*sizeof(void*));
        arr->data[index]=elem;
        arr->size++;
    }
}
void dynarray_clear(dynarray *arr){
    memset(arr->data,0,arr->size*sizeof(void*));
    arr->size=0;
}

void dynarray_remove_index(dynarray *arr, int index){
    memmove(arr->data+index,arr->data+index+1,(arr->size-index-1)*sizeof(void*));
    arr->size--;
}
void dynarray_free(dynarray *arr){
    free(arr->data);
    free(arr);
}