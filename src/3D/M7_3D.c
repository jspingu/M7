#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Math/stride.h>

#include "M7_3D_c.h"

void M7_3D_RegisterToECS(ECS *ecs) {
    M7_Components.World = ECS_RegisterComponent(ecs, M7_World, {
        .init = M7_World_Init,
        .free = M7_World_Free,
    });

    M7_Components.Rasterizer = ECS_RegisterComponent(ecs, M7_Rasterizer, {
        .attach = M7_Rasterizer_Attach,
        .init = M7_Rasterizer_Init
    });

    M7_Components.Model = ECS_RegisterComponent(ecs, M7_Model, {
        .attach = M7_Model_Attach,
        .detach = M7_Model_Detach,
        .init = M7_Model_Init,
        .free = M7_Model_Free
    });

    M7_Components.XformComposer = ECS_RegisterComponent(ecs, M7_XformComposer, {});
    M7_Components.Position = ECS_RegisterComponent(ecs, vec3, {});
    M7_Components.Basis = ECS_RegisterComponent(ecs, mat3x3, {});
    M7_Components.PerspectiveFOV = ECS_RegisterComponent(ecs, M7_PerspectiveFOV, { .init = M7_PerspectiveFOV_Init });

    M7_Components.MeshPrimitive = ECS_RegisterComponent(ecs, M7_Mesh *, { .init = M7_MeshPrimitive_Init, .free = M7_MeshPrimitive_Free });
    M7_Components.Teapot = ECS_RegisterComponent(ecs, M7_Teapot, {});
    M7_Components.Torus = ECS_RegisterComponent(ecs, M7_Torus, {});
    M7_Components.Sphere = ECS_RegisterComponent(ecs, M7_Sphere, {});
    M7_Components.Rect = ECS_RegisterComponent(ecs, M7_Rect, {});
    M7_Components.Cubemap = ECS_RegisterComponent(ecs, M7_Cubemap, {});

    M7_Components.SolidColor = ECS_RegisterComponent(ecs, M7_SolidColor, {});
    M7_Components.Checkerboard = ECS_RegisterComponent(ecs, M7_Checkerboard, {});
    M7_Components.TextureMap = ECS_RegisterComponent(ecs, M7_TextureMap, {
        .attach = M7_TextureMap_Attach,
        .detach = M7_TextureMap_Detach,
        .init = M7_TextureMap_Init,
        .free = M7_TextureMap_Free
    });

    ECS_SystemGroup_RegisterSystem(M7_SystemGroups.Render, SD_SELECT(M7_Rasterizer_Render), M7_Components.Rasterizer);
    ECS_SystemGroup_RegisterSystem(M7_SystemGroups.OnXform, M7_Model_OnXform, M7_Components.Model);
}
