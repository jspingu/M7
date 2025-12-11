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
        { M7_Components.Canvas, &(M7_Canvas){
            .width = WIDTH,
            .height = HEIGHT,
            .parallelism = 4
        }},
        { M7_Components.World, nullptr },
        { M7_Components.XformComposer, &(M7_XformComposer){M7_XformComposeDefault} }
    );

    ECS_Entity_AddChildren(root, 
        { /* Camera */
            ECS_Components(
                { M7_Components.Rasterizer, &(M7_RasterizerArgs) {
                    .project = SD_SELECT(M7_ProjectPerspective),
                    .scan = SD_SELECT(M7_ScanPerspective),
                    .near = 5,
                    .parallelism = 4
                }},
                { Components.FreeCam, &(FreeCam){} },
                { M7_Components.Position, &(vec3){} },
                { M7_Components.Basis, (mat3x3 []){mat3x3_identity} },
            )
        },
        { /* Teapot */
            ECS_Components(
                { M7_Components.Position, &(vec3){ .y=-150, .z=600 } },
                { M7_Components.Basis, (mat3x3 []){mat3x3_identity} },
                { M7_Components.MeshPrimitive, nullptr },
                { M7_Components.Teapot, &(M7_Teapot) { .scale = 50 } },
                { M7_Components.Model, &(M7_ModelArgs) {
                    .get_mesh = M7_Teapot_GetMesh,
                    .instances = (M7_ModelInstance []) {
                        (M7_ModelInstance) {
                            .shader_pipeline = (M7_FragmentShader []) { SD_SELECT(solid_green), SD_SELECT(light) },
                            .nshaders = 2,
                            .render_batch = 0,
                            .flags = M7_RASTERIZER_CULL_BACKFACE
                                   | M7_RASTERIZER_WRITE_DEPTH
                                   | M7_RASTERIZER_TEST_DEPTH
                                   | M7_RASTERIZER_INTERPOLATE_NORMALS
                        },
                    },
                    .ninstances = 1
                }},
                { M7_Components.XformComposer, &(M7_XformComposer){M7_XformComposeDefault} }
            )
        },
        { /* Torus */
            ECS_Components(
                { M7_Components.Position, &(vec3){ .x=400, .y=-100, .z=200 } },
                { M7_Components.Basis, (mat3x3 []){mat3x3_rotate(mat3x3_identity, vec3_i, SDL_PI_F / 2)} },
                { M7_Components.MeshPrimitive, nullptr },
                { M7_Components.Torus, &(M7_Torus) { .outer_radius=100, .inner_radius=50, .outer_precision=32, .inner_precision=16 } },
                { M7_Components.Model, &(M7_ModelArgs) {
                    .get_mesh = M7_Torus_GetMesh,
                    .instances = (M7_ModelInstance []) {
                        (M7_ModelInstance) {
                            .shader_pipeline = (M7_FragmentShader []) { SD_SELECT(solid_green), SD_SELECT(light) },
                            .nshaders = 2,
                            .render_batch = 0,
                            .flags = M7_RASTERIZER_CULL_BACKFACE
                                   | M7_RASTERIZER_WRITE_DEPTH
                                   | M7_RASTERIZER_TEST_DEPTH
                                   | M7_RASTERIZER_INTERPOLATE_NORMALS
                        },
                    },
                    .ninstances = 1
                }},
                { M7_Components.XformComposer, &(M7_XformComposer){M7_XformComposeDefault} }
            )
        },
        { /* Floor */
            ECS_Components(
                { M7_Components.Position, &(vec3){ .y=-150, .z=600 } },
                { M7_Components.Basis, (mat3x3 []){mat3x3_rotate(mat3x3_identity, vec3_i, SDL_PI_F / 2)} },
                { M7_Components.MeshPrimitive, nullptr },
                { M7_Components.Rect, &(M7_Rect) { .width=5000, .height=5000 } },
                { M7_Components.Model, &(M7_ModelArgs) {
                    .get_mesh = M7_Rect_GetMesh,
                    .instances = (M7_ModelInstance []) {
                        (M7_ModelInstance) {
                            .shader_pipeline = (M7_FragmentShader []) { SD_SELECT(checkerboard), SD_SELECT(light) },
                            .nshaders = 2,
                            .render_batch = 0,
                            .flags = M7_RASTERIZER_CULL_BACKFACE
                                   | M7_RASTERIZER_WRITE_DEPTH
                                   | M7_RASTERIZER_TEST_DEPTH
                        },
                    },
                    .ninstances = 1
                }},
                { M7_Components.XformComposer, &(M7_XformComposer){M7_XformComposeDefault} }
            )
        }
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
