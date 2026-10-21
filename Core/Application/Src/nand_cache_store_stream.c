#include "nand_cache_store.h"

#include <string.h>

#define STORE_METADATA_PAGE_BASE 0x80000000UL
#define STORE_SLOT_BYTES \
    (NAND_CACHE_SLOT_PAGE_COUNT * NAND_CACHE_PAGE_MAIN_SIZE)

typedef struct
{
    const uint8_t *data;
    uint32_t length;
} memory_reader_t;

static uint32_t crc_update(uint32_t crc, const uint8_t *data, uint32_t length)
{
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
    return crc;
}

static int args_valid(const nand_cache_layout_t *layout,
                      const nand_cache_store_ops_t *ops,
                      nand_cache_store_workspace_t *workspace)
{
    return ((layout != NULL) && (ops != NULL) && (workspace != NULL) &&
            (ops->erase_block != NULL) && (ops->program_page != NULL) &&
            (ops->read_page != NULL) && (ops->block_is_bad != NULL)) ? 1 : 0;
}

static int write_verified_page(const nand_cache_store_ops_t *ops,
                               nand_cache_store_workspace_t *workspace,
                               uint32_t physical_page, uint32_t logical_page,
                               const uint8_t *data, uint32_t length)
{
    uint32_t verified_length;
    int result;

    if (nand_cache_page_encode(logical_page, data, length,
                               workspace->main_area,
                               workspace->spare_area) != NAND_CACHE_PAGE_OK)
        return NAND_CACHE_STORE_INVALID_ARG;
    if (ops->program_page(physical_page, workspace->main_area,
                          workspace->spare_area, ops->context) != 0)
        return NAND_CACHE_STORE_PROGRAM_ERROR;
    if (ops->read_page(physical_page, workspace->verify_main,
                       workspace->verify_spare, ops->context) != 0)
        return NAND_CACHE_STORE_READ_ERROR;
    result = nand_cache_page_decode(logical_page, workspace->verify_main,
                                    workspace->verify_spare, &verified_length);
    if ((result != NAND_CACHE_PAGE_OK) &&
        (result != NAND_CACHE_PAGE_CORRECTED))
        return NAND_CACHE_STORE_PAGE_ERROR;
    if ((verified_length != length) ||
        (memcmp(workspace->verify_main, workspace->main_area, length) != 0))
        return NAND_CACHE_STORE_VERIFY_ERROR;
    return NAND_CACHE_STORE_OK;
}

static int read_crc(nand_cache_store_workspace_t *workspace, uint32_t length,
                    nand_cache_store_reader_fn reader, void *context,
                    uint32_t *data_crc)
{
    uint32_t offset;
    uint32_t chunk;
    uint32_t crc = 0xFFFFFFFFUL;

    for (offset = 0U; offset < length; offset += chunk)
    {
        chunk = length - offset;
        if (chunk > NAND_CACHE_PAGE_MAIN_SIZE)
            chunk = NAND_CACHE_PAGE_MAIN_SIZE;
        if (reader(offset, workspace->verify_main, chunk, context) != 0)
            return NAND_CACHE_STORE_READ_ERROR;
        crc = crc_update(crc, workspace->verify_main, chunk);
    }
    *data_crc = ~crc;
    return NAND_CACHE_STORE_OK;
}

