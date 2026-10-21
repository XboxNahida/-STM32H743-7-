#include "ov5640_service.h"

#include <stddef.h>
#include <string.h>

#include "ov5640_registers.h"

#define OV5640_DEVICE_ADDR 0x78U
#define OV5640_ID_HIGH_REG 0x300AU
#define OV5640_ID_LOW_REG  0x300BU
#define OV5640_EXPECTED_ID 0x5640U

#if defined(OV5640_HOST_TEST)
extern int ov5640_test_sccb_write(uint8_t device,
                                  uint16_t reg,
                                  uint8_t value);
extern int ov5640_test_sccb_read(uint8_t device,
                                 uint16_t reg,
                                 uint8_t *value);

#define ov5640_bus_write ov5640_test_sccb_write
#define ov5640_bus_read  ov5640_test_sccb_read
#else
#include "main.h"
#include "pcf8574.h"
#include "sccb.h"

#define ov5640_bus_write sccb_write_reg16
#define ov5640_bus_read  sccb_read_reg16
#endif

static ov5640_status_t g_ov5640_status;

static int ov5640_write(uint16_t reg, uint8_t value)
{
    if (ov5640_bus_write(OV5640_DEVICE_ADDR, reg, value) != 0)
    {
        g_ov5640_status.last_result = OV5640_IO_ERROR;
        return OV5640_IO_ERROR;
    }
    return OV5640_OK;
}

static int ov5640_write_table(const ov5640_reg_value_t *table,
                              uint32_t count)
{
    uint32_t index;

    if ((table == NULL) || (count == 0U))
    {
        return OV5640_INVALID_ARG;
    }
    for (index = 0U; index < count; index++)
    {
        if (ov5640_write(table[index].reg, table[index].value) != OV5640_OK)
        {
            return OV5640_IO_ERROR;
        }
    }
    return OV5640_OK;
}

static int ov5640_set_pwdn(uint8_t high)
{
#if defined(OV5640_HOST_TEST)
    (void)high;
    return OV5640_OK;
#else
    if (pcf8574_write_bit_safe(DCMI_PWDN_IO, high) != 0)
    {
        return OV5640_IO_ERROR;
    }
    return OV5640_OK;
#endif
}

void ov5640_service_init(void)
{
    memset(&g_ov5640_status, 0, sizeof(g_ov5640_status));
    g_ov5640_status.last_result = OV5640_NOT_PRESENT;
}

