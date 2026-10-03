#ifndef PARROT_CORE_SERIALIZE_H_
#define PARROT_CORE_SERIALIZE_H_

#include <stddef.h>

#include "parrot/core/binary.h"
#include "parrot/core/util.h"

PARROT_API void
Parrot_serialize_bytes(ParrotBuffer *output, ParrotReflect *reflect, size_t type, const void *data, bool with_ptrs);
/// Return type is allocated with malloc() that the caller takes ownership of or NULL if could not be deserialized
PARROT_API void *Parrot_deserialize_bytes(ParrotBuffer *input, ParrotReflect *reflect, size_t *out_type, bool with_ptrs);

#endif // PARROT_CORE_SERIALIZE_H_
