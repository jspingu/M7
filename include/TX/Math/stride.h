/*
 * Architecture-agnostic scalable vectorization library
 *
 * Supported extensions:
 *  - x86: SSE2, AVX2, AVX512F
 *  - ARM: NEON, SVE
 */

#ifndef STRIDE_H
#define STRIDE_H

#include <SDL3/SDL.h>
#include <limits.h>

#ifdef __SSE2__
#include <immintrin.h>
#endif

#ifdef __ARM_FEATURE_SVE
#include <arm_sve.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#define SD_DEFINE_TYPES_FIXED(suffix,float_type,int_type,mask_type)  \
    typedef float_type sd_float_##suffix;                            \
    typedef int_type sd_int_##suffix;                                \
    typedef mask_type sd_mask_##suffix;                              \
                                                                     \
    typedef struct sd_vec2_##suffix {                                \
        sd_float_##suffix x, y;                                      \
    } sd_vec2_##suffix;                                              \
                                                                     \
    typedef struct sd_vec3_##suffix {                                \
        sd_float_##suffix x, y, z;                                   \
    } sd_vec3_##suffix;                                              \
                                                                     \
    typedef struct sd_vec4_##suffix {                                \
        sd_float_##suffix x, y, z, w;                                \
    } sd_vec4_##suffix;

#define SD_DEFINE_TYPES_SCALABLE(suffix,float_type,floatx2_type,floatx3_type,floatx4_type,int_type,mask_type)  \
    typedef float_type sd_float_##suffix;                                                                      \
    typedef floatx2_type sd_vec2_##suffix;                                                                     \
    typedef floatx3_type sd_vec3_##suffix;                                                                     \
    typedef floatx4_type sd_vec4_##suffix;                                                                     \
    typedef int_type sd_int_##suffix;                                                                          \
    typedef mask_type sd_mask_##suffix;

#define SD_TYPEDEFS(suffix)              \
    typedef sd_mask_##suffix sd_mask;    \
    typedef sd_int_##suffix sd_int;      \
    typedef sd_float_##suffix sd_float;  \
    typedef sd_vec2_##suffix sd_vec2;    \
    typedef sd_vec3_##suffix sd_vec3;    \
    typedef sd_vec4_##suffix sd_vec4;

SD_DEFINE_TYPES_FIXED(scalar, float, int32_t, bool)

#ifdef __AVX512F__
    #define SD_VARIANT(fnname)  fnname##_avx512f
    SD_DEFINE_TYPES_FIXED(avx512f, __m512, __m512i, __mmask16)
    SD_TYPEDEFS(avx512f)
#elifdef __AVX2__
    #define SD_VARIANT(fnname)  fnname##_avx2
    SD_DEFINE_TYPES_FIXED(avx2, __m256, __m256i, __m256i)
    SD_TYPEDEFS(avx2)
#elifdef __SSE2__
    #define SD_VARIANT(fnname)  fnname##_sse2
    SD_DEFINE_TYPES_FIXED(sse2, __m128, __m128i, __m128i)
    SD_TYPEDEFS(sse2)
#elifdef __ARM_FEATURE_SVE
    #define SD_VARIANT(fnname)  fnname##_sve
    SD_DEFINE_TYPES_SCALABLE(sve, svfloat32_t, svfloat32x2_t, svfloat32x3_t, svfloat32x4_t, svint32_t, svbool_t)
    SD_TYPEDEFS(sve)
#elifdef __ARM_NEON
    #define SD_VARIANT(fnname)  fnname##_neon
    SD_DEFINE_TYPES_FIXED(neon, float32x4_t, int32x4_t, uint32x4_t)
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

#ifdef __AVX512F__
    #define SD_SELECT(fnname)  ( fnname##_avx512f )
#elifdef __AVX2__
    #define SD_SELECT(fnname)  ( fnname##_avx2 )
#elifdef __SSE2__
    #define SD_SELECT(fnname)  ( fnname##_sse2 )
#elifdef __ARM_FEATURE_SVE
    #define SD_SELECT(fnname)  ( fnname##_sve )
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
#ifdef __ARM_FEATURE_SVE
    #define SD_SELECT(fnname)  ( fnname##_sve )
#else
    #define SD_SELECT(fnname)  ( __builtin_cpu_supports("sve") ? fnname##_sve : fnname##_neon )
#endif
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

#else
    #error "Vectorization dispatch method not defined. Define either SD_DISPATCH_STATIC or SD_DISPATCH_DYNAMIC"

#endif /* SD_DISPATCH */

#define SD_DECLARE(rettype,fnname,...)                              \
    typeof(rettype) fnname##_avx512f(SD_PARAMS(__VA_ARGS__));       \
    typeof(rettype) fnname##_avx2(SD_PARAMS(__VA_ARGS__));          \
    typeof(rettype) fnname##_sse2(SD_PARAMS(__VA_ARGS__));          \
    typeof(rettype) fnname##_sve(SD_PARAMS(__VA_ARGS__));           \
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
    void fnname##_sve(SD_PARAMS(__VA_ARGS__));           \
    void fnname##_neon(SD_PARAMS(__VA_ARGS__));          \
    void fnname##_scalar(SD_PARAMS(__VA_ARGS__));        \
    static inline void fnname(SD_PARAMS(__VA_ARGS__)) {  \
        typeof(&fnname) sd_fn = SD_SELECT(fnname);       \
        sd_fn(SD_PARAM_NAMES(__VA_ARGS__));              \
    }

#ifdef __ARM_FEATURE_SVE
    #define sd_vx(v)  _Generic((v),  \
        sd_vec2: svget2,             \
        sd_vec3: svget3,             \
        sd_vec4: svget4              \
    )(v, 0)
    
    #define sd_vy(v)  _Generic((v),  \
        sd_vec2: svget2,             \
        sd_vec3: svget3,             \
        sd_vec4: svget4              \
    )(v, 1)
    
    #define sd_vz(v)  _Generic((v),  \
        sd_vec3: svget3,             \
        sd_vec4: svget4              \
    )(v, 2)
    
    #define sd_vw(v)  _Generic((v),  \
        sd_vec4: svget4              \
    )(v, 3)
#else
    #define sd_vx(v)  (v).x
    #define sd_vy(v)  (v).y
    #define sd_vz(v)  (v).z
    #define sd_vw(v)  (v).w
#endif

#define sd_vxy(v)   ( sd_vec2_create(sd_vx(v), sd_vy(v)) )
#define sd_vxyz(v)  ( sd_vec3_create(sd_vx(v), sd_vy(v), sd_vz(v)) )

