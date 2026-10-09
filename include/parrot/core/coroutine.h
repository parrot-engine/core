#ifndef PARROT_CORE_INCLUDE_PARROT_CORE_COROUTINE_H_
#define PARROT_CORE_INCLUDE_PARROT_CORE_COROUTINE_H_

#include "parrot/core/util.h"
#include <stdbool.h>

/// See `Parrot_coroutine_createx`
#define Parrot_coroutine_create(type, func) ((type *)Parrot_coroutine_createx(sizeof(type), func))
/// Does not start the coroutine
PARROT_API void *Parrot_coroutine_createx(size_t size, void (*func)(void *coroutine));
/// Can cancel coroutine but caller is then responsible for cleaning up coroutine's non-stack memory
PARROT_API void Parrot_coroutine_delete(void *coroutine);
/// @returns If coroutine can continue running
PARROT_API bool Parrot_coroutine_resume(void *coroutine);
PARROT_API void Parrot_coroutine_yield(void *coroutine);

#endif // PARROT_CORE_INCLUDE_PARROT_CORE_COROUTINE_H_
