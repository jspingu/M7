#ifndef M7_3D_C_H
#define M7_3D_C_H

#include <M7/M7_3D.h>
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

typedef struct M7_PolyChain {
    size_t *indices;
    size_t nindices;
} M7_PolyChain;

typedef struct M7_Sculpture {
    List(vec3) *verts;
    List(M7_MeshFace) *faces;
    List(M7_PolyChain *) *chains;
} M7_Sculpture;

typedef struct M7_WorldGeometry {
    M7_World *world;
    List(M7_RenderInstance *) *instances;
    M7_Mesh *mesh;
    sd_vec3 *vs_verts;
    sd_vec3 *vs_nrmls;
    sd_vec2 *ss_verts;
    xform3 xform;
} M7_WorldGeometry;

typedef struct M7_RenderInstance {
    M7_WorldGeometry *geometry;
    List(M7_FragmentShader) *shader_pipeline;
    ECS_Handle *shader_state;
    size_t render_batch;
    M7_RasterizerFlags flags;
} M7_RenderInstance;

typedef struct M7_World {
    List(M7_WorldGeometry *) *geometry;
    /* List of arrays of Lists of RenderInstance */
    List(List(M7_RenderInstance *) *[M7_RASTERIZER_FLAG_COMBINATIONS]) *render_batches;
} M7_World;

typedef struct M7_Model {
    M7_Mesh *(*get_mesh)(ECS_Handle *self);
    M7_WorldGeometry *geometry;
    M7_RenderInstance *instance;
    List(M7_RenderInstance *) *instances;
    List(M7_ModelInstance) *instances_init;
} M7_Model;

typedef struct M7_Rasterizer {
    ECS_Handle *world;
    ECS_Handle *target;
    M7_VertexProjector project;
    M7_RasterScanner scan;
    float near;
    int parallelism;
} M7_Rasterizer;

void M7_3D_RegisterToECS(ECS *ecs);

void M7_Model_Update(ECS_Handle *self, double delta);
void M7_Model_OnXform(ECS_Handle *self, xform3 composed);
void M7_Model_Attach(ECS_Handle *self);
void M7_Model_Detach(ECS_Handle *self);
void M7_Model_Init(void *component, void *args);
void M7_Model_Free(void *component);

void M7_World_Init(void *component, void *args);
void M7_World_Free(void *component);

SD_DECLARE_VOID_RETURN(M7_Rasterizer_Render, ECS_Handle *, self)
void M7_Rasterizer_Attach(ECS_Handle *self);
void M7_Rasterizer_Init(void *component, void *args);

#endif /* M7_3D_C_H */
