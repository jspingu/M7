#ifndef M7_BITMAP_H
#define M7_BITMAP_H

#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/Math/stride.h>

typedef struct M7_Viewport {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_FRect rect;
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

#endif /* M7_BITMAP_H */
