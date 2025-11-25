#include <SDL3/SDL.h>
#include <M7/Collections/List.h>
#include <M7/Collections/Bitset.h>
#include <M7/ECS.h>

#include "ECS_c.h"

static ECS_ComponentConstruction ECS_ConstructComponent(ECS_ComponentConstruction cc) {
    ECS_Handle *component = cc.component;
    ECS_Column column = List_Get(component->ecs->columns, component->index);
    void *structure = SDL_malloc(column.component_size);
    void (*init)(void *, void *) = column.component_callbacks.init;

    if (init)
        init(structure, cc.structure);
    else if (cc.structure)
        SDL_memcpy(structure, cc.structure, column.component_size);

    return (ECS_ComponentConstruction) {
        .component = component,
        .structure = structure
    };
}

static ECS_Construction ECS_PrepareConstruction(ECS_Construction c) {
    ECS_ComponentConstruction *components = SDL_malloc(sizeof(ECS_ComponentConstruction) * c.ncomponent_constructions);
    ECS_Construction *children = SDL_malloc(sizeof(ECS_Construction) * c.nchildren);
    Bitset *seen_components = Bitset_Create();
    size_t nunique_components = 0;

    for (size_t i = 0; i < c.ncomponent_constructions; ++i) {
        ECS_Handle *component = c.component_constructions[i].component;
        if (!Bitset_Test(seen_components, component->index)) {
            components[nunique_components++] = ECS_ConstructComponent(c.component_constructions[i]);
            Bitset_Set(seen_components, component->index);
        }
    }

    Bitset_Free(seen_components);

    for (size_t i = 0; i < c.nchildren; ++i)
        children[i] = ECS_PrepareConstruction(c.children[i]);

    return (ECS_Construction) {
        .component_constructions = components,
        .children = children,
        .ncomponent_constructions = nunique_components,
        .nchildren = c.nchildren
    };
}

static bool ECS_Entity_ComponentAttachmentQueued(ECS_Handle *e, size_t component_index) {
    List_ForEach(e->ecs->headers[e->index].components_attach, to_attach, {
        ECS_Handle *component = to_attach.component;

        if (component->index == component_index)
            return true;
    });

    return false;
}

static bool ECS_Entity_ComponentDetachmentQueued(ECS_Handle *e, size_t component_index) {
    List_ForEach(e->ecs->headers[e->index].components_detach, to_detach, {
        if (to_detach->index == component_index)
            return true;
    });

    return false;
}

void *ECS_Entity_GetComponentActual(ECS_Handle *e, void *component) {
    ECS_Handle *c = component;
    ECS_Column column = List_Get(e->ecs->columns, c->index);
    bool has_component = Bitset_Test(e->ecs->headers[e->index].active_components, c->index);
    return has_component ? column.component_entries + column.component_size * e->index : nullptr;
}

ECS_Handle *ECS_Entity_GetParent(ECS_Handle *e) {
    return e->ecs->headers[e->index].parent;
}

ECS_Handle **ECS_Entity_GetChildren(ECS_Handle *e) {
    return List_GetAddress(e->ecs->headers[e->index].children, 0);
}

size_t ECS_Entity_GetChildCount(ECS_Handle *e) {
    return List_Length(e->ecs->headers[e->index].children);
}

ECS_Handle *ECS_Entity_AncestorWithComponent(ECS_Handle *e, void *component, bool inclusive) {
    for (ECS_Handle *ancestor = inclusive ? e : ECS_Entity_GetParent(e); ancestor; ancestor = ECS_Entity_GetParent(ancestor)) {
        if (ECS_Entity_GetComponentActual(ancestor, component))
            return ancestor;
    }

    return nullptr;
}

ECS_Handle *ECS_Entity_DescendantWithComponent(ECS_Handle *e, void *component, bool inclusive) {
    for (size_t index = e->index + !inclusive, count = inclusive ? 1 : ECS_Entity_GetChildCount(e); count; ++index) {
        ECS_Handle *descendant = e->ecs->headers[index].self;

        if (ECS_Entity_GetComponentActual(descendant, component))
            return descendant;

        count += ECS_Entity_GetChildCount(descendant) - 1;
    }

    return nullptr;
}

void ECS_Entity_AttachComponentsCount(ECS_Handle *e, ECS_ComponentConstruction *ccs, size_t count) {
    ECS_EntityHeader header = e->ecs->headers[e->index];

    for (size_t i = 0; i < count; ++i) {
        ECS_ComponentConstruction cc = ccs[i];
        ECS_Handle *component = cc.component;

        if (!Bitset_Test(header.active_components, component->index) && !ECS_Entity_ComponentAttachmentQueued(e, component->index))
            List_Push(header.components_attach, ECS_ConstructComponent(cc));
    }
}

void ECS_Entity_DetachComponentsCount(ECS_Handle *e, void **components, size_t count) {
    ECS_EntityHeader header = e->ecs->headers[e->index];

    for (size_t i = 0; i < count; ++i) {
        ECS_Handle *component = components[i];

        if (Bitset_Test(header.active_components, component->index) && !ECS_Entity_ComponentDetachmentQueued(e, component->index))
            List_Push(header.components_detach, component);
    }
}

void ECS_Entity_AddChildrenCount(ECS_Handle *e, ECS_Construction *children, size_t count) {
    ECS_EntityHeader header = e->ecs->headers[e->index];
    ECS_Construction *new_construction = List_PushSpace(header.children_add, count);

    for (size_t i = 0; i < count; ++i)
        new_construction[i] = ECS_PrepareConstruction(children[i]);
}

ECS_Handle *ECS_Entity_Next(ECS_Handle *e) {
    return e->index < e->ecs->length - 1 ? e->ecs->headers[e->index + 1].self : nullptr;
}

ECS_Handle *ECS_Entity_Prev(ECS_Handle *e) {
    return e->index ? e->ecs->headers[e->index - 1].self : nullptr;
}

bool ECS_Entity_MatchesSystem(ECS_Handle *e, void *system_group, size_t index) {
    ECS_Handle *group_handle = system_group;
    ECS_SystemGroup group = List_Get(group_handle->ecs->system_groups, group_handle->index);
    ECS_System system = List_Get(group.systems, index);

    for (size_t i = 0; i < system.ndependencies; ++i) {
        if (!Bitset_Test(e->ecs->headers[e->index].active_components, system.dependencies[i]))
            return false;
    }

    return true;
}

void ECS_Entity_Free(ECS_Handle *e) {
    for (size_t index = e->index, count = 1; count; ++index) {
        ECS_EntityHeader *header = e->ecs->headers + index;
        Bitset_ForEach(header->active_components, index, ECS_Entity_DetachComponents(header->self, List_Get(e->ecs->columns, index).component); );
        header->free = true;
        count += ECS_Entity_GetChildCount(header->self) - 1;
    }
}
