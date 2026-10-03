#ifndef PARROT_CORE_MATH_H_
#define PARROT_CORE_MATH_H_

#include <math.h> // IWYU pragma: keep
#include <stdint.h>

#include "parrot/core/util.h"
#include "parrot/core/reflect.h"

/// 48.16 fixed point
typedef int64_t ParrotFixed64i;
#define ParrotFixed64i_MIN ((int64_t)-281474976710657)
#define ParrotFixed64i_MAX ((int64_t)281474976710657)
#define ParrotFixed64i_to_double(n) ((double)(n) / 65536.0f)
#define ParrotFixed64i_from_double(n)                                                                                   \
    ((ParrotFixed64i)(PARROT_CLAMP(ParrotFixed64i_MIN, n, ParrotFixed64i_MAX) * 65536.0f + 0.5f) - ((n) < 0))

/// 32.32 fixed point
typedef int64_t ParrotFixed64s;
#define ParrotFixed64s_MIN ((int64_t)-2147483647)
#define ParrotFixed64s_MAX ((int64_t)2147483647)
#define ParrotFixed64s_to_double(n) ((double)(n) / 2147483648.0f)
#define ParrotFixed64s_from_double(n)                                                                                   \
    ((ParrotFixed64s)(PARROT_CLAMP(ParrotFixed64s_MIN, n, ParrotFixed64s_MAX) * 2147483648.0f + 0.5f) - ((n) < 0))

/// 16.48 fixed point
typedef int64_t ParrotFixed64f;
#define ParrotFixed64f_MIN ((int64_t)-65535)
#define ParrotFixed64f_MAX ((int64_t)65535)
#define ParrotFixed64f_to_double(n) ((double)(n) / 281474976710658.0f)
#define ParrotFixed64f_from_double(n)                                                                                   \
    ((ParrotFixed64f)(PARROT_CLAMP(ParrotFixed64f_MIN, n, ParrotFixed64f_MAX) * 281474976710658.0f + 0.5f) - ((n) < 0))

/// 24.8 fixed point
typedef int32_t ParrotFixed32i;
#define ParrotFixed32i_MIN ((int32_t)-8388607)
#define ParrotFixed32i_MAX ((int32_t)8388607)
#define ParrotFixed32i_to_float(n) ((float)(n) / 256.0f)
#define ParrotFixed32i_from_float(n)                                                                                    \
    ((ParrotFixed32i)(PARROT_CLAMP(ParrotFixed32i_MIN, n, ParrotFixed32i_MAX) * 256.0f + 0.5f) - ((n) < 0))

/// 16.16 fixed point
typedef int32_t ParrotFixed32s;
#define ParrotFixed32s_MIN ((int32_t)-32767)
#define ParrotFixed32s_MAX ((int32_t)32767)
#define ParrotFixed32s_to_float(n) ((float)(n) / 65536.0f)
#define ParrotFixed32s_from_float(n)                                                                                    \
    ((ParrotFixed32s)(PARROT_CLAMP(ParrotFixed32s_MIN, n, ParrotFixed32s_MAX) * 65536.0f + 0.5f) - ((n) < 0))

/// 8.24 fixed point
typedef int32_t ParrotFixed32f;
#define ParrotFixed32f_MIN ((int32_t)-127)
#define ParrotFixed32f_MAX ((int32_t)127)
#define ParrotFixed32f_to_float(n) ((float)(n) / 8388608.0f)
#define ParrotFixed32f_from_float(n)                                                                                    \
    ((ParrotFixed32f)(PARROT_CLAMP(ParrotFixed32f_MIN, n, ParrotFixed32f_MAX) * 8388608.0f + 0.5f) - ((n) < 0))

/// 12.4 fixed point
typedef int16_t ParrotFixed16i;
#define ParrotFixed16i_MIN ((int16_t)-4095)
#define ParrotFixed16i_MAX ((int16_t)4095)
#define ParrotFixed16i_to_float(n) ((float)(n) / 16.0f)
#define ParrotFixed16i_from_float(n)                                                                                    \
    ((ParrotFixed16i)(PARROT_CLAMP(ParrotFixed16i_MIN, n, ParrotFixed16i_MAX) * 16.0f + 0.5f) - ((n) < 0))

