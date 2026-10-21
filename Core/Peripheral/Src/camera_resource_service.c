#include "camera_resource_service.h"

#include <string.h>

#include "ov5640_service.h"
#include "resource_manager.h"
#include "sdcard_manager.h"

#define CAMERA_RESOURCE_OWNER 0x0505U
#define SDCARD_RESOURCE_OWNER 0x0101U

#if defined(CAMERA_RESOURCE_HOST_TEST)
extern int camera_resource_test_configure_dcmi_pins(void);
extern int camera_resource_test_restore_sd_pins(void);
#else
#include "stm32h7xx_hal.h"
#endif

static camera_resource_status_t g_camera_resource_status;

static int camera_platform_configure_dcmi_pins(void)
{
#if defined(CAMERA_RESOURCE_HOST_TEST)
    return camera_resource_test_configure_dcmi_pins();
#else
    GPIO_InitTypeDef gpio;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    memset(&gpio, 0, sizeof(gpio));
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF13_DCMI;

    gpio.Pin = GPIO_PIN_6;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 |
               GPIO_PIN_9 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOC, &gpio);
    gpio.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOD, &gpio);
    gpio.Pin = GPIO_PIN_8;
    HAL_GPIO_Init(GPIOH, &gpio);
    return 0;
#endif
}

static int camera_platform_restore_sd_pins(void *context)
{
    (void)context;
#if defined(CAMERA_RESOURCE_HOST_TEST)
    return camera_resource_test_restore_sd_pins();
#else
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_6);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 |
                           GPIO_PIN_9 | GPIO_PIN_11);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_3);
    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_8);
    return 0;
#endif
}

static void camera_resource_set_result(int result)
{
    g_camera_resource_status.last_result = result;
    g_camera_resource_status.acquired =
        (g_camera_resource_status.stage == CAMERA_RESOURCE_ACTIVE) ? 1U : 0U;
}

static void camera_resource_cleanup(int result)
{
    uint8_t errors = 0U;
    camera_resource_stage_t stage = g_camera_resource_status.stage;

    if (stage >= CAMERA_RESOURCE_SENSOR_POWERED)
    {
        if (ov5640_power_off() != OV5640_OK)
        {
            errors++;
        }
    }
    if (stage >= CAMERA_RESOURCE_DMA_ACQUIRED)
    {
        if (resource_release(RESOURCE_DMA1_STREAM1,
                             CAMERA_RESOURCE_OWNER) != RESOURCE_OK)
        {
            errors++;
        }
    }
    if (stage >= CAMERA_RESOURCE_PINS_ACQUIRED)
    {
        if (resource_restore_default(RESOURCE_SDMMC1_DCMI_PINS,
                                     CAMERA_RESOURCE_OWNER) != RESOURCE_OK)
        {
            errors++;
        }
    }
    if (stage >= CAMERA_RESOURCE_SD_SUSPENDED)
    {
        sdcard_manager_resume();
    }

    g_camera_resource_status.stage = CAMERA_RESOURCE_IDLE;
    g_camera_resource_status.acquired = 0U;
    g_camera_resource_status.release_errors = errors;
    g_camera_resource_status.last_result = result;
}

int camera_resource_service_init(void)
{
    memset(&g_camera_resource_status, 0, sizeof(g_camera_resource_status));
    g_camera_resource_status.stage = CAMERA_RESOURCE_IDLE;
    g_camera_resource_status.last_result = CAMERA_RESOURCE_OK;

    if (resource_configure(RESOURCE_SDMMC1_DCMI_PINS,
                           SDCARD_RESOURCE_OWNER,
                           camera_platform_restore_sd_pins, NULL) != RESOURCE_OK)
    {
        camera_resource_set_result(CAMERA_RESOURCE_RESOURCE_ERROR);
        return CAMERA_RESOURCE_RESOURCE_ERROR;
    }
    if (resource_configure(RESOURCE_DMA1_STREAM1, RESOURCE_OWNER_NONE,
                           NULL, NULL) != RESOURCE_OK)
    {
        camera_resource_set_result(CAMERA_RESOURCE_RESOURCE_ERROR);
        return CAMERA_RESOURCE_RESOURCE_ERROR;
    }
    return CAMERA_RESOURCE_OK;
}

