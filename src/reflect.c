#include <ctype.h>
#include <stdio.h>

#include "parrot/core/array.h"
#include "parrot/core/reflect.h"
#include "parrot/core/scope.h"

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

    ParrotScope_push_arrfree(self->scope, self->sh_types);
    ParrotScope_push_arrfree(self->scope, self->sh_aliases);

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
    ParrotScope_push_arrfree(type_info.scope, *type_info.sh_fields);

    type_info.sh_tags = malloc(sizeof(type_info.sh_tags));
    ParrotScope_push_free(type_info.scope, type_info.sh_tags);
    *type_info.sh_tags = NULL;
    ParrotScope_push_arrfree(type_info.scope, *type_info.sh_tags);
    if (description->tags) {
        for (size_t i = 0; description->tags[i][0]; i++) {
            const char **tag = description->tags[i];

            char *key = strcpy(calloc(strlen(tag[0]) + 1, sizeof(char)), tag[0]);
            ParrotScope_push_free(type_info.scope, key);

            char *value = strcpy(calloc(strlen(tag[1]) + 1, sizeof(char)), tag[1]);
            ParrotScope_push_free(type_info.scope, value);

            ParrotArray_puts(*type_info.sh_tags, ((ParrotReflectTag){key, value}));
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
                ParrotArray_push(arr_dimensions, size);

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

                if (ParrotArray_size(arr_dimensions) > 1) {
                    size_t key_len = strlen(description->name) + ParrotArray_size(arr_dimensions) * 24;
                    field_info.key = calloc(key_len, sizeof(char));
                    strcpy(field_info.key, description->name);

                    size_t remaining = i;
                    size_t *indices = calloc(ParrotArray_size(arr_dimensions), sizeof(size_t));

                    for (size_t j = ParrotArray_size(arr_dimensions); j-- > 0;) {
                        indices[j] = remaining % arr_dimensions[j];
                        remaining /= arr_dimensions[j];
                    }

                    for (size_t j = 0; j < ParrotArray_size(arr_dimensions); j++) {
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
                ParrotScope_push_arrfree(field_info.scope, *field_info.sh_tags);
                if (description->tags) {
                    for (size_t j = 0; description->tags[j][0]; j++) {
                        const char **tag = description->tags[j];

                        char *key = strcpy(calloc(strlen(tag[0]) + 1, sizeof(char)), tag[0]);
                        ParrotScope_push_free(field_info.scope, key);

                        char *value = strcpy(calloc(strlen(tag[1]) + 1, sizeof(char)), tag[1]);
                        ParrotScope_push_free(field_info.scope, value);

                        ParrotArray_put(*field_info.sh_tags, ((ParrotReflectTag){key, value}));
                    }
                }

                field_info.array_index = i;
                field_info.array_size = ParrotArray_size(arr_dimensions) > 0 ? dimension_count : 0;

                ParrotArray_puts(*type_info.sh_fields, field_info);
            }

            ParrotScope_delete(scope);
        } break;

        default:
            FAIL_INVALID_TYPE(description->type);
            break;
        }

        description++;
    }

    ParrotArray_puts(self->sh_types, type_info);
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

        ParrotArray_puts(self->sh_aliases, alias_info);
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
    size_t type_info_index = ParrotArray_find(self->sh_types, type);
    ParrotReflectTypeInfo *type_info = &self->sh_types[type_info_index];
    if (type_info) {
        ParrotReflectTypeInfo local_type_info = *type_info;
        ParrotArray_del(self->sh_types, type_info_index);

        ParrotScope_delete(local_type_info.scope);
        return;
    }

    size_t alias_info_index = ParrotArray_find(self->sh_aliases, type);
    ParrotReflectAliasInfo *alias_info = &self->sh_aliases[alias_info_index];
    if (alias_info) {
        ParrotReflectAliasInfo local_alias_info = *alias_info;
        ParrotArray_del(self->sh_aliases, alias_info_index);

        ParrotScope_delete(local_alias_info.scope);
        return;
    }
}

size_t ParrotReflect_get_type_count(ParrotReflect *self) {
    PARROT_FAIL_NULL(self);
    return ParrotArray_size(self->sh_types);
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
        ParrotArray_push(arr_type_str, *type++);
    }

    while (arr_type_str[ParrotArray_size(arr_type_str) - 1] == ' ' ||
           arr_type_str[ParrotArray_size(arr_type_str) - 1] == '*') {
        if (out_ptr_level && arr_type_str[ParrotArray_size(arr_type_str) - 1] == '*') {
            (*out_ptr_level)++;
        }
        ParrotArray_del(arr_type_str, ParrotArray_size(arr_type_str) - 1);
    }

    ParrotArray_push(arr_type_str, '\0');

    char *ret = strcpy(calloc(ParrotArray_size(arr_type_str), sizeof(char)), arr_type_str);
    ParrotArray_free(arr_type_str);
    return ret;
}

