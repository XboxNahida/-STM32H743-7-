#ifndef ADC_MEASUREMENT_SERVICE_H
#define ADC_MEASUREMENT_SERVICE_H

#include <stdint.h>

typedef enum
{
    ADC_MEASUREMENT_OK = 0,
    ADC_MEASUREMENT_NOT_READY,
    ADC_MEASUREMENT_TIMEOUT,
    ADC_MEASUREMENT_CALIBRATION_ERROR,
    ADC_MEASUREMENT_CONVERSION_ERROR,
    ADC_MEASUREMENT_RANGE_ERROR,
    ADC_MEASUREMENT_INVALID_ARG
} adc_measurement_result_t;

typedef struct
{
    uint8_t adc1_calibrated;
    uint8_t adc3_calibrated;
    uint8_t external_valid;
    uint8_t temperature_valid;
    uint8_t temperature_attempt_valid;
    uint16_t external_raw;
    uint16_t temperature_raw;
    uint16_t temperature_attempt_raw;
    uint16_t temperature_cal1;
    uint16_t temperature_cal2;
    uint32_t external_mv;
    int32_t temperature_centi_c;
    int32_t temperature_cal2_temp;
    int32_t result;
    uint32_t elapsed_ms;
    uint32_t read_count;
} adc_measurement_status_t;

void adc_measurement_service_init(void);
int adc_measurement_read_external(uint32_t timeout_ms);
int adc_measurement_read_temperature(uint32_t timeout_ms);
int adc_measurement_read_all(uint32_t timeout_ms);
void adc_measurement_get_status(adc_measurement_status_t *status);
void adc_measurement_print_status(void);

#endif
