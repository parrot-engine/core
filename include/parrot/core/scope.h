#ifndef PARROT_CORE_SCOPE_H_
#define PARROT_CORE_SCOPE_H_

#include <stddef.h>
#include <stdint.h>

#include "parrot/core/util.h"

typedef struct ParrotScope ParrotScope;

PARROT_API ParrotScope *ParrotScope_new(/* NULL = no parent */ ParrotScope *parent);
PARROT_API void ParrotScope_delete(ParrotScope *self);
PARROT_API void ParrotScope_vdelete(void *self);

PARROT_API void ParrotScope_set_ctx(ParrotScope *self, void *ctx);
#define ParrotScope_alloc_ctx(self, type) ((type *)ParrotScope_alloc_ctx_raw(self, sizeof(type)))
PARROT_API void *ParrotScope_alloc_ctx_raw(ParrotScope *self, size_t size);
#define ParrotScope_get_ctx(self, type) ((type)ParrotScope_get_ctx_raw(self))
PARROT_API void *ParrotScope_get_ctx_raw(ParrotScope *self);

#define ParrotScope_alloc_ptr_arrfree(p_self, p_arr)                                                                    \
    do {                                                                                                                \
        p_arr = malloc(sizeof(void *));                                                                                 \
        ParrotScope_push_free(p_self, p_arr);                                                                           \
        *(p_arr) = NULL;                                                                                                \
                                                                                                                        \
        ParrotScope_push_arrfree(p_self, *(p_arr));                                                                     \
    } while (0)

PARROT_API void ParrotScope_set_parent(ParrotScope *self, /* NULL = no parent */ ParrotScope *parent);

PARROT_API uint32_t ParrotScope_push(ParrotScope *self, void (*func)(void *ctx), void *ctx);
PARROT_API uint32_t ParrotScope_push_with_free(ParrotScope *self,
                                               void (*func)(void *ctx),
                                               /* NULL = uncalled */ void (*free_func)(void *ctx),
                                               void *ctx);

/// Pushes a free(ptr) function
PARROT_API uint32_t ParrotScope_push_free(ParrotScope *self, void *ptr);

#define ParrotScope_push_arrfree(p_self, p_arr) ParrotScope_push_arrfree_raw(p_self, (void **)&(p_arr))
uint32_t ParrotScope_push_arrfree_raw(ParrotScope *self, void **arr);

/// Does not call pushed function. Does nothing on non-existant id
PARROT_API void ParrotScope_cancel(ParrotScope *self, uint32_t id);

#endif // PARROT_CORE_SCOPE_H_
