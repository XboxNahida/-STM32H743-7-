#ifndef NAND_SCAN_SERVICE_H
#define NAND_SCAN_SERVICE_H

#include <stdint.h>

typedef enum
{
    NAND_SCAN_IDLE = 0,
    NAND_SCAN_RUNNING,
    NAND_SCAN_COMPLETE,
    NAND_SCAN_STOPPED,
    NAND_SCAN_ERROR
} nand_scan_state_t;

typedef struct
{
    nand_scan_state_t state;
    uint32_t total_blocks;
    uint32_t scanned_blocks;
    uint32_t bad_blocks;
    uint32_t read_errors;
    uint32_t timeout_errors;
    uint32_t first_error_block;
    int32_t last_result;
} nand_scan_status_t;

void nand_scan_init(void);
int nand_scan_start(void);
int nand_scan_stop(void);
void nand_scan_poll(void);
void nand_scan_get_status(nand_scan_status_t *status);
void nand_scan_print_status(void);

#endif
