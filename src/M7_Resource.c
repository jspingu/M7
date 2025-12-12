#include <SDL3/SDL.h>
#include <M7/ECS.h>
#include <M7/Collections/Strmap.h>

#include <M7/M7_Resource.h>

typedef struct M7_Resource {
    void *data;
    size_t refcount;
} M7_Resource;

typedef struct M7_ResourceBank {
    ECS_Handle *self;
    Strmap(M7_Resource) *map;
    M7_ResourceLoad load;
    M7_ResourceFree free;
} M7_ResourceBank;

void *M7_ResourceBank_GetActual(void *resource_bank, char *path) {
    M7_ResourceBank *rb = resource_bank;
    M7_Resource *resource = Strmap_GetAddress(rb->map, path);

    if (resource) {
        resource->refcount += 1;
        return resource->data;
    }
    
    M7_Resource new_resource = {
        .data = rb->load(rb->self, path),
        .refcount = 1
    };

    Strmap_Set(rb->map, path, new_resource);
    return new_resource.data;
}

void M7_ResourceBank_Release(void *resource_bank, char *path) {
    M7_ResourceBank *rb = resource_bank;
    M7_Resource *resource = Strmap_GetAddress(rb->map, path);

    if (!resource)
        return;

    resource->refcount -= 1;

    if (!resource->refcount) {
        rb->free(rb->self, resource->data);
        Strmap_Remove(rb->map, path);
    }
}

void *M7_ResourceBank_Create(ECS_Handle *self, size_t pathlen, M7_ResourceLoad load, M7_ResourceFree free) {
    M7_ResourceBank *resource_bank = SDL_malloc(sizeof(M7_ResourceBank));
    resource_bank->self = self;
    resource_bank->map = Strmap_Create(M7_Resource, pathlen);
    resource_bank->load = load;
    resource_bank->free = free;
    return resource_bank;
}

void M7_ResourceBank_Free(void *resource_bank) {
    M7_ResourceBank *rb = resource_bank;
    Strmap_ForEach(rb->map, resource, rb->free(rb->self, resource.data); );
    Strmap_Free(rb->map);
    SDL_free(rb);
}
