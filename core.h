#ifndef _PARROT_CORE_H_
#define _PARROT_CORE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h> // IWYU pragma: keep
#include <stdlib.h>
#include <string.h> // IWYU pragma: keep

/**
 *
 * PARROT_CORE_SELECTIVE
 *	Select individial components instead of including all
 *
 * PARROT_CORE_MATH
 * PARROT_CORE_HASH
 * PARROT_CORE_BINARY
 * PARROT_CORE_TIMING
 * PARROT_CORE_MAIN_LOOP
 * PARROT_CORE_SERIALIZE
 *
 * PARROT_CORE_IMPL
 *	Implement the selected components
 *
 * PARROT_CORE_IMPL_CORE
 *	Implement the core part of this library (Requires PARROT_CORE_IMPL)
 *
 * PARROT_PLATFORM_UNIX
 *  Marks the current platform as unix based
 *
 * PARROT_PLATFORM_LINUX
 *  Marks the current platform as linux
 *
 */

#ifndef PARROT_CORE_SELECTIVE

#define PARROT_CORE_MATH
#define PARROT_CORE_HASH
#define PARROT_CORE_BINARY
#define PARROT_CORE_TIMING
#define PARROT_CORE_MAIN_LOOP
#define PARROT_CORE_SERIALIZE

#define PARROT_CORE_IMPL_CORE

#endif

/// Handles crashing the program and sometimes collecting extra info
typedef void (*ParrotCrashHandlerFunc)(const char *cause);

extern ParrotCrashHandlerFunc Parrot_crash_handler;

#ifdef __cplusplus
#define PARROT_CPP(...) __VA_ARGS__
#define PARROT_C(...)
#else
#define PARROT_CPP(...)
#define PARROT_C(...) __VA_ARGS__
#endif

#define PARROT_DEPEND(...) __asm__ volatile("" ::__VA_ARGS__)

#define PARROT_STRING(x) #x
#define PARROT_TYPE_STRING(type) ((void)sizeof(*(type *)NULL), #type)

#define PARROT_FAIL_FMT(fmt, ...)                                                                                       \
    do {                                                                                                                \
        fprintf(stderr, "(%s:%d in \"%s\") ERROR: " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);             \
        Parrot_crash_handler("Assertion Failed");                                                                       \
    } while (0)
#define PARROT_FAIL_MSG(msg) PARROT_FAIL_FMT("%s", msg)

#define PARROT_FAIL_COND_FMT(cond, fmt, ...)                                                                            \
    do {                                                                                                                \
        if (cond) {                                                                                                     \
            PARROT_FAIL_FMT(fmt, ##__VA_ARGS__);                                                                        \
        }                                                                                                               \
    } while (0)
#define PARROT_FAIL_COND_MSG(cond, msg) PARROT_FAIL_COND_FMT(cond, "%s", msg)
#define PARROT_FAIL_COND(cond) PARROT_FAIL_COND_FMT(cond, "Condition \"%s\" succeeded", #cond)

#define PARROT_FAIL_NULL_FMT(value, fmt, ...) PARROT_FAIL_COND_FMT((value) == NULL, fmt, ##__VA_ARGS__)
#define PARROT_FAIL_NULL_MSG(value, msg) PARROT_FAIL_NULL_FMT(value, "%s", msg)
#define PARROT_FAIL_NULL(value) PARROT_FAIL_COND_FMT((value) == NULL, "\"%s\" is NULL", #value)

#define PARROT_RET_COND_V(cond, value)                                                                                  \
    do {                                                                                                                \
        if (cond) {                                                                                                     \
            return value;                                                                                               \
        }                                                                                                               \
    } while (0)

#define PARROT_RET_COND(cond) PARROT_RET_COND_V(cond, )

#define PARROT_ALIGN_UP(n, align) (((n) + (align) - 1) & ~((align) - 1))
#define PARROT_ALIGN_DOWN(n, align) ((n) & ~((align) - 1))

#define PARROT_ARRAY_LEN(arr) (sizeof(arr) / sizeof(*(arr)))

#define PARROT_ALLOC(type_) (type_ *)memset(malloc(sizeof(type_)), 0, sizeof(type_))

#define PARROT_API PARROT_CPP(extern "C")

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct ParrotScope ParrotScope;

PARROT_API ParrotScope *ParrotScope_new(/* NULL = no parent */ ParrotScope *parent);
PARROT_API void ParrotScope_delete(ParrotScope *self);
PARROT_API void ParrotScope_vdelete(void *self);

PARROT_API void ParrotScope_set_ctx(ParrotScope *self, void *ctx);
#define ParrotScope_alloc_ctx(self, type) ((type *)ParrotScope_alloc_ctx_raw(self, sizeof(type)))
PARROT_API void *ParrotScope_alloc_ctx_raw(ParrotScope *self, size_t size);
#define ParrotScope_get_ctx(self, type) ((type)ParrotScope_get_ctx_raw(self))
PARROT_API void *ParrotScope_get_ctx_raw(ParrotScope *self);

PARROT_API void ParrotScope_set_parent(ParrotScope *self, /* NULL = no parent */ ParrotScope *parent);

PARROT_API uint32_t ParrotScope_push(ParrotScope *self, void (*func)(void *ctx), void *ctx);

/// Pushes a free(ptr) function
PARROT_API uint32_t ParrotScope_push_free(ParrotScope *self, void *ptr);

#define ParrotScope_push_arrfree(p_self, p_arr) ParrotScope_push_arrfree_raw(p_self, (void **)&(p_arr), sizeof(*(p_arr)))
void ParrotScope_push_arrfree_raw(ParrotScope *self, void **arr, size_t element_size);

#define ParrotScope_push_hmfree(p_self, p_hm) ParrotScope_push_hmfree_raw(p_self, (void **)&(p_hm), sizeof(*(p_hm)))
void ParrotScope_push_hmfree_raw(ParrotScope *self, void **hm, size_t element_size);

#define ParrotScope_push_shfree_raw ParrotScope_push_hmfree_raw
#define ParrotScope_push_shfree ParrotScope_push_hmfree

/// Does not call pushed function. Does nothing on non-existant id
PARROT_API void ParrotScope_cancel(ParrotScope *self, uint32_t id);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum {
    ParrotReflectEntryType_END = 0,

    ParrotReflectEntryType_TYPE_HEADER = 0x100,
    ParrotReflectEntryType_TYPE_FIELD,

    ParrotReflectEntryType_ALIAS = 0x200,

    ParrotReflectEntryType_COLLECTION_HEADER = 0x300,
    ParrotReflectEntryType_COLLECTION_DESCRIPTION,
} ParrotReflectEntryType;

typedef struct ParrotReflectDescription ParrotReflectDescription;

struct ParrotReflectDescription {
    ParrotReflectEntryType type;

    const char *name;
    /**
     * NULL-terminated array or keys and values
     *
     * Example:
     * ```
     * const char *tags[][2] = {
     *     {"key", "value"},
     *     {NULL, NULL},
     * };
     * ```
     *
     */
    const char *(*tags)[2];

    union {
        struct {
            size_t size;
        } type_header_data;
        struct {
            size_t offset;

            size_t field_base_size;

            const char *type;
            const char *suffix;
        } type_field_data;

        struct {
            const char *name;

            const char *alias_of_name;
            size_t size;
        } alias_data;

        struct {
            const ParrotReflectDescription *description;
        } collection_description_data;
    } unique_data;
};

#ifndef __cplusplus
#define PARROT_REFLECT_END()                                                                                            \
    {                                                                                                                   \
        .type = ParrotReflectEntryType_END,                                                                             \
        .name = NULL,                                                                                                   \
        .tags = NULL,                                                                                                   \
    }
#else
#define PARROT_REFLECT_END()                                                                                            \
    {                                                                                                                   \
        .type = ParrotReflectEntryType_END,                                                                             \
        .name = NULL,                                                                                                   \
        .tags = NULL,                                                                                                   \
        .unique_data = {},                                                                                              \
    }
#endif

#define PARROT_REFLECT_TYPE_HEADER(p_type)                                                                              \
    {                                                                                                                   \
        .type = ParrotReflectEntryType_TYPE_HEADER,                                                                     \
        .name = PARROT_STRING(p_type),                                                                                  \
        .tags = NULL,                                                                                                   \
        .unique_data =                                                                                                  \
            {                                                                                                           \
                .type_header_data =                                                                                     \
                    {                                                                                                   \
                        .size = sizeof(p_type),                                                                         \
                    },                                                                                                  \
            },                                                                                                          \
    }
#define PARROT_REFLECT_TYPE_HEADER_TAG(p_type, p_tags)                                                                  \
    {                                                                                                                   \
        .type = ParrotReflectEntryType_TYPE_HEADER,                                                                     \
        .name = PARROT_STRING(p_type),                                                                                  \
        .tags = p_tags,                                                                                                 \
        .unique_data =                                                                                                  \
            {                                                                                                           \
                .type_header_data =                                                                                     \
                    {                                                                                                   \
                        .size = sizeof(p_type),                                                                         \
                    },                                                                                                  \
            },                                                                                                          \
    }
#define PARROT_REFLECT_TYPE_FIELD(p_type, p_field_type, p_field_name, p_field_suffix)                                   \
    {                                                                                                                   \
        .type = ParrotReflectEntryType_TYPE_FIELD,                                                                      \
        .name = PARROT_STRING(p_field_name),                                                                            \
        .tags = NULL,                                                                                                   \
        .unique_data =                                                                                                  \
            {                                                                                                           \
                .type_field_data =                                                                                      \
                    {                                                                                                   \
                        .offset = offsetof(p_type, p_field_name),                                                       \
                        .field_base_size = sizeof(p_field_type),                                                        \
                        .type = PARROT_STRING(p_field_type),                                                            \
                        .suffix = PARROT_STRING(p_field_suffix),                                                        \
                    },                                                                                                  \
            },                                                                                                          \
    }
#define PARROT_REFLECT_TYPE_FIELD_TAG(p_type, p_field_type, p_field_name, p_field_suffix, p_tags)                       \
    {                                                                                                                   \
        .type = ParrotReflectEntryType_TYPE_FIELD,                                                                      \
        .name = PARROT_STRING(p_field_name),                                                                            \
        .tags = p_tags,                                                                                                 \
        .unique_data =                                                                                                  \
            {                                                                                                           \
                .type_field_data =                                                                                      \
                    {                                                                                                   \
                        .offset = offsetof(p_type, p_field_name),                                                       \
                        .field_base_size = sizeof(p_field_type),                                                        \
                        .type = PARROT_STRING(p_field_type),                                                            \
                        .suffix = PARROT_STRING(p_field_suffix),                                                        \
                    },                                                                                                  \
            },                                                                                                          \
    }

/// Immediately acts as the end as well
#define PARROT_REFLECT_ALIAS(p_type, p_alias_of)                                                                        \
    {                                                                                                                   \
        .type = ParrotReflectEntryType_ALIAS,                                                                           \
        .name = PARROT_STRING(p_type),                                                                                  \
        .tags = NULL,                                                                                                   \
        .unique_data =                                                                                                  \
            {                                                                                                           \
                .alias_data =                                                                                           \
                    {                                                                                                   \
                        .alias_of_name = PARROT_STRING(p_alias_of),                                                     \
                        .size = sizeof(p_alias_of),                                                                     \
                    },                                                                                                  \
            },                                                                                                          \
    }

#ifndef __cplusplus
#define PARROT_REFLECT_COLLECTION_HEADER()                                                                              \
    {                                                                                                                   \
        .type = ParrotReflectEntryType_COLLECTION_HEADER,                                                               \
        .name = NULL,                                                                                                   \
        .tags = NULL,                                                                                                   \
    }
#else
#define PARROT_REFLECT_COLLECTION_HEADER()                                                                              \
    {                                                                                                                   \
        .type = ParrotReflectEntryType_COLLECTION_HEADER,                                                               \
        .name = NULL,                                                                                                   \
        .tags = NULL,                                                                                                   \
        .unique_data = {},                                                                                              \
    }
#endif
#define PARROT_REFLECT_COLLECTION_DESCRIPTION(p_description)                                                            \
    {                                                                                                                   \
        .type = ParrotReflectEntryType_COLLECTION_DESCRIPTION,                                                          \
        .name = NULL,                                                                                                   \
        .tags = NULL,                                                                                                   \
        .unique_data =                                                                                                  \
            {                                                                                                           \
                .collection_description_data =                                                                          \
                    {                                                                                                   \
                        .description = p_description,                                                                   \
                    },                                                                                                  \
            },                                                                                                          \
    }

typedef struct ParrotReflect ParrotReflect;

PARROT_API ParrotReflect *ParrotReflect_new(void);
PARROT_API void ParrotReflect_delete(ParrotReflect *self);
PARROT_API void ParrotReflect_vdelete(void *self);

PARROT_API void ParrotReflect_register(ParrotReflect *self, const ParrotReflectDescription *description);
/// Does not error on unregister of not registered type
PARROT_API void ParrotReflect_unregister(ParrotReflect *self, const char *type);

PARROT_API size_t ParrotReflect_get_type_count(ParrotReflect *self);

PARROT_API char *ParrotReflect_parse_type(const char *type,
                                          /* NULL = unwritten */ size_t *out_ptr_level,
                                          /* NULL = unwritten */ bool *out_is_const);

#define ParrotReflect_resolve_type(self, type) ParrotReflect_resolve_type_ex(self, type, NULL, NULL, NULL)
/// 0< = Not found
PARROT_API ptrdiff_t
ParrotReflect_resolve_type_ex(ParrotReflect *self,
                              const char *type,
                              /* NULL = unwritten. See `ParrotReflect_parse_type` return */ char **out_parsed_type,
                              /* NULL = unwritten */ size_t *out_ptr_level,
                              /* NULL = unwritten */ bool *out_is_const);
/// 0< = Not found
PARROT_API ptrdiff_t ParrotReflect_resolve_type_by_index(ParrotReflect *self, size_t index);

/// Return type is allocated with malloc() that the caller tkes ownership of
PARROT_API char *ParrotReflect_get_type_name(ParrotReflect *self, size_t type);
PARROT_API size_t ParrotReflect_get_type_size(ParrotReflect *self, size_t type);
/// Return type is allocated with malloc() that the caller takes ownership of or NULL if tag doesn't exist
PARROT_API char *ParrotReflect_get_type_tag(ParrotReflect *self, size_t type, const char *key);

/// 0< = Not found
PARROT_API ptrdiff_t ParrotReflect_get_type_field(ParrotReflect *self, size_t type, const char *name);
PARROT_API size_t ParrotReflect_get_type_field_count(ParrotReflect *self, size_t type);
PARROT_API size_t ParrotReflect_get_type_field_offset(ParrotReflect *self, size_t type, size_t field);
/// Return type is allocated with malloc() that the caller tkes ownership of
PARROT_API char *ParrotReflect_get_type_field_typename(ParrotReflect *self, size_t type, size_t field);
/// Return type is allocated with malloc() that the caller takes ownership of
PARROT_API char *ParrotReflect_get_type_field_name(ParrotReflect *self, size_t type, size_t field);
/*
 * @returns name of field without array index allocated with malloc() that the caller takes ownership of. Result can't
 * be used as a field lookup
 */
PARROT_API char *ParrotReflect_get_type_field_basename(ParrotReflect *self, size_t type, size_t field);
PARROT_API size_t ParrotReflect_get_type_field_size(ParrotReflect *self, size_t type, size_t field);
PARROT_API size_t ParrotReflect_get_type_field_array_index(ParrotReflect *self, size_t type, size_t field);
/// Returns 0 if no array
PARROT_API size_t ParrotReflect_get_type_field_array_size(ParrotReflect *self, size_t type, size_t field);
/// Return type is allocated with malloc() that the caller takes ownership of or NULL if tag doesn't exist
PARROT_API char *ParrotReflect_get_type_field_tag(ParrotReflect *self, size_t type, size_t field, const char *key);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PARROT_CORE_MATH

/// 48.16 fixed point
typedef int64_t ParrotFixed64i;
#define ParrotFixed64i_MIN ((int64_t)-281474976710657)
#define ParrotFixed64i_MAX ((int64_t)281474976710657)
#define ParrotFixed64i_to_double(n) ((double)(n) / 65536.0f)
#define ParrotFixed64i_from_double(n)                                                                                   \
    ((ParrotFixed64i)(PARROT_CLAMP(ParrotFixed64i_MIN, n, ParrotFixed64i_MAX) * 65536.0f + 0.5f) - ((n) < 0))

/// 32.32 fixed point
typedef int64_t ParrotFixed64s;
#define ParrotFixed64s_MIN ((int64_t)-2147483647)
#define ParrotFixed64s_MAX ((int64_t)2147483647)
#define ParrotFixed64s_to_double(n) ((double)(n) / 2147483648.0f)
#define ParrotFixed64s_from_double(n)                                                                                   \
    ((ParrotFixed64s)(PARROT_CLAMP(ParrotFixed64s_MIN, n, ParrotFixed64s_MAX) * 2147483648.0f + 0.5f) - ((n) < 0))

/// 16.48 fixed point
typedef int64_t ParrotFixed64f;
#define ParrotFixed64f_MIN ((int64_t)-65535)
#define ParrotFixed64f_MAX ((int64_t)65535)
#define ParrotFixed64f_to_double(n) ((double)(n) / 281474976710658.0f)
#define ParrotFixed64f_from_double(n)                                                                                   \
    ((ParrotFixed64f)(PARROT_CLAMP(ParrotFixed64f_MIN, n, ParrotFixed64f_MAX) * 281474976710658.0f + 0.5f) - ((n) < 0))

/// 24.8 fixed point
typedef int32_t ParrotFixed32i;
#define ParrotFixed32i_MIN ((int32_t)-8388607)
#define ParrotFixed32i_MAX ((int32_t)8388607)
#define ParrotFixed32i_to_float(n) ((float)(n) / 256.0f)
#define ParrotFixed32i_from_float(n)                                                                                    \
    ((ParrotFixed32i)(PARROT_CLAMP(ParrotFixed32i_MIN, n, ParrotFixed32i_MAX) * 256.0f + 0.5f) - ((n) < 0))

/// 16.16 fixed point
typedef int32_t ParrotFixed32s;
#define ParrotFixed32s_MIN ((int32_t)-32767)
#define ParrotFixed32s_MAX ((int32_t)32767)
#define ParrotFixed32s_to_float(n) ((float)(n) / 65536.0f)
#define ParrotFixed32s_from_float(n)                                                                                    \
    ((ParrotFixed32s)(PARROT_CLAMP(ParrotFixed32s_MIN, n, ParrotFixed32s_MAX) * 65536.0f + 0.5f) - ((n) < 0))

/// 8.24 fixed point
typedef int32_t ParrotFixed32f;
#define ParrotFixed32f_MIN ((int32_t)-127)
#define ParrotFixed32f_MAX ((int32_t)127)
#define ParrotFixed32f_to_float(n) ((float)(n) / 8388608.0f)
#define ParrotFixed32f_from_float(n)                                                                                    \
    ((ParrotFixed32f)(PARROT_CLAMP(ParrotFixed32f_MIN, n, ParrotFixed32f_MAX) * 8388608.0f + 0.5f) - ((n) < 0))

/// 12.4 fixed point
typedef int16_t ParrotFixed16i;
#define ParrotFixed16i_MIN ((int16_t)-4095)
#define ParrotFixed16i_MAX ((int16_t)4095)
#define ParrotFixed16i_to_float(n) ((float)(n) / 16.0f)
#define ParrotFixed16i_from_float(n)                                                                                    \
    ((ParrotFixed16i)(PARROT_CLAMP(ParrotFixed16i_MIN, n, ParrotFixed16i_MAX) * 16.0f + 0.5f) - ((n) < 0))

/// 8.8 fixed point
typedef int16_t ParrotFixed16s;
#define ParrotFixed16s_MIN ((int16_t)-255)
#define ParrotFixed16s_MAX ((int16_t)255)
#define ParrotFixed16s_to_float(n) ((float)(n) / 256.0f)
#define ParrotFixed16s_from_float(n)                                                                                    \
    ((ParrotFixed16s)(PARROT_CLAMP(ParrotFixed16s_MIN, n, ParrotFixed16s_MAX) * 256.0f + 0.5f) - ((n) < 0))

/// 4.12 fixed point
typedef int16_t ParrotFixed16f;
#define ParrotFixed16f_MIN ((int16_t)-15)
#define ParrotFixed16f_MAX ((int16_t)15)
#define ParrotFixed16f_to_float(n) ((float)(n) / 4096.0f)
#define ParrotFixed16f_from_float(n)                                                                                    \
    ((ParrotFixed16f)(PARROT_CLAMP(ParrotFixed16f_MIN, n, ParrotFixed16f_MAX) * 4096.0f + 0.5f) - ((n) < 0))

#ifndef PARROT_DOUBLE_PRECISION
#define ParrotReal float
#define ParrotReal_sqrt sqrtf
#define ParrotReal_sin sinf
#define ParrotReal_cos cosf
#define ParrotReal_atan2 atan2f
#else
#define ParrotReal double
#define ParrotReal_sqrt sqrt
#define ParrotReal_sin sin
#define ParrotReal_cos cos
#define ParrotReal_atan2 atan2
#endif

static const ParrotReflectDescription ParrotReal_description[] = {
#ifndef PARROT_DOUBE_PRECISION
    PARROT_REFLECT_ALIAS(ParrotReal, float),
#else
    PARROT_REFLECT_ALIAS(ParrotReal, double),
#endif
};

#define PARROT_CLAMP(min, value, max) PARROT_MAX(min, PARROT_MIN(value, max))
#define PARROT_MIN(a, b) ((a) > (b) ? (b) : (a))
#define PARROT_MAX(a, b) ((a) > (b) ? (a) : (b))

PARROT_API ParrotReal Parrot_lerp(ParrotReal a, ParrotReal b, ParrotReal t);
PARROT_API float Parrot_lerpf(float a, float b, float t);
PARROT_API double Parrot_lerpd(double a, double b, double t);

PARROT_API void ParrotReal_to_float_array(const ParrotReal *src, float *dest, size_t count);

typedef struct {
    ParrotReal x;
    ParrotReal y;
} ParrotVec2;

static const ParrotReflectDescription ParrotVec2_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotVec2),

    PARROT_REFLECT_TYPE_FIELD(ParrotVec2, ParrotReal, x, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotVec2, ParrotReal, y, ),

    PARROT_REFLECT_END(),
};

