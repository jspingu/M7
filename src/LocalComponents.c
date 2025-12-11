#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include "LocalComponents.h"

struct LocalComponents Components;

void RegisterToECS(ECS *ecs) {
    Components.FreeCam = ECS_RegisterComponent(ecs, FreeCam, {});

    ECS_SystemGroup_RegisterSystem(M7_SystemGroups.Update, FreeCam_Update,
        Components.FreeCam,
        M7_Components.Position,
        M7_Components.Basis
    );
}
