#include <stdbool.h>
#include <stdio.h>

#include "parrot/core/os.h"
#include "parrot/core/util.h"

#include <math.h>

#ifdef __unix__
#define HAS_IMPL

#define __USE_POSIX199309 // Will not give me clock_* otherwise
#include <sched.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

static uint64_t Parrot_os_get_performance_counter_impl(void) {
    struct timespec time;
    clock_gettime(/* Would use CLOCK_MONOTONIC_RAW if not linux specific */ CLOCK_MONOTONIC, &time);
    return time.tv_nsec + (time.tv_sec * 1e9L);
}

static uint64_t Parrot_os_get_performance_frequency_impl(void) {
    struct timespec resolution;
    clock_getres(CLOCK_MONOTONIC, &resolution);
    return resolution.tv_nsec != 0 ?
               1e9L / resolution.tv_nsec :
               /* Probability of this happening? Less than 1/1,000,000,000 but just to be safe */ 1e9;
}

static void Parrot_os_sleep_impl(double seconds) {
    if (seconds < 0) {
        if (sched_yield() < 0) {
            perror("(?) sched_yield");
            sleep(0);
        }
        return;
    }

    struct timespec time = {
        .tv_sec = seconds,
        .tv_nsec = (long)(fmod(seconds, 1.0) * 1e9),
    };
    nanosleep(&time, NULL);
}

struct ParrotOSThreadState {
    ucontext_t context;
    ucontext_t return_context;
};

static ParrotOSThreadState *
ParrotOSThreadState_new_impl(void (*func)(void *ctx), void *ctx, void *stack, size_t stack_size) {
    PARROT_FAIL_NULL(func);
    PARROT_FAIL_NULL(stack);

    ParrotOSThreadState *self = PARROT_ALLOC(ParrotOSThreadState);

    getcontext(&self->context);
    self->context.uc_stack.ss_sp = stack;
    self->context.uc_stack.ss_size = stack_size;
    self->context.uc_link = &self->return_context;

    makecontext(&self->context, (void (*)(void))func, 1, ctx);

    return self;
}

static void ParrotOSThreadState_delete_impl(ParrotOSThreadState *self) {
    PARROT_FAIL_NULL(self);

    free(self);
}

static void ParrotOSThreadState_restore_impl(ParrotOSThreadState *self, ParrotOSThreadState **out_current) {
    PARROT_FAIL_NULL(self);

    if (out_current) {
        if (!*out_current) {
            *out_current = PARROT_ALLOC(ParrotOSThreadState);
        }
        **out_current = (ParrotOSThreadState){0};
    }

    bool called = false;

    getcontext(&self->return_context);

    if (!called) {
        called = true;
        swapcontext(out_current ? &(*out_current)->context : &self->return_context, &self->context);
    }
}

#endif

#ifdef HAS_IMPL
#define IMPL(func) func
#else
#define IMPL(func) NULL
#endif

uint64_t (*Parrot_os_get_performance_counter)(void) = IMPL(Parrot_os_get_performance_counter_impl);

uint64_t (*Parrot_os_get_performance_frequency)(void) = IMPL(Parrot_os_get_performance_frequency_impl);

void (*Parrot_os_sleep)(double seconds) = IMPL(Parrot_os_sleep_impl);

void Parrot_os_sleep_precise(double seconds) {
    uint64_t begin = Parrot_os_get_performance_counter();
    uint64_t frequency = Parrot_os_get_performance_frequency();

    Parrot_os_sleep(seconds - 0.002);

    while (Parrot_os_get_performance_counter() - begin < frequency * seconds) {
        Parrot_os_yield();
    }
}

ParrotOSThreadState *(*ParrotOSThreadState_new)(void (*func)(void *ctx),
                                                void *ctx,
                                                void *stack,
                                                size_t stack_size) = IMPL(ParrotOSThreadState_new_impl);

void (*ParrotOSThreadState_delete)(ParrotOSThreadState *self) = IMPL(ParrotOSThreadState_delete_impl);

void ParrotOSThreadState_vdelete(void *self) {
    ParrotOSThreadState_delete(self);
}

void (*ParrotOSThreadState_restore)(ParrotOSThreadState *self,
                                    ParrotOSThreadState **out_current) = IMPL(ParrotOSThreadState_restore_impl);
