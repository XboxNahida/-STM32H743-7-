#ifndef FONT_CACHE_SERVICE_H
#define FONT_CACHE_SERVICE_H

#include <stdint.h>

#include "nand_cache_metadata.h"

typedef enum
{
    FONT_CACHE_SERVICE_IDLE = 0,
    FONT_CACHE_SERVICE_READY_CACHE,
    FONT_CACHE_SERVICE_FALLBACK,
    FONT_CACHE_SERVICE_READY_REBUILT
} font_cache_service_state_t;

typedef enum
{
    FONT_CACHE_REBUILD_STAGE_NONE = 0,
    FONT_CACHE_REBUILD_STAGE_PLAN,
    FONT_CACHE_REBUILD_STAGE_STORE_CRC,
    FONT_CACHE_REBUILD_STAGE_BAD_BLOCK,
    FONT_CACHE_REBUILD_STAGE_ERASE_DATA,
    FONT_CACHE_REBUILD_STAGE_PROGRAM_DATA,
    FONT_CACHE_REBUILD_STAGE_READBACK_DATA,
    FONT_CACHE_REBUILD_STAGE_ERASE_METADATA,
    FONT_CACHE_REBUILD_STAGE_PROGRAM_METADATA,
    FONT_CACHE_REBUILD_STAGE_READBACK_METADATA,
    FONT_CACHE_REBUILD_STAGE_COMPLETE
} font_cache_rebuild_stage_t;

typedef struct
{
    font_cache_rebuild_stage_t stage;
    uint32_t target;
    int32_t detail_result;
    uint8_t nand_status;
    uint8_t nand_status_valid;
    uint8_t nand_ready;
} font_cache_rebuild_diagnostics_t;

typedef struct
{
    font_cache_service_state_t state;
    uint8_t active_slot;
    uint32_t sequence;
    uint32_t package_size;
    uint32_t corrected_pages;
    uint32_t load_elapsed_ms;
    uint32_t rebuild_elapsed_ms;
    int32_t last_result;
    font_cache_rebuild_diagnostics_t rebuild;
} font_cache_service_status_t;

typedef void (*font_cache_service_progress_fn)(uint32_t completed_pages,
                                               uint32_t total_pages,
                                               void *context);

void font_cache_service_init(void);
void font_cache_service_set_load_progress_callback(
    font_cache_service_progress_fn callback, void *context);
int font_cache_service_try_load(void);
int font_cache_service_rebuild(void);
int font_cache_service_invalidate(void);
void font_cache_service_get_status(font_cache_service_status_t *status);

#endif
