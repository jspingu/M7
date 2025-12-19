#ifndef M7_INPUTSTATE_C_H
#define M7_INPUTSTATE_C_H

#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/Math/linalg.h>

typedef struct M7_InputState {
    bool (*prev)[SDL_SCANCODE_COUNT];
    bool (*curr)[SDL_SCANCODE_COUNT];
    vec2 mouse_motion;
    vec2 wheel_motion;
} M7_InputState;

void M7_InputState_OnSDLEvent(ECS_Handle *e, SDL_Event *ev);
void M7_InputState_Step(ECS_Handle *e);

void M7_InputState_Init(void *component, void *args);
void M7_InputState_Free(void *component);

#endif /* M7_INPUTSTATE_C_H */
