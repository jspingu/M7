#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include "M7_InputState_c.h"

bool M7_InputState_KeyDown(ECS_Handle *e, SDL_Scancode sc) {
    M7_InputState *is = ECS_Entity_GetComponent(e, M7_Components.InputState);
    return (*is->curr)[sc];
}

bool M7_InputState_KeyJustDown(ECS_Handle *e, SDL_Scancode sc) {
    M7_InputState *is = ECS_Entity_GetComponent(e, M7_Components.InputState);
    return !(*is->prev)[sc] && (*is->curr)[sc];
}

bool M7_InputState_KeyJustUp(ECS_Handle *e, SDL_Scancode sc) {
    M7_InputState *is = ECS_Entity_GetComponent(e, M7_Components.InputState);
    return (*is->prev)[sc] && !(*is->curr)[sc];
}

void M7_InputState_OnSDLEvent(ECS_Handle *self, SDL_Event *ev) {
    M7_InputState *is = ECS_Entity_GetComponent(self, M7_Components.InputState);

    switch (ev->type) {
        case SDL_EVENT_KEY_DOWN:
            (*is->curr)[ev->key.scancode] = true;
            break;

        case SDL_EVENT_KEY_UP:
            (*is->curr)[ev->key.scancode] = false;
            break;
    }
}

void M7_InputState_Step(ECS_Handle *self) {
    M7_InputState *is = ECS_Entity_GetComponent(self, M7_Components.InputState);
    SDL_memcpy(*is->prev, *is->curr, sizeof(*is->curr));
}

void M7_InputState_Init(void *component, void *args) {
    (void)args;

    M7_InputState *is = component;
    is->prev = SDL_calloc(SDL_SCANCODE_COUNT, sizeof(bool));
    is->curr = SDL_calloc(SDL_SCANCODE_COUNT, sizeof(bool));
}

void M7_InputState_Free(void *component) {
    M7_InputState *is = component;
    SDL_free(is->prev);
    SDL_free(is->curr);
}
