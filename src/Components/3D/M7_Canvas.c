#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/stride.h>
#include <M7/gamma.h>

#if defined(__SSE2__) || defined(__AVX2__)
#include <immintrin.h>
#endif

#if defined(__SSE2__) && !defined(__AVX2__)
static inline __m128i gather_sse2(const uint8_t *buf, __m128i idx) {
    alignas(16) int elems[4];
    alignas(16) int idxs[4];
    SDL_memcpy(idxs, &idx, sizeof(idx));

    elems[0] = buf[idxs[0]];
    elems[1] = buf[idxs[1]];
    elems[2] = buf[idxs[2]];
    elems[3] = buf[idxs[3]];

    __m128i out;
    SDL_memcpy(&out, elems, sizeof(out));
    return out;
}
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>

static inline uint32x4_t gather_neon(const uint8_t *buf, uint32x4_t idx) {
    return (uint32x4_t) { buf[idx[0]], buf[idx[1]], buf[idx[2]], buf[idx[3]] };
}
#endif

typedef struct PresentData {
    ECS_Handle *canvas;
    uint32_t *pixels;
    int start, end;
} PresentData;

static int PresentThread(void *data) {
    PresentData *pd = data;
    M7_Canvas *canvas = ECS_Entity_GetComponent(pd->canvas, M7_Components.Canvas);
    int sd_qot = canvas->width / SD_LENGTH;
    int sd_rem = canvas->width % SD_LENGTH;

    for (int i = pd->start; i < pd->end; ++i) {
        sd_vec3 *base = canvas->color + i * sd_bounding_size(canvas->width);

        for (int j = 0; j < sd_qot; ++j) {
            sd_vec3 col = base[j];
            col = sd_vec3_clamp(col, sd_float_zero(), sd_float_one());
            col = sd_vec3_mul(col, sd_float_set(0xFFFF));
#ifdef __AVX2__
            __m256i byte = _mm256_set1_epi32(0xFF);

            __m256i r = _mm256_cvtps_epi32(col.r.val);
                    r = _mm256_i32gather_epi32((const int *)gamma_encode_lut, r, 1);
                    r = _mm256_slli_epi32(r, 16);
            __m256i g = _mm256_cvtps_epi32(col.g.val);
                    g = _mm256_i32gather_epi32((const int *)gamma_encode_lut, g, 1);
                    g = _mm256_and_si256(g, byte);
                    g = _mm256_slli_epi32(g, 8);
            __m256i b = _mm256_cvtps_epi32(col.b.val);
                    b = _mm256_i32gather_epi32((const int *)gamma_encode_lut, b, 1);
                    b = _mm256_and_si256(b, byte);

            __m256i col_out = _mm256_or_si256(_mm256_or_si256(r, g), b);
            _mm256_storeu_si256((__m256i *)(pd->pixels + i * canvas->width) + j, col_out);
#elifdef __SSE2__
            __m128i byte = _mm_set1_epi32(0xFF);

            __m128i r = _mm_cvtps_epi32(col.r.val);
                    r = gather_sse2(gamma_encode_lut, r);
                    r = _mm_slli_epi32(r, 16);
            __m128i g = _mm_cvtps_epi32(col.g.val);
                    g = gather_sse2(gamma_encode_lut, g);
                    g = _mm_and_si128(g, byte);
                    g = _mm_slli_epi32(g, 8);
            __m128i b = _mm_cvtps_epi32(col.b.val);
                    b = gather_sse2(gamma_encode_lut, b);
                    b = _mm_and_si128(b, byte);

            __m128i col_out = _mm_or_si128(_mm_or_si128(r, g), b);
            _mm_storeu_si128((__m128i *)(pd->pixels + i * canvas->width) + j, col_out);
#elifdef __ARM_NEON
            uint32x4_t byte = vdupq_n_u32(0xFF);

            uint32x4_t r = vcvtq_u32_f32(col.r.val);
                       r = gather_neon(gamma_encode_lut, r);
                       r = vshlq_n_u32(r, 16);
            uint32x4_t g = vcvtq_u32_f32(col.g.val);
                       g = gather_neon(gamma_encode_lut, g);
                       g = vandq_u32(g, byte);
                       g = vshlq_n_u32(g, 8);
            uint32x4_t b = vcvtq_u32_f32(col.b.val);
                       b = gather_neon(gamma_encode_lut, b);
                       b = vandq_u32(b, byte);

            uint32x4_t col_out = vorrq_u32(vorrq_u32(r, g), b);
            *((uint32x4_t *)(pd->pixels + i * canvas->width) + j) = col_out;
#else
            uint16_t r = col.r.val;
                     r = gamma_encode_lut[r];
            uint16_t g = col.g.val;
                     g = gamma_encode_lut[g];
            uint16_t b = col.b.val;
                     b = gamma_encode_lut[b];

            pd->pixels[i * canvas->width + j] = (r << 16) | (g << 8) | b;
#endif
        }

        for (int j = 0; j < sd_rem; ++j) {
            sd_vec3_scalar col = sd_vec3_arr_get(base + sd_qot, j);

            uint16_t r = col.r.val * 0xFFFF;
                     r = gamma_encode_lut[r];
            uint16_t g = col.g.val * 0xFFFF;
                     g = gamma_encode_lut[g];
            uint16_t b = col.b.val * 0xFFFF;
                     b = gamma_encode_lut[b];

            pd->pixels[i * canvas->width + sd_qot * SD_LENGTH + j] = (r << 16) | (g << 8) | b;
        }
    }

    return 0;
}

