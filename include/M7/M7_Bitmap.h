#ifndef M7_BITMAP_H
#define M7_BITMAP_H

#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/Math/stride.h>

typedef struct M7_Viewport {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int width, height;
} M7_Viewport;

typedef struct M7_ViewportArgs {
    char *title;
    int width, height;
} M7_ViewportArgs;

typedef struct M7_Canvas {
    ECS_Handle *vp;
    sd_vec3 *color;
    sd_float *depth;
    int width, height;
    int parallelism;
} M7_Canvas;

typedef struct M7_Texture {
    float *color;
    int width, height;
    int unit;
} M7_Texture;

static inline sd_vec4 M7_SampleNearest(M7_Texture *texture, sd_vec2 ts) {
    sd_float unit = sd_float_set(texture->unit);
    sd_vec2 ss = sd_vec2_mul(ts, unit);

    sd_int pixel_index = sd_int_add(sd_int_mul(
        sd_float_to_int(ss.y),
        sd_int_set(texture->width)
    ), sd_float_to_int(ss.x));

    return sd_vec4_gather(texture->color, pixel_index);
}

#endif /* M7_BITMAP_H */
