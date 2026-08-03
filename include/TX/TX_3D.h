#ifndef TX_3D_H
#define TX_3D_H

#include <TX/ECS.h>
#include <TX/TX_Bitmap.h>
#include <TX/Collections/List.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

#define TX_SHADER_DECLARE(name)  SD_DECLARE(sd_vec4, name, void *, state, TX_ShaderParams, fragment)

typedef enum TX_RasterizerFlags {
    TX_RASTERIZER_ALPHA_BLEND         = 1 << 0,
    TX_RASTERIZER_ALPHA_SCISSOR       = 1 << 1,
    TX_RASTERIZER_WRITE_DEPTH         = 1 << 2,
    TX_RASTERIZER_TEST_DEPTH          = 1 << 3,
    TX_RASTERIZER_INTERPOLATE_NORMALS = 1 << 4,
    TX_RASTERIZER_CULL_BACKFACE       = 1 << 5,
    TX_RASTERIZER_SORT_TRIANGLES      = 1 << 6,
    TX_RASTERIZER_FLAG_COMBINATIONS   = 1 << 7
} TX_RasterizerFlags;

typedef struct TX_Mesh TX_Mesh;
typedef struct TX_Sculpture TX_Sculpture;
typedef struct TX_PolyChain TX_PolyChain;
typedef struct TX_WorldGeometry TX_WorldGeometry;
typedef struct TX_RenderInstance TX_RenderInstance;
typedef struct TX_World TX_World;
typedef struct TX_Model TX_Model;
typedef struct TX_ModelInstance TX_ModelInstance;
typedef struct TX_Rasterizer TX_Rasterizer;
typedef struct TX_TriangleDraw TX_TriangleDraw;
typedef struct TX_ShaderParams TX_ShaderParams;

typedef xform3 (*TX_XformComposer)(ECS_Handle *self, xform3 lhs);

typedef sd_vec4 (*TX_FragmentShader)(void *state, TX_ShaderParams fragment);
typedef sd_vec2 (*TX_VertexProjector)(ECS_Handle *self, sd_vec3 pos, sd_vec2 midpoint);
typedef void (*TX_RasterScanner)(ECS_Handle *self, TX_TriangleDraw triangle, TX_RasterizerFlags flags, int (*scanlines)[2], int range[2]);

typedef struct TX_ShaderParams {
    sd_vec4 col;
    sd_vec3 vs, nrml;
    sd_vec2 ts;
    sd_vec3 vs2ws_xform[3];
} TX_ShaderParams;

typedef struct TX_ShaderComponent {
    TX_FragmentShader callback;
    void *state;
} TX_ShaderComponent;

typedef struct TX_MeshFace {
    size_t idx_verts[3];
    size_t idx_tverts[3];
} TX_MeshFace;

typedef struct TX_TriangleDraw {
    TX_FragmentShader *shader_pipeline;
    void **shader_states;
    size_t nshaders;
    vec3 vs_verts[3];
    vec3 vs_nrmls[3];
    vec2 ts_verts[3];
    vec2 ss_verts[3];
} TX_TriangleDraw;

typedef struct TX_RasterizerArgs {
    TX_VertexProjector project;
    TX_RasterScanner scan;
    float near;
    int parallelism;
} TX_RasterizerArgs;

typedef struct TX_ParallelProjector {
    vec2 slope;
    vec2 scale;
} TX_ParallelProjector;

typedef struct TX_PerspectiveFOV {
    float fov;
    float tan_half_fov;
} TX_PerspectiveFOV;

typedef struct TX_ModelArgs {
    TX_Mesh *(*get_mesh)(ECS_Handle *self);
} TX_ModelArgs;

typedef struct TX_ModelInstanceArgs {
    ECS_Component(TX_ShaderComponent) **shader_components;
    size_t nshaders;
    size_t render_batch;
    TX_RasterizerFlags flags;
} TX_ModelInstanceArgs;

typedef struct TX_Teapot {
    float scale;
} TX_Teapot;

typedef struct TX_Torus {
    size_t outer_precision, inner_precision;
    float outer_radius, inner_radius;
} TX_Torus;

typedef struct TX_Sphere {
    size_t nrings, ring_precision;
    float radius;
} TX_Sphere;

typedef struct TX_Rect {
    float width, height;
} TX_Rect;

typedef struct TX_Cubemap {
    float scale;
} TX_Cubemap;

typedef struct TX_SolidColor {
    float r, g, b;
} TX_SolidColor;

typedef struct TX_Checkerboard {
    int tiles;
    float r1, g1, b1;
    float r2, g2, b2;
} TX_Checkerboard;

