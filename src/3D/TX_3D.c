#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/stride.h>

#include "TX_3D_c.h"

void TX_3D_RegisterToECS(ECS *ecs) {
    TX_Components.World = ECS_RegisterComponent(ecs, TX_World, {
        .init = TX_World_Init,
        .free = TX_World_Free,
    });

    TX_Components.Rasterizer = ECS_RegisterComponent(ecs, TX_Rasterizer, {
        .attach = TX_Rasterizer_Attach,
        .init = TX_Rasterizer_Init
    });

    TX_Components.Model = ECS_RegisterComponent(ecs, TX_Model, {
        .attach = TX_Model_Attach,
        .detach = TX_Model_Detach,
        .init = TX_Model_Init
    });

    TX_Components.ModelInstance = ECS_RegisterComponent(ecs, TX_RenderInstance, {
        .attach = TX_ModelInstance_Attach,
        .detach = TX_ModelInstance_Detach,
        .init = TX_ModelInstance_Init,
        .free = TX_ModelInstance_Free
    });

    TX_Components.XformComposer = ECS_RegisterComponent(ecs, TX_XformComposer, {});
    TX_Components.Position = ECS_RegisterComponent(ecs, vec3, {});
    TX_Components.Basis = ECS_RegisterComponent(ecs, mat3x3, {});
    TX_Components.ParallelProjector = ECS_RegisterComponent(ecs, TX_ParallelProjector, {});
    TX_Components.PerspectiveFOV = ECS_RegisterComponent(ecs, TX_PerspectiveFOV, { .init = TX_PerspectiveFOV_Init });

    TX_Components.MeshPrimitive = ECS_RegisterComponent(ecs, TX_Mesh *, { .init = TX_MeshPrimitive_Init, .free = TX_MeshPrimitive_Free });
    TX_Components.Teapot = ECS_RegisterComponent(ecs, TX_Teapot, {});
    TX_Components.Torus = ECS_RegisterComponent(ecs, TX_Torus, {});
    TX_Components.Sphere = ECS_RegisterComponent(ecs, TX_Sphere, {});
    TX_Components.Rect = ECS_RegisterComponent(ecs, TX_Rect, {});
    TX_Components.Cubemap = ECS_RegisterComponent(ecs, TX_Cubemap, {});

    TX_Components.LightEnvironment = ECS_RegisterComponent(ecs, TX_LightEnvironment *, {
        .attach = TX_LightEnvironment_Attach,
        .detach = TX_LightEnvironment_Detach,
        .init = TX_LightEnvironment_Init,
        .free = TX_LightEnvironment_Free
    });

    TX_Components.PointLight = ECS_RegisterComponent(ecs, TX_PointLight, {
        .attach = TX_PointLight_Attach,
        .detach = TX_PointLight_Detach
    });

    TX_Components.SolidColor = ECS_RegisterComponent(ecs, TX_ShaderComponent, {
        .init = TX_SolidColor_Init,
        .free = TX_ShaderComponent_Free
    });

    TX_Components.Checkerboard = ECS_RegisterComponent(ecs, TX_ShaderComponent, {
        .init = TX_Checkerboard_Init,
        .free = TX_ShaderComponent_Free
    });

    TX_Components.Lighting = ECS_RegisterComponent(ecs, TX_ShaderComponent, {
        .attach = TX_Lighting_Attach,
        .init = TX_Lighting_Init,
        .free = TX_ShaderComponent_Free
    });

    TX_Components.Sky = ECS_RegisterComponent(ecs, TX_ShaderComponent, {
        .attach = TX_Sky_Attach,
        .init = TX_Sky_Init,
        .free = TX_ShaderComponent_Free
    });

    TX_Components.TextureMap = ECS_RegisterComponent(ecs, TX_ShaderComponent, {
        .attach = TX_TextureMap_Attach,
        .detach = TX_TextureMap_Detach,
        .init = TX_TextureMap_Init,
        .free = TX_TextureMap_Free
    });

    ECS_SystemGroup_RegisterSystem(TX_SystemGroups.Render, SD_SELECT(TX_Rasterizer_Render), TX_Components.Rasterizer);
    ECS_SystemGroup_RegisterSystem(TX_SystemGroups.OnXform, TX_Model_OnXform, TX_Components.Model);
    ECS_SystemGroup_RegisterSystem(TX_SystemGroups.OnXform, TX_PointLight_OnXform, TX_Components.PointLight);
}
