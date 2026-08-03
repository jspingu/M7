#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Math/linalg.h>

#include "TX_InputState_c.h"

bool TX_InputState_KeyDown(ECS_Handle *self, SDL_Scancode sc) {
    TX_InputState *is = ECS_Entity_GetComponent(self, TX_Components.InputState);
    return (*is->curr)[sc];
}

bool TX_InputState_KeyJustDown(ECS_Handle *self, SDL_Scancode sc) {
    TX_InputState *is = ECS_Entity_GetComponent(self, TX_Components.InputState);
    return !(*is->prev)[sc] && (*is->curr)[sc];
}

bool TX_InputState_KeyJustUp(ECS_Handle *self, SDL_Scancode sc) {
    TX_InputState *is = ECS_Entity_GetComponent(self, TX_Components.InputState);
    return (*is->prev)[sc] && !(*is->curr)[sc];
}

vec2 TX_InputState_GetMouseMotion(ECS_Handle *self) {
    TX_InputState *is = ECS_Entity_GetComponent(self, TX_Components.InputState);
    return is->mouse_motion;
}

vec2 TX_InputState_GetWheelMotion(ECS_Handle *self) {
    TX_InputState *is = ECS_Entity_GetComponent(self, TX_Components.InputState);
    return is->wheel_motion;
}

void TX_InputState_OnSDLEvent(ECS_Handle *self, SDL_Event *ev) {
    TX_InputState *is = ECS_Entity_GetComponent(self, TX_Components.InputState);

    switch (ev->type) {
        case SDL_EVENT_KEY_DOWN:
            (*is->curr)[ev->key.scancode] = true;
            break;

        case SDL_EVENT_KEY_UP:
            (*is->curr)[ev->key.scancode] = false;
            break;

        case SDL_EVENT_MOUSE_MOTION:
            is->mouse_motion = (vec2) {
                .x = ev->motion.xrel,
                .y = ev->motion.yrel
            };
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            is->wheel_motion = (vec2) {
                .x = ev->wheel.x,
                .y = ev->wheel.y
            };
            break;
    }
}

void TX_InputState_Step(ECS_Handle *self) {
    TX_InputState *is = ECS_Entity_GetComponent(self, TX_Components.InputState);
    SDL_memcpy(*is->prev, *is->curr, sizeof(*is->curr));
    is->mouse_motion = vec2_zero;
    is->wheel_motion = vec2_zero;
}

void TX_InputState_Init(void *component, void *args) {
    (void)args;

    TX_InputState *is = component;
    is->prev = SDL_calloc(SDL_SCANCODE_COUNT, sizeof(bool));
    is->curr = SDL_calloc(SDL_SCANCODE_COUNT, sizeof(bool));
    is->mouse_motion = vec2_zero;
}

void TX_InputState_Free(void *component) {
    TX_InputState *is = component;
    SDL_free(is->prev);
    SDL_free(is->curr);
}
