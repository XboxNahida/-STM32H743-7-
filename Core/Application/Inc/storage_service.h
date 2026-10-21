#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H

#include <stdint.h>

typedef enum
{
    STORAGE_SD = 0,
    STORAGE_NAND,
    STORAGE_QSPI,
    STORAGE_COUNT
} storage_id_t;

typedef struct
{
    uint8_t present;
    uint8_t ready;
    int32_t last_result;
    uint32_t error_count;
    uint32_t detail0;
    uint32_t detail1;
} storage_status_t;

typedef enum
{
    STORAGE_OK = 0,
    STORAGE_INVALID_ARG,
    STORAGE_NOT_READY,
    STORAGE_NOT_IMPLEMENTED,
    STORAGE_IO_ERROR
} storage_result_t;

int storage_get_status(storage_id_t id, storage_status_t *status);
int storage_test_sd(uint32_t timeout_ms);
void storage_print_status(void);

#endif
