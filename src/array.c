#include "parrot/core/array.h"
#include "parrot/core/hash.h"
#include "parrot/core/math.h"
#include "parrot/core/util.h"
#include <stddef.h>
#include <stdint.h>

typedef struct ArrayHashEntry ArrayHashEntry;

typedef struct {
    size_t element_size;

    size_t size;
    size_t capacity;

    size_t hash_table_size;
    size_t hash_table_capacity;
    ArrayHashEntry *hash_table;
} ArrayMeta;

struct ArrayHashEntry {
    ArrayHashEntry *next;

    ParrotCRC32 hash;
    size_t index;
};

#define ARRAY_META_SIZE /* I still want SIMD */ PARROT_ALIGN_UP(sizeof(ArrayMeta), 16)
#define ARRAY_TOTAL_SIZE(capacity, element_size) (ARRAY_META_SIZE + ((capacity) * (element_size)))
#define ARRAY_METADATA(arr) ((ArrayMeta *)((uint8_t *)(arr) - ARRAY_META_SIZE))
#define ARRAY_DATA(self) ((void *)((uint8_t *)(self) + ARRAY_META_SIZE))

static void free_hash_table(ArrayHashEntry *table, size_t capacity) {
    for (size_t i = 0; i < capacity; i++) {
        ArrayHashEntry *entry = table[i].next;
        while (entry) {
            ArrayHashEntry *next = entry->next;
            free(entry);
            entry = next;
        }
    }

    free(table);
}

void *ParrotArray_initx(size_t element_size, size_t initial_capacity) {
    ArrayMeta *self = malloc(ARRAY_TOTAL_SIZE(initial_capacity, element_size));
    memset(self, 0, sizeof(ArrayMeta));

    self->element_size = element_size;
    self->capacity = initial_capacity;

    return ARRAY_DATA(self);
}

void ParrotArray_freex(void *arr) {
    PARROT_FAIL_NULL(arr);

    ArrayMeta *self = ARRAY_METADATA(arr);

    if (self->hash_table) {
        free_hash_table(self->hash_table, self->hash_table_capacity);
    }

    free(self);
}

size_t ParrotArray_sizex(const void *arr) {
    PARROT_FAIL_NULL(arr);

    return ARRAY_METADATA(arr)->size;
}

size_t ParrotArray_capacityx(const void *arr) {
    PARROT_FAIL_NULL(arr);

    return ARRAY_METADATA(arr)->capacity;
}

void *ParrotArray_reservex(void *arr, size_t new) {
    PARROT_FAIL_NULL(arr);

    ArrayMeta *self = ARRAY_METADATA(arr);

    size_t remaining_capacity = self->capacity - self->size;
    PARROT_RET_COND_V(new <= remaining_capacity, arr);

    self->capacity = PARROT_MAX(self->capacity + self->capacity / 2, new);
    self = realloc(self, ARRAY_TOTAL_SIZE(self->capacity, self->element_size));

    return ARRAY_DATA(self);
}

void *ParrotArray_allocx(void *arr, size_t index, size_t count) {
    PARROT_FAIL_NULL(arr);

    PARROT_FAIL_COND(index > ARRAY_METADATA(arr)->size);

    arr = ParrotArray_reservex(arr, count);

    ArrayMeta *self = ARRAY_METADATA(arr);

    void *from = (uint8_t *)arr + index * self->element_size;
    void *to = (uint8_t *)from + count * self->element_size;

    memmove(to, from, (self->size - index) * self->element_size);
    memset(from, 0, count * self->element_size);

    self->size += count;

    PARROT_RET_COND_V(!self->hash_table, arr);

    for (size_t i = 0; i < self->hash_table_capacity; i++) {
        for (ArrayHashEntry *entry = &self->hash_table[i]; entry; entry = entry->next) {
            if (entry->index < index || entry->index == SIZE_MAX) {
                continue;
            }

            entry->index += count;
        }
    }

    return arr;
}

