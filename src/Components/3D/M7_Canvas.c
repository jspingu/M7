#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/stride.h>

void SD_VARIANT(M7_Canvas_Present)(ECS_Handle *self) {
    M7_Canvas *canvas = ECS_Entity_GetComponent(self, M7_Components.Canvas);
    M7_Viewport *vp = canvas->vp;

    uint32_t *pixels;
    int pitch;

    SDL_LockTexture(vp->texture, nullptr, (void **)&pixels, &pitch);

    for (int i = 0; i < canvas->height; ++i) {
        sd_vec3 *base = canvas->color + i * sd_bounding_size(canvas->width);
        int sd_qot = canvas->width / SD_LENGTH;
        int sd_rem = canvas->width % SD_LENGTH;

        for (int j = 0; j < sd_qot; ++j) {
            sd_vec3 col = base[j];

            col = (sd_vec3) {
                .x = sd_float_clamp(col.x, sd_float_zero(), sd_float_one()),
                .z = sd_float_clamp(col.y, sd_float_zero(), sd_float_one()),
                .y = sd_float_clamp(col.z, sd_float_zero(), sd_float_one())
            };

            col = sd_vec3_mul(col, sd_float_set(255));
#if SD_VECTORIZE == SD_VECTORIZE_AVX2
            __m256i r = _mm256_cvtps_epi32(col.x.val);
                    r = _mm256_slli_epi32(r, 16);
            __m256i g = _mm256_cvtps_epi32(col.y.val);
                    g = _mm256_slli_epi32(g, 8);
            __m256i b = _mm256_cvtps_epi32(col.z.val);

            __m256i col_out = _mm256_or_si256(_mm256_or_si256(r, g), b);
            _mm256_storeu_si256((__m256i *)(pixels + i * vp->width) + j, col_out);
#elif SD_VECTORIZE == SD_VECTORIZE_SSE2
            __m128i r = _mm_cvtps_epi32(col.x.val);
                    r = _mm_slli_epi32(r, 16);
            __m128i g = _mm_cvtps_epi32(col.y.val);
                    g = _mm_slli_epi32(g, 8);
            __m128i b = _mm_cvtps_epi32(col.z.val);
                  
            __m128i col_out = _mm_or_si128(_mm_or_si128(r, g), b);
            _mm_storeu_si128((__m128i *)(pixels + i * vp->width) + j, col_out);
#else
            uint8_t r = col.x.val;
            uint8_t g = col.y.val;
            uint8_t b = col.z.val;
            pixels[i * vp->width + j] = (r << 16) | (g << 8) | b;
#endif
        }

        for (int j = 0; j < sd_rem; ++j) {
            sd_vec3_scalar col = sd_vec3_arr_get(base + sd_qot, j);
            uint8_t r = col.x.val * 255;
            uint8_t g = col.y.val * 255;
            uint8_t b = col.z.val * 255;
            pixels[i * vp->width + j] = (r << 16) | (g << 8) | b;
        }
    }

    SDL_UnlockTexture(vp->texture);

    SDL_RenderTexture(vp->renderer, vp->texture, nullptr, nullptr);
    SDL_RenderPresent(vp->renderer);
}

void SD_VARIANT(M7_Canvas_Attach)(ECS_Handle *self) {
    M7_Canvas *canvas = ECS_Entity_GetComponent(self, M7_Components.Canvas);
    ECS_Handle *e_vp = ECS_Entity_AncestorWithComponent(self, M7_Components.Viewport, true);
    
    canvas->vp = ECS_Entity_GetComponent(e_vp, M7_Components.Viewport);
}

void SD_VARIANT(M7_Canvas_Init)(void *component, void *args) {
    M7_Canvas *canvas = component, *cargs = args;

    canvas->width = cargs->width;
    canvas->height = cargs->height;

    size_t sd_count = sd_bounding_size(canvas->width) * canvas->height;
    canvas->color = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_count);
    canvas->depth = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_float) * sd_count);
}

#ifndef SD_VECTORIZE

void M7_Canvas_Free(void *component) {
    M7_Canvas *canvas = component;

    SDL_aligned_free(canvas->color);
    SDL_aligned_free(canvas->depth);
}

#endif /* UNVECTORIZED */
