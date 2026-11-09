#ifndef PARROT_CORE_INCLUDE_PARROT_CORE_REGEX_H_
#define PARROT_CORE_INCLUDE_PARROT_CORE_REGEX_H_

#include "parrot/core/util.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct ParrotRegex ParrotRegex;

/// NULL = compliation failed
PARROT_API ParrotRegex *ParrotRegex_new(const char *pattern);
PARROT_API void ParrotRegex_delete(ParrotRegex *self);
PARROT_API void ParrotRegex_vdelete(void *self);

PARROT_API bool ParrotRegex_match(ParrotRegex *self, const char *text, /** NULL = unwritten */ size_t *out_size);
/// Checks all positions in the string for a match. Returns <0 if no match.
PARROT_API ptrdiff_t ParrotRegex_match_all(ParrotRegex *self,
                                           const char *text,
                                           /** NULL = unwritten */ size_t *out_size);

#endif // PARROT_CORE_INCLUDE_PARROT_CORE_REGEX_H_
