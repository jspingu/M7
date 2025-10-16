#ifndef BITSET_H
#define BITSET_H

#include <SDL3/SDL.h>
#include "List.h"

typedef List(unsigned char) Bitset;

#define Bitset_ForEach(bitset,member,...)  do {                                                      \
    for (size_t Bitset_byte_iter = 0; Bitset_byte_iter < List_Length(bitset); ++Bitset_byte_iter) {  \
        unsigned char Bitset_byte = List_Get(bitset, Bitset_byte_iter);                              \
        for (int Bitset_bit_iter = 0; Bitset_byte; Bitset_byte >>= 1) {                              \
            size_t member = (Bitset_byte_iter << 3) + Bitset_bit_iter;                               \
            if (Bitset_byte & 1) __VA_ARGS__                                                         \
        }                                                                                            \
    }                                                                                                \
} while (0)

static inline void Bitset_EnsureWidth(Bitset *b, size_t bytes) {
    if (bytes <= List_Length(b))
        return;

    size_t diff = bytes - List_Length(b);
    SDL_memset(List_PushSpace(b, diff), 0, diff);
}

static inline bool Bitset_Test(Bitset *b, size_t bit) {
    if ((bit >> 3) + 1 > List_Length(b))
        return false;
    
    return List_Get(b, bit >> 3) & (1 << (bit & 7));
}

static inline void Bitset_Set(Bitset *b, size_t bit) {
    Bitset_EnsureWidth(b, (bit >> 3) + 1);
    List_Set(b, bit >> 3, List_Get(b, bit >> 3) | (1 << (bit & 7)));
}

static inline void Bitset_Unset(Bitset *b, size_t bit) {
    if ((bit >> 3) + 1 > List_Length(b))
        return;

    List_Set(b, bit >> 3, List_Get(b, bit >> 3) & ~(1 << (bit & 7)));
}

static inline void Bitset_Flip(Bitset *b, size_t bit) {
    Bitset_EnsureWidth(b, (bit >> 3) + 1);
    List_Set(b, bit >> 3, List_Get(b, bit >> 3) ^ (1 << (bit & 7)));
}

/* Compute the union of two Bitsets, storing the resulting Bitset in lhs. */
static inline Bitset *Bitset_Union(Bitset *lhs, Bitset *rhs) {
    Bitset_EnsureWidth(lhs, SDL_max(List_Length(lhs), List_Length(rhs)));

    for (size_t i = 0; i < List_Length(rhs); ++i)
        List_Set(lhs, i, List_Get(lhs, i) | List_Get(rhs, i));

    return lhs;
}

/* Compute the intersection of two Bitsets, storing the resulting Bitset in lhs. */
static inline Bitset *Bitset_Intersection(Bitset *lhs, Bitset *rhs) {
    size_t min_bytes = SDL_min(List_Length(lhs), List_Length(rhs));

    for (size_t i = 0; i < min_bytes; ++i)
        List_Set(lhs, i, List_Get(lhs, i) & List_Get(rhs, i));

    SDL_memset(List_GetAddress(lhs, min_bytes), 0, List_Length(lhs) - min_bytes);
    return lhs;
}

static inline Bitset *Bitset_Create(void) {
    return List_Create(unsigned char);
}

static inline void Bitset_Free(Bitset *b) {
    List_Free(b);
}

#endif /* BITSET_H */
