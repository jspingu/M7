/*
 * Strided data types and functions for generic vectorization
 * Currently supports SSE2, AVX2, and NEON
 * __AVX2__ implies that FMA is present
 */

#ifndef STRIDE_H
#define STRIDE_H

#include <SDL3/SDL.h>
#include <limits.h>

#if defined(__SSE2__) || defined(__AVX2__)
#include <immintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#define SD_DEFINE_TYPES(suffix,float_type,int_type)       \
    typedef union sd_int_##suffix {                       \
        int_type val;                                     \
        int elems[sizeof(int_type) / sizeof(int32_t)];    \
    } sd_int_##suffix;                                    \
                                                          \
    typedef union sd_float_##suffix {                     \
        float_type val;                                   \
        float elems[sizeof(float_type) / sizeof(float)];  \
    } sd_float_##suffix;                                  \
                                                          \
    typedef union sd_vec2_##suffix {                      \
        sd_float_##suffix xy[2];                          \
        struct {                                          \
            sd_float_##suffix x, y;                       \
        };                                                \
    } sd_vec2_##suffix;                                   \
                                                          \
    typedef union sd_vec3_##suffix {                      \
        sd_float_##suffix xyz[3];                         \
        sd_float_##suffix rgb[3];                         \
        sd_vec2_##suffix xy;                              \
        struct {                                          \
            sd_float_##suffix x, y, z;                    \
        };                                                \
        struct {                                          \
            sd_float_##suffix r, g, b;                    \
        };                                                \
    } sd_vec3_##suffix;                                   \
                                                          \
    typedef union sd_vec4_##suffix {                      \
        sd_float_##suffix xyzw[4];                        \
        sd_float_##suffix rgba[4];                        \
        sd_vec2_##suffix xy;                              \
        sd_vec3_##suffix xyz;                             \
        sd_vec3_##suffix rgb;                             \
        struct {                                          \
            sd_float_##suffix x, y, z, w;                 \
        };                                                \
        struct {                                          \
            sd_float_##suffix r, g, b, a;                 \
        };                                                \
    } sd_vec4_##suffix;

#define SD_TYPEDEFS(suffix)                    \
    typedef union sd_int_##suffix sd_int;       \
    typedef union sd_float_##suffix sd_float;  \
    typedef union sd_vec2_##suffix sd_vec2;    \
    typedef union sd_vec3_##suffix sd_vec3;    \
    typedef union sd_vec4_##suffix sd_vec4;

SD_DEFINE_TYPES(scalar, float, int32_t) // NOLINT(bugprone-sizeof-expression)

#ifdef __AVX2__
    #define SD_VARIANT(fnname)  fnname##_avx2
    SD_DEFINE_TYPES(avx2, __m256, __m256i)
    SD_TYPEDEFS(avx2)
#elifdef __SSE2__
    #define SD_VARIANT(fnname)  fnname##_sse2
    SD_DEFINE_TYPES(sse2, __m128, __m128i)
    SD_TYPEDEFS(sse2)
#elifdef __ARM_NEON
    #define SD_VARIANT(fnname)  fnname##_neon
    SD_DEFINE_TYPES(neon, float32x4_t, int32x4_t)
    SD_TYPEDEFS(neon)
#else
    #define SD_VARIANT(fnname)  fnname##_scalar
    SD_TYPEDEFS(scalar)
#endif

#define SD_PARAMS(...)                 __VA_OPT__(SD_PARAM1(__VA_ARGS__))
#define SD_PARAM1(type,name,...)       typeof(type) name __VA_OPT__(, SD_PARAM2(__VA_ARGS__))
#define SD_PARAM2(type,name,...)       typeof(type) name __VA_OPT__(, SD_PARAM3(__VA_ARGS__))
#define SD_PARAM3(type,name,...)       typeof(type) name __VA_OPT__(, SD_PARAM4(__VA_ARGS__))
#define SD_PARAM4(type,name,...)       typeof(type) name __VA_OPT__(, SD_PARAM5(__VA_ARGS__))
#define SD_PARAM5(type,name,...)       typeof(type) name __VA_OPT__(, SD_PARAM6(__VA_ARGS__))
#define SD_PARAM6(type,name,...)       typeof(type) name __VA_OPT__(, SD_PARAM7(__VA_ARGS__))
#define SD_PARAM7(type,name,...)       typeof(type) name __VA_OPT__(, SD_PARAM8(__VA_ARGS__))
#define SD_PARAM8(type,name,...)       typeof(type) name

#define SD_PARAM_NAMES(...)            __VA_OPT__(SD_PARAM_NAME1(__VA_ARGS__))
#define SD_PARAM_NAME1(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME2(__VA_ARGS__))
#define SD_PARAM_NAME2(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME3(__VA_ARGS__))
#define SD_PARAM_NAME3(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME4(__VA_ARGS__))
#define SD_PARAM_NAME4(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME5(__VA_ARGS__))
#define SD_PARAM_NAME5(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME6(__VA_ARGS__))
#define SD_PARAM_NAME6(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME7(__VA_ARGS__))
#define SD_PARAM_NAME7(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME8(__VA_ARGS__))
#define SD_PARAM_NAME8(type,name,...)  name