PARROT_API ParrotVec2 ParrotVec2_n(ParrotReal n);

PARROT_API ParrotVec2 ParrotVec2_add(ParrotVec2 a, ParrotVec2 b);
PARROT_API ParrotVec2 ParrotVec2_sub(ParrotVec2 a, ParrotVec2 b);
PARROT_API ParrotVec2 ParrotVec2_mul(ParrotVec2 a, ParrotVec2 b);
PARROT_API ParrotVec2 ParrotVec2_div(ParrotVec2 a, ParrotVec2 b);
PARROT_API ParrotVec2 ParrotVec2_scale(ParrotVec2 a, ParrotReal b);

PARROT_API ParrotVec2 ParrotVec2_normalize(ParrotVec2 self);

PARROT_API ParrotReal ParrotVec2_length(ParrotVec2 self);
PARROT_API ParrotReal ParrotVec2_dot(ParrotVec2 self, ParrotVec2 other);

typedef struct {
    ParrotReal x;
    ParrotReal y;
    ParrotReal z;
} ParrotVec3;

static const ParrotReflectDescription ParrotVec3_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotVec3),

    PARROT_REFLECT_TYPE_FIELD(ParrotVec3, ParrotReal, x, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotVec3, ParrotReal, y, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotVec3, ParrotReal, z, ),

    PARROT_REFLECT_END(),
};

PARROT_API ParrotVec3 ParrotVec3_n(ParrotReal n);
PARROT_API ParrotVec3 ParrotVec3_upgrade(ParrotVec2 v);

PARROT_API ParrotVec3 ParrotVec3_add(ParrotVec3 a, ParrotVec3 b);
PARROT_API ParrotVec3 ParrotVec3_sub(ParrotVec3 a, ParrotVec3 b);
PARROT_API ParrotVec3 ParrotVec3_mul(ParrotVec3 a, ParrotVec3 b);
PARROT_API ParrotVec3 ParrotVec3_div(ParrotVec3 a, ParrotVec3 b);
PARROT_API ParrotVec3 ParrotVec3_scale(ParrotVec3 a, ParrotReal b);

PARROT_API ParrotVec3 ParrotVec3_normalize(ParrotVec3 self);

PARROT_API ParrotReal ParrotVec3_length(ParrotVec3 self);
PARROT_API ParrotReal ParrotVec3_dot(ParrotVec3 self, ParrotVec3 other);

typedef struct {
    ParrotReal x;
    ParrotReal y;
    ParrotReal z;
    ParrotReal w;
} ParrotVec4;

static const ParrotReflectDescription ParrotVec4_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotVec4),

    PARROT_REFLECT_TYPE_FIELD(ParrotVec4, ParrotReal, x, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotVec4, ParrotReal, y, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotVec4, ParrotReal, z, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotVec4, ParrotReal, w, ),

    PARROT_REFLECT_END(),
};

PARROT_API ParrotVec4 ParrotVec4_n(ParrotReal n);
PARROT_API ParrotVec4 ParrotVec4_upgrade(ParrotVec3 v);

PARROT_API ParrotVec4 ParrotVec4_add(ParrotVec4 a, ParrotVec4 b);
PARROT_API ParrotVec4 ParrotVec4_sub(ParrotVec4 a, ParrotVec4 b);
PARROT_API ParrotVec4 ParrotVec4_mul(ParrotVec4 a, ParrotVec4 b);
PARROT_API ParrotVec4 ParrotVec4_div(ParrotVec4 a, ParrotVec4 b);
PARROT_API ParrotVec4 ParrotVec4_scale(ParrotVec4 a, ParrotReal b);

PARROT_API ParrotVec4 ParrotVec4_normalize(ParrotVec4 self);

PARROT_API ParrotReal ParrotVec4_length(ParrotVec4 self);
PARROT_API ParrotReal ParrotVec4_dot(ParrotVec4 self, ParrotVec4 other);

typedef struct {
    // Column-major (accessed like data[x][y])
    ParrotReal data[4][4];
} ParrotMat;

static const ParrotReflectDescription ParrotMat_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotMat),

    PARROT_REFLECT_TYPE_FIELD(ParrotMat, ParrotReal, data, [4][4]),

    PARROT_REFLECT_END(),
};

PARROT_API ParrotMat ParrotMat_identity(void);

PARROT_API ParrotMat ParrotMat_inverse(ParrotMat matrix);
PARROT_API ParrotMat ParrotMat_transpose(ParrotMat matrix);

PARROT_API ParrotMat ParrotMat_add(ParrotMat a, ParrotMat b);
PARROT_API ParrotMat ParrotMat_mul(ParrotMat a, ParrotMat b);

PARROT_API ParrotVec3 ParrotMat_transform3(ParrotMat matrix, ParrotVec3 vec);
PARROT_API ParrotVec2 ParrotMat_transform2(ParrotMat matrix, ParrotVec2 vec);

PARROT_API ParrotMat
ParrotMat_ortho(ParrotReal left, ParrotReal right, ParrotReal down, ParrotReal up, ParrotReal near, ParrotReal far);

PARROT_API ParrotMat ParrotMat_translation(ParrotVec3 position);
PARROT_API ParrotMat ParrotMat_rotation(ParrotVec3 rotation);
PARROT_API ParrotMat ParrotMat_scale(ParrotVec3 scale);

typedef struct {
    ParrotMat model;
    ParrotMat view;
    ParrotMat projection;
} ParrotGMatSet;

static const ParrotReflectDescription ParrotGMatSet_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotGMatSet),

    PARROT_REFLECT_TYPE_FIELD(ParrotGMatSet, ParrotMat, model, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotGMatSet, ParrotMat, view, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotGMatSet, ParrotMat, projection, ),

    PARROT_REFLECT_END(),
};

typedef struct ParrotTransform ParrotTransform;

struct ParrotTransform {
    ParrotTransform *parent;
    ParrotMat matrix;

    ParrotVec3 position;
    ParrotVec3 rotation;
    ParrotVec3 scale;
};

static const ParrotReflectDescription ParrotTransform_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotTransform),

    PARROT_REFLECT_TYPE_FIELD(ParrotTransform, ParrotTransform *, parent, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotTransform, ParrotMat, matrix, ),

    PARROT_REFLECT_TYPE_FIELD(ParrotTransform, ParrotVec3, position, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotTransform, ParrotVec3, rotation, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotTransform, ParrotVec3, scale, ),

    PARROT_REFLECT_END(),
};

ParrotTransform ParrotTransform_new(void);

ParrotMat ParrotTransform_calculate_matrix(const ParrotTransform *self);

PARROT_API ParrotMat ParrotGMatSet_combine(const ParrotGMatSet *self);

typedef enum {
    ParrotColorFormat_RGBA8888 = 0,
    ParrotColorFormat_BGRA8888,
} ParrotColorFormat;

typedef struct {
    // [0.0, 1.0]
    float r;
    // [0.0, 1.0]
    float g;
    // [0.0, 1.0]
    float b;
    // [0.0, 1.0]
    float a;
} ParrotColor;

static const ParrotReflectDescription ParrotColor_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotColor),

    PARROT_REFLECT_TYPE_FIELD(ParrotColor, float, r, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotColor, float, g, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotColor, float, b, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotColor, float, a, ),

    PARROT_REFLECT_END(),
};

PARROT_API ParrotColor ParrotColor_new(uint8_t r, uint8_t g, uint8_t b);
PARROT_API ParrotColor ParrotColor_newf(float r, float g, float b);
PARROT_API ParrotColor ParrotColor_newa(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
PARROT_API ParrotColor ParrotColor_newaf(float r, float g, float b, float a);

PARROT_API ParrotColor ParrotColor_from(ParrotColorFormat format, uint32_t color);
PARROT_API uint32_t ParrotColor_to(ParrotColor self, ParrotColorFormat format);

PARROT_API ParrotColor ParrotColor_mul(ParrotColor a, ParrotColor b);
PARROT_API ParrotColor ParrotColor_blend(ParrotColor a, ParrotColor b);
PARROT_API ParrotColor ParrotColor_lerp(ParrotColor a, ParrotColor b, float t);

#define ParrotColor_CLEAR ((ParrotColor){.r = 0.0, .g = 0.0, .b = 0.0, .a = 0.0})
#define ParrotColor_BLACK ((ParrotColor){.r = 0.0, .g = 0.0, .b = 0.0, .a = 1.0})
#define ParrotColor_WHITE ((ParrotColor){.r = 1.0, .g = 1.0, .b = 1.0, .a = 1.0})
#define ParrotColor_GRAY ((ParrotColor){.r = 0.5, .g = 0.5, .b = 0.5, .a = 1.0})
#define ParrotColor_GREY ParrotColor_GRAY
#define ParrotColor_RED ((ParrotColor){.r = 1.0, .g = 0.0, .b = 0.0, .a = 1.0})
#define ParrotColor_GREEN ((ParrotColor){.r = 0.0, .g = 1.0, .b = 0.0, .a = 1.0})
#define ParrotColor_BLUE ((ParrotColor){.r = 0.0, .g = 0.0, .b = 1.0, .a = 1.0})
#define ParrotColor_YELLOW ((ParrotColor){.r = 1.0, .g = 1.0, .b = 0.0, .a = 1.0})
#define ParrotColor_ORANGE ((ParrotColor){.r = 1.0, .g = 0.3, .b = 0.0, .a = 1.0})
#define ParrotColor_CYAN ((ParrotColor){.r = 0.0, .g = 1.0, .b = 1.0, .a = 1.0})

static const ParrotReflectDescription Parrot_core_math_collection[] = {
    PARROT_REFLECT_COLLECTION_HEADER(),

    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotVec2_description),
    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotVec3_description),
    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotVec4_description),

    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotMat_description),
    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotGMatSet_description),

    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotTransform_description),

    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotColor_description),

    PARROT_REFLECT_END(),
};

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PARROT_CORE_HASH

typedef uint32_t ParrotCRC32;

PARROT_API ParrotCRC32 Parrot_crc32(const void *data, size_t size);
PARROT_API ParrotCRC32 Parrot_crc32_combine(ParrotCRC32 crc, const void *data, size_t size);

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PARROT_CORE_BINARY

typedef bool (*ParrotBufferRead)(ParrotScope *scope, size_t offset, uint8_t *out);
typedef void (*ParrotBufferWrite)(ParrotScope *scope, uint8_t data);

typedef enum {
    ParrotBufferEndian_HOST = 0,
    ParrotBufferEndian_BIG,
    ParrotBufferEndian_LITTLE,
} ParrotBufferEndian;

