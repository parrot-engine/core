#include "parrot/core/math.h"

ParrotReal Parrot_lerp(ParrotReal a, ParrotReal b, ParrotReal t) {
    return (1 - t) * a + t * b;
}

float Parrot_lerpf(float a, float b, float t) {
    return (1 - t) * a + t * b;
}

double Parrot_lerpd(double a, double b, double t) {
    return (1 - t) * a + t * b;
}

void ParrotReal_to_float_array(const ParrotReal *src, ParrotReal *dest, size_t count) {
    for (size_t i = 0; i < count; i++) {
        dest[i] = src[i];
    }
}

ParrotVec2 ParrotVec2_n(ParrotReal n) {
    return (ParrotVec2){n, n};
}

ParrotVec2 ParrotVec2_add(ParrotVec2 a, ParrotVec2 b) {
    return (ParrotVec2){a.x + b.x, a.y + b.y};
}

ParrotVec2 ParrotVec2_sub(ParrotVec2 a, ParrotVec2 b) {
    return (ParrotVec2){a.x - b.x, a.y - b.y};
}

ParrotVec2 ParrotVec2_mul(ParrotVec2 a, ParrotVec2 b) {
    return (ParrotVec2){a.x * b.x, a.y * b.y};
}

ParrotVec2 ParrotVec2_div(ParrotVec2 a, ParrotVec2 b) {
    return (ParrotVec2){a.x / b.x, a.y / b.y};
}

ParrotVec2 ParrotVec2_scale(ParrotVec2 a, ParrotReal b) {
    return (ParrotVec2){a.x * b, a.y * b};
}

ParrotVec2 ParrotVec2_normalize(ParrotVec2 self) {
    ParrotReal length = ParrotVec2_length(self);

    if (length == 0) {
        return ParrotVec2_n(0);
    }

    return (ParrotVec2){self.x / length, self.y / length};
}

ParrotReal ParrotVec2_length(ParrotVec2 self) {
    return ParrotReal_sqrt(self.x * self.x + self.y * self.y);
}

ParrotReal ParrotVec2_dot(ParrotVec2 self, ParrotVec2 other) {
    return self.x * other.x + self.y * other.y;
}

ParrotVec3 ParrotVec3_n(ParrotReal n) {
    return (ParrotVec3){n, n, n};
}

ParrotVec3 ParrotVec3_upgrade(ParrotVec2 v) {
    return (ParrotVec3){
        v.x,
        v.y,
        0,
    };
}

ParrotVec3 ParrotVec3_add(ParrotVec3 a, ParrotVec3 b) {
    return (ParrotVec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

ParrotVec3 ParrotVec3_sub(ParrotVec3 a, ParrotVec3 b) {
    return (ParrotVec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

ParrotVec3 ParrotVec3_mul(ParrotVec3 a, ParrotVec3 b) {
    return (ParrotVec3){a.x * b.x, a.y * b.y, a.z * b.z};
}

ParrotVec3 ParrotVec3_div(ParrotVec3 a, ParrotVec3 b) {
    return (ParrotVec3){a.x / b.x, a.y / b.y, a.z / b.z};
}

ParrotVec3 ParrotVec3_scale(ParrotVec3 a, ParrotReal b) {
    return (ParrotVec3){a.x * b, a.y * b, a.z * b};
}

ParrotVec3 ParrotVec3_normalize(ParrotVec3 self) {
    ParrotReal length = ParrotVec3_length(self);

    if (length == 0) {
        return ParrotVec3_n(0);
    }

    return (ParrotVec3){self.x / length, self.y / length, self.z / length};
}

ParrotReal ParrotVec3_length(ParrotVec3 self) {
    return ParrotReal_sqrt(self.x * self.x + self.y * self.y + self.z * self.z);
}

ParrotReal ParrotVec3_dot(ParrotVec3 self, ParrotVec3 other) {
    return self.x * other.x + self.y * other.y + self.z * other.z;
}

ParrotVec4 ParrotVec4_n(ParrotReal n) {
    return (ParrotVec4){n, n, n, n};
}

ParrotVec4 ParrotVec4_upgrade(ParrotVec3 v) {
    return (ParrotVec4){v.x, v.y, v.z, 0};
}

ParrotVec4 ParrotVec4_add(ParrotVec4 a, ParrotVec4 b) {
    return (ParrotVec4){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

ParrotVec4 ParrotVec4_sub(ParrotVec4 a, ParrotVec4 b) {
    return (ParrotVec4){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

ParrotVec4 ParrotVec4_mul(ParrotVec4 a, ParrotVec4 b) {
    return (ParrotVec4){a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}

ParrotVec4 ParrotVec4_div(ParrotVec4 a, ParrotVec4 b) {
    return (ParrotVec4){a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
}

ParrotVec4 ParrotVec4_scale(ParrotVec4 a, ParrotReal b) {
    return (ParrotVec4){a.x * b, a.y * b, a.z * b, a.w * b};
}

ParrotVec4 ParrotVec4_normalize(ParrotVec4 self) {
    ParrotReal length = ParrotVec4_length(self);

    if (length == 0) {
        return ParrotVec4_n(0);
    }

    return (ParrotVec4){self.x / length, self.y / length, self.z / length, self.w / length};
}

ParrotReal ParrotVec4_length(ParrotVec4 self) {
    return ParrotReal_sqrt(self.x * self.x + self.y * self.y + self.z * self.z + self.w * self.w);
}

ParrotReal ParrotVec4_dot(ParrotVec4 self, ParrotVec4 other) {
    return self.x * other.x + self.y * other.y + self.z * other.z + self.w * other.w;
}

ParrotMat ParrotMat_identity(void) {
    ParrotMat result = {0};

    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            if (x == y) {
                result.data[x][y] = 1;
            }
        }
    }

    return result;
}

ParrotMat ParrotMat_inverse(ParrotMat matrix) {
    ParrotMat result = {0};

    float aug[4][8];
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            aug[y][x] = matrix.data[x][y];
            aug[y][x + 4] = (x == y) ? 1.0f : 0.0f;
        }
    }

    for (int col = 0; col < 4; col++) {
        int pivot = col;
        for (int row = col + 1; row < 4; row++) {
            if (fabsf(aug[row][col]) > fabsf(aug[pivot][col]))
                pivot = row;
        }

        if (pivot != col) {
            for (int k = 0; k < 8; k++) {
                float tmp = aug[col][k];
                aug[col][k] = aug[pivot][k];
                aug[pivot][k] = tmp;
            }
        }

        if (fabsf(aug[col][col]) < 1e-6f) {
            return result;
        }

        float scale = aug[col][col];
        for (int k = 0; k < 8; k++)
            aug[col][k] /= scale;

        for (int row = 0; row < 4; row++) {
            if (row == col) {
                continue;
            }
            float factor = aug[row][col];
            for (int k = 0; k < 8; k++) {
                aug[row][k] -= factor * aug[col][k];
            }
        }
    }

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            result.data[x][y] = aug[y][x + 4];
        }
    }

    return result;
}

ParrotMat ParrotMat_transpose(ParrotMat matrix) {
    ParrotMat result = {0};

    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            result.data[x][y] = matrix.data[y][x];
        }
    }

    return result;
}

ParrotMat ParrotMat_add(ParrotMat a, ParrotMat b) {
    ParrotMat result = {0};

    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            result.data[x][y] = a.data[x][y] + b.data[x][y];
        }
    }

    return result;
}

