#ifndef NAND_CACHE_IO_H
#define NAND_CACHE_IO_H

#include <stdint.h>

typedef enum
{
    NAND_CACHE_IO_OK = 0,
    NAND_CACHE_IO_INVALID_ARG,
    NAND_CACHE_IO_NOT_READY,
    NAND_CACHE_IO_TIMEOUT,
    NAND_CACHE_IO_READ_ERROR,
    NAND_CACHE_IO_PROGRAM_FAILED,
    NAND_CACHE_IO_ERASE_FAILED
} nand_cache_io_result_t;

typedef enum
{
    NAND_CACHE_IO_OPERATION_NONE = 0,
    NAND_CACHE_IO_OPERATION_PROGRAM,
    NAND_CACHE_IO_OPERATION_ERASE
} nand_cache_io_operation_t;

typedef struct
{
    nand_cache_io_operation_t operation;
    uint32_t target;
    int32_t result;
    uint8_t status;
    uint8_t status_valid;
    uint8_t ready;
} nand_cache_io_diagnostics_t;

int nand_cache_io_program_page(uint32_t page,
                               uint32_t column,
                               const uint8_t *data,
                               uint32_t length,
                               uint32_t timeout_ms);
int nand_cache_io_erase_block(uint32_t block, uint32_t timeout_ms);
int nand_cache_io_block_is_bad(uint32_t block,
                               uint32_t timeout_ms,
                               uint8_t *is_bad);
void nand_cache_io_get_diagnostics(nand_cache_io_diagnostics_t *diagnostics);

#endif
