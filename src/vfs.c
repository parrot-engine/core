#include "parrot/core/vfs.h"
#include "parrot/core/array.h"
#include "parrot/core/binary.h"
#include "parrot/core/math.h"
#include "parrot/core/scope.h"

typedef struct {
    char *key;

    /// NULL = directory
    uint8_t *data;
    size_t size;
    size_t capacity;
} ParrotVFSRAMEntry;

typedef struct {
    char *key;
    ParrotVFS *vfs;
} ParrotVFSMountEntry;

typedef struct {
    char *key;
    const ParrotVFSOverlay *overlay;
} ParrotVFSOverlayEntry;

struct ParrotVFS {
    ParrotScope *scope;

    ParrotVFSRAMEntry **sh_ramfs;
    ParrotVFSMountEntry **sh_mounts;
    ParrotVFSOverlayEntry **sh_overlays;
};

ParrotVFS *ParrotVFS_new(void) {
    ParrotVFS *self = PARROT_ALLOC(ParrotVFS);

    self->scope = ParrotScope_new(NULL);
    ParrotScope_push_free(self->scope, self);

    ParrotScope_push_arrfree(self->scope, self->sh_ramfs);

    return self;
}

void ParrotVFS_delete(ParrotVFS *self) {
    ParrotScope_delete(self->scope);
}

void ParrotVFS_vdelete(void *self) {
    ParrotVFS_delete(self);
}

static char *simplify_path_malloc(const char *path) {
    char *simplified = calloc(strlen(path) + 1, sizeof(char));
    PARROT_FAIL_NULL(simplified);

    char *out = simplified;

    bool is_absolute = *path == '/';
    if (is_absolute) {
        *out++ = *path++;
    }
    // Preserve root path
    if (*path == '/') {
        *out++ = *path++;
    }

    char *segment = out;
    int depth = 0;

    while (*path) {
        if (*path == '/') {
            // Ignore empty segments
            if (segment[0] == '\0') {
                path++;
                continue;
            }

            if (strcmp(segment, ".") == 0) {
                memset(segment, 0, (out - segment) * sizeof(char));
                out = segment;
                path++;

                continue;
            }

            if (strcmp(segment, "..") == 0) {
                char *original = out;

                out = segment;
                path++;

                if (out > simplified && *(out - 1) == '/') {
                    out--;
                }

                char *tmp = out;
                while (tmp > simplified && *(tmp - 1) != '/') {
                    tmp--;
                }
                segment = tmp;

                depth--;
                memset(out, 0, (original - out) * sizeof(char));
                if (depth < 0 && !is_absolute) {
                    depth = 0;
                }
                continue;
            }

            depth++;
            segment = out + 1;
        }

        *out++ = *path;
        path++;
    }

    // Remove trailing "/"
    if (*out == '/') {
        *out = '\0';
    }

    return simplified;
}

static ParrotVFS *find_mount(ParrotVFS *self, const char *path) {
    ParrotScope *scope = ParrotScope_new(self->scope);

    char *simplified_path = simplify_path_malloc(path);
    ParrotScope_push_free(scope, simplified_path);

    for (;;) {
        ParrotVFSMountEntry **p_entry = ParrotArray_findsp(self->sh_mounts, simplified_path);
        if (p_entry) {
            ParrotScope_delete(scope);
            return (*p_entry)->vfs;
        }

        size_t parent_seperator_index = strlen(simplified_path);

        for (size_t i = strlen(simplified_path); i > 0; i--) {
            if (simplified_path[i - 1] == '/') {
                parent_seperator_index = i - 1;
                break;
            }
        }

        if (parent_seperator_index >= strlen(simplified_path)) {
            ParrotScope_delete(scope);
            return NULL;
        }

        simplified_path[parent_seperator_index] = '\0';
    }

    ParrotScope_delete(scope);
    return NULL;
}

