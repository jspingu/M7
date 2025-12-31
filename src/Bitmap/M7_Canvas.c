#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/stride.h>
#include <M7/gamma.h>

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
            col = sd_vec3_muls(col, sd_float_set(0xFFFF));

            sd_int byte = sd_int_set(0xFF);

            sd_int r = sd_float_to_int(col.r);
                   r = sd_int_gather_i8((int8_t *)gamma_encode_lut, r);
                   r = sd_int_shl(r, 16);
            sd_int g = sd_float_to_int(col.g);
                   g = sd_int_gather_i8((int8_t *)gamma_encode_lut, g);
                   g = sd_int_and(g, byte);
                   g = sd_int_shl(g, 8);
            sd_int b = sd_float_to_int(col.b);
                   b = sd_int_gather_i8((int8_t *)gamma_encode_lut, b);
                   b = sd_int_and(b, byte);

            sd_int out = sd_int_or(r, sd_int_or(g, b));
            sd_int_store_unaligned((int32_t *)pd->pixels + i * canvas->width + j * SD_LENGTH, out);
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
    M7_Viewport *vp = ECS_Entity_GetComponent(self, M7_Components.Viewport);

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

void SD_VARIANT(M7_Canvas_Init)(void *component, void *args) {
    M7_Canvas *canvas = component, *cargs = args;

    canvas->width = cargs->width;
    canvas->height = cargs->height;
    canvas->parallelism = cargs->parallelism;

    size_t sd_count = sd_bounding_size(canvas->width) * canvas->height;
    canvas->color = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_count);
    canvas->depth = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_float) * sd_count);
}

#ifndef SD_SRC_VARIANT

void M7_Canvas_Free(void *component) {
    M7_Canvas *canvas = component;

    SDL_aligned_free(canvas->color);
    SDL_aligned_free(canvas->depth);
}

#endif /* SD_SRC_VARIANT */
