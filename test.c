#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define PARROT_CORE_IMPL
#define PARROT_CORE_IMPL_CORE
#include "core.h"

#include <stdio.h>

int main(void) {
    printf("%.02f\n", Parrot_lerp(0, 100, 0.5));
    return 0;
}
