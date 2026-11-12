#ifndef PARROT_CORE_INCLUDE_PARROT_CORE_STRING_BUILDER_H_
#define PARROT_CORE_INCLUDE_PARROT_CORE_STRING_BUILDER_H_

#include "parrot/core/util.h"

typedef struct ParrotStringBuilder ParrotStringBuilder;

PARROT_API ParrotStringBuilder *ParrotStringBuilder_new(void);
PARROT_API void ParrotStringBuilder_delete(ParrotStringBuilder *self);
PARROT_API void ParrotStringBuilder_vdelete(void *self);

PARROT_API char *ParrotStringBuilder_build_malloc(ParrotStringBuilder *self);

PARROT_API void ParrotStringBuilder_append(ParrotStringBuilder *self, const char *other);
PARROT_API void ParrotStringBuilder_printf(ParrotStringBuilder *self, const char *fmt, ...);
PARROT_API void ParrotStringBuilder_vprintf(ParrotStringBuilder *self, const char *fmt, va_list ap);

#endif // PARROT_CORE_INCLUDE_PARROT_CORE_STRING_BUILDER_H_
