/*
 * Strided data types and functions for generic vectorization
 * Currently supports SSE2 and AVX2
 * __AVX2__ implies that FMA is present
 */

#ifndef STRIDE_H
#define STRIDE_H

#include <SDL3/SDL.h>
#include <immintrin.h>
#include <limits.h>

#define SD_DEFINE_TYPES(suffix,underlying_type)                \
    typedef union sd_float_##suffix {                          \
        underlying_type val;                                   \
        float elems[sizeof(underlying_type) / sizeof(float)];  \
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
    typedef union sd_float_##suffix sd_float;  \
    typedef union sd_vec2_##suffix sd_vec2;    \
    typedef union sd_vec3_##suffix sd_vec3;    \
    typedef union sd_vec4_##suffix sd_vec4;

SD_DEFINE_TYPES(avx2, __m256)
SD_DEFINE_TYPES(sse2, __m128)
SD_DEFINE_TYPES(scalar, float) // NOLINT(bugprone-sizeof-expression)

#ifdef __AVX2__
    #define SD_VARIANT(fnname)  fnname##_avx2
    SD_TYPEDEFS(avx2)
#elifdef __SSE2__
    #define SD_VARIANT(fnname)  fnname##_sse2
    SD_TYPEDEFS(sse2)
#else
    #define SD_VARIANT(fnname)  fnname##_scalar
    SD_TYPEDEFS(scalar)
#endif

#define SD_PARAMS(...)                 __VA_OPT__(SD_PARAM1(__VA_ARGS__))
#define SD_PARAM1(type,name,...)       type name __VA_OPT__(, SD_PARAM2(__VA_ARGS__))
#define SD_PARAM2(type,name,...)       type name __VA_OPT__(, SD_PARAM3(__VA_ARGS__))
#define SD_PARAM3(type,name,...)       type name __VA_OPT__(, SD_PARAM4(__VA_ARGS__))
#define SD_PARAM4(type,name,...)       type name __VA_OPT__(, SD_PARAM5(__VA_ARGS__))
#define SD_PARAM5(type,name,...)       type name __VA_OPT__(, SD_PARAM6(__VA_ARGS__))
#define SD_PARAM6(type,name,...)       type name __VA_OPT__(, SD_PARAM7(__VA_ARGS__))
#define SD_PARAM7(type,name,...)       type name __VA_OPT__(, SD_PARAM8(__VA_ARGS__))
#define SD_PARAM8(type,name,...)       type name

#define SD_PARAM_NAMES(...)            __VA_OPT__(SD_PARAM_NAME1(__VA_ARGS__))
#define SD_PARAM_NAME1(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME2(__VA_ARGS__))
#define SD_PARAM_NAME2(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME3(__VA_ARGS__))
#define SD_PARAM_NAME3(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME4(__VA_ARGS__))
#define SD_PARAM_NAME4(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME5(__VA_ARGS__))
#define SD_PARAM_NAME5(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME6(__VA_ARGS__))
#define SD_PARAM_NAME6(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME7(__VA_ARGS__))
#define SD_PARAM_NAME7(type,name,...)  name __VA_OPT__(, SD_PARAM_NAME8(__VA_ARGS__))
#define SD_PARAM_NAME8(type,name,...)  name

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

#define SD_LENGTH          ( sizeof(sd_float) / sizeof(float) )
#define SD_ALIGN           ( alignof(sd_float) )

#define SD_DECLARE(rettype,fnname,...)                      \
    rettype fnname##_avx2(SD_PARAMS(__VA_ARGS__));          \
    rettype fnname##_sse2(SD_PARAMS(__VA_ARGS__));          \
    rettype fnname##_scalar(SD_PARAMS(__VA_ARGS__));        \
    static inline rettype fnname(SD_PARAMS(__VA_ARGS__)) {  \
        typeof(&fnname) sd_fn = SD_SELECT(fnname);          \
        return sd_fn(SD_PARAM_NAMES(__VA_ARGS__));          \
    }

#define SD_DECLARE_VOID_RETURN(fnname,...)               \
    void fnname##_avx2(SD_PARAMS(__VA_ARGS__));          \
    void fnname##_sse2(SD_PARAMS(__VA_ARGS__));          \
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

static inline size_t sd_bounding_size(size_t n) {
    return n ? (n - 1) / SD_LENGTH + 1 : 0;
}