/// 8.8 fixed point
typedef int16_t ParrotFixed16s;
#define ParrotFixed16s_MIN ((int16_t)-255)
#define ParrotFixed16s_MAX ((int16_t)255)
#define ParrotFixed16s_to_float(n) ((float)(n) / 256.0f)
#define ParrotFixed16s_from_float(n)                                                                                    \
    ((ParrotFixed16s)(PARROT_CLAMP(ParrotFixed16s_MIN, n, ParrotFixed16s_MAX) * 256.0f + 0.5f) - ((n) < 0))

/// 4.12 fixed point
typedef int16_t ParrotFixed16f;
#define ParrotFixed16f_MIN ((int16_t)-15)
#define ParrotFixed16f_MAX ((int16_t)15)
#define ParrotFixed16f_to_float(n) ((float)(n) / 4096.0f)
#define ParrotFixed16f_from_float(n)                                                                                    \
    ((ParrotFixed16f)(PARROT_CLAMP(ParrotFixed16f_MIN, n, ParrotFixed16f_MAX) * 4096.0f + 0.5f) - ((n) < 0))

#ifndef PARROT_DOUBLE_PRECISION
#define ParrotReal float
#define ParrotReal_sqrt sqrtf
#define ParrotReal_sin sinf
#define ParrotReal_cos cosf
#define ParrotReal_atan2 atan2f
#else
#define ParrotReal double
#define ParrotReal_sqrt sqrt
#define ParrotReal_sin sin
#define ParrotReal_cos cos
#define ParrotReal_atan2 atan2
#endif

#ifdef PARROT_CORE_REFLECT_H_
static const ParrotReflectDescription ParrotReal_description[] = {
#ifndef PARROT_DOUBE_PRECISION
    PARROT_REFLECT_ALIAS(ParrotReal, float),
#else
    PARROT_REFLECT_ALIAS(ParrotReal, double),
#endif
};
#endif

#define PARROT_CLAMP(min, value, max) PARROT_MAX(min, PARROT_MIN(value, max))
#define PARROT_MIN(a, b) ((a) > (b) ? (b) : (a))
#define PARROT_MAX(a, b) ((a) > (b) ? (a) : (b))

PARROT_API ParrotReal Parrot_lerp(ParrotReal a, ParrotReal b, ParrotReal t);
PARROT_API float Parrot_lerpf(float a, float b, float t);
PARROT_API double Parrot_lerpd(double a, double b, double t);

PARROT_API void ParrotReal_to_float_array(const ParrotReal *src, float *dest, size_t count);

typedef struct {
    ParrotReal x;
    ParrotReal y;
} ParrotVec2;

static const ParrotReflectDescription ParrotVec2_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotVec2),

    PARROT_REFLECT_TYPE_FIELD(ParrotVec2, ParrotReal, x, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotVec2, ParrotReal, y, ),

    PARROT_REFLECT_END(),
};

PARROT_API ParrotVec2 ParrotVec2_n(ParrotReal n);

PARROT_API ParrotVec2 ParrotVec2_add(ParrotVec2 a, ParrotVec2 b);
PARROT_API ParrotVec2 ParrotVec2_sub(ParrotVec2 a, ParrotVec2 b);
PARROT_API ParrotVec2 ParrotVec2_mul(ParrotVec2 a, ParrotVec2 b);
PARROT_API ParrotVec2 ParrotVec2_div(ParrotVec2 a, ParrotVec2 b);
PARROT_API ParrotVec2 ParrotVec2_scale(ParrotVec2 a, ParrotReal b);

PARROT_API ParrotVec2 ParrotVec2_normalize(ParrotVec2 self);

PARROT_API ParrotReal ParrotVec2_length(ParrotVec2 self);
PARROT_API ParrotReal ParrotVec2_dot(ParrotVec2 self, ParrotVec2 other);

typedef struct {
    ParrotReal x;
    ParrotReal y;
    ParrotReal z;
} ParrotVec3;

static const ParrotReflectDescription ParrotVec3_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotVec3),

    PARROT_REFLECT_TYPE_FIELD(ParrotVec3, ParrotReal, x, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotVec3, ParrotReal, y, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotVec3, ParrotReal, z, ),

    PARROT_REFLECT_END(),
};

PARROT_API ParrotVec3 ParrotVec3_n(ParrotReal n);
PARROT_API ParrotVec3 ParrotVec3_upgrade(ParrotVec2 v);

