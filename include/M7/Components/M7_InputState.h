#ifndef M7_INPUTSTATE_H
#define M7_INPUTSTATE_H

#include <SDL3/SDL.h>
#include <M7/ECS.h>

typedef struct M7_InputState {
    bool (*prev)[SDL_SCANCODE_COUNT];
    bool (*curr)[SDL_SCANCODE_COUNT];
} M7_InputState;

bool M7_InputState_KeyDown(ECS_Handle *e, SDL_Scancode sc);
bool M7_InputState_KeyJustDown(ECS_Handle *e, SDL_Scancode sc);
bool M7_InputState_KeyJustUp(ECS_Handle *e, SDL_Scancode sc);

#endif /* M7_INPUTSTATE_H */