#ifdef SD_DISPATCH_STATIC

#ifdef __AVX2__
#define SD_SELECT(fnname)  ( fnname##_avx2 )
#elifdef __SSE2__
#define SD_SELECT(fnname)  ( fnname##_sse2 )
#elifdef __ARM_NEON
#define SD_SELECT(fnname)  ( fnname##_neon )
#else
#define SD_SELECT(fnname)  ( fnname##_scalar )
#endif

#elifdef SD_DISPATCH_DYNAMIC

#ifdef __x86_64__
#ifdef __AVX2__
#define SD_SELECT(fnname)  ( fnname##_avx2 )
#else
#define SD_SELECT(fnname)  ( SDL_HasAVX2() ? fnname##_avx2 : fnname##_sse2 )
#endif
#endif /* __x86_64__ */

#ifdef __i386__
#ifdef __SSE2__
#define SD_SELECT(fnname)  ( fnname##_sse2 )
#else
#define SD_SELECT(fnname)  ( SDL_HasSSE2() ? fnname##_sse2 : fnname##_scalar )
#endif
#endif /* __i386__ */

#ifdef __aarch64__
#define SD_SELECT(fnname)  ( fnname##_neon )
#endif /* __aarch64__ */

#ifdef __arm__
#ifdef __ARM_NEON
#define SD_SELECT(fnname)  ( fnname##_neon )
#else
#define SD_SELECT(fnname)  ( SDL_HasNEON() ? fnname##_neon : fnname##_scalar )
#endif
#endif /* __arm__ */

#ifndef SD_SELECT
#define SD_SELECT(fnname)  ( fnname##_scalar )
#endif /* Unknown architecture */

#endif /* SD_DISPATCH */

#define SD_LENGTH  ( sizeof(sd_float) / sizeof(float) )
#define SD_ALIGN   ( alignof(sd_float) )

#define SD_DECLARE(rettype,fnname,...)                              \
    typeof(rettype) fnname##_avx2(SD_PARAMS(__VA_ARGS__));          \
    typeof(rettype) fnname##_sse2(SD_PARAMS(__VA_ARGS__));          \
    typeof(rettype) fnname##_neon(SD_PARAMS(__VA_ARGS__));          \
    typeof(rettype) fnname##_scalar(SD_PARAMS(__VA_ARGS__));        \
    static inline typeof(rettype) fnname(SD_PARAMS(__VA_ARGS__)) {  \
        typeof(&fnname) sd_fn = SD_SELECT(fnname);                  \
        return sd_fn(SD_PARAM_NAMES(__VA_ARGS__));                  \
    }

#define SD_DECLARE_VOID_RETURN(fnname,...)               \
    void fnname##_avx2(SD_PARAMS(__VA_ARGS__));          \
    void fnname##_sse2(SD_PARAMS(__VA_ARGS__));          \
    void fnname##_neon(SD_PARAMS(__VA_ARGS__));          \
    void fnname##_scalar(SD_PARAMS(__VA_ARGS__));        \
    static inline void fnname(SD_PARAMS(__VA_ARGS__)) {  \
        typeof(&fnname) sd_fn = SD_SELECT(fnname);       \
        sd_fn(SD_PARAM_NAMES(__VA_ARGS__));              \
    }

#define SD_DEFINE_VECFNS_BINARY_VV(base)                              \
    static inline sd_vec2 sd_vec2_##base(sd_vec2 lhs, sd_vec2 rhs) {  \
        return (sd_vec2) {                                            \
            .x = sd_float_##base(lhs.x, rhs.x),                       \
            .y = sd_float_##base(lhs.y, rhs.y)                        \
        };                                                            \
    }                                                                 \
                                                                      \
    static inline sd_vec3 sd_vec3_##base(sd_vec3 lhs, sd_vec3 rhs) {  \
        return (sd_vec3) {                                            \
            .x = sd_float_##base(lhs.x, rhs.x),                       \
            .y = sd_float_##base(lhs.y, rhs.y),                       \
            .z = sd_float_##base(lhs.z, rhs.z)                        \
        };                                                            \
    }                                                                 \
                                                                      \
    static inline sd_vec4 sd_vec4_##base(sd_vec4 lhs, sd_vec4 rhs) {  \
        return (sd_vec4) {                                            \
            .x = sd_float_##base(lhs.x, rhs.x),                       \
            .y = sd_float_##base(lhs.y, rhs.y),                       \
            .z = sd_float_##base(lhs.z, rhs.z),                       \
            .w = sd_float_##base(lhs.w, rhs.w)                        \
        };                                                            \
    }

