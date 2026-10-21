#include "boot_probe_service.h"

#include <string.h>

#include "adc.h"
#include "ap3216c_service.h"
#include "at24c02_service.h"
#include "dma2d.h"
#include "fmc.h"
#include "gt9xxx.h"
#include "imu_service.h"
#include "pressure_service.h"
#include "i2c.h"
#include "jpeg.h"
#include "ltdc.h"
#include "nand_readonly.h"
#include "norflash.h"
#include "pcf8574.h"
#include "quadspi.h"
#include "rtc.h"
#include "sai.h"
#include "sdmmc.h"
#include "spdifrx.h"
#include "storage_service.h"
#include "sh3001_service.h"
#include "tim.h"
#include "usart.h"

static int boot_probe_report_state(boot_probe_status_t *status,
                                   uint32_t state,
                                   uint32_t ready_state)
{
    status->present = 1U;
    status->ready = (state == ready_state) ? 1U : 0U;
    status->detail0 = state;
    return (status->ready != 0U) ? BOOT_PROBE_OK : BOOT_PROBE_NOT_READY;
}

#define BOOT_PROBE_DEFINE_STATE_CALLBACK(function_name, handle, ready_state) \
    static int function_name(boot_probe_status_t *status)                    \
    {                                                                         \
        return boot_probe_report_state(status, (uint32_t)(handle).State,      \
                                       (uint32_t)(ready_state));              \
    }

BOOT_PROBE_DEFINE_STATE_CALLBACK(boot_probe_i2c2, hi2c2,
                                 HAL_I2C_STATE_READY)
BOOT_PROBE_DEFINE_STATE_CALLBACK(boot_probe_sdmmc1, hsd1,
                                 HAL_SD_STATE_READY)
BOOT_PROBE_DEFINE_STATE_CALLBACK(boot_probe_fmc_sdram, hsdram1,
                                 HAL_SDRAM_STATE_READY)
BOOT_PROBE_DEFINE_STATE_CALLBACK(boot_probe_ltdc, hltdc,
                                 HAL_LTDC_STATE_READY)
BOOT_PROBE_DEFINE_STATE_CALLBACK(boot_probe_dma2d, hdma2d,
                                 HAL_DMA2D_STATE_READY)
BOOT_PROBE_DEFINE_STATE_CALLBACK(boot_probe_jpeg, hjpeg,
                                 HAL_JPEG_STATE_READY)
BOOT_PROBE_DEFINE_STATE_CALLBACK(boot_probe_rtc, hrtc,
                                 HAL_RTC_STATE_READY)
BOOT_PROBE_DEFINE_STATE_CALLBACK(boot_probe_spdifrx1, hspdif1,
                                 HAL_SPDIFRX_STATE_READY)

static int boot_probe_uart1(boot_probe_status_t *status)
{
    uint32_t global_state = (uint32_t)huart1.gState;
    uint32_t receive_state = (uint32_t)huart1.RxState;

    status->present = 1U;
    status->ready = ((global_state == (uint32_t)HAL_UART_STATE_READY) &&
                     ((receive_state == (uint32_t)HAL_UART_STATE_READY) ||
                      (receive_state == (uint32_t)HAL_UART_STATE_BUSY_RX))) ?
                    1U : 0U;
    status->detail0 = global_state;
    status->detail1 = receive_state;
    return (status->ready != 0U) ? BOOT_PROBE_OK : BOOT_PROBE_NOT_READY;
}

static int boot_probe_tim_state(boot_probe_status_t *status,
                                const TIM_HandleTypeDef *handle)
{
    uint32_t state = (uint32_t)handle->State;

    status->present = 1U;
    status->ready = ((state == (uint32_t)HAL_TIM_STATE_READY) ||
                     (state == (uint32_t)HAL_TIM_STATE_BUSY)) ? 1U : 0U;
    status->detail0 = state;
    if (status->ready != 0U)
    {
        return BOOT_PROBE_OK;
    }
    if (state == (uint32_t)HAL_TIM_STATE_TIMEOUT)
    {
        return BOOT_PROBE_TIMEOUT;
    }
    return (state == (uint32_t)HAL_TIM_STATE_ERROR) ?
           BOOT_PROBE_ERROR : BOOT_PROBE_NOT_READY;
}

static int boot_probe_tim1(boot_probe_status_t *status)
{
    return boot_probe_tim_state(status, &htim1);
}

static int boot_probe_tim2(boot_probe_status_t *status)
{
    return boot_probe_tim_state(status, &htim2);
}

static int boot_probe_tim3(boot_probe_status_t *status)
{
    return boot_probe_tim_state(status, &htim3);
}

static int boot_probe_tim6(boot_probe_status_t *status)
{
    return boot_probe_tim_state(status, &htim6);
}

