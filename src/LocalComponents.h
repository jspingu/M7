#ifndef LOCALCOMPONENTS_H
#define LOCALCOMPONENTS_H

#include <M7/ECS.h>
#include "FreeCam.h"

struct LocalComponents {
    ECS_Component(FreeCam) *FreeCam;
};

extern struct LocalComponents Components;

void RegisterToECS(ECS *ecs);

#endif /* LOCALCOMPONENTS_H */
