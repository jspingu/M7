#include <stddef.h>
#include <SDL3/SDL.h>
#include <M7/Collections/List.h>
#include <M7/Collections/Bitset.h>
#include <M7/ECS.h>

#include "ECS_c.h"

static size_t ECS_ConstructionLength(ECS_Construction c) {
    size_t length = 1;

    for (size_t i = 0; i < c.nchildren; ++i)
        length += ECS_ConstructionLength(c.children[i]);

    return length;
}

static void ECS_ComponentConstructionFree(ECS_ComponentConstruction cc) {
    ECS_Column *component = cc.component;
    void (*free)(void *) = component->component_callbacks.free; 

    if (free)
        free(cc.structure);

    SDL_free(cc.structure);
}

static void ECS_ConstructionFree(ECS_Construction c) {
    for (size_t i = 0; i < c.ncomponent_constructions; ++i)
        ECS_ComponentConstructionFree(c.component_constructions[i]);

    SDL_free(c.component_constructions);

    for (size_t i = 0; i < c.nchildren; ++i)
        ECS_ConstructionFree(c.children[i]);
}

static ECS_EntityHeader ECS_EntityHeaderCreate(ECS *ecs) {
    ECS_EntityHeader header = {
        .self = SDL_malloc(sizeof(ECS_Handle)),
        .children = List_Create(ECS_Handle *),
        .active_components = Bitset_Create(),
        .children_add = List_Create(ECS_Construction),
        .components_attach = List_Create(ECS_ComponentConstruction),
        .components_detach = List_Create(ECS_Handle *),
    };

    header.self->ecs = ecs;
    return header;
}

static void ECS_EntityHeaderFree(ECS_EntityHeader h) {
    SDL_free(h.self);
    List_Free(h.children);
    Bitset_Free(h.active_components);
    List_Free(h.components_detach);

    List_ForEach(h.children_add, construction, ECS_ConstructionFree(construction); );
    List_ForEach(h.components_attach, component_construction, ECS_ComponentConstructionFree(component_construction); );

    List_Free(h.children_add);
    List_Free(h.components_attach);
}

static size_t ECS_WriteRows(ECS *ecs, size_t index, ECS_Handle *parent, ECS_Construction *constructions, size_t nconstructions) {
    for (size_t i = 0; i < nconstructions; ++i) {
        ECS_Construction construction = constructions[i];
        ECS_EntityHeader *header = ecs->headers + index;
        *header = ECS_EntityHeaderCreate(ecs);
        header->self->index = index;
        header->parent = parent;
        List_Push(ecs->headers[parent->index].children, header->self);

        /* Queue component attachments */
        List_PushRange(header->components_attach, construction.component_constructions, construction.ncomponent_constructions);
        header->ncomponents_attach = construction.ncomponent_constructions;
        index = ECS_WriteRows(ecs, index + 1, header->self, construction.children, construction.nchildren);
        SDL_free(construction.component_constructions);
        SDL_free(construction.children);
    }

    return index;
}

static void ECS_ShiftRows(ECS *ecs, size_t start, size_t end, ptrdiff_t shift) {
    Bitset *component_union = Bitset_Create();

    for (size_t i = start; i < end; ++i) {
        ecs->headers[start].self->index += shift;
        component_union = Bitset_Union(component_union, ecs->headers[i].active_components);
    }

    Bitset_ForEach(component_union, id, {
        ECS_Column *component = List_GetAddress(ecs->columns, id);
        size_t right = end;

        while (right > start && !Bitset_Test(ecs->headers[right - 1].active_components, id))
            right -= 1;

        if (right + shift > component->capacity) {
            component->capacity = List_bounding_size(right + shift);
            component->component_entries = SDL_realloc(component->component_entries, component->capacity * component->component_size);
        }

        SDL_memmove(component->component_entries + start + shift, component->component_entries + start, (right - start) * component->component_size);
    });

    Bitset_Free(component_union);
    SDL_memmove(ecs->headers + start + shift, ecs->headers + start, (end - start) * sizeof(ECS_EntityHeader));
}

