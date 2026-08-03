#ifndef TX_RESOURCE_H
#define TX_RESOURCE_H

#include <TX/ECS.h>
#include <TX/Collections/Strmap.h>

#define TX_ResourceBank(type)                     type
#define TX_ResourceBank_Get(self,component,path)  ( (typeof(*component))TX_ResourceBank_GetActual(self, component, path) )

#define TX_RESOURCE_PATHLEN  64

typedef void *(*TX_ResourceLoad)(ECS_Handle *self, char *path);
typedef void (*TX_ResourceFree)(ECS_Handle *self, void *data);

typedef struct TX_Resource {
    void *data;
    size_t refcount;
} TX_Resource;

typedef struct TX_ResourceBank {
    Strmap(TX_Resource) *map;
    TX_ResourceLoad load;
    TX_ResourceFree free;
} TX_ResourceBank;

void *TX_ResourceBank_GetActual(ECS_Handle *self, void *component, char *path);
void TX_ResourceBank_Release(ECS_Handle *self, void *component, char *path);
void TX_ResourceBank_Attach(ECS_Handle *self, ECS_Component(void) *component, TX_ResourceLoad load, TX_ResourceFree free);
void TX_ResourceBank_Detach(ECS_Handle *self, ECS_Component(void) *component);

#endif /* TX_RESOURCE_H */
