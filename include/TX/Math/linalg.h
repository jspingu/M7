#ifndef LINALG_H
#define LINALG_H

#include <SDL3/SDL.h>

#define mat3x3_mul(lhs,rhs)  _Generic(rhs,  \
    vec3:   mat3x3_mulv,                    \
    mat3x3: mat3x3_mulm                     \
)(lhs, rhs)

#define xform3_apply(lhs,rhs)  _Generic(rhs,  \
    vec3:   xform3_applyv,                    \
    xform3: xform3_applyx                     \
)(lhs, rhs)

typedef union vec2 {
    float entries[2];
    struct {
        float x, y;
    };
} vec2;

typedef union vec3 {
    float entries[3];
    struct {
        float x, y, z;
    };
} vec3;

/* Column major */
typedef union mat3x3 {
    float entries[3][3];
    struct {
        vec3 x, y, z;
    };
} mat3x3;

typedef struct xform3 {
    mat3x3 basis;
    vec3 translation;
} xform3;

static const vec2 vec2_zero = {{ 0, 0 }};
static const vec2 vec2_i = {{ 1, 0 }};
static const vec2 vec2_j = {{ 0, 1 }};

static const vec3 vec3_zero = {{ 0, 0, 0 }};
static const vec3 vec3_i = {{ 1, 0, 0 }};
static const vec3 vec3_j = {{ 0, 1, 0 }};
static const vec3 vec3_k = {{ 0, 0, 1 }};

static const mat3x3 mat3x3_identity = {{
    { 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 }
}};

static inline vec2 vec2_add(vec2 lhs, vec2 rhs) {
    return (vec2) {
        .x = lhs.x + rhs.x,
        .y = lhs.y + rhs.y
    };
}

static inline vec3 vec3_add(vec3 lhs, vec3 rhs) {
    return (vec3) {
        .x = lhs.x + rhs.x,
        .y = lhs.y + rhs.y,
        .z = lhs.z + rhs.z
    };
}

static inline vec2 vec2_sub(vec2 lhs, vec2 rhs) {
    return (vec2) {
        .x = lhs.x - rhs.x,
        .y = lhs.y - rhs.y
    };
}

static inline vec3 vec3_sub(vec3 lhs, vec3 rhs) {
    return (vec3) {
        .x = lhs.x - rhs.x,
        .y = lhs.y - rhs.y,
        .z = lhs.z - rhs.z
    };
}

static inline vec2 vec2_mul(vec2 v, float f) {
    return (vec2) {
        .x = v.x * f,
        .y = v.y * f
    };
}

static inline vec3 vec3_mul(vec3 v, float f) {
    return (vec3) {
        .x = v.x * f,
        .y = v.y * f,
        .z = v.z * f
    };
}

static inline vec2 vec2_div(vec2 v, float f) {
    return (vec2) {
        .x = v.x / f,
        .y = v.y / f
    };
}

static inline vec3 vec3_div(vec3 v, float f) {
    return (vec3) {
        .x = v.x / f,
        .y = v.y / f,
        .z = v.z / f
    };
}

static inline float vec2_dot(vec2 lhs, vec2 rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

static inline float vec3_dot(vec3 lhs, vec3 rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

static inline vec2 vec2_orthogonal(vec2 v) {
    return (vec2) {
        .x = -v.y,
        .y = v.x
    };
}

static inline vec3 vec3_cross(vec3 lhs, vec3 rhs) {
    return (vec3) {
        .x = lhs.y * rhs.z - lhs.z * rhs.y,
        .y = lhs.z * rhs.x - lhs.x * rhs.z,
        .z = lhs.x * rhs.y - lhs.y * rhs.x
    };
}

static inline float vec2_length(vec2 v) {
    return SDL_sqrtf(vec2_dot(v, v));
}

static inline float vec3_length(vec3 v) {
    return SDL_sqrtf(vec3_dot(v, v));
}

static inline vec2 vec2_normalize(vec2 v) {
    return vec2_div(v, vec2_length(v));
}

static inline vec3 vec3_normalize(vec3 v) {
    return vec3_div(v, vec3_length(v));
}

static inline vec3 vec3_rotate(vec3 v, vec3 axis, float angle) {
    vec3 axis_projection = vec3_mul(axis, vec3_dot(v, axis));
    vec3 axis_rejection = vec3_sub(v, axis_projection);
    vec3 axis_rejection_rot = vec3_cross(axis, axis_rejection);

    return vec3_add(axis_projection, vec3_add(
        vec3_mul(axis_rejection, SDL_cosf(angle)),
        vec3_mul(axis_rejection_rot, SDL_sinf(angle))
    ));
}

static inline mat3x3 mat3x3_rotate(mat3x3 m, vec3 axis, float angle) {
    return (mat3x3) {
        .x = vec3_rotate(m.x, axis, angle),
        .y = vec3_rotate(m.y, axis, angle),
        .z = vec3_rotate(m.z, axis, angle)
    };
}

static inline vec3 mat3x3_mulv(mat3x3 m, vec3 v) {
    return (vec3) {
        .x = v.x * m.entries[0][0] + v.y * m.entries[1][0] + v.z * m.entries[2][0],
        .y = v.x * m.entries[0][1] + v.y * m.entries[1][1] + v.z * m.entries[2][1],
        .z = v.x * m.entries[0][2] + v.y * m.entries[1][2] + v.z * m.entries[2][2]
    };
}

static inline mat3x3 mat3x3_mulm(mat3x3 lhs, mat3x3 rhs) {
    return (mat3x3) {
        .x = mat3x3_mulv(lhs, rhs.x),
        .y = mat3x3_mulv(lhs, rhs.y),
        .z = mat3x3_mulv(lhs, rhs.z)
    };
}

static inline mat3x3 mat3x3_xpose(mat3x3 m) {
    return (mat3x3) {{
        { m.x.x, m.y.x, m.z.x },
        { m.x.y, m.y.y, m.z.y },
        { m.x.z, m.y.z, m.z.z }
    }};
}

static inline vec3 xform3_applyv(xform3 x, vec3 v) {
    return vec3_add(mat3x3_mul(x.basis, v), x.translation);
}

static inline xform3 xform3_applyx(xform3 lhs, xform3 rhs) {
    return (xform3) {
        .basis = mat3x3_mul(lhs.basis, rhs.basis),
        .translation = vec3_add(lhs.translation, mat3x3_mul(lhs.basis, rhs.translation))
    };
}

#endif /* LINALG_H */
