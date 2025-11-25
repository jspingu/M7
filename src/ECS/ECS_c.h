#ifndef ECS_C_H
#define ECS_C_H

#include <M7/Collections/List.h>
#include <M7/Collections/Bitset.h>
#include <M7/ECS.h>

typedef struct ECS_Handle {
    ECS *ecs;
    size_t index;
} ECS_Handle;

typedef struct ECS_EntityHeader {
    ECS_Handle *self;
    ECS_Handle *parent;
    List(ECS_Handle *) *children;
    Bitset *active_components;

    List(ECS_Construction) *children_add;
    List(ECS_ComponentConstruction) *components_attach;
    List(ECS_Handle *) *components_detach;

    size_t nchildren_add;
    size_t ndescendants_add;
    size_t ncomponents_attach;
    size_t ncomponents_detach;

    bool free;
    bool was_free;
} ECS_EntityHeader;

typedef struct ECS_Column {
    ECS_Handle *component;
    unsigned char *component_entries;
    ECS_ComponentCallbacks component_callbacks;
    size_t component_size;
    size_t capacity;
} ECS_Column;

typedef struct ECS_System {
    SDL_FunctionPointer callback;
    size_t *dependencies;
    size_t ndependencies;
} ECS_System;

typedef struct ECS_SystemGroup {
    ECS_Handle *handle;
    List(ECS_System) *systems;
} ECS_SystemGroup;

typedef struct ECS {
    ECS_EntityHeader *headers;
    List(ECS_Column) *columns;
    List(ECS_SystemGroup) *system_groups;
    size_t length;
    size_t capacity;
} ECS;

#endif /* ECS_C_H */
