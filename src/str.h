#pragma once

#include "types.h"

/*
Data type holding a pointer to a string and its length.
Is not guaranteed to be null-terminated.
May own the string, but typically does not.
*/
typedef struct str {
    char* data;
    usize len;
} str_t;

/*
Create a `str` object from a c-style string
*/
str_t str_from_cstr(char* cstr);

/*
Lexicographically compare two strings. Returns -1 if `a` < `b`, 0
if `a` == `b` and 1 if `a` > `b`
*/
i32 str_compare(const str_t a, const str_t b);

/*
Remove whitespace from both ends of the given string and return the result
*/
str_t str_trim(const str_t s);

/*
Hash given string and return an unsigned 64-bit hash result
*/
u64 str_hash(const str_t s);

/*
Print given string to the default stdio
*/
void str_print(const str_t s);

#define split_for_each(iter, capture)                                \
    for (str_t capture = split_next(&iter); capture.data != nullptr; \
         capture = split_next(&iter))

/*
Type for holding string split data, not intended for manual usage
*/
typedef struct str_split_iter {
    str_t source;
    str_t sep;
    usize cur_split_len;
} str_split_iter_t;

/*
Split given string `s` by separator `sep` and return a split iterator object
*/
str_split_iter_t str_split(const str_t s, const str_t sep);

/*
Get next element from a split iterator. End of source is indicated by a string
with a null pointer. Tip: use `str_split_iter` macro for working on consecutive
splits
*/
str_t split_next(str_split_iter_t* iter);

/*
An owning dynamic string, acts like `std::string` in C++ or `String` in Rust.
Is not guaranteed to be null-terminated.
*/
typedef struct dynamic_str {
    char* data;
    usize size;
    usize capacity;
} dynamic_str_t;

/*
Push a character into string, reallocate if needed, thus may invalidate pointers
into the data.
*/
void string_push(dynamic_str_t* s, const char c);

/*
Copy all the characters from string `u` into dynamic string.
*/
void string_concat(dynamic_str_t* s, const str_t u);

/*
Reserve space for `n` elements
*/
void string_reserve(dynamic_str_t* s, const usize n);

/*
Remove `n` characters from the end of the string
*/
void string_pop(dynamic_str_t* s, const usize n);

/*
Free a dynamic string, setting its pointer, size and capacity to zero
*/
void string_free(dynamic_str_t* s);

/*
Create a string slice from given dynamic string
*/
str_t str_from_string(const dynamic_str_t* s);
