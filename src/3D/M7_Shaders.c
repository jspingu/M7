#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/stride.h>
#include <M7/Math/linalg.h>

sd_vec4 SD_VARIANT(M7_ShadeSolidColor)(void *state, M7_ShaderParams fragment) {
    (void)fragment;
    M7_SolidColor *solid_color = state;
    return sd_vec4_set(solid_color->r, solid_color->g, solid_color->b, 1);
}

sd_vec4 SD_VARIANT(M7_ShadeCheckerboard)(void *state, M7_ShaderParams fragment) {
    M7_Checkerboard *checkerboard = state;
    sd_vec2 tile_coord = sd_vec2_muls(fragment.ts, sd_float_set(checkerboard->tiles));
    sd_int tile_idx = sd_int_add(sd_int_mul(sd_float_to_int(tile_coord.y), sd_int_set(checkerboard->tiles)), sd_float_to_int(tile_coord.x));
    sd_mask tile_mask = sd_int_gt(sd_int_and(tile_idx, sd_int_set(1)), sd_int_set(0));

    return sd_vec4_mask_blend(
        sd_vec4_set(checkerboard->r1, checkerboard->g1, checkerboard->b1, 1),
        sd_vec4_set(checkerboard->r2, checkerboard->g2, checkerboard->b2, 1),
        tile_mask
    );
}

sd_vec4 SD_VARIANT(M7_ShadeTextureMap)(void *state, M7_ShaderParams fragment) {
    M7_TextureMap *texture_map = state;
    return M7_SampleNearest(texture_map->texture, fragment.ts) ;
}

sd_vec4 SD_VARIANT(M7_ShadeLighting)(void *state, M7_ShaderParams fragment) {
    M7_OpticalMedium *medium = state;
    sd_float specularity = sd_float_set(medium->specularity);
    sd_float reflectivity = sd_float_set(medium->reflectivity);
    sd_float ambient = sd_float_set(medium->environment->ambient);
    sd_vec3 eye = sd_vec3_negate(sd_vec3_normalize(fragment.vs));

    sd_vec4 out;
    out.rgb = sd_vec3_muls(fragment.col.rgb, ambient);
    out.a = fragment.col.a;

    List_ForEach(medium->environment->lights, light, {
        sd_vec3 light_vs = sd_vec3_set(light->pos.x, light->pos.y, light->pos.z);
        sd_vec3 light_col = sd_vec3_set(light->col.x, light->col.y, light->col.z);
        sd_float light_energy = sd_float_set(light->energy);

        sd_vec3 incident = sd_vec3_sub(fragment.vs, light_vs);
        sd_float sqrlen = sd_vec3_dot(incident, incident);
        sd_float rcpsql = sd_float_rcp(sqrlen);
        sd_float rcplen = sd_float_rsqrt(sqrlen);

        sd_vec3 unit_incident = sd_vec3_muls(incident, rcplen);
        sd_float dp = sd_float_max(sd_float_negate(sd_vec3_dot(unit_incident, fragment.nrml)), sd_float_zero());

        sd_float rf_falloff = sd_float_sub(sd_float_one(), dp);
                 rf_falloff = sd_float_mul(rf_falloff, rf_falloff);
                 rf_falloff = sd_float_mul(rf_falloff, rf_falloff);

        sd_float rf_coeff = sd_float_fmadd(sd_float_sub(sd_float_one(), reflectivity), rf_falloff, reflectivity);
        sd_float sp_coeff = sd_vec3_dot(eye, sd_vec3_reflect(unit_incident, fragment.nrml));

        for (int i = 0; i < medium->exp; ++i)
            sp_coeff = sd_float_mul(sp_coeff, sp_coeff);

        sp_coeff = sd_float_mul(sp_coeff, specularity);

        sd_vec3 power_in = sd_vec3_muls(light_col, sd_float_mul(sd_float_mul(light_energy, rcpsql), dp));
        sd_vec3 power_out = sd_vec3_muls(power_in, rf_coeff);
        out.rgb = sd_vec3_add(out.rgb, sd_vec3_mul(fragment.col.rgb, sd_vec3_fmadd(power_out, sp_coeff, power_out)));
    });

    return out;
}

#ifndef SD_SRC_VARIANT

void M7_PointLight_OnXform(ECS_Handle *self, xform3 composed) {
    M7_PointLight *light = ECS_Entity_GetComponent(self, M7_Components.PointLight);
    light->active->pos = composed.translation;
}

void M7_TextureMap_Attach(ECS_Handle *self, ECS_Component(void) *component) {
    M7_ShaderComponent *shader_component = ECS_Entity_GetComponent(self, component);
    M7_TextureMap *texture_map = shader_component->state;
    ECS_Handle *tb = ECS_Entity_AncestorWithComponent(self, M7_Components.TextureBank, false);
    texture_map->texture = M7_ResourceBank_Get(tb, M7_Components.TextureBank, texture_map->texture_path);
}

void M7_Lighting_Attach(ECS_Handle *self, ECS_Component(void) *component) {
    M7_ShaderComponent *shader_component = ECS_Entity_GetComponent(self, component);
    M7_OpticalMedium *medium = shader_component->state;
    ECS_Handle *env = ECS_Entity_AncestorWithComponent(self, M7_Components.LightEnvironment, false);
    medium->environment = *ECS_Entity_GetComponent(env, M7_Components.LightEnvironment);
}

void M7_PointLight_Attach(ECS_Handle *self, ECS_Component(void) *component) {
    M7_PointLight *light = ECS_Entity_GetComponent(self, component);
    ECS_Handle *env = ECS_Entity_AncestorWithComponent(self, M7_Components.LightEnvironment, false);
    light->environment = *ECS_Entity_GetComponent(env, M7_Components.LightEnvironment);

    M7_ActiveLight *active = SDL_malloc(sizeof(M7_ActiveLight));
    active->col = light->col;
    active->energy = light->energy;
    active->pos = vec3_zero;

    light->active = active;
    List_Push(light->environment->lights, active);
}

void M7_TextureMap_Detach(ECS_Handle *self, ECS_Component(void) *component) {
    M7_ShaderComponent *shader_component = ECS_Entity_GetComponent(self, component);
    M7_TextureMap *texture_map = shader_component->state;
    ECS_Handle *tb = ECS_Entity_AncestorWithComponent(self, M7_Components.TextureBank, false);
    M7_ResourceBank_Release(tb, M7_Components.TextureBank, texture_map->texture_path);
}

void M7_PointLight_Detach(ECS_Handle *self, ECS_Component(void) *component) {
    M7_PointLight *light = ECS_Entity_GetComponent(self, component);
    List_RemoveWhere(light->environment->lights, active, active == light->active);
    SDL_free(light->active);
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
    M7_ShaderComponent *shader_component = component;
    shader_component->callback = SD_SELECT(M7_ShadeLighting);
    shader_component->state = SDL_malloc(sizeof(M7_OpticalMedium));
    SDL_memcpy(shader_component->state, args, sizeof(M7_OpticalMedium));
}

void M7_LightEnvironment_Init(void *component, void *args) {
    M7_LightEnvironment **env = component;
    M7_LightEnvironment *env_args = args;
    *env = SDL_malloc(sizeof(M7_LightEnvironment));
    (*env)->lights = List_Create(M7_ActiveLight *);
    (*env)->ambient = env_args->ambient;
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

void M7_LightEnvironment_Free(void *component) {
    M7_LightEnvironment **env = component;
    List_Free((*env)->lights);
    SDL_free(*env);
}

#endif /* SD_SRC_VARIANT */
