#include "parrot/core/array.h"
#include <stdlib.h>
#include <time.h>

#include "parrot/core/coroutine.h"
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

typedef struct {
    bool done;
} TestSharedData;

typedef struct {
    TestSharedData *coroutine;
} TestTask;

static void co_test_array(void *cox) {
    TestSharedData *co = cox;

    unsigned int *array = NULL;
    for (size_t i = 0; i < ITERATIONS; i++) {
        if (i % 32 == 0) {
            Parrot_coroutine_yield(co);
        }
        ParrotArray_push(array, rand());
    }

    ParrotArray_free(array);

    co->done = true;
    Parrot_coroutine_yield(co);
    printf("Test Array... FINISH\n");
}

static void co_test_scheduler(void *cox) {
    TestSharedData *co = cox;

    const int tasks = 10;

    ParrotScheduler *scheduler = ParrotScheduler_new(Task);
    Task **arr_tasks = NULL;

    for (size_t i = 0; i < tasks; i++) {
        if (i % 32 == 0) {
            Parrot_coroutine_yield(co);
        }

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

    const int core_count = 1;

    double time = 0;
    const double resolution = 0.001;

    Task **core_tasks = calloc(sizeof(Task *), core_count);
    for (int i = 0; i < core_count; i++) {
        core_tasks[i] = NULL;
    }

    while (time <= 6 * 60 * 60) {
        if (fmod(time, 1) == 0) {
            Parrot_coroutine_yield(co);
        }

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
            task->end = time + (double)rand() / RAND_MAX * 0.1;
            task->chosen_times++;

            core_tasks[i] = task;
            ParrotScheduler_begin_task(task);
        }

        time += resolution;
    }

    free(core_tasks);

end:
    co->done = true;
    Parrot_coroutine_yield(co);

    printf("Test Scheduler...\n");
    for (size_t i = 0; i < tasks; i++) {
        printf("  Task %u: P%d, Chosen %d time(s), Total CPU time: %fs, Average CPU time: %fs\n",
               (unsigned int)i,
               arr_tasks[i]->priority,
               arr_tasks[i]->chosen_times,
               arr_tasks[i]->total_time,
               arr_tasks[i]->chosen_times != 0 ? arr_tasks[i]->total_time / arr_tasks[i]->chosen_times : 0);
    }

    ParrotArray_free(arr_tasks);
    ParrotScheduler_delete(scheduler);
}

static void co_test_matrix(void *cox) {
    TestSharedData *co = cox;

    ParrotMat *matrices = calloc(ITERATIONS, sizeof(ParrotMat));
    uint64_t begin = Parrot_os_get_performance_counter();

    ParrotTransform transform = ParrotTransform_new();

    transform.position = (ParrotVec3){50, 50, 50};
    transform.rotation.z = 24;

    for (size_t i = 0; i < ITERATIONS; i++) {
        matrices[i] = ParrotTransform_calculate_matrix(&transform);
        Parrot_coroutine_yield(co);
    }

    float time = (float)(Parrot_os_get_performance_counter() - begin) / Parrot_os_get_performance_frequency();

    for (size_t i = 1; i < ITERATIONS; i++) {
        for (int x = 0; x < 4; x++) {
            for (int y = 0; y < 4; y++) {
                if (matrices[i - 1].data[x][y] != matrices[i].data[x][y]) {
                    co->done = true;
                    Parrot_coroutine_yield(co);

                    printf("Calculate matrix... FAIL\n");
                }
            }
        }

        Parrot_coroutine_yield(co);
    }

    co->done = true;
    Parrot_coroutine_yield(co);

    printf("Calculate matrix... %fs\n", time);

    for (int y = 0; y < 4; y++) {
        printf("  %.02f %.02f %.02f %.02f\n",
               matrices[0].data[0][y],
               matrices[0].data[1][y],
               matrices[0].data[2][y],
               matrices[0].data[3][y]);
    }

    free(matrices);
}

int main(void) {
    srand(time(NULL));

    // Overenginnering = More tests

    ParrotScheduler *scheduler = ParrotScheduler_new(TestTask);

    ParrotScheduler_create_task(scheduler, TestTask)->coroutine =
        Parrot_coroutine_create(TestSharedData, co_test_matrix);
    ParrotScheduler_create_task(scheduler, TestTask)->coroutine = Parrot_coroutine_create(TestSharedData, co_test_array);
    ParrotScheduler_create_task(scheduler, TestTask)->coroutine =
        Parrot_coroutine_create(TestSharedData, co_test_scheduler);

    for (;;) {
        TestTask *task = ParrotScheduler_next(scheduler, TestTask);
        if (!task) {
            break;
        }

        ParrotScheduler_begin_task(task);
        Parrot_coroutine_resume(task->coroutine);
        ParrotScheduler_end_task(task);

        if (task->coroutine->done) {
            while (Parrot_coroutine_resume(task->coroutine))
                ;
            Parrot_coroutine_delete(task->coroutine);
            ParrotScheduler_delete_task(task);
        }
    }

    ParrotScheduler_delete(scheduler);
    return 0;
}
