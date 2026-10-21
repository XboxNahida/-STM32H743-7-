#include "font_cache_service.h"

#include <stddef.h>
#include <string.h>

#include "font_bridge.h"
#include "font_cache_blob.h"
#include "font_cache_lvgl.h"
#include "font_cache_source.h"
#include "nand_cache_io.h"
#include "nand_cache_layout.h"
#include "nand_cache_store.h"
#include "nand_readonly.h"
#include "norflash.h"
#include "src/stdlib/builtin/lv_tlsf.h"

#define FONT_CACHE_NAND_TIMEOUT_MS 100U
#define FONT_CACHE_ERASE_TIMEOUT_MS 1000U
#define FONT_CACHE_FONT_COUNT 6U

static nand_cache_layout_t g_layout;
static nand_cache_store_workspace_t g_workspace;
static font_cache_lvgl_plan_t g_font_plan;
static uint8_t g_page_buffer[NAND_CACHE_PAGE_MAIN_SIZE +
                             NAND_CACHE_PAGE_SPARE_SIZE];
static uint8_t g_prepared;
static font_cache_rebuild_diagnostics_t g_rebuild_diagnostics;

static lv_font_t *const g_destinations[FONT_CACHE_FONT_COUNT] =
{
    &lv_font_myyh_12,
    &lv_font_myyh_14,
    &lv_font_myyh_16,
    &lv_font_myyh_18,
    &lv_font_myyh_20,
    &lv_font_myyh_30
};

static const lv_font_t *const g_sources[FONT_CACHE_FONT_COUNT] =
{
    &lv_font_myyh_12,
    &lv_font_myyh_14,
    &lv_font_myyh_16,
    &lv_font_myyh_18,
    &lv_font_myyh_20,
    &lv_font_myyh_30
};

static int layout_bad_block(uint32_t block, void *context)
{
    uint8_t is_bad = 0U;
    int result;

    (void)context;
    result = nand_cache_io_block_is_bad(block, FONT_CACHE_NAND_TIMEOUT_MS,
                                        &is_bad);
    return (result == NAND_CACHE_IO_OK) ? (int)is_bad : -1;
}

static uint8_t metadata_block(uint32_t block)
{
    uint32_t index;

    for (index = 0U; index < NAND_CACHE_METADATA_BLOCKS; index++)
        if (g_layout.metadata_blocks[index] == block)
            return 1U;
    return 0U;
}

static void capture_io(font_cache_rebuild_stage_t stage, uint32_t target,
                       int result)
{
    nand_cache_io_diagnostics_t io;

    nand_cache_io_get_diagnostics(&io);
    g_rebuild_diagnostics.stage = stage;
    g_rebuild_diagnostics.target = target;
    g_rebuild_diagnostics.detail_result = result;
    g_rebuild_diagnostics.nand_status = io.status;
    g_rebuild_diagnostics.nand_status_valid = io.status_valid;
    g_rebuild_diagnostics.nand_ready = io.ready;
}

static int store_erase(uint32_t block, void *context)
{
    int result;

    (void)context;
    result = nand_cache_io_erase_block(block, FONT_CACHE_ERASE_TIMEOUT_MS);
    capture_io((metadata_block(block) != 0U) ?
               FONT_CACHE_REBUILD_STAGE_ERASE_METADATA :
               FONT_CACHE_REBUILD_STAGE_ERASE_DATA, block, result);
    return (result == NAND_CACHE_IO_OK) ? 0 : -1;
}

static int store_program(uint32_t page, const uint8_t *main_area,
                         const uint8_t *spare_area, void *context)
{
    int result;

    (void)context;
    memcpy(g_page_buffer, main_area, NAND_CACHE_PAGE_MAIN_SIZE);
    memcpy(&g_page_buffer[NAND_CACHE_PAGE_MAIN_SIZE], spare_area,
           NAND_CACHE_PAGE_SPARE_SIZE);
    result = nand_cache_io_program_page(page, 0U, g_page_buffer,
                                        sizeof(g_page_buffer),
                                        FONT_CACHE_NAND_TIMEOUT_MS);
    capture_io((metadata_block(page / NAND_CACHE_SLOT_PAGES_PER_BLOCK) != 0U) ?
               FONT_CACHE_REBUILD_STAGE_PROGRAM_METADATA :
               FONT_CACHE_REBUILD_STAGE_PROGRAM_DATA, page, result);
    return (result == NAND_CACHE_IO_OK) ? 0 : -1;
}

