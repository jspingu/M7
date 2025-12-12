#ifndef M7_RESOURCE_H
#define M7_RESOURCE_H

#include <M7/ECS.h>

#define M7_ResourceBank(type)                    type
#define M7_ResourceBank_Get(resource_bank,path)  ( (typeof(*resource_bank))M7_ResourceBank_GetActual(resource_bank, path) )

typedef void *(*M7_ResourceLoad)(ECS_Handle *self, char *path);
typedef void (*M7_ResourceFree)(ECS_Handle *self, void *data);

void *M7_ResourceBank_GetActual(void *resource_bank, char *path);
void M7_ResourceBank_Release(void *resource_bank, char *path);
void *M7_ResourceBank_Create(ECS_Handle *self, size_t pathlen, M7_ResourceLoad load, M7_ResourceFree free);
void M7_ResourceBank_Free(void *resource_bank);
void M7_ResourceBankComponent_Free(void *component);

#endif /* M7_RESOURCE_H */
