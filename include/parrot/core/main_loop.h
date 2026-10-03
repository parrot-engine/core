#ifndef PARROT_CORE_MAIN_LOOP_H_
#define PARROT_CORE_MAIN_LOOP_H_

#include <stdbool.h>

#include "parrot/core/reflect.h"
#include "parrot/core/util.h"

typedef struct {
    void *user_data;

    // <=0 = Uncapped
    float max_fps;
} ParrotMainLoopRunSettings;

static const ParrotReflectDescription ParrotMainLoopRunSettings_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotMainLoopRunSettings),

    PARROT_REFLECT_TYPE_FIELD(ParrotMainLoopRunSettings, void *, user_data, ),

    PARROT_REFLECT_TYPE_FIELD(ParrotMainLoopRunSettings, float, max_fps, ),

    PARROT_REFLECT_END(),
};

typedef void (*ParrotMainLoopInitFunc)(ParrotMainLoopRunSettings *settings);
/// @return If the application should close
typedef bool (*ParrotMainLoopUpdateFunc)(ParrotMainLoopRunSettings *settings,
                                         float delta,
                                         /* Clears on next frame */ bool should_close);
typedef void (*ParrotMainLoopRenderFunc)(ParrotMainLoopRunSettings *settings);
typedef void (*ParrotMainLoopShutdownFunc)(ParrotMainLoopRunSettings *settings);

typedef struct ParrotMainLoop ParrotMainLoop;

PARROT_API ParrotMainLoop *ParrotMainLoop_new(void);
PARROT_API void ParrotMainLoop_delete(ParrotMainLoop *self);
PARROT_API void ParrotMainLoop_vdelete(void *self);

/// Clears on next frame
PARROT_API void ParrotMainLoop_request_close(ParrotMainLoop *self);
/// @returns If application should keep running. Clears after first return
PARROT_API bool ParrotMainLoop_next_frame(ParrotMainLoop *self,
                                          /* NULL = Unmodified */ float *delta,
                                          /* <=0 = Uncapped */ float max_fps);

/// Makes the main loop handle your program's lifecycle. Only returns after program shuts down
PARROT_API void ParrotMainLoop_run(ParrotMainLoop *self,
                                   void *user_data,
                                   /* NULL = uncalled */ ParrotMainLoopInitFunc init,
                                   /* Required */ ParrotMainLoopUpdateFunc update,
                                   /* NULL = uncalled */ ParrotMainLoopRenderFunc render,
                                   /* NULL = uncalled */ ParrotMainLoopShutdownFunc shutdown);

static const ParrotReflectDescription Parrot_core_main_loop_collection[] = {
    PARROT_REFLECT_COLLECTION_HEADER(),

    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotMainLoopRunSettings_description),

    PARROT_REFLECT_END(),
};

#endif // PARROT_CORE_MAIN_LOOP_H_
