#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "font_cache_service.h"

static uint8_t g_arena[512];
static int g_prepare_result;
static int g_source_result;
static int g_scan_result;
static int g_load_result;
static int g_bind_result;
static int g_rebuild_result;
static int g_invalidate_result;
static uint32_t g_invalidate_count;
static uint32_t g_alloc_count;
static uint32_t g_free_count;
static uint32_t g_tick;
static uint32_t g_progress_calls;
static font_cache_rebuild_diagnostics_t g_rebuild_diagnostics;

static void record_progress(uint32_t completed, uint32_t total, void *context)
{
    uint32_t *last_completed = (uint32_t *)context;

    g_progress_calls++;
    *last_completed = completed;
    if (total != 1U)
        g_progress_calls += 100U;
}

int font_cache_service_platform_prepare(void)
{
    return g_prepare_result;
}

int font_cache_service_platform_source_crc(uint32_t *source_crc)
{
    if (g_source_result == 0)
        *source_crc = 0x12345678UL;
    return g_source_result;
}

int font_cache_service_platform_scan(nand_cache_metadata_t *metadata)
{
    if (g_scan_result == 0)
    {
        memset(metadata, 0, sizeof(*metadata));
        metadata->slot = 1U;
        metadata->sequence = 9U;
        metadata->data_length = 256U;
        metadata->page_count = 1U;
    }
    return g_scan_result;
}

void *font_cache_service_platform_allocate(uint32_t size)
{
    g_alloc_count++;
    return (size <= sizeof(g_arena)) ? g_arena : NULL;
}

void font_cache_service_platform_free(void *pointer)
{
    if (pointer != NULL)
        g_free_count++;
}

int font_cache_service_platform_load(const nand_cache_metadata_t *metadata,
                                     uint8_t *destination,
                                     uint32_t *corrected_pages,
                                     font_cache_service_progress_fn progress,
                                     void *progress_context)
{
    (void)metadata;
    memset(destination, 0x5A, 256U);
    *corrected_pages = 2U;
    if (progress != NULL)
        progress(1U, 1U, progress_context);
    return g_load_result;
}

int font_cache_service_platform_bind(uint8_t *package, uint32_t package_size,
                                     uint32_t source_crc)
{
    if ((package != g_arena) || (package_size != 256U) ||
        (source_crc != 0x12345678UL))
        return -1;
    return g_bind_result;
}

int font_cache_service_platform_rebuild(
    uint32_t source_crc, const nand_cache_metadata_t *active,
    uint8_t active_valid, nand_cache_metadata_t *committed,
    uint32_t *package_size)
{
    if ((source_crc != 0x12345678UL) ||
        ((active_valid != 0U) && (active == NULL)))
        return -1;
    if (g_rebuild_result == 0)
    {
        memset(committed, 0, sizeof(*committed));
        committed->slot = (active_valid != 0U) ?
                          (uint8_t)(active->slot ^ 1U) : 0U;
        committed->sequence = (active_valid != 0U) ?
                              active->sequence + 1U : 1U;
        *package_size = 300U;
    }
    return g_rebuild_result;
}

void font_cache_service_platform_get_rebuild_diagnostics(
    font_cache_rebuild_diagnostics_t *diagnostics)
{
    *diagnostics = g_rebuild_diagnostics;
}

int font_cache_service_platform_invalidate(void)
{
    g_invalidate_count++;
    return g_invalidate_result;
}

uint32_t font_cache_service_platform_tick(void)
{
    return g_tick++;
}

static void reset_fakes(void)
{
    g_prepare_result = 0;
    g_source_result = 0;
    g_scan_result = 0;
    g_load_result = 0;
    g_bind_result = 0;
    g_rebuild_result = 0;
    g_invalidate_result = 0;
    g_invalidate_count = 0U;
    g_alloc_count = 0U;
    g_free_count = 0U;
    g_progress_calls = 0U;
    g_tick = 100U;
    memset(&g_rebuild_diagnostics, 0, sizeof(g_rebuild_diagnostics));
}

static int expect(const char *name, int actual, int expected)
{
    if (actual == expected)
        return 0;
    printf("FAIL %s actual=%d expected=%d\n", name, actual, expected);
    return 1;
}

