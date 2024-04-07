#include "headers.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LOAD_FACTOR 0.75
enum {
    MIN_SIZE = 16,
};

void headers_clear(http_headers_t* headers) {
    if (!headers)
        return;

    for (usize i = 0; i < headers->bucket_count; ++i) {
        headers->metadata[i] = false;
    }

    headers->size = 0;
}

void headers_free(http_headers_t* headers) {
    if (!headers)
        return;

    free(headers->data);
    *headers = (http_headers_t){0};
}

static usize probe(const usize i, const usize bucket_count) {
    // TODO: another kind of probing?
    return (i + 1) % bucket_count;
}

/// returns whether the key was already present
static bool insert(http_header_t* data, bool* metadata,
                   const usize bucket_count, const http_header_t* item) {
    const u64 hash = str_hash(item->key) % bucket_count;

    for (usize i = hash;; i = probe(i, bucket_count)) {
        if (!metadata[i]) {
            metadata[i] = true;
            memcpy(data + i, item, sizeof(http_header_t));
            return false;
        }

        if (str_compare(data[i].key, item->key) == 0) {
            return true;
        }
    }
}

static f64 load_factor(const http_headers_t* headers) {
    if (headers->bucket_count == 0) {
        return INFINITY;
    }

    return (f64)headers->size / (f64)headers->bucket_count;
}

static void resize_if_needed(http_headers_t* headers) {
    if (load_factor(headers) < MAX_LOAD_FACTOR) {
        return;
    }

    if (headers->bucket_count == 0) {
        *headers =
            (http_headers_t){.data = calloc(MIN_SIZE, sizeof(http_header_t)),
                             .metadata = calloc(MIN_SIZE, sizeof(bool)),
                             .bucket_count = MIN_SIZE,
                             .size = headers->size};
        return;
    }

    const usize new_bucket_count = headers->bucket_count * 2;
    http_header_t* new_data = malloc(new_bucket_count * sizeof(http_header_t));
    bool* new_metadata = calloc(new_bucket_count, sizeof(bool));

    // rehashing
    for (usize i = 0, moved_elements = 0;
         i < headers->bucket_count && moved_elements < headers->size; ++i) {
        if (!headers->metadata[i])
            continue;

        insert(new_data, new_metadata, new_bucket_count, &headers->data[i]);

        moved_elements += 1;
    }

    free(headers->data);
    free(headers->metadata);
    *headers = (http_headers_t){.data = new_data,
                                .metadata = new_metadata,
                                .bucket_count = new_bucket_count,
                                .size = headers->size};
}

bool headers_set(http_headers_t* headers, const str_t key, const str_t value) {
    resize_if_needed(headers);

    const http_header_t header = {.key = key, .value = value};
    const bool result = insert(headers->data, headers->metadata,
                               headers->bucket_count, &header);

    if (!result)
        headers->size += 1;

    return result;
}

str_t* headers_get(const http_headers_t* headers, const str_t key) {
    const u64 hash = str_hash(key) % headers->bucket_count;

    // TODO: another kind of probing?
    for (usize i = hash;; i = probe(i, headers->bucket_count)) {
        if (!headers->metadata[i]) {
            return nullptr;
        }

        if (str_compare(headers->data[i].key, key) == 0) {
            return &headers->data[i].value;
        }
    }
}
