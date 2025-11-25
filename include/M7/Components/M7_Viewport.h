#ifndef M7_VIEWPORT_H
#define M7_VIEWPORT_H

#include <SDL3/SDL.h>

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

#endif /* M7_VIEWPORT_H */