typedef struct ParrotBuffer ParrotBuffer;

PARROT_API ParrotBuffer *ParrotBuffer_new(/* Auto-deleted at end if not NULL */ ParrotScope *scope,
                                          /* NULL = no read */ ParrotBufferRead read,
                                          /* NULL = no write */ ParrotBufferWrite write);
PARROT_API ParrotBuffer *ParrotBuffer_new_file(FILE *file);
PARROT_API ParrotBuffer *ParrotBuffer_new_bytearray(/* Auto-deleted at end if not NULL */ ParrotScope *scope,
                                                    const void **p_data,
                                                    size_t size,
                                                    /* NULL = no write */ ParrotBufferWrite write);
#define ParrotBuffer_new_stbds_array(p_arr_data) ParrotBuffer_new_stbds_array_raw(&(p_arr_data));
ParrotBuffer *ParrotBuffer_new_stbds_array_raw(uint8_t **p_arr_data);
PARROT_API void ParrotBuffer_delete(ParrotBuffer *self);
PARROT_API void ParrotBuffer_vdelete(void *self);

PARROT_API void ParrotBuffer_rseek(ParrotBuffer *self, size_t position);
PARROT_API size_t ParrotBuffer_rtell(ParrotBuffer *self);
PARROT_API void ParrotBuffer_pad(ParrotBuffer *self, uint8_t data, size_t count);
PARROT_API void ParrotBuffer_pad_until(ParrotBuffer *self, uint8_t data, size_t until_position);

PARROT_API size_t ParrotBuffer_read(ParrotBuffer *self, ParrotBufferEndian endian, void *out, size_t size);
PARROT_API void ParrotBuffer_write(ParrotBuffer *self, ParrotBufferEndian endian, const void *data, size_t size);

PARROT_API bool ParrotBuffer_read8(ParrotBuffer *self, uint8_t *out);
PARROT_API bool ParrotBuffer_read16(ParrotBuffer *self, ParrotBufferEndian endian, uint16_t *out);
PARROT_API bool ParrotBuffer_read32(ParrotBuffer *self, ParrotBufferEndian endian, uint32_t *out);
PARROT_API bool ParrotBuffer_read64(ParrotBuffer *self, ParrotBufferEndian endian, uint64_t *out);

PARROT_API void ParrotBuffer_write8(ParrotBuffer *self, uint8_t data);
PARROT_API void ParrotBuffer_write16(ParrotBuffer *self, ParrotBufferEndian endian, uint16_t data);
PARROT_API void ParrotBuffer_write32(ParrotBuffer *self, ParrotBufferEndian endian, uint32_t data);
PARROT_API void ParrotBuffer_write64(ParrotBuffer *self, ParrotBufferEndian endian, uint64_t data);

PARROT_API bool ParrotBuffer_read8s(ParrotBuffer *self, int8_t *out);
PARROT_API bool ParrotBuffer_read16s(ParrotBuffer *self, ParrotBufferEndian endian, int16_t *out);
PARROT_API bool ParrotBuffer_read32s(ParrotBuffer *self, ParrotBufferEndian endian, int32_t *out);
PARROT_API bool ParrotBuffer_read64s(ParrotBuffer *self, ParrotBufferEndian endian, int64_t *out);

PARROT_API void ParrotBuffer_write8s(ParrotBuffer *self, int8_t data);
PARROT_API void ParrotBuffer_write16s(ParrotBuffer *self, ParrotBufferEndian endian, int16_t data);
PARROT_API void ParrotBuffer_write32s(ParrotBuffer *self, ParrotBufferEndian endian, int32_t data);
PARROT_API void ParrotBuffer_write64s(ParrotBuffer *self, ParrotBufferEndian endian, int64_t data);

PARROT_API void ParrotBuffer_write_ascii(ParrotBuffer *self, const char *str);

typedef struct {
    uint8_t *data;
    size_t size;
} ParrotMutableBinaryImage;

static const ParrotReflectDescription ParrotMutableBinaryImage_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotMutableBinaryImage),

    PARROT_REFLECT_TYPE_FIELD(ParrotMutableBinaryImage, uint8_t *, data, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotMutableBinaryImage, size_t, size, ),

    PARROT_REFLECT_END(),
};

typedef struct {
    const uint8_t *data;
    size_t size;
} ParrotBinaryImage;

PARROT_API ParrotBinaryImage ParrotBinaryImage_from_mutable(ParrotMutableBinaryImage image);

static const ParrotReflectDescription ParrotBinaryImage_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotBinaryImage),

    PARROT_REFLECT_TYPE_FIELD(ParrotBinaryImage, const uint8_t *, data, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotBinaryImage, size_t, size, ),

    PARROT_REFLECT_END(),
};

/**
 * The C99 standard does not guarntee ASCII repersentation of `char`. While most platforms do use ASCII for `char`, it
 * is not guarnteed.
 */
PARROT_API uint8_t Parrot_char_to_ascii(char c);
/**
 * The C99 standard does not guarntee ASCII repersentation of `char`. While most platforms do use ASCII for `char`, it
 * is not guarnteed.
 */
PARROT_API char Parrot_ascii_to_char(uint8_t ascii);

static const ParrotReflectDescription Parrot_core_binary_collection[] = {
    PARROT_REFLECT_COLLECTION_HEADER(),

    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotMutableBinaryImage_description),
    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotBinaryImage_description),

    PARROT_REFLECT_END(),
};

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PARROT_CORE_TIMING

PARROT_API uint64_t Parrot_get_performance_counter(void);
PARROT_API uint64_t Parrot_get_performance_frequency(void);

PARROT_API void Parrot_sleep(float seconds);

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PARROT_CORE_MAIN_LOOP

typedef struct {
    void *user_data;

    // <=0 = Uncapped
    float max_fps;
} ParrotMainLoopRunSettings;

static const ParrotReflectDescription ParrotMainLoopRunSettings_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotMainLoopRunSettings),

    PARROT_REFLECT_TYPE_FIELD(ParrotMainLoopRunSettings, void *, user_data, ),

    PARROT_REFLECT_TYPE_FIELD(ParrotMainLoopRunSettings, float, max_fps, ),

    PARROT_REFLECT_END(),
};

typedef void (*ParrotMainLoopInitFunc)(ParrotMainLoopRunSettings *settings);
/// @return If the application should close
typedef bool (*ParrotMainLoopUpdateFunc)(ParrotMainLoopRunSettings *settings,
                                         float delta,
                                         /* Clears on next frame */ bool should_close);
typedef void (*ParrotMainLoopRenderFunc)(ParrotMainLoopRunSettings *settings);
typedef void (*ParrotMainLoopShutdownFunc)(ParrotMainLoopRunSettings *settings);

typedef struct ParrotMainLoop ParrotMainLoop;

PARROT_API ParrotMainLoop *ParrotMainLoop_new(void);
PARROT_API void ParrotMainLoop_delete(ParrotMainLoop *self);
PARROT_API void ParrotMainLoop_vdelete(void *self);

/// Clears on next frame
PARROT_API void ParrotMainLoop_request_close(ParrotMainLoop *self);
/// @returns If application should keep running. Clears after first return
PARROT_API bool ParrotMainLoop_next_frame(ParrotMainLoop *self,
                                          /* NULL = Unmodified */ float *delta,
                                          /* <=0 = Uncapped */ float max_fps);

/// Makes the main loop handle your program's lifecycle. Only returns after program shuts down
PARROT_API void ParrotMainLoop_run(ParrotMainLoop *self,
                                   void *user_data,
                                   /* NULL = uncalled */ ParrotMainLoopInitFunc init,
                                   /* Required */ ParrotMainLoopUpdateFunc update,
                                   /* NULL = uncalled */ ParrotMainLoopRenderFunc render,
                                   /* NULL = uncalled */ ParrotMainLoopShutdownFunc shutdown);

static const ParrotReflectDescription Parrot_core_main_loop_collection[] = {
    PARROT_REFLECT_COLLECTION_HEADER(),

    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotMainLoopRunSettings_description),

    PARROT_REFLECT_END(),
};

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PARROT_CORE_SERIALIZE

PARROT_API void
Parrot_serialize_bytes(ParrotBuffer *output, ParrotReflect *reflect, size_t type, const void *data, bool with_ptrs);
/// Return type is allocated with malloc() that the caller takes ownership of or NULL if could not be deserialized
PARROT_API void *Parrot_deserialize_bytes(ParrotBuffer *input, ParrotReflect *reflect, size_t *out_type, bool with_ptrs);

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const ParrotReflectDescription Parrot_core_collection[] = {
    PARROT_REFLECT_COLLECTION_HEADER(),

#ifdef PARROT_CORE_MATH
    PARROT_REFLECT_COLLECTION_DESCRIPTION(Parrot_core_math_collection),
#endif
#ifdef PARROT_CORE_BINARY
    PARROT_REFLECT_COLLECTION_DESCRIPTION(Parrot_core_binary_collection),
#endif
#ifdef PARROT_CORE_MAIN_LOOP
    PARROT_REFLECT_COLLECTION_DESCRIPTION(Parrot_core_main_loop_collection),
#endif

    PARROT_REFLECT_END(),
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PARROT_CORE_IMPL
#ifndef __cplusplus

#include <ctype.h>
#include <math.h>

#ifdef PARROT_CORE_IMPL_CORE

static void Parrot_crash_handler_impl(const char *cause) {
    (void)cause;
    abort();
}

ParrotCrashHandlerFunc Parrot_crash_handler = Parrot_crash_handler_impl;

typedef struct {
    uint32_t key;

    void (*func)(void *ctx);
    void *ctx;
} ParrotScopeEntry;

struct ParrotScope {
    ParrotScopeEntry *hm_stack;
    uint32_t next_id;

    ParrotScope *parent;
    uint32_t parent_delete_id;

    void *ctx;
};

ParrotScope *ParrotScope_new(ParrotScope *parent) {
    ParrotScope *self = PARROT_ALLOC(ParrotScope);

    ParrotScope_set_parent(self, parent);

    return self;
}

void ParrotScope_delete(ParrotScope *self) {
    PARROT_FAIL_NULL(self);

    if (self->parent) {
        ParrotScope_cancel(self->parent, self->parent_delete_id);
    }

    while (hmlen(self->hm_stack) > 0) {
        size_t index = hmlen(self->hm_stack) - 1;
        ParrotScopeEntry entry = self->hm_stack[index];
        hmdel(self->hm_stack, entry.key);

        entry.func(entry.ctx);
    }

    hmfree(self->hm_stack);
    free(self);
}

void ParrotScope_vdelete(void *self) {
    ParrotScope_delete((ParrotScope *)self);
}

void ParrotScope_set_ctx(ParrotScope *self, void *ctx) {
    PARROT_FAIL_NULL(self);

    self->ctx = ctx;
}

void *ParrotScope_alloc_ctx_raw(ParrotScope *self, size_t size) {
    PARROT_FAIL_NULL(self);

    self->ctx = malloc(size);
    memset(self->ctx, 0, size);

    ParrotScope_push_free(self, self->ctx);
    return self->ctx;
}

void *ParrotScope_get_ctx_raw(ParrotScope *self) {
    PARROT_FAIL_NULL(self);
    return self->ctx;
}

static void scope_delete_wrapper(void *ctx) {
    ParrotScope_delete(ctx);
}

void ParrotScope_set_parent(ParrotScope *self, ParrotScope *parent) {
    PARROT_FAIL_NULL(self);

    if (self->parent) {
        ParrotScope_cancel(self->parent, self->parent_delete_id);
    }

    self->parent = parent;
    if (parent) {
        self->parent_delete_id = ParrotScope_push(parent, scope_delete_wrapper, self);
    }
}

uint32_t ParrotScope_push(ParrotScope *self, void (*func)(void *ctx), void *ctx) {
    PARROT_FAIL_NULL(self);

    ParrotScopeEntry entry = {0};
    entry.key = self->next_id++;
    entry.func = func;
    entry.ctx = ctx;

    hmputs(self->hm_stack, entry);
    return entry.key;
}

static void free_wrapper(void *ctx) {
    free(ctx);
}

uint32_t ParrotScope_push_free(ParrotScope *self, void *ptr) {
    return ParrotScope_push(self, free_wrapper, ptr);
}

typedef struct {
    void **data;
    size_t element_size;
} STBDSFreeCtx;

static void arrfree_wrapper(void *ctx_ptr) {
    STBDSFreeCtx *ctx = ctx_ptr;
    if (*ctx->data) {
        stbds_arrfreef(*ctx->data);
        *ctx->data = NULL;
    }
}

void ParrotScope_push_arrfree_raw(ParrotScope *self, void **arr, size_t element_size) {
    ParrotScope *scope = ParrotScope_new(self);
    STBDSFreeCtx *ctx = ParrotScope_alloc_ctx(scope, STBDSFreeCtx);
    ctx->data = arr;
    ctx->element_size = element_size;

    ParrotScope_push(scope, arrfree_wrapper, ctx);
}

static void hmfree_wrapper(void *ctx_ptr) {
    STBDSFreeCtx *ctx = ctx_ptr;
    if (*ctx->data) {
        stbds_hmfree_func(((uint8_t *)*ctx->data) - ctx->element_size, ctx->element_size);
        *ctx->data = NULL;
    }
}

void ParrotScope_push_hmfree_raw(ParrotScope *self, void **hm, size_t element_size) {
    ParrotScope *scope = ParrotScope_new(self);
    STBDSFreeCtx *ctx = ParrotScope_alloc_ctx(scope, STBDSFreeCtx);
    ctx->data = hm;
    ctx->element_size = element_size;

    ParrotScope_push(scope, hmfree_wrapper, ctx);
}

void ParrotScope_cancel(ParrotScope *self, uint32_t id) {
    PARROT_FAIL_NULL(self);

    hmdel(self->hm_stack, id);
}

#define FAIL_INVALID_TYPE(type) PARROT_FAIL_FMT("Unknown entry type: %d", type)
#define FAIL_REGISTERED_TYPE(type)                                                                                      \
    PARROT_FAIL_COND_MSG(ParrotReflect_resolve_type(self, type) >= 0, "Type is registered")
#define FAIL_HEADER() PARROT_FAIL_MSG("Attempt to use header twice")

typedef struct {
    char *key;
    char *value;
} ParrotReflectTag;

typedef struct {
    char *key;
    char *basename;

    ParrotScope *scope;

    size_t offset;

    char *type;
    ParrotReflectTag **sh_tags;

    size_t size;

    size_t array_index;
    size_t array_size;
} ParrotReflectTypeFieldInfo;

typedef struct {
    char *key;

    ParrotScope *scope;

    ParrotReflectTag **sh_tags;

    size_t size;

    ParrotReflectTypeFieldInfo **sh_fields;
} ParrotReflectTypeInfo;

typedef struct {
    char *key;

    ParrotScope *scope;

    char *value;
} ParrotReflectAliasInfo;

struct ParrotReflect {
    ParrotScope *scope;

    ParrotReflectTypeInfo *sh_types;
    ParrotReflectAliasInfo *sh_aliases;
};

#define BUILTIN_TYPES                                                                                                   \
    X(char, char)                                                                                                       \
    X(int, int)                                                                                                         \
    X(short, short)                                                                                                     \
    X(long, long)                                                                                                       \
    X(ll, long long)                                                                                                    \
    X(uchar, unsigned char)                                                                                             \
    X(uint, unsigned int)                                                                                               \
    X(ushort, unsigned short)                                                                                           \
    X(ulong, unsigned long)                                                                                             \
    X(ull, unsigned long long)                                                                                          \
    X(bool, bool)                                                                                                       \
    X(int8, int8_t)                                                                                                     \
    X(int16, int16_t)                                                                                                   \
    X(int32, int32_t)                                                                                                   \
    X(int64, int64_t)                                                                                                   \
    X(uint8, uint8_t)                                                                                                   \
    X(uint16, uint16_t)                                                                                                 \
    X(uint32, uint32_t)                                                                                                 \
    X(uint64, uint64_t)                                                                                                 \
    X(size, size_t)                                                                                                     \
    X(ptrdiff, ptrdiff_t)                                                                                               \
    X(float, float)                                                                                                     \
    X(double, double)

#define X(name, type)                                                                                                   \
    static const ParrotReflectDescription builtin_##name##_description[] = {                                            \
        PARROT_REFLECT_TYPE_HEADER(type),                                                                               \
        PARROT_REFLECT_END(),                                                                                           \
    };
BUILTIN_TYPES
#undef X

