#include <stdint.h>
#include <stdio.h>

#include "camera_dcmi_driver.h"
#include "camera_resource_service.h"
#include "sdram_layout.h"

typedef enum
{
    CAMERA_PREVIEW_IDLE = 0,
    CAMERA_PREVIEW_STARTING,
    CAMERA_PREVIEW_RUNNING,
    CAMERA_PREVIEW_ERROR
} camera_preview_state_t;

typedef struct
{
    camera_preview_state_t state;
    uint8_t frame_valid;
    uint32_t published_frames;
    uint32_t dropped_frames;
    uint32_t generation;
    uintptr_t rgb_address;
    uint16_t width;
    uint16_t height;
    uint32_t bytes;
    int last_result;
} camera_preview_status_t;

typedef struct
{
    uintptr_t rgb_address;
    uint16_t width;
    uint16_t height;
    uint32_t bytes;
    uint32_t generation;
} camera_preview_frame_t;

int camera_preview_init(void);
int camera_preview_start(void);
int camera_preview_stop(void);
void camera_preview_poll(void);
int camera_preview_get_frame(camera_preview_frame_t *frame);
void camera_preview_get_status(camera_preview_status_t *status);

static int g_resource_result;
static int g_dcmi_init_result;
static int g_dcmi_start_result;
static int g_dcmi_stop_result;
static uint32_t g_dcmi_events;
static uint32_t g_resource_acquire_calls;
static uint32_t g_resource_release_calls;
static uint32_t g_dcmi_start_calls;
static uint32_t g_dcmi_stop_calls;
static uint32_t g_copy_calls;
static uintptr_t g_copy_source;
static uintptr_t g_copy_destination;
static uint32_t g_copy_bytes;
static int g_copy_result;

int camera_resource_service_init(void)
{
    return CAMERA_RESOURCE_OK;
}

int camera_resource_acquire(void)
{
    g_resource_acquire_calls++;
    return g_resource_result;
}

int camera_resource_release(void)
{
    g_resource_release_calls++;
    return CAMERA_RESOURCE_OK;
}

void camera_resource_get_status(camera_resource_status_t *status)
{
    (void)status;
}

int camera_dcmi_init(void)
{
    return g_dcmi_init_result;
}

int camera_dcmi_start(void)
{
    g_dcmi_start_calls++;
    return g_dcmi_start_result;
}

int camera_dcmi_stop(void)
{
    g_dcmi_stop_calls++;
    return g_dcmi_stop_result;
}

int camera_dcmi_poll_event(uint32_t *events)
{
    *events = g_dcmi_events;
    g_dcmi_events = 0U;
    return CAMERA_DCMI_OK;
}

void camera_dcmi_get_status(camera_dcmi_status_t *status)
{
    status->frame_events = 1U;
}

int camera_preview_test_copy_frame(uintptr_t source, uintptr_t destination,
                                   uint32_t bytes)
{
    g_copy_calls++;
    g_copy_source = source;
    g_copy_destination = destination;
    g_copy_bytes = bytes;
    return g_copy_result;
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

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n", name,
               (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

int main(void)
{
    camera_preview_status_t status;
    camera_preview_frame_t frame;
    int failures = 0;

    g_resource_result = CAMERA_RESOURCE_OK;
    g_dcmi_init_result = CAMERA_DCMI_OK;
    g_dcmi_start_result = CAMERA_DCMI_OK;
    g_dcmi_stop_result = CAMERA_DCMI_OK;
    failures += expect_int("init", camera_preview_init(), CAMERA_PREVIEW_IDLE);
    failures += expect_int("start_async", camera_preview_start(), 0);
    camera_preview_get_status(&status);
    failures += expect_int("starting", status.state, CAMERA_PREVIEW_STARTING);
    failures += expect_int("no_eager_resource", g_resource_acquire_calls, 0);
    camera_preview_poll();
    camera_preview_get_status(&status);
    failures += expect_int("running", status.state, CAMERA_PREVIEW_RUNNING);
    failures += expect_u32("resource_once", g_resource_acquire_calls, 1U);
    g_dcmi_events = CAMERA_DCMI_EVENT_FRAME;
    camera_preview_poll();
    camera_preview_get_status(&status);
    failures += expect_u32("published_once", status.published_frames, 1U);
    failures += expect_int("frame_valid", status.frame_valid, 1);
    failures += expect_u32("copy_once", g_copy_calls, 1U);
    failures += expect_u32("copy_source", (uint32_t)g_copy_source,
                           SDRAM_VIDEO_RGB_A_ADDR);
    failures += expect_u32("copy_destination", (uint32_t)g_copy_destination,
                           SDRAM_VIDEO_RGB_B_ADDR);
    failures += expect_u32("copy_bytes", g_copy_bytes,
                           CAMERA_DCMI_FRAME_BYTES);
    failures += expect_u32("snapshot_restart_after_first_frame",
                           g_dcmi_start_calls, 2U);
    failures += expect_int("get_frame", camera_preview_get_frame(&frame), 0);
    failures += expect_u32("frame_generation", frame.generation, 1U);
    g_dcmi_events = CAMERA_DCMI_EVENT_FRAME;
    camera_preview_poll();
    camera_preview_get_status(&status);
    failures += expect_u32("published_twice", status.published_frames, 2U);
    failures += expect_u32("generation_twice", status.generation, 2U);
    failures += expect_u32("snapshot_restart_after_second_frame",
                           g_dcmi_start_calls, 3U);
    failures += expect_int("stop", camera_preview_stop(), 0);
    camera_preview_get_status(&status);
    failures += expect_int("stopped", status.state, CAMERA_PREVIEW_IDLE);
    failures += expect_u32("release_once", g_resource_release_calls, 1U);
    failures += expect_int("restart", camera_preview_start(), 0);
    camera_preview_poll();
    g_copy_result = 1;
    g_dcmi_events = CAMERA_DCMI_EVENT_FRAME;
    camera_preview_poll();
    camera_preview_get_status(&status);
    failures += expect_int("copy_error_state", status.state,
                           CAMERA_PREVIEW_ERROR);
    failures += expect_u32("copy_error_release_deferred",
                           g_resource_release_calls, 1U);
    failures += expect_u32("copy_error_stop", g_dcmi_stop_calls, 2U);
    failures += expect_int("copy_error_stop_page", camera_preview_stop(), 0);
    failures += expect_u32("copy_error_release_on_stop",
                           g_resource_release_calls, 2U);

    g_copy_result = 0;
    failures += expect_int("reinit_for_restart_error", camera_preview_init(),
                           CAMERA_PREVIEW_IDLE);
    failures += expect_int("start_for_restart_error", camera_preview_start(),
                           0);
    g_dcmi_start_result = CAMERA_DCMI_HAL_ERROR;
    camera_preview_poll();
    camera_preview_get_status(&status);
    failures += expect_int("start_error_state", status.state,
                           CAMERA_PREVIEW_ERROR);
    failures += expect_u32("start_error_release_deferred",
                           g_resource_release_calls, 2U);
    failures += expect_u32("start_error_stop", g_dcmi_stop_calls, 4U);
    failures += expect_int("start_error_stop_page", camera_preview_stop(), 0);
    failures += expect_u32("start_error_release_on_stop",
                           g_resource_release_calls, 3U);

    if (failures == 0)
    {
        printf("PASS camera_preview_service\n");
        return 0;
    }
    printf("FAIL camera_preview_service failures=%d\n", failures);
    return 1;
}
