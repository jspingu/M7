#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Collections/List.h>
#include <TX/Math/linalg.h>

TX_Mesh *TX_Teapot_GetMesh(ECS_Handle *self) {
    TX_Mesh **mesh = ECS_Entity_GetComponent(self, TX_Components.MeshPrimitive);
    TX_Teapot *teapot = ECS_Entity_GetComponent(self, TX_Components.Teapot);
    if (*mesh) return *mesh;

    SDL_IOStream *teapot_data = SDL_IOFromFile("assets/teapot_surface1.norm", "r");
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
    TX_MeshFace *faces = List_Create(TX_MeshFace);

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

                    List_Push(faces, ((TX_MeshFace) {
                        .idx_verts = { offset + 0, offset + 1, offset + 2 }
                    }));
                }

                break;

            default:
                chrs[nchrs++] = chr;
                break;
        }
    }

    *mesh = TX_Mesh_Create(List_GetAddress(verts, 0), List_GetAddress(nrmls, 0), nullptr, List_GetAddress(faces, 0), nverts, 0, nfaces);

    List_Free(verts);
    List_Free(nrmls);
    List_Free(faces);
    SDL_CloseIO(teapot_data);

    return *mesh;
}

TX_Mesh *TX_Torus_GetMesh(ECS_Handle *self) {
    TX_Mesh **mesh = ECS_Entity_GetComponent(self, TX_Components.MeshPrimitive);
    TX_Torus *torus = ECS_Entity_GetComponent(self, TX_Components.Torus);
    if (*mesh) return *mesh;

    TX_Sculpture *torus_sculpt = TX_Sculpture_Create();
    List(TX_PolyChain *) *rings = List_Create(TX_PolyChain *);

    for (size_t i = 0; i < torus->outer_precision; ++i) {
        vec3 outer_rot = vec3_rotate(vec3_i, vec3_k, 2 * SDL_PI_F / torus->outer_precision * i);

        List_Push(rings, TX_Sculpture_Ellipse(
            torus_sculpt,
            vec3_mul(outer_rot, torus->outer_radius),
            vec3_mul(outer_rot, torus->inner_radius),
            vec3_mul(vec3_k, torus->inner_radius),
            torus->inner_precision
        ));
    }

    for (size_t i = 0; i < List_Length(rings); ++i)
        TX_Sculpture_JoinPolyChains(torus_sculpt, List_Get(rings, i), List_Get(rings, (i + 1) % List_Length(rings)));

    *mesh = TX_Sculpture_ToMesh(torus_sculpt);
    List_Free(rings);
    TX_Sculpture_Free(torus_sculpt);

    return *mesh;
}

TX_Mesh *TX_Sphere_GetMesh(ECS_Handle *self) {
    TX_Mesh **mesh = ECS_Entity_GetComponent(self, TX_Components.MeshPrimitive);
    TX_Sphere *sphere = ECS_Entity_GetComponent(self, TX_Components.Sphere);
    if (*mesh) return *mesh;

    TX_Sculpture *sphere_sculpt = TX_Sculpture_Create();
    List(TX_PolyChain *) *rings = List_Create(TX_PolyChain *);
    float rot = SDL_PI_F / (sphere->nrings + 1);

    TX_PolyChain *bottom = TX_Sculpture_Vertex(sphere_sculpt, vec3_mul(vec3_j, -sphere->radius));

    for (size_t i = 1; i < sphere->nrings + 1; ++i) {
        float y = -SDL_cosf(rot * i) * sphere->radius;
        float x = SDL_sinf(rot * i) * sphere->radius;

        List_Push(rings, TX_Sculpture_Ellipse(
            sphere_sculpt,
            vec3_mul(vec3_j, y),
            vec3_mul(vec3_i, x),
            vec3_mul(vec3_k, x),
            sphere->ring_precision
        ));
    }

    TX_PolyChain *top = TX_Sculpture_Vertex(sphere_sculpt, vec3_mul(vec3_j, sphere->radius));

    TX_Sculpture_JoinPolyChains(sphere_sculpt, bottom, List_Get(rings, 0));
    TX_Sculpture_JoinPolyChains(sphere_sculpt, List_Get(rings, List_Length(rings) - 1), top);

    for (size_t i = 0; i < sphere->nrings - 1; ++i)
        TX_Sculpture_JoinPolyChains(sphere_sculpt, List_Get(rings, i), List_Get(rings, i + 1));

    *mesh = TX_Sculpture_ToMesh(sphere_sculpt);
    List_Free(rings);
    TX_Sculpture_Free(sphere_sculpt);

    return *mesh;
}

TX_Mesh *TX_Rect_GetMesh(ECS_Handle *self) {
    TX_Mesh **mesh = ECS_Entity_GetComponent(self, TX_Components.MeshPrimitive);
    TX_Rect *rect = ECS_Entity_GetComponent(self, TX_Components.Rect);
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

    TX_MeshFace faces[2] = {
        { .idx_verts = { 0, 1, 2 }, .idx_tverts = { 0, 1, 2 } },
        { .idx_verts = { 1, 3, 2 }, .idx_tverts = { 1, 3, 2 } },
    };

    *mesh = TX_Mesh_Create(ws_verts, nullptr, ts_verts, faces, 4, 4, 2);
    return *mesh;
}

TX_Mesh *TX_Cubemap_GetMesh(ECS_Handle *self) {
    TX_Mesh **mesh = ECS_Entity_GetComponent(self, TX_Components.MeshPrimitive);
    TX_Cubemap *cubemap = ECS_Entity_GetComponent(self, TX_Components.Cubemap);
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

    TX_MeshFace faces[12] = {
        { .idx_verts = { 0, 1, 3 }, .idx_tverts = { 0, 1, 4 } }, { .idx_verts = { 0, 3, 2 }, .idx_tverts = { 0, 4, 3 } },
        { .idx_verts = { 0, 4, 5 }, .idx_tverts = { 6, 11, 10 } }, { .idx_verts = { 0, 5, 1 }, .idx_tverts = { 6, 10, 5 } },
        { .idx_verts = { 0, 6, 4 }, .idx_tverts = { 2, 8, 7 } }, { .idx_verts = { 0, 2, 6 }, .idx_tverts = { 2, 3, 8 } },
        { .idx_verts = { 2, 7, 6 }, .idx_tverts = { 3, 9, 8 } }, { .idx_verts = { 2, 3, 7 }, .idx_tverts = { 3, 4, 9 } },
        { .idx_verts = { 3, 1, 5 }, .idx_tverts = { 4, 5, 10 } }, { .idx_verts = { 3, 5, 7 }, .idx_tverts = { 4, 10, 9 } },
        { .idx_verts = { 4, 6, 7 }, .idx_tverts = { 12, 8, 9 } }, { .idx_verts = { 4, 7, 5 }, .idx_tverts = { 12, 9, 13 } },
    };

    *mesh = TX_Mesh_Create(ws_verts, nullptr, ts_verts, faces, 8, 14, 12);
    return *mesh;
}

void TX_MeshPrimitive_Init(void *component, void *args) {
    (void)args;
    TX_Mesh **mesh = component;
    *mesh = nullptr;
}

void TX_MeshPrimitive_Free(void *component) {
    TX_Mesh **mesh = component;
    if (*mesh) TX_Mesh_Free(*mesh);
}
