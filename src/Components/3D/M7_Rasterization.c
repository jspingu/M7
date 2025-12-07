#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/M7_ECS.h>
#include <M7/Collections/List.h>
#include <M7/Math/linalg.h>
#include <M7/Math/stride.h>

#include "M7_3D_c.h"

typedef struct TriangleDispatcherData {
    SDL_Mutex *sync;
    SDL_Condition *wake;
    ECS_Handle *rasterizer;
    M7_RasterizerFlags flags;
    List(M7_TriangleDraw *) *triangles;
    M7_TriangleDraw **active_triangles;
    int id, ndispatchers;
} TriangleDispatcherData;

static inline int roundtl(float f) {
    return SDL_ceilf(f - 0.5f);
}

static inline vec3 intersect_near(vec3 from, vec3 to, float near) {
    vec3 path = vec3_sub(to, from);
    vec3 slope = vec3_div(path, path.z);
    return vec3_add(from, vec3_mul(slope, near - from.z));
}

sd_vec4 SD_VARIANT(first_shader)(ECS_Handle *self, sd_vec4 col, sd_vec3 vs, sd_vec3 nrml, sd_vec2 ts) {
    (void)self, (void)col, (void)ts;
    sd_float ambient = sd_float_set(0.01);
    sd_float energy = sd_float_set(10000);
    sd_vec3 surface_col = sd_vec3_set(0.9, 0.5, 0.5);

    sd_float sqrlen = sd_vec3_dot(vs, vs);
    sd_float rcpsql = sd_float_rcp(sqrlen);
    sd_float rcplen = sd_float_rsqrt(sqrlen);

    sd_float power = sd_float_mul(
        sd_vec3_dot(sd_vec3_mul(vs, rcplen), sd_vec3_negate(nrml)),
        sd_float_mul(energy, rcpsql)
    );

    sd_vec3 out = sd_vec3_mul(surface_col, sd_float_add(ambient, power));
    return (sd_vec4){ .rgb = out };
}

sd_vec4 SD_VARIANT(second_shader)(ECS_Handle *self, sd_vec4 col, sd_vec3 vs, sd_vec3 nrml, sd_vec2 ts) {
    (void)self, (void)vs, (void)nrml, (void)ts;
    col.g = sd_float_mul(col.g, sd_float_set(2));
    return col;
}

void SD_VARIANT(M7_ScanPerspective)(ECS_Handle *self, M7_TriangleDraw *triangle, int (*scanlines)[2], int range[2]) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
    M7_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, M7_Components.Canvas);

    sd_vec3 origin = sd_vec3_set(triangle->vs_verts[0].x, triangle->vs_verts[0].y, triangle->vs_verts[0].z);
    sd_vec3 ab = sd_vec3_sub(sd_vec3_set(triangle->vs_verts[1].x, triangle->vs_verts[1].y, triangle->vs_verts[1].z), origin);
    sd_vec3 ac = sd_vec3_sub(sd_vec3_set(triangle->vs_verts[2].x, triangle->vs_verts[2].y, triangle->vs_verts[2].z), origin);

    sd_vec3 nrml = sd_vec3_cross(ab, ac);
    sd_float inv_nrml_disp = sd_float_rcp(sd_vec3_dot(origin, nrml));

    sd_vec3 perp_ab = sd_vec3_cross(nrml, ab);
    sd_vec3 perp_ac = sd_vec3_cross(ac, nrml);

    sd_float inv_pgram_area = sd_float_rcp(sd_vec3_dot(ab, perp_ac));

    sd_vec2 inv_xform[3] = {
        sd_vec2_mul((sd_vec2) { .x = perp_ac.x, .y = perp_ab.x }, inv_pgram_area),
        sd_vec2_mul((sd_vec2) { .x = perp_ac.y, .y = perp_ab.y }, inv_pgram_area),
        sd_vec2_mul((sd_vec2) { .x = perp_ac.z, .y = perp_ab.z }, inv_pgram_area)
    };

    sd_vec3 origin_nrml = sd_vec3_set(triangle->vs_nrmls[0].x, triangle->vs_nrmls[0].y, triangle->vs_nrmls[0].z);
    sd_vec3 ab_nrml = sd_vec3_sub(sd_vec3_set(triangle->vs_nrmls[1].x, triangle->vs_nrmls[1].y, triangle->vs_nrmls[1].z), origin_nrml);
    sd_vec3 ac_nrml = sd_vec3_sub(sd_vec3_set(triangle->vs_nrmls[2].x, triangle->vs_nrmls[2].y, triangle->vs_nrmls[2].z), origin_nrml);

    sd_vec3 nrml_xform[3] = {
        sd_vec3_fmadd(ac_nrml, inv_xform[0].y, sd_vec3_mul(ab_nrml, inv_xform[0].x)),
        sd_vec3_fmadd(ac_nrml, inv_xform[1].y, sd_vec3_mul(ab_nrml, inv_xform[1].x)),
        sd_vec3_fmadd(ac_nrml, inv_xform[2].y, sd_vec3_mul(ab_nrml, inv_xform[2].x))
    };

    sd_vec2 midpoint = {
        .x = sd_float_set(canvas->width * 0.5f),
        .y = sd_float_set(canvas->height * 0.5f)
    };

    sd_float normalize_ss = sd_float_rcp(midpoint.x);

    for (int i = range[0]; i < range[1]; ++i) {
        int base = i * sd_bounding_size(canvas->width);
        int sd_left = scanlines[i][0] / SD_LENGTH;
        int sd_right = sd_bounding_size(scanlines[i][1]);

        for (int j = sd_left; j < sd_right; ++j) {
            int left = j * SD_LENGTH;
            sd_vec3 bg = canvas->color[base + j];

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

            sd_int mask = sd_float_clamp_mask(
                fragment_x,
                scanlines[i][0],
                scanlines[i][1]
            );

            /* Inverse z in depth buffer */
            sd_float bg_z = canvas->depth[base + j];
            mask = sd_int_and(mask, sd_float_gt(inv_z, bg_z));

            sd_vec4 col;
            List_ForEach(triangle->shader_pipeline, shader, col = shader(triangle->shader_state, col, fragment_vs, fragment_nrml, sd_vec2_zero()); );

            canvas->depth[base + j] = sd_float_mask_blend(bg_z, inv_z, mask);
            canvas->color[base + j] = sd_vec3_mask_blend(bg, col.rgb, mask);
        }
    }
}

