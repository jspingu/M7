#ifndef M7_RESOURCE_H
#define M7_RESOURCE_H

#include <M7/ECS.h>
#include <M7/Collections/Strmap.h>

#define M7_ResourceBank(type)                     type
#define M7_ResourceBank_Get(self,component,path)  ( (typeof(*component))M7_ResourceBank_GetActual(self, component, path) )

#define M7_RESOURCE_PATHLEN  64

typedef void *(*M7_ResourceLoad)(ECS_Handle *self, char *path);
typedef void (*M7_ResourceFree)(ECS_Handle *self, void *data);

typedef struct M7_Resource {
    void *data;
    size_t refcount;
} M7_Resource;

typedef struct M7_ResourceBank {
    Strmap(M7_Resource) *map;
    M7_ResourceLoad load;
    M7_ResourceFree free;
} M7_ResourceBank;

void *M7_ResourceBank_GetActual(ECS_Handle *self, void *component, char *path);
void M7_ResourceBank_Release(ECS_Handle *self, void *component, char *path);
void M7_ResourceBank_Attach(ECS_Handle *self, ECS_Component(void) *component, M7_ResourceLoad load, M7_ResourceFree free);
void M7_ResourceBank_Detach(ECS_Handle *self, ECS_Component(void) *component);

#endif /* M7_RESOURCE_H */