ParrotMat ParrotMat_mul(ParrotMat a, ParrotMat b) {
    ParrotMat result = {0};

    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            for (int z = 0; z < 4; z++) {
                result.data[x][y] += a.data[z][y] * b.data[x][z];
            }
        }
    }

    return result;
}

ParrotVec3 ParrotMat_transform3(ParrotMat matrix, ParrotVec3 vec) {
    float w = matrix.data[0][3] * vec.x + matrix.data[1][3] * vec.y + matrix.data[2][3] * vec.z + matrix.data[3][3];
    PARROT_RET_COND_V(w == 0, ParrotVec3_n(0));
    return (ParrotVec3){
        .x = (matrix.data[0][0] * vec.x + matrix.data[1][0] * vec.y + matrix.data[2][0] * vec.z + matrix.data[3][0]) / w,
        .y = (matrix.data[0][1] * vec.x + matrix.data[1][1] * vec.y + matrix.data[2][1] * vec.z + matrix.data[3][1]) / w,
        .z = (matrix.data[0][2] * vec.x + matrix.data[1][2] * vec.y + matrix.data[2][2] * vec.z + matrix.data[3][2]) / w,
    };
}

ParrotVec2 ParrotMat_transform2(ParrotMat matrix, ParrotVec2 vec) {
    ParrotVec3 result = ParrotMat_transform3(matrix, ParrotVec3_upgrade(vec));
    return (ParrotVec2){result.x, result.y};
}

ParrotMat
ParrotMat_ortho(ParrotReal left, ParrotReal right, ParrotReal bottom, ParrotReal top, ParrotReal near, ParrotReal far) {
    ParrotMat matrix = {0};

    ParrotReal width = right - left;
    ParrotReal height = top - bottom;
    ParrotReal total_distance = far - near;

    matrix.data[0][0] = 2.0 / width;

    matrix.data[1][1] = 2.0 / height;

    matrix.data[2][2] = -2.0 / total_distance;

    matrix.data[3][0] = -(right + left) / width;
    matrix.data[3][1] = -(top + bottom) / height;
    matrix.data[3][2] = -(far + near) / total_distance;
    matrix.data[3][3] = 1.0;

    return matrix;
}

ParrotMat ParrotMat_translation(ParrotVec3 position) {
    ParrotMat matrix = ParrotMat_identity();

    matrix.data[3][0] = position.x;
    matrix.data[3][1] = position.y;
    matrix.data[3][2] = position.z;

    return matrix;
}

