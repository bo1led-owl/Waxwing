#pragma once

#include "str.h"
#include "types.h"

typedef struct str_pair {
    str_t key;
    str_t value;
} str_pair_t;

typedef struct str_hashmap {
    str_pair_t* data;
    bool* metadata;  // replace with some kind of bitset?
    usize bucket_count;
    usize size;
} str_hashmap_t;

/*
Remove all elements from the hashmap
*/
void str_hashmap_clear(str_hashmap_t* map);

/*
Free memory allocated to the hashmap, setting all its fields to zero
*/
void str_hashmap_free(str_hashmap_t* map);

/*
Insert a new key-value pair into the hashmap. Returns whether the key was
present
*/
bool str_hashmap_insert(str_hashmap_t* map, const str_t key, const str_t value);

/*
Insert a key-value pair into the hashmap, or replace the value if the key was
already present
*/
void str_hashmap_insert_or_replace(str_hashmap_t* map, const str_t key,
                                   const str_t value);

/*
Get a const pointer to the value by key. Returns nullptr if the key is not
present
*/
str_t const* str_hashmap_get(const str_hashmap_t* map, const str_t key);
