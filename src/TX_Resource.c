#include <SDL3/SDL.h>
#include <TX/ECS.h>
#include <TX/Collections/Strmap.h>

#include <TX/TX_Resource.h>

void *TX_ResourceBank_GetActual(ECS_Handle *self, ECS_Component(void) *component, char *path) {
    TX_ResourceBank *bank = ECS_Entity_GetComponent(self, component);
    TX_Resource *resource = Strmap_GetAddress(bank->map, path);

    if (resource) {
        resource->refcount += 1;
        return resource->data;
    }
    
    TX_Resource new_resource = {
        .data = bank->load(self, path),
        .refcount = 1
    };

    Strmap_Set(bank->map, path, new_resource);
    return new_resource.data;
}

void TX_ResourceBank_Release(ECS_Handle *self, ECS_Component(void) *component, char *path) {
    TX_ResourceBank *bank = ECS_Entity_GetComponent(self, component);
    TX_Resource *resource = Strmap_GetAddress(bank->map, path);

    if (!resource)
        return;

    resource->refcount -= 1;

    if (!resource->refcount) {
        bank->free(self, resource->data);
        Strmap_Remove(bank->map, path);
    }
}

void TX_ResourceBank_Attach(ECS_Handle *self, ECS_Component(void) *component, TX_ResourceLoad load, TX_ResourceFree free) {
    TX_ResourceBank *bank = ECS_Entity_GetComponent(self, component);
    bank->map = Strmap_Create(TX_Resource, TX_RESOURCE_PATHLEN);
    bank->load = load;
    bank->free = free;
}

void TX_ResourceBank_Detach(ECS_Handle *self, ECS_Component(void) *component) {
    TX_ResourceBank *bank = ECS_Entity_GetComponent(self, component);
    Strmap_ForEach(bank->map, resource, bank->free(self, resource.data); );
    Strmap_Free(bank->map);
}
