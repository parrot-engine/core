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
    double average_duration;

    uint64_t last_run;
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

    ParrotScheduler_end_task_manual(task,
                                    Parrot_os_get_performance_counter() -
                                        PARROT_META(ParrotSchedulerTaskMeta, task)->running_counter,
                                    Parrot_os_get_performance_frequency());
}

void ParrotScheduler_end_task_manual(void *task, uint64_t duration, uint64_t frequency) {
    PARROT_FAIL_NULL(task);

    if (frequency == 0) {
        duration = 0;
        frequency = 1;
    }

    ParrotSchedulerTaskMeta *meta = PARROT_META(ParrotSchedulerTaskMeta, task);

    PARROT_RET_COND(meta->running_counter == UINT64_MAX);

    const float NEW_WEIGHT = 0.25;
    meta->average_duration = (meta->average_duration * (1.0 - NEW_WEIGHT)) + ((double)duration / frequency * NEW_WEIGHT);

    meta->running_counter = UINT64_MAX;
}

void *ParrotScheduler_nextx(ParrotScheduler *self) {
    PARROT_FAIL_NULL(self);

    for (size_t i = 0; i < ParrotArray_size(self->arr_tasks); i++) {
        if (self->arr_tasks[i]->running_counter != UINT64_MAX) {
            continue;
        }

        if (self->arr_tasks[i]->priority < 0) {
            continue;
        }

        self->arr_tasks[i]->last_run++;
    }

    ParrotSchedulerTaskMeta *best = NULL;
    float best_score = -INFINITY;

    const float PRIORITY_WEIGHT = 750.0;
    const float AGING_WEIGHT = 250.0;
    const float DURATION_PENALTY = 10000.0 * 500.0;

    for (size_t i = 0; i < ParrotArray_size(self->arr_tasks); i++) {
        ParrotSchedulerTaskMeta *task = self->arr_tasks[i];

        if (task->running_counter != UINT64_MAX) {
            continue;
        }

        if (task->priority < 0) {
            continue;
        }

        float score = (task->priority * PRIORITY_WEIGHT) + (task->last_run * AGING_WEIGHT) -
                      (task->average_duration * DURATION_PENALTY);

        if (score > best_score) {
            best_score = score;
            best = task;
        }
    }

    if (best) {
        best->last_run = 0;
        return PARROT_META_USER(ParrotSchedulerTaskMeta, best);
    }

    return NULL;
}