#define SD_DEFINE_VECFNS_BINARY_VV(name,base)                         \
    static inline sd_vec2 sd_vec2_##name(sd_vec2 lhs, sd_vec2 rhs) {  \
        return sd_vec2_create(                                        \
            sd_float_##base(sd_vx(lhs), sd_vx(rhs)),                  \
            sd_float_##base(sd_vy(lhs), sd_vy(rhs))                   \
        );                                                            \
    }                                                                 \
                                                                      \
    static inline sd_vec3 sd_vec3_##name(sd_vec3 lhs, sd_vec3 rhs) {  \
        return sd_vec3_create(                                        \
            sd_float_##base(sd_vx(lhs), sd_vx(rhs)),                  \
            sd_float_##base(sd_vy(lhs), sd_vy(rhs)),                  \
            sd_float_##base(sd_vz(lhs), sd_vz(rhs))                   \
        );                                                            \
    }                                                                 \
                                                                      \
    static inline sd_vec4 sd_vec4_##name(sd_vec4 lhs, sd_vec4 rhs) {  \
        return sd_vec4_create(                                        \
            sd_float_##base(sd_vx(lhs), sd_vx(rhs)),                  \
            sd_float_##base(sd_vy(lhs), sd_vy(rhs)),                  \
            sd_float_##base(sd_vz(lhs), sd_vz(rhs)),                  \
            sd_float_##base(sd_vw(lhs), sd_vw(rhs))                   \
        );                                                            \
    }

#define SD_DEFINE_VECFNS_BINARY_VS(name,base)                          \
    static inline sd_vec2 sd_vec2_##name(sd_vec2 lhs, sd_float rhs) {  \
        return sd_vec2_create(                                         \
            sd_float_##base(sd_vx(lhs), rhs),                          \
            sd_float_##base(sd_vy(lhs), rhs)                           \
        );                                                             \
    }                                                                  \
                                                                       \
    static inline sd_vec3 sd_vec3_##name(sd_vec3 lhs, sd_float rhs) {  \
        return sd_vec3_create(                                         \
            sd_float_##base(sd_vx(lhs), rhs),                          \
            sd_float_##base(sd_vy(lhs), rhs),                          \
            sd_float_##base(sd_vz(lhs), rhs)                           \
        );                                                             \
    }                                                                  \
                                                                       \
    static inline sd_vec4 sd_vec4_##name(sd_vec4 lhs, sd_float rhs) {  \
        return sd_vec4_create(                                         \
            sd_float_##base(sd_vx(lhs), rhs),                          \
            sd_float_##base(sd_vy(lhs), rhs),                          \
            sd_float_##base(sd_vz(lhs), rhs),                          \
            sd_float_##base(sd_vw(lhs), rhs)                           \
        );                                                             \
    }

#define SD_DEFINE_VECFNS_UNARY(name,base)              \
    static inline sd_vec2 sd_vec2_##name(sd_vec2 v) {  \
        return sd_vec2_create(                         \
            sd_float_##base(sd_vx(v)),                 \
            sd_float_##base(sd_vy(v))                  \
        );                                             \
    }                                                  \
                                                       \
    static inline sd_vec3 sd_vec3_##name(sd_vec3 v) {  \
        return sd_vec3_create(                         \
            sd_float_##base(sd_vx(v)),                 \
            sd_float_##base(sd_vy(v)),                 \
            sd_float_##base(sd_vz(v))                  \
        );                                             \
    }                                                  \
                                                       \
    static inline sd_vec4 sd_vec4_##name(sd_vec4 v) {  \
        return sd_vec4_create(                         \
            sd_float_##base(sd_vx(v)),                 \
            sd_float_##base(sd_vy(v)),                 \
            sd_float_##base(sd_vz(v)),                 \
            sd_float_##base(sd_vw(v))                  \
        );                                             \
    }

#define SD_DEFINE_VECFNS_NULLARY(name,base)       \
    static inline sd_vec2 sd_vec2_##name(void) {  \
        return sd_vec2_create(                    \
            sd_float_##base(),                    \
            sd_float_##base()                     \
        );                                        \
    }                                             \
                                                  \
    static inline sd_vec3 sd_vec3_##name(void) {  \
        return sd_vec3_create(                    \
            sd_float_##base(),                    \
            sd_float_##base(),                    \
            sd_float_##base()                     \
        );                                        \
    }                                             \
                                                  \
    static inline sd_vec4 sd_vec4_##name(void) {  \
        return sd_vec4_create(                    \
            sd_float_##base(),                    \
            sd_float_##base(),                    \
            sd_float_##base(),                    \
            sd_float_##base()                     \
        );                                        \
    }

#ifdef __ARM_FEATURE_SVE
    static constexpr size_t SD_ALIGN = alignof(float);
#else
    static constexpr size_t SD_ALIGN = alignof(sd_float);
#endif

static inline size_t sd_length(void) {
#ifdef __ARM_FEATURE_SVE
    return svcntw();
#else
    return sizeof(sd_float) / sizeof(unsigned char [4]);
#endif
}

static inline size_t sd_log_length(void) {
#ifdef __AVX512F__
    return 4;
#elifdef __AVX2__
    return 3;
#elifdef __SSE2__
    return 2;
#elifdef __ARM_FEATURE_SVE
    return __builtin_ctz(svcntw());
#elifdef __ARM_NEON
    return 2;
#else
    return 0;
#endif
}

static inline size_t sd_qot(size_t n) {
    return n >> sd_log_length();
}

static inline size_t sd_rem(size_t n) {
    return n & (sd_length() - 1);
}

static inline size_t sd_bounding_length(size_t n) {
    return n ? sd_qot(n - 1) + 1 : 0;
}

static inline size_t sd_bounding_size(size_t n) {
    return sd_bounding_length(n) * sd_length() * sizeof(unsigned char [4]);
}

static inline sd_vec2 sd_vec2_create(sd_float x, sd_float y) {
#ifdef __ARM_FEATURE_SVE
    return svcreate2(x, y);
#else
    return (sd_vec2){x, y};
#endif
}

static inline sd_vec3 sd_vec3_create(sd_float x, sd_float y, sd_float z) {
#ifdef __ARM_FEATURE_SVE
    return svcreate3(x, y, z);
#else
    return (sd_vec3){x, y, z};
#endif
}

static inline sd_vec4 sd_vec4_create(sd_float x, sd_float y, sd_float z, sd_float w) {
#ifdef __ARM_FEATURE_SVE
    return svcreate4(x, y, z, w);
#else
    return (sd_vec4){x, y, z, w};
#endif
}

static inline sd_mask sd_mask_set(bool b) {
#ifdef __AVX512F__
    return _mm512_int2mask(-(b > 0));
#elifdef __AVX2__
    return _mm256_cmpgt_epi32(_mm256_set1_epi32(b), _mm256_setzero_si256());
#elifdef __SSE2__
    return _mm_cmpgt_epi32(_mm_set1_epi32(b), _mm_setzero_si128());
#elifdef __ARM_FEATURE_SVE
    return svdup_b32(b);
#elifdef __ARM_NEON
    uint32x4_t tst = vdupq_n_u32(b);
    return vtstq_u32(tst, tst);
#else
    return b;
#endif
}

