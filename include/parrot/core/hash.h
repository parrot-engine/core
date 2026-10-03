#ifndef PARROT_CORE_HASH_H_
#define PARROT_CORE_HASH_H_

#include <stdint.h>

#include "parrot/core/util.h"

typedef uint32_t ParrotCRC32;

PARROT_API ParrotCRC32 Parrot_crc32(const void *data, size_t size);
PARROT_API ParrotCRC32 Parrot_crc32_combine(ParrotCRC32 crc, const void *data, size_t size);

#endif // PARROT_CORE_HASH_H_
