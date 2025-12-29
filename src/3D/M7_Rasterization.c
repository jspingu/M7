#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>
#include <M7/Math/stride.h>

#include "M7_3D_c.h"

typedef struct SubCanvasRenderData {
    ECS_Handle *rasterizer;
    int bounds[2];
    int (*scanlines)[2];
} SubCanvasRenderData;

static inline int roundtl(float f) {
    return SDL_ceilf(f - 0.5f);
}

static inline vec3 intersect_near(vec3 from, vec3 to, float near) {
    vec3 path = vec3_sub(to, from);
    vec3 slope = vec3_div(path, path.z);
    return vec3_add(from, vec3_mul(slope, near - from.z));
}

void SD_VARIANT(M7_ScanLinear)(ECS_Handle *self, M7_TriangleDraw triangle, M7_RasterizerFlags flags, int (*scanlines)[2], int range[2]) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
    M7_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, M7_Components.Canvas);

    sd_vec2 origin = sd_vec2_set(triangle.ss_verts[0].x ,triangle.ss_verts[0].y);
    sd_vec2 ab = sd_vec2_sub(sd_vec2_set(triangle.ss_verts[1].x, triangle.ss_verts[1].y), origin);
    sd_vec2 ac = sd_vec2_sub(sd_vec2_set(triangle.ss_verts[2].x, triangle.ss_verts[2].y), origin);

    sd_float inv_disc = sd_float_rcp(sd_float_sub(sd_float_mul(ab.x, ac.y), sd_float_mul(ab.y, ac.x)));

    sd_vec2 inv_xform[2] = {
        sd_vec2_muls((sd_vec2) { .x = ac.y, .y = sd_float_negate(ab.y) }, inv_disc),
        sd_vec2_muls((sd_vec2) { .x = sd_float_negate(ac.x), .y = ab.x }, inv_disc)
    };

    sd_vec3 origin_vs = sd_vec3_set(triangle.vs_verts[0].x, triangle.vs_verts[0].y, triangle.vs_verts[0].z);
    sd_vec3 ab_vs = sd_vec3_sub(sd_vec3_set(triangle.vs_verts[1].x, triangle.vs_verts[1].y, triangle.vs_verts[1].z), origin_vs);
    sd_vec3 ac_vs = sd_vec3_sub(sd_vec3_set(triangle.vs_verts[2].x, triangle.vs_verts[2].y, triangle.vs_verts[2].z), origin_vs);

    sd_vec3 vs_xform[2] = {
        sd_vec3_fmadd(ac_vs, inv_xform[0].y, sd_vec3_muls(ab_vs, inv_xform[0].x)),
        sd_vec3_fmadd(ac_vs, inv_xform[1].y, sd_vec3_muls(ab_vs, inv_xform[1].x))
    };

    sd_vec3 origin_nrml = sd_vec3_set(triangle.vs_nrmls[0].x, triangle.vs_nrmls[0].y, triangle.vs_nrmls[0].z);
    sd_vec3 ab_nrml = sd_vec3_sub(sd_vec3_set(triangle.vs_nrmls[1].x, triangle.vs_nrmls[1].y, triangle.vs_nrmls[1].z), origin_nrml);
    sd_vec3 ac_nrml = sd_vec3_sub(sd_vec3_set(triangle.vs_nrmls[2].x, triangle.vs_nrmls[2].y, triangle.vs_nrmls[2].z), origin_nrml);

    sd_vec3 nrml_xform[2] = {
        sd_vec3_fmadd(ac_nrml, inv_xform[0].y, sd_vec3_muls(ab_nrml, inv_xform[0].x)),
        sd_vec3_fmadd(ac_nrml, inv_xform[1].y, sd_vec3_muls(ab_nrml, inv_xform[1].x))
    };

    sd_vec2 origin_ts = sd_vec2_set(triangle.ts_verts[0].x, triangle.ts_verts[0].y);
    sd_vec2 ab_ts = sd_vec2_sub(sd_vec2_set(triangle.ts_verts[1].x, triangle.ts_verts[1].y), origin_ts);
    sd_vec2 ac_ts = sd_vec2_sub(sd_vec2_set(triangle.ts_verts[2].x, triangle.ts_verts[2].y), origin_ts);

    sd_vec2 ts_xform[2] = {
        sd_vec2_fmadd(ac_ts, inv_xform[0].y, sd_vec2_muls(ab_ts, inv_xform[0].x)),
        sd_vec2_fmadd(ac_ts, inv_xform[1].y, sd_vec2_muls(ab_ts, inv_xform[1].x))
    };

    vec3 scalar_nrml = vec3_cross(vec3_sub(triangle.vs_verts[1], triangle.vs_verts[0]), vec3_sub(triangle.vs_verts[2], triangle.vs_verts[0]));
    sd_vec3 nrml = sd_vec3_set(scalar_nrml.x, scalar_nrml.y, scalar_nrml.z);

    for (int i = range[0]; i < range[1]; ++i) {
        int base = i * sd_bounding_size(canvas->width);
        int sd_left = scanlines[i][0] / SD_LENGTH;
        int sd_right = sd_bounding_size(scanlines[i][1]);

        for (int j = sd_left; j < sd_right; ++j) {
            int left = j * SD_LENGTH;

            sd_vec2 ss = {
                .x = sd_float_add(sd_float_set(left), sd_float_add(sd_float_range(), sd_float_set(0.5f))),
                .y = sd_float_add(sd_float_set(i), sd_float_set(0.5f))
            };

            sd_vec2 relative = sd_vec2_sub(ss, origin);

            sd_vec3 fragment_vs = sd_vec3_fmadd(vs_xform[0], relative.x, origin_vs);
                    fragment_vs = sd_vec3_fmadd(vs_xform[1], relative.y, fragment_vs);

            sd_float inv_z = sd_float_rcp(fragment_vs.z);
            sd_vec3 fragment_nrml;

            if (flags & M7_RASTERIZER_INTERPOLATE_NORMALS) {
                fragment_nrml = sd_vec3_fmadd(nrml_xform[0], relative.x, origin_nrml);
                fragment_nrml = sd_vec3_fmadd(nrml_xform[1], relative.y, fragment_nrml);
            } else fragment_nrml = nrml;

            fragment_nrml = sd_vec3_normalize(fragment_nrml);

            sd_vec2 fragment_ts = sd_vec2_fmadd(ts_xform[0], relative.x, origin_ts);
                    fragment_ts = sd_vec2_fmadd(ts_xform[1], relative.y, fragment_ts);

            M7_ShaderParams fragment = {
                .vs = fragment_vs,
                .nrml = fragment_nrml,
                .ts = fragment_ts
            };

            for (size_t i = 0; i < triangle.nshaders; ++i)
                fragment.col = triangle.shader_pipeline[i](triangle.shader_states[i], fragment);

            sd_vec3 bg = canvas->color[base + j];
            sd_float bg_z = canvas->depth[base + j];
            sd_mask mask = sd_float_clamp_mask(ss.x, scanlines[i][0], scanlines[i][1]);

            if (flags & M7_RASTERIZER_TEST_DEPTH)
                mask = sd_mask_and(mask, sd_float_gt(inv_z, bg_z));

            if (flags & M7_RASTERIZER_WRITE_DEPTH)
                canvas->depth[base + j] = sd_float_mask_blend(bg_z, inv_z, mask);

            canvas->color[base + j] = sd_vec3_mask_blend(bg, fragment.col.rgb, mask);
        }
    }
}

