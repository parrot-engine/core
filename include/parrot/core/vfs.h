#ifndef PARROT_CORE_INCLUDE_PARROT_CORE_VFS_H_
#define PARROT_CORE_INCLUDE_PARROT_CORE_VFS_H_

#include "parrot/core/binary.h"
#include "parrot/core/util.h"

typedef struct ParrotVFS ParrotVFS;
typedef struct ParrotVFSOverlay ParrotVFSOverlay;

typedef enum {
    ParrotVFSOpenFlags_READ = 1 << 0,
    ParrotVFSOpenFlags_WRITE = 1 << 1,
} ParrotVFSOpenFlags;

struct ParrotVFSOverlay {
    size_t (*list_count_or_zero)(ParrotVFSOverlay *self, const char *path);
    char *(*list_malloc_or_null)(ParrotVFSOverlay *self, const char *path, size_t index);

    void (*mkdir)(ParrotVFSOverlay *self, const char *path);
    void (*rm)(ParrotVFSOverlay *self, const char *path);
    bool (*is_dir)(ParrotVFSOverlay *self, const char *path);
    bool (*is_file)(ParrotVFSOverlay *self, const char *path);

    ParrotBuffer *(*open_or_null)(ParrotVFSOverlay *self, const char *path, ParrotVFSOpenFlags flags);
};

PARROT_API ParrotVFS *ParrotVFS_new(void);
PARROT_API void ParrotVFS_delete(ParrotVFS *self);
PARROT_API void ParrotVFS_vdelete(void *self);

/// Completely replaces the directory with a portal
PARROT_API void ParrotVFS_mount(ParrotVFS *self, const char *path, ParrotVFS *vfs);
/**
 * Combines overlay's files and attempts to open in reverse addition order. File structure modifications (not to be
 * confused with file writes) at the overlay root are done on the overlay root directory, not the overlay.
 */
PARROT_API void ParrotVFS_overlay(ParrotVFS *self,
                                  const char *path,
                                  /* Does not take ownership */ const ParrotVFSOverlay *overlay);

PARROT_API size_t ParrotVFS_list_count_or_zero(ParrotVFS *self, const char *path);
PARROT_API char *ParrotVFS_list_malloc_or_null(ParrotVFS *self, const char *path, size_t index);

/// Recursively creates parents by default
PARROT_API void ParrotVFS_mkdir(ParrotVFS *self, const char *path);
PARROT_API void ParrotVFS_rm(ParrotVFS *self, const char *path);
PARROT_API bool ParrotVFS_exists(ParrotVFS *self, const char *path);
PARROT_API bool ParrotVFS_is_dir(ParrotVFS *self, const char *path);
PARROT_API bool ParrotVFS_is_file(ParrotVFS *self, const char *path);

PARROT_API ParrotBuffer *ParrotVFS_open_or_null(ParrotVFS *self, const char *path, ParrotVFSOpenFlags flags);

#endif // PARROT_CORE_INCLUDE_PARROT_CORE_VFS_H_