static const ParrotReflectDescription builtin_collection[] = {
    PARROT_REFLECT_COLLECTION_HEADER(),

#define X(name, type) PARROT_REFLECT_COLLECTION_DESCRIPTION(builtin_##name##_description),
    BUILTIN_TYPES
#undef X

        PARROT_REFLECT_END(),
};

ParrotReflect *ParrotReflect_new(void) {
    ParrotReflect *self = PARROT_ALLOC(ParrotReflect);

    self->scope = ParrotScope_new(NULL);

    ParrotScope_push_shfree(self->scope, self->sh_types);
    ParrotScope_push_shfree(self->scope, self->sh_aliases);

    ParrotReflect_register(self, builtin_collection);
    return self;
}

void ParrotReflect_delete(ParrotReflect *self) {
    PARROT_FAIL_NULL(self);

    ParrotScope_delete(self->scope);
    free(self);
}

void ParrotReflect_vdelete(void *self) {
    ParrotReflect_delete((ParrotReflect *)self);
}

static void register_type(ParrotReflect *self, const ParrotReflectDescription *description) {
    ParrotReflectTypeInfo type_info = {0};
    type_info.scope = ParrotScope_new(self->scope);

    type_info.key = strcpy(calloc(strlen(description->name) + 1, sizeof(char)), description->name);
    ParrotScope_push_free(type_info.scope, type_info.key);

    type_info.size = description->unique_data.type_header_data.size;

    type_info.sh_fields = malloc(sizeof(type_info.sh_fields));
    ParrotScope_push_free(type_info.scope, type_info.sh_fields);
    *type_info.sh_fields = NULL;
    ParrotScope_push_shfree(type_info.scope, *type_info.sh_fields);

    type_info.sh_tags = malloc(sizeof(type_info.sh_tags));
    ParrotScope_push_free(type_info.scope, type_info.sh_tags);
    *type_info.sh_tags = NULL;
    ParrotScope_push_shfree(type_info.scope, *type_info.sh_tags);
    if (description->tags) {
        for (size_t i = 0; description->tags[i][0]; i++) {
            const char **tag = description->tags[i];

            char *key = strcpy(calloc(strlen(tag[0]) + 1, sizeof(char)), tag[0]);
            ParrotScope_push_free(type_info.scope, key);

            char *value = strcpy(calloc(strlen(tag[1]) + 1, sizeof(char)), tag[1]);
            ParrotScope_push_free(type_info.scope, value);

            shput(*type_info.sh_tags, key, value);
        }
    }

    description++;

    while (description->type != ParrotReflectEntryType_END) {
        switch (description->type) {
        case ParrotReflectEntryType_END: {
        } break;

        case ParrotReflectEntryType_TYPE_HEADER: {
            FAIL_HEADER();
        } break;

        case ParrotReflectEntryType_TYPE_FIELD: {
            ParrotScope *scope = ParrotScope_new(NULL);

            size_t dimension_count = 1;
            size_t *arr_dimensions = NULL;
            ParrotScope_push_arrfree(scope, arr_dimensions);

            const char *ptr = description->unique_data.type_field_data.suffix;
            for (;;) {
                ptr = strchr(ptr, '[');
                if (!ptr) {
                    break;
                }
                ptr++;

                size_t size = strtol(ptr, NULL, 0);
                PARROT_FAIL_COND_MSG(size == 0, "Unexpected dimension with size of 0");
                arrpush(arr_dimensions, size);

                dimension_count *= size;

                ptr = strchr(ptr, ']');
                PARROT_FAIL_COND_FMT(
                    !ptr, "Expected ']' in suffix: \"%s\"", description->unique_data.type_field_data.suffix);
            }

            for (size_t i = 0; i < dimension_count; i++) {
                ParrotReflectTypeFieldInfo field_info = {0};

                field_info.scope = ParrotScope_new(type_info.scope);

                field_info.basename = strcpy(calloc(strlen(description->name) + 1, sizeof(char)), description->name);
                ParrotScope_push_free(field_info.scope, field_info.basename);

                if (arrlen(arr_dimensions) > 1) {
                    size_t key_len = strlen(description->name) + arrlen(arr_dimensions) * 24;
                    field_info.key = calloc(key_len, sizeof(char));
                    strcpy(field_info.key, description->name);

                    size_t remaining = i;
                    size_t *indices = calloc(arrlen(arr_dimensions), sizeof(size_t));

                    for (size_t j = arrlen(arr_dimensions); j-- > 0;) {
                        indices[j] = remaining % arr_dimensions[j];
                        remaining /= arr_dimensions[j];
                    }

                    for (size_t j = 0; j < arrlen(arr_dimensions); j++) {
                        char bracket[24];
                        snprintf(bracket, sizeof(bracket), "[%zu]", indices[j]);
                        strcat(field_info.key, bracket);
                    }

                    free(indices);
                } else {
                    field_info.key = strcpy(calloc(strlen(description->name) + 1, sizeof(char)), description->name);
                }
                ParrotScope_push_free(field_info.scope, field_info.key);

                field_info.offset = description->unique_data.type_field_data.offset +
                                    i * description->unique_data.type_field_data.field_base_size;

                field_info.type = strcpy(calloc(strlen(description->unique_data.type_field_data.type) + 1, sizeof(char)),
                                         description->unique_data.type_field_data.type);
                ParrotScope_push_free(field_info.scope, field_info.type);

                field_info.size = description->unique_data.type_field_data.field_base_size;

                field_info.sh_tags = malloc(sizeof(field_info.sh_tags));
                ParrotScope_push_free(field_info.scope, field_info.sh_tags);
                *field_info.sh_tags = NULL;
                ParrotScope_push_shfree(field_info.scope, *field_info.sh_tags);
                if (description->tags) {
                    for (size_t j = 0; description->tags[j][0]; j++) {
                        const char **tag = description->tags[j];

                        char *key = strcpy(calloc(strlen(tag[0]) + 1, sizeof(char)), tag[0]);
                        ParrotScope_push_free(field_info.scope, key);

                        char *value = strcpy(calloc(strlen(tag[1]) + 1, sizeof(char)), tag[1]);
                        ParrotScope_push_free(field_info.scope, value);

                        shput(*field_info.sh_tags, key, value);
                    }
                }

                field_info.array_index = i;
                field_info.array_size = arrlen(arr_dimensions) > 0 ? dimension_count : 0;

                shputs(*type_info.sh_fields, field_info);
            }

            ParrotScope_delete(scope);
        } break;

        default:
            FAIL_INVALID_TYPE(description->type);
            break;
        }

        description++;
    }

    shputs(self->sh_types, type_info);
}

static void register_collection(ParrotReflect *self, const ParrotReflectDescription *description) {
    description++;

    while (description->type != ParrotReflectEntryType_END) {
        switch (description->type) {
        case ParrotReflectEntryType_COLLECTION_HEADER: {
            FAIL_HEADER();
        } break;
        case ParrotReflectEntryType_COLLECTION_DESCRIPTION: {
            ParrotReflect_register(self, description->unique_data.collection_description_data.description);
        } break;
        default: {
            FAIL_INVALID_TYPE(description->type);
        } break;
        }

        description++;
    }
}

void ParrotReflect_register(ParrotReflect *self, const ParrotReflectDescription *description) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(description);

    switch (description->type) {
    case ParrotReflectEntryType_TYPE_HEADER: {
        FAIL_REGISTERED_TYPE(description->name);

        register_type(self, description);
    } break;
    case ParrotReflectEntryType_ALIAS: {
        FAIL_REGISTERED_TYPE(description->name);

        ParrotReflectAliasInfo alias_info = {0};

        alias_info.scope = ParrotScope_new(self->scope);

        alias_info.key = strcpy(calloc(strlen(description->unique_data.alias_data.name) + 1, sizeof(char)),
                                description->unique_data.alias_data.name);
        ParrotScope_push_free(alias_info.scope, alias_info.key);

        alias_info.value = strcpy(calloc(strlen(description->unique_data.alias_data.alias_of_name) + 1, sizeof(char)),
                                  description->unique_data.alias_data.alias_of_name);
        ParrotScope_push_free(alias_info.scope, alias_info.value);

        shputs(self->sh_aliases, alias_info);
    } break;
    case ParrotReflectEntryType_COLLECTION_HEADER: {
        register_collection(self, description);
    } break;
    default: {
        PARROT_FAIL_MSG("First entry of description must be a header");
    } break;
    }
}

void ParrotReflect_unregister(ParrotReflect *self, const char *type) {
    ParrotReflectTypeInfo *type_info = shgetp_null(self->sh_types, type);
    if (type_info) {
        ParrotReflectTypeInfo local_type_info = *type_info;
        shdel(self->sh_types, type);

        ParrotScope_delete(local_type_info.scope);
        return;
    }

    ParrotReflectAliasInfo *alias_info = shgetp_null(self->sh_aliases, type);
    if (alias_info) {
        ParrotReflectAliasInfo local_alias_info = *alias_info;
        shdel(self->sh_aliases, type);

        ParrotScope_delete(local_alias_info.scope);
        return;
    }
}

size_t ParrotReflect_get_type_count(ParrotReflect *self) {
    PARROT_FAIL_NULL(self);
    return shlen(self->sh_types);
}

char *ParrotReflect_parse_type(const char *type, size_t *out_ptr_level, bool *out_is_const) {
    PARROT_FAIL_NULL(type);

    if (out_ptr_level) {
        *out_ptr_level = 0;
    }

    if (out_is_const) {
        *out_is_const = false;
    }

    char *arr_type_str = NULL;

    while (*type && isspace(*type)) {
        type++;
    }

    if (out_is_const && strncmp(type, "const", strlen("const")) == 0) {
        *out_is_const = true;
    }

    while (*type) {
        arrpush(arr_type_str, *type++);
    }

    while (arr_type_str[arrlen(arr_type_str) - 1] == ' ' || arr_type_str[arrlen(arr_type_str) - 1] == '*') {
        if (out_ptr_level && arr_type_str[arrlen(arr_type_str) - 1] == '*') {
            (*out_ptr_level)++;
        }
        arrdel(arr_type_str, arrlen(arr_type_str) - 1);
    }

    arrpush(arr_type_str, '\0');

    char *ret = strcpy(calloc(arrlen(arr_type_str), sizeof(char)), arr_type_str);
    arrfree(arr_type_str);
    return ret;
}

ptrdiff_t ParrotReflect_resolve_type_ex(
    ParrotReflect *self, const char *type, char **out_parsed_type, size_t *out_ptr_level, bool *out_is_const) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(type);

    char *parsed_type = ParrotReflect_parse_type(type, out_ptr_level, out_is_const);

    type = parsed_type;
    while (shgeti(self->sh_aliases, type) >= 0) {
        type = shget(self->sh_aliases, type);
    }

    ptrdiff_t ret = shgeti(self->sh_types, type);
    if (out_parsed_type) {
        *out_parsed_type = parsed_type;
    } else {
        free(parsed_type);
    }
    return ret;
}

ptrdiff_t ParrotReflect_resolve_type_by_index(ParrotReflect *self, size_t index) {
    PARROT_FAIL_NULL(self);
    return shlen(self->sh_types) > index ? (ptrdiff_t)index : -1;
}

static ParrotReflectTypeInfo *resolve_type_info(ParrotReflect *self, size_t type) {
    PARROT_FAIL_NULL(self);
    PARROT_RET_COND_V(type >= shlen(self->sh_types), NULL);

    return &self->sh_types[type];
}

char *ParrotReflect_get_type_name(ParrotReflect *self, size_t type) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    return strcpy(calloc(strlen(type_info->key) + 1, sizeof(char)), type_info->key);
}

size_t ParrotReflect_get_type_size(ParrotReflect *self, size_t type) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    return type_info->size;
}

char *ParrotReflect_get_type_tag(ParrotReflect *self, size_t type, const char *key) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);

    PARROT_RET_COND_V(shgeti(*type_info->sh_tags, key) <= 0, NULL);
    char *tag = shget(*type_info->sh_tags, key);
    return strcpy(calloc(strlen(tag) + 1, sizeof(char)), tag);
}

ptrdiff_t ParrotReflect_get_type_field(ParrotReflect *self, size_t type, const char *name) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    return shgeti(*type_info->sh_fields, name);
}

size_t ParrotReflect_get_type_field_count(ParrotReflect *self, size_t type) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    return shlen(*type_info->sh_fields);
}

size_t ParrotReflect_get_type_field_offset(ParrotReflect *self, size_t type, size_t index) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(index >= shlen(*type_info->sh_fields));
    return (*type_info->sh_fields)[index].offset;
}

char *ParrotReflect_get_type_field_typename(ParrotReflect *self, size_t type, size_t field) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(field >= shlen(*type_info->sh_fields));

    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];
    return strcpy(calloc(strlen(field_info->type) + 1, sizeof(char)), field_info->type);
}

char *ParrotReflect_get_type_field_name(ParrotReflect *self, size_t type, size_t field) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(field >= shlen(*type_info->sh_fields));

    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];
    return strcpy(calloc(strlen(field_info->key) + 1, sizeof(char)), field_info->key);
}

char *ParrotReflect_get_type_field_basename(ParrotReflect *self, size_t type, size_t field) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(field >= shlen(*type_info->sh_fields));

    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];
    return strcpy(calloc(strlen(field_info->key) + 1, sizeof(char)), field_info->basename);
}

size_t ParrotReflect_get_type_field_size(ParrotReflect *self, size_t type, size_t field) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(field >= shlen(*type_info->sh_fields));

    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];
    return field_info->size;
}

size_t ParrotReflect_get_type_field_array_index(ParrotReflect *self, size_t type, size_t field) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(field >= shlen(*type_info->sh_fields));

    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];
    return field_info->array_index;
}

size_t ParrotReflect_get_type_field_array_size(ParrotReflect *self, size_t type, size_t field) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(field >= shlen(*type_info->sh_fields));

    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];
    return field_info->array_size;
}

char *ParrotReflect_get_type_field_tag(ParrotReflect *self, size_t type, size_t field, const char *key) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);

    PARROT_FAIL_COND(field >= shlen(*type_info->sh_fields));
    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];

    PARROT_RET_COND_V(shgeti(*field_info->sh_tags, key) <= 0, NULL);
    char *tag = shget(*field_info->sh_tags, key);
    return strcpy(calloc(strlen(tag) + 1, sizeof(char)), tag);
}

#undef FAIL_INVALID_TYPE
#undef FAIL_REGISTERED_TYPE
#undef FAIL_HEADER

#endif

#ifdef PARROT_CORE_MATH

ParrotReal Parrot_lerp(ParrotReal a, ParrotReal b, ParrotReal t) {
    return (1 - t) * a + t * b;
}

float Parrot_lerpf(float a, float b, float t) {
    return (1 - t) * a + t * b;
}

double Parrot_lerpd(double a, double b, double t) {
    return (1 - t) * a + t * b;
}

void ParrotReal_to_float_array(const ParrotReal *src, ParrotReal *dest, size_t count) {
    for (size_t i = 0; i < count; i++) {
        dest[i] = src[i];
    }
}

ParrotVec2 ParrotVec2_n(ParrotReal n) {
    return (ParrotVec2){n, n};
}

ParrotVec2 ParrotVec2_add(ParrotVec2 a, ParrotVec2 b) {
    return (ParrotVec2){a.x + b.x, a.y + b.y};
}

ParrotVec2 ParrotVec2_sub(ParrotVec2 a, ParrotVec2 b) {
    return (ParrotVec2){a.x - b.x, a.y - b.y};
}

ParrotVec2 ParrotVec2_mul(ParrotVec2 a, ParrotVec2 b) {
    return (ParrotVec2){a.x * b.x, a.y * b.y};
}

ParrotVec2 ParrotVec2_div(ParrotVec2 a, ParrotVec2 b) {
    return (ParrotVec2){a.x / b.x, a.y / b.y};
}

ParrotVec2 ParrotVec2_scale(ParrotVec2 a, ParrotReal b) {
    return (ParrotVec2){a.x * b, a.y * b};
}

ParrotVec2 ParrotVec2_normalize(ParrotVec2 self) {
    ParrotReal length = ParrotVec2_length(self);

    if (length == 0) {
        return ParrotVec2_n(0);
    }

    return (ParrotVec2){self.x / length, self.y / length};
}

ParrotReal ParrotVec2_length(ParrotVec2 self) {
    return ParrotReal_sqrt(self.x * self.x + self.y * self.y);
}

