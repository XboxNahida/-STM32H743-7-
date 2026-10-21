#include "nand_cache_store.h"

#include <string.h>

#define NAND_CACHE_STORE_METADATA_PAGE_BASE 0x80000000UL
#define NAND_CACHE_STORE_SLOT_BYTES \
    (NAND_CACHE_SLOT_PAGE_COUNT * NAND_CACHE_PAGE_MAIN_SIZE)

static int store_args_valid(const nand_cache_layout_t *layout,
                            const nand_cache_store_ops_t *ops,
                            nand_cache_store_workspace_t *workspace)
{
    return ((layout != NULL) && (ops != NULL) && (workspace != NULL) &&
            (ops->erase_block != NULL) && (ops->program_page != NULL) &&
            (ops->read_page != NULL) && (ops->block_is_bad != NULL)) ? 1 : 0;
}

static uint32_t store_crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t byte_index;
    uint32_t bit_index;

    for (byte_index = 0U; byte_index < length; byte_index++)
    {
        crc ^= data[byte_index];
        for (bit_index = 0U; bit_index < 8U; bit_index++)
        {
            uint32_t mask = (uint32_t)(0U - (crc & 1U));
            crc = (crc >> 1U) ^ (0xEDB88320UL & mask);
        }
    }
    return ~crc;
}

static int decode_page(nand_cache_store_workspace_t *workspace,
                       uint32_t logical_page, uint32_t *valid_length,
                       uint32_t *corrected_pages)
{
    int result = nand_cache_page_decode(logical_page, workspace->verify_main,
                                        workspace->verify_spare, valid_length);

    if (result == NAND_CACHE_PAGE_CORRECTED)
    {
        if (corrected_pages != NULL)
            (*corrected_pages)++;
        return NAND_CACHE_STORE_OK;
    }
    return (result == NAND_CACHE_PAGE_OK) ? NAND_CACHE_STORE_OK :
                                           NAND_CACHE_STORE_PAGE_ERROR;
}

int nand_cache_store_scan(const nand_cache_layout_t *layout,
                          const nand_cache_store_ops_t *ops,
                          nand_cache_store_workspace_t *workspace,
                          nand_cache_metadata_t *active)
{
    nand_cache_metadata_t candidates[NAND_CACHE_METADATA_BLOCKS];
    uint8_t valid[NAND_CACHE_METADATA_BLOCKS] = {0U};
    uint32_t copy;
    uint32_t page;
    uint32_t length;
    uint32_t selected;

    if ((store_args_valid(layout, ops, workspace) == 0) || (active == NULL))
        return NAND_CACHE_STORE_INVALID_ARG;

    for (copy = 0U; copy < NAND_CACHE_METADATA_BLOCKS; copy++)
    {
        nand_cache_slot_metadata_page_address(layout, copy, &page);
        if (ops->read_page(page, workspace->verify_main,
                           workspace->verify_spare, ops->context) != 0)
            continue;
        if (decode_page(workspace,
                        NAND_CACHE_STORE_METADATA_PAGE_BASE + copy,
                        &length, NULL) != NAND_CACHE_STORE_OK)
            continue;
        if ((length < NAND_CACHE_METADATA_RECORD_SIZE) ||
            (nand_cache_metadata_decode(workspace->verify_main,
                                        NAND_CACHE_METADATA_RECORD_SIZE,
                                        &candidates[copy]) !=
             NAND_CACHE_METADATA_OK))
            continue;
        valid[copy] = 1U;
    }
    if (nand_cache_metadata_select(candidates, valid,
                                   NAND_CACHE_METADATA_BLOCKS, &selected) !=
        NAND_CACHE_METADATA_OK)
        return NAND_CACHE_STORE_NO_VALID_METADATA;
    *active = candidates[selected];
    return NAND_CACHE_STORE_OK;
}

int nand_cache_store_load_progress(
    const nand_cache_layout_t *layout,
    const nand_cache_store_ops_t *ops,
    nand_cache_store_workspace_t *workspace,
    const nand_cache_metadata_t *metadata,
    uint8_t *destination,
    uint32_t capacity,
    uint32_t *corrected_pages,
    nand_cache_store_progress_fn progress,
    void *progress_context)
{
    uint32_t logical_page;
    uint32_t physical_page;
    uint32_t offset;
    uint32_t expected_length;
    uint32_t actual_length;

    if ((store_args_valid(layout, ops, workspace) == 0) ||
        (metadata == NULL) || (destination == NULL) ||
        (capacity < metadata->data_length) ||
        (metadata->slot >= NAND_CACHE_SLOT_COUNT) ||
        (metadata->page_count > NAND_CACHE_SLOT_PAGE_COUNT))
        return NAND_CACHE_STORE_INVALID_ARG;
    if (corrected_pages != NULL)
        *corrected_pages = 0U;

    for (logical_page = 0U; logical_page < metadata->page_count; logical_page++)
    {
        offset = logical_page * NAND_CACHE_PAGE_MAIN_SIZE;
        expected_length = metadata->data_length - offset;
        if (expected_length > NAND_CACHE_PAGE_MAIN_SIZE)
            expected_length = NAND_CACHE_PAGE_MAIN_SIZE;
        nand_cache_slot_page_address(layout, metadata->slot, logical_page,
                                     &physical_page);
        if (ops->read_page(physical_page, workspace->verify_main,
                           workspace->verify_spare, ops->context) != 0)
            return NAND_CACHE_STORE_READ_ERROR;
        if (decode_page(workspace, logical_page, &actual_length,
                        corrected_pages) != NAND_CACHE_STORE_OK)
            return NAND_CACHE_STORE_PAGE_ERROR;
        if (actual_length != expected_length)
            return NAND_CACHE_STORE_VERIFY_ERROR;
        memcpy(&destination[offset], workspace->verify_main, actual_length);
        if (progress != NULL)
            progress(logical_page + 1U, metadata->page_count,
                     progress_context);
    }
    if (store_crc32(destination, metadata->data_length) != metadata->data_crc)
        return NAND_CACHE_STORE_CRC_ERROR;
    return NAND_CACHE_STORE_OK;
}

int nand_cache_store_load(const nand_cache_layout_t *layout,
                          const nand_cache_store_ops_t *ops,
                          nand_cache_store_workspace_t *workspace,
                          const nand_cache_metadata_t *metadata,
                          uint8_t *destination,
                          uint32_t capacity,
                          uint32_t *corrected_pages)
{
    return nand_cache_store_load_progress(
        layout, ops, workspace, metadata, destination, capacity,
        corrected_pages, NULL, NULL);
}