ParrotMat ParrotMat_rotation(ParrotVec3 rotation) {
    ParrotMat matrix = ParrotMat_identity();

    ParrotReal sx = ParrotReal_sin(rotation.x), cx = ParrotReal_cos(rotation.x);
    ParrotReal sy = ParrotReal_sin(rotation.y), cy = ParrotReal_cos(rotation.y);
    ParrotReal sz = ParrotReal_sin(rotation.z), cz = ParrotReal_cos(rotation.z);

    matrix.data[0][0] = cz * cy;
    matrix.data[0][1] = sz * cy;
    matrix.data[0][2] = -sy;

    matrix.data[1][0] = cz * sy * sx - sz * cx;
    matrix.data[1][1] = sz * sy * sx + cz * cx;
    matrix.data[1][2] = cy * sx;

    matrix.data[2][0] = cz * sy * cx + sz * sx;
    matrix.data[2][1] = sz * sy * cx - cz * sx;
    matrix.data[2][2] = cy * cx;

    return matrix;
}

ParrotMat ParrotMat_scale(ParrotVec3 scale) {
    ParrotMat matrix = ParrotMat_identity();

    matrix.data[0][0] = scale.x;
    matrix.data[1][1] = scale.y;
    matrix.data[2][2] = scale.z;

    return matrix;
}

ParrotTransform ParrotTransform_new(void) {
    return (ParrotTransform){
        .matrix = ParrotMat_identity(),

        .scale = ParrotVec3_n(1),
    };
}

ParrotMat ParrotTransform_calculate_matrix(const ParrotTransform *self) {
    ParrotMat matrix = ParrotMat_identity();
    if (self->parent) {
        matrix = ParrotTransform_calculate_matrix(self->parent);
    }

    matrix = ParrotMat_mul(matrix, ParrotMat_translation(self->position));
    matrix = ParrotMat_mul(matrix, ParrotMat_rotation(self->rotation));
    matrix = ParrotMat_mul(matrix, ParrotMat_scale(self->scale));
    matrix = ParrotMat_mul(matrix, self->matrix);

    return matrix;
}

ParrotMat ParrotGMatSet_combine(const ParrotGMatSet *self) {
    return ParrotMat_mul(ParrotMat_mul(self->projection, self->view), self->model);
}

ParrotColor ParrotColor_new(uint8_t r, uint8_t g, uint8_t b) {
    return ParrotColor_newa(r, g, b, 255);
}

ParrotColor ParrotColor_newf(float r, float g, float b) {
    return ParrotColor_newaf(r, g, b, 1);
}

ParrotColor ParrotColor_newa(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ParrotColor_newaf((float)r / 255, (float)g / 255, (float)b / 255, (float)a / 255);
}

ParrotColor ParrotColor_newaf(float r, float g, float b, float a) {
    return (ParrotColor){
        .r = PARROT_CLAMP(0.0, r, 1.0),
        .g = PARROT_CLAMP(0.0, g, 1.0),
        .b = PARROT_CLAMP(0.0, b, 1.0),
        .a = PARROT_CLAMP(0.0, a, 1.0),
    };
}

ParrotColor ParrotColor_from(ParrotColorFormat format, uint32_t color) {
    switch (format) {
    case ParrotColorFormat_RGBA8888:
        return ParrotColor_newa(color >> 24, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    case ParrotColorFormat_BGRA8888:
        return ParrotColor_newa(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF, color >> 24);
    }
    return ParrotColor_CLEAR;
}

uint32_t ParrotColor_to(ParrotColor self, ParrotColorFormat format) {
    switch (format) {
    case ParrotColorFormat_RGBA8888: {
        uint8_t r = PARROT_CLAMP(0.0, self.r, 1.0) * 255;
        uint8_t g = PARROT_CLAMP(0.0, self.g, 1.0) * 255;
        uint8_t b = PARROT_CLAMP(0.0, self.b, 1.0) * 255;
        uint8_t a = PARROT_CLAMP(0.0, self.a, 1.0) * 255;
        return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    } break;
    case ParrotColorFormat_BGRA8888: {
        uint8_t r = PARROT_CLAMP(0.0, self.r, 1.0) * 255;
        uint8_t g = PARROT_CLAMP(0.0, self.g, 1.0) * 255;
        uint8_t b = PARROT_CLAMP(0.0, self.b, 1.0) * 255;
        uint8_t a = PARROT_CLAMP(0.0, self.a, 1.0) * 255;
        return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
    } break;
    }
    return 0;
}

ParrotColor ParrotColor_mul(ParrotColor a, ParrotColor b) {
    return ParrotColor_newaf(a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a);
}

ParrotColor ParrotColor_blend(ParrotColor a, ParrotColor b) {
    return ParrotColor_lerp(a, b, 0.5);
}

ParrotColor ParrotColor_lerp(ParrotColor a, ParrotColor b, float t) {
    return ParrotColor_newaf(
        Parrot_lerpf(a.r, b.r, t), Parrot_lerpf(a.g, b.g, t), Parrot_lerpf(a.b, b.b, t), Parrot_lerpf(a.a, b.a, t));
}