static int boot_probe_qspi(boot_probe_status_t *status)
{
    uint32_t state = (uint32_t)hqspi.State;

    status->present = 1U;
    status->ready = ((state == (uint32_t)HAL_QSPI_STATE_READY) ||
                     (state == (uint32_t)HAL_QSPI_STATE_BUSY_MEM_MAPPED)) ?
                    1U : 0U;
    status->detail0 = state;
    return (status->ready != 0U) ? BOOT_PROBE_OK : BOOT_PROBE_NOT_READY;
}

static int boot_probe_adc(boot_probe_status_t *status, uint32_t state)
{
    uint32_t errors = state & (HAL_ADC_STATE_ERROR_INTERNAL |
                               HAL_ADC_STATE_ERROR_CONFIG |
                               HAL_ADC_STATE_ERROR_DMA);

    status->present = 1U;
    status->ready = (((state & HAL_ADC_STATE_READY) != 0U) &&
                     (errors == 0U)) ? 1U : 0U;
    status->detail0 = state;
    status->detail1 = errors;
    return (errors != 0U) ? BOOT_PROBE_ERROR :
           ((status->ready != 0U) ? BOOT_PROBE_OK : BOOT_PROBE_NOT_READY);
}

static int boot_probe_adc1(boot_probe_status_t *status)
{
    return boot_probe_adc(status, hadc1.State);
}

static int boot_probe_adc3(boot_probe_status_t *status)
{
    return boot_probe_adc(status, hadc3.State);
}

static int boot_probe_sai1(boot_probe_status_t *status)
{
    uint32_t state_a = (uint32_t)hsai_BlockA1.State;
    uint32_t state_b = (uint32_t)hsai_BlockB1.State;

    status->present = 1U;
    status->ready = ((state_a == (uint32_t)HAL_SAI_STATE_READY) &&
                     (state_b == (uint32_t)HAL_SAI_STATE_READY)) ? 1U : 0U;
    status->detail0 = state_a;
    status->detail1 = state_b;
    return (status->ready != 0U) ? BOOT_PROBE_OK : BOOT_PROBE_NOT_READY;
}

static int boot_probe_gt9xxx(boot_probe_status_t *status)
{
    char product_id[5];
    uint32_t packed_id;

    memset(product_id, 0, sizeof(product_id));
    if (gt9xxx_probe_readonly(product_id) != 0U)
    {
        return BOOT_PROBE_ABSENT;
    }

    packed_id = ((uint32_t)(uint8_t)product_id[0] << 24) |
                ((uint32_t)(uint8_t)product_id[1] << 16) |
                ((uint32_t)(uint8_t)product_id[2] << 8) |
                (uint32_t)(uint8_t)product_id[3];
    status->detail0 = packed_id;
    if ((packed_id == 0U) || (packed_id == 0xFFFFFFFFUL))
    {
        return BOOT_PROBE_ABSENT;
    }
    status->present = 1U;
    status->ready = 1U;
    return BOOT_PROBE_OK;
}

static int boot_probe_pcf8574(boot_probe_status_t *status)
{
    status->detail0 = PCF8574_ADDR;
    if (pcf8574_probe_readonly(5U) != 0U)
    {
        return BOOT_PROBE_ABSENT;
    }
    status->present = 1U;
    status->ready = 1U;
    return BOOT_PROBE_OK;
}

static int boot_probe_sd(boot_probe_status_t *status)
{
    storage_status_t storage;
    int result = storage_get_status(STORAGE_SD, &storage);

    status->present = storage.present;
    status->ready = storage.ready;
    status->detail0 = storage.detail0;
    status->detail1 = storage.detail1;
    if (storage.ready != 0U)
    {
        return BOOT_PROBE_OK;
    }
    if (storage.present == 0U)
    {
        return BOOT_PROBE_ABSENT;
    }
    return (result == STORAGE_NOT_READY) ?
           BOOT_PROBE_NOT_READY : BOOT_PROBE_ERROR;
}

static int boot_probe_qspi_flash(boot_probe_status_t *status)
{
    uint16_t id;

    if (norflash_probe_readonly(&id, 5U) != 0U)
    {
        return ((hqspi.State == HAL_QSPI_STATE_READY) ||
                (hqspi.State == HAL_QSPI_STATE_BUSY_MEM_MAPPED)) ?
               BOOT_PROBE_ABSENT : BOOT_PROBE_NOT_READY;
    }
    status->detail0 = id;
    if ((id == 0U) || (id == 0xFFFFU))
    {
        return BOOT_PROBE_ABSENT;
    }
    status->present = 1U;
    status->ready = 1U;
    return BOOT_PROBE_OK;
}

