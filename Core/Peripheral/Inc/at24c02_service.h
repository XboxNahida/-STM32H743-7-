#ifndef AT24C02_SERVICE_H
#define AT24C02_SERVICE_H

#include <stdint.h>

#include "board_i2c.h"

typedef enum
{
    AT24C02_RESULT_OK = 0,
    AT24C02_RESULT_ABSENT,
    AT24C02_RESULT_NOT_READY,
    AT24C02_RESULT_TIMEOUT,
    AT24C02_RESULT_ERROR,
    AT24C02_RESULT_INVALID_ARG
} at24c02_result_t;

typedef struct
{
    uint8_t present;
    uint8_t ready;
    uint16_t last_offset;
    uint16_t last_length;
    int32_t result;
    uint32_t elapsed_ms;
    uint32_t probe_count;
    uint32_t read_count;
    uint32_t write_count;
} at24c02_status_t;

void at24c02_service_init(void);
int at24c02_probe_readonly(uint32_t timeout_ms);
int at24c02_read(uint16_t offset, uint8_t *data, uint16_t length,
                 uint32_t timeout_ms);
int at24c02_write(uint16_t offset, const uint8_t *data, uint16_t length,
                  uint32_t timeout_ms);
void at24c02_get_status(at24c02_status_t *status);
void at24c02_print_status(void);

#endif
