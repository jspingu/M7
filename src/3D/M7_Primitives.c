#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>

M7_Mesh *M7_Teapot_GetMesh(ECS_Handle *self) {
    M7_Mesh **mesh = ECS_Entity_GetComponent(self, M7_Components.MeshPrimitive);
    M7_Teapot *teapot = ECS_Entity_GetComponent(self, M7_Components.Teapot);
    if (*mesh) return *mesh;

    SDL_IOStream *teapot_data = SDL_IOFromFile("assets/teapot_surface0.norm", "r");
    Uint8 chr;
    char chrs[64];
    int nchrs = 0;

    /* Get triangle count */
    while (SDL_ReadU8(teapot_data, &chr)) {
        if (chr == '\n') {
            chrs[nchrs] = '\0';
            nchrs = 0;
            break;
        }

        chrs[nchrs++] = chr;
    }

    size_t nfaces = SDL_strtoull(chrs, nullptr, 0);
    size_t nverts = nfaces * 3;

    vec3 *verts = List_Create(vec3);
    vec3 *nrmls = List_Create(vec3);
    M7_MeshFace *faces = List_Create(M7_MeshFace);

    /* Read vertex/normal data */
    float vals[18];
    int nvals = 0;

    while (SDL_ReadU8(teapot_data, &chr)) {
        switch (chr) {
            case ' ':
            case '\n':
                if (!nchrs)
                    break;

                chrs[nchrs] = '\0';
                nchrs = 0;
                vals[nvals++] = SDL_strtod(chrs, nullptr);

                if (nvals == 18) {
                    nvals = 0;
                    size_t offset = List_Length(verts);

                    for (int i = 0; i < 3; ++i) {
                        List_Push(verts, vec3_mul((vec3) {
                            .x = vals[i * 6 + 0],
                            .y = vals[i * 6 + 1],
                            .z = vals[i * 6 + 2]
                        }, teapot->scale));

                        List_Push(nrmls, vec3_normalize((vec3) {
                            .x = vals[i * 6 + 3],
                            .y = vals[i * 6 + 4],
                            .z = vals[i * 6 + 5]
                        }));
                    }

                    List_Push(faces, ((M7_MeshFace) {
                        .idx_verts = { offset + 0, offset + 1, offset + 2 }
                    }));
                }

                break;

            default:
                chrs[nchrs++] = chr;
                break;
        }
    }

    *mesh = M7_Mesh_Create(List_GetAddress(verts, 0), List_GetAddress(nrmls, 0), nullptr, List_GetAddress(faces, 0), nverts, 0, nfaces);

    List_Free(verts);
    List_Free(nrmls);
    List_Free(faces);
    SDL_CloseIO(teapot_data);

    return *mesh;
}

M7_Mesh *M7_Torus_GetMesh(ECS_Handle *self) {
    M7_Mesh **mesh = ECS_Entity_GetComponent(self, M7_Components.MeshPrimitive);
    M7_Torus *torus = ECS_Entity_GetComponent(self, M7_Components.Torus);
    if (*mesh) return *mesh;

    M7_Sculpture *torus_sculpt = M7_Sculpture_Create();
    List(M7_PolyChain *) *rings = List_Create(M7_PolyChain *);

    for (size_t i = 0; i < torus->outer_precision; ++i) {
        vec3 outer_rot = vec3_rotate(vec3_i, vec3_k, 2 * SDL_PI_F / torus->outer_precision * i);

        List_Push(rings, M7_Sculpture_Ellipse(
            torus_sculpt,
            vec3_mul(outer_rot, torus->outer_radius),
            vec3_mul(outer_rot, torus->inner_radius),
            vec3_mul(vec3_k, torus->inner_radius),
            torus->inner_precision
        ));
    }

    for (size_t i = 0; i < List_Length(rings); ++i)
        M7_Sculpture_JoinPolyChains(torus_sculpt, List_Get(rings, i), List_Get(rings, (i + 1) % List_Length(rings)));

    *mesh = M7_Sculpture_ToMesh(torus_sculpt);
    List_Free(rings);
    M7_Sculpture_Free(torus_sculpt);

    return *mesh;
}

M7_Mesh *M7_Sphere_GetMesh(ECS_Handle *self) {
    M7_Mesh **mesh = ECS_Entity_GetComponent(self, M7_Components.MeshPrimitive);
    M7_Sphere *sphere = ECS_Entity_GetComponent(self, M7_Components.Sphere);
    if (*mesh) return *mesh;

    M7_Sculpture *sphere_sculpt = M7_Sculpture_Create();
    List(M7_PolyChain *) *rings = List_Create(M7_PolyChain *);
    float rot = SDL_PI_F / (sphere->nrings + 1);

    M7_PolyChain *bottom = M7_Sculpture_Vertex(sphere_sculpt, vec3_mul(vec3_j, -sphere->radius));

    for (size_t i = 1; i < sphere->nrings + 1; ++i) {
        float y = -SDL_cosf(rot * i) * sphere->radius;
        float x = SDL_sinf(rot * i) * sphere->radius;

        List_Push(rings, M7_Sculpture_Ellipse(
            sphere_sculpt,
            vec3_mul(vec3_j, y),
            vec3_mul(vec3_i, x),
            vec3_mul(vec3_k, x),
            sphere->ring_precision
        ));
    }

    M7_PolyChain *top = M7_Sculpture_Vertex(sphere_sculpt, vec3_mul(vec3_j, sphere->radius));

    M7_Sculpture_JoinPolyChains(sphere_sculpt, bottom, List_Get(rings, 0));
    M7_Sculpture_JoinPolyChains(sphere_sculpt, List_Get(rings, List_Length(rings) - 1), top);

    for (size_t i = 0; i < sphere->nrings - 1; ++i)
        M7_Sculpture_JoinPolyChains(sphere_sculpt, List_Get(rings, i), List_Get(rings, i + 1));

    *mesh = M7_Sculpture_ToMesh(sphere_sculpt);
    List_Free(rings);
    M7_Sculpture_Free(sphere_sculpt);

    return *mesh;
}

