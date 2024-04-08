#pragma once

#include "types.h"

typedef struct vector {
    void* items;
    usize size;
    usize capacity;
} vector_t;

/*
Copy `n` elements of size `size` from `elements` into vector
*/
void vector_push(vector_t* vec, const usize size, const void* elements,
                 const usize n);

/*
Reserve enough space for `n` elements of size `size`
*/
void vector_reserve(vector_t* vec, const usize size, const usize n);

/*
Remove n elements from the end of the vector
*/
void vector_pop(vector_t* vec, const usize n);

/*
Free the buffer and set all the fields to zero
*/
void vector_free(vector_t* vec);
