#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/TX_ECS.h>
#include <TX/Collections/List.h>
#include <TX/Math/linalg.h>
#include <TX/Math/stride.h>

#include "TX_3D_c.h"

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

void SD_VARIANT(TX_ScanLinear)(ECS_Handle *self, TX_TriangleDraw triangle, TX_RasterizerFlags flags, int (*scanlines)[2], int range[2]) {
    TX_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, TX_Components.Rasterizer);
    TX_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, TX_Components.Canvas);
    xform3 scalar_vs2ws_xform = TX_Entity_GetXform(self);

    sd_vec3 vs2ws_xform_i = sd_vec3_set(scalar_vs2ws_xform.basis.x.x, scalar_vs2ws_xform.basis.x.y, scalar_vs2ws_xform.basis.x.z);
    sd_vec3 vs2ws_xform_j = sd_vec3_set(scalar_vs2ws_xform.basis.y.x, scalar_vs2ws_xform.basis.y.y, scalar_vs2ws_xform.basis.y.z);
    sd_vec3 vs2ws_xform_k = sd_vec3_set(scalar_vs2ws_xform.basis.z.x, scalar_vs2ws_xform.basis.z.y, scalar_vs2ws_xform.basis.z.z);

    sd_vec2 origin = sd_vec2_set(triangle.ss_verts[0].x ,triangle.ss_verts[0].y);
    sd_vec2 ab = sd_vec2_sub(sd_vec2_set(triangle.ss_verts[1].x, triangle.ss_verts[1].y), origin);
    sd_vec2 ac = sd_vec2_sub(sd_vec2_set(triangle.ss_verts[2].x, triangle.ss_verts[2].y), origin);

    sd_float inv_disc = sd_float_rcp(sd_float_sub(sd_float_mul(sd_vx(ab), sd_vy(ac)), sd_float_mul(sd_vy(ab), sd_vx(ac))));

    sd_vec2 inv_xform_i = sd_vec2_muls(sd_vec2_create(sd_vy(ac), sd_float_negate(sd_vy(ab))), inv_disc);
    sd_vec2 inv_xform_j = sd_vec2_muls(sd_vec2_create(sd_vx(ac), sd_float_negate(sd_vx(ab))), inv_disc);

    sd_vec3 origin_vs = sd_vec3_set(triangle.vs_verts[0].x, triangle.vs_verts[0].y, triangle.vs_verts[0].z);
    sd_vec3 ab_vs = sd_vec3_sub(sd_vec3_set(triangle.vs_verts[1].x, triangle.vs_verts[1].y, triangle.vs_verts[1].z), origin_vs);
    sd_vec3 ac_vs = sd_vec3_sub(sd_vec3_set(triangle.vs_verts[2].x, triangle.vs_verts[2].y, triangle.vs_verts[2].z), origin_vs);

    sd_vec3 vs_xform_i = sd_vec3_fsmadd(ac_vs, sd_vy(inv_xform_i), sd_vec3_muls(ab_vs, sd_vx(inv_xform_i)));
    sd_vec3 vs_xform_j = sd_vec3_fsmadd(ac_vs, sd_vy(inv_xform_j), sd_vec3_muls(ab_vs, sd_vx(inv_xform_j)));

    sd_vec3 origin_nrml = sd_vec3_set(triangle.vs_nrmls[0].x, triangle.vs_nrmls[0].y, triangle.vs_nrmls[0].z);
    sd_vec3 ab_nrml = sd_vec3_sub(sd_vec3_set(triangle.vs_nrmls[1].x, triangle.vs_nrmls[1].y, triangle.vs_nrmls[1].z), origin_nrml);
    sd_vec3 ac_nrml = sd_vec3_sub(sd_vec3_set(triangle.vs_nrmls[2].x, triangle.vs_nrmls[2].y, triangle.vs_nrmls[2].z), origin_nrml);

    sd_vec3 nrml_xform_i = sd_vec3_fsmadd(ac_nrml, sd_vy(inv_xform_i), sd_vec3_muls(ab_nrml, sd_vx(inv_xform_i)));
    sd_vec3 nrml_xform_j = sd_vec3_fsmadd(ac_nrml, sd_vy(inv_xform_j), sd_vec3_muls(ab_nrml, sd_vx(inv_xform_j)));

    sd_vec2 origin_ts = sd_vec2_set(triangle.ts_verts[0].x, triangle.ts_verts[0].y);
    sd_vec2 ab_ts = sd_vec2_sub(sd_vec2_set(triangle.ts_verts[1].x, triangle.ts_verts[1].y), origin_ts);
    sd_vec2 ac_ts = sd_vec2_sub(sd_vec2_set(triangle.ts_verts[2].x, triangle.ts_verts[2].y), origin_ts);
    
    sd_vec2 ts_xform_i = sd_vec2_fsmadd(ac_ts, sd_vy(inv_xform_i), sd_vec2_muls(ab_ts, sd_vx(inv_xform_i)));
    sd_vec2 ts_xform_j = sd_vec2_fsmadd(ac_ts, sd_vy(inv_xform_j), sd_vec2_muls(ab_ts, sd_vx(inv_xform_j)));

    vec3 scalar_nrml = vec3_cross(vec3_sub(triangle.vs_verts[1], triangle.vs_verts[0]), vec3_sub(triangle.vs_verts[2], triangle.vs_verts[0]));
    sd_vec3 nrml = sd_vec3_set(scalar_nrml.x, scalar_nrml.y, scalar_nrml.z);

    for (int i = range[0]; i < range[1]; ++i) {
        int base = i * sd_bounding_length(canvas->width);
        int sd_left = sd_qot(scanlines[i][0]);
        int sd_right = sd_bounding_length(scanlines[i][1]);

        for (int j = sd_left; j < sd_right; ++j) {
            int left = j * sd_length();

            sd_vec2 ss = sd_vec2_create(
                sd_float_add(sd_float_set(left), sd_float_add(sd_float_range(), sd_float_set(0.5f))),
                sd_float_add(sd_float_set(i), sd_float_set(0.5f))
            );

            sd_vec2 relative = sd_vec2_sub(ss, origin);

            sd_vec3 fragment_vs = sd_vec3_fsmadd(vs_xform_i, sd_vx(relative), origin_vs);
                    fragment_vs = sd_vec3_fsmadd(vs_xform_j, sd_vy(relative), fragment_vs);

            sd_float inv_z = sd_float_rcp(sd_vz(fragment_vs));
            sd_vec3 fragment_nrml;

            if (flags & TX_RASTERIZER_INTERPOLATE_NORMALS) {
                fragment_nrml = sd_vec3_fsmadd(nrml_xform_i, sd_vx(relative), origin_nrml);
                fragment_nrml = sd_vec3_fsmadd(nrml_xform_j, sd_vy(relative), fragment_nrml);
            } else fragment_nrml = nrml;

            fragment_nrml = sd_vec3_normalize(fragment_nrml);

            sd_vec2 fragment_ts = sd_vec2_fsmadd(ts_xform_i, sd_vx(relative), origin_ts);
                    fragment_ts = sd_vec2_fsmadd(ts_xform_j, sd_vy(relative), fragment_ts);

            TX_ShaderParams fragment = {
                .vs = &fragment_vs,
                .nrml = &fragment_nrml,
                .ts = &fragment_ts,
                .vs2ws_xform = { &vs2ws_xform_i, &vs2ws_xform_j, &vs2ws_xform_k }
            };

            sd_vec4 col;
            sd_vec3 bg = sd_vec3_load(canvas->color, base + j);
            sd_float bg_z = sd_float_load(canvas->depth, base + j);
            sd_mask mask = sd_float_between(sd_vx(ss), scanlines[i][0], scanlines[i][1]);

            if (flags & TX_RASTERIZER_TEST_DEPTH)
                mask = sd_mask_and(mask, sd_float_gt(inv_z, bg_z));

            if (flags & TX_RASTERIZER_WRITE_DEPTH)
                sd_float_store(canvas->depth, base + j, sd_float_mask_blend(bg_z, inv_z, mask));

            for (size_t i = 0; i < triangle.nshaders; ++i)
                col = triangle.shader_pipeline[i](triangle.shader_states[i], col, fragment);

            sd_vec3_store(canvas->color, base + j, sd_vec3_mask_blend(bg, sd_vxyz(col), mask));
        }
    }
}

