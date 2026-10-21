#ifndef ST480MC_SERVICE_H
#define ST480MC_SERVICE_H

#include <stdint.h>

#include "board_i2c.h"

typedef enum
{
    ST480MC_RESULT_OK = 0,
    ST480MC_RESULT_ABSENT,
    ST480MC_RESULT_NOT_READY,
    ST480MC_RESULT_TIMEOUT,
    ST480MC_RESULT_DEVICE_ERROR,
    ST480MC_RESULT_PROTOCOL_ERROR,
    ST480MC_RESULT_ERROR,
    ST480MC_RESULT_INVALID_ARG
} st480mc_result_t;

typedef struct
{
    uint8_t present;
    uint8_t ready;
    uint8_t status_byte;
    uint16_t reg0_value;
    int32_t result;
    uint32_t elapsed_ms;
    uint32_t probe_count;
} st480mc_status_t;

void st480mc_service_init(void);
int st480mc_probe_readonly(uint32_t timeout_ms);
void st480mc_get_status(st480mc_status_t *status);
void st480mc_print_status(void);

#endif
