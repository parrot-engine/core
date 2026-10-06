#ifndef PARROT_CORE_OS_H_
#define PARROT_CORE_OS_H_

#include <stdint.h>

#include "parrot/core/util.h"

PARROT_API uint64_t Parrot_os_get_performance_counter(void);
PARROT_API uint64_t Parrot_os_get_performance_frequency(void);

#define Parrot_os_yield() Parrot_os_sleep(0)
PARROT_API void Parrot_os_sleep(/* <=0 = treated as yield */ double seconds);
PARROT_API void Parrot_os_sleep_precise(/* <=0 = treated as yield */ double seconds);

#endif // PARROT_CORE_OS_H_
