#ifndef PARROT_CORE_SERIALIZE_H_
#define PARROT_CORE_SERIALIZE_H_

#include <stddef.h>

#include "parrot/core/binary.h"
#include "parrot/core/util.h"

PARROT_API void
Parrot_serialize_bytes(ParrotBuffer *output, ParrotReflect *reflect, size_t type, const void *data, bool with_ptrs);
PARROT_API void *
Parrot_deserialize_bytes_malloc(ParrotBuffer *input, ParrotReflect *reflect, size_t *out_type, bool with_ptrs);

#endif // PARROT_CORE_SERIALIZE_H_
