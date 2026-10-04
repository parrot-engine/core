#include "parrot/core/scope.h"
#include "parrot/core/array.h"

typedef struct {
    uint32_t key;

    void (*func)(void *ctx);
    void (*free_func)(void *ctx);
    void *ctx;
} ParrotScopeEntry;

struct ParrotScope {
    ParrotScopeEntry *hm_stack;
    uint32_t next_id;

    ParrotScope *parent;
    uint32_t parent_delete_id;

    void *ctx;
};

ParrotScope *ParrotScope_new(ParrotScope *parent) {
    ParrotScope *self = PARROT_ALLOC(ParrotScope);

    ParrotScope_set_parent(self, parent);

    return self;
}

void ParrotScope_delete(ParrotScope *self) {
    PARROT_FAIL_NULL(self);

    if (self->parent) {
        ParrotScope_cancel(self->parent, self->parent_delete_id);
    }

    while (ParrotArray_size(self->hm_stack) > 0) {
        size_t index = ParrotArray_size(self->hm_stack) - 1;
        ParrotScopeEntry entry = self->hm_stack[index];
        ParrotArray_del(self->hm_stack, index);

        entry.func(entry.ctx);

        if (entry.free_func) {
            entry.free_func(entry.ctx);
        }
    }

    ParrotArray_free(self->hm_stack);
    free(self);
}

void ParrotScope_vdelete(void *self) {
    ParrotScope_delete((ParrotScope *)self);
}

void ParrotScope_set_ctx(ParrotScope *self, void *ctx) {
    PARROT_FAIL_NULL(self);

    self->ctx = ctx;
}

void *ParrotScope_alloc_ctx_raw(ParrotScope *self, size_t size) {
    PARROT_FAIL_NULL(self);

    self->ctx = malloc(size);
    memset(self->ctx, 0, size);

    ParrotScope_push_free(self, self->ctx);
    return self->ctx;
}

void *ParrotScope_get_ctx_raw(ParrotScope *self) {
    PARROT_FAIL_NULL(self);
    return self->ctx;
}

static void scope_delete_wrapper(void *ctx) {
    ParrotScope_delete(ctx);
}

void ParrotScope_set_parent(ParrotScope *self, ParrotScope *parent) {
    PARROT_FAIL_NULL(self);

    if (self->parent) {
        ParrotScope_cancel(self->parent, self->parent_delete_id);
    }

    self->parent = parent;
    if (parent) {
        self->parent_delete_id = ParrotScope_push(parent, scope_delete_wrapper, self);
    }
}

uint32_t ParrotScope_push(ParrotScope *self, void (*func)(void *ctx), void *ctx) {
    return ParrotScope_push_with_free(self, func, NULL, ctx);
}

uint32_t
ParrotScope_push_with_free(ParrotScope *self, void (*func)(void *ctx), void (*free_func)(void *ctx), void *ctx) {
    PARROT_FAIL_NULL(self);

    ParrotScopeEntry entry = {0};
    entry.key = self->next_id++;
    entry.func = func;
    entry.free_func = free_func;
    entry.ctx = ctx;

    ParrotArray_put(self->hm_stack, entry);
    return entry.key;
}

static void free_wrapper(void *ctx) {
    free(ctx);
}

uint32_t ParrotScope_push_free(ParrotScope *self, void *ptr) {
    return ParrotScope_push(self, free_wrapper, ptr);
}

typedef struct {
    void **data;
} STBDSFreeCtx;

static void arrfree_wrapper(void *ctx_ptr) {
    STBDSFreeCtx *ctx = ctx_ptr;
    if (*ctx->data) {
        ParrotArray_free(*ctx->data);
        *ctx->data = NULL;
    }
}

uint32_t ParrotScope_push_arrfree_raw(ParrotScope *self, void **arr) {
    STBDSFreeCtx *ctx = PARROT_ALLOC(STBDSFreeCtx);
    ctx->data = arr;
    return ParrotScope_push_with_free(self, arrfree_wrapper, free, ctx);
}

void ParrotScope_cancel(ParrotScope *self, uint32_t id) {
    PARROT_FAIL_NULL(self);

    ptrdiff_t index = ParrotArray_find(self->hm_stack, id);
    if (index >= 0) {
        ParrotScopeEntry *entry = &self->hm_stack[index];
        if (entry->free_func) {
            entry->free_func(entry->ctx);
        }

        ParrotArray_del(self->hm_stack, index);
    }
}