void ParrotArray_delx(void *arr, size_t index, size_t count) {
    PARROT_FAIL_NULL(arr);

    ArrayMeta *self = ARRAY_METADATA(arr);

    PARROT_FAIL_COND(index >= self->size);
    PARROT_FAIL_COND(index + count > self->size);

    void *to = (uint8_t *)arr + index * self->element_size;
    void *from = (uint8_t *)to + count * self->element_size;

    memmove(to, from, (self->size - (index + count)) * self->element_size);
    self->size -= count;

    PARROT_RET_COND(!self->hash_table);

    for (size_t i = 0; i < self->hash_table_capacity; i++) {
        ArrayHashEntry *prev = NULL;
        ArrayHashEntry *entry = &self->hash_table[i];
        while (entry) {
            if (entry->index < index) {
                prev = entry;
                entry = entry->next;
                continue;
            }

            if (entry->index >= index + count) {
                entry->index -= count;

                prev = entry;
                entry = entry->next;
                continue;
            }

            if (prev) {
                prev->next = entry->next;
            }

            ArrayHashEntry *next = entry->next;
            if (!prev) {
                if (next) {
                    *entry = *next;
                    free(next);
                } else {
                    entry->index = SIZE_MAX;
                }
            } else {
                free(entry);
                entry = next;
            }

            self->hash_table_size--;
        }
    }
}

static void
map_hash(ArrayMeta *self,
         size_t index,
         /* TODO: CRC32 has a few problems with security and is inefficent for hash tables. Replace with
            siphash later or FNV-1a if comparing keys will be added but isn't ideal because storing keys is memory
            expensive. Doesn't block release as collisions are rare-ish but is something to do next release. */
         ParrotCRC32 hash) {
    size_t hash_index = hash % self->hash_table_capacity;

    ArrayHashEntry *entry = &self->hash_table[hash_index];

    if (entry->index != SIZE_MAX) {
        while (entry->next && entry->hash != hash) {
            entry = entry->next;
        }

        if (entry->hash != hash) {
            entry->next = PARROT_ALLOC(ArrayHashEntry);
            entry = entry->next;
        }
    }

    *entry = (ArrayHashEntry){
        .hash = hash,
        .index = index,
    };
}

static void rehash_table(ArrayMeta *self) {
    size_t old_capacity = self->hash_table_capacity;
    ArrayHashEntry *old_table = self->hash_table;

    self->hash_table_capacity = PARROT_MAX(self->hash_table_capacity * 1.5, 64);
    self->hash_table = calloc(self->hash_table_capacity, sizeof(*self->hash_table));

    for (size_t i = 0; i < self->hash_table_capacity; i++) {
        self->hash_table[i].index = SIZE_MAX;
    }

    PARROT_RET_COND(!old_table);

    for (size_t i = 0; i < old_capacity; i++) {
        for (ArrayHashEntry *entry = &old_table[i]; entry; entry = entry->next) {
            map_hash(self, entry->index, entry->hash);
        }
    }

    free_hash_table(old_table, old_capacity);
}

void ParrotArray_mapx(void *arr, size_t index, const void *key, size_t key_size) {
    PARROT_FAIL_NULL(arr);

    ArrayMeta *self = ARRAY_METADATA(arr);

    PARROT_FAIL_COND(index >= self->size);

    if (!self->hash_table) {
        rehash_table(self);
    }

    map_hash(self, index, Parrot_crc32(key, key_size));

    if (++self->hash_table_size >= self->hash_table_capacity * 0.75) {
        rehash_table(self);
    }
}

ptrdiff_t ParrotArray_findx(void *arr, const void *key, size_t key_size) {
    PARROT_FAIL_NULL(arr);

    ArrayMeta *self = ARRAY_METADATA(arr);

    PARROT_RET_COND_V(!self->hash_table, -1);

    uint32_t hash = Parrot_crc32(key, key_size);
    for (ArrayHashEntry *entry = &self->hash_table[hash % self->hash_table_capacity]; entry; entry = entry->next) {
        PARROT_RET_COND_V(entry->hash == hash, entry->index);
    }

    return -1;
}

void *ParrotArray_findpx(void *arr, const void *key, size_t key_size) {
    ptrdiff_t index = ParrotArray_findx(arr, key, key_size);
    PARROT_RET_COND_V(index < 0, NULL);
    return (uint8_t *)arr + ARRAY_METADATA(arr)->element_size * index;
}

void ParrotArray_delkx(void *arr, const void *key, size_t key_size) {
    ptrdiff_t index = ParrotArray_findx(arr, key, key_size);
    PARROT_RET_COND(index < 0);
    ParrotArray_delx(arr, index, 1);
}