ParrotReal ParrotVec2_dot(ParrotVec2 self, ParrotVec2 other) {
    return self.x * other.x + self.y * other.y;
}

ParrotVec3 ParrotVec3_n(ParrotReal n) {
    return (ParrotVec3){n, n, n};
}

ParrotVec3 ParrotVec3_upgrade(ParrotVec2 v) {
    return (ParrotVec3){
        v.x,
        v.y,
        0,
    };
}

ParrotVec3 ParrotVec3_add(ParrotVec3 a, ParrotVec3 b) {
    return (ParrotVec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

ParrotVec3 ParrotVec3_sub(ParrotVec3 a, ParrotVec3 b) {
    return (ParrotVec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

ParrotVec3 ParrotVec3_mul(ParrotVec3 a, ParrotVec3 b) {
    return (ParrotVec3){a.x * b.x, a.y * b.y, a.z * b.z};
}

ParrotVec3 ParrotVec3_div(ParrotVec3 a, ParrotVec3 b) {
    return (ParrotVec3){a.x / b.x, a.y / b.y, a.z / b.z};
}

ParrotVec3 ParrotVec3_scale(ParrotVec3 a, ParrotReal b) {
    return (ParrotVec3){a.x * b, a.y * b, a.z * b};
}

ParrotVec3 ParrotVec3_normalize(ParrotVec3 self) {
    ParrotReal length = ParrotVec3_length(self);

    if (length == 0) {
        return ParrotVec3_n(0);
    }

    return (ParrotVec3){self.x / length, self.y / length, self.z / length};
}

ParrotReal ParrotVec3_length(ParrotVec3 self) {
    return ParrotReal_sqrt(self.x * self.x + self.y * self.y + self.z * self.z);
}

ParrotReal ParrotVec3_dot(ParrotVec3 self, ParrotVec3 other) {
    return self.x * other.x + self.y * other.y + self.z * other.z;
}

ParrotVec4 ParrotVec4_n(ParrotReal n) {
    return (ParrotVec4){n, n, n, n};
}

ParrotVec4 ParrotVec4_upgrade(ParrotVec3 v) {
    return (ParrotVec4){v.x, v.y, v.z, 0};
}

ParrotVec4 ParrotVec4_add(ParrotVec4 a, ParrotVec4 b) {
    return (ParrotVec4){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

ParrotVec4 ParrotVec4_sub(ParrotVec4 a, ParrotVec4 b) {
    return (ParrotVec4){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

ParrotVec4 ParrotVec4_mul(ParrotVec4 a, ParrotVec4 b) {
    return (ParrotVec4){a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}

ParrotVec4 ParrotVec4_div(ParrotVec4 a, ParrotVec4 b) {
    return (ParrotVec4){a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
}

ParrotVec4 ParrotVec4_scale(ParrotVec4 a, ParrotReal b) {
    return (ParrotVec4){a.x * b, a.y * b, a.z * b, a.w * b};
}

ParrotVec4 ParrotVec4_normalize(ParrotVec4 self) {
    ParrotReal length = ParrotVec4_length(self);

    if (length == 0) {
        return ParrotVec4_n(0);
    }

    return (ParrotVec4){self.x / length, self.y / length, self.z / length, self.w / length};
}

ParrotReal ParrotVec4_length(ParrotVec4 self) {
    return ParrotReal_sqrt(self.x * self.x + self.y * self.y + self.z * self.z + self.w * self.w);
}

ParrotReal ParrotVec4_dot(ParrotVec4 self, ParrotVec4 other) {
    return self.x * other.x + self.y * other.y + self.z * other.z + self.w * other.w;
}

ParrotMat ParrotMat_identity(void) {
    ParrotMat result = {0};

    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            if (x == y) {
                result.data[x][y] = 1;
            }
        }
    }

    return result;
}

ParrotMat ParrotMat_inverse(ParrotMat matrix) {
    ParrotMat result = {0};

    float aug[4][8];
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            aug[y][x] = matrix.data[x][y];
            aug[y][x + 4] = (x == y) ? 1.0f : 0.0f;
        }
    }

    for (int col = 0; col < 4; col++) {
        int pivot = col;
        for (int row = col + 1; row < 4; row++) {
            if (fabsf(aug[row][col]) > fabsf(aug[pivot][col]))
                pivot = row;
        }

        if (pivot != col) {
            for (int k = 0; k < 8; k++) {
                float tmp = aug[col][k];
                aug[col][k] = aug[pivot][k];
                aug[pivot][k] = tmp;
            }
        }

        if (fabsf(aug[col][col]) < 1e-6f) {
            return result;
        }

        float scale = aug[col][col];
        for (int k = 0; k < 8; k++)
            aug[col][k] /= scale;

        for (int row = 0; row < 4; row++) {
            if (row == col) {
                continue;
            }
            float factor = aug[row][col];
            for (int k = 0; k < 8; k++) {
                aug[row][k] -= factor * aug[col][k];
            }
        }
    }

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            result.data[x][y] = aug[y][x + 4];
        }
    }

    return result;
}

ParrotMat ParrotMat_transpose(ParrotMat matrix) {
    ParrotMat result = {0};

    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            result.data[x][y] = matrix.data[y][x];
        }
    }

    return result;
}

ParrotMat ParrotMat_add(ParrotMat a, ParrotMat b) {
    ParrotMat result = {0};

    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            result.data[x][y] = a.data[x][y] + b.data[x][y];
        }
    }

    return result;
}

ParrotMat ParrotMat_mul(ParrotMat a, ParrotMat b) {
    ParrotMat result = {0};

    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            for (int z = 0; z < 4; z++) {
                result.data[x][y] += a.data[z][y] * b.data[x][z];
            }
        }
    }

    return result;
}

ParrotVec3 ParrotMat_transform3(ParrotMat matrix, ParrotVec3 vec) {
    float w = matrix.data[0][3] * vec.x + matrix.data[1][3] * vec.y + matrix.data[2][3] * vec.z + matrix.data[3][3];
    PARROT_RET_COND_V(w == 0, ParrotVec3_n(0));
    return (ParrotVec3){
        .x = (matrix.data[0][0] * vec.x + matrix.data[1][0] * vec.y + matrix.data[2][0] * vec.z + matrix.data[3][0]) / w,
        .y = (matrix.data[0][1] * vec.x + matrix.data[1][1] * vec.y + matrix.data[2][1] * vec.z + matrix.data[3][1]) / w,
        .z = (matrix.data[0][2] * vec.x + matrix.data[1][2] * vec.y + matrix.data[2][2] * vec.z + matrix.data[3][2]) / w,
    };
}

ParrotVec2 ParrotMat_transform2(ParrotMat matrix, ParrotVec2 vec) {
    ParrotVec3 result = ParrotMat_transform3(matrix, ParrotVec3_upgrade(vec));
    return (ParrotVec2){result.x, result.y};
}

ParrotMat
ParrotMat_ortho(ParrotReal left, ParrotReal right, ParrotReal bottom, ParrotReal top, ParrotReal near, ParrotReal far) {
    ParrotMat matrix = {0};

    ParrotReal width = right - left;
    ParrotReal height = top - bottom;
    ParrotReal total_distance = far - near;

    matrix.data[0][0] = 2.0 / width;

    matrix.data[1][1] = 2.0 / height;

    matrix.data[2][2] = -2.0 / total_distance;

    matrix.data[3][0] = -(right + left) / width;
    matrix.data[3][1] = -(top + bottom) / height;
    matrix.data[3][2] = -(far + near) / total_distance;
    matrix.data[3][3] = 1.0;

    return matrix;
}

ParrotMat ParrotMat_translation(ParrotVec3 position) {
    ParrotMat matrix = ParrotMat_identity();

    matrix.data[3][0] = position.x;
    matrix.data[3][1] = position.y;
    matrix.data[3][2] = position.z;

    return matrix;
}

ParrotMat ParrotMat_rotation(ParrotVec3 rotation) {
    ParrotMat matrix = ParrotMat_identity();

    ParrotReal sx = ParrotReal_sin(rotation.x), cx = ParrotReal_cos(rotation.x);
    ParrotReal sy = ParrotReal_sin(rotation.y), cy = ParrotReal_cos(rotation.y);
    ParrotReal sz = ParrotReal_sin(rotation.z), cz = ParrotReal_cos(rotation.z);

    matrix.data[0][0] = cz * cy;
    matrix.data[0][1] = sz * cy;
    matrix.data[0][2] = -sy;

    matrix.data[1][0] = cz * sy * sx - sz * cx;
    matrix.data[1][1] = sz * sy * sx + cz * cx;
    matrix.data[1][2] = cy * sx;

    matrix.data[2][0] = cz * sy * cx + sz * sx;
    matrix.data[2][1] = sz * sy * cx - cz * sx;
    matrix.data[2][2] = cy * cx;

    return matrix;
}

ParrotMat ParrotMat_scale(ParrotVec3 scale) {
    ParrotMat matrix = ParrotMat_identity();

    matrix.data[0][0] = scale.x;
    matrix.data[1][1] = scale.y;
    matrix.data[2][2] = scale.z;

    return matrix;
}

ParrotTransform ParrotTransform_new(void) {
    return (ParrotTransform){
        .matrix = ParrotMat_identity(),

        .scale = ParrotVec3_n(1),
    };
}

ParrotMat ParrotTransform_calculate_matrix(const ParrotTransform *self) {
    ParrotMat matrix = ParrotMat_identity();
    if (self->parent) {
        matrix = ParrotTransform_calculate_matrix(self->parent);
    }

    matrix = ParrotMat_mul(matrix, ParrotMat_translation(self->position));
    matrix = ParrotMat_mul(matrix, ParrotMat_rotation(self->rotation));
    matrix = ParrotMat_mul(matrix, ParrotMat_scale(self->scale));
    matrix = ParrotMat_mul(matrix, self->matrix);

    return matrix;
}

ParrotMat ParrotGMatSet_combine(const ParrotGMatSet *self) {
    return ParrotMat_mul(ParrotMat_mul(self->projection, self->view), self->model);
}

ParrotColor ParrotColor_new(uint8_t r, uint8_t g, uint8_t b) {
    return ParrotColor_newa(r, g, b, 255);
}

ParrotColor ParrotColor_newf(float r, float g, float b) {
    return ParrotColor_newaf(r, g, b, 1);
}

ParrotColor ParrotColor_newa(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ParrotColor_newaf((float)r / 255, (float)g / 255, (float)b / 255, (float)a / 255);
}

ParrotColor ParrotColor_newaf(float r, float g, float b, float a) {
    return (ParrotColor){
        .r = PARROT_CLAMP(0.0, r, 1.0),
        .g = PARROT_CLAMP(0.0, g, 1.0),
        .b = PARROT_CLAMP(0.0, b, 1.0),
        .a = PARROT_CLAMP(0.0, a, 1.0),
    };
}

