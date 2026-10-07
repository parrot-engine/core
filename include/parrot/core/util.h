#ifndef PARROT_CORE_UTIL_H_
#define PARROT_CORE_UTIL_H_

#include <stdint.h> // IWYU pragma: keep
#include <stdlib.h> // IWYU pragma: keep
#include <string.h> // IWYU pragma: keep

/// Handles crashing the program and sometimes collecting extra info
typedef void (*ParrotCrashHandlerFunc)(const char *fmt, ...);

extern ParrotCrashHandlerFunc Parrot_crash_handler;

#ifdef __cplusplus
#define PARROT_CPP(...) __VA_ARGS__
#define PARROT_C_CPP(c, cpp) cpp
#define PARROT_C(...)
#else
#define PARROT_CPP(...)
#define PARROT_C_CPP(c, cpp) c
#define PARROT_C(...) __VA_ARGS__
#endif

#define PARROT_DEPEND(...) __asm__ volatile("" ::__VA_ARGS__)

#define PARROT_STRING(x) #x
#define PARROT_TYPE_STRING(type) ((void)sizeof(*(type *)NULL), #type)

#define PARROT_API PARROT_CPP(extern "C")

#define PARROT_ALLOC(type_) (type_ *)memset(malloc(sizeof(type_)), 0, sizeof(type_))

#define PARROT_ALIGN_UP(n, align) (((n) + (align) - 1) & ~((align) - 1))
#define PARROT_ALIGN_DOWN(n, align) ((n) & ~((align) - 1))

#define PARROT_ARRAY_LEN(arr) (sizeof(arr) / sizeof(*(arr)))

#define PARROT_META_SIZE(type, user_size) (PARROT_ALIGN_UP(sizeof(type), 16) + (user_size))
#define PARROT_META(type, data) ((type *)((uint8_t *)(data) - PARROT_META_SIZE(type, 0)))
#define PARROT_META_USER(type, meta) ((void *)((uint8_t *)(meta) + PARROT_META_SIZE(type, 0)))

#define PARROT_FAIL_FMT(fmt, ...)                                                                                       \
    Parrot_crash_handler("(%s:%d in \"%s\") ERROR: " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#define PARROT_FAIL_MSG(msg) PARROT_FAIL_FMT("%s", msg)

#define PARROT_FAIL_COND_FMT(cond, fmt, ...)                                                                            \
    do {                                                                                                                \
        if (cond) {                                                                                                     \
            PARROT_FAIL_FMT(fmt, ##__VA_ARGS__);                                                                        \
        }                                                                                                               \
    } while (0)
#define PARROT_FAIL_COND_MSG(cond, msg) PARROT_FAIL_COND_FMT(cond, "%s", msg)
#define PARROT_FAIL_COND(cond) PARROT_FAIL_COND_FMT(cond, "Condition \"%s\" succeeded", #cond)

#define PARROT_FAIL_NULL_FMT(value, fmt, ...) PARROT_FAIL_COND_FMT((value) == NULL, fmt, ##__VA_ARGS__)
#define PARROT_FAIL_NULL_MSG(value, msg) PARROT_FAIL_NULL_FMT(value, "%s", msg)
#define PARROT_FAIL_NULL(value) PARROT_FAIL_COND_FMT((value) == NULL, "\"%s\" is NULL", #value)

#define PARROT_RET_COND_V(cond, value)                                                                                  \
    do {                                                                                                                \
        if (cond) {                                                                                                     \
            return value;                                                                                               \
        }                                                                                                               \
    } while (0)

#define PARROT_RET_COND(cond) PARROT_RET_COND_V(cond, )

#define PARROT_UNREACHABLE() PARROT_FAIL_MSG("Unreachable")

#endif // PARROT_CORE_UTIL_H_
