#ifndef AP3216C_SERVICE_H
#define AP3216C_SERVICE_H

#include <stdint.h>

#include "board_i2c.h"

typedef enum
{
    AP3216C_RESULT_OK = 0,
    AP3216C_RESULT_ABSENT,
    AP3216C_RESULT_NOT_READY,
    AP3216C_RESULT_TIMEOUT,
    AP3216C_RESULT_PROTOCOL_ERROR,
    AP3216C_RESULT_ERROR,
    AP3216C_RESULT_INVALID_ARG
} ap3216c_result_t;

typedef struct
{
    uint8_t present;
    uint8_t ready;
    uint8_t initialized;
    uint8_t system_mode;
    uint8_t ir_valid;
    uint8_t proximity_valid;
    uint8_t proximity_near;
    uint16_t ir_raw;
    uint16_t als_raw;
    uint16_t proximity_raw;
    uint32_t lux_milli;
    int32_t result;
    uint32_t elapsed_ms;
    uint32_t probe_count;
    uint32_t read_count;
} ap3216c_status_t;

void ap3216c_service_init(void);
int ap3216c_probe_readonly(uint32_t timeout_ms);
int ap3216c_read_once(uint32_t timeout_ms);
void ap3216c_get_status(ap3216c_status_t *status);
void ap3216c_print_status(void);

#endif