static int boot_probe_nand(boot_probe_status_t *status)
{
    nand_id_t id;
    int result = nand_readonly_probe(100U);

    memset(&id, 0, sizeof(id));
    if (nand_readonly_get_id(&id) == NAND_READONLY_OK)
    {
        status->present = 1U;
        status->detail0 = id.compact_id;
    }

    switch (result)
    {
        case NAND_READONLY_OK:
            status->present = 1U;
            status->ready = 1U;
            return BOOT_PROBE_OK;
        case NAND_READONLY_NOT_PRESENT:
            return BOOT_PROBE_ABSENT;
        case NAND_READONLY_UNKNOWN_ID:
        case NAND_READONLY_BUSY:
            return BOOT_PROBE_NOT_READY;
        case NAND_READONLY_TIMEOUT:
            return BOOT_PROBE_TIMEOUT;
        default:
            return BOOT_PROBE_ERROR;
    }
}

static int boot_probe_imu(boot_probe_status_t *status)
{
    imu_status_t sensor;

    memset(&sensor, 0, sizeof(sensor));
    imu_get_status(&sensor);
    status->present = sensor.present;
    status->ready = sensor.ready;
    status->detail0 = ((uint32_t)sensor.model << 16) |
                     ((uint32_t)sensor.id0 << 8) |
                     (uint32_t)sensor.id1;
    status->detail1 = sensor.probe_count;

    switch (sensor.result)
    {
        case IMU_RESULT_OK:
            return BOOT_PROBE_OK;
        case IMU_RESULT_ABSENT:
            return BOOT_PROBE_ABSENT;
        case IMU_RESULT_NOT_READY:
            return BOOT_PROBE_NOT_READY;
        case IMU_RESULT_TIMEOUT:
            return BOOT_PROBE_TIMEOUT;
        case IMU_RESULT_ID_MISMATCH:
        case IMU_RESULT_ERROR:
        case IMU_RESULT_INVALID_ARG:
        default:
            return BOOT_PROBE_ERROR;
    }
}

static int boot_probe_pressure(boot_probe_status_t *status)
{
    pressure_status_t sensor;

    memset(&sensor, 0, sizeof(sensor));
    pressure_get_status(&sensor);
    status->present = sensor.present;
    status->ready = sensor.ready;
    status->detail0 = ((uint32_t)sensor.chip_id << 24) |
                      ((uint32_t)sensor.meas_cfg << 16);
    status->detail1 = sensor.probe_count;

    switch (sensor.result)
    {
        case PRESSURE_RESULT_OK:
            return BOOT_PROBE_OK;
        case PRESSURE_RESULT_ABSENT:
            return BOOT_PROBE_ABSENT;
        case PRESSURE_RESULT_NOT_READY:
            return BOOT_PROBE_NOT_READY;
        case PRESSURE_RESULT_TIMEOUT:
            return BOOT_PROBE_TIMEOUT;
        case PRESSURE_RESULT_ID_MISMATCH:
        case PRESSURE_RESULT_CALIB_INVALID:
        case PRESSURE_RESULT_RANGE_ERROR:
        case PRESSURE_RESULT_ERROR:
        case PRESSURE_RESULT_INVALID_ARG:
        default:
            return BOOT_PROBE_ERROR;
    }
}

static int boot_probe_ap3216c(boot_probe_status_t *status)
{
    ap3216c_status_t sensor;

    memset(&sensor, 0, sizeof(sensor));
    ap3216c_get_status(&sensor);
    status->present = sensor.present;
    status->ready = sensor.ready;
    status->detail0 = sensor.system_mode;
    status->detail1 = sensor.probe_count;

    switch (sensor.result)
    {
        case AP3216C_RESULT_OK:
            return BOOT_PROBE_OK;
        case AP3216C_RESULT_ABSENT:
            return BOOT_PROBE_ABSENT;
        case AP3216C_RESULT_NOT_READY:
            return BOOT_PROBE_NOT_READY;
        case AP3216C_RESULT_TIMEOUT:
            return BOOT_PROBE_TIMEOUT;
        case AP3216C_RESULT_PROTOCOL_ERROR:
        case AP3216C_RESULT_ERROR:
        case AP3216C_RESULT_INVALID_ARG:
        default:
            return BOOT_PROBE_ERROR;
    }
}

