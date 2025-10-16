#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>
#include <M7/Math/stride.h>

#include "M7_3D_c.h"

// System group: Render
// Dependencies: Rasterizer, XformComposer
void SD_VARIANT(M7_Rasterizer_Render)(ECS_Handle *self) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);

    /* Transform registered world geometry */
    List(M7_WorldGeometry *) *geometry = rasterizer->world->geometry;
    xform3 cam_xform = M7_Entity_GetXform(self);

    xform3 ws2vs_xform = {
        mat3x3_xpose(cam_xform.basis),
        vec3_mul(mat3x3_mul(mat3x3_xpose(cam_xform.basis), cam_xform.translation), -1)
    };

    M7_Entity_Xform(self, ws2vs_xform);

    List_ForEach(geometry, wg, {
        sd_vec3 translation = sd_vec3_set(
            ws2vs_xform.translation.x,
            ws2vs_xform.translation.y,
            ws2vs_xform.translation.z
        );

        size_t sd_count = sd_bounding_size(wg->mesh->nverts);
        for (size_t i = 0; i < sd_count; ++i)
            wg->vs_verts[i] = translation;

        for (int i = 0; i < 3; ++i) {
            sd_vec3 column_vec = sd_vec3_set(
                ws2vs_xform.basis.entries[i][0],
                ws2vs_xform.basis.entries[i][1],
                ws2vs_xform.basis.entries[i][2]
            );

            for (size_t j = 0; j < sd_count; ++j) {
                wg->vs_verts[j] = sd_vec3_fmadd(wg->mesh->ws_verts[j].xyz[i], column_vec, wg->vs_verts[j]);
                wg->vs_nrmls[j] = sd_vec3_fmadd(wg->mesh->ws_nrmls[j].xyz[i], column_vec, wg->vs_nrmls[j]);
            }
        }
    });

    for (size_t i = 0; i < List_Length(rasterizer->world->render_batches); ++i) {
        for (int j = 0; j < M7_RASTERIZER_FLAG_COMBINATIONS; ++j) {
            List(M7_RenderInstance *) *flag_batch = List_Get(rasterizer->world->render_batches, i)[j];

            if (!flag_batch)
                continue;

            for (size_t k = 0; k < List_Length(flag_batch); ++k) {

            }
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
    rasterizer->scanlines = SDL_malloc(sizeof(size_t [2]) * rasterizer->target->height);
}

void M7_Rasterizer_Init(void *component, void *args) {
    M7_Rasterizer *rasterizer = component;
    M7_RasterizerArgs *raster_args = args;

    *rasterizer = (M7_Rasterizer) {
        .project = raster_args->project,
        .scan = raster_args->scan,
    };
}

void M7_Rasterizer_Free(void *component) {
    M7_Rasterizer *rasterizer = component;
    SDL_free(rasterizer->scanlines);
    SDL_free(rasterizer->ss_verts);
}

#endif /* UNVECTORIZED */
