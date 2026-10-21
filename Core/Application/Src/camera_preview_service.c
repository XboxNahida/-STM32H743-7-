#include "camera_preview_service.h"

#include <string.h>

#include "camera_dcmi_driver.h"
#include "camera_resource_service.h"
#include "sdram_layout.h"

#if !defined(CAMERA_PREVIEW_HOST_TEST)
#include "stm32h7xx_hal.h"
#endif

#if defined(CAMERA_PREVIEW_HOST_TEST)
extern int camera_preview_test_copy_frame(uintptr_t source,
                                          uintptr_t destination,
                                          uint32_t bytes);
#endif

static camera_preview_status_t g_preview_status;
static uint8_t g_resource_held;

static int camera_preview_copy_frame(void)
{
#if defined(CAMERA_PREVIEW_HOST_TEST)
    return camera_preview_test_copy_frame(SDRAM_VIDEO_RGB_A_ADDR,
                                          SDRAM_VIDEO_RGB_B_ADDR,
                                          CAMERA_DCMI_FRAME_BYTES);
#else
    memcpy((void *)SDRAM_VIDEO_RGB_B_ADDR,
           (const void *)SDRAM_VIDEO_RGB_A_ADDR,
           CAMERA_DCMI_FRAME_BYTES);
    SCB_CleanDCache_by_Addr((uint32_t *)SDRAM_VIDEO_RGB_B_ADDR,
                            CAMERA_DCMI_FRAME_BYTES);
    return 0;
#endif
}

static void camera_preview_set_error(int result)
{
    g_preview_status.state = CAMERA_PREVIEW_ERROR;
    g_preview_status.last_result = result;
    g_preview_status.dropped_frames++;
}

static void camera_preview_fail(int result)
{
    if (g_resource_held != 0U)
    {
        /* Keep SD suspended until the error page is rendered and the user
         * leaves the camera page. Releasing here would let the same main-loop
         * iteration re-enter HAL_SD_Init(). */
        (void)camera_dcmi_stop();
    }
    camera_preview_set_error(result);
}

int camera_preview_init(void)
{
    memset(&g_preview_status, 0, sizeof(g_preview_status));
    g_preview_status.state = CAMERA_PREVIEW_IDLE;
    g_preview_status.width = CAMERA_DCMI_WIDTH;
    g_preview_status.height = CAMERA_DCMI_HEIGHT;
    g_preview_status.bytes = CAMERA_DCMI_FRAME_BYTES;
    g_preview_status.rgb_address = SDRAM_VIDEO_RGB_B_ADDR;
    g_preview_status.last_result = CAMERA_PREVIEW_IDLE;
    g_resource_held = 0U;
    if (camera_resource_service_init() != CAMERA_RESOURCE_OK)
    {
        g_preview_status.state = CAMERA_PREVIEW_ERROR;
        g_preview_status.last_result = CAMERA_PREVIEW_ERROR;
        return CAMERA_PREVIEW_ERROR;
    }
    return CAMERA_PREVIEW_IDLE;
}

int camera_preview_start(void)
{
    if (g_preview_status.state == CAMERA_PREVIEW_RUNNING ||
        g_preview_status.state == CAMERA_PREVIEW_STARTING)
    {
        return CAMERA_PREVIEW_ERROR;
    }
    g_preview_status.state = CAMERA_PREVIEW_STARTING;
    g_preview_status.last_result = CAMERA_PREVIEW_IDLE;
    return CAMERA_PREVIEW_IDLE;
}

int camera_preview_stop(void)
{
    if (g_preview_status.state == CAMERA_PREVIEW_IDLE)
    {
        return CAMERA_PREVIEW_IDLE;
    }
    if (g_resource_held != 0U)
    {
        (void)camera_dcmi_stop();
        (void)camera_resource_release();
        g_resource_held = 0U;
    }
    g_preview_status.state = CAMERA_PREVIEW_IDLE;
    g_preview_status.last_result = CAMERA_PREVIEW_IDLE;
    return CAMERA_PREVIEW_IDLE;
}

void camera_preview_poll(void)
{
    uint32_t events = 0U;
    int result;

    if (g_preview_status.state == CAMERA_PREVIEW_STARTING)
    {
        result = camera_resource_acquire();
        if (result != CAMERA_RESOURCE_OK)
        {
            camera_preview_fail(result);
            return;
        }
        g_resource_held = 1U;
        if (camera_dcmi_init() != CAMERA_DCMI_OK ||
            camera_dcmi_start() != CAMERA_DCMI_OK)
        {
            camera_preview_fail(CAMERA_PREVIEW_ERROR);
            return;
        }
        g_preview_status.state = CAMERA_PREVIEW_RUNNING;
        g_preview_status.last_result = CAMERA_PREVIEW_IDLE;
        return;
    }
    if (g_preview_status.state != CAMERA_PREVIEW_RUNNING)
    {
        return;
    }
    if (camera_dcmi_poll_event(&events) != CAMERA_DCMI_OK)
    {
        camera_preview_fail(CAMERA_PREVIEW_ERROR);
        return;
    }
    if ((events & CAMERA_DCMI_EVENT_ERROR) != 0U)
    {
        camera_preview_fail(CAMERA_PREVIEW_ERROR);
        return;
    }
    if ((events & CAMERA_DCMI_EVENT_FRAME) != 0U)
    {
        if (camera_preview_copy_frame() != 0)
        {
            camera_preview_fail(CAMERA_PREVIEW_ERROR);
            return;
        }
        g_preview_status.generation++;
        g_preview_status.published_frames++;
        g_preview_status.frame_valid = 1U;
        g_preview_status.rgb_address = SDRAM_VIDEO_RGB_B_ADDR;
        g_preview_status.last_result = CAMERA_PREVIEW_IDLE;
        if (camera_dcmi_start() != CAMERA_DCMI_OK)
        {
            camera_preview_fail(CAMERA_PREVIEW_ERROR);
        }
    }
}

int camera_preview_get_frame(camera_preview_frame_t *frame)
{
    if (frame == NULL)
    {
        return CAMERA_PREVIEW_ERROR;
    }
    if (g_preview_status.frame_valid == 0U)
    {
        return CAMERA_PREVIEW_ERROR;
    }
    frame->rgb_address = g_preview_status.rgb_address;
    frame->width = g_preview_status.width;
    frame->height = g_preview_status.height;
    frame->bytes = g_preview_status.bytes;
    frame->generation = g_preview_status.generation;
    return CAMERA_PREVIEW_IDLE;
}

void camera_preview_get_status(camera_preview_status_t *status)
{
    if (status != NULL)
    {
        *status = g_preview_status;
    }
}
