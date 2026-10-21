#ifndef NES_CORE_ARENA_H
#define NES_CORE_ARENA_H

#include <stdint.h>

typedef enum
{
    NES_CORE_ARENA_OK = 0,
    NES_CORE_ARENA_INVALID_ARG
} nes_core_arena_result_t;

typedef struct
{
    uint8_t *base;
    uint32_t capacity;
    uint32_t offset;
} nes_core_arena_t;

int nes_core_arena_init(nes_core_arena_t *arena,
                        void *storage,
                        uint32_t capacity);
void *nes_core_arena_alloc(nes_core_arena_t *arena,
                           uint32_t size,
                           uint32_t alignment);
void nes_core_arena_reset(nes_core_arena_t *arena);
uint32_t nes_core_arena_used(const nes_core_arena_t *arena);

#endif
