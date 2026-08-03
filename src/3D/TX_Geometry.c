#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Collections/List.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

#include "TX_3D_c.h"

TX_Mesh *SD_VARIANT(TX_Mesh_Create)(vec3 *ws_verts, vec3 *ws_nrmls, vec2 *ts_verts, TX_MeshFace *faces, size_t nverts, size_t nts_verts, size_t nfaces) {
    TX_Mesh *mesh = SDL_malloc(sizeof(TX_Mesh));
    sd_vec3 *vbuf = SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_bounding_size(nverts));
    sd_vec3 *nbuf = ws_nrmls ? SDL_aligned_alloc(SD_ALIGN, sizeof(sd_vec3) * sd_bounding_size(nverts)) : nullptr;

    for (size_t i = 0; i < nverts; ++i)
        sd_vec3_arr_set(vbuf, i, ws_verts[i].x, ws_verts[i].y, ws_verts[i].z);

    if (nbuf)
        for (size_t i = 0; i < nverts; ++i)
            sd_vec3_arr_set(nbuf, i, ws_nrmls[i].x, ws_nrmls[i].y, ws_nrmls[i].z);

    *mesh = (TX_Mesh) {
        .ws_verts = vbuf,
        .ws_nrmls = nbuf,
        .ts_verts = nts_verts ? SDL_memcpy(SDL_malloc(sizeof(vec2) * nts_verts), ts_verts, sizeof(vec2) * nts_verts) : nullptr,
        .faces = SDL_memcpy(SDL_malloc(sizeof(TX_MeshFace) * nfaces), faces, sizeof(TX_MeshFace) * nfaces),
        .nverts = nverts,
        .nfaces = nfaces
    };

    return mesh;
}

