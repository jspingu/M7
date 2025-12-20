#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>
#include <M7/Math/stride.h>

#include "M7_3D_c.h"

M7_Mesh *SD_VARIANT(M7_Mesh_Create)(vec3 *ws_verts, vec3 *ws_nrmls, vec2 *ts_verts, M7_MeshFace *faces, size_t nverts, size_t nts_verts, size_t nfaces) {
    M7_Mesh *mesh = SDL_malloc(sizeof(M7_Mesh));
    sd_vec3 *vbuf = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_bounding_size(nverts));
    sd_vec3 *nbuf = ws_nrmls ? SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_bounding_size(nverts)) : nullptr;

    for (size_t i = 0; i < nverts; ++i)
        sd_vec3_arr_set(vbuf, i, ws_verts[i].x, ws_verts[i].y, ws_verts[i].z);

    if (nbuf)
        for (size_t i = 0; i < nverts; ++i)
            sd_vec3_arr_set(nbuf, i, ws_nrmls[i].x, ws_nrmls[i].y, ws_nrmls[i].z);

    *mesh = (M7_Mesh) {
        .ws_verts = vbuf,
        .ws_nrmls = nbuf,
        .ts_verts = nts_verts ? SDL_memcpy(SDL_malloc(sizeof(vec2) * nts_verts), ts_verts, sizeof(vec2) * nts_verts) : nullptr,
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
        .vs_nrmls = mesh->ws_nrmls ? SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_count) : nullptr,
        .ss_verts = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec2) * sd_count),
        .xform = { mat3x3_identity, vec3_zero }
    };

    List_Push(world->geometry, geometry);
    return geometry;
}

#ifndef SD_SRC_VARIANT

M7_RenderInstance *M7_WorldGeometry_Instance(M7_WorldGeometry *geometry, M7_FragmentShader *shaders, size_t nshaders, ECS_Handle *shader_state, size_t render_batch, M7_RasterizerFlags flags) {
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
    List(M7_FragmentShader) *fs_pipe = List_Create(M7_FragmentShader);
    List_PushRange(fs_pipe, shaders, nshaders);

    *instance = (M7_RenderInstance) {
        .geometry = geometry,
        .shader_pipeline = fs_pipe,
        .shader_state = shader_state,
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

void M7_Model_Attach(ECS_Handle *self) {
    M7_Model *mdl = ECS_Entity_GetComponent(self, M7_Components.Model);
    ECS_Handle *world = ECS_Entity_AncestorWithComponent(self, M7_Components.World, true);
    M7_Mesh *mesh = mdl->get_mesh(self);
    mdl->geometry = M7_World_RegisterGeometry(world, mesh);

    List_ForEach(mdl->instances_init, inst, {
        List_Push(mdl->instances, M7_WorldGeometry_Instance(
            mdl->geometry,
            inst.shader_pipeline,
            inst.nshaders,
            self,
            inst.render_batch,
            inst.flags
        ));
    });
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
    M7_Model *mdl = component;
    M7_ModelArgs *mdl_args = args;

    mdl->get_mesh = mdl_args->get_mesh;
    mdl->instances = List_Create(M7_RenderInstance *);
    mdl->instances_init = List_Create(M7_ModelInstance);

    for (size_t i = 0; i < mdl_args->ninstances; ++i) {
        M7_ModelInstance inst = mdl_args->instances[i];
        M7_ModelInstance *new_inst = List_PushSpace(mdl->instances_init, 1);
        size_t nshaders = inst.nshaders;

        SDL_memcpy(new_inst, &inst, sizeof(M7_ModelInstance));

        new_inst->shader_pipeline = SDL_memcpy(
            SDL_malloc(sizeof(M7_FragmentShader) * nshaders),
            mdl_args->instances[i].shader_pipeline,
            sizeof(M7_FragmentShader) * nshaders
        );
    }
}

void M7_Model_Free(void *component) {
    M7_Model *mdl = component;
    List_Free(mdl->instances_init);
    List_ForEach(mdl->instances_init, inst, SDL_free(inst.shader_pipeline); );
    List_Free(mdl->instances);
}

void M7_RenderInstance_Free(M7_RenderInstance *instance) {
    M7_World *world = instance->geometry->world;

    List(M7_RenderInstance *) *flag_batch = List_Get(world->render_batches, instance->render_batch)[instance->flags];
    List_RemoveWhere(flag_batch, instanced, instanced == instance);
    List_RemoveWhere(instance->geometry->instances, instanced, instanced == instance);
    List_Free(instance->shader_pipeline);
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

            List_ForEach(flag_batch, instance, { 
                List_Free(instance->shader_pipeline);
                SDL_free(instance);
            });

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

#endif /* SD_SRC_VARIANT */
