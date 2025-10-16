#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/linalg.h>

void M7_Entity_Xform(ECS_Handle *self, xform3 lhs) {
    M7_XformComposer *compose = ECS_Entity_GetComponent(self, M7_Components.XformComposer);
    if (!compose) return;

    xform3 composed = (*compose)(self, lhs);
    ECS_Entity_ProcessSystemGroup(self, M7_SystemGroups.OnXform, composed);
    ECS_Entity_ForEachChild(self, c, M7_Entity_Xform(c, composed); );
}

xform3 M7_XformComposeDefault(ECS_Handle *self, xform3 lhs) {
    mat3x3 *basis = ECS_Entity_GetComponent(self, M7_Components.Basis);
    vec3 *position = ECS_Entity_GetComponent(self, M7_Components.Position);

    xform3 local = {
        basis ? *basis : mat3x3_identity,
        position ? *position : vec3_zero
    };

    return xform3_apply(lhs, local);
}

xform3 M7_XformComposeBillboard(ECS_Handle *self, xform3 lhs) {
    mat3x3 *basis = ECS_Entity_GetComponent(self, M7_Components.Basis);
    vec3 *position = ECS_Entity_GetComponent(self, M7_Components.Position);

    xform3 local = {
        basis ? *basis : mat3x3_identity,
        position ? *position : vec3_zero
    };

    return (xform3) {
        local.basis,
        vec3_add(lhs.translation, mat3x3_mul(lhs.basis, local.translation))
    };
}

xform3 M7_XformComposeAbsolute(ECS_Handle *self, xform3 lhs) {
    mat3x3 *basis = ECS_Entity_GetComponent(self, M7_Components.Basis);
    vec3 *position = ECS_Entity_GetComponent(self, M7_Components.Position);

    xform3 local = {
        basis ? *basis : mat3x3_identity,
        position ? *position : vec3_zero
    };

    return (xform3) {
        mat3x3_mul(lhs.basis, local.basis),
        local.translation
    };
}
