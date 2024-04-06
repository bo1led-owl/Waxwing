#pragma once

#include "types.h"

typedef struct vector {
    void* items;
    usize size;
    usize capacity;
} vector_t;

void vector_push(vector_t* vec, const usize elem_size, const void* elems,
                 const usize elems_count);
void vector_reserve(vector_t* vec, const usize elem_size, const usize size);
void vector_pop(vector_t* vec, const usize n);
void vector_free(vector_t* vec);
