#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>
#include <M7/Math/stride.h>

#include "M7_3D_c.h"

// No external linkage, SD_VARIANT not required
static void M7_Rasterizer_TransformGeometry(ECS_Handle *self) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
    List(M7_WorldGeometry *) *geometry = rasterizer->world->geometry;

    xform3 cam_xform;

    List_ForEach(geometry, wg, {
        xform3 vs_transform = {
            .basis = mat3x3_mul(mat3x3_xpose(cam_xform.basis), wg->transform.basis),
            .translation = mat3x3_mul(mat3x3_xpose(cam_xform.basis), vec3_sub(wg->transform.translation, cam_xform.translation))
        };

        sd_vec3 translation = sd_vec3_set(
            vs_transform.translation.x,
            vs_transform.translation.y,
            vs_transform.translation.z
        );

        size_t sd_count = sd_bounding_size(wg->mesh->nverts);
        for (size_t i = 0; i < sd_count; ++i)
            wg->vs_verts[i] = translation;

        for (int i = 0; i < 3; ++i) {
            sd_vec3 column_vec = sd_vec3_set(
                vs_transform.basis.entries[i][0],
                vs_transform.basis.entries[i][1],
                vs_transform.basis.entries[i][2]
            );

            for (size_t j = 0; j < sd_count; ++j) {
                wg->vs_verts[j] = sd_vec3_fmadd(wg->mesh->ws_verts[j].xyz[i], column_vec, wg->vs_verts[j]);
                wg->vs_nrmls[j] = sd_vec3_fmadd(wg->mesh->ws_nrmls[j].xyz[i], column_vec, wg->vs_nrmls[j]);
            }
        }
    });
}

// System group: Render
// Dependencies: Rasterizer, Position, Basis
void SD_VARIANT(M7_Rasterizer_Render)(ECS_Handle *self) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);

    for (size_t i = 0; i < List_Length(rasterizer->world->render_batches); ++i) {
        for (int j = 0; j < M7_RASTERIZER_FLAG_COMBINATIONS; ++j) {
            // List(M7_WorldGeometry *) *flag_batch = List_Get(rasterizer->world.render_batches, i)[j];

            // if (!flag_batch)
            //     continue;
            //
            // for (size_t k = 0; k < List_Length(flag_batch); ++k) {
            //     M7_WorldGeometry *geometry = List_Get(flag_batch, k);
            //     M7_Rasterizer_DrawGeometry(self, geometry);
            // }
        }
    }
}

#ifndef SD_VECTORIZE

void M7_Rasterizer_Attach(ECS_Handle *self) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
    ECS_Handle *e_world = ECS_Entity_AncestorWithComponent(self, M7_Components.World, true);
    ECS_Handle *e_canvas = ECS_Entity_AncestorWithComponent(self, M7_Components.Canvas, true);

    rasterizer->world = ECS_Entity_GetComponent(e_world, M7_Components.World);
    rasterizer->target = ECS_Entity_GetComponent(e_canvas, M7_Components.Canvas);
}

void M7_Rasterizer_Init(void *component, void *args) {
    M7_Rasterizer *rasterizer = component;
    M7_RasterizerArgs *raster_args = args;

    *rasterizer = (M7_Rasterizer) {
        .project = raster_args->project,
        .scan = raster_args->scan,
        .scanlines = List_Create(size_t [2])
    };
}

void M7_Rasterizer_Free(void *component) {
    M7_Rasterizer *rasterizer = component;
    List_Free(rasterizer->scanlines);
    SDL_free(rasterizer->ss_verts);
}

#endif /* UNVECTORIZED */
