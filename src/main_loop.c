#include "parrot/core/main_loop.h"
#include "parrot/core/os.h"

struct ParrotMainLoop {
    bool should_close;

    uint64_t frame_start;
};

ParrotMainLoop *ParrotMainLoop_new(void) {
    ParrotMainLoop *self = PARROT_ALLOC(ParrotMainLoop);

    return self;
}

void ParrotMainLoop_delete(ParrotMainLoop *self) {
    PARROT_FAIL_NULL(self);

    free(self);
}

void ParrotMainLoop_vdelete(void *self) {
    ParrotMainLoop_delete((ParrotMainLoop *)self);
}

void ParrotMainLoop_request_close(ParrotMainLoop *self) {
    PARROT_FAIL_NULL(self);

    self->should_close = true;
}

bool ParrotMainLoop_next_frame(ParrotMainLoop *self, float *delta, float max_fps) {
    PARROT_FAIL_NULL(self);

    if (max_fps > 0) {
        float target_delta = 1.0 / max_fps;
        uint64_t frequency = Parrot_os_get_performance_frequency();

        float elapsed = (Parrot_os_get_performance_counter() - self->frame_start) / (float)frequency;
        float remaining = target_delta - elapsed;
        if (remaining > 0.002) {
            Parrot_os_sleep(remaining - 0.002);
        }

        while ((Parrot_os_get_performance_counter() - self->frame_start) / (float)frequency < target_delta) {
            Parrot_os_yield();
        }
    }

    uint64_t frame_end = Parrot_os_get_performance_counter();
    if (delta) {
        *delta = (float)(frame_end - self->frame_start) / Parrot_os_get_performance_frequency();
    }
    self->frame_start = frame_end;

    bool should_close = self->should_close;
    self->should_close = false;
    return !should_close;
}

void ParrotMainLoop_run(ParrotMainLoop *self,
                        void *user_data,
                        ParrotMainLoopInitFunc init,
                        ParrotMainLoopUpdateFunc update,
                        ParrotMainLoopRenderFunc render,
                        ParrotMainLoopShutdownFunc shutdown) {
    PARROT_FAIL_NULL(self);

    PARROT_FAIL_NULL(update);

    ParrotMainLoopRunSettings settings = {0};
    settings.user_data = user_data;

    float delta = 1.0 / 60;

    if (init) {
        init(&settings);
    }

    ParrotMainLoop_next_frame(self, &delta, settings.max_fps);
    for (;;) {
        if (update(&settings, delta, !ParrotMainLoop_next_frame(self, &delta, settings.max_fps))) {
            break;
        }

        if (render) {
            render(&settings);
        }
    }

    if (shutdown) {
        shutdown(&settings);
    }
}