void SD_VARIANT(TX_ScanPerspective)(ECS_Handle *self, TX_TriangleDraw triangle, TX_RasterizerFlags flags, int (*scanlines)[2], int range[2]) {
    TX_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, TX_Components.Rasterizer);
    TX_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, TX_Components.Canvas);
    TX_PerspectiveFOV *perspective_fov = ECS_Entity_GetComponent(self, TX_Components.PerspectiveFOV);
    xform3 scalar_vs2ws_xform = TX_Entity_GetXform(self);

    sd_vec3 vs2ws_xform_i = sd_vec3_set(scalar_vs2ws_xform.basis.x.x, scalar_vs2ws_xform.basis.x.y, scalar_vs2ws_xform.basis.x.z);
    sd_vec3 vs2ws_xform_j = sd_vec3_set(scalar_vs2ws_xform.basis.y.x, scalar_vs2ws_xform.basis.y.y, scalar_vs2ws_xform.basis.y.z);
    sd_vec3 vs2ws_xform_k = sd_vec3_set(scalar_vs2ws_xform.basis.z.x, scalar_vs2ws_xform.basis.z.y, scalar_vs2ws_xform.basis.z.z);

    sd_vec3 origin = sd_vec3_set(triangle.vs_verts[0].x, triangle.vs_verts[0].y, triangle.vs_verts[0].z);
    sd_vec3 ab = sd_vec3_sub(sd_vec3_set(triangle.vs_verts[1].x, triangle.vs_verts[1].y, triangle.vs_verts[1].z), origin);
    sd_vec3 ac = sd_vec3_sub(sd_vec3_set(triangle.vs_verts[2].x, triangle.vs_verts[2].y, triangle.vs_verts[2].z), origin);

    sd_vec3 nrml = sd_vec3_cross(ab, ac);
    sd_float inv_nrml_disp = sd_float_rcp(sd_vec3_dot(origin, nrml));

    sd_vec3 perp_ab = sd_vec3_cross(nrml, ab);
    sd_vec3 perp_ac = sd_vec3_cross(ac, nrml);

    sd_float inv_pgram_area = sd_float_rcp(sd_vec3_dot(ab, perp_ac));

    sd_vec2 inv_xform_i = sd_vec2_muls(sd_vec2_create(sd_vx(perp_ac), sd_vx(perp_ab)), inv_pgram_area);
    sd_vec2 inv_xform_j = sd_vec2_muls(sd_vec2_create(sd_vy(perp_ac), sd_vy(perp_ab)), inv_pgram_area);
    sd_vec2 inv_xform_k = sd_vec2_muls(sd_vec2_create(sd_vz(perp_ac), sd_vz(perp_ab)), inv_pgram_area);

    sd_vec3 origin_nrml = sd_vec3_set(triangle.vs_nrmls[0].x, triangle.vs_nrmls[0].y, triangle.vs_nrmls[0].z);
    sd_vec3 ab_nrml = sd_vec3_sub(sd_vec3_set(triangle.vs_nrmls[1].x, triangle.vs_nrmls[1].y, triangle.vs_nrmls[1].z), origin_nrml);
    sd_vec3 ac_nrml = sd_vec3_sub(sd_vec3_set(triangle.vs_nrmls[2].x, triangle.vs_nrmls[2].y, triangle.vs_nrmls[2].z), origin_nrml);

    sd_vec3 nrml_xform_i = sd_vec3_fsmadd(ac_nrml, sd_vy(inv_xform_i), sd_vec3_muls(ab_nrml, sd_vx(inv_xform_i)));
    sd_vec3 nrml_xform_j = sd_vec3_fsmadd(ac_nrml, sd_vy(inv_xform_j), sd_vec3_muls(ab_nrml, sd_vx(inv_xform_j)));
    sd_vec3 nrml_xform_k = sd_vec3_fsmadd(ac_nrml, sd_vy(inv_xform_k), sd_vec3_muls(ab_nrml, sd_vx(inv_xform_k)));

    sd_vec2 origin_ts = sd_vec2_set(triangle.ts_verts[0].x, triangle.ts_verts[0].y);
    sd_vec2 ab_ts = sd_vec2_sub(sd_vec2_set(triangle.ts_verts[1].x, triangle.ts_verts[1].y), origin_ts);
    sd_vec2 ac_ts = sd_vec2_sub(sd_vec2_set(triangle.ts_verts[2].x, triangle.ts_verts[2].y), origin_ts);

    sd_vec2 ts_xform_i = sd_vec2_fsmadd(ac_ts, sd_vy(inv_xform_i), sd_vec2_muls(ab_ts, sd_vx(inv_xform_i)));
    sd_vec2 ts_xform_j = sd_vec2_fsmadd(ac_ts, sd_vy(inv_xform_j), sd_vec2_muls(ab_ts, sd_vx(inv_xform_j)));
    sd_vec2 ts_xform_k = sd_vec2_fsmadd(ac_ts, sd_vy(inv_xform_k), sd_vec2_muls(ab_ts, sd_vx(inv_xform_k)));

    sd_vec2 midpoint = sd_vec2_set(canvas->width * 0.5f, canvas->height * 0.5f);
    sd_float normalize_ss = sd_float_mul(sd_float_set(perspective_fov->tan_half_fov), sd_float_rcp(sd_vx(midpoint)));

    for (int i = range[0]; i < range[1]; ++i) {
        int base = i * sd_bounding_length(canvas->width);
        int sd_left = sd_qot(scanlines[i][0]);
        int sd_right = sd_bounding_length(scanlines[i][1]);

        for (int j = sd_left; j < sd_right; ++j) {
            int left = j * sd_length();

            sd_float fragment_x = sd_float_add(sd_float_range(), sd_float_set(0.5));
                     fragment_x = sd_float_add(fragment_x, sd_float_set(left));

            sd_float fragment_y = sd_float_add(sd_float_set(i), sd_float_set(0.5));

            sd_vec2 proj_plane = sd_vec2_muls(sd_vec2_sub(
                sd_vec2_create(fragment_x, sd_vy(midpoint)),
                sd_vec2_create(sd_vx(midpoint), fragment_y)
            ), normalize_ss);

            sd_float inv_z = sd_float_mul(sd_vec3_dot(sd_vec3_create(
                sd_vx(proj_plane),
                sd_vy(proj_plane),
                sd_float_one()
            ), nrml), inv_nrml_disp);

            sd_float fragment_z = sd_float_rcp(inv_z);

            sd_vec3 fragment_vs = sd_vec3_create(
                sd_float_mul(sd_vx(proj_plane), fragment_z),
                sd_float_mul(sd_vy(proj_plane), fragment_z),
                fragment_z
            );

            sd_vec3 relative = sd_vec3_sub(fragment_vs, origin);
            sd_vec3 fragment_nrml;

            if (flags & TX_RASTERIZER_INTERPOLATE_NORMALS) {
                fragment_nrml = sd_vec3_fsmadd(nrml_xform_i, sd_vx(relative), origin_nrml);
                fragment_nrml = sd_vec3_fsmadd(nrml_xform_j, sd_vy(relative), fragment_nrml);
                fragment_nrml = sd_vec3_fsmadd(nrml_xform_k, sd_vz(relative), fragment_nrml);
            } else fragment_nrml = nrml;

            fragment_nrml = sd_vec3_normalize(fragment_nrml);

            sd_vec2 fragment_ts = sd_vec2_fsmadd(ts_xform_i, sd_vx(relative), origin_ts);
                    fragment_ts = sd_vec2_fsmadd(ts_xform_j, sd_vy(relative), fragment_ts);
                    fragment_ts = sd_vec2_fsmadd(ts_xform_k, sd_vz(relative), fragment_ts);

            TX_ShaderParams fragment = {
                .vs = &fragment_vs,
                .nrml = &fragment_nrml,
                .ts = &fragment_ts,
                .vs2ws_xform = { &vs2ws_xform_i, &vs2ws_xform_j, &vs2ws_xform_k }
            };

            sd_vec4 col;
            sd_vec3 bg = sd_vec3_load(canvas->color, base + j);
            sd_float bg_z = sd_float_load(canvas->depth, base + j);
            sd_mask mask = sd_float_between(fragment_x, scanlines[i][0], scanlines[i][1]);

            if (flags & TX_RASTERIZER_TEST_DEPTH)
                mask = sd_mask_and(mask, sd_float_gt(inv_z, bg_z));

            if (flags & TX_RASTERIZER_WRITE_DEPTH)
                sd_float_store(canvas->depth, base + j, sd_float_mask_blend(bg_z, inv_z, mask));

            for (size_t i = 0; i < triangle.nshaders; ++i)
                col = triangle.shader_pipeline[i](triangle.shader_states[i], col, fragment);

            sd_vec3_store(canvas->color, base + j, sd_vec3_mask_blend(bg, sd_vxyz(col), mask));
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

    // TODO: Remove iterative algorithm & vectorize
}

static void TX_Rasterizer_DrawTriangle(ECS_Handle *self, TX_TriangleDraw triangle, TX_RasterizerFlags flags, int (*scanlines)[2], int bounds[2]) {
    TX_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, TX_Components.Rasterizer);
    TX_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, TX_Components.Canvas);

    float min_y = SDL_min(SDL_min(triangle.ss_verts[0].y, triangle.ss_verts[1].y), triangle.ss_verts[2].y);
    float max_y = SDL_max(SDL_max(triangle.ss_verts[0].y, triangle.ss_verts[1].y), triangle.ss_verts[2].y);

    int high = SDL_clamp(roundtl(min_y), bounds[0], bounds[1]);
    int low = SDL_clamp(roundtl(max_y), bounds[0], bounds[1]);

    for (int i = 0; i < 3; ++i)
        Trace(canvas->width, bounds, scanlines, (vec2 [2]) { triangle.ss_verts[i], triangle.ss_verts[(i + 1) % 3] });

    rasterizer->scan(self, triangle, flags, scanlines, (int [2]) { high, low });
}

