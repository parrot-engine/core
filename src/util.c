#include <stdarg.h>
#include <stdio.h>

#include "parrot/core/util.h"

static void Parrot_crash_handler_impl(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    abort();
}

char *Parrot_vsprintf_malloc(const char *fmt, va_list ap) {
    PARROT_FAIL_NULL(fmt);

    if (!fmt) {
        return NULL;
    }

    char test = 0;
    va_list test_ap;
    va_copy(test_ap, ap);

    char *str = calloc(vsnprintf(&test, 1, fmt, test_ap) + 1, sizeof(char));
    PARROT_RET_COND_V(!str, NULL);
    vsprintf(str, fmt, ap);

    return str;
}

char *Parrot_sprintf_malloc(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *ret = Parrot_vsprintf_malloc(fmt, ap);
    va_end(ap);
    return ret;
}

char *Parrot_strdup_malloc(const char *str) {
    PARROT_FAIL_NULL(str);

    char *dup = calloc(strlen(str) + 1, sizeof(char));
    PARROT_RET_COND_V(!dup, NULL);

    strcpy(dup, str);
    return dup;
}

ParrotCrashHandlerFunc Parrot_crash_handler = Parrot_crash_handler_impl;