PARROT_API ParrotVec3 ParrotVec3_add(ParrotVec3 a, ParrotVec3 b);
PARROT_API ParrotVec3 ParrotVec3_sub(ParrotVec3 a, ParrotVec3 b);
PARROT_API ParrotVec3 ParrotVec3_mul(ParrotVec3 a, ParrotVec3 b);
PARROT_API ParrotVec3 ParrotVec3_div(ParrotVec3 a, ParrotVec3 b);
PARROT_API ParrotVec3 ParrotVec3_scale(ParrotVec3 a, ParrotReal b);

PARROT_API ParrotVec3 ParrotVec3_normalize(ParrotVec3 self);

PARROT_API ParrotReal ParrotVec3_length(ParrotVec3 self);
PARROT_API ParrotReal ParrotVec3_dot(ParrotVec3 self, ParrotVec3 other);

typedef struct {
    ParrotReal x;
    ParrotReal y;
    ParrotReal z;
    ParrotReal w;
} ParrotVec4;

static const ParrotReflectDescription ParrotVec4_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotVec4),

    PARROT_REFLECT_TYPE_FIELD(ParrotVec4, ParrotReal, x, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotVec4, ParrotReal, y, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotVec4, ParrotReal, z, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotVec4, ParrotReal, w, ),

    PARROT_REFLECT_END(),
};

PARROT_API ParrotVec4 ParrotVec4_n(ParrotReal n);
PARROT_API ParrotVec4 ParrotVec4_upgrade(ParrotVec3 v);

PARROT_API ParrotVec4 ParrotVec4_add(ParrotVec4 a, ParrotVec4 b);
PARROT_API ParrotVec4 ParrotVec4_sub(ParrotVec4 a, ParrotVec4 b);
PARROT_API ParrotVec4 ParrotVec4_mul(ParrotVec4 a, ParrotVec4 b);
PARROT_API ParrotVec4 ParrotVec4_div(ParrotVec4 a, ParrotVec4 b);
PARROT_API ParrotVec4 ParrotVec4_scale(ParrotVec4 a, ParrotReal b);

PARROT_API ParrotVec4 ParrotVec4_normalize(ParrotVec4 self);

PARROT_API ParrotReal ParrotVec4_length(ParrotVec4 self);
PARROT_API ParrotReal ParrotVec4_dot(ParrotVec4 self, ParrotVec4 other);

typedef struct {
    // Column-major (accessed like data[x][y])
    ParrotReal data[4][4];
} ParrotMat;

static const ParrotReflectDescription ParrotMat_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotMat),

    PARROT_REFLECT_TYPE_FIELD(ParrotMat, ParrotReal, data, [4][4]),

    PARROT_REFLECT_END(),
};

PARROT_API ParrotMat ParrotMat_identity(void);

PARROT_API ParrotMat ParrotMat_inverse(ParrotMat matrix);
PARROT_API ParrotMat ParrotMat_transpose(ParrotMat matrix);

PARROT_API ParrotMat ParrotMat_add(ParrotMat a, ParrotMat b);
PARROT_API ParrotMat ParrotMat_mul(ParrotMat a, ParrotMat b);

PARROT_API ParrotVec3 ParrotMat_transform3(ParrotMat matrix, ParrotVec3 vec);
PARROT_API ParrotVec2 ParrotMat_transform2(ParrotMat matrix, ParrotVec2 vec);

PARROT_API ParrotMat
ParrotMat_ortho(ParrotReal left, ParrotReal right, ParrotReal down, ParrotReal up, ParrotReal near, ParrotReal far);

PARROT_API ParrotMat ParrotMat_translation(ParrotVec3 position);
PARROT_API ParrotMat ParrotMat_rotation(ParrotVec3 rotation);
PARROT_API ParrotMat ParrotMat_scale(ParrotVec3 scale);

typedef struct {
    ParrotMat model;
    ParrotMat view;
    ParrotMat projection;
} ParrotGMatSet;

static const ParrotReflectDescription ParrotGMatSet_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotGMatSet),

    PARROT_REFLECT_TYPE_FIELD(ParrotGMatSet, ParrotMat, model, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotGMatSet, ParrotMat, view, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotGMatSet, ParrotMat, projection, ),

    PARROT_REFLECT_END(),
};

typedef struct ParrotTransform ParrotTransform;

struct ParrotTransform {
    ParrotTransform *parent;
    ParrotMat matrix;

