#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/Collections/Strmap.h>

#include <M7/M7_Resource.h>

void *M7_ResourceBank_GetActual(ECS_Handle *self, ECS_Component(void) *component, char *path) {
    M7_ResourceBank *bank = ECS_Entity_GetComponent(self, component);
    M7_Resource *resource = Strmap_GetAddress(bank->map, path);

    if (resource) {
        resource->refcount += 1;
        return resource->data;
    }
    
    M7_Resource new_resource = {
        .data = bank->load(self, path),
        .refcount = 1
    };

    Strmap_Set(bank->map, path, new_resource);
    return new_resource.data;
}

void M7_ResourceBank_Release(ECS_Handle *self, ECS_Component(void) *component, char *path) {
    M7_ResourceBank *bank = ECS_Entity_GetComponent(self, component);
    M7_Resource *resource = Strmap_GetAddress(bank->map, path);

    if (!resource)
        return;

    resource->refcount -= 1;

    if (!resource->refcount) {
        bank->free(self, resource->data);
        Strmap_Remove(bank->map, path);
    }
}

void M7_ResourceBank_Attach(ECS_Handle *self, ECS_Component(void) *component, M7_ResourceLoad load, M7_ResourceFree free) {
    M7_ResourceBank *bank = ECS_Entity_GetComponent(self, component);
    bank->map = Strmap_Create(M7_Resource, M7_RESOURCE_PATHLEN);
    bank->load = load;
    bank->free = free;
}

void M7_ResourceBank_Detach(ECS_Handle *self, ECS_Component(void) *component) {
    M7_ResourceBank *bank = ECS_Entity_GetComponent(self, component);
    Strmap_ForEach(bank->map, resource, bank->free(self, resource.data); );
    Strmap_Free(bank->map);
}
