#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <SDL3/SDL_pixels.h>

#include "M7_Viewport_c.h"

void M7_Viewport_Init(void *component, void *args) {
    M7_Viewport *vp = component;
    M7_ViewportArgs *vp_args = args;

    vp->width = vp_args->width;
    vp->height = vp_args->height;

    SDL_CreateWindowAndRenderer(
        vp_args->title,
        vp_args->width,
        vp_args->height,
        SDL_WINDOW_RESIZABLE,
        &vp->window,
        &vp->renderer
    );

    SDL_SetRenderVSync(vp->renderer, 1);
    SDL_SetWindowRelativeMouseMode(vp->window, true);

    vp->texture = SDL_CreateTexture(
        vp->renderer,
        SDL_PIXELFORMAT_BGRX32,
        SDL_TEXTUREACCESS_STREAMING,
        vp->width,
        vp->height
    );
}

void M7_Viewport_Free(void *component) {
    M7_Viewport *viewport = component;
    SDL_DestroyTexture(viewport->texture);
    SDL_DestroyRenderer(viewport->renderer);
    SDL_DestroyWindow(viewport->window);
}