static inline sd_mask sd_mask_and(sd_mask lhs, sd_mask rhs) {
#ifdef __AVX512F__
    return _mm512_kand(lhs, rhs);
#elifdef __AVX2__
    return _mm256_and_si256(lhs, rhs);
#elifdef __SSE2__
    return _mm_and_si128(lhs, rhs);
#elifdef __ARM_FEATURE_SVE
    return svand_z(svptrue_b32(), lhs, rhs);
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
#elifdef __ARM_FEATURE_SVE
    return svorr_z(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vorrq_u32(lhs, rhs);
#else
    return lhs || rhs;
#endif
}

static inline sd_mask sd_mask_not(sd_mask m) {
#ifdef __AVX512F__
    return _mm512_knot(m);
#elifdef __AVX2__
    return _mm256_xor_si256(m, _mm256_set1_epi32(-1));
#elifdef __SSE2__
    return _mm_xor_si128(m, _mm_set1_epi32(-1));
#elifdef __ARM_FEATURE_SVE
    return svnot_z(svptrue_b32(), m);
#elifdef __ARM_NEON
    return vmvnq_u32(m);
#else
    return !m;
#endif
}

static inline sd_mask sd_mask_andn(sd_mask lhs, sd_mask rhs) {
#ifdef __AVX512F__
    return _mm512_kandn(rhs, lhs);
#elifdef __AVX2__
    return _mm256_andnot_si256(rhs, lhs);
#elifdef __SSE2__
    return _mm_andnot_si128(rhs, lhs);
#elifdef __ARM_FEATURE_SVE
    return svbic_z(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vbicq_u32(lhs, rhs);
#else
    return lhs && !rhs;
#endif
}

static inline sd_int sd_int_set(int32_t i) {
#ifdef __AVX512F__
    return _mm512_set1_epi32(i);
#elifdef __AVX2__
    return _mm256_set1_epi32(i);
#elifdef __SSE2__
    return _mm_set1_epi32(i);
#elifdef __ARM_FEATURE_SVE
    return svdup_s32(i);
#elifdef __ARM_NEON
    return vdupq_n_s32(i);
#else
    return i;
#endif
}

static inline sd_int sd_int_add(sd_int lhs, sd_int rhs) {
#ifdef __AVX512F__
    return _mm512_add_epi32(lhs, rhs);
#elifdef __AVX2__
    return _mm256_add_epi32(lhs, rhs);
#elifdef __SSE2__
    return _mm_add_epi32(lhs, rhs);
#elifdef __ARM_FEATURE_SVE
    return svadd_x(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vaddq_s32(lhs, rhs);
#else
    return lhs + rhs;
#endif
}

static inline sd_int sd_int_sub(sd_int lhs, sd_int rhs) {
#ifdef __AVX512F__
    return _mm512_sub_epi32(lhs, rhs);
#elifdef __AVX2__
    return _mm256_sub_epi32(lhs, rhs);
#elifdef __SSE2__
    return _mm_sub_epi32(lhs, rhs);
#elifdef __ARM_FEATURE_SVE
    return svsub_x(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vsubq_s32(lhs, rhs);
#else
    return lhs - rhs;
#endif
}

static inline sd_int sd_int_mul(sd_int lhs, sd_int rhs) {
#ifdef __AVX512F__
    return _mm512_mullo_epi32(lhs, rhs);
#elifdef __AVX2__
    return _mm256_mullo_epi32(lhs, rhs);
#elifdef __SSE2__
    __m128i lhs31 = _mm_shuffle_epi32(lhs, 0b00'11'00'01);
    __m128i rhs31 = _mm_shuffle_epi32(rhs, 0b00'11'00'01);

    __m128i mul20 = _mm_mul_epu32(lhs, rhs);
            mul20 = _mm_shuffle_epi32(mul20, 0b00'00'10'00);
    __m128i mul31 = _mm_mul_epu32(lhs31, rhs31);
            mul31 = _mm_shuffle_epi32(mul31, 0b00'00'10'00);

    return _mm_unpacklo_epi32(mul20, mul31);
#elifdef __ARM_FEATURE_SVE
    return svmul_x(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vmulq_s32(lhs, rhs);
#else
    return lhs * rhs;
#endif
}

static inline sd_mask sd_int_lt(sd_int lhs, sd_int rhs) {
#ifdef __AVX512F__
    return _mm512_cmplt_epi32_mask(lhs, rhs);
#elifdef __AVX2__
    return _mm256_cmpgt_epi32(rhs, lhs);
#elifdef __SSE2__
    return _mm_cmplt_epi32(lhs, rhs);
#elifdef __ARM_FEATURE_SVE
    return svcmplt(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vcltq_s32(lhs, rhs);
#else
    return lhs < rhs;
#endif
}

static inline sd_mask sd_int_gt(sd_int lhs, sd_int rhs) {
#ifdef __AVX512F__
    return _mm512_cmpgt_epi32_mask(lhs, rhs);
#elifdef __AVX2__
    return _mm256_cmpgt_epi32(lhs, rhs);
#elifdef __SSE2__
    return _mm_cmpgt_epi32(lhs, rhs);
#elifdef __ARM_FEATURE_SVE
    return svcmpgt(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vcgtq_s32(lhs, rhs);
#else
    return lhs > rhs;
#endif
}

static inline sd_int sd_int_and(sd_int lhs, sd_int rhs) {
#ifdef __AVX512F__
    return _mm512_and_si512(lhs, rhs);
#elifdef __AVX2__
    return _mm256_and_si256(lhs, rhs);
#elifdef __SSE2__
    return _mm_and_si128(lhs, rhs);
#elifdef __ARM_FEATURE_SVE
    return svand_x(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vandq_s32(lhs, rhs);
#else
    return lhs & rhs;
#endif
}

static inline sd_int sd_int_or(sd_int lhs, sd_int rhs) {
#ifdef __AVX512F__
    return _mm512_or_si512(lhs, rhs);
#elifdef __AVX2__
    return _mm256_or_si256(lhs, rhs);
#elifdef __SSE2__
    return _mm_or_si128(lhs, rhs);
#elifdef __ARM_FEATURE_SVE
    return svorr_x(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vorrq_s32(lhs, rhs);
#else
    return lhs | rhs;
#endif
}

static inline sd_int sd_int_shl(sd_int i, int shift) {
#ifdef __AVX512F__
    return _mm512_slli_epi32(i, shift);
#elifdef __AVX2__
    return _mm256_slli_epi32(i, shift);
#elifdef __SSE2__
    return _mm_slli_epi32(i, shift);
#elifdef __ARM_FEATURE_SVE
    return svlsl_x(svptrue_b32(), i, shift);
#elifdef __ARM_NEON
    int32x4_t shiftv = vdupq_n_s32(shift);
    return vshlq_s32(i, shiftv);
#else
    return i << shift;
#endif
}

static inline sd_int sd_int_shr(sd_int i, int shift) {
#ifdef __AVX512F__
    return _mm512_srai_epi32(i, shift);
#elifdef __AVX2__
    return _mm256_srai_epi32(i, shift);
#elifdef __SSE2__
    return _mm_srai_epi32(i, shift);
#elifdef __ARM_FEATURE_SVE
    return svasr_x(svptrue_b32(), i, shift);
#elifdef __ARM_NEON
    int32x4_t shiftv = vdupq_n_s32(-shift);
    return vshlq_s32(i, shiftv);
#else
    return i >> shift;
#endif
}

static inline sd_int sd_int_mask_blend(sd_int bg, sd_int fg, sd_mask mask) {
#ifdef __AVX512F__
    return _mm512_mask_blend_epi32(mask, bg, fg);
#elifdef __AVX2__
    __m256i select_bg = _mm256_andnot_si256(mask, bg);
    __m256i select_fg = _mm256_and_si256(mask, fg);
    return _mm256_or_si256(select_bg, select_fg);
#elifdef __SSE2__
    __m128i select_bg = _mm_andnot_si128(mask, bg);
    __m128i select_fg = _mm_and_si128(mask, fg);
    return _mm_or_si128(select_bg, select_fg);
#elifdef __ARM_FEATURE_SVE
    return svsel(mask, fg, bg);
#elifdef __ARM_NEON
    return vbslq_s32(mask, fg, bg);
#else
    return mask ? fg : bg;
#endif
}

static inline sd_int sd_int_gather_u8(const uint8_t *buf, sd_int index) {
#ifdef __AVX512F__
    return _mm512_i32gather_epi32(index, buf, 1);
#elifdef __AVX2__
    return _mm256_i32gather_epi32((int const *)buf, index, 1);
#elifdef __SSE2__
    alignas(SD_ALIGN) int32_t elems[4], idxs[4];
    SDL_memcpy(idxs, &index, sizeof(sd_int));

    for (int i = 0; i < 4; ++i)
        elems[i] = buf[idxs[i]];

    sd_int out;
    SDL_memcpy(&out, elems, sizeof(sd_int));
    return out;
#elifdef __ARM_FEATURE_SVE
    return svld1ub_gather_offset_s32(svptrue_b32(), buf, index);
#elifdef __ARM_NEON
    return (sd_int){buf[index[0]], buf[index[1]], buf[index[2]], buf[index[3]]};
#else
    return buf[index];
#endif
}

static inline int32_t sd_int_loads(sd_int *src, size_t index) {
    return ((int32_t *)src)[index];
}

static inline void sd_int_store(sd_int *dst, size_t index, sd_int i) {
#ifdef __AVX512F__
    _mm512_store_si512(dst + index, i);
#elifdef __AVX2__
    _mm256_store_si256(dst + index, i);
#elifdef __SSE2__
    _mm_store_si128(dst + index, i);
#elifdef __ARM_FEATURE_SVE
    svst1_vnum(svptrue_b32(), (int32_t *)dst, index, i);
#elifdef __ARM_NEON
    vst1q_s32((int32_t *)dst + index * sd_length(), i);
#else
    dst[index] = i;
#endif
}

static inline void sd_int_storeu(int32_t *dst, sd_int i) {
#ifdef __AVX512F__
    _mm512_storeu_epi32(dst, i);
#elifdef __AVX2__
    _mm256_storeu_si256((__m256i *)dst, i);
#elifdef __SSE2__
    _mm_storeu_si128((__m128i *)dst, i);
#elifdef __ARM_FEATURE_SVE
    svst1(svptrue_b32(), dst, i);
#elifdef __ARM_NEON
    *((int32x4_t *)dst) = i;
#else
    *dst = i;
#endif
}

static inline sd_int sd_float_to_int(sd_float f) {
#ifdef __AVX512F__
    return _mm512_cvttps_epi32(f);
#elifdef __AVX2__
    return _mm256_cvttps_epi32(f);
#elifdef __SSE2__
    return _mm_cvttps_epi32(f);
#elifdef __ARM_FEATURE_SVE
    return svcvt_s32_x(svptrue_b32(), f);
#elifdef __ARM_NEON
    return vcvtq_s32_f32(f);
#else
    return (int32_t)f;
#endif
}

static inline sd_float sd_int_to_float(sd_int i) {
#ifdef __AVX512F__
    return _mm512_cvtepi32_ps(i);
#elifdef __AVX2__
    return _mm256_cvtepi32_ps(i);
#elifdef __SSE2__
    return _mm_cvtepi32_ps(i);
#elifdef __ARM_FEATURE_SVE
    return svcvt_f32_x(svptrue_b32(), i);
#elifdef __ARM_NEON
    return vcvtq_f32_s32(i);
#else
    return (float)i;
#endif
}

static inline sd_float sd_float_set(float f) {
#ifdef __AVX512F__
    return _mm512_set1_ps(f);
#elifdef __AVX2__
    return _mm256_set1_ps(f);
#elifdef __SSE2__
    return _mm_set1_ps(f);
#elifdef __ARM_FEATURE_SVE
    return svdup_f32(f);
#elifdef __ARM_NEON
    return vdupq_n_f32(f);
#else
    return f;
#endif
}

static inline sd_vec2 sd_vec2_set(float x, float y) {
    return sd_vec2_create(
        sd_float_set(x),
        sd_float_set(y)
    );
}

static inline sd_vec3 sd_vec3_set(float x, float y, float z) {
    return sd_vec3_create(
        sd_float_set(x),
        sd_float_set(y),
        sd_float_set(z)
    );
}

static inline sd_vec4 sd_vec4_set(float x, float y, float z, float w) {
    return sd_vec4_create(
        sd_float_set(x),
        sd_float_set(y),
        sd_float_set(z),
        sd_float_set(w)
    );
}

static inline sd_float sd_float_zero(void) {
    return sd_float_set(0);
}

SD_DEFINE_VECFNS_NULLARY(zero, zero)

static inline sd_float sd_float_one(void) {
    return sd_float_set(1);
}

SD_DEFINE_VECFNS_NULLARY(one, one)

static inline sd_float sd_float_add(sd_float lhs, sd_float rhs) {
#ifdef __AVX512F__
    return _mm512_add_ps(lhs, rhs);
#elifdef __AVX2__
    return _mm256_add_ps(lhs, rhs);
#elifdef __SSE2__
    return _mm_add_ps(lhs, rhs);
#elifdef __ARM_FEATURE_SVE
    return svadd_x(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vaddq_f32(lhs, rhs);
#else
    return lhs + rhs;
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(add, add)
SD_DEFINE_VECFNS_BINARY_VS(adds, add)

static inline sd_float sd_float_sub(sd_float lhs, sd_float rhs) {
#ifdef __AVX512F__
    return _mm512_sub_ps(lhs, rhs);
#elifdef __AVX2__
    return _mm256_sub_ps(lhs, rhs);
#elifdef __SSE2__
    return _mm_sub_ps(lhs, rhs);
#elifdef __ARM_FEATURE_SVE
    return svsub_x(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vsubq_f32(lhs, rhs);
#else
    return lhs - rhs;
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(sub, sub)
SD_DEFINE_VECFNS_BINARY_VS(subs, sub)

static inline sd_float sd_float_mul(sd_float lhs, sd_float rhs) {
#ifdef __AVX512F__
    return _mm512_mul_ps(lhs, rhs);
#elifdef __AVX2__
    return _mm256_mul_ps(lhs, rhs);
#elifdef __SSE2__
    return _mm_mul_ps(lhs, rhs);
#elifdef __ARM_FEATURE_SVE
    return svmul_x(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vmulq_f32(lhs, rhs);
#else
    return lhs * rhs;
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(mul, mul)
SD_DEFINE_VECFNS_BINARY_VS(muls, mul)

static inline sd_float sd_float_abs(sd_float f) {
#ifdef __AVX512F__
    return _mm512_abs_ps(f);
#elifdef __AVX2__
    __m256i minus_one = _mm256_set1_epi32(-1);
    __m256 abs_mask = _mm256_castsi256_ps(_mm256_srli_epi32(minus_one, 1));
    return _mm256_and_ps(f, abs_mask);
#elifdef __SSE2__
    __m128i minus_one = _mm_set1_epi32(-1);
    __m128 abs_mask = _mm_castsi128_ps(_mm_srli_epi32(minus_one, 1));
    return _mm_and_ps(f, abs_mask);
#elifdef __ARM_FEATURE_SVE
    return svabs_x(svptrue_b32(), f);
#elifdef __ARM_NEON
    return vabsq_f32(f);
#else
    return SDL_fabsf(f);
#endif
}

SD_DEFINE_VECFNS_UNARY(abs, abs)

static inline sd_float sd_float_negate(sd_float f) {
#ifdef __AVX512F__
    return _mm512_mul_ps(f, _mm512_set1_ps(-1));
#elifdef __AVX2__
    return _mm256_mul_ps(f, _mm256_set1_ps(-1));
#elifdef __SSE2__
    return _mm_mul_ps(f, _mm_set1_ps(-1));
#elifdef __ARM_FEATURE_SVE
    return svneg_x(svptrue_b32(), f);
#elifdef __ARM_NEON
    return vnegq_f32(f);
#else
    return -f;
#endif
}

SD_DEFINE_VECFNS_UNARY(negate, negate)

static inline sd_float sd_float_rcp(sd_float f) {
#ifdef __AVX512F__
    __m512 est = _mm512_rcp14_ps(f);
    __m512 two = _mm512_set1_ps(2);
    return _mm512_mul_ps(est, _mm512_fnmadd_ps(f, est, two));
#elifdef __AVX2__
    __m256 est = _mm256_rcp_ps(f);
    __m256 two = _mm256_set1_ps(2);
    return _mm256_mul_ps(est, _mm256_fnmadd_ps(f, est, two));
#elifdef __SSE2__
    __m128 est = _mm_rcp_ps(f);
    __m128 rhs = _mm_mul_ps(f, _mm_mul_ps(est, est));
    return _mm_sub_ps(_mm_add_ps(est, est), rhs);
#elifdef __ARM_FEATURE_SVE
    svfloat32_t est = svrecpe(f);
    return svmul_x(svptrue_b32(), est, svrecps(f, est));
#elifdef __ARM_NEON
    float32x4_t est = vrecpeq_f32(f);
    return vmulq_f32(est, vrecpsq_f32(f, est));
#else
    return 1 / f;
#endif
}

SD_DEFINE_VECFNS_UNARY(rcp, rcp)

static inline sd_float sd_float_rsqrt(sd_float f) {
#ifdef __AVX512F__
    __m512 est = _mm512_rsqrt14_ps(f);
    __m512 hlfest = _mm512_mul_ps(est, _mm512_set1_ps(0.5));
    __m512 sqrest = _mm512_mul_ps(est, est);
    __m512 three = _mm512_set1_ps(3);
    return _mm512_mul_ps(hlfest, _mm512_fnmadd_ps(f, sqrest, three));
#elifdef __AVX2__
    __m256 est = _mm256_rsqrt_ps(f);
    __m256 hlfest = _mm256_mul_ps(est, _mm256_set1_ps(0.5));
    __m256 sqrest = _mm256_mul_ps(est, est);
    __m256 three = _mm256_set1_ps(3);
    return _mm256_mul_ps(hlfest, _mm256_fnmadd_ps(f, sqrest, three));
#elifdef __SSE2__
    __m128 est = _mm_rsqrt_ps(f);
    __m128 hlfest = _mm_mul_ps(est, _mm_set1_ps(0.5));
    __m128 sqrest = _mm_mul_ps(est, est);
    __m128 three = _mm_set1_ps(3);
    __m128 rhs = _mm_sub_ps(three, _mm_mul_ps(f, sqrest));
    return _mm_mul_ps(hlfest, rhs);
#elifdef __ARM_FEATURE_SVE
    svfloat32_t est = svrsqrte(f);
    svfloat32_t sqrest = svmul_x(svptrue_b32(), est, est);
    return svmul_x(svptrue_b32(), est, svrsqrts(f, sqrest));
#elifdef __ARM_NEON
    float32x4_t est = vrsqrteq_f32(f);
    float32x4_t sqrest = vmulq_f32(est, est);
    return vmulq_f32(est, vrsqrtsq_f32(f, sqrest));
#else
    return 1 / SDL_sqrtf(f);
#endif
}

SD_DEFINE_VECFNS_UNARY(rsqrt, rsqrt)

static inline sd_float sd_float_trunc(sd_float f) {
    return sd_int_to_float(sd_float_to_int(f));
}

SD_DEFINE_VECFNS_UNARY(trunc, trunc)

static inline sd_float sd_float_frac(sd_float f) {
    return sd_float_sub(f, sd_float_trunc(f));
}

SD_DEFINE_VECFNS_UNARY(frac, frac)

static inline sd_float sd_float_fmadd(sd_float multiplicand, sd_float multiplier, sd_float addend) {
#ifdef __AVX512F__
    return _mm512_fmadd_ps(multiplicand, multiplier, addend);
#elifdef __AVX2__
    return _mm256_fmadd_ps(multiplicand, multiplier, addend);
#elifdef __ARM_FEATURE_SVE
    return svmad_x(svptrue_b32(), multiplicand, multiplier, addend);
#elifdef __ARM_NEON
    return vmlaq_f32(addend, multiplicand, multiplier);
#else
    return sd_float_add(sd_float_mul(multiplicand, multiplier), addend);
#endif
}

static inline sd_vec2 sd_vec2_fsmadd(sd_vec2 multiplicand, sd_float multiplier, sd_vec2 addend) {
    return sd_vec2_create(
        sd_float_fmadd(sd_vx(multiplicand), multiplier, sd_vx(addend)),
        sd_float_fmadd(sd_vy(multiplicand), multiplier, sd_vy(addend))
    );
}

static inline sd_vec3 sd_vec3_fsmadd(sd_vec3 multiplicand, sd_float multiplier, sd_vec3 addend) {
    return sd_vec3_create(
        sd_float_fmadd(sd_vx(multiplicand), multiplier, sd_vx(addend)),
        sd_float_fmadd(sd_vy(multiplicand), multiplier, sd_vy(addend)),
        sd_float_fmadd(sd_vz(multiplicand), multiplier, sd_vz(addend))
    );
}

static inline sd_vec4 sd_vec4_fsmadd(sd_vec4 multiplicand, sd_float multiplier, sd_vec4 addend) {
    return sd_vec4_create(
        sd_float_fmadd(sd_vx(multiplicand), multiplier, sd_vx(addend)),
        sd_float_fmadd(sd_vy(multiplicand), multiplier, sd_vy(addend)),
        sd_float_fmadd(sd_vz(multiplicand), multiplier, sd_vz(addend)),
        sd_float_fmadd(sd_vw(multiplicand), multiplier, sd_vw(addend))
    );
}

static inline sd_float sd_float_fmsub(sd_float multiplicand, sd_float multiplier, sd_float subtrahend) {
#ifdef __AVX512F__
    return _mm512_fmsub_ps(multiplicand, multiplier, subtrahend);
#elifdef __AVX2__
    return _mm256_fmsub_ps(multiplicand, multiplier, subtrahend);
#elifdef __ARM_FEATURE_SVE
    return svnmsb_x(svptrue_b32(), multiplicand, multiplier, subtrahend);
#elifdef __ARM_NEON
    return vmlaq_f32(vnegq_f32(subtrahend), multiplicand, multiplier);
#else
    return sd_float_sub(sd_float_mul(multiplicand, multiplier), subtrahend);
#endif
}

static inline sd_float sd_float_blend(sd_float bg, sd_float fg, sd_float coeff) {
    return sd_float_fmadd(sd_float_sub(fg, bg), coeff, bg);
}

static inline sd_vec2 sd_vec2_blend(sd_vec2 bg, sd_vec2 fg, sd_float coeff) {
    return sd_vec2_fsmadd(sd_vec2_sub(fg, bg), coeff, bg);
}

static inline sd_vec3 sd_vec3_blend(sd_vec3 bg, sd_vec3 fg, sd_float coeff) {
    return sd_vec3_fsmadd(sd_vec3_sub(fg, bg), coeff, bg);
}

static inline sd_vec4 sd_vec4_blend(sd_vec4 bg, sd_vec4 fg, sd_float coeff) {
    return sd_vec4_fsmadd(sd_vec4_sub(fg, bg), coeff, bg);
}

static inline sd_float sd_float_min(sd_float lhs, sd_float rhs) {
#ifdef __AVX512F__
    return _mm512_min_ps(lhs, rhs);
#elifdef __AVX2__
    return _mm256_min_ps(lhs, rhs);
#elifdef __SSE2__
    return _mm_min_ps(lhs, rhs);
#elifdef __ARM_FEATURE_SVE
    return svmin_x(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vminq_f32(lhs, rhs);
#else
    return SDL_min(lhs, rhs);
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(min, min)
SD_DEFINE_VECFNS_BINARY_VS(mins, min)

static inline sd_float sd_float_max(sd_float lhs, sd_float rhs) {
#ifdef __AVX512F__
    return _mm512_max_ps(lhs, rhs);
#elifdef __AVX2__
    return _mm256_max_ps(lhs, rhs);
#elifdef __SSE2__
    return _mm_max_ps(lhs, rhs);
#elifdef __ARM_FEATURE_SVE
    return svmax_x(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vmaxq_f32(lhs, rhs);
#else
    return SDL_max(lhs, rhs);
#endif
}

SD_DEFINE_VECFNS_BINARY_VV(max, max)
SD_DEFINE_VECFNS_BINARY_VS(maxs, max)

static inline sd_float sd_float_clamp(sd_float f, sd_float min, sd_float max) {
    return sd_float_min(sd_float_max(f, min), max);
}

static inline sd_vec2 sd_vec2_clamp(sd_vec2 v, sd_float min, sd_float max) {
    return sd_vec2_create(
        sd_float_clamp(sd_vx(v), min, max),
        sd_float_clamp(sd_vy(v), min, max)
    );
}

static inline sd_vec3 sd_vec3_clamp(sd_vec3 v, sd_float min, sd_float max) {
    return sd_vec3_create(
        sd_float_clamp(sd_vx(v), min, max),
        sd_float_clamp(sd_vy(v), min, max),
        sd_float_clamp(sd_vz(v), min, max)
    );
}

static inline sd_vec4 sd_vec4_clamp(sd_vec4 v, sd_float min, sd_float max) {
    return sd_vec4_create(
        sd_float_clamp(sd_vx(v), min, max),
        sd_float_clamp(sd_vy(v), min, max),
        sd_float_clamp(sd_vz(v), min, max),
        sd_float_clamp(sd_vw(v), min, max)
    );
}

static inline sd_mask sd_float_lt(sd_float lhs, sd_float rhs) {
#ifdef __AVX512F__
    return _mm512_cmp_ps_mask(lhs, rhs, _CMP_LT_OQ);
#elifdef __AVX2__
    return _mm256_castps_si256(_mm256_cmp_ps(lhs, rhs, _CMP_LT_OQ));
#elifdef __SSE2__
    return _mm_castps_si128(_mm_cmplt_ps(lhs, rhs));
#elifdef __ARM_FEATURE_SVE
    return svcmplt(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vcltq_f32(lhs, rhs);
#else
    return lhs < rhs;
#endif
}

static inline sd_mask sd_float_gt(sd_float lhs, sd_float rhs) {
#ifdef __AVX512F__
    return _mm512_cmp_ps_mask(lhs, rhs, _CMP_GT_OQ);
#elifdef __AVX2__
    return _mm256_castps_si256(_mm256_cmp_ps(lhs, rhs, _CMP_GT_OQ));
#elifdef __SSE2__
    return _mm_castps_si128(_mm_cmpgt_ps(lhs, rhs));
#elifdef __ARM_FEATURE_SVE
    return svcmpgt(svptrue_b32(), lhs, rhs);
#elifdef __ARM_NEON
    return vcgtq_f32(lhs, rhs);
#else
    return lhs > rhs;
#endif
}

static inline sd_mask sd_float_between(sd_float f, float min, float max) {
    return sd_mask_and(sd_float_gt(f, sd_float_set(min)), sd_float_lt(f, sd_float_set(max)));
}

static inline sd_float sd_float_mask_blend(sd_float bg, sd_float fg, sd_mask mask) {
#ifdef __AVX512F__
    return _mm512_mask_blend_ps(mask, bg, fg);
#elifdef __AVX2__
    return _mm256_blendv_ps(bg, fg, _mm256_castsi256_ps(mask));
#elifdef __SSE2__
    __m128 select_bg = _mm_andnot_ps(_mm_castsi128_ps(mask), bg);
    __m128 select_fg = _mm_and_ps(_mm_castsi128_ps(mask), fg);
    return _mm_or_ps(select_bg, select_fg);
#elifdef __ARM_FEATURE_SVE
    return svsel(mask, fg, bg);
#elifdef __ARM_NEON
    return vbslq_f32(mask, fg, bg);
#else
    return mask ? fg : bg;
#endif
}

static inline sd_vec2 sd_vec2_mask_blend(sd_vec2 bg, sd_vec2 fg, sd_mask mask) {
    return sd_vec2_create(
        sd_float_mask_blend(sd_vx(bg), sd_vx(fg), mask),
        sd_float_mask_blend(sd_vy(bg), sd_vy(fg), mask)
    );
}

static inline sd_vec3 sd_vec3_mask_blend(sd_vec3 bg, sd_vec3 fg, sd_mask mask) {
    return sd_vec3_create(
        sd_float_mask_blend(sd_vx(bg), sd_vx(fg), mask),
        sd_float_mask_blend(sd_vy(bg), sd_vy(fg), mask),
        sd_float_mask_blend(sd_vz(bg), sd_vz(fg), mask)
    );
}

static inline sd_vec4 sd_vec4_mask_blend(sd_vec4 bg, sd_vec4 fg, sd_mask mask) {
    return sd_vec4_create(
        sd_float_mask_blend(sd_vx(bg), sd_vx(fg), mask),
        sd_float_mask_blend(sd_vy(bg), sd_vy(fg), mask),
        sd_float_mask_blend(sd_vz(bg), sd_vz(fg), mask),
        sd_float_mask_blend(sd_vw(bg), sd_vw(fg), mask)
    );
}

static inline sd_float sd_float_range(void) {
#ifdef __AVX512F__
    return _mm512_set_ps(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
#elifdef __AVX2__
    return _mm256_set_ps(7, 6, 5, 4, 3, 2, 1, 0);
#elifdef __SSE2__
    return _mm_set_ps(3, 2, 1, 0);
#elifdef __ARM_FEATURE_SVE
    return svcvt_f32_x(svptrue_b32(), svindex_s32(0, 1));
#elifdef __ARM_NEON
    return (sd_float){0,1,2,3};
#else
    return 0;
#endif
}

SD_DEFINE_VECFNS_NULLARY(range, range)

static inline sd_float sd_float_gather(float *buf, sd_int index) {
#ifdef __AVX512F__
    return _mm512_i32gather_ps(index, buf, 4);
#elifdef __AVX2__
    return _mm256_i32gather_ps(buf, index, 4);
#elifdef __SSE2__
    alignas(SD_ALIGN) float elems[4];
    alignas(SD_ALIGN) int32_t idxs[4];
    SDL_memcpy(idxs, &index, sizeof(sd_int));

    for (int i = 0; i < 4; ++i)
        elems[i] = buf[idxs[i]];

    sd_float out;
    SDL_memcpy(&out, elems, sizeof(sd_float));
    return out;
#elifdef __ARM_FEATURE_SVE
    return svld1_gather_index(svptrue_b32(), buf, index);
#elifdef __ARM_NEON
    return (sd_float){buf[index[0]], buf[index[1]], buf[index[2]], buf[index[3]]};
#else
    return buf[index];
#endif
}

static inline sd_vec3 sd_vec3_gather_strided(sd_vec3 *buf, sd_int index) {
    sd_int sd_qot = sd_int_shr(index, sd_log_length());
    sd_int sd_rem = sd_int_and(index, sd_int_set(sd_length() - 1));
    sd_int sd_idx = sd_int_add(sd_qot, sd_int_shl(sd_qot, 1));
           sd_idx = sd_int_shl(sd_idx, sd_log_length());
           sd_idx = sd_int_add(sd_idx, sd_rem);

    return sd_vec3_create(
        sd_float_gather((float *)&buf + sd_length() * 0, sd_idx),
        sd_float_gather((float *)&buf + sd_length() * 1, sd_idx),
        sd_float_gather((float *)&buf + sd_length() * 2, sd_idx)
    );
}

static inline sd_vec4 sd_vec4_gather(float *buf, sd_int index) {
    sd_int base = sd_int_shl(index, 2);

    return sd_vec4_create(
        sd_float_gather(buf + 0, base),
        sd_float_gather(buf + 1, base),
        sd_float_gather(buf + 2, base),
        sd_float_gather(buf + 3, base)
    );
}

static inline sd_float sd_float_load(sd_float *src, size_t index) {
#ifdef __AVX512F__
    return _mm512_load_ps((float *)src + index * sd_length());
#elifdef __AVX2__
    return _mm256_load_ps((float *)src + index * sd_length());
#elifdef __SSE2__
    return _mm_load_ps((float *)src + index * sd_length());
#elifdef __ARM_FEATURE_SVE
    return svld1_vnum(svptrue_b32(), (float *)src, index);
#elifdef __ARM_NEON
    return vld1q_f32((float *)src + index * sd_length());
#else
    return src[index];
#endif
}

static inline sd_vec2 sd_vec2_load(sd_vec2 *src, size_t index) {
    return sd_vec2_create(
        sd_float_load((sd_float *)src, index * 2 + 0),
        sd_float_load((sd_float *)src, index * 2 + 1)
    );
}

static inline sd_vec3 sd_vec3_load(sd_vec3 *src, size_t index) {
    return sd_vec3_create(
        sd_float_load((sd_float *)src, index * 3 + 0),
        sd_float_load((sd_float *)src, index * 3 + 1),
        sd_float_load((sd_float *)src, index * 3 + 2)
    );
}

static inline sd_vec4 sd_vec4_load(sd_vec4 *src, size_t index) {
    return sd_vec4_create(
        sd_float_load((sd_float *)src, index * 4 + 0),
        sd_float_load((sd_float *)src, index * 4 + 1),
        sd_float_load((sd_float *)src, index * 4 + 2),
        sd_float_load((sd_float *)src, index * 4 + 3)
    );
}

static inline void sd_float_store(sd_float *dst, size_t index, sd_float f) {
#ifdef __AVX512F__
    _mm512_store_ps((float *)dst + index * sd_length(), f);
#elifdef __AVX2__
    _mm256_store_ps((float *)dst + index * sd_length(), f);
#elifdef __SSE2__
    _mm_store_ps((float *)dst + index * sd_length(), f);
#elifdef __ARM_FEATURE_SVE
    svst1_vnum(svptrue_b32(), (float *)dst, index, f);
#elifdef __ARM_NEON
    vst1q_f32((float *)dst + index * sd_length(), f);
#else
    dst[index] = f;
#endif
}

static inline void sd_vec2_store(sd_vec2 *src, size_t index, sd_vec2 v) {
    sd_float_store((sd_float *)src, index * 2 + 0, sd_vx(v));
    sd_float_store((sd_float *)src, index * 2 + 1, sd_vy(v));
}

static inline void sd_vec3_store(sd_vec3 *src, size_t index, sd_vec3 v) {
    sd_float_store((sd_float *)src, index * 3 + 0, sd_vx(v));
    sd_float_store((sd_float *)src, index * 3 + 1, sd_vy(v));
    sd_float_store((sd_float *)src, index * 3 + 2, sd_vz(v));
}

static inline void sd_vec4_store(sd_vec4 *src, size_t index, sd_vec4 v) {
    sd_float_store((sd_float *)src, index * 4 + 0, sd_vx(v));
    sd_float_store((sd_float *)src, index * 4 + 1, sd_vy(v));
    sd_float_store((sd_float *)src, index * 4 + 2, sd_vz(v));
    sd_float_store((sd_float *)src, index * 4 + 3, sd_vw(v));
}

static inline float sd_float_loads(sd_float *src, size_t index) {
    return ((float *)src)[index];
}

static inline sd_vec2_scalar sd_vec2_loads(sd_vec2 *src, size_t index) {
    float *sd_base = (float *)src + sd_qot(index) * sd_length() * 2;

    return (sd_vec2_scalar) {
        .x = sd_base[0 * sd_length() + sd_rem(index)],
        .y = sd_base[1 * sd_length() + sd_rem(index)]
    };
}

static inline sd_vec3_scalar sd_vec3_loads(sd_vec3 *src, size_t index) {
    float *sd_base = (float *)src + sd_qot(index) * sd_length() * 3;

    return (sd_vec3_scalar) {
        .x = sd_base[0 * sd_length() + sd_rem(index)],
        .y = sd_base[1 * sd_length() + sd_rem(index)],
        .z = sd_base[2 * sd_length() + sd_rem(index)]
    };
}

static inline sd_vec4_scalar sd_vec4_loads(sd_vec4 *src, size_t index) {
    float *sd_base = (float *)src + sd_qot(index) * sd_length() * 4;
    
    return (sd_vec4_scalar) {
        .x = sd_base[0 * sd_length() + sd_rem(index)],
        .y = sd_base[1 * sd_length() + sd_rem(index)],
        .z = sd_base[2 * sd_length() + sd_rem(index)],
        .w = sd_base[3 * sd_length() + sd_rem(index)]
    };
}

static inline void sd_float_stores(sd_float *dst, size_t index, float x) {
    ((float *)dst)[index] = x;
}

static inline void sd_vec2_stores(sd_vec2 *dst, size_t index, float x, float y) {
    float *sd_base = (float *)dst + sd_qot(index) * sd_length() * 2;
    sd_base[0 * sd_length() + sd_rem(index)] = x;
    sd_base[1 * sd_length() + sd_rem(index)] = y;
}

static inline void sd_vec3_stores(sd_vec3 *dst, size_t index, float x, float y, float z) {
    float *sd_base = (float *)dst + sd_qot(index) * sd_length() * 3;
    sd_base[0 * sd_length() + sd_rem(index)] = x;
    sd_base[1 * sd_length() + sd_rem(index)] = y;
    sd_base[2 * sd_length() + sd_rem(index)] = z;
}

static inline void sd_vec4_stores(sd_vec4 *dst, size_t index, float x, float y, float z, float w) {
    float *sd_base = (float *)dst + sd_qot(index) * sd_length() * 4;
    sd_base[0 * sd_length() + sd_rem(index)] = x;
    sd_base[1 * sd_length() + sd_rem(index)] = y;
    sd_base[2 * sd_length() + sd_rem(index)] = z;
    sd_base[3 * sd_length() + sd_rem(index)] = w;
}

static inline sd_float sd_vec2_dot(sd_vec2 lhs, sd_vec2 rhs) {
    sd_float out = sd_float_mul(sd_vx(lhs), sd_vx(rhs));
    return sd_float_fmadd(sd_vy(lhs), sd_vy(rhs), out);
}

static inline sd_float sd_vec3_dot(sd_vec3 lhs, sd_vec3 rhs) {
    sd_float out = sd_float_mul(sd_vx(lhs), sd_vx(rhs));
    out = sd_float_fmadd(sd_vy(lhs), sd_vy(rhs), out);
    return sd_float_fmadd(sd_vz(lhs), sd_vz(rhs), out);
}

static inline sd_vec3 sd_vec3_cross(sd_vec3 lhs, sd_vec3 rhs) {
    return sd_vec3_create(
        sd_float_fmsub(sd_vy(lhs), sd_vz(rhs), sd_float_mul(sd_vz(lhs), sd_vy(rhs))),
        sd_float_fmsub(sd_vz(lhs), sd_vx(rhs), sd_float_mul(sd_vx(lhs), sd_vz(rhs))),
        sd_float_fmsub(sd_vx(lhs), sd_vy(rhs), sd_float_mul(sd_vy(lhs), sd_vx(rhs)))
    );
}

static inline sd_vec2 sd_vec2_normalize(sd_vec2 v) {
    sd_float sqrlen = sd_vec2_dot(v, v);
    sd_float rcplen = sd_float_rsqrt(sqrlen);
    return sd_vec2_muls(v, rcplen);
}

static inline sd_vec3 sd_vec3_normalize(sd_vec3 v) {
    sd_float sqrlen = sd_vec3_dot(v, v);
    sd_float rcplen = sd_float_rsqrt(sqrlen);
    return sd_vec3_muls(v, rcplen);
}

static inline sd_vec2 sd_vec2_reflect(sd_vec2 v, sd_vec2 nrml) {
    sd_vec2 prj = sd_vec2_muls(nrml, sd_vec2_dot(v, nrml));
    sd_vec2 rej = sd_vec2_sub(v, prj);
    return sd_vec2_add(rej, sd_vec2_negate(prj));
}

static inline sd_vec3 sd_vec3_reflect(sd_vec3 v, sd_vec3 nrml) {
    sd_vec3 prj = sd_vec3_muls(nrml, sd_vec3_dot(v, nrml));
    sd_vec3 rej = sd_vec3_sub(v, prj);
    return sd_vec3_add(rej, sd_vec3_negate(prj));
}

#endif /* STRIDE_H */