static void Trace(int width, int height, int (*scanlines)[2], vec2 line[2]) {
    vec2 path = vec2_sub(line[1], line[0]);

    int trace_range[2] = {
        SDL_clamp(roundtl(line[0].y), 0, height),
        SDL_clamp(roundtl(line[1].y), 0, height)
    };

    bool descending = path.y > 0;
    float slope = path.x / path.y;
    float offset = line[!descending].x + (trace_range[!descending] + 0.5f - line[!descending].y) * slope;

    for (int i = trace_range[!descending]; i < trace_range[descending]; ++i) {
        scanlines[i][descending] = SDL_clamp(roundtl(offset), 0, width);
        offset += slope;
    }
}

static void M7_Rasterizer_DrawTriangle(ECS_Handle *self, M7_TriangleDraw *triangle, int (*scanlines)[2]) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
    M7_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, M7_Components.Canvas);

    float min_y = SDL_min(SDL_min(triangle->ss_verts[0].y, triangle->ss_verts[1].y), triangle->ss_verts[2].y);
    float max_y = SDL_max(SDL_max(triangle->ss_verts[0].y, triangle->ss_verts[1].y), triangle->ss_verts[2].y);

    int high = SDL_clamp(roundtl(min_y), 0, canvas->height);
    int low = SDL_clamp(roundtl(max_y), 0, canvas->height);

    for (int i = 0; i < 3; ++i)
        Trace(canvas->width, canvas->height, scanlines, (vec2 [2]) { triangle->ss_verts[i], triangle->ss_verts[(i + 1) % 3] });

    rasterizer->scan(self, triangle, scanlines, (int [2]) { high, low });
}

static M7_TriangleDraw *ExtractSafeTriangleDraw(List(M7_TriangleDraw *) *triangles, M7_TriangleDraw **active, int ndispatchers) {
    for (size_t i = List_Length(triangles); i > 0; --i) {
        M7_TriangleDraw *triangle = List_Get(triangles, i - 1);
        bool skip = false;

        for (int j = 0; !skip && j < ndispatchers; ++j)
            skip = active[j] && triangle->sd_bounding_box.left < active[j]->sd_bounding_box.right
                             && triangle->sd_bounding_box.right > active[j]->sd_bounding_box.left
                             && triangle->sd_bounding_box.top < active[j]->sd_bounding_box.bottom
                             && triangle->sd_bounding_box.bottom > active[j]->sd_bounding_box.top;

        if (!skip) {
            List_Remove(triangles, i - 1);
            return triangle;
        }
    }

    return nullptr;
}

