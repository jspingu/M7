#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_ECS.h>

void TX_Viewport_Init(void *component, void *args) {
    TX_Viewport *vp = component;
    TX_ViewportArgs *vp_args = args;

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

    SDL_SetRenderVSync(vp->renderer, 0);

    vp->texture = SDL_CreateTexture(
        vp->renderer,
        SDL_PIXELFORMAT_BGRX32,
        SDL_TEXTUREACCESS_STREAMING,
        vp->width,
        vp->height
    );
}

void TX_Viewport_Free(void *component) {
    TX_Viewport *viewport = component;
    SDL_DestroyTexture(viewport->texture);
    SDL_DestroyRenderer(viewport->renderer);
    SDL_DestroyWindow(viewport->window);
}