ParrotColor ParrotColor_from(ParrotColorFormat format, uint32_t color) {
    switch (format) {
    case ParrotColorFormat_RGBA8888:
        return ParrotColor_newa(color >> 24, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    case ParrotColorFormat_BGRA8888:
        return ParrotColor_newa(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF, color >> 24);
    }
    return ParrotColor_CLEAR;
}

uint32_t ParrotColor_to(ParrotColor self, ParrotColorFormat format) {
    switch (format) {
    case ParrotColorFormat_RGBA8888: {
        uint8_t r = PARROT_CLAMP(0.0, self.r, 1.0) * 255;
        uint8_t g = PARROT_CLAMP(0.0, self.g, 1.0) * 255;
        uint8_t b = PARROT_CLAMP(0.0, self.b, 1.0) * 255;
        uint8_t a = PARROT_CLAMP(0.0, self.a, 1.0) * 255;
        return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    } break;
    case ParrotColorFormat_BGRA8888: {
        uint8_t r = PARROT_CLAMP(0.0, self.r, 1.0) * 255;
        uint8_t g = PARROT_CLAMP(0.0, self.g, 1.0) * 255;
        uint8_t b = PARROT_CLAMP(0.0, self.b, 1.0) * 255;
        uint8_t a = PARROT_CLAMP(0.0, self.a, 1.0) * 255;
        return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
    } break;
    }
    return 0;
}

ParrotColor ParrotColor_mul(ParrotColor a, ParrotColor b) {
    return ParrotColor_newaf(a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a);
}

ParrotColor ParrotColor_blend(ParrotColor a, ParrotColor b) {
    return ParrotColor_lerp(a, b, 0.5);
}

ParrotColor ParrotColor_lerp(ParrotColor a, ParrotColor b, float t) {
    return ParrotColor_newaf(
        Parrot_lerpf(a.r, b.r, t), Parrot_lerpf(a.g, b.g, t), Parrot_lerpf(a.b, b.b, t), Parrot_lerpf(a.a, b.a, t));
}

#endif

#ifdef PARROT_CORE_HASH

extern uint32_t Parrot_crc32_table[];

ParrotCRC32 Parrot_crc32(const void *data, size_t size) {
    return Parrot_crc32_combine(~0xFFFFFFFF, data, size);
}

ParrotCRC32 Parrot_crc32_combine(ParrotCRC32 crc, const void *void_data, size_t size) {
    const uint8_t *data = (uint8_t *)void_data;

    PARROT_FAIL_NULL(data);

    crc = ~crc;

    while (size-- > 0) {
        crc ^= *data++;
        for (int i = 0; i < 8; i++) {
            const uint32_t poly = 0xEDB88320;

            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (mask & poly);
        }
    }

    return ~crc;
}

#endif

#ifdef PARROT_CORE_BINARY

ParrotBinaryImage ParrotBinaryImage_from_mutable(ParrotMutableBinaryImage image) {
    return (ParrotBinaryImage){
        .data = image.data,
        .size = image.size,
    };
}

uint8_t Parrot_char_to_ascii(char c) {
    switch (c) {
    case '\0':
        return 0x00;
    case '0':
        return 0x30;
    case '1':
        return 0x31;
    case '2':
        return 0x32;
    case '3':
        return 0x33;
    case '4':
        return 0x34;
    case '5':
        return 0x35;
    case '6':
        return 0x36;
    case '7':
        return 0x37;
    case '8':
        return 0x38;
    case '9':
        return 0x39;
    case 'A':
        return 0x41;
    case 'B':
        return 0x42;
    case 'C':
        return 0x43;
    case 'D':
        return 0x44;
    case 'E':
        return 0x45;
    case 'F':
        return 0x46;
    case 'G':
        return 0x47;
    case 'H':
        return 0x48;
    case 'I':
        return 0x49;
    case 'J':
        return 0x4A;
    case 'K':
        return 0x4B;
    case 'L':
        return 0x4C;
    case 'M':
        return 0x4D;
    case 'N':
        return 0x4E;
    case 'O':
        return 0x4F;
    case 'P':
        return 0x50;
    case 'Q':
        return 0x51;
    case 'R':
        return 0x52;
    case 'S':
        return 0x53;
    case 'T':
        return 0x54;
    case 'U':
        return 0x55;
    case 'V':
        return 0x56;
    case 'W':
        return 0x57;
    case 'X':
        return 0x58;
    case 'Y':
        return 0x59;
    case 'Z':
        return 0x5A;
    case 'a':
        return 0x61;
    case 'b':
        return 0x62;
    case 'c':
        return 0x63;
    case 'd':
        return 0x64;
    case 'e':
        return 0x65;
    case 'f':
        return 0x66;
    case 'g':
        return 0x67;
    case 'h':
        return 0x68;
    case 'i':
        return 0x69;
    case 'j':
        return 0x6A;
    case 'k':
        return 0x6B;
    case 'l':
        return 0x6C;
    case 'm':
        return 0x6D;
    case 'n':
        return 0x6E;
    case 'o':
        return 0x6F;
    case 'p':
        return 0x70;
    case 'q':
        return 0x71;
    case 'r':
        return 0x72;
    case 's':
        return 0x73;
    case 't':
        return 0x74;
    case 'u':
        return 0x75;
    case 'v':
        return 0x76;
    case 'w':
        return 0x77;
    case 'x':
        return 0x78;
    case 'y':
        return 0x79;
    case 'z':
        return 0x7A;
    case '!':
        return 0x21;
    case '"':
        return 0x22;
    case '#':
        return 0x23;
    case '%':
        return 0x25;
    case '&':
        return 0x26;
    case '\'':
        return 0x27;
    case '(':
        return 0x28;
    case ')':
        return 0x29;
    case '*':
        return 0x2A;
    case '+':
        return 0x2B;
    case ',':
        return 0x2C;
    case '-':
        return 0x2D;
    case '.':
        return 0x2E;
    case '/':
        return 0x2F;
    case ':':
        return 0x3A;
    case ';':
        return 0x3B;
    case '<':
        return 0x3C;
    case '=':
        return 0x3D;
    case '>':
        return 0x3E;
    default:
    case '?':
        return 0x3F;
    case '[':
        return 0x5B;
    case '\\':
        return 0x5C;
    case ']':
        return 0x5D;
    case '^':
        return 0x5E;
    case '_':
        return 0x5F;
    case '{':
        return 0x7B;
    case '|':
        return 0x7C;
    case '}':
        return 0x7D;
    case '~':
        return 0x7E;
    }
}

char Parrot_ascii_to_char(uint8_t ascii) {
    switch (ascii) {
    case 0x00:
        return '\0';
    case 0x30:
        return '0';
    case 0x31:
        return '1';
    case 0x32:
        return '2';
    case 0x33:
        return '3';
    case 0x34:
        return '4';
    case 0x35:
        return '5';
    case 0x36:
        return '6';
    case 0x37:
        return '7';
    case 0x38:
        return '8';
    case 0x39:
        return '9';
    case 0x41:
        return 'A';
    case 0x42:
        return 'B';
    case 0x43:
        return 'C';
    case 0x44:
        return 'D';
    case 0x45:
        return 'E';
    case 0x46:
        return 'F';
    case 0x47:
        return 'G';
    case 0x48:
        return 'H';
    case 0x49:
        return 'I';
    case 0x4A:
        return 'J';
    case 0x4B:
        return 'K';
    case 0x4C:
        return 'L';
    case 0x4D:
        return 'M';
    case 0x4E:
        return 'N';
    case 0x4F:
        return 'O';
    case 0x50:
        return 'P';
    case 0x51:
        return 'Q';
    case 0x52:
        return 'R';
    case 0x53:
        return 'S';
    case 0x54:
        return 'T';
    case 0x55:
        return 'U';
    case 0x56:
        return 'V';
    case 0x57:
        return 'W';
    case 0x58:
        return 'X';
    case 0x59:
        return 'Y';
    case 0x5A:
        return 'Z';
    case 0x61:
        return 'a';
    case 0x62:
        return 'b';
    case 0x63:
        return 'c';
    case 0x64:
        return 'd';
    case 0x65:
        return 'e';
    case 0x66:
        return 'f';
    case 0x67:
        return 'g';
    case 0x68:
        return 'h';
    case 0x69:
        return 'i';
    case 0x6A:
        return 'j';
    case 0x6B:
        return 'k';
    case 0x6C:
        return 'l';
    case 0x6D:
        return 'm';
    case 0x6E:
        return 'n';
    case 0x6F:
        return 'o';
    case 0x70:
        return 'p';
    case 0x71:
        return 'q';
    case 0x72:
        return 'r';
    case 0x73:
        return 's';
    case 0x74:
        return 't';
    case 0x75:
        return 'u';
    case 0x76:
        return 'v';
    case 0x77:
        return 'w';
    case 0x78:
        return 'x';
    case 0x79:
        return 'y';
    case 0x7A:
        return 'z';
    case 0x21:
        return '!';
    case 0x22:
        return '"';
    case 0x23:
        return '#';
    case 0x25:
        return '%';
    case 0x26:
        return '&';
    case 0x27:
        return '\'';
    case 0x28:
        return '(';
    case 0x29:
        return ')';
    case 0x2A:
        return '*';
    case 0x2B:
        return '+';
    case 0x2C:
        return ',';
    case 0x2D:
        return '-';
    case 0x2E:
        return '.';
    case 0x2F:
        return '/';
    case 0x3A:
        return ':';
    case 0x3B:
        return ';';
    case 0x3C:
        return '<';
    case 0x3D:
        return '=';
    case 0x3E:
        return '>';
    default:
    case 0x3F:
        return '?';
    case 0x5B:
        return '[';
    case 0x5C:
        return '\\';
    case 0x5D:
        return ']';
    case 0x5E:
        return '^';
    case 0x5F:
        return '_';
    case 0x7B:
        return '{';
    case 0x7C:
        return '|';
    case 0x7D:
        return '}';
    case 0x7E:
        return '~';
    }
}

struct ParrotBuffer {
    ParrotBufferRead read;
    ParrotBufferWrite write;
    ParrotScope *scope;

    size_t bytes_written;
    size_t read_position;
};

ParrotBuffer *ParrotBuffer_new(ParrotScope *scope, ParrotBufferRead read, ParrotBufferWrite write) {
    ParrotBuffer *self = PARROT_ALLOC(ParrotBuffer);

    self->read = read;
    self->write = write;
    self->scope = scope;

    return self;
}

static bool file_read(ParrotScope *scope, size_t position, uint8_t *out) {
    FILE *file = ParrotScope_get_ctx(scope, FILE *);

    size_t old_position = ftell(file);
    fseek(file, position, SEEK_SET);
    bool success = fread(out, sizeof(*out), 1, file) == sizeof(*out);
    fseek(file, old_position, SEEK_SET);

    return success;
}

static void file_write(ParrotScope *scope, uint8_t byte) {
    FILE *file = ParrotScope_get_ctx(scope, FILE *);
    fwrite(&byte, sizeof(byte), 1, file);
}

ParrotBuffer *ParrotBuffer_new_file(FILE *file) {
    PARROT_FAIL_NULL(file);

    ParrotScope *scope = ParrotScope_new(NULL);
    ParrotScope_set_ctx(scope, file);

    return ParrotBuffer_new(scope, file_read, file_write);
}

typedef struct {
    const uint8_t **p_data;
    size_t size;

    ParrotScope *write_scope;
    ParrotBufferWrite write;
} BytearrayCtx;

static bool bytearray_read(ParrotScope *scope, size_t position, uint8_t *out) {
    BytearrayCtx *ctx = ParrotScope_get_ctx(scope, BytearrayCtx *);

    if (position >= ctx->size) {
        return false;
    }

    *out = (*ctx->p_data)[position];
    return true;
}

static void bytearray_write_wrapper(ParrotScope *scope, uint8_t byte) {
    BytearrayCtx *ctx = ParrotScope_get_ctx(scope, BytearrayCtx *);

    if (ctx->write) {
        ctx->write(ctx->write_scope, byte);
    }
}

ParrotBuffer *
ParrotBuffer_new_bytearray(ParrotScope *write_scope, const void **p_data, size_t size, ParrotBufferWrite write) {
    PARROT_FAIL_NULL(p_data);

    ParrotScope *scope = ParrotScope_new(NULL);
    BytearrayCtx *ctx = ParrotScope_alloc_ctx(scope, BytearrayCtx);

    ctx->p_data = (const uint8_t **)p_data;
    ctx->size = size;

    ctx->write_scope = write_scope;
    ctx->write = write;

    if (ctx->write_scope) {
        ParrotScope_set_parent(write_scope, scope);
    }

    return ParrotBuffer_new(scope, bytearray_read, bytearray_write_wrapper);
}

static bool stbds_array_read(ParrotScope *scope, size_t position, uint8_t *out) {
    uint8_t **p_arr_data = ParrotScope_get_ctx(scope, uint8_t **);

    if (position >= arrlen(*p_arr_data)) {
        return false;
    }

    *out = (*p_arr_data)[position];
    return true;
}

static void stbds_array_write(ParrotScope *scope, uint8_t byte) {
    uint8_t **p_arr_data = ParrotScope_get_ctx(scope, uint8_t **);
    arrpush(*p_arr_data, byte);
}

ParrotBuffer *ParrotBuffer_new_stbds_array_raw(uint8_t **p_arr_data) {
    ParrotScope *scope = ParrotScope_new(NULL);
    ParrotScope_set_ctx(scope, p_arr_data);
    return ParrotBuffer_new(scope, stbds_array_read, stbds_array_write);
}

void ParrotBuffer_delete(ParrotBuffer *self) {
    PARROT_FAIL_NULL(self);

    if (self->scope) {
        ParrotScope_delete(self->scope);
    }

    free(self);
}

void ParrotBuffer_vdelete(void *self) {
    ParrotBuffer_delete((ParrotBuffer *)self);
}

void ParrotBuffer_rseek(ParrotBuffer *self, size_t position) {
    PARROT_FAIL_NULL(self);

    self->read_position = position;
}

size_t ParrotBuffer_rtell(ParrotBuffer *self) {
    PARROT_FAIL_NULL(self);

    return self->read_position;
}

void ParrotBuffer_pad(ParrotBuffer *self, uint8_t data, size_t count) {
    PARROT_FAIL_NULL(self);

    for (size_t i = 0; i < count; i++) {
        ParrotBuffer_write8(self, data);
    }
}

void ParrotBuffer_pad_until(ParrotBuffer *self, uint8_t data, size_t until_position) {
    PARROT_FAIL_NULL(self);

    while (until_position >= self->bytes_written) {
        ParrotBuffer_write8(self, data);
    }
}

size_t ParrotBuffer_read(ParrotBuffer *self, ParrotBufferEndian endian, void *out_ptr, size_t size) {
    uint8_t *out = out_ptr;

    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(out);

    PARROT_RET_COND_V(!self->read, 0);

    for (size_t i = 0; i < size; i++) {
        if (!self->read(self->scope, self->read_position, &out[i])) {
            return i;
        }
        self->read_position++;
    }

    if (endian != ParrotBufferEndian_HOST) {
        uint16_t endian_test_big = 1;
        uint8_t endian_test = 0;
        memcpy(&endian_test, &endian_test_big, sizeof(uint8_t));

        ParrotBufferEndian host_endian = endian_test == 1 ? ParrotBufferEndian_LITTLE : ParrotBufferEndian_BIG;
        if (endian != host_endian) {
            for (size_t i = 0; i < size / 2; i++) {
                uint8_t tmp = out[i];
                out[i] = out[size - 1 - i];
                out[size - 1 - i] = tmp;
            }
        }
    }

    return size;
}

void ParrotBuffer_write(ParrotBuffer *self, ParrotBufferEndian endian, const void *data_ptr, size_t size) {
    const uint8_t *data = data_ptr;

    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(data);

    PARROT_RET_COND(!self->write);

    uint16_t endian_test_big = 1;
    uint8_t endian_test = 0;
    memcpy(&endian_test, &endian_test_big, sizeof(uint8_t));

    ParrotBufferEndian host_endian = endian_test == 1 ? ParrotBufferEndian_LITTLE : ParrotBufferEndian_BIG;
    if (endian != ParrotBufferEndian_HOST && endian != host_endian) {
        for (size_t i = size; i > 0; i--) {
            self->write(self->scope, data[i - 1]);
            self->bytes_written++;
        }
    } else {
        for (size_t i = 0; i < size; i++) {
            self->write(self->scope, data[i]);
            self->bytes_written++;
        }
    }
}

bool ParrotBuffer_read8(ParrotBuffer *self, uint8_t *out) {
    return ParrotBuffer_read(self, ParrotBufferEndian_HOST, out, sizeof(*out));
}

bool ParrotBuffer_read16(ParrotBuffer *self, ParrotBufferEndian endian, uint16_t *out) {
    return ParrotBuffer_read(self, endian, out, sizeof(*out));
}

bool ParrotBuffer_read32(ParrotBuffer *self, ParrotBufferEndian endian, uint32_t *out) {
    return ParrotBuffer_read(self, endian, out, sizeof(*out));
}

bool ParrotBuffer_read64(ParrotBuffer *self, ParrotBufferEndian endian, uint64_t *out) {
    return ParrotBuffer_read(self, endian, out, sizeof(*out));
}

void ParrotBuffer_write8(ParrotBuffer *self, uint8_t data) {
    ParrotBuffer_write(self, ParrotBufferEndian_HOST, &data, sizeof(data));
}

void ParrotBuffer_write16(ParrotBuffer *self, ParrotBufferEndian endian, uint16_t data) {
    ParrotBuffer_write(self, endian, &data, sizeof(data));
}

void ParrotBuffer_write32(ParrotBuffer *self, ParrotBufferEndian endian, uint32_t data) {
    ParrotBuffer_write(self, endian, &data, sizeof(data));
}

void ParrotBuffer_write64(ParrotBuffer *self, ParrotBufferEndian endian, uint64_t data) {
    ParrotBuffer_write(self, endian, &data, sizeof(data));
}

bool ParrotBuffer_read8s(ParrotBuffer *self, int8_t *out) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(out);

    uint8_t value;

    if (!ParrotBuffer_read8(self, &value)) {
        return false;
    }

    if (value <= INT8_MAX) {
        *out = (int8_t)value;
    } else {
        *out = -(int8_t)(UINT8_MAX - value) - 1;
    }

    return true;
}

bool ParrotBuffer_read16s(ParrotBuffer *self, ParrotBufferEndian endian, int16_t *out) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(out);

    uint16_t value;

    if (!ParrotBuffer_read16(self, endian, &value)) {
        return false;
    }

    if (value <= INT16_MAX) {
        *out = (int16_t)value;
    } else {
        *out = -(int16_t)(UINT16_MAX - value) - 1;
    }

    return true;
}

bool ParrotBuffer_read32s(ParrotBuffer *self, ParrotBufferEndian endian, int32_t *out) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(out);

    uint32_t value;

    if (!ParrotBuffer_read32(self, endian, &value)) {
        return false;
    }

    if (value <= INT32_MAX) {
        *out = (int32_t)value;
    } else {
        *out = -(int32_t)(UINT32_MAX - value) - 1;
    }

    return true;
}

bool ParrotBuffer_read64s(ParrotBuffer *self, ParrotBufferEndian endian, int64_t *out) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(out);

    uint64_t value;

    if (!ParrotBuffer_read64(self, endian, &value)) {
        return false;
    }

    if (value <= INT64_MAX) {
        *out = (int64_t)value;
    } else {
        *out = -(int64_t)(UINT64_MAX - value) - 1;
    }

    return true;
}

void ParrotBuffer_write8s(ParrotBuffer *self, int8_t data) {
    PARROT_FAIL_NULL(self);

    uint8_t value;

    if (data < 0) {
        value = (uint8_t)(UINT8_MAX - (uint8_t)(-(data + 1)));
    } else {
        value = (uint8_t)data;
    }

    ParrotBuffer_write8(self, value);
}

void ParrotBuffer_write16s(ParrotBuffer *self, ParrotBufferEndian endian, int16_t data) {
    PARROT_FAIL_NULL(self);

    uint16_t value;

    if (data < 0) {
        value = (uint16_t)(UINT16_MAX - (uint16_t)(-(data + 1)));
    } else {
        value = (uint16_t)data;
    }

    ParrotBuffer_write16(self, endian, value);
}

void ParrotBuffer_write32s(ParrotBuffer *self, ParrotBufferEndian endian, int32_t data) {
    PARROT_FAIL_NULL(self);

    uint32_t value;

    if (data < 0) {
        value = (uint32_t)(UINT32_MAX - (uint32_t)(-(data + 1)));
    } else {
        value = (uint32_t)data;
    }

    ParrotBuffer_write32(self, endian, value);
}

void ParrotBuffer_write64s(ParrotBuffer *self, ParrotBufferEndian endian, int64_t data) {
    PARROT_FAIL_NULL(self);

    uint64_t value;

    if (data < 0) {
        value = (uint64_t)(UINT64_MAX - (uint64_t)(-(data + 1)));
    } else {
        value = (uint64_t)data;
    }

    ParrotBuffer_write64(self, endian, value);
}

void ParrotBuffer_write_ascii(ParrotBuffer *self, const char *str) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(str);

    while (*str) {
        ParrotBuffer_write8(self, *str++);
    }
    ParrotBuffer_write8(self, 0);
}

#endif

#ifdef PARROT_CORE_TIMING

#ifdef PARROT_PLATFORM_UNIX
#define __USE_POSIX199309
#include <time.h>
#endif

uint64_t Parrot_get_performance_counter(void) {
#ifdef PARROT_PLATFORM_UNIX
    struct timespec time;
    clock_gettime(/* Would use CLOCK_MONOTONIC_RAW if not linux specific */ CLOCK_MONOTONIC, &time);
    return time.tv_nsec + (time.tv_sec * 1e9L);
#endif
}

uint64_t Parrot_get_performance_frequency(void) {
#ifdef PARROT_PLATFORM_UNIX
    struct timespec resolution;
    clock_getres(CLOCK_MONOTONIC, &resolution);
    return resolution.tv_nsec != 0 ?
               1e9L / resolution.tv_nsec :
               /* Probability of this happening? Less than 1/1,000,000,000 but just to be safe */ 1e9;
#endif
}

