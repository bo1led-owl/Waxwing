#include "vector.h"

#include <stdlib.h>
#include <string.h>

#include "types.h"

enum {
    VECTOR_MIN_CAPACITY = 8
};

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

void vector_reserve(vector_t* vec, const usize elem_size, const usize size) {
    if (vec->capacity >= size) {
        return;
    }

    if (vec->capacity == 0) {
        vec->capacity = VECTOR_MIN_CAPACITY;
        vec->items = realloc(vec->items, vec->capacity * elem_size);
        return;
    }

    const usize diff = size / vec->capacity;
    usize resize_factor;
    if (diff == 1) {
        resize_factor = 2;
    } else {
        // round factor up to the next power of two
        resize_factor = (diff & (1 << (SIZE_WIDTH - __builtin_clzll(diff) - 1)))
                        << 1;
    }
    vec->capacity *= resize_factor;
    vec->items = realloc(vec->items, vec->capacity * elem_size);
}

void vector_push(vector_t* vec, const usize elem_size, const void* elems,
                 const usize elems_count) {
    vector_reserve(vec, elem_size, vec->size + elems_count);
    memcpy((char*)vec->items + (vec->size * elem_size), elems,
           elem_size * elems_count);
    vec->size += elems_count;
}

void vector_pop(vector_t* vec, const usize n) {
    vec->size -= min(vec->size, n);
}

void vector_free(vector_t* vec) {
    if (!vec)
        return;

    free(vec->items);
    *vec = (vector_t){0};
}
