#include "parrot/core/array.h"
#include <stdlib.h>
#include <time.h>

#include "parrot/core/math.h"
#include "parrot/core/os.h"
#include "parrot/core/scheduler.h"

#include <stdio.h>

#define ITERATIONS (4194304)

typedef struct {
    int id;
    int priority;

    double begin;
    double end;

    int chosen_times;
    double total_time;
} Task;

int main(void) {
    srand(time(NULL));

    {
        printf("Test Array... ");
        fflush(stdout);

        unsigned int seed = rand();
        srand(seed);

        unsigned int *array = NULL;
        for (size_t i = 0; i < ITERATIONS; i++) {
            ParrotArray_push(array, rand());
        }

        srand(seed);

        for (size_t i = 0; i < ITERATIONS; i++) {
            if (array[i] != rand()) {
                printf("FAIL\n");
                return 1;
            }
        }

        printf("PASS\n");
        ParrotArray_free(array);
    }

    {
        printf("Test Scheduler... ");
        fflush(stdout);

        const int tasks = 10;

        ParrotScheduler *scheduler = ParrotScheduler_new(Task);
        Task **arr_tasks = NULL;

        for (size_t i = 0; i < tasks; i++) {
            Task *task = ParrotScheduler_create_task(scheduler, Task);

            task->id = i;

            float random = (float)rand() / RAND_MAX;
            if (random > 0.75) {
                task->priority = 2;
            } else if (random > 0.5) {
                task->priority = 1;
            }

            ParrotScheduler_set_task_priority(task, task->priority);
            ParrotArray_push(arr_tasks, task);
        }

        const int core_count = 2;

        double time = 0;
        const double resolution = 0.00025;

        Task *core_tasks[core_count];
        for (int i = 0; i < core_count; i++) {
            core_tasks[i] = NULL;
        }

        while (time <= 5 * 60) {
            for (int i = 0; i < core_count; i++) {
                Task *old_task = core_tasks[i];
                if (old_task) {
                    old_task->total_time += resolution;
                    if (old_task->end > time) {
                        continue;
                    }

                    ParrotScheduler_end_task_manual(
                        old_task, (old_task->end - old_task->begin) / resolution, 1.0 / resolution);
                }

                Task *task = ParrotScheduler_next(scheduler, Task);
                if (!task) {
                    goto end;
                }

                task->begin = time;
                task->end = time + fmod(tasks - task->id * resolution, 0.001);
                task->chosen_times++;

                core_tasks[i] = task;
                ParrotScheduler_begin_task(task);
            }

            time += resolution;
        }

        printf("\n");
        for (size_t i = 0; i < tasks; i++) {
            printf("  Task %u: Priority %d, Chosen %d time(s), Total CPU time: %fs, Average CPU time: %fs\n",
                   (unsigned int)i,
                   arr_tasks[i]->priority,
                   arr_tasks[i]->chosen_times,
                   arr_tasks[i]->total_time,
                   arr_tasks[i]->chosen_times != 0 ? arr_tasks[i]->total_time / arr_tasks[i]->chosen_times : 0);
        }

    end:
        ParrotArray_free(arr_tasks);
        ParrotScheduler_delete(scheduler);
    }

    {
        printf("Calculate matrix... ");
        fflush(stdout);

        ParrotMat *matrices = calloc(ITERATIONS, sizeof(ParrotMat));
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
                        printf(" FAIL\n");
                        return 1;
                    }
                }
            }
        }

        printf("%fs\n", time);

        for (int y = 0; y < 4; y++) {
            printf("  %.02f %.02f %.02f %.02f\n",
                   matrices[0].data[0][y],
                   matrices[0].data[1][y],
                   matrices[0].data[2][y],
                   matrices[0].data[3][y]);
        }

        free(matrices);
    }

    return 0;
}
