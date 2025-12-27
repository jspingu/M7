#ifndef M7_BITMAP_C_H
#define M7_BITMAP_C_H

#include <M7/M7_Bitmap.h>
#include <M7/Collections/Strmap.h>

void M7_Viewport_Init(void *component, void *args);
void M7_Viewport_Free(void *component);

SD_DECLARE_VOID_RETURN(M7_Canvas_Present, ECS_Handle *, self)
SD_DECLARE_VOID_RETURN(M7_Canvas_Attach, ECS_Handle *, self, ECS_Component(void) *, component)
SD_DECLARE_VOID_RETURN(M7_Canvas_Init, void *, component, void *, args)
void M7_Canvas_Free(void *component);

void M7_TextureBank_Attach(ECS_Handle *self, ECS_Component(void) *component);

void M7_Bitmap_RegisterToECS(ECS *ecs);

#endif /* M7_BITMAP_C_H */
