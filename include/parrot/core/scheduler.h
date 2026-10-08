#ifndef PARROT_CORE_INCLUDE_PARROT_CORE_SCHEDULER_H_
#define PARROT_CORE_INCLUDE_PARROT_CORE_SCHEDULER_H_

#include "parrot/core/util.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct ParrotScheduler ParrotScheduler;

#define ParrotScheduler_new(type) ParrotScheduler_newx(sizeof(type))
PARROT_API ParrotScheduler *ParrotScheduler_newx(size_t task_size);
PARROT_API void ParrotScheduler_delete(ParrotScheduler *self);
PARROT_API void ParrotScheduler_vdelete(void *self);

#define ParrotScheduler_create_task(self, type) ((type *)ParrotScheduler_create_taskx(self))
PARROT_API void *ParrotScheduler_create_taskx(ParrotScheduler *self);
PARROT_API void ParrotScheduler_delete_task(void *task);

PARROT_API void ParrotScheduler_set_task_priority(void *task, /* <0 = blocked, 0 = lowest priority */ int priority);

PARROT_API void ParrotScheduler_begin_task(void *task);
PARROT_API void ParrotScheduler_end_task(void *task);
PARROT_API void ParrotScheduler_end_task_manual(void *task, uint64_t duration, uint64_t frequency);

/// See ParrotScheduler_nextx
#define ParrotScheduler_next(self, type) ((type *)ParrotScheduler_nextx(self))
/// Returns NULL if no task
PARROT_API void *ParrotScheduler_nextx(ParrotScheduler *self);

#endif // PARROT_CORE_INCLUDE_PARROT_CORE_SCHEDULER_H_
