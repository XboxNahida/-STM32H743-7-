#ifndef NAND_READONLY_H
#define NAND_READONLY_H

#include <stdint.h>

typedef enum
{
    NAND_READONLY_OK = 0,
    NAND_READONLY_INVALID_ARG,
    NAND_READONLY_NOT_INITIALIZED,
    NAND_READONLY_NOT_PRESENT,
    NAND_READONLY_UNKNOWN_ID,
    NAND_READONLY_TIMEOUT,
    NAND_READONLY_IO_ERROR,
    NAND_READONLY_BUSY,
    NAND_READONLY_STOPPED
} nand_readonly_result_t;

typedef struct
{
    uint8_t bytes[5];
    uint32_t compact_id;
} nand_id_t;

typedef struct
{
    uint32_t page_main_size;
    uint32_t page_spare_size;
    uint32_t pages_per_block;
    uint32_t block_count;
    uint8_t row_address_cycles;
} nand_geometry_t;

void nand_readonly_init(void);
int nand_readonly_probe(uint32_t timeout_ms);
int nand_readonly_get_id(nand_id_t *id);
int nand_readonly_get_geometry(nand_geometry_t *geometry);
int nand_readonly_read_status(uint8_t *status);
int nand_readonly_read_page(uint32_t page,
                            uint32_t column,
                            uint8_t *buffer,
                            uint32_t length,
                            uint32_t timeout_ms);

#endif
