#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/stride.h>

#include "Components/M7_Viewport_c.h"
#include "Components/M7_InputState_c.h"
#include "Components/3D/M7_3D_c.h"

struct M7_Components M7_Components;
struct M7_SystemGroups M7_SystemGroups;

void M7_RegisterToECS(ECS *ecs) {
    M7_SystemGroups.OnSDLEvent = ECS_RegisterSystemGroup(ecs);
    M7_SystemGroups.Update = ECS_RegisterSystemGroup(ecs);
    M7_SystemGroups.PostUpdate = ECS_RegisterSystemGroup(ecs);
    M7_SystemGroups.Render = ECS_RegisterSystemGroup(ecs);
    M7_SystemGroups.RenderPresent = ECS_RegisterSystemGroup(ecs);
    M7_SystemGroups.OnXform = ECS_RegisterSystemGroup(ecs);

    M7_Components.Viewport = ECS_RegisterComponent(ecs, M7_Viewport, {
        .init = M7_Viewport_Init,
        .free = M7_Viewport_Free
    });

    M7_Components.InputState = ECS_RegisterComponent(ecs, M7_InputState, {
        .init = M7_InputState_Init,
        .free = M7_InputState_Free
    });

    M7_Components.CameraMovement = ECS_RegisterComponent(ecs, CameraMovement, {});

    ECS_SystemGroup_RegisterSystem(M7_SystemGroups.OnSDLEvent, M7_InputState_OnSDLEvent, M7_Components.InputState);
    ECS_SystemGroup_RegisterSystem(M7_SystemGroups.PostUpdate, M7_InputState_Step, M7_Components.InputState);

    M7_3D_RegisterToECS(ecs);

    ECS_SystemGroup_RegisterSystem(M7_SystemGroups.Update, CameraMovement_Update,
        M7_Components.CameraMovement,
        M7_Components.Position,
        M7_Components.Basis
    );
}