static int store_read(uint32_t page, uint8_t *main_area,
                      uint8_t *spare_area, void *context)
{
    int result;

    (void)context;
    result = nand_readonly_read_page(page, 0U, g_page_buffer,
                                     sizeof(g_page_buffer),
                                     FONT_CACHE_NAND_TIMEOUT_MS);
    g_rebuild_diagnostics.stage =
        (metadata_block(page / NAND_CACHE_SLOT_PAGES_PER_BLOCK) != 0U) ?
        FONT_CACHE_REBUILD_STAGE_READBACK_METADATA :
        FONT_CACHE_REBUILD_STAGE_READBACK_DATA;
    g_rebuild_diagnostics.target = page;
    g_rebuild_diagnostics.detail_result = result;
    g_rebuild_diagnostics.nand_status = 0U;
    g_rebuild_diagnostics.nand_status_valid = 0U;
    g_rebuild_diagnostics.nand_ready = 0U;
    if (result != NAND_READONLY_OK)
        return -1;
    memcpy(main_area, g_page_buffer, NAND_CACHE_PAGE_MAIN_SIZE);
    memcpy(spare_area, &g_page_buffer[NAND_CACHE_PAGE_MAIN_SIZE],
           NAND_CACHE_PAGE_SPARE_SIZE);
    return 0;
}

static int store_bad(uint32_t block, void *context)
{
    int result = layout_bad_block(block, context);

    g_rebuild_diagnostics.stage = FONT_CACHE_REBUILD_STAGE_BAD_BLOCK;
    g_rebuild_diagnostics.target = block;
    g_rebuild_diagnostics.detail_result = result;
    g_rebuild_diagnostics.nand_status = 0U;
    g_rebuild_diagnostics.nand_status_valid = 0U;
    g_rebuild_diagnostics.nand_ready = 0U;
    return result;
}

static nand_cache_store_ops_t make_store_ops(void)
{
    nand_cache_store_ops_t ops;

    ops.erase_block = store_erase;
    ops.program_page = store_program;
    ops.read_page = store_read;
    ops.block_is_bad = store_bad;
    ops.context = NULL;
    return ops;
}

int font_cache_service_platform_prepare(void)
{
    nand_geometry_t geometry;
    int result;

    g_prepared = 0U;
    nand_readonly_init();
    result = nand_readonly_probe(FONT_CACHE_NAND_TIMEOUT_MS);
    if (result != NAND_READONLY_OK)
        return result;
    result = nand_readonly_get_geometry(&geometry);
    if (result != NAND_READONLY_OK)
        return result;
    result = nand_cache_layout_build(
        geometry.page_main_size, geometry.pages_per_block,
        geometry.block_count, layout_bad_block, NULL, &g_layout);
    if (result != NAND_CACHE_LAYOUT_OK)
        return result;
    g_prepared = 1U;
    return 0;
}

int font_cache_service_platform_source_crc(uint32_t *source_crc)
{
    font_cache_source_t source[FONT_CACHE_FONT_COUNT];
    uint32_t total_length;

    if (source_crc == NULL)
        return FONT_CACHE_SOURCE_INVALID_ARG;
    norflash_enter_memmap();
    source[0].data = (const uint8_t *)(QSPI_BASE_ADDR + QSPI_FONT12_OFFSET);
    source[0].capacity = QSPI_FONT12_MAX_SIZE;
    source[1].data = (const uint8_t *)(QSPI_BASE_ADDR + QSPI_FONT14_OFFSET);
    source[1].capacity = QSPI_FONT14_MAX_SIZE;
    source[2].data = (const uint8_t *)(QSPI_BASE_ADDR + QSPI_FONT16_OFFSET);
    source[2].capacity = QSPI_FONT16_MAX_SIZE;
    source[3].data = (const uint8_t *)(QSPI_BASE_ADDR + QSPI_FONT18_OFFSET);
    source[3].capacity = QSPI_FONT18_MAX_SIZE;
    source[4].data = (const uint8_t *)(QSPI_BASE_ADDR + QSPI_FONT20_OFFSET);
    source[4].capacity = QSPI_FONT20_MAX_SIZE;
    source[5].data = (const uint8_t *)(QSPI_BASE_ADDR + QSPI_FONT30_OFFSET);
    source[5].capacity = QSPI_FONT30_MAX_SIZE;
    return font_cache_source_collection_crc(source, FONT_CACHE_FONT_COUNT,
                                            source_crc, &total_length);
}

