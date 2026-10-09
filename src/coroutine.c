#include "parrot/core/coroutine.h"
#include "parrot/core/os.h"
#include "parrot/core/scope.h"
#include "parrot/core/util.h"
#include <stdio.h>
#include <sys/ucontext.h>

#define STACK_SIZE (8192)

typedef struct {
    ParrotScope *scope;

    size_t user_size;

    bool yield;
    ParrotOSThreadState *state;
    ParrotOSThreadState *return_state;
} ParrotCoroutineMeta;

void *Parrot_coroutine_createx(size_t size, void (*func)(void *coroutine)) {
    PARROT_FAIL_NULL(func);

    ParrotCoroutineMeta *meta = malloc(PARROT_META_SIZE(ParrotCoroutineMeta, size));
    memset(meta, 0, PARROT_META_SIZE(ParrotCoroutineMeta, size));

    meta->scope = ParrotScope_new(NULL);
    ParrotScope_push_free(meta->scope, meta);

    meta->user_size = size;

    void *stack = malloc(STACK_SIZE);
    ParrotScope_push_free(meta->scope, stack);

    meta->state = ParrotOSThreadState_new(func, PARROT_META_USER(ParrotCoroutineMeta, meta), stack, STACK_SIZE);
    ParrotScope_push(meta->scope, ParrotOSThreadState_vdelete, meta->state);

    return PARROT_META_USER(ParrotCoroutineMeta, meta);
}

void Parrot_coroutine_delete(void *coroutine) {
    PARROT_FAIL_NULL(coroutine);

    ParrotCoroutineMeta *meta = PARROT_META(ParrotCoroutineMeta, coroutine);

    ParrotScope_delete(meta->scope);
}

bool Parrot_coroutine_resume(void *coroutine) {
    PARROT_FAIL_NULL(coroutine);

    ParrotCoroutineMeta *meta = PARROT_META(ParrotCoroutineMeta, coroutine);

    PARROT_RET_COND_V(!meta->state, false);

    meta->yield = false;
    ParrotOSThreadState_restore(meta->state, &meta->return_state);

    ParrotOSThreadState_delete(meta->return_state);
    meta->return_state = NULL;

    if (!meta->yield) {
        meta->state = NULL;
        return false;
    }

    return true;
}

void Parrot_coroutine_yield(void *coroutine) {
    PARROT_FAIL_NULL(coroutine);

    ParrotCoroutineMeta *meta = PARROT_META(ParrotCoroutineMeta, coroutine);

    meta->yield = true;
    ParrotOSThreadState_restore(meta->return_state, &meta->state);
}
