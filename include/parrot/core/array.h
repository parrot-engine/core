#ifndef PARROT_CORE_INCLUDE_PARROT_CORE_CONTAINERS_H_
#define PARROT_CORE_INCLUDE_PARROT_CORE_CONTAINERS_H_

#include "parrot/core/util.h"
#include <stdbool.h>
#include <stddef.h>

#define ParrotArray_init_capacity(arr, capacity) ((arr) = ParrotArray_initx(sizeof(*(arr)), capacity))
#define ParrotArray_init(arr) ((arr) = ParrotArray_initx(sizeof(*(arr)), 16))
#define ParrotArray_free(arr) (ParrotArray_freex(arr), (arr) = PARROT_C_CPP((void *)0, nullptr))

#define ParrotArray_force_init(arr) (!(arr) ? ParrotArray_initx(sizeof(*(arr)), 16) : (arr))

#define ParrotArray_size(arr) ((arr) ? ParrotArray_sizex(arr) : 0)
#define ParrotArray_capacity(arr) ((arr) ? ParrotArray_capacityx(arr) : 0)

#define ParrotArray_reserve(arr, new) ((arr) = ParrotArray_reservex(ParrotArray_force_init(arr), new))
/// Allocated data is guarnteed to be zeroed
#define ParrotArray_alloc_at(arr, index, count) ((arr) = ParrotArray_allocx(ParrotArray_force_init(arr), index, count))
/**
 * Allocates elements at end
 *
 * @returns index of first new element
 */
#define ParrotArray_alloc(arr, count)                                                                                   \
    (ParrotArray_alloc_at(arr, ParrotArray_size(arr), count), ParrotArray_size(arr) - 1)

#define ParrotArray_push(arr, value)                                                                                    \
    do {                                                                                                                \
        size_t _idx = ParrotArray_alloc(arr, 1);                                                                        \
        (arr)[_idx] = (value);                                                                                          \
    } while (0)
#define ParrotArray_deln(arr, index, count) ParrotArray_delx(arr, index, count)
#define ParrotArray_del(arr, index) ParrotArray_deln(arr, index, 1)

#define ParrotArray_put_key(arr, value, key, key_size)                                                                  \
    do {                                                                                                                \
        size_t _idx = ParrotArray_alloc(arr, 1);                                                                        \
        (arr)[_idx] = (value);                                                                                          \
                                                                                                                        \
        ptrdiff_t _old_idx = ParrotArray_findx(arr, key, key_size);                                                     \
        if (_old_idx >= 0) {                                                                                            \
            ParrotArray_del(arr, _old_idx);                                                                             \
        }                                                                                                               \
                                                                                                                        \
        ParrotArray_mapx(arr, _idx, key, key_size);                                                                     \
    } while (0)
#define ParrotArray_put(arr, value) ParrotArray_put_key(arr, value, &(value).key, sizeof((value).key));
#define ParrotArray_puts(arr, value) ParrotArray_put_key(arr, value, (value).key, strlen((value).key));
/// Returns <0 if not found
#define ParrotArray_find(arr, key) ((arr) ? ParrotArray_findx(arr, &(key), sizeof(key)) : -1)
/// Returns <0 if not found
#define ParrotArray_finds(arr, key) ((arr) ? ParrotArray_findx(arr, key, strlen(key)) : -1)
/// Returns NULL if not found
#define ParrotArray_findp(arr, key) ((arr) ? ParrotArray_findpx(arr, &(key), sizeof(key)) : NULL)
/// Returns NULL if not found
#define ParrotArray_findsp(arr, key) ((arr) ? ParrotArray_findpx(arr, key, strlen(key)) : NULL)

PARROT_API void *ParrotArray_initx(size_t element_size, size_t initial_capacity);
PARROT_API void ParrotArray_freex(void *arr);

PARROT_API size_t ParrotArray_sizex(const void *arr);
PARROT_API size_t ParrotArray_capacityx(const void *arr);

PARROT_API void *ParrotArray_reservex(void *arr, size_t new);
/// Allocated data is guarnteed to be zeroed
PARROT_API void *ParrotArray_allocx(void *arr, size_t index, size_t count);
PARROT_API void ParrotArray_delx(void *arr, size_t index, size_t count);

PARROT_API void ParrotArray_mapx(void *arr, size_t index, const void *key, size_t key_size);
/// Returns <0 if not found
PARROT_API ptrdiff_t ParrotArray_findx(void *arr, const void *key, size_t key_size);
/// Returns NULL if not found
PARROT_API void *ParrotArray_findpx(void *arr, const void *key, size_t key_size);

#endif // PARROT_CORE_INCLUDE_PARROT_CORE_CONTAINERS_H_
