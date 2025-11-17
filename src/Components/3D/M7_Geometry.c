#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>
#include <M7/Math/stride.h>

#include "M7_3D_c.h"

M7_Mesh *SD_VARIANT(M7_Mesh_Create)(vec3 *ws_verts, vec3 *ws_norms, vec2 *ts_verts, M7_MeshFace *faces, size_t nverts, size_t nts_verts, size_t nfaces) {
    M7_Mesh *mesh = SDL_malloc(sizeof(M7_Mesh));
    sd_vec3 *vbuf = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_bounding_size(nverts));
    sd_vec3 *nbuf = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_bounding_size(nverts));

    for (size_t i = 0; i < nverts; ++i) {
        sd_vec3_arr_set(vbuf, i, ws_verts[i].x, ws_verts[i].y, ws_verts[i].z);
        sd_vec3_arr_set(nbuf, i, ws_norms[i].x, ws_norms[i].y, ws_norms[i].z);
    }

    *mesh = (M7_Mesh) {
        .ws_verts = vbuf,
        .ws_nrmls = nbuf,
        .ts_verts = SDL_memcpy(SDL_malloc(sizeof(vec2) * nverts), ts_verts, sizeof(vec2) * nts_verts),
        .faces = SDL_memcpy(SDL_malloc(sizeof(M7_MeshFace) * nfaces), faces, sizeof(M7_MeshFace) * nfaces),
        .nverts = nverts,
        .nfaces = nfaces
    };

    return mesh;
}

M7_WorldGeometry *SD_VARIANT(M7_World_RegisterGeometry)(ECS_Handle *self, M7_Mesh *mesh) {
    M7_World *world = ECS_Entity_GetComponent(self, M7_Components.World);
    M7_WorldGeometry *geometry = SDL_malloc(sizeof(M7_WorldGeometry));
    size_t sd_count = sd_bounding_size(mesh->nverts);

    *geometry = (M7_WorldGeometry) {
        .world = world,
        .instances = List_Create(M7_RenderInstance *),
        .mesh = mesh,
        .vs_verts = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_count),
        .vs_nrmls = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_count),
        .ss_verts = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec2) * sd_count),
        .xform = { mat3x3_identity, vec3_zero }
    };

    List_Push(world->geometry, geometry);
    return geometry;
}

#ifdef SD_BASE

M7_RenderInstance *M7_WorldGeometry_Instance(M7_WorldGeometry *geometry, M7_FragmentShader shader, size_t render_batch, M7_RasterizerFlags flags) {
    M7_World *world = geometry->world;

    if (List_Length(world->render_batches) < render_batch + 1) {
        size_t diff = render_batch - List_Length(world->render_batches) + 1;
        List(M7_RenderInstance *) *(*new_batches)[M7_RASTERIZER_FLAG_COMBINATIONS] = List_PushSpace(world->render_batches, diff);

        for (size_t i = 0; i < diff; ++i)
            for (int j = 0; j < M7_RASTERIZER_FLAG_COMBINATIONS; ++j)
                new_batches[i][j] = nullptr;
    }

    List(M7_RenderInstance *) **flag_batches = List_Get(world->render_batches, render_batch);
    
    if (!flag_batches[flags])
        flag_batches[flags] = List_Create(M7_RenderInstance *);

    M7_RenderInstance *instance = SDL_malloc(sizeof(M7_RenderInstance));

    *instance = (M7_RenderInstance) {
        .geometry = geometry,
        .shader = shader,
        .render_batch = render_batch,
        .flags = flags
    };

    List_Push(flag_batches[flags], instance);
    List_Push(geometry->instances, instance);
    return instance;
}

void M7_Model_OnXform(ECS_Handle *self, xform3 composed) {
    M7_Model *model = ECS_Entity_GetComponent(self, M7_Components.Model);
    model->geometry->xform = composed;
}

void M7_Model_Update(ECS_Handle *self, double delta) {
    static float pitch = 0;
    static float yaw = 0;

    mat3x3 *basis = ECS_Entity_GetComponent(self, M7_Components.Basis);
    pitch += 2.718 / 2 * delta;
    yaw -= 3.141 / 2 * delta;
    *basis = mat3x3_rotate(mat3x3_rotate(mat3x3_identity, vec3_i, pitch), vec3_j, yaw);
}

void M7_Model_Attach(ECS_Handle *self) {
    M7_Model *mdl = ECS_Entity_GetComponent(self, M7_Components.Model);
    ECS_Handle *world = ECS_Entity_AncestorWithComponent(self, M7_Components.World, true);

    mdl->geometry = M7_World_RegisterGeometry(world, mdl->mesh);
    mdl->instance = M7_WorldGeometry_Instance(mdl->geometry, nullptr, 0, M7_RASTERIZER_CULL_BACKFACE);
}

void M7_Model_Detach(ECS_Handle *self) {
    M7_Model *mdl = ECS_Entity_GetComponent(self, M7_Components.Model);
    M7_WorldGeometry_Free(mdl->geometry);
}

void M7_World_Init(void *component, void *args) {
    (void)args;

    M7_World *world = component;
    world->geometry = List_Create(M7_WorldGeometry *);
    world->render_batches = List_Create(List(M7_RenderInstance *) *[M7_RASTERIZER_FLAG_COMBINATIONS]);
}

