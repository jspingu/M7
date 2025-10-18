#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>
#include <M7/Math/stride.h>

#include "M7_3D_c.h"

static inline int roundtl(float f) {
    return SDL_ceilf(f - 0.5f);
}

static void M7_Rasterizer_Trace(M7_RasterContext ctx, vec2 line[2]) {
    vec2 path = vec2_sub(line[1], line[0]);
    if (path.y == 0) return;

    int trace_range[2] = {
        SDL_clamp(roundtl(line[0].y), 0, ctx.target->height),
        SDL_clamp(roundtl(line[1].y), 0, ctx.target->height)
    };

    bool descending = path.y > 0;
    float slope = path.x / path.y;
    float offset = line[!descending].x + (trace_range[!descending] + 0.5f - line[!descending].y) * slope;

    for (int i = trace_range[!descending]; i < trace_range[descending]; ++i) {
        ctx.scanlines[i][descending] = SDL_clamp(roundtl(offset), 0, ctx.target->width);
        offset += slope;
    }
}

static void M7_Rasterizer_SimpleScan(M7_RasterContext ctx, int high, int low) {
    for (int i = high; i < low; ++i) {
        int base = i * sd_bounding_size(ctx.target->width);
        int sd_left = ctx.scanlines[i][0] / SD_LENGTH;
        int sd_right = sd_bounding_size(ctx.scanlines[i][1]);

        for (int j = sd_left; j < sd_right; ++j) {
            sd_vec3 white = sd_vec3_set(1, 1, 1);
            sd_vec3 bg = ctx.target->color[base + j];

            sd_float fragment_x = sd_float_add(sd_float_range(), sd_float_set(0.5));
                     fragment_x = sd_float_add(fragment_x, sd_float_set(j * SD_LENGTH));

            ctx.target->color[base + j] = sd_vec3_mask_blend(bg, white, sd_float_clamp_mask(fragment_x, ctx.scanlines[i][0], ctx.scanlines[i][1]));
        }
    }
}

