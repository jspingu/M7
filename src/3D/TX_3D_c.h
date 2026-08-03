#ifndef TX_3D_C_H
#define TX_3D_C_H

#include <TX/TX_3D.h>
#include <TX/Collections/List.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

typedef struct TX_Mesh {
    sd_vec3 *ws_verts;
    sd_vec3 *ws_nrmls;
    vec2 *ts_verts;
    TX_MeshFace *faces;
    size_t nverts, nfaces;
} TX_Mesh;

typedef struct TX_PolyChain {
    size_t *indices;
    size_t nindices;
} TX_PolyChain;

typedef struct TX_Sculpture {
    List(vec3) *verts;
    List(TX_MeshFace) *faces;
    List(TX_PolyChain *) *chains;
} TX_Sculpture;

typedef struct TX_WorldGeometry {
    TX_World *world;
    List(TX_RenderInstance *) *instances;
    TX_Mesh *mesh;
    sd_vec3 *vs_verts;
    sd_vec3 *vs_nrmls;
    sd_vec2 *ss_verts;
    xform3 xform;
} TX_WorldGeometry;

typedef struct TX_RenderInstance {
    TX_WorldGeometry *geometry;
    TX_FragmentShader *shader_pipeline;
    void **shader_states;
    size_t nshaders;
    size_t render_batch;
    TX_RasterizerFlags flags;
} TX_RenderInstance;

typedef struct TX_World {
    List(TX_WorldGeometry *) *geometry;
    /* List of arrays of Lists of RenderInstance */
    List(List(TX_RenderInstance *) *[TX_RASTERIZER_FLAG_COMBINATIONS]) *render_batches;
} TX_World;

typedef struct TX_Model {
    TX_WorldGeometry *geometry;
    TX_Mesh *(*get_mesh)(ECS_Handle *self);
} TX_Model;

typedef struct TX_ModelInstance {
    TX_RenderInstance *instance;
    ECS_Component(TX_ShaderComponent) **shader_components;
    size_t nshaders;
    size_t render_batch;
    TX_RasterizerFlags flags;
} TX_ModelInstance;

typedef struct TX_Rasterizer {
    ECS_Handle *world;
    ECS_Handle *target;
    TX_VertexProjector project;
    TX_RasterScanner scan;
    float near;
    int parallelism;
} TX_Rasterizer;

void TX_3D_RegisterToECS(ECS *ecs);

void TX_LightEnvironment_Attach(ECS_Handle *self, ECS_Component(void) *component);
void TX_LightEnvironment_Detach(ECS_Handle *self, ECS_Component(void) *component);
void TX_LightEnvironment_Init(void *component, void *args);
void TX_LightEnvironment_Free(void *component);

void TX_PointLight_OnXform(ECS_Handle *self, xform3 composed);
void TX_PointLight_Attach(ECS_Handle *self, ECS_Component(void) *component);
void TX_PointLight_Detach(ECS_Handle *self, ECS_Component(void) *component);

void TX_Lighting_Attach(ECS_Handle *self, ECS_Component(void) *component);
void TX_Lighting_Init(void *component, void *args);

void TX_Sky_Attach(ECS_Handle *self, ECS_Component(void) *component);
void TX_Sky_Init(void *component, void *args);

void TX_SolidColor_Init(void *component, void *args);
void TX_Checkerboard_Init(void *component, void *args);

void TX_ShaderComponent_Free(void *component);

void TX_TextureMap_Attach(ECS_Handle *self, ECS_Component(void) *component);
void TX_TextureMap_Detach(ECS_Handle *self, ECS_Component(void) *component);
void TX_TextureMap_Init(void *component, void *args);
void TX_TextureMap_Free(void *component);

void TX_MeshPrimitive_Init(void *component, void *args);
void TX_MeshPrimitive_Free(void *component);

void TX_Model_Update(ECS_Handle *self, double delta);
void TX_Model_OnXform(ECS_Handle *self, xform3 composed);
void TX_Model_Attach(ECS_Handle *self, ECS_Component(void) *component);
void TX_Model_Detach(ECS_Handle *self, ECS_Component(void) *component);
void TX_Model_Init(void *component, void *args);

void TX_ModelInstance_Attach(ECS_Handle *self, ECS_Component(void) *component);
void TX_ModelInstance_Detach(ECS_Handle *self, ECS_Component(void) *component);
void TX_ModelInstance_Init(void *component, void *args);
void TX_ModelInstance_Free(void *component);

void TX_World_Init(void *component, void *args);
void TX_World_Free(void *component);

SD_DECLARE_VOID_RETURN(TX_Rasterizer_Render, ECS_Handle *, self)
void TX_Rasterizer_Attach(ECS_Handle *self, ECS_Component(void) *component);
void TX_Rasterizer_Init(void *component, void *args);

void TX_PerspectiveFOV_Init(void *component, void *args);

#endif /* TX_3D_C_H */
