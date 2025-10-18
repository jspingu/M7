#ifndef CAMERAMOVEMENT_H
#define CAMERAMOVEMENT_H

#include <M7/ECS.h>
#include <M7/Math/linalg.h>

typedef struct CameraMovement {
    float yaw, pitch;
} CameraMovement;

void CameraMovement_Update(ECS_Handle *self, double delta);

#endif /* CAMERAMOVEMENT_H */
