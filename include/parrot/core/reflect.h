#ifndef PARROT_CORE_REFLECT_H_
#define PARROT_CORE_REFLECT_H_

#include <stdbool.h>
#include <stddef.h>

#include "parrot/core/util.h"

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

PARROT_API char *ParrotReflect_get_type_name_malloc(ParrotReflect *self, size_t type);
PARROT_API size_t ParrotReflect_get_type_size(ParrotReflect *self, size_t type);
PARROT_API char *ParrotReflect_get_type_tag_malloc(ParrotReflect *self, size_t type, const char *key);

/// 0< = Not found
PARROT_API ptrdiff_t ParrotReflect_get_type_field(ParrotReflect *self, size_t type, const char *name);
PARROT_API size_t ParrotReflect_get_type_field_count(ParrotReflect *self, size_t type);
PARROT_API size_t ParrotReflect_get_type_field_offset(ParrotReflect *self, size_t type, size_t field);
PARROT_API char *ParrotReflect_get_type_field_typename_malloc(ParrotReflect *self, size_t type, size_t field);
PARROT_API char *ParrotReflect_get_type_field_name_malloc(ParrotReflect *self, size_t type, size_t field);
/*
 * @returns name of field without array index allocated with malloc() that the caller takes ownership of. Result can't
 * be used as a field lookup
 */
PARROT_API char *ParrotReflect_get_type_field_basename_malloc(ParrotReflect *self, size_t type, size_t field);
PARROT_API size_t ParrotReflect_get_type_field_size(ParrotReflect *self, size_t type, size_t field);
PARROT_API size_t ParrotReflect_get_type_field_array_index(ParrotReflect *self, size_t type, size_t field);
/// Returns 0 if no array
PARROT_API size_t ParrotReflect_get_type_field_array_size(ParrotReflect *self, size_t type, size_t field);
PARROT_API char *
ParrotReflect_get_type_field_tag_malloc(ParrotReflect *self, size_t type, size_t field, const char *key);

#endif // PARROT_CORE_REFLECT_H_
