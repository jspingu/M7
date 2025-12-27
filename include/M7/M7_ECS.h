#ifndef M7_ECS_H
#define M7_ECS_H

#include <M7/ECS.h>
#include "M7_InputState.h"
#include "M7_3D.h"
#include "M7_Bitmap.h"
#include "M7_Resource.h"

typedef void (*M7_Entity_OnSDLEvent)(ECS_Handle *, SDL_Event *);
typedef void (*M7_Entity_Update)(ECS_Handle *, double);
typedef void (*M7_Entity_PostUpdate)(ECS_Handle *);
typedef void (*M7_Entity_Render)(ECS_Handle *);
typedef void (*M7_Entity_RenderPresent)(ECS_Handle *);
typedef void (*M7_Entity_OnXform)(ECS_Handle *, xform3);

struct M7_Components {
    ECS_Component(M7_InputState) *InputState;

    /* 3D */
    ECS_Component(M7_World) *World;
    ECS_Component(M7_Rasterizer) *Rasterizer;
    ECS_Component(M7_Model) *Model;
    ECS_Component(M7_ModelInstance) *ModelInstance;
    ECS_Component(M7_XformComposer) *XformComposer;
    ECS_Component(vec3) *Position;
    ECS_Component(mat3x3) *Basis;
    ECS_Component(M7_ParallelProjector) *ParallelProjector;
    ECS_Component(M7_PerspectiveFOV) *PerspectiveFOV;

    /* Shader components */
    ECS_Component(M7_ShaderComponent) *SolidColor;
    ECS_Component(M7_ShaderComponent) *Checkerboard;
    ECS_Component(M7_ShaderComponent) *TextureMap;
    ECS_Component(M7_ShaderComponent) *Lighting;

    /* 3D primitives */
    ECS_Component(M7_Mesh *) *MeshPrimitive;
    ECS_Component(M7_Teapot) *Teapot;
    ECS_Component(M7_Torus) *Torus;
    ECS_Component(M7_Sphere) *Sphere;
    ECS_Component(M7_Rect) *Rect;
    ECS_Component(M7_Cubemap) *Cubemap;

    /* Bitmap */
    ECS_Component(M7_Viewport) *Viewport;
    ECS_Component(M7_Canvas) *Canvas;
    ECS_Component(M7_ResourceBank(M7_Texture *)) *TextureBank;
};

struct M7_SystemGroups {
    ECS_SystemGroup(M7_Entity_OnSDLEvent) *OnSDLEvent;
    ECS_SystemGroup(M7_Entity_Update) *Update;
    ECS_SystemGroup(M7_Entity_PostUpdate) *PostUpdate;
    ECS_SystemGroup(M7_Entity_Render) *Render;
    ECS_SystemGroup(M7_Entity_RenderPresent) *RenderPresent;
    ECS_SystemGroup(M7_Entity_OnXform) *OnXform;
};

extern struct M7_Components M7_Components;
extern struct M7_SystemGroups M7_SystemGroups;

void M7_RegisterToECS(ECS *ecs);

#endif /* M7_ECS_H */
