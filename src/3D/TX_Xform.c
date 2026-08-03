#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

sd_vec2 SD_VARIANT(TX_ProjectParallel)(ECS_Handle *self, sd_vec3 pos, sd_vec2 midpoint) {
    TX_ParallelProjector *parallel_projector = ECS_Entity_GetComponent(self, TX_Components.ParallelProjector);
    sd_vec2 projected = sd_vec2_fmadd(sd_vec2_set(parallel_projector->slope.x, parallel_projector->slope.y), pos.z, pos.xy);
    return sd_vec2_add(midpoint, sd_vec2_mul(projected, sd_vec2_set(parallel_projector->scale.x, -parallel_projector->scale.y)));
}

sd_vec2 SD_VARIANT(TX_ProjectPerspective)(ECS_Handle *self, sd_vec3 pos, sd_vec2 midpoint) {
    TX_PerspectiveFOV *perspective_fov = ECS_Entity_GetComponent(self, TX_Components.PerspectiveFOV);

    sd_vec2 normalized = { .x = pos.x, .y = sd_float_negate(pos.y) };
            normalized = sd_vec2_muls(normalized, sd_float_rcp(sd_float_mul(pos.z, sd_float_set(perspective_fov->tan_half_fov))));

    return sd_vec2_fmadd(normalized, midpoint.x, midpoint);
}

#ifndef SD_SRC_VARIANT

xform3 TX_Entity_GetXform(ECS_Handle *self) {
    mat3x3 *basis = ECS_Entity_GetComponent(self, TX_Components.Basis);
    vec3 *position = ECS_Entity_GetComponent(self, TX_Components.Position);

    return (xform3) {
        basis ? *basis : mat3x3_identity,
        position ? *position : vec3_zero
    };
}

void TX_Entity_Xform(ECS_Handle *self, xform3 lhs) {
    TX_XformComposer *compose = ECS_Entity_GetComponent(self, TX_Components.XformComposer);
    if (!compose) return;

    xform3 composed = (*compose)(self, lhs);
    ECS_Entity_ProcessSystemGroup(self, TX_SystemGroups.OnXform, composed);
    ECS_Entity_ForEachChild(self, c, TX_Entity_Xform(c, composed); );
}

xform3 TX_XformComposeDefault(ECS_Handle *self, xform3 lhs) {
    return xform3_apply(lhs, TX_Entity_GetXform(self));
}

xform3 TX_XformComposeBillboard(ECS_Handle *self, xform3 lhs) {
    xform3 local = TX_Entity_GetXform(self);

    return (xform3) {
        local.basis,
        vec3_add(lhs.translation, mat3x3_mul(lhs.basis, local.translation))
    };
}

xform3 TX_XformComposeCubemap(ECS_Handle *self, xform3 lhs) {
    xform3 local = TX_Entity_GetXform(self);

    return (xform3) {
        mat3x3_mul(lhs.basis, local.basis),
        local.translation
    };
}

xform3 TX_XformComposeAbsolute(ECS_Handle *self, xform3 lhs) {
    (void)lhs;
    return TX_Entity_GetXform(self);
}

#endif /* SD_SRC_VARIANT */
