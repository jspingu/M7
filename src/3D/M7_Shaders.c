#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/stride.h>

sd_vec4 SD_VARIANT(M7_ShadeSolidColor)(ECS_Handle *self, sd_vec4 col, sd_vec3 vs, sd_vec3 nrml, sd_vec2 ts) {
    (void)col, (void)vs, (void)nrml, (void)ts;
    M7_SolidColor *solid_color = ECS_Entity_GetComponent(self, M7_Components.SolidColor);
    return (sd_vec4) { .rgb = sd_vec3_set(solid_color->r, solid_color->g, solid_color->b) };
}

sd_vec4 SD_VARIANT(M7_ShadeCheckerboard)(ECS_Handle *self, sd_vec4 col, sd_vec3 vs, sd_vec3 nrml, sd_vec2 ts) {
    (void)col, (void)vs, (void)nrml;
    M7_Checkerboard *checkerboard = ECS_Entity_GetComponent(self, M7_Components.Checkerboard);
    sd_vec2 tile_coord = sd_vec2_mul(ts, sd_float_set(checkerboard->tiles));
    sd_int tile_idx = sd_int_add(sd_int_mul(sd_float_to_int(tile_coord.y), sd_int_set(checkerboard->tiles)), sd_float_to_int(tile_coord.x));
    sd_mask tile_mask = sd_int_gt(sd_int_and(tile_idx, sd_int_set(1)), sd_int_set(0));

    sd_vec3 out = sd_vec3_mask_blend(
        sd_vec3_set(checkerboard->r1, checkerboard->g1, checkerboard->b1),
        sd_vec3_set(checkerboard->r2, checkerboard->g2, checkerboard->b2),
        tile_mask
    );

    return (sd_vec4) { .rgb = out };
}

sd_vec4 SD_VARIANT(M7_ShadeOriginLight)(ECS_Handle *self, sd_vec4 col, sd_vec3 vs, sd_vec3 nrml, sd_vec2 ts) {
    (void)self, (void)col, (void)ts;
    sd_float ambient = sd_float_set(0.1);
    sd_float energy = sd_float_set(20000);

    sd_float sqrlen = sd_vec3_dot(vs, vs);
    sd_float rcpsql = sd_float_rcp(sqrlen);
    sd_float rcplen = sd_float_rsqrt(sqrlen);

    sd_float power = sd_float_mul(
        sd_vec3_dot(sd_vec3_mul(vs, rcplen), sd_vec3_negate(nrml)),
        sd_float_mul(energy, rcpsql)
    );

    sd_vec3 out = sd_vec3_mul(col.rgb, sd_float_add(ambient, power));
    return (sd_vec4) { .rgb = out };
}
