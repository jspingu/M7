#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/linalg.h>
#include <M7/Math/stride.h>

sd_vec2 SD_VARIANT(M7_ProjectPerspective)(ECS_Handle *self, sd_vec3 pos, sd_vec2 midpoint) {
    (void)self;

    sd_vec2 normalized = sd_vec2_div((sd_vec2) {
        .x = pos.x,
        .y = sd_float_negate(pos.y)
    }, pos.z);

    return sd_vec2_add(midpoint, sd_vec2_mul(normalized, midpoint.x));
}

#ifdef SD_BASE

xform3 M7_Entity_GetXform(ECS_Handle *self) {
    mat3x3 *basis = ECS_Entity_GetComponent(self, M7_Components.Basis);
    vec3 *position = ECS_Entity_GetComponent(self, M7_Components.Position);

    return (xform3) {
        basis ? *basis : mat3x3_identity,
        position ? *position : vec3_zero
    };
}

void M7_Entity_Xform(ECS_Handle *self, xform3 lhs) {
    M7_XformComposer *compose = ECS_Entity_GetComponent(self, M7_Components.XformComposer);
    if (!compose) return;

    xform3 composed = (*compose)(self, lhs);
    ECS_Entity_ProcessSystemGroup(self, M7_SystemGroups.OnXform, composed);
    ECS_Entity_ForEachChild(self, c, M7_Entity_Xform(c, composed); );
}

xform3 M7_XformComposeDefault(ECS_Handle *self, xform3 lhs) {
    return xform3_apply(lhs, M7_Entity_GetXform(self));
}

xform3 M7_XformComposeBillboard(ECS_Handle *self, xform3 lhs) {
    xform3 local = M7_Entity_GetXform(self);

    return (xform3) {
        local.basis,
        vec3_add(lhs.translation, mat3x3_mul(lhs.basis, local.translation))
    };
}

xform3 M7_XformComposeCubemap(ECS_Handle *self, xform3 lhs) {
    xform3 local = M7_Entity_GetXform(self);

    return (xform3) {
        mat3x3_mul(lhs.basis, local.basis),
        local.translation
    };
}

xform3 M7_XformComposeAbsolute(ECS_Handle *self, xform3 lhs) {
    (void)lhs;
    return M7_Entity_GetXform(self);
}

#endif /* SD_BASE */
