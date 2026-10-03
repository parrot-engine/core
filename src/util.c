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

ParrotCrashHandlerFunc Parrot_crash_handler = Parrot_crash_handler_impl;
