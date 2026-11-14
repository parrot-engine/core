#ifndef PARROT_CORE_INCLUDE_PARROT_CORE_COLLISION_H_
#define PARROT_CORE_INCLUDE_PARROT_CORE_COLLISION_H_

#include "parrot/core/math.h"

typedef enum {
    ParrotCollisionShapeType_TRIANGLE = 0,
} ParrotCollisionShapeType;

typedef struct {
    ParrotCollisionShapeType type;
    union {
        struct {
            ParrotVec3 a;
            ParrotVec3 b;
            ParrotVec3 c;
        } triangle;
    } data;
} ParrotCollisionShape;

typedef struct {
    ParrotVec3 point;
    ParrotVec3 normal;
    ParrotReal depth;
} ParrotCollisionInfo;

typedef struct CollisionNode {
    struct CollisionNode *left;
    struct CollisionNode *right;

    ParrotCollisionShape *arr_shapes;
} CollisionNode;

PARROT_API ParrotCollisionShape ParrotCollisionShape_to_triangle(const ParrotCollisionShape *self);

PARROT_API bool ParrotCollisionShape_collision(const ParrotCollisionShape *a,
                                               const ParrotCollisionShape *b,
                                               ParrotCollisionInfo *out_info_or_null);

PARROT_API bool ParrotCollisionShape_raycast(const ParrotCollisionShape *shape, ParrotVec3 origin, ParrotVec3 direction);

#endif // PARROT_CORE_INCLUDE_PARROT_CORE_COLLISION_H_
