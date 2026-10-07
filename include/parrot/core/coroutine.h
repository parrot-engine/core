#ifndef PARROT_CORE_INCLUDE_PARROT_CORE_COROUTINE_H_
#define PARROT_CORE_INCLUDE_PARROT_CORE_COROUTINE_H_

#include "parrot/core/util.h"
#include <stdbool.h>

#define Parrot_coroutine_create(type, func) ((type *)Parrot_coroutine_createx(sizeof(type), func))
PARROT_API void *Parrot_coroutine_createx(size_t size, void (*func)(void *coroutine));
PARROT_API void Parrot_coroutine_delete(void *coroutine);
/// @returns If coroutine can continue running
PARROT_API bool Parrot_coroutine_resume(void *coroutine);
PARROT_API void Parrot_coroutine_yield(void *coroutine);

#endif // PARROT_CORE_INCLUDE_PARROT_CORE_COROUTINE_H_