int font_cache_service_platform_scan(nand_cache_metadata_t *metadata)
{
    nand_cache_store_ops_t ops = make_store_ops();

    if (g_prepared == 0U)
        return NAND_CACHE_STORE_INVALID_ARG;
    return nand_cache_store_scan(&g_layout, &ops, &g_workspace, metadata);
}

void *font_cache_service_platform_allocate(uint32_t size)
{
    return lv_malloc(size);
}

void font_cache_service_platform_free(void *pointer)
{
    lv_free(pointer);
}

int font_cache_service_platform_load(const nand_cache_metadata_t *metadata,
                                     uint8_t *destination,
                                     uint32_t *corrected_pages,
                                     font_cache_service_progress_fn progress,
                                     void *progress_context)
{
    nand_cache_store_ops_t ops = make_store_ops();

    if ((g_prepared == 0U) || (metadata == NULL))
        return NAND_CACHE_STORE_INVALID_ARG;
    return nand_cache_store_load_progress(
        &g_layout, &ops, &g_workspace, metadata, destination,
        metadata->data_length, corrected_pages, progress, progress_context);
}

int font_cache_service_platform_bind(uint8_t *package, uint32_t package_size,
                                     uint32_t source_crc)
{
    return font_cache_lvgl_bind(package, package_size, g_destinations,
                                FONT_CACHE_FONT_COUNT, source_crc);
}

static uint32_t allocation_size(const void *pointer, void *context)
{
    size_t size;

    (void)context;
    size = lv_tlsf_block_size((void *)pointer);
    return (size <= UINT32_MAX) ? (uint32_t)size : 0U;
}

static int plan_reader(uint32_t offset, uint8_t *destination,
                       uint32_t length, void *context)
{
    const font_cache_blob_plan_t *plan = context;

    return (font_cache_blob_plan_read(plan, offset, destination, length) ==
            FONT_CACHE_BLOB_OK) ? 0 : -1;
}

int font_cache_service_platform_rebuild(
    uint32_t source_crc, const nand_cache_metadata_t *active,
    uint8_t active_valid, nand_cache_metadata_t *committed,
    uint32_t *package_size)
{
    nand_cache_store_ops_t ops = make_store_ops();
    uint32_t size;
    int result;

    if ((g_prepared == 0U) || (committed == NULL) || (package_size == NULL))
        return NAND_CACHE_STORE_INVALID_ARG;
    memset(&g_rebuild_diagnostics, 0, sizeof(g_rebuild_diagnostics));
    g_rebuild_diagnostics.stage = FONT_CACHE_REBUILD_STAGE_PLAN;
    result = font_cache_lvgl_plan_init(
        &g_font_plan, g_sources, FONT_CACHE_FONT_COUNT, allocation_size,
        NULL, source_crc);
    if (result != FONT_CACHE_LVGL_OK)
    {
        g_rebuild_diagnostics.detail_result = result;
        return result;
    }
    size = font_cache_blob_plan_size(&g_font_plan.blob);
    g_rebuild_diagnostics.stage = FONT_CACHE_REBUILD_STAGE_STORE_CRC;
    result = nand_cache_store_write_stream(
        &g_layout, &ops, &g_workspace, active, active_valid, size,
        plan_reader, &g_font_plan.blob, committed);
    if (result == NAND_CACHE_STORE_OK)
    {
        *package_size = size;
        g_rebuild_diagnostics.stage = FONT_CACHE_REBUILD_STAGE_COMPLETE;
    }
    else if (g_rebuild_diagnostics.detail_result == 0)
        g_rebuild_diagnostics.detail_result = result;
    return result;
}

void font_cache_service_platform_get_rebuild_diagnostics(
    font_cache_rebuild_diagnostics_t *diagnostics)
{
    if (diagnostics != NULL)
        *diagnostics = g_rebuild_diagnostics;
}

int font_cache_service_platform_invalidate(void)
{
    uint32_t index;

    if (g_prepared == 0U)
        return NAND_CACHE_STORE_INVALID_ARG;
    for (index = 0U; index < NAND_CACHE_METADATA_BLOCKS; index++)
    {
        if (store_erase(g_layout.metadata_blocks[index], NULL) != 0)
            return NAND_CACHE_STORE_ERASE_ERROR;
    }
    return NAND_CACHE_STORE_OK;
}

uint32_t font_cache_service_platform_tick(void)
{
    return HAL_GetTick();
}