int main(void)
{
    font_cache_service_status_t status;
    uint32_t last_completed = 0U;
    int failures = 0;

    reset_fakes();
    font_cache_service_init();
    font_cache_service_set_load_progress_callback(record_progress,
                                                  &last_completed);
    failures += expect("cache_hit", font_cache_service_try_load(), 1);
    font_cache_service_get_status(&status);
    if ((status.state != FONT_CACHE_SERVICE_READY_CACHE) ||
        (status.active_slot != 1U) || (status.sequence != 9U) ||
        (status.package_size != 256U) || (status.corrected_pages != 2U) ||
        (g_alloc_count != 1U) || (g_free_count != 0U) ||
        (g_progress_calls != 1U) || (last_completed != 1U))
    {
        printf("FAIL cache_hit_status\n");
        failures++;
    }

    reset_fakes();
    g_scan_result = 2;
    font_cache_service_init();
    failures += expect("empty_fallback", font_cache_service_try_load(), 0);
    font_cache_service_get_status(&status);
    if ((status.state != FONT_CACHE_SERVICE_FALLBACK) ||
        (g_alloc_count != 0U) || (g_free_count != 0U))
    {
        printf("FAIL empty_fallback_status\n");
        failures++;
    }
    failures += expect("rebuild_after_fallback",
                       font_cache_service_rebuild(), 0);
    font_cache_service_get_status(&status);
    if ((status.state != FONT_CACHE_SERVICE_READY_REBUILT) ||
        (status.active_slot != 0U) || (status.sequence != 1U) ||
        (status.package_size != 300U))
    {
        printf("FAIL rebuilt_status\n");
        failures++;
    }

    reset_fakes();
    g_load_result = 7;
    font_cache_service_init();
    failures += expect("damaged_fallback", font_cache_service_try_load(), 0);
    if ((g_alloc_count != 1U) || (g_free_count != 1U))
    {
        printf("FAIL damaged_arena_release\n");
        failures++;
    }

    reset_fakes();
    font_cache_service_init();
    failures += expect("invalidate_load", font_cache_service_try_load(), 1);
    failures += expect("invalidate", font_cache_service_invalidate(), 0);
    font_cache_service_get_status(&status);
    if ((g_invalidate_count != 1U) || (status.active_slot != 0xFFU) ||
        (status.sequence != 0U) || (g_free_count != 0U))
    {
        printf("FAIL invalidate_status\n");
        failures++;
    }
    failures += expect("rebuild_after_invalidate",
                       font_cache_service_rebuild(), 0);
    font_cache_service_get_status(&status);
    if ((status.active_slot != 0U) || (status.sequence != 1U))
    {
        printf("FAIL rebuild_after_invalidate_status\n");
        failures++;
    }

    g_invalidate_result = 5;
    failures += expect("invalidate_error", font_cache_service_invalidate(), 5);

    reset_fakes();
    g_rebuild_result = 4;
    g_rebuild_diagnostics.stage = FONT_CACHE_REBUILD_STAGE_ERASE_DATA;
    g_rebuild_diagnostics.target = 37U;
    g_rebuild_diagnostics.detail_result = 6;
    g_rebuild_diagnostics.nand_status = 0x41U;
    g_rebuild_diagnostics.nand_status_valid = 1U;
    g_rebuild_diagnostics.nand_ready = 1U;
    font_cache_service_init();
    failures += expect("diagnostic_load", font_cache_service_try_load(), 1);
    failures += expect("diagnostic_rebuild", font_cache_service_rebuild(), 4);
    font_cache_service_get_status(&status);
    if ((status.rebuild.stage != FONT_CACHE_REBUILD_STAGE_ERASE_DATA) ||
        (status.rebuild.target != 37U) ||
        (status.rebuild.detail_result != 6) ||
        (status.rebuild.nand_status != 0x41U) ||
        (status.rebuild.nand_status_valid != 1U) ||
        (status.rebuild.nand_ready != 1U))
    {
        printf("FAIL rebuild_diagnostics_status\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL font_cache_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS font_cache_service\n");
    return 0;
}