void SD_VARIANT(M7_ScanPerspective)(ECS_Handle *self, M7_TriangleDraw triangle, M7_RasterizerFlags flags, int (*scanlines)[2], int range[2]) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
    M7_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, M7_Components.Canvas);
    M7_PerspectiveFOV *perspective_fov = ECS_Entity_GetComponent(self, M7_Components.PerspectiveFOV);

    sd_vec3 origin = sd_vec3_set(triangle.vs_verts[0].x, triangle.vs_verts[0].y, triangle.vs_verts[0].z);
    sd_vec3 ab = sd_vec3_sub(sd_vec3_set(triangle.vs_verts[1].x, triangle.vs_verts[1].y, triangle.vs_verts[1].z), origin);
    sd_vec3 ac = sd_vec3_sub(sd_vec3_set(triangle.vs_verts[2].x, triangle.vs_verts[2].y, triangle.vs_verts[2].z), origin);

    sd_vec3 nrml = sd_vec3_cross(ab, ac);
    sd_float inv_nrml_disp = sd_float_rcp(sd_vec3_dot(origin, nrml));

    sd_vec3 perp_ab = sd_vec3_cross(nrml, ab);
    sd_vec3 perp_ac = sd_vec3_cross(ac, nrml);

    sd_float inv_pgram_area = sd_float_rcp(sd_vec3_dot(ab, perp_ac));

    sd_vec2 inv_xform[3] = {
        sd_vec2_muls((sd_vec2) { .x = perp_ac.x, .y = perp_ab.x }, inv_pgram_area),
        sd_vec2_muls((sd_vec2) { .x = perp_ac.y, .y = perp_ab.y }, inv_pgram_area),
        sd_vec2_muls((sd_vec2) { .x = perp_ac.z, .y = perp_ab.z }, inv_pgram_area)
    };

    sd_vec3 origin_nrml = sd_vec3_set(triangle.vs_nrmls[0].x, triangle.vs_nrmls[0].y, triangle.vs_nrmls[0].z);
    sd_vec3 ab_nrml = sd_vec3_sub(sd_vec3_set(triangle.vs_nrmls[1].x, triangle.vs_nrmls[1].y, triangle.vs_nrmls[1].z), origin_nrml);
    sd_vec3 ac_nrml = sd_vec3_sub(sd_vec3_set(triangle.vs_nrmls[2].x, triangle.vs_nrmls[2].y, triangle.vs_nrmls[2].z), origin_nrml);

    sd_vec3 nrml_xform[3] = {
        sd_vec3_fmadd(ac_nrml, inv_xform[0].y, sd_vec3_muls(ab_nrml, inv_xform[0].x)),
        sd_vec3_fmadd(ac_nrml, inv_xform[1].y, sd_vec3_muls(ab_nrml, inv_xform[1].x)),
        sd_vec3_fmadd(ac_nrml, inv_xform[2].y, sd_vec3_muls(ab_nrml, inv_xform[2].x))
    };

    sd_vec2 origin_ts = sd_vec2_set(triangle.ts_verts[0].x, triangle.ts_verts[0].y);
    sd_vec2 ab_ts = sd_vec2_sub(sd_vec2_set(triangle.ts_verts[1].x, triangle.ts_verts[1].y), origin_ts);
    sd_vec2 ac_ts = sd_vec2_sub(sd_vec2_set(triangle.ts_verts[2].x, triangle.ts_verts[2].y), origin_ts);

    sd_vec2 ts_xform[3] = {
        sd_vec2_fmadd(ac_ts, inv_xform[0].y, sd_vec2_muls(ab_ts, inv_xform[0].x)),
        sd_vec2_fmadd(ac_ts, inv_xform[1].y, sd_vec2_muls(ab_ts, inv_xform[1].x)),
        sd_vec2_fmadd(ac_ts, inv_xform[2].y, sd_vec2_muls(ab_ts, inv_xform[2].x))
    };

    sd_vec2 midpoint = {
        .x = sd_float_set(canvas->width * 0.5f),
        .y = sd_float_set(canvas->height * 0.5f)
    };

    sd_float normalize_ss = sd_float_mul(sd_float_set(perspective_fov->tan_half_fov), sd_float_rcp(midpoint.x));

    for (int i = range[0]; i < range[1]; ++i) {
        int base = i * sd_bounding_size(canvas->width);
        int sd_left = scanlines[i][0] / SD_LENGTH;
        int sd_right = sd_bounding_size(scanlines[i][1]);

        for (int j = sd_left; j < sd_right; ++j) {
            int left = j * SD_LENGTH;
            sd_float fragment_x = sd_float_add(sd_float_range(), sd_float_set(0.5));
                     fragment_x = sd_float_add(fragment_x, sd_float_set(left));

            sd_float fragment_y = sd_float_add(sd_float_set(i), sd_float_set(0.5));

            sd_vec2 proj_plane = sd_vec2_muls(sd_vec2_sub(
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
            sd_vec3 fragment_nrml;

            if (flags & M7_RASTERIZER_INTERPOLATE_NORMALS) {
                fragment_nrml = sd_vec3_fmadd(nrml_xform[0], relative.x, origin_nrml);
                fragment_nrml = sd_vec3_fmadd(nrml_xform[1], relative.y, fragment_nrml);
                fragment_nrml = sd_vec3_fmadd(nrml_xform[2], relative.z, fragment_nrml);
            } else fragment_nrml = nrml;

            fragment_nrml = sd_vec3_normalize(fragment_nrml);

            sd_vec2 fragment_ts = sd_vec2_fmadd(ts_xform[0], relative.x, origin_ts);
                    fragment_ts = sd_vec2_fmadd(ts_xform[1], relative.y, fragment_ts);
                    fragment_ts = sd_vec2_fmadd(ts_xform[2], relative.z, fragment_ts);

            M7_ShaderParams fragment = {
                .vs = fragment_vs,
                .nrml = fragment_nrml,
                .ts = fragment_ts
            };

            for (size_t i = 0; i < triangle.nshaders; ++i)
                fragment.col = triangle.shader_pipeline[i](triangle.shader_states[i], fragment);

            sd_vec3 bg = canvas->color[base + j];
            sd_float bg_z = canvas->depth[base + j];
            sd_mask mask = sd_float_clamp_mask(fragment_x, scanlines[i][0], scanlines[i][1]);

            if (flags & M7_RASTERIZER_TEST_DEPTH)
                mask = sd_mask_and(mask, sd_float_gt(inv_z, bg_z));

            if (flags & M7_RASTERIZER_WRITE_DEPTH)
                canvas->depth[base + j] = sd_float_mask_blend(bg_z, inv_z, mask);

            canvas->color[base + j] = sd_vec3_mask_blend(bg, fragment.col.rgb, mask);
        }
    }
}

static void Trace(int width, int bounds[2], int (*scanlines)[2], vec2 line[2]) {
    vec2 path = vec2_sub(line[1], line[0]);

    int trace_range[2] = {
        SDL_clamp(roundtl(line[0].y), bounds[0], bounds[1]),
        SDL_clamp(roundtl(line[1].y), bounds[0], bounds[1])
    };

    bool descending = path.y > 0;
    float slope = path.x / path.y;
    float offset = line[!descending].x + (trace_range[!descending] + 0.5f - line[!descending].y) * slope;

    for (int i = trace_range[!descending]; i < trace_range[descending]; ++i) {
        scanlines[i][descending] = SDL_clamp(roundtl(offset), 0, width);
        offset += slope;
    }
}

static void M7_Rasterizer_DrawTriangle(ECS_Handle *self, M7_TriangleDraw triangle, M7_RasterizerFlags flags, int (*scanlines)[2], int bounds[2]) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
    M7_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, M7_Components.Canvas);

    float min_y = SDL_min(SDL_min(triangle.ss_verts[0].y, triangle.ss_verts[1].y), triangle.ss_verts[2].y);
    float max_y = SDL_max(SDL_max(triangle.ss_verts[0].y, triangle.ss_verts[1].y), triangle.ss_verts[2].y);

    int high = SDL_clamp(roundtl(min_y), bounds[0], bounds[1]);
    int low = SDL_clamp(roundtl(max_y), bounds[0], bounds[1]);

    for (int i = 0; i < 3; ++i)
        Trace(canvas->width, bounds, scanlines, (vec2 [2]) { triangle.ss_verts[i], triangle.ss_verts[(i + 1) % 3] });

    rasterizer->scan(self, triangle, flags, scanlines, (int [2]) { high, low });
}

