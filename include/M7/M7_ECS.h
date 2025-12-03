#ifndef M7_ECS_H
#define M7_ECS_H

#include <M7/ECS.h>
#include "Components/M7_InputState.h"
#include "Components/M7_Viewport.h"
#include "Components/M7_3D.h"
#include "Components/CameraMovement.h"

typedef void (*M7_Entity_OnSDLEvent)(ECS_Handle *, SDL_Event *);
typedef void (*M7_Entity_Update)(ECS_Handle *, double);
typedef void (*M7_Entity_PostUpdate)(ECS_Handle *);
typedef void (*M7_Entity_Render)(ECS_Handle *);
typedef void (*M7_Entity_RenderPresent)(ECS_Handle *);
typedef void (*M7_Entity_OnXform)(ECS_Handle *, xform3);

struct M7_Components {
    ECS_Component(M7_Viewport) *Viewport;
    ECS_Component(M7_InputState) *InputState;

    ECS_Component(CameraMovement) *CameraMovement;

    /* 3D */
    ECS_Component(M7_Canvas) *Canvas;
    ECS_Component(M7_World) *World;
    ECS_Component(M7_Rasterizer) *Rasterizer;
    ECS_Component(M7_Model) *Model;
    ECS_Component(M7_XformComposer) *XformComposer;
    ECS_Component(vec3) *Position;
    ECS_Component(mat3x3) *Basis;

    /* 3D primitives */
    ECS_Component(M7_Teapot) *Teapot;
    ECS_Component(M7_Torus) *Torus;
    ECS_Component(M7_Sphere) *Sphere;
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
