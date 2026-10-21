#ifndef NES_CORE_ENGINE_H
#define NES_CORE_ENGINE_H

#include <stdint.h>

#include "nes_core_runtime.h"

typedef enum
{
    NES_CORE_ENGINE_OK = 0,
    NES_CORE_ENGINE_INVALID_ARG,
    NES_CORE_ENGINE_NOT_READY,
    NES_CORE_ENGINE_VIDEO_BUSY,
    NES_CORE_ENGINE_CORE_ERROR
} nes_core_engine_result_t;

typedef struct
{
    nes_core_runtime_t *runtime;
    uint32_t frame_count;
    uint8_t ready;
} nes_core_engine_t;

typedef struct
{
    uint8_t valid;
    uint8_t reg1;
    uint16_t reg0;
} nes_core_cpu_fault_t;

int nes_core_engine_prepare(nes_core_engine_t *engine,
                            nes_core_runtime_t *runtime);
int nes_core_engine_run_frame(nes_core_engine_t *engine);
void nes_core_engine_stop(nes_core_engine_t *engine);
int nes_core_engine_is_ready(const nes_core_engine_t *engine);
uint32_t nes_core_engine_frame_count(const nes_core_engine_t *engine);
void nes_core_engine_get_cpu_fault(nes_core_cpu_fault_t *fault);
void nes_core_engine_clear_cpu_fault(void);

#endif
