#ifndef M7_VIEWPORT_C_H
#define M7_VIEWPORT_C_H

#include <M7/ECS.h>

// void M7_ViewportUpdate(ECS_Handle *self, double delta);
// void M7_ViewportRenderPresent(ECS_Handle *self);

void M7_Viewport_Init(void *component, void *args);
void M7_Viewport_Free(void *component);

#endif /* M7_VIEWPORT_C_H */
