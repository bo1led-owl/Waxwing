#include "str.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

enum {
    STRING_MIN_CAPACITY = 32
};

str_t str_from_cstr(char* cstr) {
    return (str_t){.data = cstr, .len = strlen(cstr)};
}

i32 str_compare(const str_t a, const str_t b) {
    const usize len = min(a.len, b.len);
    for (usize i = 0; i < len; ++i) {
        if (a.data[i] > b.data[i]) {
            return 1;
        } else if (a.data[i] < b.data[i]) {
            return -1;
        }
    }

    if (a.len < b.len) {
        return -1;
    } else if (a.len > b.len) {
        return 1;
    } else {
        return 0;
    }
}

void str_to_lower(str_t s) {
    for (usize i = 0; i < s.len; ++i) {
        s.data[i] = tolower(s.data[i]);
    }
}

void str_to_upper(str_t s) {
    for (usize i = 0; i < s.len; ++i) {
        s.data[i] = toupper(s.data[i]);
    }
}

str_t str_trim(const str_t s) {
    if (!s.data) {
        return (str_t){0};
    }

    str_t res = s;
    for (usize i = 0; i < s.len; ++i) {
        if (!isspace(s.data[i]))
            break;

        res.data += 1;
        res.len -= 1;
    }

    for (isize i = res.len - 1; i >= 0; --i) {
        if (!isspace(res.data[i]))
            break;
        res.len -= 1;
    }

    return res;
}

// djb2 algorithm
u64 str_hash(const str_t s) {
    u64 hash = 5381;
    for (usize i = 0; i < s.len; ++i) {
        hash = ((hash << 5) + hash) + s.data[i];
    }

    return hash;
}

void str_print(const str_t s) {
    if (!s.data)
        return;
    fwrite(s.data, sizeof(char), s.len, stdout);
}

str_split_iter_t str_split(const str_t s, const str_t sep) {
    return (str_split_iter_t){.source = s, .sep = sep, .cur_split_len = 0};
}

str_t split_next(str_split_iter_t* iter) {
    iter->source.data += iter->cur_split_len;
    iter->source.len -= min(iter->cur_split_len, iter->source.len);

    if (iter->source.len == 0) {
        return (str_t){.data = nullptr, .len = 0};
    }

    iter->cur_split_len = 0;
    usize sep_i = 0;
    while (iter->cur_split_len < iter->source.len && sep_i < iter->sep.len) {
        if (iter->source.data[iter->cur_split_len] == iter->sep.data[sep_i]) {
            sep_i += 1;
        } else {
            sep_i = 0;
        }

        iter->cur_split_len++;
    }

    if (sep_i != iter->sep.len) {
        return iter->source;
    }
    return (str_t){.data = iter->source.data,
                   .len = iter->cur_split_len - iter->sep.len};
}

void string_reserve(dynamic_str_t* s, const usize size) {
    if (s->capacity >= size) {
        return;
    }

    if (s->capacity == 0) {
        s->capacity = STRING_MIN_CAPACITY;
        s->data = realloc(s->data, s->capacity * sizeof(char));
        return;
    }

    const usize diff = size / s->capacity;
    usize resize_factor;
    if (diff == 1) {
        resize_factor = 2;
    } else {
        // round factor up to the next power of two
        resize_factor = (diff & (1 << (SIZE_WIDTH - __builtin_clzll(diff) - 1)))
                        << 1;
    }
    s->capacity *= resize_factor;
    s->data = realloc(s->data, s->capacity * sizeof(char));
}

void string_push(dynamic_str_t* s, const char c) {
    string_reserve(s, s->size + 1);
    s->data[s->size] = c;
    s->size += 1;
}

void string_concat(dynamic_str_t* s, const str_t u) {
    if (!u.data || u.len == 0) {
        return;
    }

    string_reserve(s, s->size + u.len);
    memcpy(s->data + s->size, u.data, u.len * sizeof(char));
    s->size += u.len;
}

void string_pop(dynamic_str_t* s, const usize n) {
    s->size -= min(s->size, n);
}

void string_free(dynamic_str_t* s) {
    if (!s)
        return;

    free(s->data);
    *s = (dynamic_str_t){0};
}

str_t str_from_string(const dynamic_str_t* s) {
    return (str_t){.data = s->data, .len = s->size};
}
