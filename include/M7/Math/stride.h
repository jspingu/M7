/*
 * Strided data types and functions for generic vectorization
 * Currently supports SSE2, AVX2, AVX512F, and NEON
 * __AVX2__ implies that FMA is present
 */

#ifndef STRIDE_H
#define STRIDE_H

#include <SDL3/SDL.h>
#include <limits.h>

#if defined(__SSE2__) || defined(__AVX2__) || defined(__AVX512F__)
#include <immintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#define SD_DEFINE_TYPES(suffix,float_type,int_type,mask_type)  \
    typedef mask_type sd_mask_##suffix;                        \
                                                               \
    typedef union sd_int_##suffix {                            \
        int_type val;                                          \
        int elems[sizeof(int_type) / sizeof(int32_t)];         \
    } sd_int_##suffix;                                         \
                                                               \
    typedef union sd_float_##suffix {                          \
        float_type val;                                        \
        float elems[sizeof(float_type) / sizeof(float)];       \
    } sd_float_##suffix;                                       \
                                                               \
    typedef union sd_vec2_##suffix {                           \
        sd_float_##suffix xy[2];                               \
        struct {                                               \
            sd_float_##suffix x, y;                            \
        };                                                     \
    } sd_vec2_##suffix;                                        \
                                                               \
    typedef union sd_vec3_##suffix {                           \
        sd_float_##suffix xyz[3];                              \
        sd_float_##suffix rgb[3];                              \
        sd_vec2_##suffix xy;                                   \
        struct {                                               \
            sd_float_##suffix x, y, z;                         \
        };                                                     \
        struct {                                               \
            sd_float_##suffix r, g, b;                         \
        };                                                     \
    } sd_vec3_##suffix;                                        \
                                                               \
    typedef union sd_vec4_##suffix {                           \
        sd_float_##suffix xyzw[4];                             \
        sd_float_##suffix rgba[4];                             \
        sd_vec2_##suffix xy;                                   \
        sd_vec3_##suffix xyz;                                  \
        sd_vec3_##suffix rgb;                                  \
        struct {                                               \
            sd_float_##suffix x, y, z, w;                      \
        };                                                     \
        struct {                                               \
            sd_float_##suffix r, g, b, a;                      \
        };                                                     \
    } sd_vec4_##suffix;

#define SD_TYPEDEFS(suffix)                    \
    typedef sd_mask_##suffix sd_mask;          \
    typedef union sd_int_##suffix sd_int;      \
    typedef union sd_float_##suffix sd_float;  \
    typedef union sd_vec2_##suffix sd_vec2;    \
    typedef union sd_vec3_##suffix sd_vec3;    \
    typedef union sd_vec4_##suffix sd_vec4;

SD_DEFINE_TYPES(scalar, float, int32_t, bool) // NOLINT(bugprone-sizeof-expression)

#ifdef __AVX512F__
    #define SD_VARIANT(fnname)  fnname##_avx512f
    #define SD_LOG_LENGTH       4
    SD_DEFINE_TYPES(avx512f, __m512, __m512i, __mmask16)
    SD_TYPEDEFS(avx512f)
#elifdef __AVX2__
    #define SD_VARIANT(fnname)  fnname##_avx2
    #define SD_LOG_LENGTH       3
    SD_DEFINE_TYPES(avx2, __m256, __m256i, __m256i)
    SD_TYPEDEFS(avx2)
#elifdef __SSE2__
    #define SD_VARIANT(fnname)  fnname##_sse2
    #define SD_LOG_LENGTH       2
    SD_DEFINE_TYPES(sse2, __m128, __m128i, __m128i)
    SD_TYPEDEFS(sse2)
#elifdef __ARM_NEON
    #define SD_VARIANT(fnname)  fnname##_neon
    #define SD_LOG_LENGTH       2
    SD_DEFINE_TYPES(neon, float32x4_t, int32x4_t, uint32x4_t)
    SD_TYPEDEFS(neon)
#else
    #define SD_VARIANT(fnname)  fnname##_scalar
    #define SD_LOG_LENGTH       0
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

