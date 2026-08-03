#ifndef TX_BITMAP_C_H
#define TX_BITMAP_C_H

#include <TX/TX_Bitmap.h>
#include <TX/Collections/Strmap.h>

void TX_Viewport_Init(void *component, void *args);
void TX_Viewport_Free(void *component);

SD_DECLARE_VOID_RETURN(TX_Canvas_Present, ECS_Handle *, self)
SD_DECLARE_VOID_RETURN(TX_Canvas_Init, void *, component, void *, args)
void TX_Canvas_Free(void *component);

void TX_TextureBank_Attach(ECS_Handle *self, ECS_Component(void) *component);

void TX_Bitmap_RegisterToECS(ECS *ecs);

#endif /* TX_BITMAP_C_H */