static void M7_Rasterizer_DrawBatch(ECS_Handle *self, List(M7_RenderInstance *) *batch, M7_RasterizerFlags flags, int (*scanlines)[2], int bounds[2]) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
    M7_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, M7_Components.Canvas);

    /* Draw triangles */
    List_ForEach(batch, instance, {
        M7_MeshFace *faces = instance->geometry->mesh->faces;
        size_t nfaces = instance->geometry->mesh->nfaces;

        for (size_t i = 0; i < nfaces; ++i) {
            vec3 vs_verts[3];

            SDL_memcpy(vs_verts, &(sd_vec3_scalar [3]) {
                sd_vec3_arr_get(instance->geometry->vs_verts, faces[i].idx_verts[0]),
                sd_vec3_arr_get(instance->geometry->vs_verts, faces[i].idx_verts[1]),
                sd_vec3_arr_get(instance->geometry->vs_verts, faces[i].idx_verts[2])
            }, sizeof(vec3 [3]));

            /* Perform near plane clipping */
            vec2 ss_verts[3];
            vec2 clipped[4];
            int nclipped = 0;

            SDL_memcpy(ss_verts, (sd_vec2_scalar [3]) {
                sd_vec2_arr_get(instance->geometry->ss_verts, faces[i].idx_verts[0]),
                sd_vec2_arr_get(instance->geometry->ss_verts, faces[i].idx_verts[1]),
                sd_vec2_arr_get(instance->geometry->ss_verts, faces[i].idx_verts[2]),
            }, sizeof(vec2 [3]));

            for (int j = 0; j < 3; ++j) {
                vec3 curr = vs_verts[j];
                vec3 next = vs_verts[(j + 1) % 3];

                if (curr.z >= rasterizer->near)
                    clipped[nclipped++] = ss_verts[j];

                if ((curr.z < rasterizer->near) != (next.z < rasterizer->near)) {
                    vec3 intercept = intersect_near(curr, next, rasterizer->near);

                    sd_vec2 projected = rasterizer->project(self,
                        sd_vec3_set(intercept.x, intercept.y, rasterizer->near),
                        sd_vec2_set(canvas->width * 0.5f, canvas->height * 0.5f)
                    );

                    sd_vec2_scalar projected_scalar = sd_vec2_arr_get(&projected, 0);
                    SDL_memcpy(clipped + nclipped++, &projected_scalar, sizeof(vec2));
                }
            }

            /* Triangle fan clipped verticies */
            for (int j = 1; j < nclipped - 1; ++j) {
                /* Compute extremes */
                float min_x = SDL_min(clipped[0].x, SDL_min(clipped[j].x, clipped[j + 1].x));
                float max_x = SDL_max(clipped[0].x, SDL_max(clipped[j].x, clipped[j + 1].x));
                float min_y = SDL_min(clipped[0].y, SDL_min(clipped[j].y, clipped[j + 1].y));
                float max_y = SDL_max(clipped[0].y, SDL_max(clipped[j].y, clipped[j + 1].y));

                /* Cull off-screen triangles */
                if (min_x > canvas->width || max_x < 0 ||
                    min_y > bounds[1] || max_y < bounds[0] 
                ) continue;

                bool verts_cw = vec2_dot(
                    vec2_orthogonal(vec2_sub(clipped[j], clipped[0])),
                    vec2_sub(clipped[j + 1], clipped[0])
                ) > 0;

                if (flags & M7_RASTERIZER_CULL_BACKFACE && !verts_cw)
                    continue;;

                M7_TriangleDraw triangle = {
                    .shader_pipeline = instance->shader_pipeline,
                    .shader_states = instance->shader_states,
                    .nshaders = instance->nshaders
                };

                SDL_memcpy(triangle.vs_verts, (vec3 [3]) { vs_verts[0], vs_verts[1 + !verts_cw], vs_verts[1 + verts_cw] }, sizeof(vec3 [3]));
                SDL_memcpy(triangle.ss_verts, (vec2 [3]) { clipped[0], clipped[j + !verts_cw], clipped[j + verts_cw] }, sizeof(vec2 [3]));

                if (instance->geometry->vs_nrmls)
                    SDL_memcpy(triangle.vs_nrmls, (sd_vec3_scalar [3]) {
                        sd_vec3_arr_get(instance->geometry->vs_nrmls, faces[i].idx_verts[0]),
                        sd_vec3_arr_get(instance->geometry->vs_nrmls, faces[i].idx_verts[1 + !verts_cw]),
                        sd_vec3_arr_get(instance->geometry->vs_nrmls, faces[i].idx_verts[1 + verts_cw])
                    }, sizeof(vec3 [3]));

                if (instance->geometry->mesh->ts_verts)
                    SDL_memcpy(triangle.ts_verts, (vec2 [3]) {
                        instance->geometry->mesh->ts_verts[faces[i].idx_tverts[0]],
                        instance->geometry->mesh->ts_verts[faces[i].idx_tverts[1 + !verts_cw]],
                        instance->geometry->mesh->ts_verts[faces[i].idx_tverts[1 + verts_cw]]
                    }, sizeof(vec2 [3]));

                M7_Rasterizer_DrawTriangle(self, triangle, flags, scanlines, bounds);
            }
        }
    });
}