int camera_resource_acquire(void)
{
    int result;

    if (g_camera_resource_status.stage != CAMERA_RESOURCE_IDLE)
    {
        camera_resource_set_result(CAMERA_RESOURCE_BUSY);
        return CAMERA_RESOURCE_BUSY;
    }

    result = sdcard_manager_request_suspend();
    if (result != SDCARD_MANAGER_OK)
    {
        result = (result == SDCARD_MANAGER_BUSY) ? CAMERA_RESOURCE_BUSY :
                                                  CAMERA_RESOURCE_RESOURCE_ERROR;
        camera_resource_set_result(result);
        return result;
    }
    g_camera_resource_status.stage = CAMERA_RESOURCE_SD_SUSPENDED;

    if (resource_release(RESOURCE_SDMMC1_DCMI_PINS,
                         SDCARD_RESOURCE_OWNER) != RESOURCE_OK)
    {
        camera_resource_cleanup(CAMERA_RESOURCE_RESOURCE_ERROR);
        return CAMERA_RESOURCE_RESOURCE_ERROR;
    }
    if (resource_acquire(RESOURCE_SDMMC1_DCMI_PINS,
                         CAMERA_RESOURCE_OWNER) != RESOURCE_OK)
    {
        (void)resource_acquire(RESOURCE_SDMMC1_DCMI_PINS,
                               SDCARD_RESOURCE_OWNER);
        camera_resource_cleanup(CAMERA_RESOURCE_RESOURCE_ERROR);
        return CAMERA_RESOURCE_RESOURCE_ERROR;
    }
    g_camera_resource_status.stage = CAMERA_RESOURCE_PINS_ACQUIRED;

    if (resource_acquire(RESOURCE_DMA1_STREAM1,
                         CAMERA_RESOURCE_OWNER) != RESOURCE_OK)
    {
        camera_resource_cleanup(CAMERA_RESOURCE_RESOURCE_ERROR);
        return CAMERA_RESOURCE_RESOURCE_ERROR;
    }
    g_camera_resource_status.stage = CAMERA_RESOURCE_DMA_ACQUIRED;

    if (camera_platform_configure_dcmi_pins() != 0)
    {
        camera_resource_cleanup(CAMERA_RESOURCE_RESOURCE_ERROR);
        return CAMERA_RESOURCE_RESOURCE_ERROR;
    }
    g_camera_resource_status.stage = CAMERA_RESOURCE_GPIO_CONFIGURED;
    g_camera_resource_status.stage = CAMERA_RESOURCE_SENSOR_POWERED;
    if (ov5640_power_on() != OV5640_OK)
    {
        camera_resource_cleanup(CAMERA_RESOURCE_SENSOR_ERROR);
        return CAMERA_RESOURCE_SENSOR_ERROR;
    }
    if ((ov5640_probe() != OV5640_OK) ||
        (ov5640_configure_rgb565_600x400() != OV5640_OK))
    {
        camera_resource_cleanup(CAMERA_RESOURCE_SENSOR_ERROR);
        return CAMERA_RESOURCE_SENSOR_ERROR;
    }

    g_camera_resource_status.stage = CAMERA_RESOURCE_ACTIVE;
    camera_resource_set_result(CAMERA_RESOURCE_OK);
    return CAMERA_RESOURCE_OK;
}

int camera_resource_release(void)
{
    if (g_camera_resource_status.stage == CAMERA_RESOURCE_IDLE)
    {
        camera_resource_set_result(CAMERA_RESOURCE_OK);
        return CAMERA_RESOURCE_OK;
    }
    camera_resource_cleanup(CAMERA_RESOURCE_OK);
    if (g_camera_resource_status.release_errors != 0U)
    {
        g_camera_resource_status.last_result = CAMERA_RESOURCE_RESOURCE_ERROR;
        return CAMERA_RESOURCE_RESOURCE_ERROR;
    }
    return CAMERA_RESOURCE_OK;
}

void camera_resource_get_status(camera_resource_status_t *status)
{
    if (status != NULL)
    {
        *status = g_camera_resource_status;
    }
}
