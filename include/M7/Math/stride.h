/*
 * Strided data types and functions for generic vectorization
 * Currently supports SSE2 and AVX2
 * SD_VECTORIZE_AVX2 assumes FMA
 */

#ifndef STRIDE_H
#define STRIDE_H

#include <SDL3/SDL.h>
#include <immintrin.h>

#define SD_VECTORIZE_AVX2  1
#define SD_VECTORIZE_SSE2  2

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
        struct {                                               \
            sd_float_##suffix x, y, z;                         \
        };                                                     \
    } sd_vec3_##suffix;

#define SD_TYPEDEFS(suffix)                    \
    typedef union sd_float_##suffix sd_float;  \
    typedef union sd_vec2_##suffix sd_vec2;    \
    typedef union sd_vec3_##suffix sd_vec3;

SD_DEFINE_TYPES(avx2, __m256)
SD_DEFINE_TYPES(sse2, __m128)
SD_DEFINE_TYPES(scalar, float) // NOLINT(bugprone-sizeof-expression)

#if SD_VECTORIZE == SD_VECTORIZE_AVX2
    #define SD_VARIANT(fnname)  fnname##_avx2
    SD_TYPEDEFS(avx2)
#elif SD_VECTORIZE == SD_VECTORIZE_SSE2
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

#define SD_SELECT(fnname)  ( SDL_HasAVX2() ? fnname##_avx2 : SDL_HasSSE2() ? fnname##_sse2 : fnname##_scalar )
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

static inline size_t sd_bounding_size(size_t n) {
    return n ? (n - 1) / SD_LENGTH + 1 : 0;
}

static inline sd_float sd_float_add(sd_float lhs, sd_float rhs) {
#if SD_VECTORIZE == SD_VECTORIZE_AVX2
    return (sd_float){_mm256_add_ps(lhs.val, rhs.val)};
#elif SD_VECTORIZE == SD_VECTORIZE_SSE2
    return (sd_float){_mm_add_ps(lhs.val, rhs.val)};
#else
    return (sd_float){lhs.val + rhs.val};
#endif
}

static inline sd_float sd_float_sub(sd_float lhs, sd_float rhs) {
#if SD_VECTORIZE == SD_VECTORIZE_AVX2
    return (sd_float){_mm256_sub_ps(lhs.val, rhs.val)};
#elif SD_VECTORIZE == SD_VECTORIZE_SSE2
    return (sd_float){_mm_sub_ps(lhs.val, rhs.val)};
#else
    return (sd_float){lhs.val - rhs.val};
#endif
}

static inline sd_float sd_float_mul(sd_float lhs, sd_float rhs) {
#if SD_VECTORIZE == SD_VECTORIZE_AVX2
    return (sd_float){_mm256_mul_ps(lhs.val, rhs.val)};
#elif SD_VECTORIZE == SD_VECTORIZE_SSE2
    return (sd_float){_mm_mul_ps(lhs.val, rhs.val)};
#else
    return (sd_float){lhs.val * rhs.val};
#endif
}

static inline sd_float sd_float_div(sd_float lhs, sd_float rhs) {
#if SD_VECTORIZE == SD_VECTORIZE_AVX2
    return (sd_float){_mm256_div_ps(lhs.val, rhs.val)};
#elif SD_VECTORIZE == SD_VECTORIZE_SSE2
    return (sd_float){_mm_div_ps(lhs.val, rhs.val)};
#else
    return (sd_float){lhs.val / rhs.val};
#endif
}

static inline sd_float sd_float_rcp(sd_float f) {
#if SD_VECTORIZE == SD_VECTORIZE_AVX2
    return (sd_float){_mm256_rcp_ps(f.val)};
#elif SD_VECTORIZE == SD_VECTORIZE_SSE2
    return (sd_float){_mm_rcp_ps(f.val)};
#else
    return (sd_float){1 / f.val};
#endif
}

static inline sd_float sd_float_fmadd(sd_float multiplier, sd_float multiplicand, sd_float addend) {
#if SD_VECTORIZE == SD_VECTORIZE_AVX2
    return (sd_float){_mm256_fmadd_ps(multiplier.val, multiplicand.val, addend.val)};
#elif SD_VECTORIZE == SD_VECTORIZE_SSE2
    return sd_float_add(sd_float_mul(multiplier, multiplicand), addend);
#else
    return (sd_float){multiplier.val * multiplicand.val + addend.val};
#endif
}