void Parrot_sleep(float seconds) {
#ifdef PARROT_PLATFORM_UNIX
    struct timespec time = {
        .tv_sec = seconds,
        .tv_nsec = (long)(fmod(seconds, 1.0) * 1e9),
    };
    nanosleep(&time, NULL);
#endif
}

#endif

#ifdef PARROT_CORE_MAIN_LOOP

struct ParrotMainLoop {
    bool should_close;

    uint64_t frame_start;
};

ParrotMainLoop *ParrotMainLoop_new(void) {
    ParrotMainLoop *self = PARROT_ALLOC(ParrotMainLoop);

    return self;
}

void ParrotMainLoop_delete(ParrotMainLoop *self) {
    PARROT_FAIL_NULL(self);

    free(self);
}

void ParrotMainLoop_vdelete(void *self) {
    ParrotMainLoop_delete((ParrotMainLoop *)self);
}

void ParrotMainLoop_request_close(ParrotMainLoop *self) {
    PARROT_FAIL_NULL(self);

    self->should_close = true;
}

bool ParrotMainLoop_next_frame(ParrotMainLoop *self, float *delta, float max_fps) {
    PARROT_FAIL_NULL(self);

    if (max_fps > 0) {
        float target_delta = 1.0 / max_fps;
        uint64_t frequency = Parrot_get_performance_frequency();

        float elapsed = (Parrot_get_performance_counter() - self->frame_start) / (float)frequency;
        float remaining = target_delta - elapsed;
        if (remaining > 0.002) {
            Parrot_sleep(remaining - 0.002);
        }

        while ((Parrot_get_performance_counter() - self->frame_start) / (float)frequency < target_delta)
            ;
    }

    uint64_t frame_end = Parrot_get_performance_counter();
    if (delta) {
        *delta = (float)(frame_end - self->frame_start) / Parrot_get_performance_frequency();
    }
    self->frame_start = frame_end;

    bool should_close = self->should_close;
    self->should_close = false;
    return !should_close;
}

void ParrotMainLoop_run(ParrotMainLoop *self,
                        void *user_data,
                        ParrotMainLoopInitFunc init,
                        ParrotMainLoopUpdateFunc update,
                        ParrotMainLoopRenderFunc render,
                        ParrotMainLoopShutdownFunc shutdown) {
    PARROT_FAIL_NULL(self);

    PARROT_FAIL_NULL(update);

    ParrotMainLoopRunSettings settings = {0};
    settings.user_data = user_data;

    float delta = 1.0 / 60;

    if (init) {
        init(&settings);
    }

    ParrotMainLoop_next_frame(self, &delta, settings.max_fps);
    for (;;) {
        if (update(&settings, delta, !ParrotMainLoop_next_frame(self, &delta, settings.max_fps))) {
            break;
        }

        if (render) {
            render(&settings);
        }
    }

    if (shutdown) {
        shutdown(&settings);
    }
}

#endif

// TODO:
#if 0 // #ifdef PARROT_CORE_SERIALIZE

typedef uint16_t ObjectEntryType;
#define ObjectEntryType_OBJECT (0x00)
#define ObjectEntryType_SINT (0x08)
#define ObjectEntryType_UINT (0x10)
#define ObjectEntryType_ASCII (0x18)
#define ObjectEntryType_NUMBER (0x20)

typedef struct {
    char *name;
    uint32_t index;
} ObjectEntryFieldEntry;

typedef struct {
    ptrdiff_t type;
    const uint8_t *data;
    ObjectEntryFieldEntry **p_arr_fields;
} ObjectEntryDataObject;

typedef union {
    ObjectEntryDataObject object;
    int64_t sint;
    uint64_t uint;
    char ascii;
    double number;
} ObjectEntryData;

typedef struct {
    ParrotScope *scope;

    ObjectEntryType type;
    ObjectEntryData **p_arr_data;
} ObjectEntry;

typedef struct {
    uintptr_t ptr;
    size_t type;
} ObjectPtrMapKey;

typedef struct {
    ObjectPtrMapKey key;
    uint32_t value;
} ObjectPtrMap;