#ifdef __AVX512F__
#define SD_SELECT(fnname)  ( fnname##_avx512f )
#elifdef __AVX2__
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
#ifdef __AVX512F__
#define SD_SELECT(fnname)  ( fnname##_avx512f )
#elifdef __AVX2__
#define SD_SELECT(fnname)  ( SDL_HasAVX512F() ? fnname##_avx512f : fnname##_avx2 )
#else
#define SD_SELECT(fnname)  ( SDL_HasAVX512F() ? fnname##_avx512f : SDL_HasAVX2() ? fnname##_avx2 : fnname##_sse2 )
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
    typeof(rettype) fnname##_avx512f(SD_PARAMS(__VA_ARGS__));       \
    typeof(rettype) fnname##_avx2(SD_PARAMS(__VA_ARGS__));          \
    typeof(rettype) fnname##_sse2(SD_PARAMS(__VA_ARGS__));          \
    typeof(rettype) fnname##_neon(SD_PARAMS(__VA_ARGS__));          \
    typeof(rettype) fnname##_scalar(SD_PARAMS(__VA_ARGS__));        \
    static inline typeof(rettype) fnname(SD_PARAMS(__VA_ARGS__)) {  \
        typeof(&fnname) sd_fn = SD_SELECT(fnname);                  \
        return sd_fn(SD_PARAM_NAMES(__VA_ARGS__));                  \
    }

#define SD_DECLARE_VOID_RETURN(fnname,...)               \
    void fnname##_avx512f(SD_PARAMS(__VA_ARGS__));       \
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

static inline sd_mask sd_mask_and(sd_mask lhs, sd_mask rhs) {
#ifdef __AVX512F__
    return _mm512_kand(lhs, rhs);
#elifdef __AVX2__
    return _mm256_and_si256(lhs, rhs);
#elifdef __SSE2__
    return _mm_and_si128(lhs, rhs);
#elifdef __ARM_NEON
    return vandq_u32(lhs, rhs);
#else
    return lhs && rhs;
#endif
}

static inline sd_mask sd_mask_or(sd_mask lhs, sd_mask rhs) {
#ifdef __AVX512F__
    return _mm512_kor(lhs, rhs);
#elifdef __AVX2__
    return _mm256_or_si256(lhs, rhs);
#elifdef __SSE2__
    return _mm_or_si128(lhs, rhs);
#elifdef __ARM_NEON
    return vorrq_u32(lhs, rhs);
#else
    return lhs || rhs;
#endif
}

