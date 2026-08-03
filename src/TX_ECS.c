#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/stride.h>

#include "TX_InputState_c.h"
#include "3D/TX_3D_c.h"
#include "Bitmap/TX_Bitmap_c.h"

struct TX_Components TX_Components;
struct TX_SystemGroups TX_SystemGroups;

void TX_RegisterToECS(ECS *ecs) {
    TX_SystemGroups.OnSDLEvent = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.Update = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.PostUpdate = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.Render = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.RenderPresent = ECS_RegisterSystemGroup(ecs);
    TX_SystemGroups.OnXform = ECS_RegisterSystemGroup(ecs);

    TX_Components.InputState = ECS_RegisterComponent(ecs, TX_InputState, {
        .init = TX_InputState_Init,
        .free = TX_InputState_Free
    });

    ECS_SystemGroup_RegisterSystem(TX_SystemGroups.OnSDLEvent, TX_InputState_OnSDLEvent, TX_Components.InputState);
    ECS_SystemGroup_RegisterSystem(TX_SystemGroups.PostUpdate, TX_InputState_Step, TX_Components.InputState);

    TX_3D_RegisterToECS(ecs);
    TX_Bitmap_RegisterToECS(ecs);
}
