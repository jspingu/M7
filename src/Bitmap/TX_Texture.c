#include <SDL3/SDL.h>
#include <TX/TX_ECS.h>
#include <TX/TX_Resource.h>
#include <TX/gamma.h>

void *TX_TextureBank_LoadTexture(ECS_Handle *self, char *path) {
    (void)self;
    SDL_Surface *img = SDL_LoadPNG(path);
    SDL_Surface *img_abgr = SDL_ConvertSurface(img, SDL_PIXELFORMAT_ABGR32);

    TX_Texture *texture = SDL_malloc(sizeof(TX_Texture));
    texture->color = SDL_malloc(sizeof(float [4]) * img_abgr->w * img_abgr->h);
    texture->width = img_abgr->w;
    texture->height = img_abgr->h;
    texture->unit = SDL_max(img_abgr->w, img_abgr->h);

    for (int i = 0; i < img_abgr->w * img_abgr->h; ++i) {
        uint32_t *px = img_abgr->pixels;
        texture->color[i * 4 + 0] = (float)gamma_decode_lut[(px[i] >> 24) & 0xFF] / 0xFFFF;
        texture->color[i * 4 + 1] = (float)gamma_decode_lut[(px[i] >> 16) & 0xFF] / 0xFFFF;
        texture->color[i * 4 + 2] = (float)gamma_decode_lut[(px[i] >>  8) & 0xFF] / 0xFFFF;
        texture->color[i * 4 + 3] = (float)(px[i] & 0xFF) / 0xFF;
    }

    SDL_DestroySurface(img_abgr);
    SDL_DestroySurface(img);
    return texture;
}

void TX_TextureBank_FreeTexture(ECS_Handle *self, void *data) {
    (void)self;
    TX_Texture *texture = data;
    SDL_free(texture->color);
    SDL_free(texture);
}

void TX_TextureBank_Attach(ECS_Handle *self, ECS_Component(void) *component) {
    TX_ResourceBank_Attach(self, component, TX_TextureBank_LoadTexture, TX_TextureBank_FreeTexture);
}
