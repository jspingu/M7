#ifndef TX_ECS_H
#define TX_ECS_H

#include <TX/ECS.h>
#include "TX_InputState.h"
#include "TX_3D.h"
#include "TX_Bitmap.h"
#include "TX_Resource.h"

typedef void (*TX_Entity_OnSDLEvent)(ECS_Handle *, SDL_Event *);
typedef void (*TX_Entity_Update)(ECS_Handle *, double);
typedef void (*TX_Entity_PostUpdate)(ECS_Handle *);
typedef void (*TX_Entity_Render)(ECS_Handle *);
typedef void (*TX_Entity_RenderPresent)(ECS_Handle *);
typedef void (*TX_Entity_OnXform)(ECS_Handle *, xform3);

struct TX_Components {
    ECS_Component(TX_InputState) *InputState;

    /* 3D */
    ECS_Component(TX_World) *World;
    ECS_Component(TX_Rasterizer) *Rasterizer;
    ECS_Component(TX_Model) *Model;
    ECS_Component(TX_ModelInstance) *ModelInstance;
    ECS_Component(TX_XformComposer) *XformComposer;
    ECS_Component(vec3) *Position;
    ECS_Component(mat3x3) *Basis;
    ECS_Component(TX_ParallelProjector) *ParallelProjector;
    ECS_Component(TX_PerspectiveFOV) *PerspectiveFOV;

    ECS_Component(TX_LightEnvironment *) *LightEnvironment;
    ECS_Component(TX_PointLight) *PointLight;

    /* Shader components */
    ECS_Component(TX_ShaderComponent) *SolidColor;
    ECS_Component(TX_ShaderComponent) *Checkerboard;
    ECS_Component(TX_ShaderComponent) *TextureMap;
    ECS_Component(TX_ShaderComponent) *Lighting;
    ECS_Component(TX_ShaderComponent) *Sky;

    /* 3D primitives */
    ECS_Component(TX_Mesh *) *MeshPrimitive;
    ECS_Component(TX_Teapot) *Teapot;
    ECS_Component(TX_Torus) *Torus;
    ECS_Component(TX_Sphere) *Sphere;
    ECS_Component(TX_Rect) *Rect;
    ECS_Component(TX_Cubemap) *Cubemap;

    /* Bitmap */
    ECS_Component(TX_Viewport) *Viewport;
    ECS_Component(TX_Canvas) *Canvas;
    ECS_Component(TX_ResourceBank(TX_Texture *)) *TextureBank;
};

struct TX_SystemGroups {
    ECS_SystemGroup(TX_Entity_OnSDLEvent) *OnSDLEvent;
    ECS_SystemGroup(TX_Entity_Update) *Update;
    ECS_SystemGroup(TX_Entity_PostUpdate) *PostUpdate;
    ECS_SystemGroup(TX_Entity_Render) *Render;
    ECS_SystemGroup(TX_Entity_RenderPresent) *RenderPresent;
    ECS_SystemGroup(TX_Entity_OnXform) *OnXform;
};

extern struct TX_Components TX_Components;
extern struct TX_SystemGroups TX_SystemGroups;

void TX_RegisterToECS(ECS *ecs);

#endif /* TX_ECS_H */