typedef struct TX_TextureMap {
    TX_Texture *texture;
    char *texture_path;
    float scale;
} TX_TextureMap;

typedef struct TX_ActiveLight {
    float energy;
    vec3 col;
    vec3 pos;
} TX_ActiveLight;

typedef struct TX_LightEnvironment {
    char *sky_texture_path;
    TX_Texture *sky;
    List(TX_ActiveLight *) *lights;
    float ambient;
} TX_LightEnvironment;

typedef struct TX_PointLight {
    TX_LightEnvironment *environment;
    TX_ActiveLight *active;
    vec3 col;
    float energy;
} TX_PointLight;

typedef struct TX_OpticalMedium {
    TX_LightEnvironment *environment;
    float reflectivity;
    float specularity;
    int exp;
} TX_OpticalMedium;

TX_SHADER_DECLARE(TX_ShadeSolidColor)
TX_SHADER_DECLARE(TX_ShadeCheckerboard)
TX_SHADER_DECLARE(TX_ShadeTextureMap)
TX_SHADER_DECLARE(TX_ShadeLighting)
TX_SHADER_DECLARE(TX_ShadeSky)

TX_Mesh *TX_Teapot_GetMesh(ECS_Handle *self);
TX_Mesh *TX_Torus_GetMesh(ECS_Handle *self);
TX_Mesh *TX_Sphere_GetMesh(ECS_Handle *self);
TX_Mesh *TX_Rect_GetMesh(ECS_Handle *self);
TX_Mesh *TX_Cubemap_GetMesh(ECS_Handle *self);

xform3 TX_Entity_GetXform(ECS_Handle *self);
void TX_Entity_Xform(ECS_Handle *self, xform3 lhs);

xform3 TX_XformComposeDefault(ECS_Handle *self, xform3 lhs);
xform3 TX_XformComposeBillboard(ECS_Handle *self, xform3 lhs);
xform3 TX_XformComposeCubemap(ECS_Handle *self, xform3 lhs);
xform3 TX_XformComposeAbsolute(ECS_Handle *self, xform3 lhs);

SD_DECLARE(TX_Mesh *, TX_Mesh_Create, vec3 *, ws_verts, vec3 *, ws_nrmls, vec2 *, ts_verts, TX_MeshFace *, faces, size_t, nverts, size_t, nts_verts, size_t, nfaces)
void TX_Mesh_Free(TX_Mesh *mesh);

void TX_Sculpture_JoinPolyChains(TX_Sculpture *sculpture, TX_PolyChain *pc1, TX_PolyChain *pc2);
TX_PolyChain *TX_Sculpture_Vertex(TX_Sculpture *sculpture, vec3 pos);
TX_PolyChain *TX_Sculpture_Ellipse(TX_Sculpture *sculpture, vec3 center, vec3 axis1, vec3 axis2, size_t precision);
TX_Mesh *TX_Sculpture_ToMesh(TX_Sculpture *sculpture);
TX_Sculpture *TX_Sculpture_Create(void);
void TX_Sculpture_Free(TX_Sculpture *sculpture);

SD_DECLARE(TX_WorldGeometry *, TX_World_RegisterGeometry, ECS_Handle *, self, TX_Mesh *, mesh)

void TX_RenderInstance_Free(TX_RenderInstance *instance);

TX_RenderInstance *TX_WorldGeometry_Instance(TX_WorldGeometry *geometry, TX_FragmentShader *shader_pipeline, void **shader_states, size_t nshaders, size_t render_batch, TX_RasterizerFlags flags);
void TX_WorldGeometry_Free(TX_WorldGeometry *geometry);

SD_DECLARE(sd_vec2, TX_ProjectParallel, ECS_Handle *, self, sd_vec3, point, sd_vec2, midpoint)
SD_DECLARE_VOID_RETURN(TX_ScanLinear, ECS_Handle *, self, TX_TriangleDraw, triangle, TX_RasterizerFlags, flags, int (*)[2], scanlines, int [2], range)

SD_DECLARE(sd_vec2, TX_ProjectPerspective, ECS_Handle *, self, sd_vec3, point, sd_vec2, midpoint)
SD_DECLARE_VOID_RETURN(TX_ScanPerspective, ECS_Handle *, self, TX_TriangleDraw, triangle, TX_RasterizerFlags, flags, int (*)[2], scanlines, int [2], range)

void TX_PerspectiveFOV_Set(ECS_Handle *self, float fov);

#endif /* TX_3D_H */