#define SD_DEFINE_VECFNS_BINARY_VS(base)                       \
    static inline sd_vec2 sd_vec2_##base(sd_vec2 lhs, sd_float rhs) {  \
        return (sd_vec2) {                                             \
            .x = sd_float_##base(lhs.x, rhs),                          \
            .y = sd_float_##base(lhs.y, rhs)                           \
        };                                                             \
    }                                                                  \
                                                                       \
    static inline sd_vec3 sd_vec3_##base(sd_vec3 lhs, sd_float rhs) {  \
        return (sd_vec3) {                                             \
            .x = sd_float_##base(lhs.x, rhs),                          \
            .y = sd_float_##base(lhs.y, rhs),                          \
            .z = sd_float_##base(lhs.z, rhs)                           \
        };                                                             \
    }                                                                  \
                                                                       \
    static inline sd_vec4 sd_vec4_##base(sd_vec4 lhs, sd_float rhs) {  \
        return (sd_vec4) {                                             \
            .x = sd_float_##base(lhs.x, rhs),                          \
            .y = sd_float_##base(lhs.y, rhs),                          \
            .z = sd_float_##base(lhs.z, rhs),                          \
            .w = sd_float_##base(lhs.w, rhs)                           \
        };                                                             \
    }

#define SD_DEFINE_VECFNS_UNARY(base)                   \
    static inline sd_vec2 sd_vec2_##base(sd_vec2 v) {  \
        return (sd_vec2) {                             \
            .x = sd_float_##base(v.x),                 \
            .y = sd_float_##base(v.y)                  \
        };                                             \
    }                                                  \
                                                       \
    static inline sd_vec3 sd_vec3_##base(sd_vec3 v) {  \
        return (sd_vec3) {                             \
            .x = sd_float_##base(v.x),                 \
            .y = sd_float_##base(v.y),                 \
            .z = sd_float_##base(v.z)                  \
        };                                             \
    }                                                  \
                                                       \
    static inline sd_vec4 sd_vec4_##base(sd_vec4 v) {  \
        return (sd_vec4) {                             \
            .x = sd_float_##base(v.x),                 \
            .y = sd_float_##base(v.y),                 \
            .z = sd_float_##base(v.z),                 \
            .w = sd_float_##base(v.w)                  \
        };                                             \
    }

#define SD_DEFINE_VECFNS_NULLARY(base)              \
    static inline sd_vec2 sd_vec2_##base(void) {    \
        return (sd_vec2) {                          \
            .x = sd_float_##base(),                 \
            .y = sd_float_##base()                  \
        };                                          \
    }                                               \
                                                    \
    static inline sd_vec3 sd_vec3_##base(void) {    \
        return (sd_vec3) {                          \
            .x = sd_float_##base(),                 \
            .y = sd_float_##base(),                 \
            .z = sd_float_##base()                  \
        };                                          \
    }                                               \
                                                    \
    static inline sd_vec4 sd_vec4_##base(void) {    \
        return (sd_vec4) {                          \
            .x = sd_float_##base(),                 \
            .y = sd_float_##base(),                 \
            .z = sd_float_##base(),                 \
            .w = sd_float_##base()                  \
        };                                          \
    }

static inline size_t sd_bounding_size(size_t n) {
    return n ? (n - 1) / SD_LENGTH + 1 : 0;
}

