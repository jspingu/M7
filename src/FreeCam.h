#ifndef FREECAM_H
#define FREECAM_H

#include <M7/ECS.h>
#include <M7/Math/linalg.h>

typedef struct FreeCam {
    float yaw, pitch;
} FreeCam;

void FreeCam_Update(ECS_Handle *self, double delta);

#endif /* FREECAM_H */
