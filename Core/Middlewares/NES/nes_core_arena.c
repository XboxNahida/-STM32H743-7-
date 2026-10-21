#include "nes_core_arena.h"

#include <stddef.h>
#include <string.h>

int nes_core_arena_init(nes_core_arena_t *arena,
                        void *storage,
                        uint32_t capacity)
{
    if ((arena == NULL) || (storage == NULL) || (capacity == 0U))
    {
        return NES_CORE_ARENA_INVALID_ARG;
    }
    arena->base = (uint8_t *)storage;
    arena->capacity = capacity;
    arena->offset = 0U;
    return NES_CORE_ARENA_OK;
}

void *nes_core_arena_alloc(nes_core_arena_t *arena,
                           uint32_t size,
                           uint32_t alignment)
{
    uintptr_t current;
    uintptr_t aligned;
    uint32_t padding;
    uint32_t required;
    void *result;

    if ((arena == NULL) || (arena->base == NULL) || (size == 0U) ||
        (alignment == 0U) || ((alignment & (alignment - 1U)) != 0U) ||
        (arena->offset > arena->capacity))
    {
        return NULL;
    }
    current = (uintptr_t)arena->base + (uintptr_t)arena->offset;
    aligned = (current + (uintptr_t)alignment - 1U) &
              ~((uintptr_t)alignment - 1U);
    if (aligned < current)
    {
        return NULL;
    }
    padding = (uint32_t)(aligned - current);
    if ((padding > arena->capacity - arena->offset) ||
        (size > arena->capacity - arena->offset - padding))
    {
        return NULL;
    }
    required = padding + size;
    result = (void *)aligned;
    memset(result, 0, size);
    arena->offset += required;
    return result;
}

void nes_core_arena_reset(nes_core_arena_t *arena)
{
    if ((arena != NULL) && (arena->base != NULL))
    {
        arena->offset = 0U;
    }
}

uint32_t nes_core_arena_used(const nes_core_arena_t *arena)
{
    return (arena != NULL) ? arena->offset : 0U;
}
