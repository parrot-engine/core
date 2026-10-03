#ifndef PARROT_CORE_BINARY_H_
#define PARROT_CORE_BINARY_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "parrot/core/util.h"
#include "parrot/core/scope.h"
#include "parrot/core/reflect.h"

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

#endif // PARROT_CORE_BINARY_H_
