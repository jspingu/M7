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

static inline vec3 intersect_near(vec3 from, vec3 to, float near) {
    vec3 path = vec3_sub(to, from);
    vec3 slope = vec3_div(path, path.z);
    return vec3_add(from, vec3_mul(slope, near - from.z));
}

static void M7_Rasterizer_Trace(M7_RasterContext ctx, vec2 line[2]) {
    vec2 path = vec2_sub(line[1], line[0]);

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

static void M7_Rasterizer_ScanPerspective(M7_RasterContext ctx, int high, int low) {
    sd_vec3 origin = sd_vec3_set(ctx.vs_verts[0].x, ctx.vs_verts[0].y, ctx.vs_verts[0].z);
    sd_vec3 ab = sd_vec3_sub(sd_vec3_set(ctx.vs_verts[1].x, ctx.vs_verts[1].y, ctx.vs_verts[1].z), origin);
    sd_vec3 ac = sd_vec3_sub(sd_vec3_set(ctx.vs_verts[2].x, ctx.vs_verts[2].y, ctx.vs_verts[2].z), origin);

    sd_vec3 nrml = sd_vec3_cross(ab, ac);
    // sd_vec3 unit_nrml = sd_vec3_normalize(nrml);
    sd_float inv_nrml_disp = sd_float_div(sd_float_one(), sd_vec3_dot(origin, nrml));

    sd_vec3 perp_ab = sd_vec3_cross(nrml, ab);
    sd_vec3 perp_ac = sd_vec3_cross(ac, nrml);

    sd_vec2 inv_xform[3] = {
        sd_vec2_div((sd_vec2) { .x = perp_ac.x, .y = perp_ab.x }, sd_vec3_dot(ab, perp_ac)),
        sd_vec2_div((sd_vec2) { .x = perp_ac.y, .y = perp_ab.y }, sd_vec3_dot(ab, perp_ac)),
        sd_vec2_div((sd_vec2) { .x = perp_ac.z, .y = perp_ab.z }, sd_vec3_dot(ab, perp_ac))
    };

    sd_vec3 origin_nrml = sd_vec3_set(ctx.vs_nrmls[0].x, ctx.vs_nrmls[0].y, ctx.vs_nrmls[0].z);
    sd_vec3 ab_nrml = sd_vec3_sub(sd_vec3_set(ctx.vs_nrmls[1].x, ctx.vs_nrmls[1].y, ctx.vs_nrmls[1].z), origin_nrml);
    sd_vec3 ac_nrml = sd_vec3_sub(sd_vec3_set(ctx.vs_nrmls[2].x, ctx.vs_nrmls[2].y, ctx.vs_nrmls[2].z), origin_nrml);

    sd_vec3 nrml_xform[3] = {
        sd_vec3_fmadd(ac_nrml, inv_xform[0].y, sd_vec3_mul(ab_nrml, inv_xform[0].x)),
        sd_vec3_fmadd(ac_nrml, inv_xform[1].y, sd_vec3_mul(ab_nrml, inv_xform[1].x)),
        sd_vec3_fmadd(ac_nrml, inv_xform[2].y, sd_vec3_mul(ab_nrml, inv_xform[2].x))
    };

    sd_vec2 midpoint = {
        .x = sd_float_set(ctx.target->width * 0.5f),
        .y = sd_float_set(ctx.target->height * 0.5f)
    };

    sd_float normalize_ss = sd_float_div(sd_float_one(), midpoint.x);

    for (int i = high; i < low; ++i) {
        int base = i * sd_bounding_size(ctx.target->width);
        int sd_left = ctx.scanlines[i][0] / SD_LENGTH;
        int sd_right = sd_bounding_size(ctx.scanlines[i][1]);

        for (int j = sd_left; j < sd_right; ++j) {
            int left = j * SD_LENGTH;
            sd_vec3 bg = ctx.target->color[base + j];

            sd_float fragment_x = sd_float_add(sd_float_range(), sd_float_set(0.5));
                     fragment_x = sd_float_add(fragment_x, sd_float_set(left));

            sd_float fragment_y = sd_float_add(sd_float_set(i), sd_float_set(0.5));

            sd_vec2 proj_plane = sd_vec2_mul(sd_vec2_sub(
                (sd_vec2) { .x = fragment_x, .y = midpoint.y },
                (sd_vec2) { .x = midpoint.x, .y = fragment_y }
            ), normalize_ss);

            sd_float inv_z = sd_float_mul(sd_vec3_dot((sd_vec3) {
                .x = proj_plane.x,
                .y = proj_plane.y,
                .z = sd_float_one()
            }, nrml), inv_nrml_disp);

            sd_float fragment_z = sd_float_rcp(inv_z);

            sd_vec3 fragment_vs = (sd_vec3) {
                .x = sd_float_mul(proj_plane.x, fragment_z),
                .y = sd_float_mul(proj_plane.y, fragment_z),
                .z = fragment_z
            };

            sd_vec3 relative = sd_vec3_sub(fragment_vs, origin);

            sd_vec3 fragment_nrml = sd_vec3_fmadd(nrml_xform[0], relative.x, origin_nrml);
                    fragment_nrml = sd_vec3_fmadd(nrml_xform[1], relative.y, fragment_nrml);
                    fragment_nrml = sd_vec3_fmadd(nrml_xform[2], relative.z, fragment_nrml);
                    fragment_nrml = sd_vec3_normalize(fragment_nrml);

            sd_vec3 col = sd_vec3_set(0.7, 0.4, 0.1);
            sd_float intensity = sd_float_mul(
                sd_float_mul(
                    sd_float_negate(sd_vec3_dot(fragment_vs, fragment_nrml)),
                    sd_float_rcp(sd_vec3_dot(fragment_vs, fragment_vs))
                ),
                sd_float_set(50)
            );

            col = sd_vec3_mul(col, sd_float_add(sd_float_set(0.05), intensity));

            sd_float mask = sd_float_clamp_mask(
                fragment_x,
                ctx.scanlines[i][0],
                ctx.scanlines[i][1]
            );

            /* Inverse z in depth buffer */
            sd_float bg_z = ctx.target->depth[base + j];
            mask = sd_float_and(mask, sd_float_gt(inv_z, bg_z));

            ctx.target->depth[base + j] = sd_float_mask_blend(bg_z, inv_z, mask);
            ctx.target->color[base + j] = sd_vec3_mask_blend(bg, col, mask);
        }
    }
}

static void M7_Rasterizer_DrawTriangle(M7_RasterContext ctx, vec2 ss_verts[3]) {
    float min_y = SDL_min(SDL_min(ss_verts[0].y, ss_verts[1].y), ss_verts[2].y);
    float max_y = SDL_max(SDL_max(ss_verts[0].y, ss_verts[1].y), ss_verts[2].y);

    int high = SDL_clamp(roundtl(min_y), 0, ctx.target->height);
    int low = SDL_clamp(roundtl(max_y), 0, ctx.target->height);

    bool ss_verts_cw = vec2_dot(
        vec2_orthogonal(vec2_sub(ss_verts[1], ss_verts[0])),
        vec2_sub(ss_verts[2], ss_verts[0])
    ) > 0;

    if (ctx.flags & M7_RASTERIZER_CULL_BACKFACE && !ss_verts_cw)
        return;

    M7_Rasterizer_Trace(ctx, (vec2 [2]) { ss_verts[0], ss_verts[1 + !ss_verts_cw] });
    M7_Rasterizer_Trace(ctx, (vec2 [2]) { ss_verts[1 + !ss_verts_cw], ss_verts[1 + ss_verts_cw] });
    M7_Rasterizer_Trace(ctx, (vec2 [2]) { ss_verts[1 + ss_verts_cw], ss_verts[0] });

    M7_Rasterizer_ScanPerspective(ctx, high, low);
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

        for (size_t i = 0; i < sd_count; ++i) {
            wg->vs_verts[i] = translation;
            wg->vs_nrmls[i] = sd_vec3_set(0, 0, 0);
        }

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
    for (size_t i = 0; i < sd_bounding_size(c_canvas->width) * c_canvas->height; ++i) {
        c_canvas->color[i] = sd_vec3_set(0, 0, 0);
        c_canvas->depth[i] = sd_float_set(0);
    }

    /* Draw geometry */
    for (size_t i = 0; i < List_Length(c_world->render_batches); ++i) {
        /* Ignore flags for now */
        for (int flags = 0; flags < M7_RASTERIZER_FLAG_COMBINATIONS; ++flags) {
            List(M7_RenderInstance *) *flag_batch = List_Get(c_world->render_batches, i)[flags];

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
                        .flags = flags
                    };

                    /* This should segfault, since mesh->ts_verts is nullptr, but it somehow doesn't when compiled on gcc */
                    // SDL_memcpy(ctx.ts_verts, (vec2 [3]) {
                    //     instance->geometry->mesh->ts_verts[faces[j].idx_tverts[0]],
                    //     instance->geometry->mesh->ts_verts[faces[j].idx_tverts[1]],
                    //     instance->geometry->mesh->ts_verts[faces[j].idx_tverts[2]],
                    // }, sizeof(vec2 [3]));

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

                    vec2 ss_verts[3];

                    SDL_memcpy(ss_verts, &(sd_vec2_scalar [3]) {
                        sd_vec2_arr_get(instance->geometry->ss_verts, faces[j].idx_verts[0]),
                        sd_vec2_arr_get(instance->geometry->ss_verts, faces[j].idx_verts[1]),
                        sd_vec2_arr_get(instance->geometry->ss_verts, faces[j].idx_verts[2]),
                    }, sizeof(vec2 [3]));

                    /* Perform near plane clipping */
                    vec2 clipped[4];
                    int nclipped = 0;

                    for (int k = 0; k < 3; ++k) {
                        vec3 curr = ctx.vs_verts[k];
                        vec3 next = ctx.vs_verts[(k + 1) % 3];

                        if (curr.z >= rasterizer->near)
                            clipped[nclipped++] = ss_verts[k];

                        if ((curr.z < rasterizer->near) != (next.z < rasterizer->near)) {
                            vec3 intercept = intersect_near(curr, next, rasterizer->near);

                            sd_vec2 projected = rasterizer->project(self,
                                sd_vec3_set(intercept.x, intercept.y, rasterizer->near),
                                sd_vec2_set(c_canvas->width * 0.5f, c_canvas->height * 0.5f)
                            );

                            sd_vec2_scalar projected_scalar = sd_vec2_arr_get(&projected, 0);
                            SDL_memcpy(clipped + nclipped++, &projected_scalar, sizeof(vec2));
                        }
                    }

                    for (int k = 0; k < nclipped - 2; ++k)
                        M7_Rasterizer_DrawTriangle(ctx, (vec2 [3]) { clipped[0], clipped[k + 1], clipped[k + 2] });
                }
            });
        }
    }
}

#ifdef SD_BASE

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
        .near = raster_args->near
    };
}

void M7_Rasterizer_Free(void *component) {
    M7_Rasterizer *rasterizer = component;
    SDL_free(rasterizer->scanlines);
}

#endif /* SD_BASE */
