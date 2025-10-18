#ifndef M7_INPUTSTATE_H
#define M7_INPUTSTATE_H

#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/Math/linalg.h>

typedef struct M7_InputState {
    bool (*prev)[SDL_SCANCODE_COUNT];
    bool (*curr)[SDL_SCANCODE_COUNT];
    vec2 mouse_motion;
} M7_InputState;

bool M7_InputState_KeyDown(ECS_Handle *self, SDL_Scancode sc);
bool M7_InputState_KeyJustDown(ECS_Handle *self, SDL_Scancode sc);
bool M7_InputState_KeyJustUp(ECS_Handle *self, SDL_Scancode sc);

vec2 M7_InputState_GetMouseMotion(ECS_Handle *self);

#endif /* M7_INPUTSTATE_H */
