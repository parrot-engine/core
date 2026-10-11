#include "parrot/core/serialize.h"
#include "parrot/core/array.h"
#include "parrot/core/math.h"

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

    ptrdiff_t index = ParrotArray_find(hm_ptr_map, entry_key);

    if (index >= 0) {
        ParrotScope_delete(work_scope);
        return index;
    }

    char *typename = ParrotReflect_get_type_name_malloc(reflect, type);
    ParrotScope_push_free(work_scope, typename);

    uint32_t entry_index = ParrotArray_size(arr_objects);
    ParrotArray_push(arr_objects, (ObjectEntry){0});

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
                ParrotArray_push(*entry.p_arr_data,                                                                     \
                                 (signed_) ? ((ObjectEntryData){                                                        \
                                                 .sint = ((int8_t *)data)[i],                                           \
                                             }) :                                                                       \
                                             ((ObjectEntryData){                                                        \
                                                 .uint = ((uint8_t *)data)[i],                                          \
                                             }));                                                                       \
                break;                                                                                                  \
            case 2:                                                                                                     \
                ParrotArray_push(*entry.p_arr_data,                                                                     \
                                 (signed_) ? ((ObjectEntryData){                                                        \
                                                 .sint = ((int16_t *)data)[i],                                          \
                                             }) :                                                                       \
                                             ((ObjectEntryData){                                                        \
                                                 .uint = ((uint16_t *)data)[i],                                         \
                                             }));                                                                       \
                break;                                                                                                  \
            case 4:                                                                                                     \
                ParrotArray_push(*entry.p_arr_data,                                                                     \
                                 (signed_) ? ((ObjectEntryData){                                                        \
                                                 .sint = ((int32_t *)data)[i],                                          \
                                             }) :                                                                       \
                                             ((ObjectEntryData){                                                        \
                                                 .uint = ((uint32_t *)data)[i],                                         \
                                             }));                                                                       \
                break;                                                                                                  \
            case 8:                                                                                                     \
                ParrotArray_push(*entry.p_arr_data,                                                                     \
                                 (signed_) ? ((ObjectEntryData){                                                        \
                                                 .sint = ((int64_t *)data)[i],                                          \
                                             }) :                                                                       \
                                             ((ObjectEntryData){                                                        \
                                                 .uint = ((uint64_t *)data)[i],                                         \
                                             }));                                                                       \
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
                ParrotArray_push(*entry.p_arr_data,
                                 ((ObjectEntryData){
                                     .ascii = ((char *)data)[i],
                                 }));
            }
        }

        if (strcmp(typename, "float") == 0) {
            primitive = true;
            entry.type = ObjectEntryType_NUMBER;
            ParrotArray_push(*entry.p_arr_data,
                             ((ObjectEntryData){
                                 .number = ((float *)data)[i],
                             }));
        }

        if (strcmp(typename, "double") == 0) {
            primitive = true;
            entry.type = ObjectEntryType_NUMBER;
            ParrotArray_push(*entry.p_arr_data,
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
            char *field_typename = ParrotReflect_get_type_field_typename_malloc(reflect, type, j);
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

            field.name = ParrotReflect_get_type_field_name_malloc(reflect, type, j);
            ParrotScope_push_free(entry.scope, field.name);

            char *basename = ParrotReflect_get_type_field_basename_malloc(reflect, type, j);
            ParrotScope_push_free(work_scope, basename);

            size_t count = ParrotReflect_get_type_field_array_size(reflect, type, j);
            if (count == 0) {
                count = 1;
            }

            field.index =
                queue_object(p_arr_objects, p_hm_ptr_map, scope, reflect, field_type, field_data, count, with_ptrs);

            ParrotArray_push(*object.p_arr_fields, field);
            j += count - 1;
        }

        ParrotArray_push(*entry.p_arr_data,
                         ((ObjectEntryData){
                             .object = object,
                         }));
    }

    arr_objects[entry_index] = entry;
    ParrotArray_put(hm_ptr_map,
                    ((ObjectPtrMap){
                        .key = entry_key,
                        .value = ParrotArray_size(arr_objects) - 1,
                    }));

    ParrotScope_delete(work_scope);
    return ParrotArray_size(arr_objects) - 1;
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

    ParrotArray_free(hm_ptr_map);
}

void Parrot_serialize_bytes_malloc(
    ParrotBuffer *output, ParrotReflect *reflect, size_t type, const void *data_ptr, bool with_ptrs) {
    PARROT_FAIL_NULL(reflect);
    PARROT_FAIL_NULL(data_ptr);

    ParrotScope *scope = ParrotScope_new(NULL);

    ObjectEntry *arr_objects = NULL;
    ParrotScope_push_arrfree(scope, arr_objects);
    generate_object_queue(&arr_objects, scope, reflect, type, data_ptr, with_ptrs);

    uint32_t object_count = ParrotArray_size(arr_objects);
    ParrotBuffer_write32(output, ParrotBufferEndian_BIG, object_count);

    for (size_t i = 0; i < ParrotArray_size(arr_objects); i++) {
        ParrotBuffer_write16(output, ParrotBufferEndian_BIG, arr_objects[i].type);
        ParrotBuffer_write32(output, ParrotBufferEndian_BIG, ParrotArray_size(*arr_objects[i].p_arr_data));

        for (size_t j = 0; j < ParrotArray_size(*arr_objects[i].p_arr_data); j++) {
            switch (arr_objects[i].type) {
            case ObjectEntryType_OBJECT: {
                char *typename =
                    ParrotReflect_get_type_name_malloc(reflect, (*arr_objects[i].p_arr_data)[j].object.type);
                ParrotScope_push_free(scope, typename);
                ParrotBuffer_write_ascii(output, typename);

                size_t field_count = ParrotArray_size(*(*arr_objects[i].p_arr_data)[j].object.p_arr_fields);
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

/*

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
            ParrotArray_push(state->arr_relocations, relocation);

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

        ParrotArray_push(state.arr_objects, object);
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

#define FAIL() \
    do { \
        ParrotScope_delete(scope); \
        return NULL; \
        \ \ } while (0)
#define CHECK_FAIL(expr) \
    do { \
        if (!(expr)) { \
            FAIL(); \
        } \
        \ \ } while (0)

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

                ParrotArray_push(arr_object_type, Parrot_ascii_to_char(ascii));

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

                    ParrotArray_push(arr_field_name, Parrot_ascii_to_char(ascii));

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

                ParrotArray_push(*entry.data.object.p_arr_fields, field_entry);

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

                ParrotArray_push(*entry.data.p_arr_sint, number);
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

                ParrotArray_push(*entry.data.p_arr_uint, number);
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

                ParrotArray_push(*entry.data.p_arr_ascii, Parrot_ascii_to_char(ascii));
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

                ParrotArray_push(*entry.data.p_arr_number, ParrotFixed64i_to_double(number));
            }
        } break;
        default:
            FAIL();
            break;
        }

        ParrotScope_set_parent(entry.scope, scope);
        ParrotScope_delete(object_scope);

        ParrotArray_push(arr_objects, entry);
    }

#undef CHECK_FAIL
#undef FAIL

    void *ret = deserialize(arr_objects, arrlen(arr_objects), reflect, out_type, with_ptrs);
    ParrotScope_delete(scope);
    return ret;
}
*/