void SD_VARIANT(M7_Rasterizer_Render)(ECS_Handle *self) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
    M7_World *c_world = ECS_Entity_GetComponent(rasterizer->world, M7_Components.World);
    M7_Canvas *c_canvas = ECS_Entity_GetComponent(rasterizer->target, M7_Components.Canvas);

    /* Transform registered world geometry */
    List(M7_WorldGeometry *) *geometry = c_world->geometry;
    xform3 cam_xform = M7_Entity_GetXform(self);

    xform3 ws2vs_xform = {
        mat3x3_xpose(cam_xform.basis),
        vec3_mul(mat3x3_mul(mat3x3_xpose(cam_xform.basis), cam_xform.translation), -1)
    };

    M7_Entity_Xform(rasterizer->world, ws2vs_xform);

    List_ForEach(geometry, wg, {
        size_t sd_count = sd_bounding_size(wg->mesh->nverts);

        sd_vec3 translation = sd_vec3_set(
            wg->xform.translation.x,
            wg->xform.translation.y,
            wg->xform.translation.z
        );

        for (size_t i = 0; i < sd_count; ++i)
            wg->vs_verts[i] = translation;

        for (int i = 0; i < 3; ++i) {
            sd_vec3 column_vec = sd_vec3_set(
                wg->xform.basis.entries[i][0],
                wg->xform.basis.entries[i][1],
                wg->xform.basis.entries[i][2]
            );

            for (size_t j = 0; j < sd_count; ++j) {
                wg->vs_verts[j] = sd_vec3_fmadd(column_vec, wg->mesh->ws_verts[j].xyz[i], wg->vs_verts[j]);
                wg->vs_nrmls[j] = sd_vec3_fmadd(column_vec, wg->mesh->ws_nrmls[j].xyz[i], wg->vs_nrmls[j]);
            }
        }
    });

    /* Project transformed verticies */
    List_ForEach(geometry, wg, {
        size_t sd_count = sd_bounding_size(wg->mesh->nverts);

        for (size_t i = 0; i < sd_count; ++i)
            wg->ss_verts[i] = rasterizer->project(self, wg->vs_verts[i], sd_vec2_set(c_canvas->width * 0.5f, c_canvas->height * 0.5f));
    });

    /* Clear canvas */
    for (size_t i = 0; i < sd_bounding_size(c_canvas->width) * c_canvas->height; ++i)
        c_canvas->color[i] = sd_vec3_set(0, 0, 0);

    /* Draw geometry */
    for (size_t i = 0; i < List_Length(c_world->render_batches); ++i) {
        /* Ignore flags for now */
        for (int flag = 0; flag < M7_RASTERIZER_FLAG_COMBINATIONS; ++flag) {
            List(M7_RenderInstance *) *flag_batch = List_Get(c_world->render_batches, i)[flag];

            if (!flag_batch)
                continue;

            List_ForEach(flag_batch, instance, {
                M7_MeshFace *faces = instance->geometry->mesh->faces;
                size_t nfaces = instance->geometry->mesh->nfaces;

                for (size_t j = 0; j < nfaces; ++j) {
                    M7_RasterContext ctx = {
                        .target = c_canvas,
                        .shader = instance->shader,
                        .scanlines = rasterizer->scanlines,
                        .ts_verts = {
                            instance->geometry->mesh->ts_verts[faces[j].idx_tverts[0]],
                            instance->geometry->mesh->ts_verts[faces[j].idx_tverts[1]],
                            instance->geometry->mesh->ts_verts[faces[j].idx_tverts[2]],
                        }
                    };

                    SDL_memcpy(ctx.vs_verts, &(sd_vec3_scalar [3]) {
                        sd_vec3_arr_get(instance->geometry->vs_verts, faces[j].idx_verts[0]),
                        sd_vec3_arr_get(instance->geometry->vs_verts, faces[j].idx_verts[1]),
                        sd_vec3_arr_get(instance->geometry->vs_verts, faces[j].idx_verts[2])
                    }, sizeof(vec3 [3]));

                    SDL_memcpy(ctx.vs_nrmls, &(sd_vec3_scalar [3]) {
                        sd_vec3_arr_get(instance->geometry->vs_nrmls, faces[j].idx_verts[0]),
                        sd_vec3_arr_get(instance->geometry->vs_nrmls, faces[j].idx_verts[1]),
                        sd_vec3_arr_get(instance->geometry->vs_nrmls, faces[j].idx_verts[2])
                    }, sizeof(vec3 [3]));

                    /* Forget about near-clipping for now */
                    vec2 ss_verts[3];

                    SDL_memcpy(ss_verts, &(sd_vec2_scalar [3]) {
                        sd_vec2_arr_get(instance->geometry->ss_verts, faces[j].idx_verts[0]),
                        sd_vec2_arr_get(instance->geometry->ss_verts, faces[j].idx_verts[1]),
                        sd_vec2_arr_get(instance->geometry->ss_verts, faces[j].idx_verts[2]),
                    }, sizeof(vec2 [3]));

                    float min_y = SDL_min(SDL_min(ss_verts[0].y, ss_verts[1].y), ss_verts[2].y);
                    float max_y = (SDL_max(SDL_max(ss_verts[0].y, ss_verts[1].y), ss_verts[2].y));

                    int high = SDL_clamp(roundtl(min_y), 0, ctx.target->height);
                    int low = SDL_clamp(roundtl(max_y), 0, ctx.target->height);

                    bool ss_verts_cw = vec2_dot(
                        vec2_orthogonal(vec2_sub(ss_verts[1], ss_verts[0])),
                        vec2_sub(ss_verts[2], ss_verts[0])
                    ) > 0;

                    M7_Rasterizer_Trace(ctx, (vec2 [2]) { ss_verts[0], ss_verts[1 + !ss_verts_cw] });
                    M7_Rasterizer_Trace(ctx, (vec2 [2]) { ss_verts[1 + !ss_verts_cw], ss_verts[1 + ss_verts_cw] });
                    M7_Rasterizer_Trace(ctx, (vec2 [2]) { ss_verts[1 + ss_verts_cw], ss_verts[0] });

                    // Looks good
                    // for (int scan_idx = high; scan_idx < low; ++scan_idx)
                    //     SDL_Log("left: %i, right: %i", ctx.scanlines[scan_idx][0], ctx.scanlines[scan_idx][1]);

                    M7_Rasterizer_SimpleScan(ctx, high, low);
                }
            });
        }
    }
}

#ifndef SD_VECTORIZE

void M7_Rasterizer_Attach(ECS_Handle *self) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
    rasterizer->world = ECS_Entity_AncestorWithComponent(self, M7_Components.World, true);
    rasterizer->target = ECS_Entity_AncestorWithComponent(self, M7_Components.Canvas, true);

    M7_Canvas *c_canvas = ECS_Entity_GetComponent(rasterizer->target, M7_Components.Canvas);
    rasterizer->scanlines = SDL_malloc(sizeof(int [2]) * c_canvas->height);
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
}

#endif /* UNVECTORIZED */
