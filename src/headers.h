#pragma once

#include "str.h"
#include "types.h"

typedef struct http_header {
    str_t key;
    str_t value;
} http_header_t;

typedef struct http_headers {
    http_header_t* data;
    bool* metadata;  // replace with some kind of bitset?
    usize bucket_count;
    usize size;
} http_headers_t;

void headers_clear(http_headers_t* headers);
void headers_free(http_headers_t* headers);
bool headers_set(http_headers_t* headers, const str_t key, const str_t value);
str_t* headers_get(const http_headers_t* headers, const str_t key);