void M7_Model_Init(void *component, void *args) {
    (void)args;
    M7_Model *mdl = component;

    M7_Sculpture *torus = M7_Sculpture_Create();
    List(M7_PolyChain *) *rings = List_Create(M7_PolyChain *);
    List(vec3) *nrmls = List_Create(vec3);

    size_t outer_precision = 64;
    size_t inner_precision = 32;
    float outer_radius = 100;
    float inner_radius = 64;

    for (size_t i = 0; i < outer_precision; ++i) {
        vec3 outer_rot = vec3_rotate(vec3_i, vec3_k, 2 * SDL_PI_F / outer_precision * i);

        List_Push(rings, M7_Sculpture_Ellipse(
            torus,
            vec3_mul(outer_rot, outer_radius),
            vec3_mul(outer_rot, inner_radius),
            vec3_mul(vec3_k, inner_radius),
            inner_precision
        ));

        for (size_t j = 0; j < inner_precision; ++j)
            List_Push(nrmls, vec3_rotate(outer_rot, vec3_cross(outer_rot, vec3_k), 2 * SDL_PI_F / inner_precision * j));
    }

    for (size_t i = 0; i < List_Length(rings); ++i)
        M7_Sculpture_JoinPolyChains(torus, List_Get(rings, i), List_Get(rings, (i + 1) % List_Length(rings)));

    mdl->mesh = M7_Sculpture_ToMesh(torus, List_GetAddress(nrmls, 0));
    List_Free(rings);
    M7_Sculpture_Free(torus);

    // M7_Sculpture *sphere = M7_Sculpture_Create();
    // List(M7_PolyChain *) *rings = List_Create(M7_PolyChain *);
    //
    // int nrings = 32;
    // int ring_precision = 32;
    // float radius = 128;
    // float rot = SDL_PI_F / (nrings + 1);
    //
    // M7_PolyChain *bottom = M7_Sculpture_Vertex(sphere, vec3_mul(vec3_j, -radius));
    // M7_PolyChain *top = M7_Sculpture_Vertex(sphere, vec3_mul(vec3_j, radius));
    //
    // for (int i = 1; i < nrings + 1; ++i) {
    //     float y = -SDL_cosf(rot * i) * radius;
    //     float x = SDL_sinf(rot * i) * radius;
    //
    //     List_Push(rings, M7_Sculpture_Ellipse(
    //         sphere,
    //         vec3_mul(vec3_j, y),
    //         vec3_mul(vec3_i, x),
    //         vec3_mul(vec3_k, x),
    //         ring_precision
    //     ));
    // }
    //
    // M7_Sculpture_JoinPolyChains(sphere, bottom, List_Get(rings, 0));
    // M7_Sculpture_JoinPolyChains(sphere, List_Get(rings, List_Length(rings) - 1), top);
    //
    // for (int i = 0; i < nrings - 1; ++i)
    //     M7_Sculpture_JoinPolyChains(sphere, List_Get(rings, i), List_Get(rings, i + 1));
    //
    // mdl->mesh = M7_Sculpture_ToMesh(sphere);
    // List_Free(rings);
    // M7_Sculpture_Free(sphere);
}

void M7_Model_Free(void *component) {
    M7_Model *mdl = component;
    M7_Mesh_Free(mdl->mesh);
}

void M7_RenderInstance_Free(M7_RenderInstance *instance) {
    M7_World *world = instance->geometry->world;

    List(M7_RenderInstance *) *flag_batch = List_Get(world->render_batches, instance->render_batch)[instance->flags];
    List_RemoveWhere(flag_batch, instanced, instanced == instance);
    List_RemoveWhere(instance->geometry->instances, instanced, instanced == instance);
    SDL_free(instance);
}

void M7_WorldGeometry_Free(M7_WorldGeometry *geometry) {
    M7_World *world = geometry->world;

    List_ForEach(geometry->instances, instance, M7_RenderInstance_Free(instance); );
    List_Free(geometry->instances);

    List_RemoveWhere(world->geometry, registered, registered == geometry);

    SDL_aligned_free(geometry->vs_verts);
    SDL_aligned_free(geometry->vs_nrmls);
}

void M7_World_Free(void *component) {
    M7_World *world = component;

    List_ForEach(world->geometry, geometry, {
        List_Free(geometry->instances);
        SDL_aligned_free(geometry->vs_verts);
        SDL_aligned_free(geometry->vs_nrmls);
        SDL_free(geometry);
    });

    List_Free(world->geometry);

    for (size_t i = 0; i < List_Length(world->render_batches); ++i) {
        for (int j = 0; j < M7_RASTERIZER_FLAG_COMBINATIONS; ++j) {
            List(M7_RenderInstance *) *flag_batch = List_Get(world->render_batches, i)[j];

            if (!flag_batch)
                continue;

            List_ForEach(flag_batch, instance, SDL_free(instance); );
            List_Free(flag_batch);
        }
    }

    List_Free(world->render_batches);
}

void M7_Mesh_Free(M7_Mesh *mesh) {
    SDL_aligned_free(mesh->ws_verts);
    SDL_aligned_free(mesh->ws_nrmls);
    SDL_free(mesh->ts_verts);
    SDL_free(mesh->faces);
}

#endif /* SD_BASE */