void ParrotVFS_mount(ParrotVFS *self, const char *path, ParrotVFS *vfs) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(path);

    ParrotScope *scope = ParrotScope_new(self->scope);

    char *simplified_path = simplify_path_malloc(path);
    ParrotScope_push_free(scope, simplified_path);

    ParrotVFSMountEntry *entry = PARROT_ALLOC(ParrotVFSMountEntry);

    entry->key = calloc(strlen(simplified_path) + 1, sizeof(char));
    strcpy(entry->key, simplified_path);

    entry->vfs = vfs;

    ParrotArray_put_key(self->sh_mounts, entry, entry->key, strlen(entry->key));

    ParrotScope_delete(scope);
}

void ParrotVFS_overlay(ParrotVFS *self, const char *path, const ParrotVFSOverlay *overlay) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(path);

    ParrotScope *scope = ParrotScope_new(self->scope);

    char *simplified_path = simplify_path_malloc(path);
    ParrotScope_push_free(scope, simplified_path);

    ParrotVFSOverlayEntry *entry = PARROT_ALLOC(ParrotVFSOverlayEntry);

    entry->key = calloc(strlen(simplified_path) + 1, sizeof(char));
    strcpy(entry->key, simplified_path);

    entry->overlay = overlay;

    ParrotArray_put_key(self->sh_overlays, entry, entry->key, strlen(entry->key));

    ParrotScope_delete(scope);
}

static bool is_direct_descandant(const char *path, const char *parent) {
    if (strncmp(path, parent, strlen(parent)) != 0) {
        return false;
    }

    if (path[strlen(parent)] != '/') {
        return false;
    }

    for (size_t j = strlen(parent) + 1; j < strlen(path); j++) {
        if (path[j] == '/') {
            return false;
        }
    }

    return true;
}

size_t ParrotVFS_list_count(ParrotVFS *self, const char *path) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(path);

    size_t ret = 0;
    ParrotScope *scope = ParrotScope_new(self->scope);

    ParrotVFS *mount = find_mount(self, path);
    if (mount) {
        ret = ParrotVFS_list_count(mount, path);
        goto end;
    }

    char *simplified_path = simplify_path_malloc(path);
    ParrotScope_push_free(scope, simplified_path);

    ptrdiff_t entry_index = ParrotArray_finds(self->sh_ramfs, simplified_path);
    if (entry_index < 0) {
        goto end;
    }

    if (self->sh_ramfs[entry_index]->data) {
        goto end;
    }

    for (size_t i = entry_index + 1; i < ParrotArray_size(self->sh_ramfs); i++) {
        ParrotVFSRAMEntry *entry = self->sh_ramfs[i];

        if (!is_direct_descandant(entry->key, simplified_path)) {
            break;
        }
        ret++;
    }

end:
    ParrotScope_delete(scope);
    return ret;
}

char *ParrotVFS_list_malloc(ParrotVFS *self, const char *path, size_t index) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(path);

    char *ret = NULL;
    ParrotScope *scope = ParrotScope_new(self->scope);

    ParrotVFS *mount = find_mount(self, path);
    if (mount) {
        ret = ParrotVFS_list_malloc(mount, path, index);
        goto end;
    }

    char *simplified_path = simplify_path_malloc(path);
    ParrotScope_push_free(scope, simplified_path);

    ptrdiff_t entry_index = ParrotArray_finds(self->sh_ramfs, simplified_path);
    if (entry_index < 0) {
        goto end;
    }

    if (self->sh_ramfs[entry_index]->data) {
        goto end;
    }

    size_t list_index = 0;
    for (size_t i = entry_index + 1; i < ParrotArray_size(self->sh_ramfs); i++) {
        ParrotVFSRAMEntry *entry = self->sh_ramfs[i];

        if (!is_direct_descandant(entry->key, simplified_path)) {
            break;
        }

        if (list_index++ != index) {
            continue;
        }

        ret = calloc(strlen(entry->key) + 1, sizeof(char));
        strcpy(ret, entry->key);
        break;
    }

end:
    ParrotScope_delete(scope);
    return ret;
}