    ParrotVec3 position;
    ParrotVec3 rotation;
    ParrotVec3 scale;
};

static const ParrotReflectDescription ParrotTransform_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotTransform),

    PARROT_REFLECT_TYPE_FIELD(ParrotTransform, ParrotTransform *, parent, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotTransform, ParrotMat, matrix, ),

    PARROT_REFLECT_TYPE_FIELD(ParrotTransform, ParrotVec3, position, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotTransform, ParrotVec3, rotation, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotTransform, ParrotVec3, scale, ),

    PARROT_REFLECT_END(),
};

ParrotTransform ParrotTransform_new(void);

ParrotMat ParrotTransform_calculate_matrix(const ParrotTransform *self);

PARROT_API ParrotMat ParrotGMatSet_combine(const ParrotGMatSet *self);

typedef enum {
    ParrotColorFormat_RGBA8888 = 0,
    ParrotColorFormat_BGRA8888,
} ParrotColorFormat;

typedef struct {
    // [0.0, 1.0]
    float r;
    // [0.0, 1.0]
    float g;
    // [0.0, 1.0]
    float b;
    // [0.0, 1.0]
    float a;
} ParrotColor;

static const ParrotReflectDescription ParrotColor_description[] = {
    PARROT_REFLECT_TYPE_HEADER(ParrotColor),

    PARROT_REFLECT_TYPE_FIELD(ParrotColor, float, r, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotColor, float, g, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotColor, float, b, ),
    PARROT_REFLECT_TYPE_FIELD(ParrotColor, float, a, ),

    PARROT_REFLECT_END(),
};

PARROT_API ParrotColor ParrotColor_new(uint8_t r, uint8_t g, uint8_t b);
PARROT_API ParrotColor ParrotColor_newf(float r, float g, float b);
PARROT_API ParrotColor ParrotColor_newa(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
PARROT_API ParrotColor ParrotColor_newaf(float r, float g, float b, float a);

PARROT_API ParrotColor ParrotColor_from(ParrotColorFormat format, uint32_t color);
PARROT_API uint32_t ParrotColor_to(ParrotColor self, ParrotColorFormat format);

PARROT_API ParrotColor ParrotColor_mul(ParrotColor a, ParrotColor b);
PARROT_API ParrotColor ParrotColor_blend(ParrotColor a, ParrotColor b);
PARROT_API ParrotColor ParrotColor_lerp(ParrotColor a, ParrotColor b, float t);

#define ParrotColor_CLEAR ((ParrotColor){.r = 0.0, .g = 0.0, .b = 0.0, .a = 0.0})
#define ParrotColor_BLACK ((ParrotColor){.r = 0.0, .g = 0.0, .b = 0.0, .a = 1.0})
#define ParrotColor_WHITE ((ParrotColor){.r = 1.0, .g = 1.0, .b = 1.0, .a = 1.0})
#define ParrotColor_GRAY ((ParrotColor){.r = 0.5, .g = 0.5, .b = 0.5, .a = 1.0})
#define ParrotColor_GREY ParrotColor_GRAY
#define ParrotColor_RED ((ParrotColor){.r = 1.0, .g = 0.0, .b = 0.0, .a = 1.0})
#define ParrotColor_GREEN ((ParrotColor){.r = 0.0, .g = 1.0, .b = 0.0, .a = 1.0})
#define ParrotColor_BLUE ((ParrotColor){.r = 0.0, .g = 0.0, .b = 1.0, .a = 1.0})
#define ParrotColor_YELLOW ((ParrotColor){.r = 1.0, .g = 1.0, .b = 0.0, .a = 1.0})
#define ParrotColor_ORANGE ((ParrotColor){.r = 1.0, .g = 0.3, .b = 0.0, .a = 1.0})
#define ParrotColor_CYAN ((ParrotColor){.r = 0.0, .g = 1.0, .b = 1.0, .a = 1.0})

static const ParrotReflectDescription Parrot_core_math_collection[] = {
    PARROT_REFLECT_COLLECTION_HEADER(),

    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotVec2_description),
    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotVec3_description),
    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotVec4_description),

    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotMat_description),
    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotGMatSet_description),

    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotTransform_description),

    PARROT_REFLECT_COLLECTION_DESCRIPTION(ParrotColor_description),

    PARROT_REFLECT_END(),
};

#endif // PARROT_CORE_MATH_H_
