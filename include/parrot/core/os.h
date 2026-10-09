#ifndef PARROT_CORE_OS_H_
#define PARROT_CORE_OS_H_

#include <stdbool.h>
#include <stdint.h>

#include "parrot/core/util.h"

typedef struct ParrotOSThreadState ParrotOSThreadState;

/// NULL if not supported and if NULL, can be implemented by the user
PARROT_API extern uint64_t (*Parrot_os_get_performance_counter)(void);
/// NULL if not supported and if NULL, can be implemented by the user
PARROT_API extern uint64_t (*Parrot_os_get_performance_frequency)(void);

#define Parrot_os_yield() Parrot_os_sleep(0)
/// NULL if not supported and if NULL, can be implemented by the user
PARROT_API extern void (*Parrot_os_sleep)(/* <=0 = treated as yield */ double seconds);
/// See `Parrot_os_sleep`
PARROT_API void Parrot_os_sleep_precise(double seconds);

PARROT_API extern ParrotOSThreadState *(*ParrotOSThreadState_new)(void (*func)(void *ctx),
                                                                  void *ctx,
                                                                  void *stack,
                                                                  size_t stack_size);
PARROT_API extern void (*ParrotOSThreadState_delete)(ParrotOSThreadState *self);
PARROT_API void ParrotOSThreadState_vdelete(void *self);

PARROT_API extern void (*ParrotOSThreadState_restore)(
    ParrotOSThreadState *self,
    /* NULL = not saved, will not reallocate if pointer behind pointer is not NULL */ ParrotOSThreadState **out_current);

#endif // PARROT_CORE_OS_H_