static inline sd_int sd_int_add(sd_int lhs, sd_int rhs) {
#ifdef __AVX2__
    return (sd_int){_mm256_add_epi32(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_int){_mm_add_epi32(lhs.val, rhs.val)};
#elifdef __ARM_NEON
    return (sd_int){vaddq_s32(lhs.val, rhs.val)};
#else
    return (sd_int){lhs.val + rhs.val};
#endif
}

static inline sd_int sd_int_sub(sd_int lhs, sd_int rhs) {
#ifdef __AVX2__
    return (sd_int){_mm256_sub_epi32(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_int){_mm_sub_epi32(lhs.val, rhs.val)};
#elifdef __ARM_NEON
    return (sd_int){vsubq_s32(lhs.val, rhs.val)};
#else
    return (sd_int){lhs.val - rhs.val};
#endif
}

static inline sd_int sd_int_mul(sd_int lhs, sd_int rhs) {
#ifdef __AVX2__
    return (sd_int){_mm256_mullo_epi32(lhs.val, rhs.val)};
#elifdef __SSE2__
    __m128i lhs31 = _mm_shuffle_epi32(lhs.val, 0b00'11'00'01);
    __m128i rhs31 = _mm_shuffle_epi32(rhs.val, 0b00'11'00'01);

    __m128i mul20 = _mm_mul_epi32(lhs.val, rhs.val);
            mul20 = _mm_shuffle_epi32(mul20, 0b00'00'10'00);
    __m128i mul31 = _mm_mul_epi32(lhs31, rhs31);
            mul31 = _mm_shuffle_epi32(mul31, 0b00'00'10'00);

    return (sd_int){_mm_unpacklo_epi32(mul20, mul31)};
#elifdef __ARM_NEON
    return (sd_int){vmulq_s32(lhs.val, rhs.val)};
#else
    return (sd_int){lhs.val * rhs.val};
#endif
}

static inline sd_int sd_int_and(sd_int lhs, sd_int rhs) {
#ifdef __AVX2__
    return (sd_int){_mm256_and_si256(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_int){_mm_and_si128(lhs.val, rhs.val)};
#elifdef __ARM_NEON
    return (sd_int){vandq_s32(lhs.val, rhs.val)};
#else
    return (sd_int){lhs.val & rhs.val};
#endif
}

static inline sd_int sd_int_or(sd_int lhs, sd_int rhs) {
#ifdef __AVX2__
    return (sd_int){_mm256_or_si256(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_int){_mm_or_si128(lhs.val, rhs.val)};
#elifdef __ARM_NEON
    return (sd_int){vorrq_s32(lhs.val, rhs.val)};
#else
    return (sd_int){lhs.val | rhs.val};
#endif
}

static inline sd_int sd_int_shl(sd_int i, int shift) {
#ifdef __AVX2__
    return (sd_int){_mm256_slli_epi32(i.val, shift)};
#elifdef __SSE2__
    return (sd_int){_mm_slli_epi32(i.val, shift)};
#elifdef __ARM_NEON
    int32x4_t shiftv = vdupq_n_s32(shift);
    return (sd_int){vshlq_s32(i.val, shiftv)};
#else
    return (sd_int){i.val << shift};
#endif
}

static inline sd_int sd_int_shr(sd_int i, int shift) {
#ifdef __AVX2__
    return (sd_int){_mm256_srai_epi32(i.val, shift)};
#elifdef __SSE2__
    return (sd_int){_mm_srai_epi32(i.val, shift)};
#elifdef __ARM_NEON
    int32x4_t shiftv = vdupq_n_s32(-shift);
    return (sd_int){vshlq_s32(i.val, shiftv)};
#else
    return (sd_int){i.val >> shift};
#endif
}

static inline sd_int sd_int_set(int32_t i) {
#ifdef __AVX2__
    return (sd_int){_mm256_set1_epi32(i)};
#elifdef __SSE2__
    return (sd_int){_mm_set1_epi32(i)};
#elifdef __ARM_NEON
    return (sd_int){vdupq_n_s32(i)};
#else
    return (sd_int){i};
#endif
}

static inline sd_int sd_int_gather_i8(int8_t *buf, sd_int index) {
#ifdef __AVX2__
    return (sd_int){_mm256_i32gather_epi32((int const *)buf, index.val, 1)};
#elifdef __SSE2__
    alignas(SD_ALIGN) int32_t elems[4], idxs[4];
    SDL_memcpy(idxs, &index, sizeof(sd_int));

    for (int i = 0; i < 4; ++i)
        elems[i] = buf[idxs[i]];

    sd_int out;
    SDL_memcpy(&out, elems, sizeof(sd_int));
    return out;
#elifdef __ARM_NEON
    return (sd_int){{buf[index.val[0]], buf[index.val[1]], buf[index.val[2]], buf[index.val[3]]}};
#else
    return (sd_int){buf[index.val]};
#endif
}

static inline void sd_int_store_unaligned(int32_t *dst, sd_int i) {
#ifdef __AVX2__
    _mm256_storeu_si256((__m256i *)dst, i.val);
#elifdef __SSE2__
    _mm_storeu_si128((__m128i *)dst, i.val);
#elifdef __ARM_NEON
    *((int32x4_t *)dst) = i.val;
#else
    *dst = i.val;
#endif
}

static inline sd_int sd_float_to_int(sd_float f) {
#ifdef __AVX2__
    return (sd_int){_mm256_cvtps_epi32(f.val)};
#elifdef __SSE2__
    return (sd_int){_mm_cvtps_epi32(f.val)};
#elifdef __ARM_NEON
    return (sd_int){vcvtq_s32_f32(f.val)};
#else
    return (sd_int){(int32_t)f.val};
#endif
}

static inline sd_float sd_int_to_float(sd_int i) {
#ifdef __AVX2__
    return (sd_float){_mm256_cvtepi32_ps(i.val)};
#elifdef __SSE2__
    return (sd_float){_mm_cvtepi32_ps(i.val)};
#elifdef __ARM_NEON
    return (sd_float){vcvtq_f32_s32(i.val)};
#else
    return (sd_float){(float)i.val};
#endif
}

static inline sd_float sd_float_add(sd_float lhs, sd_float rhs) {
#ifdef __AVX2__
    return (sd_float){_mm256_add_ps(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_float){_mm_add_ps(lhs.val, rhs.val)};
#elifdef __ARM_NEON
    return (sd_float){vaddq_f32(lhs.val, rhs.val)};
#else
    return (sd_float){lhs.val + rhs.val};
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(add)

static inline sd_float sd_float_sub(sd_float lhs, sd_float rhs) {
#ifdef __AVX2__
    return (sd_float){_mm256_sub_ps(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_float){_mm_sub_ps(lhs.val, rhs.val)};
#elifdef __ARM_NEON
    return (sd_float){vsubq_f32(lhs.val, rhs.val)};
#else
    return (sd_float){lhs.val - rhs.val};
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(sub)

static inline sd_float sd_float_mul(sd_float lhs, sd_float rhs) {
#ifdef __AVX2__
    return (sd_float){_mm256_mul_ps(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_float){_mm_mul_ps(lhs.val, rhs.val)};
#elifdef __ARM_NEON
    return (sd_float){vmulq_f32(lhs.val, rhs.val)};
#else
    return (sd_float){lhs.val * rhs.val};
#endif
}

SD_DEFINE_VECFNS_BINARY_VS(mul)

static inline sd_float sd_float_abs(sd_float f) {
#ifdef __AVX2__
    __m256i minus_one = _mm256_set1_epi32(-1);
    __m256 abs_mask = _mm256_castsi256_ps(_mm256_srli_epi32(minus_one, 1));
    return (sd_float){_mm256_and_ps(f.val, abs_mask)};
#elifdef __SSE2__
    __m128i minus_one = _mm_set1_epi32(-1);
    __m128 abs_mask = _mm_castsi128_ps(_mm_srli_epi32(minus_one, 1));
    return (sd_float){_mm_and_ps(f.val, abs_mask)};
#elifdef __ARM_NEON
    return (sd_float){vabsq_f32(f.val)};
#else
    return (sd_float){SDL_fabsf(f.val)};
#endif
}

SD_DEFINE_VECFNS_UNARY(abs)

static inline sd_float sd_float_negate(sd_float f) {
#ifdef __AVX2__
    return (sd_float){_mm256_mul_ps(f.val, _mm256_set1_ps(-1))};
#elifdef __SSE2__
    return (sd_float){_mm_mul_ps(f.val, _mm_set1_ps(-1))};
#elifdef __ARM_NEON
    return (sd_float){vnegq_f32(f.val)};
#else
    return (sd_float){-f.val};
#endif
}

SD_DEFINE_VECFNS_UNARY(negate)

static inline sd_float sd_float_rcp(sd_float f) {
#ifdef __AVX2__
    return (sd_float){_mm256_rcp_ps(f.val)};
#elifdef __SSE2__
    return (sd_float){_mm_rcp_ps(f.val)};
#elifdef __ARM_NEON
    return (sd_float){vrecpeq_f32(f.val)};
#else
    return (sd_float){1 / f.val};
#endif
}

SD_DEFINE_VECFNS_UNARY(rcp)

static inline sd_float sd_float_fmadd(sd_float multiplicand, sd_float multiplier, sd_float addend) {
#ifdef __AVX2__
    return (sd_float){_mm256_fmadd_ps(multiplicand.val, multiplier.val, addend.val)};
#elifdef __ARM_NEON
    return (sd_float){vmlaq_f32(addend.val, multiplicand.val, multiplier.val)};
#else
    return sd_float_add(sd_float_mul(multiplicand, multiplier), addend);
#endif
}

static inline sd_vec2 sd_vec2_fmadd(sd_vec2 multiplicand, sd_float multiplier, sd_vec2 addend) {
    return (sd_vec2) {
        .x = sd_float_fmadd(multiplicand.x, multiplier, addend.x),
        .y = sd_float_fmadd(multiplicand.y, multiplier, addend.y)
    };
}

static inline sd_vec3 sd_vec3_fmadd(sd_vec3 multiplicand, sd_float multiplier, sd_vec3 addend) {
    return (sd_vec3) {
        .x = sd_float_fmadd(multiplicand.x, multiplier, addend.x),
        .y = sd_float_fmadd(multiplicand.y, multiplier, addend.y),
        .z = sd_float_fmadd(multiplicand.z, multiplier, addend.z)
    };
}

static inline sd_vec4 sd_vec4_fmadd(sd_vec4 multiplicand, sd_float multiplier, sd_vec4 addend) {
    return (sd_vec4) {
        .x = sd_float_fmadd(multiplicand.x, multiplier, addend.x),
        .y = sd_float_fmadd(multiplicand.y, multiplier, addend.y),
        .z = sd_float_fmadd(multiplicand.z, multiplier, addend.z),
        .w = sd_float_fmadd(multiplicand.w, multiplier, addend.w)
    };
}

static inline sd_float sd_float_fmsub(sd_float multiplicand, sd_float multiplier, sd_float subtrahend) {
#ifdef __AVX2__
    return (sd_float){_mm256_fmsub_ps(multiplicand.val, multiplier.val, subtrahend.val)};
#elifdef __ARM_NEON
    return (sd_float){vmlaq_f32(vnegq_f32(subtrahend.val), multiplicand.val, multiplier.val)};
#else
    return sd_float_sub(sd_float_mul(multiplicand, multiplier), subtrahend);
#endif
}

static inline sd_float sd_float_rsqrt(sd_float f) {
#ifdef __AVX2__
    return (sd_float){_mm256_rsqrt_ps(f.val)};
#elifdef __SSE2__
    return (sd_float){_mm_rsqrt_ps(f.val)};
#elifdef __ARM_NEON
    return (sd_float){vrsqrteq_f32(f.val)};
#else
    return (sd_float){1 / SDL_sqrtf(f.val)};
#endif
}

SD_DEFINE_VECFNS_UNARY(rsqrt)

static inline sd_float sd_float_min(sd_float f, sd_float min) {
#ifdef __AVX2__
    return (sd_float){_mm256_min_ps(f.val, min.val)};
#elifdef __SSE2__
    return (sd_float){_mm_min_ps(f.val, min.val)};
#elifdef __ARM_NEON
    return (sd_float){vminq_f32(f.val, min.val)};
#else
    return (sd_float){SDL_min(f.val, min.val)};
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(min)

static inline sd_float sd_float_max(sd_float f, sd_float max) {
#ifdef __AVX2__
    return (sd_float){_mm256_max_ps(f.val, max.val)};
#elifdef __SSE2__
    return (sd_float){_mm_max_ps(f.val, max.val)};
#elifdef __ARM_NEON
    return (sd_float){vmaxq_f32(f.val, max.val)};
#else
    return (sd_float){SDL_max(f.val, max.val)};
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(max)

static inline sd_float sd_float_clamp(sd_float f, sd_float min, sd_float max) {
    return sd_float_min(sd_float_max(f, min), max);
}

static inline sd_vec2 sd_vec2_clamp(sd_vec2 v, sd_float min, sd_float max) {
    return (sd_vec2) {
        .x = sd_float_clamp(v.x, min, max),
        .y = sd_float_clamp(v.y, min, max)
    };
}

static inline sd_vec3 sd_vec3_clamp(sd_vec3 v, sd_float min, sd_float max) {
    return (sd_vec3) {
        .x = sd_float_clamp(v.x, min, max),
        .y = sd_float_clamp(v.y, min, max),
        .z = sd_float_clamp(v.z, min, max)
    };
}

static inline sd_vec4 sd_vec4_clamp(sd_vec4 v, sd_float min, sd_float max) {
    return (sd_vec4) {
        .x = sd_float_clamp(v.x, min, max),
        .y = sd_float_clamp(v.y, min, max),
        .z = sd_float_clamp(v.z, min, max),
        .w = sd_float_clamp(v.w, min, max)
    };
}

static inline sd_int sd_float_lt(sd_float lhs, sd_float rhs) {
#ifdef __AVX2__
    return (sd_int){_mm256_castps_si256(_mm256_cmp_ps(lhs.val, rhs.val, _CMP_LT_OQ))};
#elifdef __SSE2__
    return (sd_int){_mm_castps_si128(_mm_cmplt_ps(lhs.val, rhs.val))};
#elifdef __ARM_NEON
    return (sd_int){vreinterpretq_s32_u32(vcltq_f32(lhs.val, rhs.val))};
#else
    return (sd_int){lhs.val < rhs.val ? ~0 : 0};
#endif
}

static inline sd_int sd_float_gt(sd_float lhs, sd_float rhs) {
#ifdef __AVX2__
    return (sd_int){_mm256_castps_si256(_mm256_cmp_ps(lhs.val, rhs.val, _CMP_GT_OQ))};
#elifdef __SSE2__
    return (sd_int){_mm_castps_si128(_mm_cmpgt_ps(lhs.val, rhs.val))};
#elifdef __ARM_NEON
    return (sd_int){vreinterpretq_s32_u32(vcgtq_f32(lhs.val, rhs.val))};
#else
    return (sd_int){lhs.val > rhs.val ? ~0 : 0};
#endif
}

static inline sd_int sd_float_clamp_mask(sd_float f, float min, float max) {
#ifdef __AVX2__
    __m256 mask_gt = _mm256_cmp_ps(f.val, _mm256_set1_ps(min), _CMP_GT_OQ);
    __m256 mask_lt = _mm256_cmp_ps(f.val, _mm256_set1_ps(max), _CMP_LT_OQ);
    return (sd_int){_mm256_castps_si256(_mm256_and_ps(mask_gt, mask_lt))};
#elifdef __SSE2__
    __m128 mask_gt = _mm_cmpgt_ps(f.val, _mm_set1_ps(min));
    __m128 mask_lt = _mm_cmplt_ps(f.val, _mm_set1_ps(max));
    return (sd_int){_mm_castps_si128(_mm_and_ps(mask_gt, mask_lt))};
#elifdef __ARM_NEON
    uint32x4_t mask_gt = vcgtq_f32(f.val, vdupq_n_f32(min));
    uint32x4_t mask_lt = vcltq_f32(f.val, vdupq_n_f32(max));
    return (sd_int){vreinterpretq_s32_u32(vandq_u32(mask_gt, mask_lt))};
#else
    return (sd_int){f.val > min && f.val < max ? ~0 : 0};
#endif
}

static inline sd_float sd_float_mask_blend(sd_float bg, sd_float fg, sd_int mask) {
#ifdef __AVX2__
    return (sd_float){_mm256_blendv_ps(bg.val, fg.val, _mm256_castsi256_ps(mask.val))};
#elifdef __SSE2__
    __m128 select_bg = _mm_andnot_ps(_mm_castsi128_ps(mask.val), bg.val);
    __m128 select_fg = _mm_and_ps(_mm_castsi128_ps(mask.val), fg.val);
    return (sd_float){_mm_or_ps(select_bg, select_fg)};
#elifdef __ARM_NEON
    return (sd_float){vbslq_f32(vreinterpretq_u32_s32(mask.val), fg.val, bg.val)};
#else
    uint32_t bg_i, fg_i;
    SDL_memcpy(&bg_i, &bg, sizeof(float));
    SDL_memcpy(&fg_i, &fg, sizeof(float));

    sd_float out;
    uint32_t res = (bg_i & ~mask.val) | (fg_i & mask.val);
    SDL_memcpy(&out, &res, sizeof(float));
    
    return out;
#endif
}

static inline sd_vec2 sd_vec2_mask_blend(sd_vec2 bg, sd_vec2 fg, sd_int mask) {
    return (sd_vec2) {
        .x = sd_float_mask_blend(bg.x, fg.x, mask),
        .y = sd_float_mask_blend(bg.y, fg.y, mask)
    };
}

static inline sd_vec3 sd_vec3_mask_blend(sd_vec3 bg, sd_vec3 fg, sd_int mask) {
    return (sd_vec3) {
        .x = sd_float_mask_blend(bg.x, fg.x, mask),
        .y = sd_float_mask_blend(bg.y, fg.y, mask),
        .z = sd_float_mask_blend(bg.z, fg.z, mask)
    };
}

static inline sd_vec4 sd_vec4_mask_blend(sd_vec4 bg, sd_vec4 fg, sd_int mask) {
    return (sd_vec4) {
        .x = sd_float_mask_blend(bg.x, fg.x, mask),
        .y = sd_float_mask_blend(bg.y, fg.y, mask),
        .z = sd_float_mask_blend(bg.z, fg.z, mask),
        .w = sd_float_mask_blend(bg.w, fg.w, mask)
    };
}

static inline sd_float sd_float_range(void) {
#ifdef __AVX2__
    return (sd_float){_mm256_set_ps(7, 6, 5, 4, 3, 2, 1, 0)};
#elifdef __SSE2__
    return (sd_float){_mm_set_ps(3, 2, 1, 0)};
#elifdef __ARM_NEON
    return (sd_float){{0,1,2,3}};
#else
    return (sd_float){0};
#endif
}

SD_DEFINE_VECFNS_NULLARY(range)

static inline sd_float sd_float_zero(void) {
#ifdef __AVX2__
    return (sd_float){_mm256_setzero_ps()};
#elifdef __SSE2__
    return (sd_float){_mm_setzero_ps()};
#elifdef __ARM_NEON
    return (sd_float){vdupq_n_f32(0)};
#else
    return (sd_float){0};
#endif
}

SD_DEFINE_VECFNS_NULLARY(zero)

static inline sd_float sd_float_one(void) {
#ifdef __AVX2__
    return (sd_float){_mm256_set1_ps(1)};
#elifdef __SSE2__
    return (sd_float){_mm_set1_ps(1)};
#elifdef __ARM_NEON
    return (sd_float){vdupq_n_f32(1)};
#else
    return (sd_float){1};
#endif
}

SD_DEFINE_VECFNS_NULLARY(one)

static inline sd_float sd_float_set(float f) {
#ifdef __AVX2__
    return (sd_float){_mm256_set1_ps(f)};
#elifdef __SSE2__
    return (sd_float){_mm_set1_ps(f)};
#elifdef __ARM_NEON
    return (sd_float){vdupq_n_f32(f)};
#else
    return (sd_float){f};
#endif
}

static inline sd_vec2 sd_vec2_set(float x, float y) {
    return (sd_vec2) {
        .x = sd_float_set(x),
        .y = sd_float_set(y)
    };
}

static inline sd_vec3 sd_vec3_set(float x, float y, float z) {
    return (sd_vec3) {
        .x = sd_float_set(x),
        .y = sd_float_set(y),
        .z = sd_float_set(z)
    };
}

static inline sd_vec4 sd_vec4_set(float x, float y, float z, float w) {
    return (sd_vec4) {
        .x = sd_float_set(x),
        .y = sd_float_set(y),
        .z = sd_float_set(z),
        .w = sd_float_set(w)
    };
}

static inline sd_float_scalar sd_float_arr_get(sd_float *arr, size_t index) {
    return (sd_float_scalar){arr[index / SD_LENGTH].elems[index % SD_LENGTH]};
}

static inline sd_vec2_scalar sd_vec2_arr_get(sd_vec2 *arr, size_t index) {
    return (sd_vec2_scalar) {
        .x = {arr[index / SD_LENGTH].x.elems[index % SD_LENGTH]},
        .y = {arr[index / SD_LENGTH].y.elems[index % SD_LENGTH]}
    };
}

static inline sd_vec3_scalar sd_vec3_arr_get(sd_vec3 *arr, size_t index) {
    return (sd_vec3_scalar) {
        .x = {arr[index / SD_LENGTH].x.elems[index % SD_LENGTH]},
        .y = {arr[index / SD_LENGTH].y.elems[index % SD_LENGTH]},
        .z = {arr[index / SD_LENGTH].z.elems[index % SD_LENGTH]}
    };
}

static inline sd_vec4_scalar sd_vec4_arr_get(sd_vec4 *arr, size_t index) {
    return (sd_vec4_scalar) {
        .x = {arr[index / SD_LENGTH].x.elems[index % SD_LENGTH]},
        .y = {arr[index / SD_LENGTH].y.elems[index % SD_LENGTH]},
        .z = {arr[index / SD_LENGTH].z.elems[index % SD_LENGTH]},
        .w = {arr[index / SD_LENGTH].w.elems[index % SD_LENGTH]}
    };
}

static inline void sd_float_arr_set(sd_float *arr, size_t index, float x) {
    arr[index / SD_LENGTH].elems[index % SD_LENGTH] = x;
}

static inline void sd_vec2_arr_set(sd_vec2 *arr, size_t index, float x, float y) {
    arr[index / SD_LENGTH].x.elems[index % SD_LENGTH] = x;
    arr[index / SD_LENGTH].y.elems[index % SD_LENGTH] = y;
}

static inline void sd_vec3_arr_set(sd_vec3 *arr, size_t index, float x, float y, float z) {
    arr[index / SD_LENGTH].x.elems[index % SD_LENGTH] = x;
    arr[index / SD_LENGTH].y.elems[index % SD_LENGTH] = y;
    arr[index / SD_LENGTH].z.elems[index % SD_LENGTH] = z;
}

static inline void sd_vec4_arr_set(sd_vec4 *arr, size_t index, float x, float y, float z, float w) {
    arr[index / SD_LENGTH].x.elems[index % SD_LENGTH] = x;
    arr[index / SD_LENGTH].y.elems[index % SD_LENGTH] = y;
    arr[index / SD_LENGTH].z.elems[index % SD_LENGTH] = z;
    arr[index / SD_LENGTH].w.elems[index % SD_LENGTH] = w;
}

static inline sd_float sd_vec2_dot(sd_vec2 lhs, sd_vec2 rhs) {
    sd_float out = sd_float_mul(lhs.x, rhs.x);
    return sd_float_fmadd(lhs.y, rhs.y, out);
}

static inline sd_float sd_vec3_dot(sd_vec3 lhs, sd_vec3 rhs) {
    sd_float out = sd_float_mul(lhs.x, rhs.x);
    out = sd_float_fmadd(lhs.y, rhs.y, out);
    return sd_float_fmadd(lhs.z, rhs.z, out);
}

static inline sd_vec3 sd_vec3_cross(sd_vec3 lhs, sd_vec3 rhs) {
    return (sd_vec3) {
        .x = sd_float_fmsub(lhs.y, rhs.z, sd_float_mul(lhs.z, rhs.y)),
        .y = sd_float_fmsub(lhs.z, rhs.x, sd_float_mul(lhs.x, rhs.z)),
        .z = sd_float_fmsub(lhs.x, rhs.y, sd_float_mul(lhs.y, rhs.x)),
    };
}

static inline sd_vec2 sd_vec2_normalize(sd_vec2 v) {
    sd_float sqrlen = sd_vec2_dot(v, v);
    sd_float rcplen = sd_float_rsqrt(sqrlen);
    return sd_vec2_mul(v, rcplen);
}

static inline sd_vec3 sd_vec3_normalize(sd_vec3 v) {
    sd_float sqrlen = sd_vec3_dot(v, v);
    sd_float rcplen = sd_float_rsqrt(sqrlen);
    return sd_vec3_mul(v, rcplen);
}

#endif /* STRIDE_H */
