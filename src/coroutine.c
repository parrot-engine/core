#include "parrot/core/coroutine.h"
#include "parrot/core/os.h"
#include "parrot/core/scope.h"
#include "parrot/core/util.h"
#include <stdio.h>

#ifdef __unix__
#include <sys/mman.h>
#include <unistd.h>
#endif

#define STACK_SIZE (1024 * 1024)

typedef struct {
    ParrotScope *scope;

    size_t user_size;

    bool yield;
    ParrotOSThreadState *state;
    ParrotOSThreadState *return_state;
} ParrotCoroutineMeta;

#ifdef __unix__
typedef struct {
    void *ptr;
    size_t size;
} MunMapCtx;

static void munmap_wrapper(void *ctxx) {
    MunMapCtx *ctx = ctxx;
    munmap(ctx->ptr, ctx->size);
}
#endif

void *Parrot_coroutine_createx(size_t size, void (*func)(void *coroutine)) {
    PARROT_FAIL_NULL(func);

    ParrotCoroutineMeta *meta = malloc(PARROT_META_SIZE(ParrotCoroutineMeta, size));
    memset(meta, 0, PARROT_META_SIZE(ParrotCoroutineMeta, size));

    meta->scope = ParrotScope_new(NULL);
    ParrotScope_push_free(meta->scope, meta);

    meta->user_size = size;

    void *stack = NULL;

#ifdef __unix__
    {
        size_t page_size = (size_t)sysconf(_SC_PAGESIZE);

        MunMapCtx *ctx = PARROT_ALLOC(MunMapCtx);
        ctx->size = STACK_SIZE + page_size;
        ctx->ptr = mmap(NULL, ctx->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        PARROT_FAIL_COND(ctx->ptr == MAP_FAILED);

        stack = ctx->ptr;
        ParrotScope_push_with_free(meta->scope, munmap_wrapper, free, ctx);

        PARROT_FAIL_COND(mprotect(stack, page_size, PROT_NONE) < 0);
        stack = (uint8_t *)stack + page_size;
    }
#else
    stack = malloc(STACK_SIZE);
    ParrotScope_push_free(meta->scope, stack);
#endif

    meta->state = ParrotOSThreadState_new(func, PARROT_META_USER(ParrotCoroutineMeta, meta), stack, STACK_SIZE);
    ParrotScope_push(meta->scope, ParrotOSThreadState_vdelete, meta->state);

    return PARROT_META_USER(ParrotCoroutineMeta, meta);
}

void Parrot_coroutine_delete(void *coroutine) {
    PARROT_FAIL_NULL(coroutine);

    ParrotScope_delete(PARROT_META(ParrotCoroutineMeta, coroutine)->scope);
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
