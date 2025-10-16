#ifndef ECS_H
#define ECS_H

#include <SDL3/SDL_stdinc.h>

#define ECS_Component(type)                                  typeof(type)
#define ECS_SystemGroup(callback_type)                       typeof(callback_type)

#define ECS_Entity_GetComponent(e,component)                 ( (typeof(component))ECS_Entity_GetComponentActual(e, component) )
#define ECS_Entity_AttachComponents(e,...)                   ( ECS_Entity_AttachComponentsCount(e, (ECS_ComponentConstruction []){__VA_ARGS__}, SDL_arraysize(((ECS_ComponentConstruction []){__VA_ARGS__}))) )
#define ECS_Entity_DetachComponents(e,...)                   ( ECS_Entity_DetachComponentsCount(e, (void *[]){__VA_ARGS__}, SDL_arraysize(((void *[]){__VA_ARGS__}))) )
#define ECS_Entity_AddChildren(e,...)                        ( ECS_Entity_AddChildrenCount(e, (ECS_Construction []){__VA_ARGS__}, SDL_arraysize(((ECS_Construction []){__VA_ARGS__}))) )

#define ECS_RegisterComponent(ecs,cmp_type,...)              ( ECS_RegisterComponentActual(ecs, sizeof(cmp_type), (ECS_ComponentCallbacks)__VA_ARGS__) )
#define ECS_SystemGroup_RegisterSystem(group,callback,...)   ( *(typeof(group))ECS_SystemGroup_RegisterSystemActual(group, (void *[]){__VA_ARGS__}, SDL_arraysize(((void *[]){__VA_ARGS__}))) = callback )

#define ECS_Components(...)                                                                    \
    .ncomponent_constructions = SDL_arraysize(((ECS_ComponentConstruction []){__VA_ARGS__})),  \
    .component_constructions = (ECS_ComponentConstruction []){__VA_ARGS__}

#define ECS_Children(...)                                                                      \
    .nchildren = SDL_arraysize(((ECS_Construction []){__VA_ARGS__})),                          \
    .children = (ECS_Construction []){__VA_ARGS__}

#define ECS_SystemGroup_Process(group,...)  do {                                                                    \
    for (size_t i = 0, n = ECS_SystemGroup_Length(group); i < n; ++i) {                                             \
        typeof(*group) callback = (typeof(*group))(ECS_SystemGroup_GetCallback(group, i));                          \
        ECS *ecs = ECS_SystemGroup_GetECS(group);                                                                   \
        for (ECS_Handle *e = ECS_GetRoot(ecs); e && ECS_Entity_MatchesSystem(e, group, i); e = ECS_Entity_Next(e))  \
            callback(e __VA_OPT__(,) __VA_ARGS__);                                                                  \
    }                                                                                                               \
} while (0)

#define ECS_SystemGroup_ProcessReverse(group,...)  do {                                                             \
    for (size_t i = 0, n = ECS_SystemGroup_Length(group); i < n; ++i) {                                             \
        typeof(*group) callback = (typeof(*group))(ECS_SystemGroup_GetCallback(group, i));                          \
        ECS *ecs = ECS_SystemGroup_GetECS(group);                                                                   \
        for (ECS_Handle *e = ECS_GetLast(ecs); e && ECS_Entity_MatchesSystem(e, group, i); e = ECS_Entity_Prev(e))  \
            callback(e __VA_OPT__(,) __VA_ARGS__);                                                                  \
    }                                                                                                               \
} while (0)

#define ECS_Entity_ProcessSystemGroup(e,group,...)  do {                                                            \
    for (size_t i = 0, n = ECS_SystemGroup_Length(group); i < n; ++i) {                                             \
        typeof(*group) callback = (typeof(*group))(ECS_SystemGroup_GetCallback(group, i));                          \
        if (ECS_Entity_MatchesSystem(e, group, i))                                                                  \
            callback(e __VA_OPT__(,) __VA_ARGS__);                                                                  \
    }                                                                                                               \
} while (0)

#define ECS_Entity_ForEachChild(e,child,...)  do {                                                                     \
    ECS_Handle **ECS_Entity_children = ECS_Entity_GetChildren(e);                                                      \
    size_t ECS_Entity_child_count = ECS_Entity_GetChildCount(e);                                                       \
    for (size_t ECS_Entity_child_iter = 0; ECS_Entity_child_iter < ECS_Entity_child_count; ++ECS_Entity_child_iter) {  \
        ECS_Handle *child = ECS_Entity_children[ECS_Entity_child_iter];                                                \
        do __VA_ARGS__ while (0);                                                                                      \
    }                                                                                                                  \
} while (0)

typedef struct ECS ECS;
typedef struct ECS_Handle ECS_Handle;

typedef struct ECS_ComponentCallbacks {
    void (*init)(void *component, void *args);
    void (*free)(void *component);
    void (*attach)(ECS_Handle *self);
    void (*detach)(ECS_Handle *self);
} ECS_ComponentCallbacks;

typedef struct ECS_ComponentConstruction {
    void *component;
    void *structure;
} ECS_ComponentConstruction;

typedef struct ECS_Construction {
    ECS_ComponentConstruction *component_constructions;
    struct ECS_Construction *children;
    size_t ncomponent_constructions;
    size_t nchildren;
} ECS_Construction;

void *ECS_Entity_GetComponentActual(ECS_Handle *e, void *component);
ECS_Handle **ECS_Entity_GetChildren(ECS_Handle *e);
size_t ECS_Entity_GetChildCount(ECS_Handle *e);

ECS_Handle *ECS_Entity_AncestorWithComponent(ECS_Handle *e, void *component, bool inclusive);
ECS_Handle *ECS_Entity_DescendantWithComponent(ECS_Handle *e, void *component, bool inclusive);

void ECS_Entity_AttachComponentsCount(ECS_Handle *e, ECS_ComponentConstruction *components, size_t count);
void ECS_Entity_DetachComponentsCount(ECS_Handle *e, void **components, size_t count);
void ECS_Entity_AddChildrenCount(ECS_Handle *e, ECS_Construction *children, size_t count);

ECS_Handle *ECS_Entity_Next(ECS_Handle *e);
ECS_Handle *ECS_Entity_Prev(ECS_Handle *e);
bool ECS_Entity_MatchesSystem(ECS_Handle *e, void *system_group, size_t index);

void ECS_Entity_Free(ECS_Handle *e);

void *ECS_RegisterComponentActual(ECS *ecs, size_t component_size, ECS_ComponentCallbacks callbacks);
void *ECS_RegisterSystemGroup(ECS *ecs);
SDL_FunctionPointer *ECS_SystemGroup_RegisterSystemActual(void *system_group, void **dependencies, size_t ndependencies);

ECS *ECS_SystemGroup_GetECS(void *system_group);
size_t ECS_SystemGroup_Length(void *system_group);
SDL_FunctionPointer ECS_SystemGroup_GetCallback(void *system_group, size_t index);

ECS_Handle *ECS_GetRoot(ECS *ecs);
ECS_Handle *ECS_GetLast(ECS *ecs);

void ECS_Update(ECS *ecs);

ECS *ECS_Create(void);
void ECS_Free(ECS *ecs);

#endif /* ECS_H */