static int DispatchTriangleDraws(void *data) {
    TriangleDispatcherData *dispatch = data;
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(dispatch->rasterizer, M7_Components.Rasterizer);
    M7_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, M7_Components.Canvas);
    int (*scanlines)[2] = SDL_malloc(sizeof(int [2]) * canvas->height);

    while (true) {
        SDL_LockMutex(dispatch->sync);
        
        do {
            if (!List_Length(dispatch->triangles)) {
                SDL_UnlockMutex(dispatch->sync);
                SDL_free(scanlines);
                return 0;
            }

            M7_TriangleDraw *safe_triangle = ExtractSafeTriangleDraw(dispatch->triangles, dispatch->active_triangles, dispatch->ndispatchers);

            if (safe_triangle)
                dispatch->active_triangles[dispatch->id] = safe_triangle;
            else
                SDL_WaitCondition(dispatch->wake, dispatch->sync);
        } while (!dispatch->active_triangles[dispatch->id]);

        SDL_UnlockMutex(dispatch->sync);

        M7_Rasterizer_DrawTriangle(dispatch->rasterizer, dispatch->active_triangles[dispatch->id], scanlines);

        SDL_LockMutex(dispatch->sync);
        SDL_free(dispatch->active_triangles[dispatch->id]);
        dispatch->active_triangles[dispatch->id] = nullptr;
        SDL_BroadcastCondition(dispatch->wake);
        SDL_UnlockMutex(dispatch->sync);
    }
}

