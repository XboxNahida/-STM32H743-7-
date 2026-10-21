#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "camera_dcmi_driver.h"
#include "sdram_layout.h"

#define TEST_CHUNK_LINES       40U
#define TEST_CHUNK_BYTES       (CAMERA_DCMI_LINE_BYTES * TEST_CHUNK_LINES)
#define TEST_CHUNK_WORDS       (TEST_CHUNK_BYTES / 4U)
#define TEST_CHUNKS_PER_FRAME  (CAMERA_DCMI_HEIGHT / TEST_CHUNK_LINES)

static uint32_t g_platform_init_calls;
static uint32_t g_platform_start_calls;
static uint32_t g_chunk_start_calls;
static uint32_t g_platform_stop_calls;
static uint32_t g_irq_enable_calls;
static uint32_t g_irq_disable_calls;
static uint32_t g_cache_calls;
static uintptr_t g_start_address;
static uintptr_t g_chunk_address[2];
static uint32_t g_start_words;
static uint8_t g_start_snapshot;
static uintptr_t g_cache_address;
static uint32_t g_cache_length;
static uint32_t g_copy_calls;
static uintptr_t g_copy_source[TEST_CHUNKS_PER_FRAME];
static uintptr_t g_copy_destination[TEST_CHUNKS_PER_FRAME];
static uint32_t g_copy_bytes[TEST_CHUNKS_PER_FRAME];

__attribute__((weak)) void camera_dcmi_chunk_callback(uint8_t completed_buffer)
{
    (void)completed_buffer;
}

int camera_dcmi_test_platform_init(void)
{
    g_platform_init_calls++;
    return 0;
}

int camera_dcmi_test_platform_start(uintptr_t address, uint32_t words,
                                    uint8_t snapshot)
{
    g_platform_start_calls++;
    g_start_address = address;
    g_start_words = words;
    g_start_snapshot = snapshot;
    return 0;
}

int camera_dcmi_test_platform_start_chunks(uintptr_t address0,
                                           uintptr_t address1,
                                           uint32_t words,
                                           uint8_t snapshot)
{
    g_chunk_start_calls++;
    g_chunk_address[0] = address0;
    g_chunk_address[1] = address1;
    g_start_words = words;
    g_start_snapshot = snapshot;
    return 0;
}

int camera_dcmi_test_platform_copy_chunk(uintptr_t source,
                                         uintptr_t destination,
                                         uint32_t bytes)
{
    if (g_copy_calls < TEST_CHUNKS_PER_FRAME)
    {
        g_copy_source[g_copy_calls] = source;
        g_copy_destination[g_copy_calls] = destination;
        g_copy_bytes[g_copy_calls] = bytes;
    }
    g_copy_calls++;
    return 0;
}

int camera_dcmi_test_platform_copy_busy(void)
{
    return 0;
}

int camera_dcmi_test_platform_stop(void)
{
    g_platform_stop_calls++;
    return 0;
}

void camera_dcmi_test_invalidate_cache(uintptr_t address, uint32_t length)
{
    g_cache_calls++;
    g_cache_address = address;
    g_cache_length = length;
}

