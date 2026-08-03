#ifndef TX_INPUTSTATE_C_H
#define TX_INPUTSTATE_C_H

#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/Math/linalg.h>

typedef struct TX_InputState {
    bool (*prev)[SDL_SCANCODE_COUNT];
    bool (*curr)[SDL_SCANCODE_COUNT];
    vec2 mouse_motion;
    vec2 wheel_motion;
} TX_InputState;

void TX_InputState_OnSDLEvent(ECS_Handle *e, SDL_Event *ev);
void TX_InputState_Step(ECS_Handle *e);

void TX_InputState_Init(void *component, void *args);
void TX_InputState_Free(void *component);

#endif /* TX_INPUTSTATE_C_H */
