#ifndef OV5640_SERVICE_H
#define OV5640_SERVICE_H

#include <stdint.h>

typedef enum
{
    OV5640_OK = 0,
    OV5640_INVALID_ARG,
    OV5640_NOT_PRESENT,
    OV5640_IO_ERROR
} ov5640_result_t;

typedef struct
{
    uint8_t present;
    uint8_t ready;
    uint8_t mirror;
    uint8_t flip;
    uint8_t flash;
    uint8_t test_pattern;
    uint16_t chip_id;
    uint16_t width;
    uint16_t height;
    int last_result;
} ov5640_status_t;

void ov5640_service_init(void);
int ov5640_power_on(void);
int ov5640_power_off(void);
int ov5640_probe(void);
int ov5640_configure_rgb565_600x400(void);
int ov5640_set_mirror(uint8_t enabled);
int ov5640_set_flip(uint8_t enabled);
int ov5640_set_flash(uint8_t enabled);
int ov5640_set_test_pattern(uint8_t enabled);
void ov5640_get_status(ov5640_status_t *status);

#endif
