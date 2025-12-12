#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/stride.h>

#include "M7_Bitmap_c.h"

void M7_Bitmap_RegisterToECS(ECS *ecs) {
    M7_Components.Canvas = ECS_RegisterComponent(ecs, M7_Canvas, {
        .attach = SD_SELECT(M7_Canvas_Attach),
        .init = SD_SELECT(M7_Canvas_Init),
        .free = M7_Canvas_Free
    });

    M7_Components.Viewport = ECS_RegisterComponent(ecs, M7_Viewport, {
        .init = M7_Viewport_Init,
        .free = M7_Viewport_Free
    });

    M7_Components.TextureBank = ECS_RegisterComponent(ecs, M7_ResourceBank(M7_Texture *) *, {
        .attach = M7_TextureBank_Attach,
        .detach = M7_TextureBank_Detach
    });

    ECS_SystemGroup_RegisterSystem(M7_SystemGroups.RenderPresent, SD_SELECT(M7_Canvas_Present), M7_Components.Viewport, M7_Components.Canvas);
}