static void TX_Rasterizer_DrawBatch(ECS_Handle *self, List(TX_RenderInstance *) *batch, TX_RasterizerFlags flags, int (*scanlines)[2], int bounds[2]) {
    TX_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, TX_Components.Rasterizer);
    TX_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, TX_Components.Canvas);

    /* Draw triangles */
    List_ForEach(batch, instance, {
        TX_MeshFace *faces = instance->geometry->mesh->faces;
        size_t nfaces = instance->geometry->mesh->nfaces;

        for (size_t i = 0; i < nfaces; ++i) {
            vec3 vs_verts[3];

            SDL_memcpy(vs_verts, &(sd_vec3_scalar [3]) {
                sd_vec3_loads(instance->geometry->vs_verts, faces[i].idx_verts[0]),
                sd_vec3_loads(instance->geometry->vs_verts, faces[i].idx_verts[1]),
                sd_vec3_loads(instance->geometry->vs_verts, faces[i].idx_verts[2])
            }, sizeof(vec3 [3]));

            /* Perform near plane clipping */
            vec2 ss_verts[3];
            vec2 clipped[4];
            int nclipped = 0;

            SDL_memcpy(ss_verts, (sd_vec2_scalar [3]) {
                sd_vec2_loads(instance->geometry->ss_verts, faces[i].idx_verts[0]),
                sd_vec2_loads(instance->geometry->ss_verts, faces[i].idx_verts[1]),
                sd_vec2_loads(instance->geometry->ss_verts, faces[i].idx_verts[2]),
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

                    sd_vec2_scalar projected_scalar = sd_vec2_loads(&projected, 0);
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

                if (flags & TX_RASTERIZER_CULL_BACKFACE && !verts_cw)
                    continue;

                TX_TriangleDraw triangle = {
                    .shader_pipeline = instance->shader_pipeline,
                    .shader_states = instance->shader_states,
                    .nshaders = instance->nshaders
                };

                SDL_memcpy(triangle.vs_verts, (vec3 [3]) { vs_verts[0], vs_verts[1 + !verts_cw], vs_verts[1 + verts_cw] }, sizeof(vec3 [3]));
                SDL_memcpy(triangle.ss_verts, (vec2 [3]) { clipped[0], clipped[j + !verts_cw], clipped[j + verts_cw] }, sizeof(vec2 [3]));

                if (instance->geometry->vs_nrmls)
                    SDL_memcpy(triangle.vs_nrmls, (sd_vec3_scalar [3]) {
                        sd_vec3_loads(instance->geometry->vs_nrmls, faces[i].idx_verts[0]),
                        sd_vec3_loads(instance->geometry->vs_nrmls, faces[i].idx_verts[1 + !verts_cw]),
                        sd_vec3_loads(instance->geometry->vs_nrmls, faces[i].idx_verts[1 + verts_cw])
                    }, sizeof(vec3 [3]));

                if (instance->geometry->mesh->ts_verts)
                    SDL_memcpy(triangle.ts_verts, (vec2 [3]) {
                        instance->geometry->mesh->ts_verts[faces[i].idx_tverts[0]],
                        instance->geometry->mesh->ts_verts[faces[i].idx_tverts[1 + !verts_cw]],
                        instance->geometry->mesh->ts_verts[faces[i].idx_tverts[1 + verts_cw]]
                    }, sizeof(vec2 [3]));

                TX_Rasterizer_DrawTriangle(self, triangle, flags, scanlines, bounds);
            }
        }
    });
}