static int boot_probe_at24c02(boot_probe_status_t *status)
{
    at24c02_status_t eeprom;

    memset(&eeprom, 0, sizeof(eeprom));
    at24c02_get_status(&eeprom);
    status->present = eeprom.present;
    status->ready = eeprom.ready;
    status->detail0 = ((uint32_t)eeprom.last_offset << 16) |
                      (uint32_t)eeprom.last_length;
    status->detail1 = eeprom.probe_count;

    switch (eeprom.result)
    {
        case AT24C02_RESULT_OK:
            return BOOT_PROBE_OK;
        case AT24C02_RESULT_ABSENT:
            return BOOT_PROBE_ABSENT;
        case AT24C02_RESULT_NOT_READY:
            return BOOT_PROBE_NOT_READY;
        case AT24C02_RESULT_TIMEOUT:
            return BOOT_PROBE_TIMEOUT;
        case AT24C02_RESULT_ERROR:
        case AT24C02_RESULT_INVALID_ARG:
        default:
            return BOOT_PROBE_ERROR;
    }
}

static const boot_probe_entry_t g_boot_probe_entries[] =
{
    {"uart1", BOOT_PROBE_KIND_INTERNAL, 1U, boot_probe_uart1},
    {"i2c2", BOOT_PROBE_KIND_INTERNAL, 0U, boot_probe_i2c2},
    {"sdmmc1", BOOT_PROBE_KIND_INTERNAL, 0U, boot_probe_sdmmc1},
    {"qspi", BOOT_PROBE_KIND_INTERNAL, 1U, boot_probe_qspi},
    {"fmc_sdram", BOOT_PROBE_KIND_INTERNAL, 1U, boot_probe_fmc_sdram},
    {"ltdc", BOOT_PROBE_KIND_INTERNAL, 1U, boot_probe_ltdc},
    {"dma2d", BOOT_PROBE_KIND_INTERNAL, 1U, boot_probe_dma2d},
    {"jpeg", BOOT_PROBE_KIND_INTERNAL, 0U, boot_probe_jpeg},
    {"rtc", BOOT_PROBE_KIND_INTERNAL, 0U, boot_probe_rtc},
    {"adc1", BOOT_PROBE_KIND_INTERNAL, 0U, boot_probe_adc1},
    {"adc3", BOOT_PROBE_KIND_INTERNAL, 0U, boot_probe_adc3},
    {"sai1", BOOT_PROBE_KIND_AUDIO, 0U, boot_probe_sai1},
    {"spdifrx1", BOOT_PROBE_KIND_AUDIO, 0U, boot_probe_spdifrx1},
    {"tim1", BOOT_PROBE_KIND_INTERNAL, 0U, boot_probe_tim1},
    {"tim2", BOOT_PROBE_KIND_INTERNAL, 0U, boot_probe_tim2},
    {"tim3", BOOT_PROBE_KIND_INTERNAL, 0U, boot_probe_tim3},
    {"tim6", BOOT_PROBE_KIND_INTERNAL, 0U, boot_probe_tim6},
    {"gt9xxx", BOOT_PROBE_KIND_I2C, 1U, boot_probe_gt9xxx},
    {"pcf8574", BOOT_PROBE_KIND_I2C, 0U, boot_probe_pcf8574},
    {"sd", BOOT_PROBE_KIND_STORAGE, 0U, boot_probe_sd},
    {"qspi_flash", BOOT_PROBE_KIND_STORAGE, 1U, boot_probe_qspi_flash},
    {"nand", BOOT_PROBE_KIND_STORAGE, 0U, boot_probe_nand},
    {"lcd_panel", BOOT_PROBE_KIND_DISPLAY, 1U, NULL},
    {"sdram", BOOT_PROBE_KIND_STORAGE, 1U, boot_probe_fmc_sdram},
    {"imu", BOOT_PROBE_KIND_SENSOR, 0U, boot_probe_imu},
    {"pressure", BOOT_PROBE_KIND_SENSOR, 0U, boot_probe_pressure},
    {"es8388", BOOT_PROBE_KIND_AUDIO, 0U, NULL},
    {"ov5640", BOOT_PROBE_KIND_CAMERA, 0U, NULL},
    {"ap3216c", BOOT_PROBE_KIND_SENSOR, 0U, boot_probe_ap3216c},
    {"24c02", BOOT_PROBE_KIND_STORAGE, 0U, boot_probe_at24c02},
    {"dht11", BOOT_PROBE_KIND_SENSOR, 0U, NULL},
    {"ds18b20", BOOT_PROBE_KIND_SENSOR, 0U, NULL}
};

#define BOOT_PROBE_TARGET_COUNT \
    (sizeof(g_boot_probe_entries) / sizeof(g_boot_probe_entries[0]))
typedef char boot_probe_target_count_must_match[
    (BOOT_PROBE_TARGET_COUNT == BOOT_PROBE_MAX_ENTRIES) ? 1 : -1];

const boot_probe_entry_t *boot_probe_target_entries(uint32_t *count)
{
    if (count != NULL)
    {
        *count = BOOT_PROBE_TARGET_COUNT;
    }
    return g_boot_probe_entries;
}
