#pragma once

#include "types.h"

typedef struct str {
    char* data;
    usize len;
} str_t;

str_t str_from_cstr(char* cstr);
i32 str_compare(const str_t a, const str_t b);
str_t str_trim(const str_t s);
u64 str_hash(const str_t target);
void str_print(const str_t s);

#define split_for_each(iter, capture)                                \
    for (str_t capture = split_next(&iter); capture.data != nullptr; \
         capture = split_next(&iter))

typedef struct str_split_iter {
    str_t source;
    str_t sep;
    usize cur_split_len;
} str_split_iter_t;

str_split_iter_t str_split(const str_t s, const str_t sep);
str_t split_next(str_split_iter_t* iter);

typedef struct dynamic_str {
    char* data;
    usize size;
    usize capacity;
} dynamic_str_t;

void string_push(dynamic_str_t* s, const char c);
void string_concat(dynamic_str_t* s, const str_t u);
void string_reserve(dynamic_str_t* s, const usize size);
void string_pop(dynamic_str_t* s, const usize n);
void string_free(dynamic_str_t* s);

str_t str_from_string(const dynamic_str_t* s);
