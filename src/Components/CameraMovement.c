#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/linalg.h>

#include <M7/Components/CameraMovement.h>

bool vec3_eq(vec3 lhs, vec3 rhs) {
    return lhs.x == rhs.x &&
           lhs.y == rhs.y &&
           lhs.z == rhs.y;
}

void CameraMovement_Update(ECS_Handle *self, double delta) {
    ECS_Handle *is = ECS_Entity_AncestorWithComponent(self, M7_Components.InputState, true);

    CameraMovement *cam = ECS_Entity_GetComponent(self, M7_Components.CameraMovement);
    vec3 *pos = ECS_Entity_GetComponent(self, M7_Components.Position);
    mat3x3 *basis = ECS_Entity_GetComponent(self, M7_Components.Basis);

    vec3 input_axis = vec3_zero;

    if (M7_InputState_KeyDown(is, SDL_SCANCODE_A))
        input_axis.x -= 1;
    if (M7_InputState_KeyDown(is, SDL_SCANCODE_D))
        input_axis.x += 1;
    if (M7_InputState_KeyDown(is, SDL_SCANCODE_S))
        input_axis.z -= 1;
    if (M7_InputState_KeyDown(is, SDL_SCANCODE_W))
        input_axis.z += 1;
    if (M7_InputState_KeyDown(is, SDL_SCANCODE_LSHIFT))
        input_axis.y -= 1;
    if (M7_InputState_KeyDown(is, SDL_SCANCODE_SPACE))
        input_axis.y += 1;

    input_axis = (vec3_eq(input_axis, vec3_zero)) ? vec3_zero : vec3_normalize(input_axis);
    input_axis = vec3_rotate(input_axis, vec3_j, cam->yaw);
    *pos = vec3_add(*pos, vec3_mul(input_axis, 200 * delta));

    vec2 mouse_motion = M7_InputState_GetMouseMotion(is);
    cam->yaw += mouse_motion.x * 0.001;
    cam->pitch += mouse_motion.y * 0.001;

    *basis = mat3x3_rotate(
        mat3x3_rotate(mat3x3_identity, vec3_i, cam->pitch),
        vec3_j, cam->yaw
    );
}