static void ECS_UpdateSubtable(ECS *ecs, size_t index, size_t left, size_t right, ptrdiff_t shift) {
    while (index < right) {
        if (ecs->headers[index].was_free) {
            size_t fwd_index = index + 1;
            ptrdiff_t fwd_shift = shift - 1;

            while (fwd_index < right && ecs->headers[fwd_index].was_free) {
                fwd_index += 1;
                fwd_shift -= 1;
            }

            if (fwd_shift < 0) {
                ECS_ShiftRows(ecs, left, index, shift);
                ECS_UpdateSubtable(ecs, fwd_index, fwd_index, right, fwd_shift);
            }
            else {
                ECS_UpdateSubtable(ecs, fwd_index, fwd_index, right, fwd_shift);
                ECS_ShiftRows(ecs, left, index, shift);
            }

            return;
        }

        if (ecs->headers[index].ndescendants_add) {
            size_t fwd_index = index;
            ptrdiff_t fwd_shift = shift;

            for (size_t count = 1; count > 0; ++fwd_index) {
                if (ecs->headers[fwd_index].was_free) {
                    fwd_shift -= 1;
                }
                else {
                    count += List_Length(ecs->headers[fwd_index].children) - 1;
                    fwd_shift += ecs->headers[fwd_index].ndescendants_add;
                }
            }

            if (fwd_shift < 0) {
                ECS_UpdateSubtable(ecs, index + 1, left, fwd_index, shift);
                ECS_UpdateSubtable(ecs, fwd_index, fwd_index, right, fwd_shift);
            }
            else {
                ECS_UpdateSubtable(ecs, fwd_index, fwd_index, right, fwd_shift);
                ECS_UpdateSubtable(ecs, index + 1, left, fwd_index, shift);
            }

            ECS_EntityHeader header = ecs->headers[index];
            ECS_WriteRows(ecs, fwd_index + shift, header.self, List_GetAddress(header.children_add, 0), header.nchildren_add);
            List_RemoveSpace(header.children_add, 0, header.nchildren_add);
            return;
        }

        index += 1;
    }

    ECS_ShiftRows(ecs, left, right, shift);
}

void ECS_Update(ECS *ecs) {
    ptrdiff_t nent_diff = 0;

    for (size_t i = 0; i < ecs->length; ++i) {
        ECS_EntityHeader *header = ecs->headers + i;
        header->nchildren_add = List_Length(header->children_add);
        header->ncomponents_attach = List_Length(header->components_attach);
        header->ncomponents_detach = List_Length(header->components_detach);
        header->ndescendants_add = 0;
        header->was_free = header->free;

        if (header->free) {
            nent_diff -= 1;
        }
        else {
            List_ForEach(header->children_add, construction, header->ndescendants_add += ECS_ConstructionLength(construction); );
            nent_diff += header->ndescendants_add;
        }
    }

    if (ecs->length + nent_diff > ecs->capacity) {
        ecs->capacity = List_bounding_size(ecs->length + nent_diff);
        ecs->headers = SDL_realloc(ecs->headers, ecs->capacity * sizeof(ECS_EntityHeader));
    }

    /* Detach components */
    for (size_t i = ecs->length; i > 0; --i) {
        ECS_EntityHeader header = ecs->headers[i - 1];

        List_For(header.components_detach, 0, header.ncomponents_detach, component, {
            ECS_Column column = List_Get(ecs->columns, component->index);

            if (column.component_callbacks.detach)
                column.component_callbacks.detach(header.self);
        });
    }

    /* Free detached components and entities as necessary */
    for (size_t i = ecs->length; i > 0; --i) {
        ECS_EntityHeader header = ecs->headers[i - 1];

        /* Free components */
        List_For(header.components_detach, 0, header.ncomponents_detach, component, {
            ECS_Column column = List_Get(component->ecs->columns, component->index);

            if (column.component_callbacks.free)
                column.component_callbacks.free(column.component_entries + (i - 1) * column.component_size);

            Bitset_Unset(header.active_components, component->index);
        });

        /* Dequeue component detachments */
        List_RemoveSpace(header.components_detach, 0, header.ncomponents_detach);

        /* Free entity */
        if (header.was_free) {
            if (header.parent) {
                List(ECS_Handle *) *parent_children = ecs->headers[header.parent->index].children; 
                List_RemoveWhere(parent_children, child, child == header.self);
            }

            ECS_EntityHeaderFree(header);
        }
    }

    /* Shuffle and write rows, taking into account all deletions and insertions */
    ECS_UpdateSubtable(ecs, 0, 0, ecs->length, 0);
    ecs->length += nent_diff;

    /* Prepare component attachments */
    for (size_t i = ecs->length; i > 0; --i) {
        ECS_EntityHeader header = ecs->headers[i - 1];

        /* Copy constructed components */
        List_For(header.components_attach, 0, header.ncomponents_attach, component_construction, {
            ECS_Handle *component = component_construction.component;
            ECS_Column *column = List_GetAddress(ecs->columns, component->index);
            Bitset_Set(header.active_components, component->index);

            if (column->capacity < i) {
                column->capacity = List_bounding_size(i);
                column->component_entries = SDL_realloc(column->component_entries, column->component_size * column->capacity);
            }

            SDL_memcpy(column->component_entries + column->component_size * (i - 1), component_construction.structure, column->component_size);
            SDL_free(component_construction.structure);
        });
    }

    /* Attach components */
    for (size_t i = ecs->length; i > 0; --i) {
        ECS_EntityHeader header = ecs->headers[i - 1];

        List_For(header.components_attach, 0, header.ncomponents_attach, component_construction, {
            ECS_Handle *component = component_construction.component;
            ECS_Column column = List_Get(ecs->columns, component->index);
            void (*attach)(ECS_Handle *) = column.component_callbacks.attach;

            if (attach)
                attach(header.self);
        });

        /* Dequeue component attachments */
        List_RemoveSpace(header.components_attach, 0, header.ncomponents_attach);
    }
}