void ParrotVFS_mkdir(ParrotVFS *self, const char *path) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(path);

    ParrotScope *scope = ParrotScope_new(self->scope);

    char *simplified_path = simplify_path_malloc(path);
    ParrotScope_push_free(scope, simplified_path);

    if (ParrotArray_findsp(self->sh_ramfs, simplified_path)) {
        ParrotScope_delete(scope);
        return;
    }

    size_t parent_seperator_index = strlen(simplified_path);
    for (size_t i = strlen(simplified_path); i > 0; i--) {
        if (simplified_path[i - 1] == '/') {
            parent_seperator_index = i - 1;
            break;
        }
    }

    if (parent_seperator_index >= strlen(simplified_path)) {
        ParrotScope_delete(scope);
        return;
    }

    simplified_path[parent_seperator_index] = '\0';

    ptrdiff_t parent_entry_index = ParrotArray_finds(self->sh_ramfs, simplified_path);
    if (parent_entry_index < 0) {
        ParrotVFS_mkdir(self, simplified_path);
        parent_entry_index = ParrotArray_finds(self->sh_ramfs, simplified_path);

        if (parent_entry_index < 0) {
            ParrotScope_delete(scope);
            return;
        }
    }

    ParrotVFSRAMEntry *parent_entry = self->sh_ramfs[parent_entry_index];
    if (parent_entry->data) {
        ParrotScope_delete(scope);
        return;
    }

    const char *filename = &simplified_path[parent_seperator_index + 1];
    simplified_path[parent_seperator_index] = '/';

    ParrotVFSRAMEntry *entry = PARROT_ALLOC(ParrotVFSRAMEntry);

    entry->key = calloc(strlen(filename) + 1, sizeof(char));
    strcpy(entry->key, filename);

    ParrotArray_alloc_at(self->sh_ramfs, parent_entry_index + 1, 1);
    self->sh_ramfs[parent_entry_index + 1] = entry;
    ParrotArray_mapx(self->sh_ramfs, parent_entry_index + 1, entry->key, strlen(entry->key));
}

void ParrotVFS_rm(ParrotVFS *self, const char *path) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(path);

    ParrotScope *scope = ParrotScope_new(self->scope);

    char *simplified_path = simplify_path_malloc(path);
    ParrotScope_push_free(scope, simplified_path);

    ptrdiff_t entry_index = ParrotArray_finds(self->sh_ramfs, simplified_path);
    if (entry_index < 0) {
        ParrotScope_delete(scope);
        return;
    }

    ParrotVFSRAMEntry *entry = self->sh_ramfs[entry_index];

    if (entry->data) {
        ParrotArray_del(self->sh_ramfs, entry_index);
        ParrotScope_delete(scope);
        return;
    }

    char **delete_list = NULL;
    ParrotScope_push_arrfree(scope, delete_list);

    for (size_t i = 0; i < ParrotVFS_list_count(self, simplified_path); i++) {
        char *subpath = ParrotVFS_list_malloc(self, simplified_path, i);
        ParrotScope_push_free(scope, subpath);
        ParrotArray_push(delete_list, subpath);
    }

    for (size_t i = 0; i < ParrotArray_size(delete_list); i++) {
        ParrotVFS_rm(self, delete_list[i]);
    }

    ParrotScope_delete(scope);
}

bool ParrotVFS_exists(ParrotVFS *self, const char *path) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(path);

    return ParrotVFS_is_file(self, path) || ParrotVFS_is_dir(self, path);
}

bool ParrotVFS_is_dir(ParrotVFS *self, const char *path) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(path);

    bool ret = false;
    ParrotScope *scope = ParrotScope_new(self->scope);

    ParrotVFS *mount = find_mount(self, path);
    if (mount) {
        ret = ParrotVFS_is_dir(mount, path);
        goto end;
    }

    char *simplified_path = simplify_path_malloc(path);
    ParrotScope_push_free(scope, simplified_path);

    ParrotVFSRAMEntry *entry = *(ParrotVFSRAMEntry **)ParrotArray_findsp(self->sh_ramfs, simplified_path);
    ret = entry && !entry->data;

end:
    ParrotScope_delete(scope);
    return ret;
}

