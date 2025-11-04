#include <SDL3/SDL.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>

#include "M7_3D_c.h"

void M7_Sculpture_JoinPolyChains(M7_Sculpture *sculpture, M7_PolyChain *pc1, M7_PolyChain *pc2) {
    size_t pc1_segments = pc1->nindices - 1;
    size_t pc2_segments = pc2->nindices - 1;

    M7_MeshFace *new_faces = List_PushSpace(sculpture->faces, pc1_segments + pc2_segments);

    /* True: pc2 is the pivot. When triangle fanning, reverse the ordering of adjacent vertices in pc1 */
    bool reverse_winding = pc1_segments > pc2_segments;
    size_t *pivot, *fan;
    size_t npivots, nsegments;

    if (!reverse_winding) {
        pivot = pc1->indices;
        fan = pc2->indices;
        npivots = pc1->nindices;
        nsegments = pc2_segments;
    }
    else {
        pivot = pc2->indices;
        fan = pc1->indices;
        npivots = pc2->nindices;
        nsegments = pc1_segments;
    }

    /* Segments per pivot */
    size_t seg_pivot = nsegments / npivots;
    /* Remainder. The first seg_remain pivots will fan an extra triangle */
    size_t seg_remain = nsegments % npivots;
    /* Current index of the vertex in the fanning poly chain */
    size_t fan_acc = seg_pivot + !!seg_remain;
    /* Current index of the triangle in the output */
    size_t tri_acc = fan_acc;

    /* Perform triangle fanning with the first pivot, which does not need to "fan back" to the previous pivot */
    for (size_t i = 0; i < fan_acc; ++i) {
        new_faces[i].idx_verts[0] = *pivot;
        new_faces[i].idx_verts[1] = fan[i + reverse_winding];
        new_faces[i].idx_verts[2] = fan[i + !reverse_winding];
    }

    /* Perform triangle fanning with the remaining pivots */
    for (size_t i = 1; i < npivots; ++i) {
        new_faces[tri_acc].idx_verts[0] = fan[fan_acc];
        new_faces[tri_acc].idx_verts[1] = pivot[i - reverse_winding];
        new_faces[tri_acc].idx_verts[2] = pivot[i - !reverse_winding];
        tri_acc += 1;

        for (size_t j = 0; j < seg_pivot + (i < seg_remain); ++j) {
            new_faces[tri_acc].idx_verts[0] = pivot[i];
            new_faces[tri_acc].idx_verts[1] = fan[fan_acc + reverse_winding];
            new_faces[tri_acc].idx_verts[2] = fan[fan_acc + !reverse_winding];
            fan_acc += 1;
            tri_acc += 1;
        }
    }
}

M7_PolyChain *M7_Sculpture_Ellipse(M7_Sculpture *sculpture, vec3 center, vec3 axis1, vec3 axis2, size_t precision) {
    M7_PolyChain *chain = SDL_malloc(sizeof(M7_PolyChain));
    chain->indices = SDL_malloc(sizeof(size_t) * (precision + 1));
    chain->nindices = precision + 1;

    size_t old_len = List_Length(sculpture->verts);
    vec3 *new_verts = List_PushSpace(sculpture->verts, precision);

    for (size_t i = 0; i < precision; ++i) {
        new_verts[i] = vec3_add(center, vec3_add(
            vec3_mul(axis1, SDL_cosf(2 * SDL_PI_F / precision * i)),
            vec3_mul(axis2, SDL_sinf(2 * SDL_PI_F / precision * i))
        ));
        chain->indices[i] = old_len + i;
    }

    chain->indices[precision] = old_len;
    return chain;
}

M7_Mesh *M7_Sculpture_ToMesh(M7_Sculpture *sculpture) {
    vec3 *dummy_nrmls = SDL_malloc(sizeof(vec3) * List_Length(sculpture->verts));

    M7_Mesh *mesh = M7_Mesh_Create(
        List_GetAddress(sculpture->verts, 0),
        dummy_nrmls,
        nullptr,
        List_GetAddress(sculpture->faces, 0),
        List_Length(sculpture->verts),
        0,
        List_Length(sculpture->faces)
    );

    return mesh;
}

M7_Sculpture *M7_Sculpture_Create(void) {
    M7_Sculpture *sculpture = SDL_malloc(sizeof(M7_Sculpture));
    sculpture->verts = List_Create(vec3);
    sculpture->faces = List_Create(M7_MeshFace);
    sculpture->chains = List_Create(M7_PolyChain *);

    return sculpture;
}

void M7_Sculpture_Free(M7_Sculpture *sculpture) {
    List_Free(sculpture->verts);
    List_Free(sculpture->faces);
    List_ForEach(sculpture->chains, chain, SDL_free(chain->indices); );
    List_Free(sculpture->chains);
}