TX_WorldGeometry *SD_VARIANT(TX_World_RegisterGeometry)(ECS_Handle *self, TX_Mesh *mesh) {
    TX_World *world = ECS_Entity_GetComponent(self, TX_Components.World);
    TX_WorldGeometry *geometry = SDL_malloc(sizeof(TX_WorldGeometry));
    size_t sd_count = sd_bounding_size(mesh->nverts);

    *geometry = (TX_WorldGeometry) {
        .world = world,
        .instances = List_Create(TX_RenderInstance *),
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

TX_RenderInstance *TX_WorldGeometry_Instance(TX_WorldGeometry *geometry, TX_FragmentShader *shader_pipeline, void **shader_states, size_t nshaders, size_t render_batch, TX_RasterizerFlags flags) {
    TX_World *world = geometry->world;

    if (List_Length(world->render_batches) < render_batch + 1) {
        size_t diff = render_batch - List_Length(world->render_batches) + 1;
        List(TX_RenderInstance *) *(*new_batches)[TX_RASTERIZER_FLAG_COMBINATIONS] = List_PushSpace(world->render_batches, diff);

        for (size_t i = 0; i < diff; ++i)
            for (int j = 0; j < TX_RASTERIZER_FLAG_COMBINATIONS; ++j)
                new_batches[i][j] = nullptr;
    }

    List(TX_RenderInstance *) **flag_batches = List_Get(world->render_batches, render_batch);
    
    if (!flag_batches[flags])
        flag_batches[flags] = List_Create(TX_RenderInstance *);

    TX_RenderInstance *instance = SDL_malloc(sizeof(TX_RenderInstance));

    *instance = (TX_RenderInstance) {
        .geometry = geometry,
        .shader_pipeline = SDL_memcpy(SDL_malloc(sizeof(TX_FragmentShader) * nshaders), shader_pipeline, sizeof(TX_FragmentShader) * nshaders),
        .shader_states = SDL_memcpy(SDL_malloc(sizeof(void *) * nshaders), shader_states, sizeof(void *) * nshaders),
        .nshaders = nshaders,
        .render_batch = render_batch,
        .flags = flags
    };

    List_Push(flag_batches[flags], instance);
    List_Push(geometry->instances, instance);
    return instance;
}

void TX_Model_OnXform(ECS_Handle *self, xform3 composed) {
    TX_Model *model = ECS_Entity_GetComponent(self, TX_Components.Model);
    model->geometry->xform = composed;
}

void TX_Model_Attach(ECS_Handle *self, ECS_Component(void) *component) {
    TX_Model *mdl = ECS_Entity_GetComponent(self, component);
    ECS_Handle *world = ECS_Entity_AncestorWithComponent(self, TX_Components.World, false);
    TX_Mesh *mesh = mdl->get_mesh(self);
    mdl->geometry = TX_World_RegisterGeometry(world, mesh);
}

void TX_ModelInstance_Attach(ECS_Handle *self, ECS_Component(void) *component) {
    TX_ModelInstance *mdlinst = ECS_Entity_GetComponent(self, component);
    ECS_Handle *mdl = ECS_Entity_AncestorWithComponent(self, TX_Components.Model, false);
    TX_WorldGeometry *geometry = ECS_Entity_GetComponent(mdl, TX_Components.Model)->geometry;

    TX_FragmentShader *shader_pipeline = SDL_malloc(sizeof(TX_FragmentShader) * mdlinst->nshaders);
    void **shader_states = SDL_malloc(sizeof(void *) * mdlinst->nshaders);

    for (size_t i = 0; i < mdlinst->nshaders; ++i) {
        TX_ShaderComponent *shader_component = ECS_Entity_GetComponent(self, mdlinst->shader_components[i]);
        shader_pipeline[i] = shader_component->callback;
        shader_states[i] = shader_component->state;
    }

    mdlinst->instance = TX_WorldGeometry_Instance(geometry, shader_pipeline, shader_states, mdlinst->nshaders, mdlinst->render_batch, mdlinst->flags);
    SDL_free(shader_pipeline);
    SDL_free(shader_states);
}

void TX_Model_Detach(ECS_Handle *self, ECS_Component(void) *component) {
    TX_Model *mdl = ECS_Entity_GetComponent(self, component);
    TX_WorldGeometry_Free(mdl->geometry);
}

void TX_ModelInstance_Detach(ECS_Handle *self, ECS_Component(void) *component) {
    TX_ModelInstance *mdlinst = ECS_Entity_GetComponent(self, component);
    TX_RenderInstance_Free(mdlinst->instance);
}

void TX_World_Init(void *component, void *args) {
    (void)args;

    TX_World *world = component;
    world->geometry = List_Create(TX_WorldGeometry *);
    world->render_batches = List_Create(List(TX_RenderInstance *) *[TX_RASTERIZER_FLAG_COMBINATIONS]);
}

void TX_Model_Init(void *component, void *args) {
    TX_Model *mdl = component;
    TX_ModelArgs *mdl_args = args;
    mdl->get_mesh = mdl_args->get_mesh;
}

void TX_ModelInstance_Init(void *component, void *args) {
    TX_ModelInstance *mdlinst = component;
    TX_ModelInstanceArgs *mdlinst_args = args;
    mdlinst->nshaders = mdlinst_args->nshaders;
    mdlinst->render_batch = mdlinst_args->render_batch;
    mdlinst->flags = mdlinst_args->flags;

    mdlinst->shader_components = SDL_memcpy(
        SDL_malloc(sizeof(ECS_Component(TX_ShaderComponent) *) * mdlinst->nshaders),
        mdlinst_args->shader_components,
        sizeof(ECS_Component(TX_ShaderComponent) *) * mdlinst->nshaders
    );
}

void TX_ModelInstance_Free(void *component) {
    TX_ModelInstance *mdlinst = component;
    SDL_free(mdlinst->shader_components);
}

void TX_RenderInstance_Free(TX_RenderInstance *instance) {
    TX_World *world = instance->geometry->world;

    List(TX_RenderInstance *) *flag_batch = List_Get(world->render_batches, instance->render_batch)[instance->flags];
    List_RemoveWhere(flag_batch, instanced, instanced == instance);
    List_RemoveWhere(instance->geometry->instances, instanced, instanced == instance);
    List_Free(instance->shader_pipeline);
    SDL_free(instance);
}

void TX_WorldGeometry_Free(TX_WorldGeometry *geometry) {
    TX_World *world = geometry->world;

    List_ForEach(geometry->instances, instance, TX_RenderInstance_Free(instance); );
    List_Free(geometry->instances);

    List_RemoveWhere(world->geometry, registered, registered == geometry);

    SDL_aligned_free(geometry->vs_verts);
    SDL_aligned_free(geometry->vs_nrmls);
}

void TX_World_Free(void *component) {
    TX_World *world = component;

    List_ForEach(world->geometry, geometry, {
        List_Free(geometry->instances);
        SDL_aligned_free(geometry->vs_verts);
        SDL_aligned_free(geometry->vs_nrmls);
        SDL_free(geometry);
    });

    List_Free(world->geometry);

    for (size_t i = 0; i < List_Length(world->render_batches); ++i) {
        for (int j = 0; j < TX_RASTERIZER_FLAG_COMBINATIONS; ++j) {
            List(TX_RenderInstance *) *flag_batch = List_Get(world->render_batches, i)[j];

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

void TX_Mesh_Free(TX_Mesh *mesh) {
    SDL_aligned_free(mesh->ws_verts);
    SDL_aligned_free(mesh->ws_nrmls);
    SDL_free(mesh->ts_verts);
    SDL_free(mesh->faces);
}

#endif /* SD_SRC_VARIANT */
