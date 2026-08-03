#ifndef TX_INPUTSTATE_H
#define TX_INPUTSTATE_H

#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/Math/linalg.h>

typedef struct TX_InputState TX_InputState;

bool TX_InputState_KeyDown(ECS_Handle *self, SDL_Scancode sc);
bool TX_InputState_KeyJustDown(ECS_Handle *self, SDL_Scancode sc);
bool TX_InputState_KeyJustUp(ECS_Handle *self, SDL_Scancode sc);

vec2 TX_InputState_GetMouseMotion(ECS_Handle *self);
vec2 TX_InputState_GetWheelMotion(ECS_Handle *self);

#endif /* TX_INPUTSTATE_H */
