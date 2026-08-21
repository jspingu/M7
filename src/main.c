#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_ECS.h>
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
    TX_RegisterToECS(ecs);
    RegisterToECS(ecs);

    ECS_Handle *root = ECS_GetRoot(ecs);

    ECS_Entity_AttachComponents(root,
        { TX_Components.Viewport, &(TX_ViewportArgs){
            .title = "Good morning!",
            .width = WIDTH,
            .height = HEIGHT
        }},
        { TX_Components.InputState, nullptr },
        { TX_Components.TextureBank, nullptr },
        { TX_Components.Canvas, &(TX_Canvas){
            .width = WIDTH,
            .height = HEIGHT
        }},
        { Components.GrabMouse, &(bool){} }
    );

    ECS_Entity_AddChildren(root, 
        { /* Main world */
            ECS_Components(
                { TX_Components.World, nullptr },
                { TX_Components.XformComposer, &(TX_XformComposer){TX_XformComposeDefault}},
                { TX_Components.LightEnvironment, &(TX_LightEnvironment){ .ambient=0.08, .sky_texture_path="assets/Nalovardo.png" } }
            ),
            ECS_Children(
                { /* Camera */
                    ECS_Components(
                        { TX_Components.ParallelProjector, &(TX_ParallelProjector) {
                            .slope = { .x=0, .y=0 },
                            .scale = { .x=0.5, .y=0.5 }
                        }},
                        { TX_Components.PerspectiveFOV, &(float) { SDL_PI_F / 2 } },
                        { TX_Components.Rasterizer, &(TX_RasterizerArgs) {
                            .project = SD_SELECT(TX_ProjectPerspective),
                            .scan = SD_SELECT(TX_ScanPerspective),
                            .near = 1
                        }},
                        { TX_Components.Position, &(vec3){} },
                        { TX_Components.Basis, (mat3x3 []){mat3x3_identity} },
                        { Components.FreeCam, &(FreeCam){} }
                    )
                },
                { /* Teapot */
                    ECS_Components(
                        { TX_Components.Position, &(vec3){ .y=-150, .z=600 } },
                        { TX_Components.Basis, (mat3x3 []){mat3x3_identity} },
                        { TX_Components.MeshPrimitive, nullptr },
                        { TX_Components.Teapot, &(TX_Teapot) { .scale=50 } },
                        { TX_Components.Model, &(TX_ModelArgs) { .get_mesh = TX_Teapot_GetMesh }},
                        { TX_Components.XformComposer, &(TX_XformComposer){TX_XformComposeDefault} }
                    ),
                    ECS_Children({ECS_Components(
                        { TX_Components.SolidColor, &(TX_SolidColor) { .r=1.0, .g=1.0, .b=1.0 } },
                        { TX_Components.Lighting, &(TX_OpticalMedium) { .reflectivity=1.0, .specularity=1.0, .exp=4 } },
                        { TX_Components.ModelInstance, &(TX_ModelInstanceArgs) {
                            .shader_components = (ECS_Component(TX_ShaderComponent) *[]) { TX_Components.SolidColor, TX_Components.Lighting },
                            .nshaders = 2,
                            .render_batch = Opaque,
                            .flags = TX_RASTERIZER_CULL_BACKFACE
                                   | TX_RASTERIZER_TEST_DEPTH
                                   | TX_RASTERIZER_WRITE_DEPTH
                                   | TX_RASTERIZER_INTERPOLATE_NORMALS
                        }}
                    )})
                },
                { /* Floor */
                    ECS_Components(
                        { TX_Components.Position, &(vec3){ .y=-150, .z=600 } },
                        { TX_Components.Basis, (mat3x3 []){mat3x3_rotate(mat3x3_identity, vec3_i, SDL_PI_F / 2)} },
                        { TX_Components.MeshPrimitive, nullptr },
                        { TX_Components.Rect, &(TX_Rect) { .width=2000, .height=2000 } },
                        { TX_Components.Model, &(TX_ModelArgs) { .get_mesh = TX_Rect_GetMesh }},
                        { TX_Components.XformComposer, &(TX_XformComposer){TX_XformComposeDefault} }
                    ),
                    ECS_Children({ECS_Components(
                        { TX_Components.Checkerboard, &(TX_Checkerboard) {
                            .tiles = 31,
                            .r1 = 0.4, .g1 = 0.4, .b1 = 0.8,
                            .r2 = 1.0, .g2 = 1.0, .b2 = 1.0,
                        }},
                        { TX_Components.Lighting, &(TX_OpticalMedium) { .reflectivity=0.4, .specularity=0.4, .exp=4 } },
                        { TX_Components.ModelInstance, &(TX_ModelInstanceArgs) {
                            .shader_components = (ECS_Component(TX_ShaderComponent) *[]) { TX_Components.Checkerboard, TX_Components.Lighting },
                            .nshaders = 2,
                            .render_batch = Opaque,
                            .flags = TX_RASTERIZER_CULL_BACKFACE
                                   | TX_RASTERIZER_TEST_DEPTH
                                   | TX_RASTERIZER_WRITE_DEPTH
                        }}
                    )})
                },
                { /* Light */
                    ECS_Components(
                        { TX_Components.Position, &(vec3){ .x=-150, .y=-115, .z=400 } },
                        { TX_Components.Basis, (mat3x3 []){mat3x3_identity} },
                        { TX_Components.MeshPrimitive, nullptr },
                        { TX_Components.Sphere, &(TX_Sphere) { .radius=32, .nrings=16, .ring_precision=16 } },
                        { TX_Components.Model, &(TX_ModelArgs) { .get_mesh = TX_Sphere_GetMesh }},
                        { TX_Components.XformComposer, &(TX_XformComposer){TX_XformComposeDefault} },
                        { TX_Components.PointLight, &(TX_PointLight) { .col={{ 1.0, 0.8, 0.2 }}, .energy=20000 } }
                    ),
                    ECS_Children({ECS_Components(
                        { TX_Components.SolidColor, &(TX_SolidColor) { .r=1, .g=1, .b=1 }},
                        { TX_Components.ModelInstance, &(TX_ModelInstanceArgs) {
                            .shader_components = (ECS_Component(TX_ShaderComponent) *[]) { TX_Components.SolidColor },
                            .nshaders = 1,
                            .render_batch = Opaque,
                            .flags = TX_RASTERIZER_CULL_BACKFACE
                                   | TX_RASTERIZER_TEST_DEPTH
                                   | TX_RASTERIZER_WRITE_DEPTH
                        }}
                    )})
                },
                { /* Light */
                    ECS_Components(
                        { TX_Components.Position, &(vec3){ .x=150, .y=-115, .z=400 } },
                        { TX_Components.Basis, (mat3x3 []){mat3x3_identity} },
                        { TX_Components.MeshPrimitive, nullptr },
                        { TX_Components.Sphere, &(TX_Sphere) { .radius=32, .nrings=16, .ring_precision=16 } },
                        { TX_Components.Model, &(TX_ModelArgs) { .get_mesh = TX_Sphere_GetMesh }},
                        { TX_Components.XformComposer, &(TX_XformComposer){TX_XformComposeDefault} },
                        { TX_Components.PointLight, &(TX_PointLight) { .col={{ 0.2, 1.0, 0.5 }}, .energy=20000 } }
                    ),
                    ECS_Children({ECS_Components(
                        { TX_Components.SolidColor, &(TX_SolidColor) { .r=1, .g=1, .b=1 }},
                        { TX_Components.ModelInstance, &(TX_ModelInstanceArgs) {
                            .shader_components = (ECS_Component(TX_ShaderComponent) *[]) { TX_Components.SolidColor },
                            .nshaders = 1,
                            .render_batch = Opaque,
                            .flags = TX_RASTERIZER_CULL_BACKFACE
                                   | TX_RASTERIZER_TEST_DEPTH
                                   | TX_RASTERIZER_WRITE_DEPTH
                        }}
                    )})
                },
                { /* Light */
                    ECS_Components(
                        { TX_Components.Position, &(vec3){ .x=-150, .y=-115, .z=800 } },
                        { TX_Components.Basis, (mat3x3 []){mat3x3_identity} },
                        { TX_Components.MeshPrimitive, nullptr },
                        { TX_Components.Sphere, &(TX_Sphere) { .radius=32, .nrings=16, .ring_precision=16 } },
                        { TX_Components.Model, &(TX_ModelArgs) { .get_mesh = TX_Sphere_GetMesh }},
                        { TX_Components.XformComposer, &(TX_XformComposer){TX_XformComposeDefault} },
                        { TX_Components.PointLight, &(TX_PointLight) { .col={{ 1.0, 0.2, 0.1 }}, .energy=20000 } }
                    ),
                    ECS_Children({ECS_Components(
                        { TX_Components.SolidColor, &(TX_SolidColor) { .r=1, .g=1, .b=1 }},
                        { TX_Components.ModelInstance, &(TX_ModelInstanceArgs) {
                            .shader_components = (ECS_Component(TX_ShaderComponent) *[]) { TX_Components.SolidColor },
                            .nshaders = 1,
                            .render_batch = Opaque,
                            .flags = TX_RASTERIZER_CULL_BACKFACE
                                   | TX_RASTERIZER_TEST_DEPTH
                                   | TX_RASTERIZER_WRITE_DEPTH
                        }}
                    )})
                },
                { /* Light */
                    ECS_Components(
                        { TX_Components.Position, &(vec3){ .x=150, .y=-115, .z=800 } },
                        { TX_Components.Basis, (mat3x3 []){mat3x3_identity} },
                        { TX_Components.MeshPrimitive, nullptr },
                        { TX_Components.Sphere, &(TX_Sphere) { .radius=32, .nrings=16, .ring_precision=16 } },
                        { TX_Components.Model, &(TX_ModelArgs) { .get_mesh = TX_Sphere_GetMesh }},
                        { TX_Components.XformComposer, &(TX_XformComposer){TX_XformComposeDefault} },
                        { TX_Components.PointLight, &(TX_PointLight) { .col={{ 0.9, 0.2, 1.0 }}, .energy=20000 } }
                    ),
                    ECS_Children({ECS_Components(
                        { TX_Components.SolidColor, &(TX_SolidColor) { .r=1, .g=1, .b=1 }},
                        { TX_Components.ModelInstance, &(TX_ModelInstanceArgs) {
                            .shader_components = (ECS_Component(TX_ShaderComponent) *[]) { TX_Components.SolidColor },
                            .nshaders = 1,
                            .render_batch = Opaque,
                            .flags = TX_RASTERIZER_CULL_BACKFACE
                                   | TX_RASTERIZER_TEST_DEPTH
                                   | TX_RASTERIZER_WRITE_DEPTH
                        }}
                    )})
                },
                { /* Skybox */
                    ECS_Components(
                        { TX_Components.MeshPrimitive, nullptr },
                        { TX_Components.Cubemap, &(TX_Cubemap) { .scale=100 } },
                        { TX_Components.Model, &(TX_ModelArgs) { .get_mesh = TX_Cubemap_GetMesh } },
                        { TX_Components.XformComposer, &(TX_XformComposer){TX_XformComposeCubemap} },
                    ),
                    ECS_Children({ECS_Components(
                        { TX_Components.Sky, nullptr },
                        { TX_Components.ModelInstance, &(TX_ModelInstanceArgs) {
                            .shader_components = (ECS_Component(TX_ShaderComponent) *[]) { TX_Components.Sky },
                            .nshaders = 1,
                            .render_batch = Sky,
                        }}
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

    ECS_SystemGroup_Process(TX_SystemGroups.Update, delta);

    ECS_Update(ecs);

    ECS_SystemGroup_Process(TX_SystemGroups.PostUpdate);
    ECS_SystemGroup_ProcessReverse(TX_SystemGroups.Render);
    ECS_SystemGroup_Process(TX_SystemGroups.RenderPresent);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    (void)appstate;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        default:
            ECS_SystemGroup_Process(TX_SystemGroups.OnSDLEvent, event);
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
