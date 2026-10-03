#define STB_DS_IMPLEMENTATION
#include "parrot/core/stb_ds.h"

#include "parrot/core/math.h"
#include "parrot/core/os.h"

#include <stdio.h>

#define ITERATIONS (1000000)

ParrotMat matrices[ITERATIONS] = {0};

int main(void) {
    uint64_t begin = Parrot_os_get_performance_counter();

    ParrotTransform transform = ParrotTransform_new();

    transform.position = (ParrotVec3){50, 50, 50};
    transform.rotation.z = 24;

    for (size_t i = 0; i < ITERATIONS; i++) {
        matrices[i] = ParrotTransform_calculate_matrix(&transform);
    }

    float time = (float)(Parrot_os_get_performance_counter() - begin) / Parrot_os_get_performance_frequency();

    for (size_t i = 1; i < ITERATIONS; i++) {
        for (int x = 0; x < 4; x++) {
            for (int y = 0; y < 4; y++) {
                if (matrices[i - 1].data[x][y] != matrices[i].data[x][y]) {
                    return 1;
                }
            }
        }
    }

    for (int y = 0; y < 4; y++) {
        printf("%.02f %.02f %.02f %.02f\n",
               matrices[0].data[0][y],
               matrices[0].data[1][y],
               matrices[0].data[2][y],
               matrices[0].data[3][y]);
    }

    printf("Took: %fs (%d iterations)\n", time, ITERATIONS);

    return 0;
}