void SD_VARIANT(M7_Canvas_Present)(ECS_Handle *self) {
    M7_Canvas *canvas = ECS_Entity_GetComponent(self, M7_Components.Canvas);
    M7_Viewport *vp = ECS_Entity_GetComponent(canvas->vp, M7_Components.Viewport);

    uint32_t *pixels;
    int pitch;

    SDL_LockTexture(vp->texture, nullptr, (void **)&pixels, &pitch);

    SDL_Thread **threads = SDL_malloc(sizeof(SDL_Thread *) * canvas->parallelism);
    PresentData *present_data = SDL_malloc(sizeof(PresentData) * canvas->parallelism);

    int qot = canvas->height / canvas->parallelism;
    int rem = canvas->height % canvas->parallelism;

    for (int i = 0; i < canvas->parallelism; ++i) {
        present_data[i] = (PresentData) {
            .canvas = self,
            .pixels = pixels,
            .start = i * qot + SDL_min(i, rem),
            .end = (i + 1) * qot + SDL_min(i + 1, rem)
        };

        threads[i] = SDL_CreateThread(PresentThread, "present", present_data + i);
    }

    for (int i = 0; i < canvas->parallelism; ++i)
        SDL_WaitThread(threads[i], nullptr);

    SDL_free(present_data);
    SDL_free(threads);

    SDL_UnlockTexture(vp->texture);
    SDL_RenderTexture(vp->renderer, vp->texture, nullptr, nullptr);
    SDL_RenderPresent(vp->renderer);
}

void SD_VARIANT(M7_Canvas_Attach)(ECS_Handle *self) {
    M7_Canvas *canvas = ECS_Entity_GetComponent(self, M7_Components.Canvas);
    canvas->vp = ECS_Entity_AncestorWithComponent(self, M7_Components.Viewport, true);
}

void SD_VARIANT(M7_Canvas_Init)(void *component, void *args) {
    M7_Canvas *canvas = component, *cargs = args;

    canvas->width = cargs->width;
    canvas->height = cargs->height;
    canvas->parallelism = cargs->parallelism;

    size_t sd_count = sd_bounding_size(canvas->width) * canvas->height;
    canvas->color = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_count);
    canvas->depth = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_float) * sd_count);
}

#ifdef SD_BASE

void M7_Canvas_Free(void *component) {
    M7_Canvas *canvas = component;

    SDL_aligned_free(canvas->color);
    SDL_aligned_free(canvas->depth);
}

#endif /* SD_BASE */
