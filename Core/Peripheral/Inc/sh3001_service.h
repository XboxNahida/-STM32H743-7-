#ifndef SH3001_SERVICE_H
#define SH3001_SERVICE_H

#include <stdint.h>

#include "board_i2c.h"

typedef enum
{
    SH3001_RESULT_OK = 0,
    SH3001_RESULT_ABSENT,
    SH3001_RESULT_NOT_READY,
    SH3001_RESULT_TIMEOUT,
    SH3001_RESULT_ID_MISMATCH,
    SH3001_RESULT_ERROR,
    SH3001_RESULT_INVALID_ARG
} sh3001_result_t;

typedef struct
{
    uint8_t present;
    uint8_t ready;
    uint8_t chip_id;
    int32_t result;
    uint32_t elapsed_ms;
    uint32_t probe_count;
} sh3001_status_t;

void sh3001_service_init(void);
int sh3001_probe_readonly(uint32_t timeout_ms);
void sh3001_get_status(sh3001_status_t *status);
void sh3001_print_status(void);

#endif
