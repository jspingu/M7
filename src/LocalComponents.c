#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/linalg.h>
#include "LocalComponents.h"

struct LocalComponents Components;

static bool vec3_eq(vec3 lhs, vec3 rhs) {
    return lhs.x == rhs.x &&
           lhs.y == rhs.y &&
           lhs.z == rhs.y;
}

void GrabMouse_Update(ECS_Handle *self, double delta) {
    (void)delta;
    bool *grabbed = ECS_Entity_GetComponent(self, Components.GrabMouse);
    ECS_Handle *is = ECS_Entity_AncestorWithComponent(self, M7_Components.InputState, true);
    ECS_Handle *vp = ECS_Entity_AncestorWithComponent(self, M7_Components.Viewport, true);
    M7_Viewport *c_vp = ECS_Entity_GetComponent(vp, M7_Components.Viewport);
    
    if (M7_InputState_KeyJustDown(is, SDL_SCANCODE_ESCAPE)) {
        *grabbed = !*grabbed;       
        SDL_SetWindowRelativeMouseMode(c_vp->window, *grabbed);
    }
}

void FreeCam_Update(ECS_Handle *self, double delta) {
    ECS_Handle *mouse_grab = ECS_Entity_AncestorWithComponent(self, Components.GrabMouse, true);
    ECS_Handle *is = ECS_Entity_AncestorWithComponent(self, M7_Components.InputState, true);

    if (!*ECS_Entity_GetComponent(mouse_grab, Components.GrabMouse))
        return;

    FreeCam *cam = ECS_Entity_GetComponent(self, Components.FreeCam);
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
    *pos = vec3_add(*pos, vec3_mul(input_axis, 250 * delta));

    vec2 mouse_motion = M7_InputState_GetMouseMotion(is);

    cam->yaw += mouse_motion.x * 0.15 * delta;
    cam->yaw = cam->yaw - SDL_floor(cam->yaw / (2 * SDL_PI_F)) * 2 * SDL_PI_F;
    cam->pitch += mouse_motion.y * 0.15 * delta;
    cam->pitch = SDL_clamp(cam->pitch, -SDL_PI_F / 2, SDL_PI_F / 2);

    ECS_Handle *fov = ECS_Entity_AncestorWithComponent(self, M7_Components.PerspectiveFOV, true);
    float fov_curr = ECS_Entity_GetComponent(fov, M7_Components.PerspectiveFOV)->fov;
    M7_PerspectiveFOV_Set(fov, fov_curr - M7_InputState_GetWheelMotion(is).y * 16 * delta);

    *basis = mat3x3_rotate(
        mat3x3_rotate(mat3x3_identity, vec3_i, cam->pitch),
        vec3_j, cam->yaw
    );
}

void RegisterToECS(ECS *ecs) {
    Components.GrabMouse = ECS_RegisterComponent(ecs, bool, {});
    Components.FreeCam = ECS_RegisterComponent(ecs, FreeCam, {});

    ECS_SystemGroup_RegisterSystem(M7_SystemGroups.Update, GrabMouse_Update, Components.GrabMouse);

    ECS_SystemGroup_RegisterSystem(M7_SystemGroups.Update, FreeCam_Update,
        Components.FreeCam,
        M7_Components.Position,
        M7_Components.Basis
    );
}