static int RenderToSubCanvas(void *data) {
    SubCanvasRenderData *render = data;
    TX_Rasterizer *rasterizer = ECS_Entity_GetComponent(render->rasterizer, TX_Components.Rasterizer);
    TX_World *world = ECS_Entity_GetComponent(rasterizer->world, TX_Components.World);
    TX_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, TX_Components.Canvas);
    size_t sd_width = sd_bounding_length(canvas->width);

    /* Reset depth */
    for (size_t i = sd_width * render->bounds[0]; i < sd_width * render->bounds[1]; ++i)
        sd_float_store(canvas->depth, i, sd_float_zero());

    /* Draw geometry in batches, according to render order and rasterizer flags */
    for (size_t i = 0; i < List_Length(world->render_batches); ++i) {
        for (int flags = 0; flags < TX_RASTERIZER_FLAG_COMBINATIONS; ++flags) {
            List(TX_RenderInstance *) *flag_batch = List_Get(world->render_batches, i)[flags];

            if (flag_batch)
                TX_Rasterizer_DrawBatch(render->rasterizer, flag_batch, flags, render->scanlines, render->bounds);
        }
    }

    return 0;
}

void SD_VARIANT(TX_Rasterizer_Render)(ECS_Handle *self) {
    TX_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, TX_Components.Rasterizer);
    TX_World *world = ECS_Entity_GetComponent(rasterizer->world, TX_Components.World);
    TX_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, TX_Components.Canvas);

    /* Transform registered world geometry */
    List(TX_WorldGeometry *) *geometry = world->geometry;
    xform3 cam_xform = TX_Entity_GetXform(self);

    xform3 ws2vs_xform = {
        mat3x3_xpose(cam_xform.basis),
        vec3_mul(mat3x3_mul(mat3x3_xpose(cam_xform.basis), cam_xform.translation), -1)
    };

    TX_Entity_Xform(rasterizer->world, ws2vs_xform);

    List_ForEach(geometry, wg, {
        size_t sd_count = sd_bounding_length(wg->mesh->nverts);

        sd_vec3 translation = sd_vec3_set(
            wg->xform.translation.x,
            wg->xform.translation.y,
            wg->xform.translation.z
        );

        sd_vec3 sd_xform_i = sd_vec3_set(wg->xform.basis.x.x, wg->xform.basis.x.y, wg->xform.basis.x.z);
        sd_vec3 sd_xform_j = sd_vec3_set(wg->xform.basis.y.x, wg->xform.basis.y.y, wg->xform.basis.y.z);
        sd_vec3 sd_xform_k = sd_vec3_set(wg->xform.basis.z.x, wg->xform.basis.z.y, wg->xform.basis.z.z);

        for (size_t i = 0; i < sd_count; ++i) {
            sd_vec3 ws_vert = sd_vec3_load(wg->mesh->ws_verts, i);

            sd_vec3 vs_vert = sd_vec3_fsmadd(sd_xform_i, sd_vx(ws_vert), translation);
                    vs_vert = sd_vec3_fsmadd(sd_xform_j, sd_vy(ws_vert), vs_vert);
                    vs_vert = sd_vec3_fsmadd(sd_xform_k, sd_vz(ws_vert), vs_vert);

            sd_vec3_store(wg->vs_verts, i, vs_vert);

            if (wg->vs_nrmls) {
                sd_vec3 ws_nrml = sd_vec3_load(wg->mesh->ws_nrmls, i);

                sd_vec3 vs_nrml = sd_vec3_muls(sd_xform_i, sd_vx(ws_nrml));
                        vs_nrml = sd_vec3_fsmadd(sd_xform_j, sd_vy(ws_nrml), vs_nrml);
                        vs_nrml = sd_vec3_fsmadd(sd_xform_k, sd_vz(ws_nrml), vs_nrml);

                sd_vec3_store(wg->vs_nrmls, i, sd_vec3_normalize(vs_nrml));
            }

            sd_vec2_store(wg->ss_verts, i, rasterizer->project(self, vs_vert, sd_vec2_set(canvas->width * 0.5f, canvas->height * 0.5f)));
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

void TX_Rasterizer_Attach(ECS_Handle *self, ECS_Component(void) *component) {
    TX_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, component);
    rasterizer->world = ECS_Entity_AncestorWithComponent(self, TX_Components.World, true);
    rasterizer->target = ECS_Entity_AncestorWithComponent(self, TX_Components.Canvas, true);
}

void TX_Rasterizer_Init(void *component, void *args) {
    TX_Rasterizer *rasterizer = component;
    TX_RasterizerArgs *rasterizer_args = args;

    *rasterizer = (TX_Rasterizer) {
        .project = rasterizer_args->project,
        .scan = rasterizer_args->scan,
        .near = rasterizer_args->near,
        .parallelism = rasterizer_args->parallelism
    };
}

void TX_PerspectiveFOV_Set(ECS_Handle *self, float fov) {
    TX_PerspectiveFOV *perspective_fov = ECS_Entity_GetComponent(self, TX_Components.PerspectiveFOV);
    perspective_fov->fov = fov;
    perspective_fov->tan_half_fov = SDL_tanf(fov / 2);
}

void TX_PerspectiveFOV_Init(void *component, void *args) {
    TX_PerspectiveFOV *perspective_fov = component;
    float *fov = args;

    perspective_fov->fov = *fov;
    perspective_fov->tan_half_fov = SDL_tanf(*fov / 2);
}

#endif /* SD_SRC_VARIANT */
