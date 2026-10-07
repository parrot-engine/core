#include "parrot/core/scheduler.h"
#include "parrot/core/array.h"
#include "parrot/core/os.h"
#include "parrot/core/scope.h"
#include "parrot/core/util.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    ParrotScope *scope;
    ParrotScheduler *self;

    int priority;

    uint64_t running_counter;

    double total_duration;
} ParrotSchedulerTaskMeta;

struct ParrotScheduler {
    ParrotScope *scope;

    size_t task_size;
    ParrotSchedulerTaskMeta **arr_tasks;
};

ParrotScheduler *ParrotScheduler_newx(size_t task_size) {
    ParrotScheduler *self = PARROT_ALLOC(ParrotScheduler);

    self->scope = ParrotScope_new(NULL);
    ParrotScope_push_free(self->scope, self);

    self->task_size = task_size;
    ParrotScope_push_arrfree(self->scope, self->arr_tasks);

    return self;
}

void ParrotScheduler_delete(ParrotScheduler *self) {
    PARROT_FAIL_NULL(self);

    ParrotScope_delete(self->scope);
}

void ParrotScheduler_vdelete(void *self) {
    ParrotScheduler_delete(self);
}

void *ParrotScheduler_create_taskx(ParrotScheduler *self) {
    PARROT_FAIL_NULL(self);

    ParrotSchedulerTaskMeta *meta = malloc(PARROT_META_SIZE(ParrotSchedulerTaskMeta, self->task_size));
    memset(meta, 0, PARROT_META_SIZE(ParrotSchedulerTaskMeta, self->task_size));

    meta->scope = ParrotScope_new(self->scope);
    ParrotScope_push_free(meta->scope, meta);

    meta->self = self;

    meta->running_counter = UINT64_MAX;

    ParrotArray_put_key(self->arr_tasks, meta, &meta, sizeof(ParrotSchedulerTaskMeta *));
    return PARROT_META_USER(ParrotSchedulerTaskMeta, meta);
}

void ParrotScheduler_delete_task(void *task) {
    PARROT_FAIL_NULL(task);

    ParrotSchedulerTaskMeta *meta = PARROT_META(ParrotSchedulerTaskMeta, task);

    ParrotArray_delkx(meta->self->arr_tasks, &meta, sizeof(ParrotSchedulerTaskMeta *));
    ParrotScope_delete(meta->scope);
}

void ParrotScheduler_set_task_priority(void *task, /* <0 = blocked */ int priority) {
    PARROT_FAIL_NULL(task);

    ParrotSchedulerTaskMeta *meta = PARROT_META(ParrotSchedulerTaskMeta, task);

    meta->priority = priority;
}

void ParrotScheduler_begin_task(void *task) {
    PARROT_FAIL_NULL(task);

    ParrotSchedulerTaskMeta *meta = PARROT_META(ParrotSchedulerTaskMeta, task);

    meta->running_counter = Parrot_os_get_performance_counter();
}

void ParrotScheduler_end_task(void *task) {
    PARROT_FAIL_NULL(task);

    uint64_t time = Parrot_os_get_performance_counter() - PARROT_META(ParrotSchedulerTaskMeta, task)->running_counter;
    ParrotScheduler_end_task_manual(task, time, Parrot_os_get_performance_frequency());
}

void ParrotScheduler_end_task_manual(void *task, uint64_t duration, uint64_t frequency) {
    PARROT_FAIL_NULL(task);

    if (frequency == 0) {
        duration = 0;
        frequency = 1;
    }

    ParrotSchedulerTaskMeta *meta = PARROT_META(ParrotSchedulerTaskMeta, task);

    PARROT_RET_COND(meta->running_counter == UINT64_MAX);

    meta->total_duration += (double)duration / frequency;

    meta->running_counter = UINT64_MAX;
}

void *ParrotScheduler_nextx(ParrotScheduler *self) {
    PARROT_FAIL_NULL(self);

    ParrotSchedulerTaskMeta *best = NULL;
    int64_t best_score = INT64_MIN;

    const int64_t MULTIPLIER = 1000;

    const int64_t PRIORITY_WEIGHT = 7500;
    const int64_t DURATION_PENALTY = 10;

    for (size_t i = 0; i < ParrotArray_size(self->arr_tasks); i++) {
        ParrotSchedulerTaskMeta *task = self->arr_tasks[i];

        if (task->running_counter != UINT64_MAX) {
            continue;
        }

        if (task->priority < 0) {
            continue;
        }

        int64_t score = 0;
        score += task->priority * PRIORITY_WEIGHT * MULTIPLIER;
        score -= task->total_duration * DURATION_PENALTY * MULTIPLIER;

        if (score > best_score) {
            best_score = score;
            best = task;
        }
    }

    if (best) {
        return PARROT_META_USER(ParrotSchedulerTaskMeta, best);
    }

    return NULL;
}
