#ifndef LOCALCOMPONENTS_H
#define LOCALCOMPONENTS_H

#include <M7/ECS.h>

typedef struct FreeCam {
    float yaw, pitch;
} FreeCam;

struct LocalComponents {
    ECS_Component(FreeCam) *FreeCam;
    ECS_Component(bool) *GrabMouse;
};

extern struct LocalComponents Components;

void GrabMouse_Update(ECS_Handle *self, double delta);
void FreeCam_Update(ECS_Handle *self, double delta);
void RegisterToECS(ECS *ecs);

#endif /* LOCALCOMPONENTS_H */