int nand_cache_store_write_stream(const nand_cache_layout_t *layout,
                                  const nand_cache_store_ops_t *ops,
                                  nand_cache_store_workspace_t *workspace,
                                  const nand_cache_metadata_t *active,
                                  uint8_t active_valid, uint32_t length,
                                  nand_cache_store_reader_fn reader,
                                  void *reader_context,
                                  nand_cache_metadata_t *committed)
{
    nand_cache_metadata_t next;
    uint8_t record[NAND_CACHE_METADATA_RECORD_SIZE];
    uint32_t block_index;
    uint32_t logical_page;
    uint32_t physical_page;
    uint32_t offset;
    uint32_t chunk;
    uint32_t metadata_copy;
    int result;

    if ((args_valid(layout, ops, workspace) == 0) || (reader == NULL) ||
        (committed == NULL) || (length == 0U) ||
        (length > STORE_SLOT_BYTES) ||
        ((active_valid != 0U) && (active == NULL)))
        return NAND_CACHE_STORE_INVALID_ARG;
    next.slot = (active_valid != 0U) ? (uint8_t)(active->slot ^ 1U) : 0U;
    next.sequence = (active_valid != 0U) ? active->sequence + 1U : 1U;
    next.data_length = length;
    next.page_count = (length + NAND_CACHE_PAGE_MAIN_SIZE - 1U) /
                      NAND_CACHE_PAGE_MAIN_SIZE;
    result = read_crc(workspace, length, reader, reader_context,
                      &next.data_crc);
    if (result != NAND_CACHE_STORE_OK)
        return result;

    for (block_index = 0U; block_index < NAND_CACHE_SLOT_BLOCKS; block_index++)
    {
        result = ops->block_is_bad(
            layout->slot_blocks[next.slot][block_index], ops->context);
        if (result != 0)
            return (result > 0) ? NAND_CACHE_STORE_BAD_BLOCK :
                                  NAND_CACHE_STORE_READ_ERROR;
    }
    for (block_index = 0U; block_index < NAND_CACHE_SLOT_BLOCKS; block_index++)
    {
        if (ops->erase_block(layout->slot_blocks[next.slot][block_index],
                             ops->context) != 0)
            return NAND_CACHE_STORE_ERASE_ERROR;
    }
    for (logical_page = 0U; logical_page < next.page_count; logical_page++)
    {
        offset = logical_page * NAND_CACHE_PAGE_MAIN_SIZE;
        chunk = length - offset;
        if (chunk > NAND_CACHE_PAGE_MAIN_SIZE)
            chunk = NAND_CACHE_PAGE_MAIN_SIZE;
        if (reader(offset, workspace->verify_main, chunk,
                   reader_context) != 0)
            return NAND_CACHE_STORE_READ_ERROR;
        nand_cache_slot_page_address(layout, next.slot, logical_page,
                                     &physical_page);
        result = write_verified_page(ops, workspace, physical_page,
                                     logical_page, workspace->verify_main,
                                     chunk);
        if (result != NAND_CACHE_STORE_OK)
            return result;
    }

    metadata_copy = (next.sequence - 1U) % NAND_CACHE_METADATA_BLOCKS;
    if (ops->erase_block(layout->metadata_blocks[metadata_copy],
                         ops->context) != 0)
        return NAND_CACHE_STORE_ERASE_ERROR;
    nand_cache_metadata_encode(&next, 1U, record, sizeof(record));
    nand_cache_slot_metadata_page_address(layout, metadata_copy,
                                          &physical_page);
    result = write_verified_page(ops, workspace, physical_page,
                                 STORE_METADATA_PAGE_BASE + metadata_copy,
                                 record, sizeof(record));
    if (result != NAND_CACHE_STORE_OK)
        return result;
    *committed = next;
    return NAND_CACHE_STORE_OK;
}

static int memory_read(uint32_t offset, uint8_t *destination,
                       uint32_t length, void *context)
{
    const memory_reader_t *reader = context;

    if ((offset > reader->length) || (length > reader->length - offset))
        return -1;
    memcpy(destination, &reader->data[offset], length);
    return 0;
}

int nand_cache_store_write(const nand_cache_layout_t *layout,
                           const nand_cache_store_ops_t *ops,
                           nand_cache_store_workspace_t *workspace,
                           const nand_cache_metadata_t *active,
                           uint8_t active_valid, const uint8_t *data,
                           uint32_t length, nand_cache_metadata_t *committed)
{
    memory_reader_t reader;

    if (data == NULL)
        return NAND_CACHE_STORE_INVALID_ARG;
    reader.data = data;
    reader.length = length;
    return nand_cache_store_write_stream(
        layout, ops, workspace, active, active_valid, length, memory_read,
        &reader, committed);
}
