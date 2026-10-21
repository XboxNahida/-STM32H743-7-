#include "font_cache_service.h"

#include <stddef.h>
#include <string.h>

extern int font_cache_service_platform_prepare(void);
extern int font_cache_service_platform_source_crc(uint32_t *source_crc);
extern int font_cache_service_platform_scan(nand_cache_metadata_t *metadata);
extern void *font_cache_service_platform_allocate(uint32_t size);
extern void font_cache_service_platform_free(void *pointer);
extern int font_cache_service_platform_load(
    const nand_cache_metadata_t *metadata,
    uint8_t *destination,
    uint32_t *corrected_pages,
    font_cache_service_progress_fn progress,
    void *progress_context);
extern int font_cache_service_platform_bind(uint8_t *package,
                                            uint32_t package_size,
                                            uint32_t source_crc);
extern int font_cache_service_platform_rebuild(
    uint32_t source_crc,
    const nand_cache_metadata_t *active,
    uint8_t active_valid,
    nand_cache_metadata_t *committed,
    uint32_t *package_size);
extern void font_cache_service_platform_get_rebuild_diagnostics(
    font_cache_rebuild_diagnostics_t *diagnostics);
extern int font_cache_service_platform_invalidate(void);
extern uint32_t font_cache_service_platform_tick(void);

#define FONT_CACHE_SERVICE_NO_SLOT 0xFFU
#define FONT_CACHE_SERVICE_ALLOC_ERROR (-100)

static font_cache_service_status_t g_status;
static nand_cache_metadata_t g_active;
static uint32_t g_source_crc;
static uint8_t g_active_valid;
static uint8_t g_source_valid;
static font_cache_service_progress_fn g_load_progress;
static void *g_load_progress_context;

static void set_fallback(int result, uint32_t started)
{
    g_status.state = FONT_CACHE_SERVICE_FALLBACK;
    g_status.last_result = result;
    g_status.load_elapsed_ms =
        font_cache_service_platform_tick() - started;
}

void font_cache_service_init(void)
{
    memset(&g_status, 0, sizeof(g_status));
    memset(&g_active, 0, sizeof(g_active));
    g_status.state = FONT_CACHE_SERVICE_IDLE;
    g_status.active_slot = FONT_CACHE_SERVICE_NO_SLOT;
    g_active_valid = 0U;
    g_source_valid = 0U;
    g_source_crc = 0U;
    g_load_progress = NULL;
    g_load_progress_context = NULL;
}

void font_cache_service_set_load_progress_callback(
    font_cache_service_progress_fn callback, void *context)
{
    g_load_progress = callback;
    g_load_progress_context = context;
}

int font_cache_service_try_load(void)
{
    uint32_t started = font_cache_service_platform_tick();
    uint32_t corrected_pages = 0U;
    void *arena;
    int result;

    result = font_cache_service_platform_prepare();
    if (result != 0)
    {
        set_fallback(result, started);
        return 0;
    }
    result = font_cache_service_platform_source_crc(&g_source_crc);
    if (result != 0)
    {
        set_fallback(result, started);
        return 0;
    }
    g_source_valid = 1U;
    result = font_cache_service_platform_scan(&g_active);
    if (result != 0)
    {
        set_fallback(result, started);
        return 0;
    }
    g_active_valid = 1U;
    arena = font_cache_service_platform_allocate(g_active.data_length);
    if (arena == NULL)
    {
        set_fallback(FONT_CACHE_SERVICE_ALLOC_ERROR, started);
        return 0;
    }
    result = font_cache_service_platform_load(
        &g_active, arena, &corrected_pages, g_load_progress,
        g_load_progress_context);
    if (result == 0)
    {
        result = font_cache_service_platform_bind(
            arena, g_active.data_length, g_source_crc);
    }
    if (result != 0)
    {
        font_cache_service_platform_free(arena);
        set_fallback(result, started);
        return 0;
    }

    g_status.state = FONT_CACHE_SERVICE_READY_CACHE;
    g_status.active_slot = g_active.slot;
    g_status.sequence = g_active.sequence;
    g_status.package_size = g_active.data_length;
    g_status.corrected_pages = corrected_pages;
    g_status.last_result = 0;
    g_status.load_elapsed_ms =
        font_cache_service_platform_tick() - started;
    return 1;
}

int font_cache_service_rebuild(void)
{
    nand_cache_metadata_t committed;
    uint32_t package_size = 0U;
    uint32_t started = font_cache_service_platform_tick();
    int result;

    if (g_source_valid == 0U)
    {
        result = font_cache_service_platform_source_crc(&g_source_crc);
        if (result != 0)
        {
            g_status.last_result = result;
            return result;
        }
        g_source_valid = 1U;
    }
    result = font_cache_service_platform_rebuild(
        g_source_crc, (g_active_valid != 0U) ? &g_active : NULL,
        g_active_valid, &committed, &package_size);
    font_cache_service_platform_get_rebuild_diagnostics(&g_status.rebuild);
    g_status.rebuild_elapsed_ms =
        font_cache_service_platform_tick() - started;
    g_status.last_result = result;
    if (result != 0)
        return result;

    g_active = committed;
    g_active_valid = 1U;
    g_status.state = FONT_CACHE_SERVICE_READY_REBUILT;
    g_status.active_slot = committed.slot;
    g_status.sequence = committed.sequence;
    g_status.package_size = package_size;
    return 0;
}

int font_cache_service_invalidate(void)
{
    int result = font_cache_service_platform_invalidate();

    g_status.last_result = result;
    if (result != 0)
        return result;
    memset(&g_active, 0, sizeof(g_active));
    g_active_valid = 0U;
    g_status.active_slot = FONT_CACHE_SERVICE_NO_SLOT;
    g_status.sequence = 0U;
    return 0;
}

void font_cache_service_get_status(font_cache_service_status_t *status)
{
    if (status != NULL)
        *status = g_status;
}
