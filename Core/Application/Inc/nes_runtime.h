#ifndef NES_RUNTIME_H
#define NES_RUNTIME_H

#include <stdint.h>

typedef enum
{
    NES_RUNTIME_IDLE = 0,
    NES_RUNTIME_STARTING,
    NES_RUNTIME_RUNNING,
    NES_RUNTIME_PAUSED,
    NES_RUNTIME_STOPPING,
    NES_RUNTIME_ERROR
} nes_runtime_state_t;

typedef enum
{
    NES_RUNTIME_OK = 0,
    NES_RUNTIME_INVALID_ARG,
    NES_RUNTIME_BUSY,
    NES_RUNTIME_STORAGE_ERROR,
    NES_RUNTIME_NO_MEMORY,
    NES_RUNTIME_INVALID_ROM,
    NES_RUNTIME_UNSUPPORTED_ROM,
    NES_RUNTIME_CORE_ERROR
} nes_runtime_result_t;

typedef struct
{
    nes_runtime_state_t state;
    int result;
    uint32_t rom_size;
    uint32_t arena_used;
    uint32_t frame_count;
    uint32_t start_count;
    uint32_t stop_count;
} nes_runtime_status_t;

void nes_runtime_init(void);
int nes_runtime_start(const char *path);
void nes_runtime_poll(uint32_t now_ms);
int nes_runtime_set_paused(uint8_t paused);
int nes_runtime_stop(void);
void nes_runtime_get_status(nes_runtime_status_t *status);

#endif