bool ParrotVFS_is_file(ParrotVFS *self, const char *path) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(path);

    bool ret = false;
    ParrotScope *scope = ParrotScope_new(self->scope);

    ParrotVFS *mount = find_mount(self, path);
    if (mount) {
        ret = ParrotVFS_is_file(mount, path);
        goto end;
    }

    char *simplified_path = simplify_path_malloc(path);
    ParrotScope_push_free(scope, simplified_path);

    ParrotVFSRAMEntry *entry = *(ParrotVFSRAMEntry **)ParrotArray_findsp(self->sh_ramfs, simplified_path);
    ret = entry && entry->data;

end:
    ParrotScope_delete(scope);
    return ret;
}

static void ramfs_buffer_write(ParrotScope *scope, size_t size, const uint8_t *data) {
    ParrotVFSRAMEntry *entry = ParrotScope_get_ctx(scope, ParrotVFSRAMEntry *);

    size_t new_size = entry->size + size;
    if (new_size > entry->capacity) {
        entry->capacity = PARROT_MAX(new_size, entry->capacity * 2);
        entry->data = realloc(entry->data, entry->capacity);
    }

    memcpy(entry->data + entry->size, data, size);
    entry->size += size;
}

static bool ramfs_buffer_read(ParrotScope *scope, size_t offset, size_t size, uint8_t *out) {
    ParrotVFSRAMEntry *entry = ParrotScope_get_ctx(scope, ParrotVFSRAMEntry *);

    if (offset + size > entry->size) {
        return false;
    }

    memcpy(out, entry->data + offset, size);
    return true;
}

ParrotBuffer *ParrotVFS_open(ParrotVFS *self, const char *path, ParrotVFSOpenFlags flags) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(path);

    ParrotBuffer *ret = NULL;
    ParrotScope *scope = ParrotScope_new(self->scope);

    ParrotVFS *mount = find_mount(self, path);
    if (mount) {
        ret = ParrotVFS_open(mount, path, flags);
        goto end;
    }

    char *simplified_path = simplify_path_malloc(path);
    ParrotScope_push_free(scope, simplified_path);

    ParrotVFSRAMEntry **p_entry = ParrotArray_findsp(self->sh_ramfs, simplified_path);
    ParrotVFSRAMEntry *entry = p_entry ? *p_entry : NULL;

    if (!entry) {
        size_t parent_seperator_index = strlen(simplified_path);
        for (size_t i = strlen(simplified_path); i > 0; i--) {
            if (simplified_path[i - 1] == '/') {
                parent_seperator_index = i - 1;
                break;
            }
        }

        if (parent_seperator_index >= strlen(simplified_path)) {
            goto end;
        }

        simplified_path[parent_seperator_index] = '\0';

        ptrdiff_t parent_entry_index = ParrotArray_finds(self->sh_ramfs, simplified_path);
        if (parent_entry_index < 0) {
            goto end;
        }

        ParrotVFSRAMEntry *parent_entry = self->sh_ramfs[parent_entry_index];
        if (parent_entry->data) {
            goto end;
        }

        const char *filename = &simplified_path[parent_seperator_index + 1];
        simplified_path[parent_seperator_index] = '/';

        entry = PARROT_ALLOC(ParrotVFSRAMEntry);

        entry->key = calloc(strlen(filename) + 1, sizeof(char));
        strcpy(entry->key, filename);

        entry->capacity = 256;
        entry->data = malloc(entry->capacity);

        ParrotArray_alloc_at(self->sh_ramfs, parent_entry_index + 1, 1);
        self->sh_ramfs[parent_entry_index + 1] = entry;
        ParrotArray_mapx(self->sh_ramfs, parent_entry_index + 1, entry->key, strlen(entry->key));
    }

    if (!entry->data) {
        goto end;
    }

    ParrotScope *buffer_scope = ParrotScope_new(self->scope);
    ParrotScope_set_ctx(buffer_scope, entry);
    ret = ParrotBuffer_new(buffer_scope,
                           flags & ParrotVFSOpenFlags_READ ? ramfs_buffer_read : NULL,
                           flags & ParrotVFSOpenFlags_WRITE ? ramfs_buffer_write : NULL);

end:
    ParrotScope_delete(scope);
    return ret;
}
