#ifndef NES_CORE_RUNTIME_H
#define NES_CORE_RUNTIME_H

#include <stdint.h>

#include "nes_core_arena.h"

typedef enum
{
    NES_CORE_RUNTIME_OK = 0,
    NES_CORE_RUNTIME_INVALID_ARG,
    NES_CORE_RUNTIME_INVALID_ROM,
    NES_CORE_RUNTIME_UNSUPPORTED_MAPPER,
    NES_CORE_RUNTIME_NO_MEMORY,
    NES_CORE_RUNTIME_BUSY
} nes_core_runtime_result_t;

typedef struct
{
    nes_core_arena_t arena;
    const uint8_t *rom;
    uint32_t rom_size;
    uint8_t active;
} nes_core_runtime_t;

/* 6502汇编需要按符号名访问这三个兼容指针。 */
extern uint8_t *NES_RAM;
extern uint8_t *NES_SRAM;
extern uint8_t *romfile;

int nes_core_runtime_open(nes_core_runtime_t *runtime,
                          void *arena_storage,
                          uint32_t arena_capacity,
                          const uint8_t *rom,
                          uint32_t rom_size);
void nes_core_runtime_close(nes_core_runtime_t *runtime);
int nes_core_runtime_is_active(const nes_core_runtime_t *runtime);
uint32_t nes_core_runtime_arena_used(const nes_core_runtime_t *runtime);

#endif
