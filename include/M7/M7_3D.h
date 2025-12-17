#ifndef M7_3D_H
#define M7_3D_H

#include <M7/ECS.h>
#include <M7/M7_Bitmap.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>
#include <M7/Math/stride.h>

#define M7_SHADER_DECLARE(name)  SD_DECLARE(sd_vec4, name, ECS_Handle *, self, sd_vec4, col, sd_vec3, vs, sd_vec3, nrml, sd_vec2, ts)

typedef struct M7_Mesh M7_Mesh;
typedef struct M7_Sculpture M7_Sculpture;
typedef struct M7_PolyChain M7_PolyChain;
typedef struct M7_WorldGeometry M7_WorldGeometry;
typedef struct M7_RenderInstance M7_RenderInstance;
typedef struct M7_World M7_World;
typedef struct M7_Model M7_Model;
typedef struct M7_Rasterizer M7_Rasterizer;
typedef struct M7_TriangleDraw M7_TriangleDraw;

typedef xform3 (*M7_XformComposer)(ECS_Handle *self, xform3 lhs);

typedef sd_vec4 (*M7_FragmentShader)(ECS_Handle *self, sd_vec4 col, sd_vec3 vs, sd_vec3 nrml, sd_vec2 ts);
typedef sd_vec2 (*M7_VertexProjector)(ECS_Handle *self, sd_vec3 pos, sd_vec2 midpoint);
typedef void (*M7_RasterScanner)(ECS_Handle *self, M7_TriangleDraw tri, int (*scanlines)[2], int range[2]);

typedef enum M7_RasterizerFlags {
    M7_RASTERIZER_ALPHA_BLEND         = 1 << 0,
    M7_RASTERIZER_ALPHA_SCISSOR       = 1 << 1,
    M7_RASTERIZER_WRITE_DEPTH         = 1 << 2,
    M7_RASTERIZER_TEST_DEPTH          = 1 << 3,
    M7_RASTERIZER_INTERPOLATE_NORMALS = 1 << 4,
    M7_RASTERIZER_CULL_BACKFACE       = 1 << 5,
    M7_RASTERIZER_SORT_TRIANGLES      = 1 << 6,
    M7_RASTERIZER_FLAG_COMBINATIONS   = 1 << 7
} M7_RasterizerFlags;

typedef struct M7_MeshFace {
    size_t idx_verts[3];
    size_t idx_tverts[3];
} M7_MeshFace;

typedef struct M7_TriangleDraw {
    List(M7_FragmentShader) *shader_pipeline;
    ECS_Handle *shader_state;
    vec3 vs_verts[3];
    vec3 vs_nrmls[3];
    vec2 ts_verts[3];
    vec2 ss_verts[3];
} M7_TriangleDraw;

typedef struct M7_RasterizerArgs {
    M7_VertexProjector project;
    M7_RasterScanner scan;
    float near;
    int parallelism;
} M7_RasterizerArgs;

typedef struct M7_ModelInstance {
   M7_FragmentShader *shader_pipeline;
   size_t nshaders;
   size_t render_batch;
   M7_RasterizerFlags flags;
} M7_ModelInstance;

typedef struct M7_ModelArgs {
    M7_Mesh *(*get_mesh)(ECS_Handle *self);
    M7_ModelInstance *instances;
    size_t ninstances;
} M7_ModelArgs;

typedef struct M7_Teapot {
    float scale;
} M7_Teapot;

typedef struct M7_Torus {
    size_t outer_precision, inner_precision;
    float outer_radius, inner_radius;
} M7_Torus;

typedef struct M7_Sphere {
    size_t nrings, ring_precision;
    float radius;
} M7_Sphere;

typedef struct M7_Rect {
    float width, height;
} M7_Rect;

typedef struct M7_Cubemap {
    float scale;
} M7_Cubemap;

typedef struct M7_SolidColor {
    float r, g, b;
} M7_SolidColor;

typedef struct M7_Checkerboard {
    int tiles;
    float r1, g1, b1;
    float r2, g2, b2;
} M7_Checkerboard;

typedef struct M7_TextureMap {
    M7_Texture *texture;
    char *texture_path;
    float scale;
} M7_TextureMap;

M7_SHADER_DECLARE(M7_ShadeSolidColor)
M7_SHADER_DECLARE(M7_ShadeCheckerboard)
M7_SHADER_DECLARE(M7_ShadeOriginLight)
M7_SHADER_DECLARE(M7_ShadeTextureMap)

M7_Mesh *M7_Teapot_GetMesh(ECS_Handle *self);
M7_Mesh *M7_Torus_GetMesh(ECS_Handle *self);
M7_Mesh *M7_Sphere_GetMesh(ECS_Handle *self);
M7_Mesh *M7_Rect_GetMesh(ECS_Handle *self);
M7_Mesh *M7_Cubemap_GetMesh(ECS_Handle *self);

xform3 M7_Entity_GetXform(ECS_Handle *self);
void M7_Entity_Xform(ECS_Handle *self, xform3 lhs);

xform3 M7_XformComposeDefault(ECS_Handle *self, xform3 lhs);
xform3 M7_XformComposeBillboard(ECS_Handle *self, xform3 lhs);
xform3 M7_XformComposeCubemap(ECS_Handle *self, xform3 lhs);
xform3 M7_XformComposeAbsolute(ECS_Handle *self, xform3 lhs);

SD_DECLARE(M7_Mesh *, M7_Mesh_Create, vec3 *, ws_verts, vec3 *, ws_norms, vec2 *, ts_verts, M7_MeshFace *, faces, size_t, nverts, size_t, nts_verts, size_t, nfaces)
void M7_Mesh_Free(M7_Mesh *mesh);

void M7_Sculpture_JoinPolyChains(M7_Sculpture *sculpture, M7_PolyChain *pc1, M7_PolyChain *pc2);
M7_PolyChain *M7_Sculpture_Vertex(M7_Sculpture *sculpture, vec3 pos);
M7_PolyChain *M7_Sculpture_Ellipse(M7_Sculpture *sculpture, vec3 center, vec3 axis1, vec3 axis2, size_t precision);
M7_Mesh *M7_Sculpture_ToMesh(M7_Sculpture *sculpture, vec3 *nrmls);
M7_Sculpture *M7_Sculpture_Create(void);
void M7_Sculpture_Free(M7_Sculpture *sculpture);

SD_DECLARE(M7_WorldGeometry *, M7_World_RegisterGeometry, ECS_Handle *, self, M7_Mesh *, mesh)

void M7_RenderInstance_Free(M7_RenderInstance *instance);

M7_RenderInstance *M7_WorldGeometry_Instance(M7_WorldGeometry *geometry, M7_FragmentShader *shaders, size_t nshaders, ECS_Handle *shader_state, size_t render_batch, M7_RasterizerFlags flags);
void M7_WorldGeometry_Free(M7_WorldGeometry *geometry);

SD_DECLARE(sd_vec2, M7_ProjectPerspective, ECS_Handle *, self, sd_vec3, point, sd_vec2, midpoint)
SD_DECLARE_VOID_RETURN(M7_ScanPerspective, ECS_Handle *, self, M7_TriangleDraw, tri, int (*)[2], scanlines, int [2], range)

#endif /* M7_3D_H */