static void M7_Rasterizer_DrawBatch(ECS_Handle *self, M7_RasterizerFlags flags, List(M7_RenderInstance *) *batch) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
    M7_Canvas *canvas = ECS_Entity_GetComponent(rasterizer->target, M7_Components.Canvas);
    List(M7_TriangleDraw *) *triangles = List_Create(M7_TriangleDraw *);

    /* Queue up triangle draws */
    List_ForEach(batch, instance, {
        M7_MeshFace *faces = instance->geometry->mesh->faces;
        size_t nfaces = instance->geometry->mesh->nfaces;

        for (size_t i = 0; i < nfaces; ++i) {
            /* Cull backface or correct the vertex ordering */
            vec3 vs_verts[3];

            SDL_memcpy(vs_verts, &(sd_vec3_scalar [3]) {
                sd_vec3_arr_get(instance->geometry->vs_verts, faces[i].idx_verts[0]),
                sd_vec3_arr_get(instance->geometry->vs_verts, faces[i].idx_verts[1]),
                sd_vec3_arr_get(instance->geometry->vs_verts, faces[i].idx_verts[2])
            }, sizeof(vec3 [3]));

            vec3 nrml = vec3_cross(vec3_sub(vs_verts[1], vs_verts[0]), vec3_sub(vs_verts[2], vs_verts[0]));
            bool verts_cw = vec3_dot(vs_verts[0], nrml) < 0;

            if (flags & M7_RASTERIZER_CULL_BACKFACE && !verts_cw)
                continue;

            SDL_memcpy(vs_verts, (vec3 [3]) { vs_verts[0], vs_verts[1 + !verts_cw], vs_verts[1 + verts_cw] }, sizeof(vec3 [3]));

            /* Perform near plane clipping */
            vec2 ss_verts[3];
            vec2 clipped[4];
            int nclipped = 0;

            SDL_memcpy(ss_verts, (sd_vec2_scalar [3]) {
                sd_vec2_arr_get(instance->geometry->ss_verts, faces[i].idx_verts[0]),
                sd_vec2_arr_get(instance->geometry->ss_verts, faces[i].idx_verts[1 + !verts_cw]),
                sd_vec2_arr_get(instance->geometry->ss_verts, faces[i].idx_verts[1 + verts_cw]),
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
                    min_y > canvas->height || max_y < 0 
                ) continue;

                M7_TriangleDraw *triangle = SDL_malloc(sizeof(M7_TriangleDraw));
                triangle->shader_pipeline = instance->shader_pipeline;
                triangle->shader_state = instance->shader_state;
                SDL_memcpy(triangle->vs_verts, vs_verts, sizeof(vec3 [3]));
                SDL_memcpy(triangle->ss_verts, (vec2 [3]) { clipped[0], clipped[j], clipped[j + 1] }, sizeof(vec2 [3]));

                SDL_memcpy(triangle->vs_nrmls, (sd_vec3_scalar [3]) {
                    sd_vec3_arr_get(instance->geometry->vs_nrmls, faces[i].idx_verts[0]),
                    sd_vec3_arr_get(instance->geometry->vs_nrmls, faces[i].idx_verts[1 + !verts_cw]),
                    sd_vec3_arr_get(instance->geometry->vs_nrmls, faces[i].idx_verts[1 + verts_cw])
                }, sizeof(vec3 [3]));

                /* Compute strided bounding box */
                triangle->sd_bounding_box.left = roundtl(min_x) / SD_LENGTH;
                triangle->sd_bounding_box.right = sd_bounding_size(roundtl(max_x));
                triangle->sd_bounding_box.top = roundtl(min_y);
                triangle->sd_bounding_box.bottom = roundtl(max_y);

                List_Push(triangles, triangle);
            }
        }
    });

    /* TODO: Triangle sort here, if the flag is set */

    /* Dispatch triangle draws with multithreading */
    SDL_Thread **threads = SDL_malloc(sizeof(SDL_Thread *) * rasterizer->parallelism);
    SDL_Mutex *sync = SDL_CreateMutex();
    SDL_Condition *wake = SDL_CreateCondition();
    M7_TriangleDraw **active_triangles = SDL_malloc(sizeof(M7_TriangleDraw *) * rasterizer->parallelism);
    TriangleDispatcherData *dispatcher_data = SDL_malloc(sizeof(TriangleDispatcherData) * rasterizer->parallelism);

    for (int i = 0; i < rasterizer->parallelism; ++i) {
        active_triangles[i] = nullptr;

        dispatcher_data[i] = (TriangleDispatcherData) {
            .sync = sync,
            .wake = wake,
            .rasterizer = self,
            .flags = flags,
            .triangles = triangles,
            .active_triangles = active_triangles,
            .id = i,
            .ndispatchers = rasterizer->parallelism
        };
    }

    for (int i = 0; i < rasterizer->parallelism; ++i)
        threads[i] = SDL_CreateThread(DispatchTriangleDraws, "triangles", dispatcher_data + i);

    for (int i = 0; i < rasterizer->parallelism; ++i)
        SDL_WaitThread(threads[i], nullptr);

    SDL_free(dispatcher_data);
    SDL_free(active_triangles);
    SDL_DestroyMutex(sync);
    SDL_DestroyCondition(wake);
    SDL_free(threads);

    List_Free(triangles);
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
            sd_vec3 vs_vert = sd_vec3_fmadd(sd_xform[0], wg->mesh->ws_verts[i].x, translation);
                    vs_vert = sd_vec3_fmadd(sd_xform[1], wg->mesh->ws_verts[i].y, vs_vert);
                    vs_vert = sd_vec3_fmadd(sd_xform[2], wg->mesh->ws_verts[i].z, vs_vert);

            sd_vec3 vs_nrml = sd_vec3_mul(sd_xform[0], wg->mesh->ws_nrmls[i].x);
                    vs_nrml = sd_vec3_fmadd(sd_xform[1], wg->mesh->ws_nrmls[i].y, vs_nrml);
                    vs_nrml = sd_vec3_fmadd(sd_xform[2], wg->mesh->ws_nrmls[i].z, vs_nrml);

            wg->vs_verts[i] = vs_vert;
            wg->vs_nrmls[i] = vs_nrml;
            wg->ss_verts[i] = rasterizer->project(self, vs_vert, sd_vec2_set(canvas->width * 0.5f, canvas->height * 0.5f));
        }
    });

    /* Clear canvas */
    for (size_t i = 0; i < sd_bounding_size(canvas->width) * canvas->height; ++i) {
        canvas->color[i] = sd_vec3_zero();
        canvas->depth[i] = sd_float_zero();
    }

    /* Draw geometry in batches, according to render order and rasterizer flags */
    for (size_t i = 0; i < List_Length(world->render_batches); ++i) {
        for (int flags = 0; flags < M7_RASTERIZER_FLAG_COMBINATIONS; ++flags) {
            List(M7_RenderInstance *) *flag_batch = List_Get(world->render_batches, i)[flags];

            if (flag_batch)
                M7_Rasterizer_DrawBatch(self, flags, flag_batch);
        }
    }
}

#ifdef SD_BASE

void M7_Rasterizer_Attach(ECS_Handle *self) {
    M7_Rasterizer *rasterizer = ECS_Entity_GetComponent(self, M7_Components.Rasterizer);
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

#endif /* SD_BASE */
