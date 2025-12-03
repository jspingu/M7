#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>

M7_Mesh *M7_Teapot_GetMesh(ECS_Handle *self) {
    M7_Teapot *teapot = ECS_Entity_GetComponent(self, M7_Components.Teapot);

    if (teapot->mesh)
        return teapot->mesh;

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

    teapot->mesh = M7_Mesh_Create(List_GetAddress(verts, 0), List_GetAddress(nrmls, 0), nullptr, List_GetAddress(faces, 0), nverts, 0, nfaces);

    List_Free(verts);
    List_Free(nrmls);
    List_Free(faces);
    SDL_CloseIO(teapot_data);

    return teapot->mesh;
}

M7_Mesh *M7_Torus_GetMesh(ECS_Handle *self) {
    M7_Torus *torus = ECS_Entity_GetComponent(self, M7_Components.Torus);

    if (torus->mesh)
        return torus->mesh;

    M7_Sculpture *torus_sculpt = M7_Sculpture_Create();
    List(M7_PolyChain *) *rings = List_Create(M7_PolyChain *);
    List(vec3) *nrmls = List_Create(vec3);

    for (size_t i = 0; i < torus->outer_precision; ++i) {
        vec3 outer_rot = vec3_rotate(vec3_i, vec3_k, 2 * SDL_PI_F / torus->outer_precision * i);

        List_Push(rings, M7_Sculpture_Ellipse(
            torus_sculpt,
            vec3_mul(outer_rot, torus->outer_radius),
            vec3_mul(outer_rot, torus->inner_radius),
            vec3_mul(vec3_k, torus->inner_radius),
            torus->inner_precision
        ));

        for (size_t j = 0; j < torus->inner_precision; ++j)
            List_Push(nrmls, vec3_rotate(outer_rot, vec3_cross(outer_rot, vec3_k), 2 * SDL_PI_F / torus->inner_precision * j));
    }

    for (size_t i = 0; i < List_Length(rings); ++i)
        M7_Sculpture_JoinPolyChains(torus_sculpt, List_Get(rings, i), List_Get(rings, (i + 1) % List_Length(rings)));

    torus->mesh = M7_Sculpture_ToMesh(torus_sculpt, List_GetAddress(nrmls, 0));
    List_Free(rings);
    M7_Sculpture_Free(torus_sculpt);

    return torus->mesh;
}

M7_Mesh *M7_Sphere_GetMesh(ECS_Handle *self) {
    M7_Sphere *sphere = ECS_Entity_GetComponent(self, M7_Components.Sphere);

    if (sphere->mesh)
        return sphere->mesh;

    M7_Sculpture *sphere_sculpt = M7_Sculpture_Create();
    List(M7_PolyChain *) *rings = List_Create(M7_PolyChain *);
    List(vec3) *nrmls = List_Create(vec3);
    float rot = SDL_PI_F / (sphere->nrings + 1);

    M7_PolyChain *bottom = M7_Sculpture_Vertex(sphere_sculpt, vec3_mul(vec3_j, -sphere->radius));
    List_Push(nrmls, vec3_mul(vec3_j, -1));

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

        for (size_t j = 0; j < sphere->ring_precision; ++j) {
            vec3 base = vec3_mul(vec3_j, y);
            vec3 rot = vec3_rotate(vec3_mul(vec3_i, x), vec3_mul(vec3_j, -1), 2 * SDL_PI_F / sphere->ring_precision * j);
            List_Push(nrmls, vec3_normalize(vec3_add(base, rot)));
        }
    }

    M7_PolyChain *top = M7_Sculpture_Vertex(sphere_sculpt, vec3_mul(vec3_j, sphere->radius));
    List_Push(nrmls, vec3_j);

    M7_Sculpture_JoinPolyChains(sphere_sculpt, bottom, List_Get(rings, 0));
    M7_Sculpture_JoinPolyChains(sphere_sculpt, List_Get(rings, List_Length(rings) - 1), top);

    for (size_t i = 0; i < sphere->nrings - 1; ++i)
        M7_Sculpture_JoinPolyChains(sphere_sculpt, List_Get(rings, i), List_Get(rings, i + 1));

    M7_Mesh *out = M7_Sculpture_ToMesh(sphere_sculpt, List_GetAddress(nrmls, 0));
    List_Free(rings);
    M7_Sculpture_Free(sphere_sculpt);

    return out;
}

void M7_Teapot_Free(void *component) {
    M7_Teapot *teapot = component;

    if (teapot->mesh)
        M7_Mesh_Free(teapot->mesh);
}

void M7_Torus_Free(void *component) {
    M7_Torus *torus = component;

    if (torus->mesh)
        M7_Mesh_Free(torus->mesh);
}

void M7_Sphere_Free(void *component) {
    M7_Sphere *sphere = component;

    if (sphere->mesh)
        M7_Mesh_Free(sphere->mesh);
}