static uint32_t queue_object(ObjectEntry **p_arr_objects,
                             ObjectPtrMap **p_hm_ptr_map,
                             ParrotScope *scope,
                             ParrotReflect *reflect,
                             size_t type,
                             const uint8_t *data,
                             size_t count,
                             bool with_ptrs) {
#define arr_objects (*p_arr_objects)
#define hm_ptr_map (*p_hm_ptr_map)

    ParrotScope *work_scope = ParrotScope_new(scope);

    ObjectPtrMapKey entry_key = {
        .ptr = (uintptr_t)data,
        .type = type,
    };
    if (hmgeti(hm_ptr_map, entry_key) >= 0) {
        ParrotScope_delete(work_scope);
        return hmgeti(hm_ptr_map, entry_key);
    }

    char *typename = ParrotReflect_get_type_name(reflect, type);
    ParrotScope_push_free(work_scope, typename);

    uint32_t entry_index = arrlen(arr_objects);
    arrpush(arr_objects, (ObjectEntry){0});

    ObjectEntry entry = {0};
    entry.scope = ParrotScope_new(scope);

    entry.p_arr_data = malloc(sizeof(*entry.p_arr_data));
    ParrotScope_push_free(entry.scope, entry.p_arr_data);
    *entry.p_arr_data = NULL;
    ParrotScope_push_arrfree(entry.scope, *entry.p_arr_data);

    for (size_t i = 0; i < count; i++) {
        bool primitive = false;
#define X(type_, signed_)                                                                                               \
    do {                                                                                                                \
        if (strcmp(typename, #type_) == 0) {                                                                            \
            primitive = true;                                                                                           \
            entry.type = (signed_) ? ObjectEntryType_SINT : ObjectEntryType_UINT;                                       \
            switch (sizeof(type_)) {                                                                                    \
            case 1:                                                                                                     \
                arrpush(*entry.p_arr_data,                                                                              \
                        (signed_) ? ((ObjectEntryData){                                                                 \
                                        .sint = ((int8_t *)data)[i],                                                    \
                                    }) :                                                                                \
                                    ((ObjectEntryData){                                                                 \
                                        .uint = ((uint8_t *)data)[i],                                                   \
                                    }));                                                                                \
                break;                                                                                                  \
            case 2:                                                                                                     \
                arrpush(*entry.p_arr_data,                                                                              \
                        (signed_) ? ((ObjectEntryData){                                                                 \
                                        .sint = ((int16_t *)data)[i],                                                   \
                                    }) :                                                                                \
                                    ((ObjectEntryData){                                                                 \
                                        .uint = ((uint16_t *)data)[i],                                                  \
                                    }));                                                                                \
                break;                                                                                                  \
            case 4:                                                                                                     \
                arrpush(*entry.p_arr_data,                                                                              \
                        (signed_) ? ((ObjectEntryData){                                                                 \
                                        .sint = ((int32_t *)data)[i],                                                   \
                                    }) :                                                                                \
                                    ((ObjectEntryData){                                                                 \
                                        .uint = ((uint32_t *)data)[i],                                                  \
                                    }));                                                                                \
                break;                                                                                                  \
            case 8:                                                                                                     \
                arrpush(*entry.p_arr_data,                                                                              \
                        (signed_) ? ((ObjectEntryData){                                                                 \
                                        .sint = ((int64_t *)data)[i],                                                   \
                                    }) :                                                                                \
                                    ((ObjectEntryData){                                                                 \
                                        .uint = ((uint64_t *)data)[i],                                                  \
                                    }));                                                                                \
                break;                                                                                                  \
            default:                                                                                                    \
                break;                                                                                                  \
            }                                                                                                           \
        }                                                                                                               \
    } while (0)
        X(short, true);
        X(int, true);
        X(long, true);
        X(long long, true);

        X(signed short, true);
        X(signed int, true);
        X(signed long, true);
        X(signed long long, true);

        X(unsigned short, false);
        X(unsigned int, false);
        X(unsigned long, false);
        X(unsigned long long, false);

        X(int8_t, true);
        X(int16_t, true);
        X(int32_t, true);
        X(int64_t, true);

        X(uint8_t, false);
        X(uint16_t, false);
        X(uint32_t, false);
        X(uint64_t, false);
#undef X

        if (strcmp(typename, "char") == 0 || strcmp(typename, "signed char") == 0 ||
            strcmp(typename, "unsigned char") == 0) {
            primitive = true;
            entry.type = ObjectEntryType_ASCII;
            for (size_t i = 0; i < count; i++) {
                arrpush(*entry.p_arr_data,
                        ((ObjectEntryData){
                            .ascii = ((char *)data)[i],
                        }));
            }
        }

        if (strcmp(typename, "float") == 0) {
            primitive = true;
            entry.type = ObjectEntryType_NUMBER;
            arrpush(*entry.p_arr_data,
                    ((ObjectEntryData){
                        .number = ((float *)data)[i],
                    }));
        }

        if (strcmp(typename, "double") == 0) {
            primitive = true;
            entry.type = ObjectEntryType_NUMBER;
            arrpush(*entry.p_arr_data,
                    ((ObjectEntryData){
                        .number = ((double *)data)[i],
                    }));
        }

        if (primitive) {
            continue;
        }

        entry.type = ObjectEntryType_OBJECT;

        ObjectEntryDataObject object = {0};

        object.type = type;
        object.data = data;

        object.p_arr_fields = malloc(sizeof(*object.p_arr_fields));
        ParrotScope_push_free(entry.scope, object.p_arr_fields);
        *object.p_arr_fields = NULL;
        ParrotScope_push_arrfree(entry.scope, *object.p_arr_fields);

        for (size_t j = 0; j < ParrotReflect_get_type_field_count(reflect, type); j++) {
            char *field_typename = ParrotReflect_get_type_field_typename(reflect, type, j);
            ParrotScope_push_free(work_scope, field_typename);

            ptrdiff_t field_type = ParrotReflect_resolve_type(reflect, field_typename);
            if (field_type < 0) {
                continue;
            }

            size_t ptr_level = 0;
            free(ParrotReflect_parse_type(field_typename, &ptr_level, NULL));

            if (ptr_level > 0 && !with_ptrs) {
                continue;
            }

            const uint8_t *field_data = data + i + ParrotReflect_get_type_field_offset(reflect, type, j);
            for (size_t k = 0; k < ptr_level; k++) {
                field_data = *(const uint8_t **)field_data;
                if (!field_data) {
                    break;
                }
            }

            if (!field_data) {
                continue;
            }

            ObjectEntryFieldEntry field = {0};

            field.name = ParrotReflect_get_type_field_name(reflect, type, j);
            ParrotScope_push_free(entry.scope, field.name);

            char *basename = ParrotReflect_get_type_field_basename(reflect, type, j);
            ParrotScope_push_free(work_scope, basename);

            size_t count = ParrotReflect_get_type_field_array_size(reflect, type, j);
            if (count == 0) {
                count = 1;
            }

            field.index =
                queue_object(p_arr_objects, p_hm_ptr_map, scope, reflect, field_type, field_data, count, with_ptrs);

            arrpush(*object.p_arr_fields, field);
            j += count - 1;
        }

        arrpush(*entry.p_arr_data,
                ((ObjectEntryData){
                    .object = object,
                }));
    }

    arr_objects[entry_index] = entry;
    hmput(hm_ptr_map, entry_key, arrlen(arr_objects) - 1);

    ParrotScope_delete(work_scope);
    return arrlen(arr_objects) - 1;
#undef hm_ptr_map
#undef arr_objects
}

static void generate_object_queue(ObjectEntry **p_arr_objects,
                                  ParrotScope *scope,
                                  ParrotReflect *reflect,
                                  size_t type,
                                  const uint8_t *data,
                                  bool with_ptrs) {
    ObjectPtrMap *hm_ptr_map = NULL;

    queue_object(p_arr_objects, &hm_ptr_map, scope, reflect, type, data, 1, with_ptrs);

    hmfree(hm_ptr_map);
}

void Parrot_serialize_bytes(
    ParrotBuffer *output, ParrotReflect *reflect, size_t type, const void *data_ptr, bool with_ptrs) {
    PARROT_FAIL_NULL(reflect);
    PARROT_FAIL_NULL(data_ptr);

    ParrotScope *scope = ParrotScope_new(NULL);

    ObjectEntry *arr_objects = NULL;
    ParrotScope_push_arrfree(scope, arr_objects);
    generate_object_queue(&arr_objects, scope, reflect, type, data_ptr, with_ptrs);

    uint32_t object_count = arrlen(arr_objects);
    ParrotBuffer_write32(output, ParrotBufferEndian_BIG, object_count);

    for (size_t i = 0; i < arrlen(arr_objects); i++) {
        ParrotBuffer_write16(output, ParrotBufferEndian_BIG, arr_objects[i].type);
        ParrotBuffer_write32(output, ParrotBufferEndian_BIG, arrlen(*arr_objects[i].p_arr_data));

        for (size_t j = 0; j < arrlen(*arr_objects[i].p_arr_data); j++) {
            switch (arr_objects[i].type) {
            case ObjectEntryType_OBJECT: {
                char *typename = ParrotReflect_get_type_name(reflect, (*arr_objects[i].p_arr_data)[j].object.type);
                ParrotScope_push_free(scope, typename);
                ParrotBuffer_write_ascii(output, typename);

                size_t field_count = arrlen(*(*arr_objects[i].p_arr_data)[j].object.p_arr_fields);
                ParrotBuffer_write32(output, ParrotBufferEndian_BIG, field_count);

                for (size_t k = 0; k < field_count; k++) {
                    ObjectEntryFieldEntry *field = *(*arr_objects[i].p_arr_data)[j].object.p_arr_fields + k;
                    ParrotBuffer_write_ascii(output, field->name);
                    ParrotBuffer_write32(output, ParrotBufferEndian_BIG, field->index);
                }
            } break;
            case ObjectEntryType_SINT:
                ParrotBuffer_write64s(output, ParrotBufferEndian_BIG, (*arr_objects[i].p_arr_data)[j].sint);
                break;
            case ObjectEntryType_UINT:
                ParrotBuffer_write64(output, ParrotBufferEndian_BIG, (*arr_objects[i].p_arr_data)[j].uint);
                break;
            case ObjectEntryType_ASCII:
                ParrotBuffer_write8(output, Parrot_char_to_ascii((*arr_objects[i].p_arr_data)[j].ascii));
                break;
            case ObjectEntryType_NUMBER:
                ParrotBuffer_write64s(
                    output, ParrotBufferEndian_BIG, ParrotFixed64i_from_double((*arr_objects[i].p_arr_data)[j].number));
                break;
            }
        }
    }

    ParrotScope_delete(scope);
}

typedef struct {
    const ObjectEntry *object;

    ptrdiff_t type;

    uint8_t *data;
    size_t count;
} DeserializeStateObject;

typedef enum {
    DeserializeStateRelocationType_MOVE = 0,
    DeserializeStateRelocationType_POINT,
} DeserializeStateRelocationType;

typedef struct {
    DeserializeStateRelocationType type;

    size_t dest_object;
    size_t dest_offset;
    size_t src_object;

    union {
        struct {
            size_t ptr_level;
        } point;
    } unique_data;
} DeserializeStateRelocation;

typedef struct {
    DeserializeStateObject *arr_objects;
    DeserializeStateRelocation *arr_relocations;
} DeserializeState;

static void deserialize_object(
    DeserializeState *state, size_t object, ptrdiff_t type, ParrotReflect *reflect, size_t count, bool with_ptrs) {
    PARROT_RET_COND(state->arr_objects[object].data);

    ParrotScope *scope = ParrotScope_new(NULL);

    state->arr_objects[object].type = type;

    state->arr_objects[object].data = malloc(ParrotReflect_get_type_size(reflect, type) * count);
    memset(state->arr_objects[object].data, 0, ParrotReflect_get_type_size(reflect, type) * count);
    state->arr_objects[object].count = count;

    char *typename = ParrotReflect_get_type_name(reflect, type);
    ParrotScope_push_free(scope, typename);

    switch (state->arr_objects[object].object->type) {
    case ObjectEntryType_OBJECT: {
        for (size_t i = 0; i < arrlen(*state->arr_objects[object].object->data.object.p_arr_fields); i++) {
            ParrotScope *field_scope = ParrotScope_new(scope);

            ptrdiff_t field = ParrotReflect_get_type_field(
                reflect, type, (*state->arr_objects[object].object->data.object.p_arr_fields)[i].name);

            if (field < 0) {
                ParrotScope_delete(field_scope);
                continue;
            }

            char *field_typename = ParrotReflect_get_type_field_typename(reflect, type, field);
            ParrotScope_push_free(scope, field_typename);

            size_t ptr_level = 0;
            ptrdiff_t field_type = ParrotReflect_resolve_type_ex(reflect, field_typename, NULL, &ptr_level, NULL);
            if (field_type < 0) {
                ParrotScope_delete(field_scope);
                continue;
            }

            size_t count = ParrotReflect_get_type_field_array_size(reflect, type, field);
            if (count == 0) {
                count = 1;
            }

            DeserializeStateRelocation relocation = {
                .dest_object = object,
                .dest_offset = ParrotReflect_get_type_field_offset(reflect, type, field),
                .src_object = (*state->arr_objects[object].object->data.object.p_arr_fields)[i].index,
            };
            relocation.type =
                ptr_level <= 0 ? DeserializeStateRelocationType_MOVE : DeserializeStateRelocationType_POINT;

            if (relocation.type == DeserializeStateRelocationType_POINT) {
                relocation.unique_data.point.ptr_level = ptr_level;
            }

            deserialize_object(state, relocation.src_object, field_type, reflect, count, with_ptrs);

            ParrotReflect_get_type_field_size(reflect, type, field);
            arrpush(state->arr_relocations, relocation);

            ParrotScope_delete(field_scope);
        }
    } break;
    case ObjectEntryType_SINT: {
        size_t copy_count = PARROT_MIN(count, arrlen(*state->arr_objects[object].object->data.p_arr_sint));
        if (strcmp(typename, "int8_t") == 0) {
            int8_t *dest = (int8_t *)state->arr_objects[object].data;
            for (size_t i = 0; i < copy_count; i++) {
                dest[i] = (*state->arr_objects[object].object->data.p_arr_sint)[i];
            }
        } else if (strcmp(typename, "int16_t") == 0) {
            int16_t *dest = (int16_t *)state->arr_objects[object].data;
            for (size_t i = 0; i < copy_count; i++) {
                dest[i] = (*state->arr_objects[object].object->data.p_arr_sint)[i];
            }
        } else if (strcmp(typename, "int32_t") == 0) {
            int32_t *dest = (int32_t *)state->arr_objects[object].data;
            for (size_t i = 0; i < copy_count; i++) {
                dest[i] = (*state->arr_objects[object].object->data.p_arr_sint)[i];
            }
        } else if (strcmp(typename, "int64_t") == 0) {
            int64_t *dest = (int64_t *)state->arr_objects[object].data;
            for (size_t i = 0; i < copy_count; i++) {
                dest[i] = (*state->arr_objects[object].object->data.p_arr_sint)[i];
            }
        }
    } break;
    case ObjectEntryType_UINT: {
        size_t copy_count = PARROT_MIN(count, arrlen(*state->arr_objects[object].object->data.p_arr_uint));
        if (strcmp(typename, "uint8_t") == 0) {
            uint8_t *dest = (uint8_t *)state->arr_objects[object].data;
            for (size_t i = 0; i < copy_count; i++) {
                dest[i] = (*state->arr_objects[object].object->data.p_arr_uint)[i];
            }
        } else if (strcmp(typename, "uint16_t") == 0) {
            uint16_t *dest = (uint16_t *)state->arr_objects[object].data;
            for (size_t i = 0; i < copy_count; i++) {
                dest[i] = (*state->arr_objects[object].object->data.p_arr_uint)[i];
            }
        } else if (strcmp(typename, "uint32_t") == 0) {
            uint32_t *dest = (uint32_t *)state->arr_objects[object].data;
            for (size_t i = 0; i < copy_count; i++) {
                dest[i] = (*state->arr_objects[object].object->data.p_arr_uint)[i];
            }
        } else if (strcmp(typename, "uint64_t") == 0) {
            uint64_t *dest = (uint64_t *)state->arr_objects[object].data;
            for (size_t i = 0; i < copy_count; i++) {
                dest[i] = (*state->arr_objects[object].object->data.p_arr_uint)[i];
            }
        }
    } break;
    case ObjectEntryType_ASCII: {
        size_t copy_count = PARROT_MIN(count, arrlen(*state->arr_objects[object].object->data.p_arr_ascii));
        if (strcmp(typename, "char") == 0 || strcmp(typename, "signed char") == 0 ||
            strcmp(typename, "unsigned char") == 0) {
            char *dest = (char *)state->arr_objects[object].data;
            for (size_t i = 0; i < copy_count; i++) {
                dest[i] = (*state->arr_objects[object].object->data.p_arr_ascii)[i];
            }
        }
    } break;
    case ObjectEntryType_NUMBER: {
        size_t copy_count = PARROT_MIN(count, arrlen(*state->arr_objects[object].object->data.p_arr_number));
        if (strcmp(typename, "float") == 0) {
            float *dest = (float *)state->arr_objects[object].data;
            for (size_t i = 0; i < copy_count; i++) {
                float value = (float)(*state->arr_objects[object].object->data.p_arr_number)[i];
                memcpy(&dest[i], &value, sizeof(float));
            }
        } else if (strcmp(typename, "double") == 0) {
            double *dest = (double *)state->arr_objects[object].data;
            for (size_t i = 0; i < copy_count; i++) {
                memcpy(&dest[i], &(*state->arr_objects[object].object->data.p_arr_number)[i], sizeof(double));
            }
        }
    } break;
    }

    ParrotScope_delete(scope);
}

static void *
deserialize(ObjectEntry *objects, size_t object_count, ParrotReflect *reflect, size_t *out_type, bool with_ptrs) {
    (void)with_ptrs;
    PARROT_RET_COND_V(object_count <= 0, NULL);

    ParrotScope *scope = ParrotScope_new(NULL);

    *out_type = objects[0].data.object.type;

    DeserializeState state = {0};
    ParrotScope_push_arrfree(scope, state.arr_objects);
    ParrotScope_push_arrfree(scope, state.arr_relocations);

    for (size_t i = 0; i < object_count; i++) {
        DeserializeStateObject object = {0};

        object.object = &objects[i];

        arrpush(state.arr_objects, object);
    }

    deserialize_object(&state, 0, *out_type, reflect, 1, with_ptrs);

    size_t pool_size = ParrotReflect_get_type_size(reflect, state.arr_objects[0].type);
    for (size_t i = 0; i < arrlen(state.arr_relocations); i++) {
        DeserializeStateRelocation relocation = state.arr_relocations[i];

        if (relocation.type != DeserializeStateRelocationType_POINT) {
            continue;
        }

        pool_size += (relocation.unique_data.point.ptr_level - 1) * sizeof(void *);

        // OPTIMIZE: Reduce unneeded size increase by only increasing pool size once per point of that object
        pool_size += ParrotReflect_get_type_size(reflect, state.arr_objects[relocation.src_object].type);
    }
    state.arr_objects[0].data = realloc(state.arr_objects[0].data, pool_size);
    uint8_t *pool_ptr = state.arr_objects[0].data + ParrotReflect_get_type_size(reflect, state.arr_objects[0].type);

    ParrotSizeSet *shm_relocated = NULL;
    ParrotScope_push_hmfree(scope, shm_relocated);

    hmputs(shm_relocated,
           ((ParrotSizeSet){
               .key = 0,
           }));

    for (size_t i = 0; i < arrlen(state.arr_relocations); i++) {
        DeserializeStateRelocation relocation = state.arr_relocations[i];

        if (relocation.type != DeserializeStateRelocationType_MOVE) {
            continue;
        }

        memcpy(state.arr_objects[relocation.dest_object].data + relocation.dest_offset,
               state.arr_objects[relocation.src_object].data,
               ParrotReflect_get_type_size(reflect, state.arr_objects[relocation.src_object].type) *
                   state.arr_objects[relocation.src_object].count);
        free(state.arr_objects[relocation.src_object].data);
        state.arr_objects[relocation.src_object].data =
            state.arr_objects[relocation.dest_object].data + relocation.dest_offset;

        hmputs(shm_relocated,
               ((ParrotSizeSet){
                   .key = relocation.src_object,
               }));
    }

    for (size_t i = 0; i < arrlen(state.arr_relocations); i++) {
        DeserializeStateRelocation relocation = state.arr_relocations[i];

        if (relocation.type != DeserializeStateRelocationType_POINT) {
            continue;
        }

        if (hmgeti(shm_relocated, relocation.src_object) >= 0) {
            continue;
        }
        memcpy(pool_ptr,
               state.arr_objects[relocation.src_object].data,
               ParrotReflect_get_type_size(reflect, state.arr_objects[relocation.src_object].type) *
                   state.arr_objects[relocation.src_object].count);
        free(state.arr_objects[relocation.src_object].data);

        state.arr_objects[relocation.src_object].data = pool_ptr;
        pool_ptr += ParrotReflect_get_type_size(reflect, state.arr_objects[relocation.src_object].type) *
                    state.arr_objects[relocation.src_object].count;

        hmputs(shm_relocated,
               ((ParrotSizeSet){
                   .key = relocation.src_object,
               }));
    }

    for (size_t i = 0; i < arrlen(state.arr_relocations); i++) {
        DeserializeStateRelocation relocation = state.arr_relocations[i];

        if (relocation.type != DeserializeStateRelocationType_POINT) {
            continue;
        }

        memcpy(state.arr_objects[relocation.dest_object].data + relocation.dest_offset,
               &state.arr_objects[relocation.src_object].data,
               sizeof(void *));
    }

    void *ret = state.arr_objects[0].data;
    ParrotScope_delete(scope);
    return ret;
}

void *Parrot_deserialize_bytes(ParrotBuffer *input, ParrotReflect *reflect, size_t *out_type, bool with_ptrs) {
    PARROT_FAIL_NULL(out_type);

    ParrotScope *scope = ParrotScope_new(NULL);

    ObjectEntry *arr_objects = NULL;
    ParrotScope_push_arrfree(scope, arr_objects);

#define FAIL()                                                                                                          \
    do {                                                                                                                \
        ParrotScope_delete(scope);                                                                                      \
        return NULL;                                                                                                    \
        \                                                                                                               \
    } while (0)
#define CHECK_FAIL(expr)                                                                                                \
    do {                                                                                                                \
        if (!(expr)) {                                                                                                  \
            FAIL();                                                                                                     \
        }                                                                                                               \
        \                                                                                                               \
    } while (0)

    uint32_t object_count = 0;
    CHECK_FAIL(ParrotBuffer_read32(input, ParrotBufferEndian_BIG, &object_count));

    for (size_t i = 0; i < object_count; i++) {
        ParrotScope *object_scope = ParrotScope_new(scope);

        ObjectEntry entry = {0};
        entry.scope = ParrotScope_new(object_scope);

        CHECK_FAIL(ParrotBuffer_read(input, ParrotBufferEndian_BIG, &entry.type, sizeof(entry.type)));

        switch (entry.type) {
        case ObjectEntryType_OBJECT: {
            char *arr_object_type = NULL;
            for (;;) {
                uint8_t ascii = 0;
                CHECK_FAIL(ParrotBuffer_read8(input, &ascii));

                arrpush(arr_object_type, Parrot_ascii_to_char(ascii));

                if (ascii == 0) {
                    break;
                }
            }

            entry.data.object.type = ParrotReflect_resolve_type(reflect, arr_object_type);
            if (entry.data.object.type < 0) {
                FAIL();
            }

            uint32_t field_count = 0;
            CHECK_FAIL(ParrotBuffer_read32(input, ParrotBufferEndian_BIG, &field_count));

            entry.data.object.p_arr_fields = malloc(sizeof(*entry.data.object.p_arr_fields));
            ParrotScope_push_free(entry.scope, entry.data.object.p_arr_fields);
            *entry.data.object.p_arr_fields = NULL;
            ParrotScope_push_arrfree(entry.scope, *entry.data.object.p_arr_fields);

            for (size_t j = 0; j < field_count; j++) {
                ParrotScope *field_scope = ParrotScope_new(object_scope);

                char *arr_field_name = NULL;
                ParrotScope_push_arrfree(field_scope, arr_field_name);
                for (;;) {
                    uint8_t ascii = 0;
                    CHECK_FAIL(ParrotBuffer_read8(input, &ascii));

                    arrpush(arr_field_name, Parrot_ascii_to_char(ascii));

                    if (ascii == 0) {
                        break;
                    }
                }

                uint32_t index = 0;
                CHECK_FAIL(ParrotBuffer_read32(input, ParrotBufferEndian_BIG, &index));

                ObjectEntryFieldEntry field_entry = {0};

                field_entry.name = calloc(arrlen(arr_field_name), sizeof(char));
                ParrotScope_push_free(entry.scope, field_entry.name);
                strcpy(field_entry.name, arr_field_name);

                field_entry.index = index;

                arrpush(*entry.data.object.p_arr_fields, field_entry);

                ParrotScope_delete(field_scope);
            }
            arrfree(arr_object_type);
        } break;
        case ObjectEntryType_SINT: {
            uint32_t count = 0;
            CHECK_FAIL(ParrotBuffer_read32(input, ParrotBufferEndian_BIG, &count));

            entry.data.p_arr_sint = malloc(sizeof(*entry.data.p_arr_sint));
            ParrotScope_push_free(entry.scope, entry.data.p_arr_sint);
            *entry.data.p_arr_sint = NULL;
            ParrotScope_push_arrfree(entry.scope, *entry.data.p_arr_sint);

            while (count--) {
                int64_t number = 0;
                CHECK_FAIL(ParrotBuffer_read64s(input, ParrotBufferEndian_BIG, &number));

                arrpush(*entry.data.p_arr_sint, number);
            }
        } break;

        case ObjectEntryType_UINT: {
            uint32_t count = 0;
            CHECK_FAIL(ParrotBuffer_read32(input, ParrotBufferEndian_BIG, &count));

            entry.data.p_arr_uint = malloc(sizeof(*entry.data.p_arr_uint));
            ParrotScope_push_free(entry.scope, entry.data.p_arr_uint);
            *entry.data.p_arr_uint = NULL;
            ParrotScope_push_arrfree(entry.scope, *entry.data.p_arr_uint);

            while (count--) {
                uint64_t number = 0;
                ParrotBuffer_read64(input, ParrotBufferEndian_BIG, &number);

                arrpush(*entry.data.p_arr_uint, number);
            }
        } break;
        case ObjectEntryType_ASCII: {
            uint32_t count = 0;
            CHECK_FAIL(ParrotBuffer_read32(input, ParrotBufferEndian_BIG, &count));

            entry.data.p_arr_ascii = malloc(sizeof(*entry.data.p_arr_ascii));
            ParrotScope_push_free(entry.scope, entry.data.p_arr_ascii);
            *entry.data.p_arr_ascii = NULL;
            ParrotScope_push_arrfree(entry.scope, *entry.data.p_arr_ascii);

            while (count--) {
                uint8_t ascii = 0;
                CHECK_FAIL(ParrotBuffer_read8(input, &ascii));

                arrpush(*entry.data.p_arr_ascii, Parrot_ascii_to_char(ascii));
            }
        } break;
        case ObjectEntryType_NUMBER: {
            uint32_t count = 0;
            CHECK_FAIL(ParrotBuffer_read32(input, ParrotBufferEndian_BIG, &count));

            entry.data.p_arr_number = malloc(sizeof(*entry.data.p_arr_number));
            ParrotScope_push_free(entry.scope, entry.data.p_arr_number);
            *entry.data.p_arr_number = NULL;
            ParrotScope_push_arrfree(entry.scope, *entry.data.p_arr_number);

            while (count--) {
                ParrotFixed64i number = 0;
                CHECK_FAIL(ParrotBuffer_read64s(input, ParrotBufferEndian_BIG, &number));

                arrpush(*entry.data.p_arr_number, ParrotFixed64i_to_double(number));
            }
        } break;
        default:
            FAIL();
            break;
        }

        ParrotScope_set_parent(entry.scope, scope);
        ParrotScope_delete(object_scope);

        arrpush(arr_objects, entry);
    }

#undef CHECK_FAIL
#undef FAIL

    void *ret = deserialize(arr_objects, arrlen(arr_objects), reflect, out_type, with_ptrs);
    ParrotScope_delete(scope);
    return ret;
}

#endif

#else
// C++ is not backwards compatible enough with C99 for this project
#error "Cannot implement core in C++"
#endif
#endif

#endif // _PARROT_CORE_H_