M7_Mesh *M7_Rect_GetMesh(ECS_Handle *self) {
    M7_Mesh **mesh = ECS_Entity_GetComponent(self, M7_Components.MeshPrimitive);
    M7_Rect *rect = ECS_Entity_GetComponent(self, M7_Components.Rect);
    if (*mesh) return *mesh;

    vec3 ws_verts[4] = {
        { .x = -rect->width * 0.5f, .y = rect->height * 0.5f },
        { .x = rect->width * 0.5f, .y = rect->height * 0.5f },
        { .x = -rect->width * 0.5f, .y = -rect->height * 0.5f },
        { .x = rect->width * 0.5f, .y = -rect->height * 0.5f },
    };

    float unit = SDL_max(rect->width, rect->height);

    vec2 ts_verts[4] = {
        vec2_zero,
        { .x = rect->width / unit },
        { .y = rect->height / unit },
        { .x = rect->width / unit, .y = rect->height / unit },
    };

    M7_MeshFace faces[2] = {
        { .idx_verts = { 0, 1, 2 }, .idx_tverts = { 0, 1, 2 } },
        { .idx_verts = { 1, 3, 2 }, .idx_tverts = { 1, 3, 2 } },
    };

    *mesh = M7_Mesh_Create(ws_verts, nullptr, ts_verts, faces, 4, 4, 2);
    return *mesh;
}

M7_Mesh *M7_Cubemap_GetMesh(ECS_Handle *self) {
    M7_Mesh **mesh = ECS_Entity_GetComponent(self, M7_Components.MeshPrimitive);
    M7_Cubemap *cubemap = ECS_Entity_GetComponent(self, M7_Components.Cubemap);
    if (*mesh) return *mesh;

    vec3 ws_verts[8] = {
        { .x=-1, .y=1, .z=-1 }, { .x=1, .y=1, .z=-1 },
        { .x=-1, .y=1, .z=1 }, { .x=1, .y=1, .z=1 },
        { .x=-1, .y=-1, .z=-1 }, { .x=1, .y=-1, .z=-1 },
        { .x=-1, .y=-1, .z=1 }, { .x=1, .y=-1, .z=1 },
    };

    for (int i = 0; i < 8; ++i)
        ws_verts[i] = vec3_mul(ws_verts[i], cubemap->scale);

    vec2 ts_verts[14] = {
        { .x=1.0/4, .y=0.0/4 }, { .x=2.0/4, .y=0.0/4 },
        { .x=0.0/4, .y=1.0/4 }, { .x=1.0/4, .y=1.0/4 }, { .x=2.0/4, .y=1.0/4 }, { .x=3.0/4, .y=1.0/4 }, { .x=4.0/4, .y=1.0/4 },
        { .x=0.0/4, .y=2.0/4 }, { .x=1.0/4, .y=2.0/4 }, { .x=2.0/4, .y=2.0/4 }, { .x=3.0/4, .y=2.0/4 }, { .x=4.0/4, .y=2.0/4 },
        { .x=1.0/4, .y=3.0/4 }, { .x=2.0/4, .y=3.0/4 },
    };

    M7_MeshFace faces[12] = {
        { .idx_verts = { 0, 1, 3 }, .idx_tverts = { 0, 1, 4 } }, { .idx_verts = { 0, 3, 2 }, .idx_tverts = { 0, 4, 3 } },
        { .idx_verts = { 0, 4, 5 }, .idx_tverts = { 6, 11, 10 } }, { .idx_verts = { 0, 5, 1 }, .idx_tverts = { 6, 10, 5 } },
        { .idx_verts = { 0, 6, 4 }, .idx_tverts = { 2, 8, 7 } }, { .idx_verts = { 0, 2, 6 }, .idx_tverts = { 2, 3, 8 } },
        { .idx_verts = { 2, 7, 6 }, .idx_tverts = { 3, 9, 8 } }, { .idx_verts = { 2, 3, 7 }, .idx_tverts = { 3, 4, 9 } },
        { .idx_verts = { 3, 1, 5 }, .idx_tverts = { 4, 5, 10 } }, { .idx_verts = { 3, 5, 7 }, .idx_tverts = { 4, 10, 9 } },
        { .idx_verts = { 4, 6, 7 }, .idx_tverts = { 12, 8, 9 } }, { .idx_verts = { 4, 7, 5 }, .idx_tverts = { 12, 9, 13 } },
    };

    *mesh = M7_Mesh_Create(ws_verts, nullptr, ts_verts, faces, 8, 14, 12);
    return *mesh;
}

void M7_MeshPrimitive_Init(void *component, void *args) {
    (void)args;
    M7_Mesh **mesh = component;
    *mesh = nullptr;
}

void M7_MeshPrimitive_Free(void *component) {
    M7_Mesh **mesh = component;
    if (*mesh) M7_Mesh_Free(*mesh);
}
