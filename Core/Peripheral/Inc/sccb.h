#ifndef SCCB_H
#define SCCB_H

#include <stdint.h>

typedef enum
{
    SCCB_OK = 0,
    SCCB_INVALID_ARG,
    SCCB_NAK,
    SCCB_BUS_ERROR
} sccb_result_t;

void sccb_init(void);
int sccb_write_reg16(uint8_t device, uint16_t reg, uint8_t value);
int sccb_read_reg16(uint8_t device, uint16_t reg, uint8_t *value);

#endif