static int RenderToSubCanvas(void *data) {
    SubCanvasRenderData *render = data;
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(render->rasterizer, M7_Components.Rasterizer);
    M7_World *world = ECS_Entity_GetComponent(rasterizer->world, M7_Components.World);
    M7_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, M7_Components.Canvas);
    size_t sd_width = sd_bounding_size(canvas->width);

    /* Reset depth */
    for (size_t i = sd_width * render->bounds[0]; i < sd_width * render->bounds[1]; ++i)
        canvas->depth[i] = sd_float_zero();

    /* Draw geometry in batches, according to render order and rasterizer flags */
    for (size_t i = 0; i < List_Length(world->render_batches); ++i) {
        for (int flags = 0; flags < M7_RASTERIZER_FLAG_COMBINATIONS; ++flags) {
            List(M7_RenderInstance *) *flag_batch = List_Get(world->render_batches, i)[flags];

            if (flag_batch)
                M7_Rasterizer_DrawBatch(render->rasterizer, flag_batch, flags, render->scanlines, render->bounds);
        }
    }

    return 0;
}

void SD_VARIANT(M7_Rasterizer_Render)(ECS_Handle *self) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
    M7_World *world = ECS_Entity_GetComponent(rasterizer->world, M7_Components.World);
    M7_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, M7_Components.Canvas);

    /* Transform registered world geometry */
    List(M7_WorldGeometry *) *geometry = world->geometry;
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

        sd_vec3 sd_xform[3] = {
            sd_vec3_set(wg->xform.basis.x.x, wg->xform.basis.x.y, wg->xform.basis.x.z),
            sd_vec3_set(wg->xform.basis.y.x, wg->xform.basis.y.y, wg->xform.basis.y.z),
            sd_vec3_set(wg->xform.basis.z.x, wg->xform.basis.z.y, wg->xform.basis.z.z)
        };

        for (size_t i = 0; i < sd_count; ++i) {
            wg->vs_verts[i] = sd_vec3_fmadd(sd_xform[0], wg->mesh->ws_verts[i].x, translation);
            wg->vs_verts[i] = sd_vec3_fmadd(sd_xform[1], wg->mesh->ws_verts[i].y, wg->vs_verts[i]);
            wg->vs_verts[i] = sd_vec3_fmadd(sd_xform[2], wg->mesh->ws_verts[i].z, wg->vs_verts[i]);

            if (wg->vs_nrmls) {
                wg->vs_nrmls[i] = sd_vec3_muls(sd_xform[0], wg->mesh->ws_nrmls[i].x);
                wg->vs_nrmls[i] = sd_vec3_fmadd(sd_xform[1], wg->mesh->ws_nrmls[i].y, wg->vs_nrmls[i]);
                wg->vs_nrmls[i] = sd_vec3_fmadd(sd_xform[2], wg->mesh->ws_nrmls[i].z, wg->vs_nrmls[i]);
            }

            wg->ss_verts[i] = rasterizer->project(self, wg->vs_verts[i], sd_vec2_set(canvas->width * 0.5f, canvas->height * 0.5f));
        }
    });

    /* Render to sub-canvases in parallel */
    SDL_Thread **threads = SDL_malloc(sizeof(SDL_Thread *) * rasterizer->parallelism);
    SubCanvasRenderData *render_data = SDL_malloc(sizeof(SubCanvasRenderData) * rasterizer->parallelism);
    int (*scanlines)[2] = SDL_malloc(sizeof(int [2]) * canvas->height);

    int qot = canvas->height / rasterizer->parallelism;
    int rem = canvas->height % rasterizer->parallelism;

    for (int i = 0; i < rasterizer->parallelism; ++i) {
        render_data[i] = (SubCanvasRenderData) {
            .rasterizer = self,
            .bounds = {
                i * qot + SDL_min(i, rem),
                (i + 1) * qot + SDL_min(i + 1, rem)
            },
            .scanlines = scanlines
        };

        threads[i] = SDL_CreateThread(RenderToSubCanvas, "rendersc", render_data + i);
    }

    for (int i = 0; i < rasterizer->parallelism; ++i)
        SDL_WaitThread(threads[i], nullptr);

    SDL_free(scanlines);
    SDL_free(render_data);
    SDL_free(threads);
}