static inline sd_int sd_int_add(sd_int lhs, sd_int rhs) {
#ifdef __AVX512F__
    return (sd_int){_mm512_add_epi32(lhs.val, rhs.val)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_int){_mm512_sub_epi32(lhs.val, rhs.val)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_int){_mm512_mullo_epi32(lhs.val, rhs.val)};
#elifdef __AVX2__
    return (sd_int){_mm256_mullo_epi32(lhs.val, rhs.val)};
#elifdef __SSE2__
    __m128i lhs31 = _mm_shuffle_epi32(lhs.val, 0b00'11'00'01);
    __m128i rhs31 = _mm_shuffle_epi32(rhs.val, 0b00'11'00'01);

    __m128i mul20 = _mm_mul_epu32(lhs.val, rhs.val);
            mul20 = _mm_shuffle_epi32(mul20, 0b00'00'10'00);
    __m128i mul31 = _mm_mul_epu32(lhs31, rhs31);
            mul31 = _mm_shuffle_epi32(mul31, 0b00'00'10'00);

    return (sd_int){_mm_unpacklo_epi32(mul20, mul31)};
#elifdef __ARM_NEON
    return (sd_int){vmulq_s32(lhs.val, rhs.val)};
#else
    return (sd_int){lhs.val * rhs.val};
#endif
}

static inline sd_mask sd_int_lt(sd_int lhs, sd_int rhs) {
#ifdef __AVX512F__
    return _mm512_cmplt_epi32_mask(lhs.val, rhs.val);
#elifdef __AVX2__
    return _mm256_cmpgt_epi32(rhs.val, lhs.val);
#elifdef __SSE2__
    return _mm_cmplt_epi32(lhs.val, rhs.val);
#elifdef __ARM_NEON
    return vcltq_s32(lhs.val, rhs.val);
#else
    return lhs.val < rhs.val;
#endif
}

static inline sd_mask sd_int_gt(sd_int lhs, sd_int rhs) {
#ifdef __AVX512F__
    return _mm512_cmpgt_epi32_mask(lhs.val, rhs.val);
#elifdef __AVX2__
    return _mm256_cmpgt_epi32(lhs.val, rhs.val);
#elifdef __SSE2__
    return _mm_cmpgt_epi32(lhs.val, rhs.val);
#elifdef __ARM_NEON
    return vcgtq_s32(lhs.val, rhs.val);
#else
    return lhs.val > rhs.val;
#endif
}

static inline sd_int sd_int_and(sd_int lhs, sd_int rhs) {
#ifdef __AVX512F__
    return (sd_int){_mm512_and_si512(lhs.val, rhs.val)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_int){_mm512_or_si512(lhs.val, rhs.val)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_int){_mm512_slli_epi32(i.val, shift)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_int){_mm512_srai_epi32(i.val, shift)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_int){_mm512_set1_epi32(i)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_int){_mm512_i32gather_epi32(index.val, buf, 1)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    _mm512_storeu_epi32(dst, i.val);
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_int){_mm512_cvttps_epi32(f.val)};
#elifdef __AVX2__
    return (sd_int){_mm256_cvttps_epi32(f.val)};
#elifdef __SSE2__
    return (sd_int){_mm_cvttps_epi32(f.val)};
#elifdef __ARM_NEON
    return (sd_int){vcvtq_s32_f32(f.val)};
#else
    return (sd_int){(int32_t)f.val};
#endif
}

static inline sd_float sd_int_to_float(sd_int i) {
#ifdef __AVX512F__
    return (sd_float){_mm512_cvtepi32_ps(i.val)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_float){_mm512_add_ps(lhs.val, rhs.val)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_float){_mm512_sub_ps(lhs.val, rhs.val)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_float){_mm512_mul_ps(lhs.val, rhs.val)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_float){_mm512_abs_ps(f.val)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_float){_mm512_mul_ps(f.val, _mm512_set1_ps(-1))};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_float){_mm512_rcp14_ps(f.val)};
#elifdef __AVX2__
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

static inline sd_float sd_float_rsqrt(sd_float f) {
#ifdef __AVX512F__
    return (sd_float){_mm512_rsqrt14_ps(f.val)};
#elifdef __AVX2__
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

static inline sd_float sd_float_trunc(sd_float f) {
    return sd_int_to_float(sd_float_to_int(f));
}

SD_DEFINE_VECFNS_UNARY(trunc)

static inline sd_float sd_float_frac(sd_float f) {
    return sd_float_sub(f, sd_float_trunc(f));
}

SD_DEFINE_VECFNS_UNARY(frac)

static inline sd_float sd_float_fmadd(sd_float multiplicand, sd_float multiplier, sd_float addend) {
#ifdef __AVX512F__
    return (sd_float){_mm512_fmadd_ps(multiplicand.val, multiplier.val, addend.val)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_float){_mm512_fmsub_ps(multiplicand.val, multiplier.val, subtrahend.val)};
#elifdef __AVX2__
    return (sd_float){_mm256_fmsub_ps(multiplicand.val, multiplier.val, subtrahend.val)};
#elifdef __ARM_NEON
    return (sd_float){vmlaq_f32(vnegq_f32(subtrahend.val), multiplicand.val, multiplier.val)};
#else
    return sd_float_sub(sd_float_mul(multiplicand, multiplier), subtrahend);
#endif
}

static inline sd_float sd_float_blend(sd_float bg, sd_float fg, sd_float coeff) {
    return sd_float_fmadd(sd_float_sub(fg, bg), coeff, bg);
}

static inline sd_vec2 sd_vec2_blend(sd_vec2 bg, sd_vec2 fg, sd_float coeff) {
    return sd_vec2_fmadd(sd_vec2_sub(fg, bg), coeff, bg);
}

static inline sd_vec3 sd_vec3_blend(sd_vec3 bg, sd_vec3 fg, sd_float coeff) {
    return sd_vec3_fmadd(sd_vec3_sub(fg, bg), coeff, bg);
}

static inline sd_vec4 sd_vec4_blend(sd_vec4 bg, sd_vec4 fg, sd_float coeff) {
    return sd_vec4_fmadd(sd_vec4_sub(fg, bg), coeff, bg);
}

static inline sd_float sd_float_min(sd_float lhs, sd_float rhs) {
#ifdef __AVX512F__
    return (sd_float){_mm512_min_ps(lhs.val, rhs.val)};
#elifdef __AVX2__
    return (sd_float){_mm256_min_ps(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_float){_mm_min_ps(lhs.val, rhs.val)};
#elifdef __ARM_NEON
    return (sd_float){vminq_f32(lhs.val, rhs.val)};
#else
    return (sd_float){SDL_min(lhs.val, rhs.val)};
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(min)

static inline sd_float sd_float_max(sd_float lhs, sd_float rhs) {
#ifdef __AVX512F__
    return (sd_float){_mm512_max_ps(lhs.val, rhs.val)};
#elifdef __AVX2__
    return (sd_float){_mm256_max_ps(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_float){_mm_max_ps(lhs.val, rhs.val)};
#elifdef __ARM_NEON
    return (sd_float){vmaxq_f32(lhs.val, rhs.val)};
#else
    return (sd_float){SDL_max(lhs.val, rhs.val)};
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

static inline sd_mask sd_float_lt(sd_float lhs, sd_float rhs) {
#ifdef __AVX512F__
    return _mm512_cmp_ps_mask(lhs.val, rhs.val, _CMP_LT_OQ);
#elifdef __AVX2__
    return _mm256_castps_si256(_mm256_cmp_ps(lhs.val, rhs.val, _CMP_LT_OQ));
#elifdef __SSE2__
    return _mm_castps_si128(_mm_cmplt_ps(lhs.val, rhs.val));
#elifdef __ARM_NEON
    return vcltq_f32(lhs.val, rhs.val);
#else
    return lhs.val < rhs.val;
#endif
}

static inline sd_mask sd_float_gt(sd_float lhs, sd_float rhs) {
#ifdef __AVX512F__
    return _mm512_cmp_ps_mask(lhs.val, rhs.val, _CMP_GT_OQ);
#elifdef __AVX2__
    return _mm256_castps_si256(_mm256_cmp_ps(lhs.val, rhs.val, _CMP_GT_OQ));
#elifdef __SSE2__
    return _mm_castps_si128(_mm_cmpgt_ps(lhs.val, rhs.val));
#elifdef __ARM_NEON
    return vcgtq_f32(lhs.val, rhs.val);
#else
    return lhs.val > rhs.val;
#endif
}

static inline sd_mask sd_float_clamp_mask(sd_float f, float min, float max) {
#ifdef __AVX512F__
    __mmask16 mask_gt = _mm512_cmp_ps_mask(f.val, _mm512_set1_ps(min), _CMP_GT_OQ);
    __mmask16 mask_lt = _mm512_cmp_ps_mask(f.val, _mm512_set1_ps(max), _CMP_LT_OQ);
    return _mm512_kand(mask_gt, mask_lt);
#elifdef __AVX2__
    __m256 mask_gt = _mm256_cmp_ps(f.val, _mm256_set1_ps(min), _CMP_GT_OQ);
    __m256 mask_lt = _mm256_cmp_ps(f.val, _mm256_set1_ps(max), _CMP_LT_OQ);
    return _mm256_castps_si256(_mm256_and_ps(mask_gt, mask_lt));
#elifdef __SSE2__
    __m128 mask_gt = _mm_cmpgt_ps(f.val, _mm_set1_ps(min));
    __m128 mask_lt = _mm_cmplt_ps(f.val, _mm_set1_ps(max));
    return _mm_castps_si128(_mm_and_ps(mask_gt, mask_lt));
#elifdef __ARM_NEON
    uint32x4_t mask_gt = vcgtq_f32(f.val, vdupq_n_f32(min));
    uint32x4_t mask_lt = vcltq_f32(f.val, vdupq_n_f32(max));
    return vandq_u32(mask_gt, mask_lt);
#else
    return f.val > min && f.val < max;
#endif
}

static inline sd_float sd_float_mask_blend(sd_float bg, sd_float fg, sd_mask mask) {
#ifdef __AVX512F__
    return (sd_float){_mm512_mask_blend_ps(mask, bg.val, fg.val)};
#elifdef __AVX2__
    return (sd_float){_mm256_blendv_ps(bg.val, fg.val, _mm256_castsi256_ps(mask))};
#elifdef __SSE2__
    __m128 select_bg = _mm_andnot_ps(_mm_castsi128_ps(mask), bg.val);
    __m128 select_fg = _mm_and_ps(_mm_castsi128_ps(mask), fg.val);
    return (sd_float){_mm_or_ps(select_bg, select_fg)};
#elifdef __ARM_NEON
    return (sd_float){vbslq_f32(mask, fg.val, bg.val)};
#else
    return mask ? fg : bg;
#endif
}

static inline sd_vec2 sd_vec2_mask_blend(sd_vec2 bg, sd_vec2 fg, sd_mask mask) {
    return (sd_vec2) {
        .x = sd_float_mask_blend(bg.x, fg.x, mask),
        .y = sd_float_mask_blend(bg.y, fg.y, mask)
    };
}

static inline sd_vec3 sd_vec3_mask_blend(sd_vec3 bg, sd_vec3 fg, sd_mask mask) {
    return (sd_vec3) {
        .x = sd_float_mask_blend(bg.x, fg.x, mask),
        .y = sd_float_mask_blend(bg.y, fg.y, mask),
        .z = sd_float_mask_blend(bg.z, fg.z, mask)
    };
}

static inline sd_vec4 sd_vec4_mask_blend(sd_vec4 bg, sd_vec4 fg, sd_mask mask) {
    return (sd_vec4) {
        .x = sd_float_mask_blend(bg.x, fg.x, mask),
        .y = sd_float_mask_blend(bg.y, fg.y, mask),
        .z = sd_float_mask_blend(bg.z, fg.z, mask),
        .w = sd_float_mask_blend(bg.w, fg.w, mask)
    };
}

static inline sd_float sd_float_range(void) {
#ifdef __AVX512F__
    return (sd_float){_mm512_set_ps(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_float){_mm512_setzero_ps()};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_float){_mm512_set1_ps(1)};
#elifdef __AVX2__
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
#ifdef __AVX512F__
    return (sd_float){_mm512_set1_ps(f)};
#elifdef __AVX2__
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

static inline sd_float sd_float_gather(float *buf, sd_int index) {
#ifdef __AVX512F__
    return (sd_float){_mm512_i32gather_ps(index.val, buf, 4)};
#elifdef __AVX2__
    return (sd_float){_mm256_i32gather_ps(buf, index.val, 4)};
#elifdef __SSE2__
    alignas(SD_ALIGN) float elems[4];
    alignas(SD_ALIGN) int32_t idxs[4];

    SDL_memcpy(idxs, &index, sizeof(sd_int));

    for (int i = 0; i < 4; ++i)
        elems[i] = buf[idxs[i]];

    sd_float out;
    SDL_memcpy(&out, elems, sizeof(sd_float));
    return out;
#elifdef __ARM_NEON
    return (sd_float){{buf[index.val[0]], buf[index.val[1]], buf[index.val[2]], buf[index.val[3]]}};
#else
    return (sd_float){buf[index.val]};
#endif
}

static inline sd_vec3 sd_vec3_gather_strided(sd_vec3 *buf, sd_int index) {
    sd_int sd_qot = sd_int_shr(index, SD_LOG_LENGTH);
    sd_int sd_rem = sd_int_and(index, sd_int_set(SD_LENGTH - 1));
    sd_int sd_idx = sd_int_add(sd_qot, sd_int_shl(sd_qot, 1));
           sd_idx = sd_int_shl(sd_idx, SD_LOG_LENGTH);
           sd_idx = sd_int_add(sd_idx, sd_rem);

    return (sd_vec3) {
        .x = sd_float_gather((float *)&buf->x, sd_idx),
        .y = sd_float_gather((float *)&buf->y, sd_idx),
        .z = sd_float_gather((float *)&buf->z, sd_idx)
    };
}

static inline sd_vec4 sd_vec4_gather(float *buf, sd_int index) {
    sd_int base = sd_int_shl(index, 2);

    return (sd_vec4) {
        .x = sd_float_gather(buf + 0, base),
        .y = sd_float_gather(buf + 1, base),
        .z = sd_float_gather(buf + 2, base),
        .w = sd_float_gather(buf + 3, base)
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

static inline sd_vec2 sd_vec2_reflect(sd_vec2 v, sd_vec2 nrml) {
    sd_vec2 prj = sd_vec2_mul(v, sd_vec2_dot(v, nrml));
    sd_vec2 rej = sd_vec2_sub(v, prj);
    return sd_vec2_add(rej, sd_vec2_negate(prj));
}

static inline sd_vec3 sd_vec3_reflect(sd_vec3 v, sd_vec3 nrml) {
    sd_vec3 prj = sd_vec3_mul(v, sd_vec3_dot(v, nrml));
    sd_vec3 rej = sd_vec3_sub(v, prj);
    return sd_vec3_add(rej, sd_vec3_negate(prj));
}

#endif /* STRIDE_H */
