#include "parrot/core/coroutine.h"
#include "parrot/core/scope.h"
#include "parrot/core/util.h"
#include <sys/ucontext.h>

#ifdef __unix__
#include <ucontext.h>
#endif

#define STACK_SIZE (8192)

#define CO_CONTINUE_STATUS (1)
#define CO_EXIT_STATUS (2)

typedef struct {
    ParrotScope *scope;

    size_t user_size;

#ifdef __unix__
    bool yield;
    ucontext_t coroutine_ctx;
    ucontext_t return_ctx;
#endif
} ParrotCoroutineMeta;

void *Parrot_coroutine_createx(size_t size, void (*func)(void *coroutine)) {
    PARROT_FAIL_NULL(func);

    ParrotCoroutineMeta *meta = malloc(PARROT_META_SIZE(ParrotCoroutineMeta, size));
    memset(meta, 0, PARROT_META_SIZE(ParrotCoroutineMeta, size));

    meta->scope = ParrotScope_new(NULL);
    ParrotScope_push_free(meta->scope, meta);

    meta->user_size = size;

#ifdef __unix__
    meta->yield = true;

    getcontext(&meta->coroutine_ctx);
    meta->coroutine_ctx.uc_stack.ss_sp = malloc(STACK_SIZE);
    meta->coroutine_ctx.uc_stack.ss_size = STACK_SIZE;
    meta->coroutine_ctx.uc_link = &meta->return_ctx;

    ParrotScope_push_free(meta->scope, meta->coroutine_ctx.uc_stack.ss_sp);

    makecontext(&meta->coroutine_ctx, (void (*)(void))func, 1, PARROT_META_USER(ParrotCoroutineMeta, meta));
#endif

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

#ifdef __unix__
    if (meta->yield) {
        meta->yield = false;
        swapcontext(&meta->return_ctx, &meta->coroutine_ctx);
    }

    return meta->yield;
#endif
}

void Parrot_coroutine_yield(void *coroutine) {
    PARROT_FAIL_NULL(coroutine);

    ParrotCoroutineMeta *meta = PARROT_META(ParrotCoroutineMeta, coroutine);

#ifdef __unix__
    meta->yield = true;
    swapcontext(&meta->coroutine_ctx, &meta->return_ctx);
#endif
}
