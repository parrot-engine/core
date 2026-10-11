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
    char test = 0;
    va_list ap_copy;
    va_copy(ap_copy, ap);

    char *str = calloc(vsnprintf(&test, 1, fmt, ap_copy) + 1, sizeof(char));
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

ParrotCrashHandlerFunc Parrot_crash_handler = Parrot_crash_handler_impl;
