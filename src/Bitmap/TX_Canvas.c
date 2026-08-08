#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/stride.h>
#include <TX/gamma.h>

typedef struct PresentData {
    ECS_Handle *canvas;
    uint32_t *pixels;
    int start, end;
} PresentData;

static int PresentThread(void *data) {
    PresentData *pd = data;
    TX_Canvas *canvas = ECS_Entity_GetComponent(pd->canvas, TX_Components.Canvas);
    int qot = sd_qot(canvas->width);
    int rem = sd_rem(canvas->width);

    for (int i = pd->start; i < pd->end; ++i) {
        int base = i * sd_bounding_length(canvas->width);

        for (int j = 0; j < qot; ++j) {
            sd_vec3 col = sd_vec3_load(canvas->color, base + j);
            col = sd_vec3_clamp(col, sd_float_zero(), sd_float_one());
            col = sd_vec3_muls(col, sd_float_set(0xFFFF));

            sd_int byte = sd_int_set(0xFF);

            sd_int r = sd_float_to_int(sd_vx(col));
                   r = sd_int_gather_u8(gamma_encode_lut, r);
                   r = sd_int_shl(r, 16);
            sd_int g = sd_float_to_int(sd_vy(col));
                   g = sd_int_gather_u8(gamma_encode_lut, g);
                   g = sd_int_and(g, byte);
                   g = sd_int_shl(g, 8);
            sd_int b = sd_float_to_int(sd_vz(col));
                   b = sd_int_gather_u8(gamma_encode_lut, b);
                   b = sd_int_and(b, byte);

            sd_int out = sd_int_or(r, sd_int_or(g, b));
            sd_int_storeu((int32_t *)pd->pixels + i * canvas->width + j * sd_length(), out);
        }

        for (int j = 0; j < rem; ++j) {
            sd_vec3_scalar col = sd_vec3_loads(canvas->color, qot * sd_length() + j);

            uint16_t r = col.x * 0xFFFF;
                     r = gamma_encode_lut[r];
            uint16_t g = col.y * 0xFFFF;
                     g = gamma_encode_lut[g];
            uint16_t b = col.z * 0xFFFF;
                     b = gamma_encode_lut[b];

            pd->pixels[i * canvas->width + qot * sd_length() + j] = (r << 16) | (g << 8) | b;
        }
    }

    return 0;
}

void SD_VARIANT(TX_Canvas_Present)(ECS_Handle *self) {
    TX_Canvas *canvas = ECS_Entity_GetComponent(self, TX_Components.Canvas);
    TX_Viewport *vp = ECS_Entity_GetComponent(self, TX_Components.Viewport);

    uint32_t *pixels;
    int pitch;

    SDL_LockTexture(vp->texture, nullptr, (void **)&pixels, &pitch);

    int parallelism = SDL_GetNumLogicalCPUCores();
    SDL_Thread **threads = SDL_malloc(sizeof(SDL_Thread *) * parallelism);
    PresentData *present_data = SDL_malloc(sizeof(PresentData) * parallelism);

    int qot = canvas->height / parallelism;
    int rem = canvas->height % parallelism;

    for (int i = 0; i < parallelism; ++i) {
        present_data[i] = (PresentData) {
            .canvas = self,
            .pixels = pixels,
            .start = i * qot + SDL_min(i, rem),
            .end = (i + 1) * qot + SDL_min(i + 1, rem)
        };

        threads[i] = SDL_CreateThread(PresentThread, "present", present_data + i);
    }

    for (int i = 0; i < parallelism; ++i)
        SDL_WaitThread(threads[i], nullptr);

    SDL_free(present_data);
    SDL_free(threads);

    SDL_UnlockTexture(vp->texture);
    SDL_RenderTexture(vp->renderer, vp->texture, nullptr, nullptr);
    SDL_RenderPresent(vp->renderer);
}

void SD_VARIANT(TX_Canvas_Init)(void *component, void *args) {
    TX_Canvas *canvas = component, *cargs = args;
    canvas->width = cargs->width;
    canvas->height = cargs->height;

    size_t sd_size = sd_bounding_size(canvas->width) * canvas->height;
    canvas->color = SDL_aligned_alloc(SD_ALIGN, sd_size * 3);
    canvas->depth = SDL_aligned_alloc(SD_ALIGN, sd_size);
    
    int parallelism = SDL_GetNumLogicalCPUCores();
    int qot = canvas->height / parallelism;
    int rem = canvas->height % parallelism;
    size_t sd_scanlines_size = sd_bounding_size(qot + 1) * rem + sd_bounding_size(qot) * (parallelism - rem);
    canvas->scanlines[0] = SDL_aligned_alloc(SD_ALIGN, sd_scanlines_size);
    canvas->scanlines[1] = SDL_aligned_alloc(SD_ALIGN, sd_scanlines_size);
}

#ifndef SD_SRC_VARIANT

void TX_Canvas_Free(void *component) {
    TX_Canvas *canvas = component;

    SDL_aligned_free(canvas->color);
    SDL_aligned_free(canvas->depth);
    SDL_aligned_free(canvas->scanlines[0]);
    SDL_aligned_free(canvas->scanlines[1]);
}

#endif /* SD_SRC_VARIANT */
