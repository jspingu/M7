#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/stride.h>

void SD_VARIANT(M7_Canvas_Present)(ECS_Handle *self) {
    M7_Canvas *canvas = ECS_Entity_GetComponent(self, M7_Components.Canvas);
    M7_Viewport *vp = canvas->target;

    uint32_t *pixels;
    int pitch;

    SDL_LockTexture(vp->texture, nullptr, (void **)&pixels, &pitch);

    for (int i = 0; i < canvas->target->height; ++i) {
        sd_vec3 *base = canvas->color + i * sd_bounding_size(canvas->target->width);

        for (int j = 0; j < canvas->target->width; ++j) {
            sd_vec3_scalar col = sd_vec3_arr_get(base, j);
            uint8_t r = col.x.val * 255;
            uint8_t g = col.y.val * 255;
            uint8_t b = col.z.val * 255;
            pixels[i * canvas->target->width + j] = (r << 16) | (g << 8) | b;
        }
    }

    SDL_UnlockTexture(vp->texture);

    SDL_RenderTexture(vp->renderer, vp->texture, nullptr, nullptr);
    SDL_RenderPresent(vp->renderer);
}

void SD_VARIANT(M7_Canvas_Attach)(ECS_Handle *self) {
    M7_Canvas *canvas = ECS_Entity_GetComponent(self, M7_Components.Canvas);
    ECS_Handle *e_vp = ECS_Entity_AncestorWithComponent(self, M7_Components.Viewport, true);
    
    canvas->target = ECS_Entity_GetComponent(e_vp, M7_Components.Viewport);
}

void SD_VARIANT(M7_Canvas_Init)(void *component, void *args) {
    M7_Canvas *canvas = component, *cargs = args;

    canvas->width = cargs->width;
    canvas->height = cargs->height;

    size_t sd_count = sd_bounding_size(canvas->width) * canvas->height;
    canvas->color = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_count);
    canvas->depth = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_float) * sd_count);

    for (size_t i = 0; i < sd_count; ++i)
        canvas->color[i] = sd_vec3_set(0.5, 0.1, 0.1);
}

#ifndef SD_VECTORIZE

void M7_Canvas_Free(void *component) {
    M7_Canvas *canvas = component;

    SDL_aligned_free(canvas->color);
    SDL_aligned_free(canvas->depth);
}

#endif /* UNVECTORIZED */
