#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/stride.h>

sd_vec4 SD_VARIANT(M7_ShadeSolidColor)(void *state, sd_vec4 col, sd_vec3 vs, sd_vec3 nrml, sd_vec2 ts) {
    (void)col, (void)vs, (void)nrml, (void)ts;
    M7_SolidColor *solid_color = state;
    return sd_vec4_set(solid_color->r, solid_color->g, solid_color->b, 1);
}

sd_vec4 SD_VARIANT(M7_ShadeCheckerboard)(void *state, sd_vec4 col, sd_vec3 vs, sd_vec3 nrml, sd_vec2 ts) {
    (void)col, (void)vs, (void)nrml;
    M7_Checkerboard *checkerboard = state;
    sd_vec2 tile_coord = sd_vec2_muls(ts, sd_float_set(checkerboard->tiles));
    sd_int tile_idx = sd_int_add(sd_int_mul(sd_float_to_int(tile_coord.y), sd_int_set(checkerboard->tiles)), sd_float_to_int(tile_coord.x));
    sd_mask tile_mask = sd_int_gt(sd_int_and(tile_idx, sd_int_set(1)), sd_int_set(0));

    return sd_vec4_mask_blend(
        sd_vec4_set(checkerboard->r1, checkerboard->g1, checkerboard->b1, 1),
        sd_vec4_set(checkerboard->r2, checkerboard->g2, checkerboard->b2, 1),
        tile_mask
    );
}

sd_vec4 SD_VARIANT(M7_ShadeOriginLight)(void *state, sd_vec4 col, sd_vec3 vs, sd_vec3 nrml, sd_vec2 ts) {
    (void)state, (void)col, (void)ts;
    sd_float ambient = sd_float_set(0.08);
    sd_float energy = sd_float_set(10000);
    sd_float reflectivity = sd_float_set(0.6);
    sd_float specularity = sd_float_set(1);
    int exp = 4;

    sd_float sqrlen = sd_vec3_dot(vs, vs);
    sd_float rcpsql = sd_float_rcp(sqrlen);
    sd_float rcplen = sd_float_rsqrt(sqrlen);

    sd_vec3 ray = sd_vec3_muls(vs, rcplen);
    sd_float dp = sd_float_negate(sd_vec3_dot(ray, nrml));

    sd_float rf_falloff = sd_float_sub(sd_float_one(), dp);
             rf_falloff = sd_float_mul(rf_falloff, rf_falloff);
             rf_falloff = sd_float_mul(rf_falloff, rf_falloff);

    sd_float rf_coeff = sd_float_add(reflectivity, sd_float_mul(sd_float_sub(sd_float_one(), reflectivity), rf_falloff));
    sd_float sp_coeff = sd_float_max(sd_float_mul(specularity, sd_float_negate(sd_vec3_dot(ray, sd_vec3_reflect(ray, nrml)))), sd_float_zero());

    for (int i = 0; i < exp; ++i)
        sp_coeff = sd_float_mul(sp_coeff, sp_coeff);

    sd_vec3 lcol = sd_vec3_set(1.0, 1.0, 0.5);
    sd_vec3 power_in = sd_vec3_muls(lcol, sd_float_mul(sd_float_mul(energy, rcpsql), dp));
    sd_vec3 power_out = sd_vec3_muls(power_in, sd_float_add(rf_coeff, sp_coeff));
    col.rgb = sd_vec3_mul(col.rgb, sd_vec3_adds(power_out, ambient));
    return col;
}

sd_vec4 SD_VARIANT(M7_ShadeTextureMap)(void *state, sd_vec4 col, sd_vec3 vs, sd_vec3 nrml, sd_vec2 ts) {
    (void)col, (void)vs, (void)nrml;
    M7_TextureMap *texture_map = state;
    return M7_SampleNearest(texture_map->texture, ts) ;
}

#ifndef SD_SRC_VARIANT

void M7_TextureMap_Attach(ECS_Handle *self, ECS_Component(void) *component) {
    M7_ShaderComponent *shader_component = ECS_Entity_GetComponent(self, component);
    M7_TextureMap *texture_map = shader_component->state;
    ECS_Handle *tb = ECS_Entity_AncestorWithComponent(self, M7_Components.TextureBank, false);
    texture_map->texture = M7_ResourceBank_Get(tb, M7_Components.TextureBank, texture_map->texture_path);
}

void M7_TextureMap_Detach(ECS_Handle *self, ECS_Component(void) *component) {
    M7_ShaderComponent *shader_component = ECS_Entity_GetComponent(self, component);
    M7_TextureMap *texture_map = shader_component->state;
    ECS_Handle *tb = ECS_Entity_AncestorWithComponent(self, M7_Components.TextureBank, false);
    M7_ResourceBank_Release(tb, M7_Components.TextureBank, texture_map->texture_path);
}

void M7_SolidColor_Init(void *component, void *args) {
    M7_ShaderComponent *shader_component = component;
    shader_component->callback = SD_SELECT(M7_ShadeSolidColor);
    shader_component->state = SDL_malloc(sizeof(M7_SolidColor));
    SDL_memcpy(shader_component->state, args, sizeof(M7_SolidColor));
}

void M7_Checkerboard_Init(void *component, void *args) {
    M7_ShaderComponent *shader_component = component;
    shader_component->callback = SD_SELECT(M7_ShadeCheckerboard);
    shader_component->state = SDL_malloc(sizeof(M7_Checkerboard));
    SDL_memcpy(shader_component->state, args, sizeof(M7_Checkerboard));
}

void M7_TextureMap_Init(void *component, void *args) {
    M7_ShaderComponent *shader_component = component;
    shader_component->callback = SD_SELECT(M7_ShadeTextureMap);
    shader_component->state = SDL_malloc(sizeof(M7_TextureMap));

    M7_TextureMap *texture_map = shader_component->state;
    char *path = args;
    texture_map->texture_path = SDL_malloc(SDL_strlen(path) + 1);
    SDL_strlcpy(texture_map->texture_path, path, 64);
}

void M7_Lighting_Init(void *component, void *args) {
    (void)args;
    M7_ShaderComponent *shader_component = component;
    shader_component->callback = SD_SELECT(M7_ShadeOriginLight);
}

void M7_TextureMap_Free(void *component) {
    M7_ShaderComponent *shader_component = component;
    M7_TextureMap *texture_map = shader_component->state;
    SDL_free(texture_map->texture_path);
    SDL_free(texture_map);
}

void M7_ShaderComponent_Free(void *component) {
    M7_ShaderComponent *shader_component = component;
    SDL_free(shader_component->state);
}

#endif /* SD_SRC_VARIANT */
