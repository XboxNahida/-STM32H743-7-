#ifndef OV5640_REGISTERS_H
#define OV5640_REGISTERS_H

#include <stdint.h>

typedef struct
{
    uint16_t reg;
    uint8_t value;
} ov5640_reg_value_t;

extern const ov5640_reg_value_t g_ov5640_init_regs[];
extern const uint32_t g_ov5640_init_reg_count;
extern const ov5640_reg_value_t g_ov5640_rgb565_regs[];
extern const uint32_t g_ov5640_rgb565_reg_count;

#endif
