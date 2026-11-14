#include "parrot/core/collision.h"
#include "parrot/core/util.h"
#include <float.h>

ParrotCollisionShape ParrotCollisionShape_to_triangle(const ParrotCollisionShape *self) {
    switch (self->type) {
    case ParrotCollisionShapeType_TRIANGLE:
    default:
        return *self;
    }
}

static ParrotVec3 cross(ParrotVec3 a, ParrotVec3 b) {
    return (ParrotVec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

static void
project(ParrotVec3 axis, ParrotVec3 p0, ParrotVec3 p1, ParrotVec3 p2, ParrotReal *out_min, ParrotReal *out_max) {
    ParrotReal d0 = ParrotVec3_dot(axis, p0);
    ParrotReal d1 = ParrotVec3_dot(axis, p1);
    ParrotReal d2 = ParrotVec3_dot(axis, p2);
    *out_min = fminf(d0, fminf(d1, d2));
    *out_max = fmaxf(d0, fmaxf(d1, d2));
}

static bool test_axis(ParrotVec3 raw_axis,
                      ParrotVec3 a0,
                      ParrotVec3 a1,
                      ParrotVec3 a2,
                      ParrotVec3 b0,
                      ParrotVec3 b1,
                      ParrotVec3 b2,
                      ParrotReal *io_best_depth,
                      ParrotVec3 *io_best_normal) {
    ParrotReal len = ParrotVec3_length(raw_axis);
    if (len < 1e-10f)
        return true;

    ParrotVec3 n = ParrotVec3_normalize(raw_axis);

    ParrotReal a_min, a_max, b_min, b_max;
    project(n, a0, a1, a2, &a_min, &a_max);
    project(n, b0, b1, b2, &b_min, &b_max);

    if (a_max < b_min || b_max < a_min) {
        return false;
    }

    ParrotReal depth = fminf(a_max - b_min, b_max - a_min);

    if (depth < *io_best_depth) {
        *io_best_depth = depth;

        ParrotReal ca = (ParrotVec3_dot(n, a0) + ParrotVec3_dot(n, a1) + ParrotVec3_dot(n, a2)) * (1.0f / 3.0f);
        ParrotReal cb = (ParrotVec3_dot(n, b0) + ParrotVec3_dot(n, b1) + ParrotVec3_dot(n, b2)) * (1.0f / 3.0f);
        *io_best_normal = (ca >= cb) ? n : ParrotVec3_scale(n, -1.0f);
    }

    return true;
}

static ParrotVec3 deepest_vertex(ParrotVec3 normal, ParrotVec3 p0, ParrotVec3 p1, ParrotVec3 p2) {
    ParrotReal d0 = ParrotVec3_dot(normal, p0);
    ParrotReal d1 = ParrotVec3_dot(normal, p1);
    ParrotReal d2 = ParrotVec3_dot(normal, p2);

    if (d0 <= d1 && d0 <= d2) {
        return p0;
    }

    if (d1 <= d2) {
        return p1;
    }

    return p2;
}

bool ParrotCollisionShape_collision(const ParrotCollisionShape *a,
                                    const ParrotCollisionShape *b,
                                    ParrotCollisionInfo *out_info_or_null) {
    PARROT_FAIL_NULL(a);
    PARROT_FAIL_NULL(b);

    if (out_info_or_null) {
        memset(out_info_or_null, 0, sizeof(ParrotCollisionInfo));
    }

    ParrotCollisionShape tri_a = ParrotCollisionShape_to_triangle(a);
    ParrotCollisionShape tri_b = ParrotCollisionShape_to_triangle(b);

    ParrotVec3 a0 = tri_a.data.triangle.a, a1 = tri_a.data.triangle.b, a2 = tri_a.data.triangle.c;
    ParrotVec3 b0 = tri_b.data.triangle.a, b1 = tri_b.data.triangle.b, b2 = tri_b.data.triangle.c;

    ParrotVec3 ea[3] = {ParrotVec3_sub(a1, a0), ParrotVec3_sub(a2, a1), ParrotVec3_sub(a0, a2)};
    ParrotVec3 eb[3] = {ParrotVec3_sub(b1, b0), ParrotVec3_sub(b2, b1), ParrotVec3_sub(b0, b2)};

    ParrotVec3 axes[11];
    axes[0] = cross(ea[0], ea[1]);
    axes[1] = cross(eb[0], eb[1]);

    int idx = 2;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            axes[idx++] = cross(ea[i], eb[j]);
        }
    }

    ParrotReal best_depth = FLT_MAX;
    ParrotVec3 best_normal = ParrotVec3_n(0);

    for (int i = 0; i < 11; i++) {
        if (!test_axis(axes[i], a0, a1, a2, b0, b1, b2, &best_depth, &best_normal)) {
            return false;
        }
    }

    if (out_info_or_null) {
        out_info_or_null->normal = best_normal;
        out_info_or_null->depth = best_depth;
        out_info_or_null->point = deepest_vertex(ParrotVec3_scale(best_normal, -1.0f), b0, b1, b2);
    }

    return true;
}

bool ParrotCollisionShape_raycast(const ParrotCollisionShape *shape, ParrotVec3 origin, ParrotVec3 direction) {
    PARROT_FAIL_NULL(shape);

    ParrotCollisionShape tri = ParrotCollisionShape_to_triangle(shape);
    ParrotVec3 a = tri.data.triangle.a;
    ParrotVec3 b = tri.data.triangle.b;
    ParrotVec3 c = tri.data.triangle.c;

    ParrotVec3 ab = ParrotVec3_sub(b, a);
    ParrotVec3 ac = ParrotVec3_sub(c, a);
    ParrotVec3 h = cross(direction, ac);
    ParrotReal det = ParrotVec3_dot(ab, h);

    if (fabsf(det) < 1e-8f) {
        return false;
    }

    ParrotReal inv_det = 1.0f / det;
    ParrotVec3 s = ParrotVec3_sub(origin, a);

    ParrotReal u = inv_det * ParrotVec3_dot(s, h);
    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    ParrotVec3 q = cross(s, ab);
    ParrotReal v = inv_det * ParrotVec3_dot(direction, q);
    if (v < 0.0f || (u + v) > 1.0f) {
        return false;
    }

    ParrotReal t = inv_det * ParrotVec3_dot(ac, q);
    return t >= 0.0f;
}