void *ECS_RegisterComponentActual(ECS *ecs, size_t size, ECS_ComponentCallbacks callbacks) {
    ECS_Column new_column = {
        .component = SDL_malloc(sizeof(ECS_Handle)),
        .component_size = size,
        .component_callbacks = callbacks
    };

    *new_column.component = (ECS_Handle) {
        .ecs = ecs,
        .index = List_Length(ecs->columns)
    };

    List_Push(ecs->columns, new_column);
    return new_column.component;
}

ECS_Handle *ECS_GetRoot(ECS *ecs) {
    return ecs->length ? ecs->headers->self : nullptr;
}

ECS_Handle *ECS_GetLast(ECS *ecs) {
    return ecs->length ? ecs->headers[ecs->length - 1].self : nullptr;
}

void *ECS_RegisterSystemGroup(ECS *ecs) {
    ECS_SystemGroup new_group = {
        .handle = SDL_malloc(sizeof(ECS_Handle)),
        .systems = List_Create(ECS_System)
    };

    *new_group.handle = (ECS_Handle) {
        .ecs = ecs,
        .index = List_Length(ecs->system_groups)
    };

    List_Push(ecs->system_groups, new_group);
    return new_group.handle;
}

SDL_FunctionPointer *ECS_SystemGroup_RegisterSystemActual(void *system_group, void **dependencies, size_t ndependencies) {
    ECS_Handle *group_handle = system_group;
    ECS_SystemGroup group = List_Get(group_handle->ecs->system_groups, group_handle->index);
    ECS_System *new_system = List_PushSpace(group.systems, 1);

    *new_system = (ECS_System) {
        .dependencies = SDL_malloc(sizeof(size_t) * ndependencies),
        .ndependencies = ndependencies
    };

    for (size_t i = 0; i < ndependencies; ++i) {
        ECS_Handle *component = dependencies[i];
        new_system->dependencies[i] = component->index;
    }

    return &new_system->callback;
}

ECS *ECS_SystemGroup_GetECS(void *system_group) {
    ECS_Handle *group_handle = system_group;
    return group_handle->ecs;
}

size_t ECS_SystemGroup_Length(void *system_group) {
    ECS_Handle *group_handle = system_group;
    ECS_SystemGroup group = List_Get(group_handle->ecs->system_groups, group_handle->index);
    return List_Length(group.systems);
}

SDL_FunctionPointer ECS_SystemGroup_GetCallback(void *system_group, size_t index) {
    ECS_Handle *group_handle = system_group;
    ECS_SystemGroup group = List_Get(group_handle->ecs->system_groups, group_handle->index);
    return List_Get(group.systems, index).callback;
}

ECS *ECS_Create(void) {
    ECS *ecs = SDL_malloc(sizeof(ECS));

    *ecs = (ECS) {
        .headers = SDL_malloc(sizeof(ECS_EntityHeader)),
        .columns = List_Create(ECS_Column),
        .system_groups = List_Create(ECS_SystemGroup),
        .length = 1,
        .capacity = 1
    };

    ecs->headers[0] = ECS_EntityHeaderCreate(ecs);
    ecs->headers->self->index = 0;

    return ecs;
}

void ECS_Free(ECS *ecs) {
    if (ecs->length) {
        ECS_Entity_Free(ecs->headers->self);
        ECS_Update(ecs);
    }

    SDL_free(ecs->headers);

    List_ForEach(ecs->columns, column, {
        SDL_free(column.component);
        SDL_free(column.component_entries);
    });

    List_Free(ecs->columns);

    List_ForEach(ecs->system_groups, system_group, {
        List_ForEach(system_group.systems, system, SDL_free(system.dependencies); );
        List_Free(system_group.systems);
        SDL_free(system_group.handle);
    });

    List_Free(ecs->system_groups);
    SDL_free(ecs);
}
