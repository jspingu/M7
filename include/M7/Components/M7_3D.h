#ifndef M7_3D_H
#define M7_3D_H

#include <M7/ECS.h>
#include <M7/Components/M7_Viewport.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>
#include <M7/Math/stride.h>

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

typedef sd_vec3 (*M7_FragmentShader)(void *ctx, sd_vec3 pos, sd_vec3 normal, sd_vec2 tex_coord);
typedef sd_vec2 (*M7_VertexProjector)(ECS_Handle *self, sd_vec3 pos, sd_vec2 midpoint);
typedef void (*M7_RasterScanner)(ECS_Handle *self, M7_TriangleDraw *tri, int (*scanlines)[2], int range[2]);

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

typedef struct M7_Canvas {
    ECS_Handle *vp;
    sd_vec3 *color;
    sd_float *depth;
    int width, height;
    int parallelism;
} M7_Canvas;

typedef struct M7_TriangleDraw {
    M7_FragmentShader shader;
    vec3 vs_verts[3];
    vec3 vs_nrmls[3];
    vec2 ts_verts[3];
    vec2 ss_verts[3];
    struct {
        int left, right;
        int top, bottom;
    } sd_bounding_box;
} M7_TriangleDraw;

typedef struct M7_RasterizerArgs {
    M7_VertexProjector project;
    M7_RasterScanner scan;
    float near;
    int parallelism;
} M7_RasterizerArgs;

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

M7_RenderInstance *M7_WorldGeometry_Instance(M7_WorldGeometry *geometry, M7_FragmentShader shader, size_t render_batch, M7_RasterizerFlags flags);
void M7_WorldGeometry_Free(M7_WorldGeometry *geometry);

SD_DECLARE(sd_vec2, M7_ProjectPerspective, ECS_Handle *, self, sd_vec3, point, sd_vec2, midpoint)
SD_DECLARE_VOID_RETURN(M7_ScanPerspective, ECS_Handle *, self, M7_TriangleDraw *, tri, int (*)[2], scanlines, int [2], range)

#endif /* M7_3D_H */
