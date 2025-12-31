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
    sd_vec2 pixel_coord = sd_vec2_muls(ts, unit);

    pixel_coord.x = sd_float_clamp(pixel_coord.x, sd_float_zero(), sd_float_set(texture->width - 1));
    pixel_coord.y = sd_float_clamp(pixel_coord.y, sd_float_zero(), sd_float_set(texture->height - 1));

    sd_int pixel_index = sd_int_add(sd_int_mul(
        sd_float_to_int(pixel_coord.y),
        sd_int_set(texture->width)
    ), sd_float_to_int(pixel_coord.x));

    return sd_vec4_gather(texture->color, pixel_index);
}

static inline sd_vec4 M7_SampleCubemap(M7_Texture *texture, sd_vec3 dir) {
    sd_float unit = sd_float_set(texture->width * 0.5f);
    sd_vec3 rcp = sd_vec3_rcp(dir);

    sd_vec2 zy = sd_vec2_muls((sd_vec2) { .x=dir.z, .y=dir.y }, sd_float_negate(rcp.x));
    sd_vec2 xz = sd_vec2_muls((sd_vec2) { .x=dir.x, .y=dir.z }, sd_float_negate(rcp.y));
    sd_vec2 xy = sd_vec2_muls((sd_vec2) { .x=dir.x, .y=dir.y }, rcp.z);

    sd_mask mask_zy = sd_mask_and(sd_float_clamp_mask(zy.x, -1, 1), sd_float_clamp_mask(zy.y, -1, 1));
    sd_mask mask_xz = sd_mask_andn(sd_mask_and(sd_float_clamp_mask(xz.x, -1, 1), sd_float_clamp_mask(xz.y, -1, 1)), mask_zy);

    sd_mask across_zy = sd_float_lt(dir.x, sd_float_zero());
    sd_mask across_xz = sd_float_lt(dir.y, sd_float_zero());
    sd_mask across_xy = sd_float_lt(dir.z, sd_float_zero());

    sd_int idx_zy = sd_int_mask_blend(sd_int_set(0), sd_int_set(3), across_zy);
    sd_int idx_xz = sd_int_mask_blend(sd_int_set(1), sd_int_set(4), across_xz);
    sd_int idx_xy = sd_int_mask_blend(sd_int_set(2), sd_int_set(5), across_xy);

    sd_vec2 flip = {
        .x = sd_float_mask_blend(sd_float_one(), sd_float_set(-1), sd_mask_andn(mask_xz, across_xz)),
        .y = sd_float_mask_blend(sd_float_set(-1), sd_float_one(), sd_mask_or(
            sd_mask_andn(mask_zy, across_zy),
            sd_mask_andn(across_xy, sd_mask_or(mask_zy, mask_xz))
        ))
    };

    sd_vec2 pixel_coord = sd_vec2_mask_blend(sd_vec2_mask_blend(xy, xz, mask_xz), zy, mask_zy);
            pixel_coord = sd_vec2_mul(pixel_coord, flip);
            pixel_coord = sd_vec2_adds(sd_vec2_muls(pixel_coord, unit), unit);
            pixel_coord = sd_vec2_clamp(pixel_coord, sd_float_zero(), sd_float_set(texture->width - 1));

    sd_int pixel_offset = sd_int_mask_blend(sd_int_mask_blend(idx_xy, idx_xz, mask_xz), idx_zy, mask_zy);
           pixel_offset = sd_int_mul(pixel_offset, sd_int_set(texture->width * texture->width));

    sd_int pixel_index = sd_int_add(pixel_offset, sd_int_add(sd_int_mul(
        sd_float_to_int(pixel_coord.y),
        sd_int_set(texture->width)
    ), sd_float_to_int(pixel_coord.x)));

    return sd_vec4_gather(texture->color, pixel_index);
}

#endif /* M7_BITMAP_H */
