#ifndef M7_BITMAP_C_H
#define M7_BITMAP_C_H

#include <M7/ECS.h>
#include <M7/Math/stride.h>

void M7_Viewport_Init(void *component, void *args);
void M7_Viewport_Free(void *component);

SD_DECLARE_VOID_RETURN(M7_Canvas_Present, ECS_Handle *, self)
SD_DECLARE_VOID_RETURN(M7_Canvas_Attach, ECS_Handle *, self)
SD_DECLARE_VOID_RETURN(M7_Canvas_Init, void *, component, void *, args)
void M7_Canvas_Free(void *component);

void M7_Bitmap_RegisterToECS(ECS *ecs);

#endif /* M7_BITMAP_C_H */