// __m128i mullo_SSE2(__m128i a, __m128i b) {
//     __m128i shufa = _mm_shuffle_epi32(a, 0b00'11'00'01);
//     __m128i shufb = _mm_shuffle_epi32(b, 0b00'11'00'01);
// 
//     __m128i mul20 = _mm_mul_epu32(a, b);
//             mul20 = _mm_shuffle_epi32(mul20, 0b00'00'10'00);
//     __m128i mul31 = _mm_mul_epu32(shufa, shufb);
//             mul31 = _mm_shuffle_epi32(mul31, 0b00'00'10'00);
// 
//     return _mm_unpacklo_epi32(mul20, mul31);
// }

static inline sd_float sd_float_add(sd_float lhs, sd_float rhs) {
#ifdef __AVX2__
    return (sd_float){_mm256_add_ps(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_float){_mm_add_ps(lhs.val, rhs.val)};
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
#else
    return (sd_float){lhs.val * rhs.val};
#endif
}

SD_DEFINE_VECFNS_BINARY_VS(mul)

static inline sd_float sd_float_div(sd_float lhs, sd_float rhs) {
#ifdef __AVX2__
    return (sd_float){_mm256_div_ps(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_float){_mm_div_ps(lhs.val, rhs.val)};
#else
    return (sd_float){lhs.val / rhs.val};
#endif
}

SD_DEFINE_VECFNS_BINARY_VS(div)

static inline sd_float sd_float_abs(sd_float f) {
#ifdef __AVX2__
    __m256i minus_one = _mm256_set1_epi32(-1);
    __m256 abs_mask = _mm256_castsi256_ps(_mm256_srli_epi32(minus_one, 1));
    return (sd_float){_mm256_and_ps(f.val, abs_mask)};
#elifdef __SSE2__
    __m128i minus_one = _mm_set1_epi32(-1);
    __m128 abs_mask = _mm_castsi128_ps(_mm_srli_epi32(minus_one, 1));
    return (sd_float){_mm_and_ps(f.val, abs_mask)};
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
#else
    return (sd_float){1 / f.val};
#endif
}

SD_DEFINE_VECFNS_UNARY(rcp)

static inline sd_float sd_float_fmadd(sd_float multiplicand, sd_float multiplier, sd_float addend) {
#ifdef __AVX2__
    return (sd_float){_mm256_fmadd_ps(multiplicand.val, multiplier.val, addend.val)};
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
#else
    return sd_float_sub(sd_float_mul(multiplicand, multiplier), subtrahend);
#endif
}

static inline sd_float sd_float_sqrt(sd_float f) {
#ifdef __AVX2__
    return (sd_float){_mm256_sqrt_ps(f.val)};
#elifdef __SSE2__
    return (sd_float){_mm_sqrt_ps(f.val)};
#else
    return (sd_float){SDL_sqrtf(f.val)};
#endif
}

SD_DEFINE_VECFNS_UNARY(sqrt)

static inline sd_float sd_float_rsqrt(sd_float f) {
#ifdef __AVX2__
    return (sd_float){_mm256_rsqrt_ps(f.val)};
#elifdef __SSE2__
    return (sd_float){_mm_rsqrt_ps(f.val)};
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

static inline sd_float sd_float_lt(sd_float lhs, sd_float rhs) {
#ifdef __AVX2__
    return (sd_float){_mm256_cmp_ps(lhs.val, rhs.val, _CMP_LT_OQ)};
#elifdef __SSE2__
    return (sd_float){_mm_cmplt_ps(lhs.val, rhs.val)};
#else
    sd_float out;
    SDL_memset(&out, UCHAR_MAX * (lhs.val < rhs.val), sizeof(sd_float));
    return out;
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(lt)

static inline sd_float sd_float_gt(sd_float lhs, sd_float rhs) {
#ifdef __AVX2__
    return (sd_float){_mm256_cmp_ps(lhs.val, rhs.val, _CMP_GT_OQ)};
#elifdef __SSE2__
    return (sd_float){_mm_cmpgt_ps(lhs.val, rhs.val)};
#else
    sd_float out;
    SDL_memset(&out, UCHAR_MAX * (lhs.val > rhs.val), sizeof(sd_float));
    return out;
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(gt)

static inline sd_float sd_float_and(sd_float lhs, sd_float rhs) {
#ifdef __AVX2__
    return (sd_float){_mm256_and_ps(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_float){_mm_and_ps(lhs.val, rhs.val)};
#else
    uint32_t lhs_i, rhs_i;
    SDL_memcpy(&lhs_i, &lhs, sizeof(sd_float));
    SDL_memcpy(&rhs_i, &rhs, sizeof(sd_float));

    sd_float out;
    uint32_t res = lhs_i & rhs_i;
    SDL_memcpy(&out, &res, sizeof(sd_float));
    return out;
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(and)

static inline sd_float sd_float_or(sd_float lhs, sd_float rhs) {
#ifdef __AVX2__
    return (sd_float){_mm256_or_ps(lhs.val, rhs.val)};
#elifdef __SSE2__
    return (sd_float){_mm_or_ps(lhs.val, rhs.val)};
#else
    uint32_t lhs_i, rhs_i;
    SDL_memcpy(&lhs_i, &lhs, sizeof(sd_float));
    SDL_memcpy(&rhs_i, &rhs, sizeof(sd_float));

    sd_float out;
    uint32_t res = lhs_i | rhs_i;
    SDL_memcpy(&out, &res, sizeof(sd_float));
    return out;
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(or)

static inline sd_float sd_float_clamp_mask(sd_float f, float min, float max) {
#ifdef __AVX2__
    __m256 mask_gt = _mm256_cmp_ps(f.val, _mm256_set1_ps(min), _CMP_GT_OQ);
    __m256 mask_lt = _mm256_cmp_ps(f.val, _mm256_set1_ps(max), _CMP_LT_OQ);
    return (sd_float){_mm256_and_ps(mask_gt, mask_lt)};
#elifdef __SSE2__
    __m128 mask_gt = _mm_cmpgt_ps(f.val, _mm_set1_ps(min));
    __m128 mask_lt = _mm_cmplt_ps(f.val, _mm_set1_ps(max));
    return (sd_float){_mm_and_ps(mask_gt, mask_lt)};
#else
    SDL_memset(&f, UCHAR_MAX * (f.val > min && f.val < max), sizeof(sd_float));
    return f;
#endif
}

static inline sd_float sd_float_mask_blend(sd_float bg, sd_float fg, sd_float mask) {
#ifdef __AVX2__
    return (sd_float){_mm256_blendv_ps(bg.val, fg.val, mask.val)};
#elifdef __SSE2__
    __m128 select_bg = _mm_andnot_ps(mask.val, bg.val);
    __m128 select_fg = _mm_and_ps(mask.val, fg.val);
    return (sd_float){_mm_or_ps(select_bg, select_fg)};
#else
    uint32_t bg_i, fg_i, mask_i;
    SDL_memcpy(&bg_i, &bg, sizeof(float));
    SDL_memcpy(&fg_i, &fg, sizeof(float));
    SDL_memcpy(&mask_i, &mask, sizeof(float));

    sd_float out;
    uint32_t res = (bg_i & ~mask_i) | (fg_i & mask_i);
    SDL_memcpy(&out, &res, sizeof(float));
    
    return out;
#endif
}

static inline sd_vec2 sd_vec2_mask_blend(sd_vec2 bg, sd_vec2 fg, sd_float mask) {
    return (sd_vec2) {
        .x = sd_float_mask_blend(bg.x, fg.x, mask),
        .y = sd_float_mask_blend(bg.y, fg.y, mask)
    };
}

static inline sd_vec3 sd_vec3_mask_blend(sd_vec3 bg, sd_vec3 fg, sd_float mask) {
    return (sd_vec3) {
        .x = sd_float_mask_blend(bg.x, fg.x, mask),
        .y = sd_float_mask_blend(bg.y, fg.y, mask),
        .z = sd_float_mask_blend(bg.z, fg.z, mask)
    };
}

static inline sd_vec4 sd_vec4_mask_blend(sd_vec4 bg, sd_vec4 fg, sd_float mask) {
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
#else
    return (sd_float){0};
#endif
}

static inline sd_float sd_float_zero(void) {
#ifdef __AVX2__
    return (sd_float){_mm256_set1_ps(0)};
#elifdef __SSE2__
    return (sd_float){_mm_set1_ps(0)};
#else
    return (sd_float){0};
#endif
}

static inline sd_float sd_float_one(void) {
#ifdef __AVX2__
    return (sd_float){_mm256_set1_ps(1)};
#elifdef __SSE2__
    return (sd_float){_mm_set1_ps(1)};
#else
    return (sd_float){1};
#endif
}

static inline sd_float sd_float_set(float f) {
#ifdef __AVX2__
    return (sd_float){_mm256_set1_ps(f)};
#elifdef __SSE2__
    return (sd_float){_mm_set1_ps(f)};
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

static inline sd_float sd_vec2_length(sd_vec2 v) {
    sd_float sqrlen = sd_vec2_dot(v, v);
    return sd_float_sqrt(sqrlen);
}

static inline sd_float sd_vec3_length(sd_vec3 v) {
    sd_float sqrlen = sd_vec3_dot(v, v);
    return sd_float_sqrt(sqrlen);
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
