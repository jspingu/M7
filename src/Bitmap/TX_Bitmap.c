#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/stride.h>

#include "TX_Bitmap_c.h"

void TX_Bitmap_RegisterToECS(ECS *ecs) {
    TX_Components.Canvas = ECS_RegisterComponent(ecs, TX_Canvas, {
        .init = SD_SELECT(TX_Canvas_Init),
        .free = TX_Canvas_Free
    });

    TX_Components.Viewport = ECS_RegisterComponent(ecs, TX_Viewport, {
        .init = TX_Viewport_Init,
        .free = TX_Viewport_Free
    });

    TX_Components.TextureBank = ECS_RegisterComponent(ecs, TX_ResourceBank, {
        .attach = TX_TextureBank_Attach,
        .detach = TX_ResourceBank_Detach
    });

    ECS_SystemGroup_RegisterSystem(TX_SystemGroups.RenderPresent, SD_SELECT(TX_Canvas_Present), TX_Components.Viewport, TX_Components.Canvas);
}

