#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "LocalComponents.h"

#define WIDTH    960
#define HEIGHT   540
#define FPS_CAP  60

enum RenderBatches {
    Sky,
    Opaque
};

static Uint64 ticks_prev;
static Uint64 ticks_freq;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    (void)argc, (void)argv;
    ECS *ecs = ECS_Create();
    M7_RegisterToECS(ecs);
    RegisterToECS(ecs);

    ECS_Handle *root = ECS_GetRoot(ecs);

    ECS_Entity_AttachComponents(root,
        { M7_Components.Viewport, &(M7_ViewportArgs){
            .title = "Good morning!",
            .width = WIDTH,
            .height = HEIGHT
        }},
        { M7_Components.InputState, nullptr },
        { M7_Components.TextureBank, nullptr },
        { M7_Components.Canvas, &(M7_Canvas){
            .width = WIDTH,
            .height = HEIGHT,
            .parallelism = SDL_GetNumLogicalCPUCores()
        }},
        { Components.GrabMouse, &(bool){} }
    );

    ECS_Entity_AddChildren(root, 
        { /* Main world */
            ECS_Components(
                { M7_Components.World, nullptr },
                { M7_Components.XformComposer, &(M7_XformComposer){M7_XformComposeDefault}},
                { M7_Components.LightEnvironment, &(M7_LightEnvironment){ .ambient=0.08 } }
            ),
            ECS_Children(
                { /* Camera */
                    ECS_Components(
                        { M7_Components.ParallelProjector, &(M7_ParallelProjector) {
                            .slope = { .x=0, .y=0 },
                            .scale = { .x=0.5, .y=0.5 }
                        }},
                        { M7_Components.PerspectiveFOV, &(float) { SDL_PI_F / 2 } },
                        { M7_Components.Rasterizer, &(M7_RasterizerArgs) {
                            .project = SD_SELECT(M7_ProjectPerspective),
                            .scan = SD_SELECT(M7_ScanPerspective),
                            .near = 1,
                            .parallelism = SDL_GetNumLogicalCPUCores()
                        }},
                        { M7_Components.Position, &(vec3){} },
                        { M7_Components.Basis, (mat3x3 []){mat3x3_identity} },
                        { Components.FreeCam, &(FreeCam){} }
                    )
                },
                { /* Teapot */
                    ECS_Components(
                        { M7_Components.Position, &(vec3){ .y=-150, .z=600 } },
                        { M7_Components.Basis, (mat3x3 []){mat3x3_identity} },
                        { M7_Components.MeshPrimitive, nullptr },
                        { M7_Components.Teapot, &(M7_Teapot) { .scale = 50 } },
                        { M7_Components.Model, &(M7_ModelArgs) { .get_mesh = M7_Teapot_GetMesh }},
                        { M7_Components.XformComposer, &(M7_XformComposer){M7_XformComposeDefault} }
                    ),
                    ECS_Children({ECS_Components(
                        { M7_Components.SolidColor, &(M7_SolidColor) { .r=0.7, .g=0.7, .b=1.0 } },
                        { M7_Components.Lighting, &(M7_OpticalMedium) { .reflectivity=1, .specularity=0.6, .exp=4 } },
                        { M7_Components.ModelInstance, &(M7_ModelInstanceArgs) {
                            .shader_components = (ECS_Component(M7_ShaderComponent) *[]) { M7_Components.SolidColor, M7_Components.Lighting },
                            .nshaders = 2,
                            .render_batch = Opaque,
                            .flags = M7_RASTERIZER_CULL_BACKFACE
                                   | M7_RASTERIZER_TEST_DEPTH
                                   | M7_RASTERIZER_WRITE_DEPTH
                                   | M7_RASTERIZER_INTERPOLATE_NORMALS
                        }}
                    )})
                },
                { /* Floor */
                    ECS_Components(
                        { M7_Components.Position, &(vec3){ .y=-150, .z=600 } },
                        { M7_Components.Basis, (mat3x3 []){mat3x3_rotate(mat3x3_identity, vec3_i, SDL_PI_F / 2)} },
                        { M7_Components.MeshPrimitive, nullptr },
                        { M7_Components.Rect, &(M7_Rect) { .width=2000, .height=2000 } },
                        { M7_Components.Model, &(M7_ModelArgs) { .get_mesh = M7_Rect_GetMesh }},
                        { M7_Components.XformComposer, &(M7_XformComposer){M7_XformComposeDefault} }
                    ),
                    ECS_Children({ECS_Components(
                        { M7_Components.Checkerboard, &(M7_Checkerboard) {
                            .tiles = 31,
                            .r1 = 0.4, .g1 = 0.4, .b1 = 0.8,
                            .r2 = 1.0, .g2 = 1.0, .b2 = 1.0,
                        }},
                        { M7_Components.Lighting, &(M7_OpticalMedium) { .reflectivity=0.5, .specularity=1, .exp=6 } },
                        { M7_Components.ModelInstance, &(M7_ModelInstanceArgs) {
                            .shader_components = (ECS_Component(M7_ShaderComponent) *[]) { M7_Components.Checkerboard, M7_Components.Lighting },
                            .nshaders = 2,
                            .render_batch = Opaque,
                            .flags = M7_RASTERIZER_CULL_BACKFACE
                                   | M7_RASTERIZER_TEST_DEPTH
                                   | M7_RASTERIZER_WRITE_DEPTH
                        }}
                    )})
                },
                { /* Light */
                    ECS_Components(
                        { M7_Components.Position, &(vec3){ .x=-200, .y=-115, .z=400 } },
                        { M7_Components.Basis, (mat3x3 []){mat3x3_identity} },
                        { M7_Components.MeshPrimitive, nullptr },
                        { M7_Components.Sphere, &(M7_Sphere) { .radius=32, .nrings=16, .ring_precision=16 } },
                        { M7_Components.Model, &(M7_ModelArgs) { .get_mesh = M7_Sphere_GetMesh }},
                        { M7_Components.XformComposer, &(M7_XformComposer){M7_XformComposeDefault} },
                        { M7_Components.PointLight, &(M7_PointLight) { .col={{ 1.0, 0.8, 0.2 }}, .energy=10000 } }
                    ),
                    ECS_Children({ECS_Components(
                        { M7_Components.SolidColor, &(M7_SolidColor) { .r=1, .g=1, .b=1 }},
                        { M7_Components.ModelInstance, &(M7_ModelInstanceArgs) {
                            .shader_components = (ECS_Component(M7_ShaderComponent) *[]) { M7_Components.SolidColor },
                            .nshaders = 1,
                            .render_batch = Opaque,
                            .flags = M7_RASTERIZER_CULL_BACKFACE
                                   | M7_RASTERIZER_TEST_DEPTH
                                   | M7_RASTERIZER_WRITE_DEPTH
                        }}
                    )})
                },
                { /* Light */
                    ECS_Components(
                        { M7_Components.Position, &(vec3){ .x=200, .y=-115, .z=400 } },
                        { M7_Components.Basis, (mat3x3 []){mat3x3_identity} },
                        { M7_Components.MeshPrimitive, nullptr },
                        { M7_Components.Sphere, &(M7_Sphere) { .radius=32, .nrings=16, .ring_precision=16 } },
                        { M7_Components.Model, &(M7_ModelArgs) { .get_mesh = M7_Sphere_GetMesh }},
                        { M7_Components.XformComposer, &(M7_XformComposer){M7_XformComposeDefault} },
                        { M7_Components.PointLight, &(M7_PointLight) { .col={{ 0.2, 1.0, 0.5 }}, .energy=10000 } }
                    ),
                    ECS_Children({ECS_Components(
                        { M7_Components.SolidColor, &(M7_SolidColor) { .r=1, .g=1, .b=1 }},
                        { M7_Components.ModelInstance, &(M7_ModelInstanceArgs) {
                            .shader_components = (ECS_Component(M7_ShaderComponent) *[]) { M7_Components.SolidColor },
                            .nshaders = 1,
                            .render_batch = Opaque,
                            .flags = M7_RASTERIZER_CULL_BACKFACE
                                   | M7_RASTERIZER_TEST_DEPTH
                                   | M7_RASTERIZER_WRITE_DEPTH
                        }}
                    )})
                }
            )
        },
        { /* Skybox world */
            ECS_Components(
                { M7_Components.World, nullptr },
                { M7_Components.XformComposer, &(M7_XformComposer){M7_XformComposeCubemap} }
            ),
            ECS_Children(
                { /* Camera */
                    ECS_Components(
                        { M7_Components.PerspectiveFOV, &(float) { SDL_PI_F / 2 } },
                        { M7_Components.Rasterizer, &(M7_RasterizerArgs) {
                            .project = SD_SELECT(M7_ProjectPerspective),
                            .scan = SD_SELECT(M7_ScanPerspective),
                            .near = 1,
                            .parallelism = SDL_GetNumLogicalCPUCores()
                        }},
                        { M7_Components.Position, &(vec3){}},
                        { M7_Components.Basis, (mat3x3 []){mat3x3_identity}},
                        { Components.FreeCam, &(FreeCam){} }
                    )
                },
                { /* Skybox */
                    ECS_Components(
                        { M7_Components.MeshPrimitive, nullptr },
                        { M7_Components.Cubemap, &(M7_Cubemap) { .scale = 1000 } },
                        { M7_Components.Model, &(M7_ModelArgs) { .get_mesh = M7_Cubemap_GetMesh }},
                        { M7_Components.XformComposer, &(M7_XformComposer){M7_XformComposeCubemap} }
                    ),
                    ECS_Children({ECS_Components(
                        { M7_Components.TextureMap, "assets/Nalovardo.png" },
                        { M7_Components.ModelInstance, &(M7_ModelInstanceArgs) {
                            .shader_components = (ECS_Component(M7_ShaderComponent) *[]) { M7_Components.TextureMap },
                            .nshaders = 1,
                            .render_batch = Sky,
                            .flags = 0
                        }},
                    )})
                }
            )
        },
    );

    ECS_Update(ecs);

    ticks_prev = SDL_GetPerformanceCounter();
    ticks_freq = SDL_GetPerformanceFrequency();
    *appstate = ecs;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    ECS *ecs = appstate;
    Uint64 ticks_frame = ticks_freq / FPS_CAP;
    Uint64 ticks_curr = SDL_GetPerformanceCounter();
    Uint64 ticks_delta = ticks_curr - ticks_prev;

    if (ticks_delta < ticks_frame) {
        Uint64 ticks_rem = ticks_frame - ticks_delta;
        SDL_Delay(ticks_rem * 1000 / ticks_freq);

        do ticks_curr = SDL_GetPerformanceCounter();
        while (ticks_curr - ticks_prev < ticks_frame);
    }

    double delta = (double)(ticks_curr - ticks_prev) / ticks_freq;
    ticks_prev = ticks_curr;

    printf("FPS: %li              \n\x1b[F", SDL_lround(1/delta));

    ECS_SystemGroup_Process(M7_SystemGroups.Update, delta);

    ECS_Update(ecs);

    ECS_SystemGroup_Process(M7_SystemGroups.PostUpdate);
    ECS_SystemGroup_ProcessReverse(M7_SystemGroups.Render);
    ECS_SystemGroup_Process(M7_SystemGroups.RenderPresent);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    (void)appstate;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        default:
            ECS_SystemGroup_Process(M7_SystemGroups.OnSDLEvent, event);
            break;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    (void)result;
    ECS *ecs = appstate;

    ECS_Free(ecs);
    SDL_Quit();
}
