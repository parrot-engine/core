#include "parrot/core/string_builder.h"
#include "parrot/core/array.h"
#include "parrot/core/scope.h"
#include <stdio.h>

struct ParrotStringBuilder {
    ParrotScope *scope;

    char *arr_str;
};

ParrotStringBuilder *ParrotStringBuilder_new(void) {
    ParrotStringBuilder *self = PARROT_ALLOC(ParrotStringBuilder);

    self->scope = ParrotScope_new(NULL);
    ParrotScope_push_free(self->scope, self);

    ParrotScope_push_arrfree(self->scope, self->arr_str);

    return self;
}

void ParrotStringBuilder_delete(ParrotStringBuilder *self) {
    PARROT_FAIL_NULL(self);

    ParrotScope_delete(self->scope);
}

void ParrotStringBuilder_vdelete(void *self) {
    ParrotStringBuilder_delete(self);
}

char *ParrotStringBuilder_build_malloc(ParrotStringBuilder *self) {
    PARROT_FAIL_NULL(self);

    char *ret = calloc(ParrotArray_size(self->arr_str) + 1, sizeof(char));
    memcpy(ret, self->arr_str, sizeof(char) * ParrotArray_size(self->arr_str));

    return ret;
}

void ParrotStringBuilder_append(ParrotStringBuilder *self, const char *other) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(other);

    size_t size = strlen(other);

    ParrotArray_alloc(self->arr_str, size);
    memcpy(&self->arr_str[ParrotArray_size(self->arr_str) - size], other, sizeof(char) * size);
}

void ParrotStringBuilder_printf(ParrotStringBuilder *self, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    ParrotStringBuilder_vprintf(self, fmt, ap);
    va_end(ap);
}

void ParrotStringBuilder_vprintf(ParrotStringBuilder *self, const char *fmt, va_list ap) {
    PARROT_FAIL_NULL(self);

    char test;
    va_list test_ap;
    va_copy(test_ap, ap);
    int size = vsnprintf(&test, 1, fmt, test_ap);

    ParrotArray_alloc(self->arr_str, size);
    vsprintf(&self->arr_str[ParrotArray_size(self->arr_str) - size], fmt, ap);
}