void camera_dcmi_test_set_irqs(uint8_t enabled)
{
    if (enabled != 0U)
    {
        g_irq_enable_calls++;
    }
    else
    {
        g_irq_disable_calls++;
    }
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

static int expect_ptr(const char *name, uintptr_t actual, uintptr_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=0x%lX expected=0x%lX\n", name,
               (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

static int expect_source_boundary(void)
{
    FILE *file = fopen("Core/Peripheral/Src/camera_dcmi_driver.c", "rb");
    char source[20000];
    size_t length;
    int failures = 0;
    if (file == NULL)
    {
        printf("FAIL source_open\n");
        return 1;
    }
    length = fread(source, 1U, sizeof(source) - 1U, file);
    fclose(file);
    source[length] = '\0';
    if (strstr(source, "printf") != NULL || strstr(source, "lv_") != NULL ||
        strstr(source, "f_open") != NULL)
    {
        printf("FAIL irq_side_effects\n");
        failures++;
    }
    if ((strstr(source, "HAL_DMAEx_MultiBufferStart_IT") == NULL) ||
        (strstr(source, "DCMI_MODE_SNAPSHOT") == NULL) ||
        (strstr(source, "CAMERA_DCMI_CHUNK_WORDS") == NULL) ||
        (strstr(source,
                "__HAL_DMA_DISABLE_IT(&g_camera_dma, DMA_IT_FE)") == NULL) ||
        (strstr(source, "HAL_DMA_IRQHandler(&g_camera_dma)") == NULL))
    {
        printf("FAIL chunked_snapshot_dma_chain\n");
        failures++;
    }
    if ((strstr(source, "g_camera_chunk_buffers[2]") == NULL) ||
        (strstr(source, "memcpy((void *)destination") == NULL) ||
        (strstr(source, "DMA2D_M2M") != NULL) ||
        (strstr(source, "HAL_DCMI_Start_DMA") != NULL) ||
        (strstr(source, "g_camera_line_buffers[2]") != NULL))
    {
        printf("FAIL chunk_buffer_boundary\n");
        failures++;
    }
    if ((strstr(source, "CLEAR_BIT(DCMI->CR, DCMI_CR_CAPTURE)") == NULL) ||
        (strstr(source, "__HAL_DCMI_DISABLE_IT") == NULL))
    {
        printf("FAIL stop_clears_capture_and_interrupts\n");
        failures++;
    }
    return failures;
}

int main(void)
{
    camera_dcmi_status_t status;
    uint32_t events = 0U;
    int failures = 0;

    failures += expect_int("start_before_init", camera_dcmi_start(),
                           CAMERA_DCMI_NOT_READY);
    failures += expect_int("init", camera_dcmi_init(), CAMERA_DCMI_OK);
    camera_dcmi_get_status(&status);
    failures += expect_int("initialized", status.initialized, 1);
    failures += expect_u32("frame_words", status.frame_words,
                           CAMERA_DCMI_FRAME_WORDS);
    failures += expect_u32("frame_bytes", status.frame_bytes,
                           CAMERA_DCMI_FRAME_BYTES);
    failures += expect_u32("cache_line", status.cache_line,
                           CAMERA_DCMI_CACHE_LINE);
    failures += expect_int("start", camera_dcmi_start(), CAMERA_DCMI_OK);
    failures += expect_int("start_duplicate", camera_dcmi_start(),
                           CAMERA_DCMI_BUSY);
    failures += expect_u32("legacy_direct_start_unused", g_platform_start_calls,
                           0U);
    failures += expect_u32("chunk_start_once", g_chunk_start_calls, 1U);
    failures += expect_int("chunk_buffers_distinct",
                           g_chunk_address[0] != g_chunk_address[1], 1);
    failures += expect_int("chunk_buffer0_not_sdram",
                           g_chunk_address[0] != SDRAM_VIDEO_RGB_A_ADDR, 1);
    failures += expect_int("chunk_buffer1_not_sdram",
                           g_chunk_address[1] != SDRAM_VIDEO_RGB_A_ADDR, 1);
    failures += expect_u32("capture_words", g_start_words,
                           TEST_CHUNK_WORDS);
    failures += expect_int("snapshot_mode", g_start_snapshot, 1);
    failures += expect_u32("irq_enable_once", g_irq_enable_calls, 1U);

    camera_dcmi_frame_callback();
    failures += expect_u32("no_cache_in_isr", g_cache_calls, 0U);
    camera_dcmi_get_status(&status);
    failures += expect_int("early_frame_keeps_running", status.running, 1);
    failures += expect_int("poll_events", camera_dcmi_poll_event(&events),
                           CAMERA_DCMI_OK);
    failures += expect_u32("early_frame_not_published", events, 0U);

    for (uint32_t index = 0U; index < TEST_CHUNKS_PER_FRAME; index++)
    {
        camera_dcmi_chunk_callback((uint8_t)(index & 1U));
        failures += expect_ptr("copy_source", g_copy_source[index],
                               g_chunk_address[index & 1U]);
        failures += expect_ptr("copy_destination", g_copy_destination[index],
                               SDRAM_VIDEO_RGB_A_ADDR +
                               (index * TEST_CHUNK_BYTES));
        failures += expect_u32("copy_bytes", g_copy_bytes[index],
                               TEST_CHUNK_BYTES);
    }
    failures += expect_u32("ten_chunk_copies", g_copy_calls,
                           TEST_CHUNKS_PER_FRAME);
    camera_dcmi_chunk_callback(0U);
    camera_dcmi_chunk_callback(1U);
    camera_dcmi_get_status(&status);
    failures += expect_u32("late_tc_does_not_copy", g_copy_calls,
                           TEST_CHUNKS_PER_FRAME);
    failures += expect_int("late_tc_keeps_snapshot_running",
                           status.running, 1);
    failures += expect_u32("late_tc_is_not_driver_error",
                           status.error_events, 0U);
    failures += expect_u32("late_tc_is_not_dma_error",
                           status.dma_errors, 0U);
    failures += expect_u32("late_tc_counted",
                           status.extra_tc_events, 2U);
    failures += expect_u32("late_tc_has_no_business_error",
                           status.business_error_code,
                           CAMERA_DCMI_BUSINESS_ERROR_NONE);
    failures += expect_int("poll_complete_frame",
                           camera_dcmi_poll_event(&events), CAMERA_DCMI_OK);
    failures += expect_u32("frame_event", events, CAMERA_DCMI_EVENT_FRAME);
    failures += expect_u32("frame_cache_calls", g_cache_calls,
                           TEST_CHUNKS_PER_FRAME + 1U);
    failures += expect_ptr("frame_cache_address", g_cache_address,
                           SDRAM_VIDEO_RGB_A_ADDR);
    failures += expect_u32("frame_cache_length", g_cache_length,
                           CAMERA_DCMI_FRAME_BYTES);
    failures += expect_u32("snapshot_dma_quiesced", g_platform_stop_calls,
                           1U);
    failures += expect_u32("snapshot_irqs_disabled", g_irq_disable_calls,
                           1U);
    failures += expect_int("poll_clears", camera_dcmi_poll_event(&events),
                           CAMERA_DCMI_OK);
    failures += expect_u32("cleared_events", events, 0U);
    failures += expect_int("restart_after_snapshot", camera_dcmi_start(),
                           CAMERA_DCMI_OK);
    camera_dcmi_chunk_callback(2U);
    failures += expect_int("poll_error", camera_dcmi_poll_event(&events),
                           CAMERA_DCMI_OK);
    failures += expect_u32("error_event", events, CAMERA_DCMI_EVENT_ERROR);
    camera_dcmi_get_status(&status);
    failures += expect_u32("frame_count", status.frame_events, 1U);
    failures += expect_u32("error_count", status.error_events, 1U);
    failures += expect_u32("dma_error_count", status.dma_errors, 1U);
    failures += expect_u32("invalid_buffer_business_error",
                           status.business_error_code,
                           CAMERA_DCMI_BUSINESS_ERROR_INVALID_BUFFER);
    failures += expect_u32("dcmi_error_code_default",
                           status.dcmi_error_code, 0U);
    failures += expect_u32("dma_error_code_default",
                           status.dma_error_code, 0U);
    failures += expect_u32("line_count_default", status.line_count, 0U);
    failures += expect_u32("frame_seen_default", status.frame_seen, 0U);
    failures += expect_u32("dcmi_cr_default", status.dcmi_cr, 0U);
    failures += expect_u32("dcmi_sr_default", status.dcmi_sr, 0U);
    failures += expect_u32("dma_ndtr_default", status.dma_ndtr, 0U);
    failures += expect_u32("dma_cr_default", status.dma_cr, 0U);
    failures += expect_int("stop", camera_dcmi_stop(), CAMERA_DCMI_OK);
    failures += expect_int("stop_idempotent", camera_dcmi_stop(),
                           CAMERA_DCMI_OK);
    failures += expect_int("restart_clears_business_error",
                           camera_dcmi_start(), CAMERA_DCMI_OK);
    camera_dcmi_get_status(&status);
    failures += expect_u32("business_error_cleared_on_restart",
                           status.business_error_code,
                           CAMERA_DCMI_BUSINESS_ERROR_NONE);
    failures += expect_int("final_stop", camera_dcmi_stop(), CAMERA_DCMI_OK);
    failures += expect_u32("platform_init_once", g_platform_init_calls, 1U);
    failures += expect_u32("legacy_direct_start_still_unused",
                           g_platform_start_calls, 0U);
    failures += expect_u32("chunk_start_three_times", g_chunk_start_calls, 3U);
    failures += expect_u32("platform_stop_three_times", g_platform_stop_calls,
                           3U);
    failures += expect_u32("irq_enable_three_times", g_irq_enable_calls, 3U);
    failures += expect_u32("irq_disable_three_times", g_irq_disable_calls,
                           3U);
    failures += expect_source_boundary();

    if (failures == 0)
    {
        printf("PASS camera_dcmi_driver\n");
        return 0;
    }
    printf("FAIL camera_dcmi_driver failures=%d\n", failures);
    return 1;
}
