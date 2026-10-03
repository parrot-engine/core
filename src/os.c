#include <stdio.h>

#include "parrot/core/os.h"

#ifdef __unix__
#define __USE_POSIX199309
#include <sched.h>
#include <time.h>
#include <unistd.h>
#endif

#include <math.h>

#if !defined(__unix__)
#error "Unsupported platform"
#endif

uint64_t Parrot_os_get_performance_counter(void) {
#ifdef __unix__
    struct timespec time;
    clock_gettime(/* Would use CLOCK_MONOTONIC_RAW if not linux specific */ CLOCK_MONOTONIC, &time);
    return time.tv_nsec + (time.tv_sec * 1e9L);
#endif
}

uint64_t Parrot_os_get_performance_frequency(void) {
#ifdef __unix__
    struct timespec resolution;
    clock_getres(CLOCK_MONOTONIC, &resolution);
    return resolution.tv_nsec != 0 ?
               1e9L / resolution.tv_nsec :
               /* Probability of this happening? Less than 1/1,000,000,000 but just to be safe */ 1e9;
#endif
}

void Parrot_os_sleep(double seconds) {
#ifdef __unix__
    if (seconds == 0) {
        if (sched_yield() < 0) {
            // My guess on how it would fail other than unimplemented is unknown
            perror("(?\?\?) sched_yield");

            sleep(0);
        }
        return;
    }

    struct timespec time = {
        .tv_sec = seconds,
        .tv_nsec = (long)(fmod(seconds, 1.0) * 1e9),
    };
    nanosleep(&time, NULL);
#endif
}
