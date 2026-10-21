#include "ltdc_video_output.h"

#include "ltdc.h"
#include "lv_port_disp.h"
#include "sdram_layout.h"
#include <string.h>

typedef struct
{
    uint32_t display_addr;
    uint32_t decode_addr;
    uint32_t decode_offset;
    volatile uint32_t pending_addr;
    volatile uint8_t pending;
    volatile uint8_t swap_done;
    uint32_t lvgl_restore_addr;
    uint8_t prepared;
    uint8_t active;
} ltdc_video_output_context_t;

static ltdc_video_output_context_t g_ltdc_video;

static uint32_t ltdc_video_other_buffer(uint32_t address)
{
    return (address == SDRAM_VIDEO_RGB_A_ADDR) ?
        SDRAM_VIDEO_RGB_B_ADDR : SDRAM_VIDEO_RGB_A_ADDR;
}

static bool ltdc_video_wait_for_swap(uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();

    while (g_ltdc_video.swap_done == 0U)
    {
        if ((HAL_GetTick() - start_tick) >= timeout_ms)
        {
            g_ltdc_video.pending = 0U;
            return false;
        }
    }

    return true;
}

static bool ltdc_video_schedule_swap(uint32_t address, uint32_t timeout_ms)
{
    if ((timeout_ms == 0U) || (g_ltdc_video.pending != 0U))
    {
        return false;
    }

    g_ltdc_video.pending_addr = address;
    g_ltdc_video.swap_done = 0U;
    g_ltdc_video.pending = 1U;

    __DSB();
    if (HAL_LTDC_SetAddress_NoReload(&hltdc, address, 0U) != HAL_OK)
    {
        g_ltdc_video.pending = 0U;
        return false;
    }
    if (HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING) != HAL_OK)
    {
        g_ltdc_video.pending = 0U;
        return false;
    }

    return ltdc_video_wait_for_swap(timeout_ms);
}

bool ltdc_video_output_prepare(uint16_t width, uint16_t height)
{
    if ((width != 1024U) || (height == 0U) || (height > 600U) ||
        (g_ltdc_video.prepared != 0U))
    {
        return false;
    }

    memset((void *)SDRAM_VIDEO_RGB_A_ADDR, 0, SDRAM_VIDEO_RGB_FRAME_SIZE);
    memset((void *)SDRAM_VIDEO_RGB_B_ADDR, 0, SDRAM_VIDEO_RGB_FRAME_SIZE);
    SCB_CleanDCache_by_Addr((uint32_t *)SDRAM_VIDEO_RGB_A_ADDR,
                            SDRAM_VIDEO_RGB_FRAME_SIZE);
    SCB_CleanDCache_by_Addr((uint32_t *)SDRAM_VIDEO_RGB_B_ADDR,
                            SDRAM_VIDEO_RGB_FRAME_SIZE);

    g_ltdc_video.lvgl_restore_addr = disp_get_frame_address();
    g_ltdc_video.display_addr = g_ltdc_video.lvgl_restore_addr;
    g_ltdc_video.decode_addr = SDRAM_VIDEO_RGB_A_ADDR;
    g_ltdc_video.decode_offset = ((uint32_t)(600U - height) / 2U) * 1024U * 2U;
    g_ltdc_video.pending_addr = 0U;
    g_ltdc_video.pending = 0U;
    g_ltdc_video.swap_done = 0U;
    g_ltdc_video.active = 0U;
    g_ltdc_video.prepared = 1U;
    return true;
}

uint32_t ltdc_video_output_acquire_decode_target(void)
{
    if ((g_ltdc_video.prepared == 0U) || (g_ltdc_video.pending != 0U))
    {
        return 0U;
    }

    return g_ltdc_video.decode_addr + g_ltdc_video.decode_offset;
}

bool ltdc_video_output_activate_first_frame(uint32_t timeout_ms)
{
    if ((g_ltdc_video.prepared == 0U) || (g_ltdc_video.active != 0U))
    {
        return false;
    }

    disp_disable_update();
    if (!ltdc_video_schedule_swap(g_ltdc_video.decode_addr, timeout_ms))
    {
        disp_enable_update();
        return false;
    }

    g_ltdc_video.display_addr = g_ltdc_video.decode_addr;
    g_ltdc_video.decode_addr = ltdc_video_other_buffer(g_ltdc_video.display_addr);
    g_ltdc_video.active = 1U;
    return true;
}

bool ltdc_video_output_present(uint32_t timeout_ms)
{
    if ((g_ltdc_video.prepared == 0U) || (g_ltdc_video.active == 0U) ||
        (g_ltdc_video.decode_addr == g_ltdc_video.display_addr))
    {
        return false;
    }

    if (!ltdc_video_schedule_swap(g_ltdc_video.decode_addr, timeout_ms))
    {
        return false;
    }

    g_ltdc_video.display_addr = g_ltdc_video.decode_addr;
    g_ltdc_video.decode_addr = ltdc_video_other_buffer(g_ltdc_video.display_addr);
    return true;
}

void ltdc_video_output_abort(void)
{
    if (g_ltdc_video.prepared != 0U)
    {
        g_ltdc_video.pending = 0U;
        (void)ltdc_video_schedule_swap(g_ltdc_video.lvgl_restore_addr, 50U);
    }

    g_ltdc_video.display_addr = g_ltdc_video.lvgl_restore_addr;
    g_ltdc_video.decode_addr = SDRAM_VIDEO_RGB_A_ADDR;
    g_ltdc_video.decode_offset = 0U;
    g_ltdc_video.pending_addr = 0U;
    g_ltdc_video.pending = 0U;
    g_ltdc_video.swap_done = 0U;
    g_ltdc_video.lvgl_restore_addr = SDRAM_LVGL_FRAME_ADDR;
    g_ltdc_video.prepared = 0U;
    g_ltdc_video.active = 0U;
    disp_enable_update();
}

void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef *hltdc_handle)
{
    if (hltdc_handle == &hltdc)
    {
        disp_on_ltdc_reload();
        if (g_ltdc_video.pending != 0U)
        {
            g_ltdc_video.display_addr = g_ltdc_video.pending_addr;
            g_ltdc_video.pending = 0U;
            g_ltdc_video.swap_done = 1U;
        }
    }
}
