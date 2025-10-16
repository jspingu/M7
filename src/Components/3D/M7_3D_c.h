#ifndef M7_3D_C_H
#define M7_3D_C_H

#include <M7/Components/M7_3D.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>
#include <M7/Math/stride.h>

typedef struct M7_Mesh {
    sd_vec3 *ws_verts;
    sd_vec3 *ws_nrmls;
    vec2 *ts_verts;
    M7_MeshFace *faces;
    size_t nverts, nfaces;
} M7_Mesh;

typedef struct M7_WorldGeometry {
    M7_World *world;
    List(M7_RenderInstance *) *instances;
    M7_Mesh *mesh;
    sd_vec3 *vs_verts;
    sd_vec3 *vs_nrmls;
    xform3 transform;
} M7_WorldGeometry;

typedef struct M7_RenderInstance {
    M7_WorldGeometry *geometry;
    M7_FragmentShader shader;
    size_t render_batch;
    M7_RasterizerFlags flags;
} M7_RenderInstance;

typedef struct M7_World {
    List(M7_WorldGeometry *) *geometry;
    // List of arrays of Lists of RenderInstance
    List(List(M7_RenderInstance *) *[M7_RASTERIZER_FLAG_COMBINATIONS]) *render_batches;
} M7_World;

typedef struct M7_Model {
    M7_Mesh *mesh;
    M7_WorldGeometry *geometry;
    M7_RenderInstance *instance;
} M7_Model;

typedef struct M7_Rasterizer {
    M7_World *world;
    M7_Canvas *target;
    M7_VertexProjector project;
    M7_RasterScanner scan;
    size_t (*scanlines)[2];
    sd_vec3 *ss_verts;
    size_t ssv_capacity;
} M7_Rasterizer;

void M7_3D_RegisterToECS(ECS *ecs);

void M7_Model_Attach(ECS_Handle *self);
void M7_Model_Detach(ECS_Handle *self);
void M7_Model_Init(void *component, void *args);
void M7_Model_Free(void *component);

void M7_World_Init(void *component, void *args);
void M7_World_Free(void *component);

SD_DECLARE_VOID_RETURN(M7_Canvas_Present, ECS_Handle *, self)
SD_DECLARE_VOID_RETURN(M7_Canvas_Attach, ECS_Handle *, self)
SD_DECLARE_VOID_RETURN(M7_Canvas_Init, void *, component, void *, args)
void M7_Canvas_Free(void *component);

#endif /* M7_3D_C_H */
