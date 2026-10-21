#include <stdint.h>
#include <stdio.h>

#include "ov5640_service.h"
#include "resource_manager.h"
#include "sdcard_manager.h"
typedef enum
{
    CAMERA_RESOURCE_OK = 0,
    CAMERA_RESOURCE_INVALID_ARG,
    CAMERA_RESOURCE_BUSY,
    CAMERA_RESOURCE_RESOURCE_ERROR,
    CAMERA_RESOURCE_SENSOR_ERROR
} camera_resource_result_t;
typedef enum
{
    CAMERA_RESOURCE_IDLE = 0,
    CAMERA_RESOURCE_SD_SUSPENDED,
    CAMERA_RESOURCE_PINS_ACQUIRED,
    CAMERA_RESOURCE_DMA_ACQUIRED,
    CAMERA_RESOURCE_GPIO_CONFIGURED,
    CAMERA_RESOURCE_SENSOR_POWERED,
    CAMERA_RESOURCE_ACTIVE
} camera_resource_stage_t;
typedef struct
{
    camera_resource_stage_t stage;
    uint8_t acquired;
    uint8_t release_errors;
    int last_result;
} camera_resource_status_t;
int camera_resource_service_init(void);
int camera_resource_acquire(void);
int camera_resource_release(void);
void camera_resource_get_status(camera_resource_status_t *status);
#define CAMERA_OWNER 0x0505U
#define EVENT_CAPACITY 32U
enum
{
    EVENT_SD_SUSPEND = 1,
    EVENT_PINS_RELEASE,
    EVENT_PINS_ACQUIRE,
    EVENT_DMA_ACQUIRE,
    EVENT_GPIO_CONFIGURE,
    EVENT_POWER_ON,
    EVENT_PROBE,
    EVENT_SENSOR_CONFIGURE,
    EVENT_POWER_OFF,
    EVENT_GPIO_RESTORE,
    EVENT_DMA_RELEASE,
    EVENT_PINS_RESTORE,
    EVENT_SD_RESUME
};
static int g_events[EVENT_CAPACITY];
static uint32_t g_event_count;
static int g_sd_suspend_result;
static resource_id_t g_fail_acquire_id;
static uint8_t g_fail_acquire_remaining;
static int g_gpio_configure_result;
static int g_power_result;
static int g_probe_result;
static int g_sensor_configure_result;
static resource_owner_t g_pins_default_owner;
static resource_restore_fn_t g_pins_restore;
static void *g_pins_restore_context;
static uint32_t g_pa8_acquire_count;
static void record_event(int event)
{
    if (g_event_count < EVENT_CAPACITY)
    {
        g_events[g_event_count] = event;
        g_event_count++;
    }
}
int sdcard_manager_request_suspend(void)
{
    record_event(EVENT_SD_SUSPEND);
    return g_sd_suspend_result;
}
void sdcard_manager_resume(void)
{
    record_event(EVENT_SD_RESUME);
}
int resource_configure(resource_id_t id, resource_owner_t default_owner,
                       resource_restore_fn_t restore, void *context)
{
    if (id == RESOURCE_SDMMC1_DCMI_PINS)
    {
        g_pins_default_owner = default_owner;
        g_pins_restore = restore;
        g_pins_restore_context = context;
    }
    return RESOURCE_OK;
}
int resource_release(resource_id_t id, resource_owner_t owner)
{
    (void)owner;
    if (id == RESOURCE_SDMMC1_DCMI_PINS)
    {
        record_event(EVENT_PINS_RELEASE);
    }
    else if (id == RESOURCE_DMA1_STREAM1)
    {
        record_event(EVENT_DMA_RELEASE);
    }
    return RESOURCE_OK;
}
int resource_acquire(resource_id_t id, resource_owner_t owner)
{
    if (id == RESOURCE_TIM1_PA8)
    {
        g_pa8_acquire_count++;
    }
    if (id == RESOURCE_SDMMC1_DCMI_PINS)
    {
        record_event((owner == CAMERA_OWNER) ? EVENT_PINS_ACQUIRE :
                     EVENT_PINS_RESTORE);
    }
    else if (id == RESOURCE_DMA1_STREAM1)
    {
        record_event(EVENT_DMA_ACQUIRE);
    }
    if ((id == g_fail_acquire_id) && (g_fail_acquire_remaining != 0U))
    {
        g_fail_acquire_remaining--;
        return RESOURCE_BUSY;
    }
    if (
        ((owner != CAMERA_OWNER) && (owner != g_pins_default_owner)))
    {
        return RESOURCE_BUSY;
    }
    return RESOURCE_OK;
}
int resource_restore_default(resource_id_t id, resource_owner_t owner)
{
    if ((id != RESOURCE_SDMMC1_DCMI_PINS) || (owner != CAMERA_OWNER))
    {
        return RESOURCE_NOT_OWNER;
    }
    if ((g_pins_restore != NULL) &&
        (g_pins_restore(g_pins_restore_context) != 0))
    {
        return RESOURCE_RESTORE_FAILED;
    }
    record_event(EVENT_PINS_RESTORE);
    return RESOURCE_OK;
}
int camera_resource_test_configure_dcmi_pins(void)
{
    record_event(EVENT_GPIO_CONFIGURE);
    return g_gpio_configure_result;
}
int camera_resource_test_restore_sd_pins(void)
{
    record_event(EVENT_GPIO_RESTORE);
    return 0;
}
int ov5640_power_on(void)
{
    record_event(EVENT_POWER_ON);
    return g_power_result;
}
int ov5640_power_off(void)
{
    record_event(EVENT_POWER_OFF);
    return OV5640_OK;
}
int ov5640_probe(void)
{
    record_event(EVENT_PROBE);
    return g_probe_result;
}
int ov5640_configure_rgb565_600x400(void)
{
    record_event(EVENT_SENSOR_CONFIGURE);
    return g_sensor_configure_result;
}
static void reset_fixture(void)
{
    uint32_t index;
    for (index = 0U; index < EVENT_CAPACITY; index++)
    {
        g_events[index] = 0;
    }
    g_event_count = 0U;
    g_sd_suspend_result = SDCARD_MANAGER_OK;
    g_fail_acquire_id = RESOURCE_COUNT;
    g_fail_acquire_remaining = 0U;
    g_gpio_configure_result = 0;
    g_power_result = OV5640_OK;
    g_probe_result = OV5640_OK;
    g_sensor_configure_result = OV5640_OK;
    g_pins_default_owner = RESOURCE_OWNER_NONE;
    g_pins_restore = NULL;
    g_pins_restore_context = NULL;
    g_pa8_acquire_count = 0U;
}
static int expect_int(const char *name, int actual, int expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%d expected=%d\n", name, actual, expected);
        return 1;
    }
    return 0;
}
static int expect_event(const char *name, uint32_t index, int expected)
{
    if ((index >= g_event_count) || (g_events[index] != expected))
    {
        printf("FAIL %s index=%lu actual=%d expected=%d count=%lu\n", name,
               (unsigned long)index,
               (index < g_event_count) ? g_events[index] : -1, expected,
               (unsigned long)g_event_count);
        return 1;
    }
    return 0;
}
static int expect_idle(const char *name, int expected_result)
{
    camera_resource_status_t status;
    camera_resource_get_status(&status);
    return expect_int(name, status.stage, CAMERA_RESOURCE_IDLE) +
           expect_int("idle_acquired", status.acquired, 0) +
           expect_int("idle_result", status.last_result, expected_result);
}
int main(void)
{
    camera_resource_status_t status;
    int failures = 0;
    reset_fixture();
    failures += expect_int("init", camera_resource_service_init(),
                           CAMERA_RESOURCE_OK);
    failures += expect_int("sd_default_nonzero",
                           g_pins_default_owner != RESOURCE_OWNER_NONE, 1);
    failures += expect_int("sd_default_not_camera",
                           g_pins_default_owner != CAMERA_OWNER, 1);
    failures += expect_int("restore_callback_registered",
                           g_pins_restore != NULL, 1);
    g_sd_suspend_result = SDCARD_MANAGER_BUSY;
    failures += expect_int("sd_busy", camera_resource_acquire(),
                           CAMERA_RESOURCE_BUSY);
    failures += expect_idle("busy_idle", CAMERA_RESOURCE_BUSY);
    failures += expect_int("busy_only_suspend", (int)g_event_count, 1);
    reset_fixture();
    failures += expect_int("reinit_pins_failure", camera_resource_service_init(),
                           CAMERA_RESOURCE_OK);
    g_fail_acquire_id = RESOURCE_SDMMC1_DCMI_PINS;
    g_fail_acquire_remaining = 1U;
    failures += expect_int("pins_failure", camera_resource_acquire(),
                           CAMERA_RESOURCE_RESOURCE_ERROR);
    failures += expect_event("pins_rollback_default", 3U,
                             EVENT_PINS_RESTORE);
    failures += expect_event("pins_rollback_sd", 4U, EVENT_SD_RESUME);
    reset_fixture();
    failures += expect_int("reinit_dma_failure", camera_resource_service_init(),
                           CAMERA_RESOURCE_OK);
    g_fail_acquire_id = RESOURCE_DMA1_STREAM1;
    g_fail_acquire_remaining = 1U;
    failures += expect_int("dma_failure", camera_resource_acquire(),
                           CAMERA_RESOURCE_RESOURCE_ERROR);
    failures += expect_event("dma_rollback_gpio", 4U, EVENT_GPIO_RESTORE);
    failures += expect_event("dma_rollback_pins", 5U, EVENT_PINS_RESTORE);
    failures += expect_event("dma_rollback_sd", 6U, EVENT_SD_RESUME);
    failures += expect_idle("dma_idle", CAMERA_RESOURCE_RESOURCE_ERROR);
    reset_fixture();
    failures += expect_int("reinit_gpio_failure", camera_resource_service_init(),
                           CAMERA_RESOURCE_OK);
    g_gpio_configure_result = 1;
    failures += expect_int("gpio_failure", camera_resource_acquire(),
                           CAMERA_RESOURCE_RESOURCE_ERROR);
    failures += expect_event("gpio_rollback_dma", 5U, EVENT_DMA_RELEASE);
    failures += expect_event("gpio_rollback_gpio", 6U, EVENT_GPIO_RESTORE);
    failures += expect_event("gpio_rollback_pins", 7U, EVENT_PINS_RESTORE);
    failures += expect_event("gpio_rollback_sd", 8U, EVENT_SD_RESUME);
    reset_fixture();
    failures += expect_int("reinit_power_failure", camera_resource_service_init(),
                           CAMERA_RESOURCE_OK);
    g_power_result = OV5640_IO_ERROR;
    failures += expect_int("power_failure", camera_resource_acquire(),
                           CAMERA_RESOURCE_SENSOR_ERROR);
    failures += expect_event("power_cleanup", 6U, EVENT_POWER_OFF);
    failures += expect_event("power_sd_resume", 10U, EVENT_SD_RESUME);
    reset_fixture();
    failures += expect_int("reinit_probe_failure", camera_resource_service_init(),
                           CAMERA_RESOURCE_OK);
    g_probe_result = OV5640_NOT_PRESENT;
    failures += expect_int("probe_failure", camera_resource_acquire(),
                           CAMERA_RESOURCE_SENSOR_ERROR);
    failures += expect_event("probe_power_off", 7U, EVENT_POWER_OFF);
    failures += expect_event("probe_dma_release", 8U, EVENT_DMA_RELEASE);
    failures += expect_event("probe_gpio_restore", 9U, EVENT_GPIO_RESTORE);
    failures += expect_event("probe_pins_restore", 10U, EVENT_PINS_RESTORE);
    failures += expect_event("probe_sd_resume", 11U, EVENT_SD_RESUME);
    reset_fixture();
    failures += expect_int("reinit_config_failure", camera_resource_service_init(),
                           CAMERA_RESOURCE_OK);
    g_sensor_configure_result = OV5640_IO_ERROR;
    failures += expect_int("config_failure", camera_resource_acquire(),
                           CAMERA_RESOURCE_SENSOR_ERROR);
    failures += expect_event("config_power_off", 8U, EVENT_POWER_OFF);
    failures += expect_event("config_sd_resume", 12U, EVENT_SD_RESUME);
    reset_fixture();
    failures += expect_int("reinit_success", camera_resource_service_init(),
                           CAMERA_RESOURCE_OK);
    failures += expect_int("acquire_success", camera_resource_acquire(),
                           CAMERA_RESOURCE_OK);
    camera_resource_get_status(&status);
    failures += expect_int("active_stage", status.stage,
                           CAMERA_RESOURCE_ACTIVE);
    failures += expect_int("active_acquired", status.acquired, 1);
    failures += expect_int("pa8_not_acquired", (int)g_pa8_acquire_count, 0);
    failures += expect_int("release_success", camera_resource_release(),
                           CAMERA_RESOURCE_OK);
    failures += expect_event("release_power", 8U, EVENT_POWER_OFF);
    failures += expect_event("release_dma", 9U, EVENT_DMA_RELEASE);
    failures += expect_event("release_gpio", 10U, EVENT_GPIO_RESTORE);
    failures += expect_event("release_pins", 11U, EVENT_PINS_RESTORE);
    failures += expect_event("release_sd", 12U, EVENT_SD_RESUME);
    failures += expect_idle("release_idle", CAMERA_RESOURCE_OK);
    failures += expect_int("release_idempotent", camera_resource_release(),
                           CAMERA_RESOURCE_OK);
    failures += expect_int("release_no_more_events", (int)g_event_count, 13);
    if (failures == 0)
    {
        printf("PASS camera_resource_service\n");
        return 0;
    }
    printf("FAIL camera_resource_service failures=%d\n", failures);
    return 1;
}
