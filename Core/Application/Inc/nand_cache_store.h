#ifndef NAND_CACHE_STORE_H
#define NAND_CACHE_STORE_H

#include <stdint.h>

#include "nand_cache_metadata.h"
#include "nand_cache_page.h"
#include "nand_cache_slot.h"

typedef enum
{
    NAND_CACHE_STORE_OK = 0,
    NAND_CACHE_STORE_INVALID_ARG,
    NAND_CACHE_STORE_NO_VALID_METADATA,
    NAND_CACHE_STORE_BAD_BLOCK,
    NAND_CACHE_STORE_ERASE_ERROR,
    NAND_CACHE_STORE_PROGRAM_ERROR,
    NAND_CACHE_STORE_READ_ERROR,
    NAND_CACHE_STORE_PAGE_ERROR,
    NAND_CACHE_STORE_VERIFY_ERROR,
    NAND_CACHE_STORE_CRC_ERROR
} nand_cache_store_result_t;

typedef int (*nand_cache_store_erase_fn)(uint32_t block, void *context);
typedef int (*nand_cache_store_program_fn)(uint32_t page,
                                           const uint8_t *main_area,
                                           const uint8_t *spare_area,
                                           void *context);
typedef int (*nand_cache_store_read_fn)(uint32_t page,
                                        uint8_t *main_area,
                                        uint8_t *spare_area,
                                        void *context);
typedef int (*nand_cache_store_bad_block_fn)(uint32_t block, void *context);
typedef int (*nand_cache_store_reader_fn)(uint32_t offset,
                                          uint8_t *destination,
                                          uint32_t length,
                                          void *context);
typedef void (*nand_cache_store_progress_fn)(uint32_t completed_pages,
                                             uint32_t total_pages,
                                             void *context);

typedef struct
{
    nand_cache_store_erase_fn erase_block;
    nand_cache_store_program_fn program_page;
    nand_cache_store_read_fn read_page;
    nand_cache_store_bad_block_fn block_is_bad;
    void *context;
} nand_cache_store_ops_t;

typedef struct
{
    uint8_t main_area[NAND_CACHE_PAGE_MAIN_SIZE];
    uint8_t spare_area[NAND_CACHE_PAGE_SPARE_SIZE];
    uint8_t verify_main[NAND_CACHE_PAGE_MAIN_SIZE];
    uint8_t verify_spare[NAND_CACHE_PAGE_SPARE_SIZE];
} nand_cache_store_workspace_t;

int nand_cache_store_scan(const nand_cache_layout_t *layout,
                          const nand_cache_store_ops_t *ops,
                          nand_cache_store_workspace_t *workspace,
                          nand_cache_metadata_t *active);
int nand_cache_store_write(const nand_cache_layout_t *layout,
                           const nand_cache_store_ops_t *ops,
                           nand_cache_store_workspace_t *workspace,
                           const nand_cache_metadata_t *active,
                           uint8_t active_valid,
                           const uint8_t *data,
                           uint32_t length,
                           nand_cache_metadata_t *committed);
int nand_cache_store_write_stream(const nand_cache_layout_t *layout,
                                  const nand_cache_store_ops_t *ops,
                                  nand_cache_store_workspace_t *workspace,
                                  const nand_cache_metadata_t *active,
                                  uint8_t active_valid,
                                  uint32_t length,
                                  nand_cache_store_reader_fn reader,
                                  void *reader_context,
                                  nand_cache_metadata_t *committed);
int nand_cache_store_load(const nand_cache_layout_t *layout,
                          const nand_cache_store_ops_t *ops,
                          nand_cache_store_workspace_t *workspace,
                          const nand_cache_metadata_t *metadata,
                          uint8_t *destination,
                          uint32_t capacity,
                          uint32_t *corrected_pages);
int nand_cache_store_load_progress(
    const nand_cache_layout_t *layout,
    const nand_cache_store_ops_t *ops,
    nand_cache_store_workspace_t *workspace,
    const nand_cache_metadata_t *metadata,
    uint8_t *destination,
    uint32_t capacity,
    uint32_t *corrected_pages,
    nand_cache_store_progress_fn progress,
    void *progress_context);

#endif