ptrdiff_t ParrotReflect_resolve_type_ex(
    ParrotReflect *self, const char *type, char **out_parsed_type, size_t *out_ptr_level, bool *out_is_const) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(type);

    char *parsed_type = ParrotReflect_parse_type(type, out_ptr_level, out_is_const);

    type = parsed_type;
    while (ParrotArray_find(self->sh_aliases, type) >= 0) {
        type = ((ParrotReflectAliasInfo *)ParrotArray_findp(self->sh_aliases, type))->value;
    }

    ptrdiff_t ret = ParrotArray_find(self->sh_types, type);
    if (out_parsed_type) {
        *out_parsed_type = parsed_type;
    } else {
        free(parsed_type);
    }
    return ret;
}

ptrdiff_t ParrotReflect_resolve_type_by_index(ParrotReflect *self, size_t index) {
    PARROT_FAIL_NULL(self);
    return ParrotArray_size(self->sh_types) > index ? (ptrdiff_t)index : -1;
}

static ParrotReflectTypeInfo *resolve_type_info(ParrotReflect *self, size_t type) {
    PARROT_FAIL_NULL(self);
    PARROT_RET_COND_V(type >= ParrotArray_size(self->sh_types), NULL);

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

    PARROT_RET_COND_V(ParrotArray_find(*type_info->sh_tags, key) <= 0, NULL);
    char *tag = ((ParrotReflectTag *)ParrotArray_findp(*type_info->sh_tags, key))->value;
    return strcpy(calloc(strlen(tag) + 1, sizeof(char)), tag);
}

ptrdiff_t ParrotReflect_get_type_field(ParrotReflect *self, size_t type, const char *name) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    return ParrotArray_find(*type_info->sh_fields, name);
}

size_t ParrotReflect_get_type_field_count(ParrotReflect *self, size_t type) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    return ParrotArray_size(*type_info->sh_fields);
}

size_t ParrotReflect_get_type_field_offset(ParrotReflect *self, size_t type, size_t index) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(index >= ParrotArray_size(*type_info->sh_fields));
    return (*type_info->sh_fields)[index].offset;
}

char *ParrotReflect_get_type_field_typename(ParrotReflect *self, size_t type, size_t field) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(field >= ParrotArray_size(*type_info->sh_fields));

    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];
    return strcpy(calloc(strlen(field_info->type) + 1, sizeof(char)), field_info->type);
}

char *ParrotReflect_get_type_field_name(ParrotReflect *self, size_t type, size_t field) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(field >= ParrotArray_size(*type_info->sh_fields));

    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];
    return strcpy(calloc(strlen(field_info->key) + 1, sizeof(char)), field_info->key);
}

char *ParrotReflect_get_type_field_basename(ParrotReflect *self, size_t type, size_t field) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(field >= ParrotArray_size(*type_info->sh_fields));

    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];
    return strcpy(calloc(strlen(field_info->key) + 1, sizeof(char)), field_info->basename);
}

size_t ParrotReflect_get_type_field_size(ParrotReflect *self, size_t type, size_t field) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(field >= ParrotArray_size(*type_info->sh_fields));

    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];
    return field_info->size;
}

size_t ParrotReflect_get_type_field_array_index(ParrotReflect *self, size_t type, size_t field) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(field >= ParrotArray_size(*type_info->sh_fields));

    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];
    return field_info->array_index;
}

size_t ParrotReflect_get_type_field_array_size(ParrotReflect *self, size_t type, size_t field) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);
    PARROT_FAIL_COND(field >= ParrotArray_size(*type_info->sh_fields));

    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];
    return field_info->array_size;
}

char *ParrotReflect_get_type_field_tag(ParrotReflect *self, size_t type, size_t field, const char *key) {
    ParrotReflectTypeInfo *type_info = resolve_type_info(self, type);
    PARROT_FAIL_NULL(type_info);

    PARROT_FAIL_COND(field >= ParrotArray_size(*type_info->sh_fields));
    ParrotReflectTypeFieldInfo *field_info = &(*type_info->sh_fields)[field];

    PARROT_RET_COND_V(ParrotArray_find(*field_info->sh_tags, key) <= 0, NULL);
    char *tag = ((ParrotReflectTag *)ParrotArray_findp(*field_info->sh_tags, key))->value;
    return strcpy(calloc(strlen(tag) + 1, sizeof(char)), tag);
}

#undef FAIL_INVALID_TYPE
#undef FAIL_REGISTERED_TYPE
#undef FAIL_HEADER
