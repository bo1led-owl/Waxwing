#include "str_hashmap.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LOAD_FACTOR 0.75
enum {
    MIN_SIZE = 16,
};

void str_hashmap_clear(str_hashmap_t* map) {
    if (!map)
        return;

    for (usize i = 0; i < map->bucket_count; ++i) {
        map->metadata[i] = false;
    }

    map->size = 0;
}

void str_hashmap_free(str_hashmap_t* map) {
    if (!map)
        return;

    free(map->data);
    *map = (str_hashmap_t){0};
}

static usize probe(const usize i, const usize bucket_count) {
    // TODO: another kind of probing?
    return (i + 1) % bucket_count;
}

static str_t* insert(str_pair_t* data, bool* metadata, const usize bucket_count,
                     const str_pair_t* item) {
    const u64 hash = str_hash(item->key) % bucket_count;

    for (usize i = hash;; i = probe(i, bucket_count)) {
        if (!metadata[i]) {
            metadata[i] = true;
            memcpy(data + i, item, sizeof(str_pair_t));
            return nullptr;
        }

        if (str_compare(data[i].key, item->key) == 0) {
            return &data[i].value;
        }
    }
}

static f64 load_factor(const str_hashmap_t* map) {
    if (map->bucket_count == 0) {
        return INFINITY;
    }

    return (f64)map->size / (f64)map->bucket_count;
}

static void resize_if_needed(str_hashmap_t* map) {
    if (load_factor(map) < MAX_LOAD_FACTOR) {
        return;
    }

    if (map->bucket_count == 0) {
        *map = (str_hashmap_t){.data = calloc(MIN_SIZE, sizeof(str_pair_t)),
                               .metadata = calloc(MIN_SIZE, sizeof(bool)),
                               .bucket_count = MIN_SIZE,
                               .size = map->size};
        return;
    }

    const usize new_bucket_count = map->bucket_count * 2;
    str_pair_t* new_data = malloc(new_bucket_count * sizeof(str_pair_t));
    bool* new_metadata = calloc(new_bucket_count, sizeof(bool));

    // rehashing
    for (usize i = 0, moved_elements = 0;
         i < map->bucket_count && moved_elements < map->size; ++i) {
        if (!map->metadata[i])
            continue;

        insert(new_data, new_metadata, new_bucket_count, &map->data[i]);

        moved_elements += 1;
    }

    free(map->data);
    free(map->metadata);
    *map = (str_hashmap_t){.data = new_data,
                           .metadata = new_metadata,
                           .bucket_count = new_bucket_count,
                           .size = map->size};
}

bool str_hashmap_insert(str_hashmap_t* map, const str_t key,
                        const str_t value) {
    resize_if_needed(map);

    const str_pair_t entry = {.key = key, .value = value};
    str_t* result = insert(map->data, map->metadata, map->bucket_count, &entry);

    if (!result)
        map->size += 1;

    // evaluates to `true` if the value was present
    return result != nullptr;
}

void str_hashmap_insert_or_replace(str_hashmap_t* map, const str_t key,
                                   const str_t value) {
    resize_if_needed(map);

    const str_pair_t entry = {.key = key, .value = value};
    str_t* result = insert(map->data, map->metadata, map->bucket_count, &entry);

    if (result) {
        *result = value;
    }
}

str_t const* str_hashmap_get(const str_hashmap_t* map, const str_t key) {
    if (map->bucket_count == 0)
        return nullptr;

    const u64 hash = str_hash(key) % map->bucket_count;

    for (usize i = hash;; i = probe(i, map->bucket_count)) {
        if (!map->metadata[i]) {
            return nullptr;
        }

        if (str_compare(map->data[i].key, key) == 0) {
            return &map->data[i].value;
        }
    }
}
