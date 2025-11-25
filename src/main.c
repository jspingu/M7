#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

static Uint64 count;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    (void)argc;
    (void)argv;

    ECS *ecs = ECS_Create();
    M7_RegisterToECS(ecs);

    ECS_Handle *root = ECS_GetRoot(ecs);

    ECS_Entity_AttachComponents(root,
        { M7_Components.Viewport, &(M7_ViewportArgs){
            .title = "Good morning!",
            .width = 960,
            .height = 540
        }},
        { M7_Components.InputState, nullptr },
        { M7_Components.Canvas, &(M7_Canvas){
            .width = 960,
            .height = 540,
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
                { M7_Components.CameraMovement, &(CameraMovement){} },
                { M7_Components.Position, &(vec3){} },
                { M7_Components.Basis, (mat3x3 []){mat3x3_identity} },
            )
        },
        { /* Model */
            ECS_Components(
                { M7_Components.Position, &(vec3){ .z=250 } },
                { M7_Components.Basis, (mat3x3 []){mat3x3_identity} },
                { M7_Components.Model, nullptr },
                { M7_Components.XformComposer, &(M7_XformComposer){M7_XformComposeDefault} }
            )
        }
    );

    ECS_Update(ecs);

    count = SDL_GetPerformanceCounter();
    *appstate = ecs;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    ECS *ecs = appstate;

    Uint64 new_count = SDL_GetPerformanceCounter();
    double delta = (double)(new_count - count) / SDL_GetPerformanceFrequency();
    count = new_count;

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