static inline sd_float sd_float_sqrt(sd_float f) {
#if SD_VECTORIZE == SD_VECTORIZE_AVX2
    return (sd_float){_mm256_sqrt_ps(f.val)};
#elif SD_VECTORIZE == SD_VECTORIZE_SSE2
    return (sd_float){_mm_sqrt_ps(f.val)};
#else
    return (sd_float){SDL_sqrtf(f.val)};
#endif
}

static inline sd_float sd_float_rsqrt(sd_float f) {
#if SD_VECTORIZE == SD_VECTORIZE_AVX2
    return (sd_float){_mm256_rsqrt_ps(f.val)};
#elif SD_VECTORIZE == SD_VECTORIZE_SSE2
    return (sd_float){_mm_rsqrt_ps(f.val)};
#else
    return (sd_float){1 / SDL_sqrtf(f.val)};
#endif
}

static inline sd_float sd_float_set(float f) {
#if SD_VECTORIZE == SD_VECTORIZE_AVX2
    return (sd_float){_mm256_set1_ps(f)};
#elif SD_VECTORIZE == SD_VECTORIZE_SSE2
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

static inline sd_float_scalar sd_float_arr_get(sd_float *arr, size_t index) {
    return (sd_float_scalar) { arr[index / SD_LENGTH].elems[index % SD_LENGTH] };
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

static inline sd_vec2 sd_vec2_add(sd_vec2 lhs, sd_vec2 rhs) {
    return (sd_vec2) {
        .x = sd_float_add(lhs.x, rhs.x),
        .y = sd_float_add(lhs.y, rhs.y)
    };
}

static inline sd_vec3 sd_vec3_add(sd_vec3 lhs, sd_vec3 rhs) {
    return (sd_vec3) {
        .x = sd_float_add(lhs.x, rhs.x),
        .y = sd_float_add(lhs.y, rhs.y),
        .z = sd_float_add(lhs.z, rhs.z)
    };
}

static inline sd_vec2 sd_vec2_sub(sd_vec2 lhs, sd_vec2 rhs) {
    return (sd_vec2) {
        .x = sd_float_sub(lhs.x, rhs.x),
        .y = sd_float_sub(lhs.y, rhs.y)
    };
}

static inline sd_vec3 sd_vec3_sub(sd_vec3 lhs, sd_vec3 rhs) {
    return (sd_vec3) {
        .x = sd_float_sub(lhs.x, rhs.x),
        .y = sd_float_sub(lhs.y, rhs.y),
        .z = sd_float_sub(lhs.z, rhs.z)
    };
}

static inline sd_vec2 sd_vec2_mul(sd_vec2 v, sd_float f) {
    return (sd_vec2) {
        .x = sd_float_mul(v.x, f),
        .y = sd_float_mul(v.y, f)
    };
}

static inline sd_vec3 sd_vec3_mul(sd_vec3 v, sd_float f) {
    return (sd_vec3) {
        .x = sd_float_mul(v.x, f),
        .y = sd_float_mul(v.y, f),
        .z = sd_float_mul(v.z, f)
    };
}

static inline sd_vec3 sd_vec3_fmadd(sd_float multiplier, sd_vec3 multiplicand, sd_vec3 addend) {
    return (sd_vec3) {
        .x = sd_float_fmadd(multiplier, multiplicand.x, addend.x),
        .y = sd_float_fmadd(multiplier, multiplicand.y, addend.y),
        .z = sd_float_fmadd(multiplier, multiplicand.z, addend.z)
    };
}

static inline sd_vec2 sd_vec2_div(sd_vec2 v, sd_float f) {
    return (sd_vec2) {
        .x = sd_float_div(v.x, f),
        .y = sd_float_div(v.y, f)
    };
}

static inline sd_vec3 sd_vec3_div(sd_vec3 v, sd_float f) {
    return (sd_vec3) {
        .x = sd_float_div(v.x, f),
        .y = sd_float_div(v.y, f),
        .z = sd_float_div(v.z, f)
    };
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

    return (sd_vec2) {
        .x = sd_float_mul(rcplen, v.x),
        .y = sd_float_mul(rcplen, v.y),
    };
}

static inline sd_vec3 sd_vec3_normalize(sd_vec3 v) {
    sd_float sqrlen = sd_vec3_dot(v, v);
    sd_float rcplen = sd_float_rsqrt(sqrlen);

    return (sd_vec3) {
        .x = sd_float_mul(rcplen, v.x),
        .y = sd_float_mul(rcplen, v.y),
        .z = sd_float_mul(rcplen, v.z)
    };
}

#endif /* STRIDE_H */