int ov5640_power_on(void)
{
#if defined(OV5640_HOST_TEST)
    return OV5640_OK;
#else
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = DCMI_RESET_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DCMI_RESET_GPIO_Port, &gpio);

    HAL_GPIO_WritePin(DCMI_RESET_GPIO_Port, DCMI_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(20U);
    if (ov5640_set_pwdn(0U) != OV5640_OK) return OV5640_IO_ERROR;
    HAL_Delay(5U);
    HAL_GPIO_WritePin(DCMI_RESET_GPIO_Port, DCMI_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(20U);
    sccb_init();
    HAL_Delay(5U);
    return OV5640_OK;
#endif
}

int ov5640_power_off(void)
{
    int first_result = g_ov5640_status.last_result;
    int result;
    int pwdn_result;

    /* Disable the sensor output before its data pins are reused by SDMMC. */
    result = ov5640_write(0x3017U, 0x00U);
    if (result == OV5640_OK)
    {
        result = ov5640_write(0x3018U, 0x00U);
    }
    if (result == OV5640_OK)
    {
        result = ov5640_set_flash(0U);
    }
    pwdn_result = ov5640_set_pwdn(1U);
    if (result == OV5640_OK)
    {
        result = pwdn_result;
    }
    g_ov5640_status.ready = 0U;
    if (first_result == OV5640_OK)
    {
        g_ov5640_status.last_result = result;
    }
    else
    {
        g_ov5640_status.last_result = first_result;
    }
    return result;
}

int ov5640_probe(void)
{
    uint8_t high = 0U;
    uint8_t low = 0U;
    uint16_t chip_id;

    if ((ov5640_bus_read(OV5640_DEVICE_ADDR,
                         OV5640_ID_HIGH_REG, &high) != 0) ||
        (ov5640_bus_read(OV5640_DEVICE_ADDR,
                         OV5640_ID_LOW_REG, &low) != 0))
    {
        g_ov5640_status.present = 0U;
        g_ov5640_status.ready = 0U;
        g_ov5640_status.last_result = OV5640_IO_ERROR;
        return OV5640_IO_ERROR;
    }
    chip_id = (uint16_t)(((uint16_t)high << 8) | low);
    g_ov5640_status.chip_id = chip_id;
    if (chip_id != OV5640_EXPECTED_ID)
    {
        g_ov5640_status.present = 0U;
        g_ov5640_status.ready = 0U;
        g_ov5640_status.last_result = OV5640_NOT_PRESENT;
        return OV5640_NOT_PRESENT;
    }
    g_ov5640_status.present = 1U;
    g_ov5640_status.last_result = OV5640_OK;
    return OV5640_OK;
}

int ov5640_configure_rgb565_600x400(void)
{
    static const ov5640_reg_value_t output_regs[] =
    {
        {0x3212U, 0x03U},
        {0x3808U, 0x02U}, {0x3809U, 0x58U},
        {0x380AU, 0x01U}, {0x380BU, 0x90U},
        {0x3810U, 0x00U}, {0x3811U, 0x10U},
        {0x3812U, 0x00U}, {0x3813U, 0x04U},
        {0x3212U, 0x13U}, {0x3212U, 0xA3U}
    };

    if (g_ov5640_status.present == 0U)
    {
        return OV5640_NOT_PRESENT;
    }
    if ((ov5640_write(0x3103U, 0x11U) != OV5640_OK) ||
        (ov5640_write(0x3008U, 0x82U) != OV5640_OK))
    {
        return OV5640_IO_ERROR;
    }
#if !defined(OV5640_HOST_TEST)
    HAL_Delay(10U);
#endif
    if ((ov5640_write_table(g_ov5640_init_regs,
                            g_ov5640_init_reg_count) != OV5640_OK) ||
        (ov5640_write_table(g_ov5640_rgb565_regs,
                            g_ov5640_rgb565_reg_count) != OV5640_OK) ||
        (ov5640_write_table(output_regs,
                            (uint32_t)(sizeof(output_regs) /
                                       sizeof(output_regs[0]))) != OV5640_OK) ||
        (ov5640_set_mirror(g_ov5640_status.mirror) != OV5640_OK) ||
        (ov5640_set_flip(g_ov5640_status.flip) != OV5640_OK))
    {
        return OV5640_IO_ERROR;
    }
    g_ov5640_status.ready = 1U;
    g_ov5640_status.width = 600U;
    g_ov5640_status.height = 400U;
    g_ov5640_status.last_result = OV5640_OK;
    return OV5640_OK;
}

int ov5640_set_mirror(uint8_t enabled)
{
    uint8_t value = (enabled != 0U) ? 0x06U : 0x00U;
    int result = ov5640_write(0x3821U, value);

    if (result == OV5640_OK) g_ov5640_status.mirror = (enabled != 0U);
    return result;
}

int ov5640_set_flip(uint8_t enabled)
{
    uint8_t value = (enabled != 0U) ? 0x40U : 0x46U;
    int result = ov5640_write(0x3820U, value);

    if (result == OV5640_OK) g_ov5640_status.flip = (enabled != 0U);
    return result;
}

int ov5640_set_flash(uint8_t enabled)
{
    if ((ov5640_write(0x3016U, 0x02U) != OV5640_OK) ||
        (ov5640_write(0x301CU, 0x02U) != OV5640_OK) ||
        (ov5640_write(0x3019U, (enabled != 0U) ? 0x02U : 0x00U) !=
         OV5640_OK))
    {
        return OV5640_IO_ERROR;
    }
    g_ov5640_status.flash = (enabled != 0U);
    return OV5640_OK;
}

int ov5640_set_test_pattern(uint8_t enabled)
{
    int result = ov5640_write(0x503DU,
                              (enabled != 0U) ? 0x80U : 0x00U);

    if (result == OV5640_OK)
    {
        g_ov5640_status.test_pattern = (enabled != 0U);
    }
    return result;
}

void ov5640_get_status(ov5640_status_t *status)
{
    if (status != NULL) *status = g_ov5640_status;
}