#ifndef SD_SRC_VARIANT

void M7_Rasterizer_Attach(ECS_Handle *self, ECS_Component(void) *component) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, component);
    rasterizer->world = ECS_Entity_AncestorWithComponent(self, M7_Components.World, true);
    rasterizer->target = ECS_Entity_AncestorWithComponent(self, M7_Components.Canvas, true);
}

void M7_Rasterizer_Init(void *component, void *args) {
    M7_Rasterizer *rasterizer = component;
    M7_RasterizerArgs *rasterizer_args = args;

    *rasterizer = (M7_Rasterizer) {
        .project = rasterizer_args->project,
        .scan = rasterizer_args->scan,
        .near = rasterizer_args->near,
        .parallelism = rasterizer_args->parallelism
    };
}

void M7_PerspectiveFOV_Set(ECS_Handle *self, float fov) {
    M7_PerspectiveFOV *perspective_fov = ECS_Entity_GetComponent(self, M7_Components.PerspectiveFOV);
    perspective_fov->fov = fov;
    perspective_fov->tan_half_fov = SDL_tanf(fov / 2);
}

void M7_PerspectiveFOV_Init(void *component, void *args) {
    M7_PerspectiveFOV *perspective_fov = component;
    float *fov = args;

    perspective_fov->fov = *fov;
    perspective_fov->tan_half_fov = SDL_tanf(*fov / 2);
}

#endif /* SD_SRC_VARIANT */
