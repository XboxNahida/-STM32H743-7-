/**
 * @file lv_port_disp.c
 * LVGL display driver - full-frame VBlank switching or legacy partial flush
 * Switch modes through lv_conf_ext.h
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include "dma2d.h"
#include "ltdc.h"
#include "sdram_layout.h"
#include <string.h>
#include "../../lvgl_GUI/custom/lv_conf_ext.h"

/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
    #define MY_DISP_HOR_RES    1024
#endif

#ifndef MY_DISP_VER_RES
    #define MY_DISP_VER_RES    600
#endif

#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */
#define DISP_RELOAD_TIMEOUT_MS 50U

/* LVGL display buffer: 60 lines x 1024 pixels x 2 bytes = 120KB, placed in AXI SRAM (DMA-accessible) */
#if !LV_USE_LTDC_FULL_FRAME_BUFFER
#define BUF_LINES 60
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);
#if LV_USE_LTDC_FULL_FRAME_BUFFER
static void disp_flush_wait(lv_display_t * disp);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
#if LV_USE_LTDC_FULL_FRAME_BUFFER
static volatile uint8_t g_reload_pending;
static volatile uint32_t g_pending_frame_addr;
static volatile uint32_t g_display_frame_addr = SDRAM_LVGL_FRAME_ADDR;
static volatile uint32_t g_reload_request_count;
static volatile uint32_t g_reload_callback_count;
static volatile uint32_t g_reload_timeout_count;
static volatile uint32_t g_reload_failure_count;
#endif
static volatile uint32_t g_fifo_underrun_count;
static volatile uint32_t g_transfer_error_count;
static volatile uint32_t g_last_ltdc_error_code;
static volatile uint32_t g_last_ltdc_error_cpsr;
static volatile uint32_t g_last_ltdc_error_frame_addr;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);

#if LV_USE_LTDC_FULL_FRAME_BUFFER
    memset((void *)SDRAM_LVGL_FRAME_ADDR, 0, SDRAM_LVGL_FRAME_SIZE);
    memset((void *)SDRAM_LVGL_BACK_FRAME_ADDR, 0, SDRAM_LVGL_FRAME_SIZE);
    SCB_CleanDCache_by_Addr((uint32_t *)SDRAM_LVGL_FRAME_ADDR,
                            SDRAM_LVGL_FRAME_SIZE);
    SCB_CleanDCache_by_Addr((uint32_t *)SDRAM_LVGL_BACK_FRAME_ADDR,
                            SDRAM_LVGL_FRAME_SIZE);
    lv_display_set_flush_wait_cb(disp, disp_flush_wait);
    /* LTDC starts on FRAME_ADDR, so render the first frame off-screen. */
    lv_display_set_buffers(disp,
                           (void *)SDRAM_LVGL_BACK_FRAME_ADDR,
                           (void *)SDRAM_LVGL_FRAME_ADDR,
                           SDRAM_LVGL_FRAME_SIZE,
                           LV_DISPLAY_RENDER_MODE_DIRECT);
#else
    /* 120KB display buffer in AXI SRAM (0x24000000), DMA2D-accessible, 32-byte aligned for cache line */
    static uint8_t buf_1[MY_DISP_HOR_RES * BUF_LINES * BYTE_PER_PIXEL]
        __attribute__((section(".lvgl_disp_buf"), aligned(32)));
    lv_display_set_buffers(disp, buf_1, NULL, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

uint32_t disp_get_frame_address(void)
{
#if LV_USE_LTDC_FULL_FRAME_BUFFER
    return g_display_frame_addr;
#else
    return SDRAM_LVGL_FRAME_ADDR;
#endif
}

void disp_on_ltdc_reload(void)
{
#if LV_USE_LTDC_FULL_FRAME_BUFFER
    if (g_reload_pending != 0U)
    {
        g_reload_callback_count++;
        g_display_frame_addr = g_pending_frame_addr;
        __DMB();
        g_reload_pending = 0U;
    }
#endif
}

void HAL_LTDC_ErrorCallback(LTDC_HandleTypeDef *hltdc_handle)
{
    uint32_t enabled_interrupts;
    uint32_t error_code;

    if (hltdc_handle != &hltdc)
    {
        return;
    }

    enabled_interrupts = hltdc_handle->Instance->IER;
    error_code = hltdc_handle->ErrorCode;
    if (((error_code & HAL_LTDC_ERROR_FU) != 0U) &&
        ((enabled_interrupts & LTDC_IT_FU) == 0U))
    {
        g_fifo_underrun_count++;
    }
    if (((error_code & HAL_LTDC_ERROR_TE) != 0U) &&
        ((enabled_interrupts & LTDC_IT_TE) == 0U))
    {
        g_transfer_error_count++;
    }

    g_last_ltdc_error_code = error_code;
    g_last_ltdc_error_cpsr = hltdc_handle->Instance->CPSR;
    g_last_ltdc_error_frame_addr =
        LTDC_LAYER(hltdc_handle, LTDC_LAYER_1)->CFBAR;

    /* HAL disables each LTDC error interrupt after its first event. */
    __HAL_LTDC_ENABLE_IT(hltdc_handle, LTDC_IT_TE | LTDC_IT_FU);
}

void disp_get_diagnostics(disp_diagnostics_t *diagnostics)
{
    uint32_t primask;

    if (diagnostics == NULL)
    {
        return;
    }
    memset(diagnostics, 0, sizeof(*diagnostics));
    primask = __get_PRIMASK();
    __disable_irq();
#if LV_USE_LTDC_FULL_FRAME_BUFFER
    diagnostics->reload_request_count = g_reload_request_count;
    diagnostics->reload_callback_count = g_reload_callback_count;
    diagnostics->reload_timeout_count = g_reload_timeout_count;
    diagnostics->reload_failure_count = g_reload_failure_count;
    diagnostics->active_frame_addr = g_display_frame_addr;
    diagnostics->pending_frame_addr = g_pending_frame_addr;
    diagnostics->reload_pending = g_reload_pending;
#else
    diagnostics->active_frame_addr = SDRAM_LVGL_FRAME_ADDR;
#endif
    diagnostics->flush_enabled = (uint8_t)(disp_flush_enabled != false);
    diagnostics->programmed_frame_addr =
        LTDC_LAYER(&hltdc, LTDC_LAYER_1)->CFBAR;
    diagnostics->ltdc_srcr = hltdc.Instance->SRCR;
    diagnostics->ltdc_isr = hltdc.Instance->ISR;
    diagnostics->fifo_underrun_count = g_fifo_underrun_count;
    diagnostics->transfer_error_count = g_transfer_error_count;
    diagnostics->last_error_code = g_last_ltdc_error_code;
    diagnostics->last_error_cpsr = g_last_ltdc_error_cpsr;
    diagnostics->last_error_frame_addr = g_last_ltdc_error_frame_addr;
    if (primask == 0U)
    {
        __enable_irq();
    }
}

#if LV_USE_LTDC_FULL_FRAME_BUFFER
static void disp_flush_wait(lv_display_t *disp)
{
    uint32_t start_tick = HAL_GetTick();

    (void)disp;
    while (g_reload_pending != 0U)
    {
        if ((HAL_GetTick() - start_tick) >= DISP_RELOAD_TIMEOUT_MS)
        {
            g_reload_timeout_count++;
            g_reload_pending = 0U;
            break;
        }
    }
}
#endif

/*Flush the content of the internal buffer the specific area on the display.
 *'px_map' contains the rendered image as raw pixel map and it should be copied to 'area' on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
#if LV_USE_LTDC_FULL_FRAME_BUFFER
    uint32_t target;

    (void)area;
    if (!lv_display_flush_is_last(disp))
    {
        return;
    }

    /* LVGL direct double buffering can copy prior dirty regions internally.
     * Clean all cache lines so LTDC sees both rendered and synchronized data. */
    SCB_CleanDCache();
    if (!disp_flush_enabled)
    {
        return;
    }

    target = (uint32_t)px_map;
    if ((target != SDRAM_LVGL_FRAME_ADDR) &&
        (target != SDRAM_LVGL_BACK_FRAME_ADDR))
    {
        return;
    }
    g_pending_frame_addr = target;
    g_reload_request_count++;
    g_reload_pending = 1U;
    __DSB();
    if ((HAL_LTDC_SetAddress_NoReload(&hltdc, target, 0U) != HAL_OK) ||
        (HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING) != HAL_OK))
    {
        g_reload_failure_count++;
        g_reload_pending = 0U;
    }
#else
    uint32_t cache_start;
    uint32_t cache_end;
    uint32_t cache_size;

    if(!disp_flush_enabled) {
        lv_display_flush_ready(disp);
        return;
    }

    uint16_t width = area->x2 - area->x1 + 1;
    uint16_t height = area->y2 - area->y1 + 1;

#if LV_USE_DMA2D_FLUSH
    /* ========== DMA2D accelerated flush ========== */
    uint32_t src_addr = (uint32_t)px_map;
    uint32_t dst_addr = 0xC0000000 + (area->y1 * MY_DISP_HOR_RES + area->x1) * 2;
    uint32_t src_size = (uint32_t)width * height * BYTE_PER_PIXEL;
    uint32_t dst_span = (((uint32_t)height - 1U) * MY_DISP_HOR_RES + width) * BYTE_PER_PIXEL;

    /* CPU renders px_map; DMA2D must read the latest cache contents. */
    cache_start = src_addr & ~0x1FUL;
    cache_end = (src_addr + src_size + 31U) & ~0x1FUL;
    SCB_CleanDCache_by_Addr((uint32_t *)cache_start, cache_end - cache_start);

    /* Discard stale framebuffer cache lines before DMA2D writes new pixels. */
    cache_start = dst_addr & ~0x1FUL;
    cache_end = (dst_addr + dst_span + 31U) & ~0x1FUL;
    cache_size = cache_end - cache_start;
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)cache_start, cache_size);

    /* Configure DMA2D: foreground source is contiguous, output skips per line */
    hdma2d.Init.Mode         = DMA2D_M2M;
    hdma2d.Init.ColorMode    = DMA2D_OUTPUT_RGB565;
    hdma2d.Init.OutputOffset = MY_DISP_HOR_RES - width;  /* skip to next line */
    HAL_DMA2D_Init(&hdma2d);

    hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
    hdma2d.LayerCfg[1].InputOffset    = 0;  /* source is contiguous in AXI SRAM */
    HAL_DMA2D_ConfigLayer(&hdma2d, 1);

    /* Blocking DMA2D copy (faster than CPU memcpy) */
    HAL_DMA2D_Start(&hdma2d, src_addr, dst_addr, width, height);

    /* Poll until DMA2D completes */
    while (HAL_DMA2D_PollForTransfer(&hdma2d, 100) != HAL_OK);

    /* DMA2D wrote SDRAM; invalidate CPU aliases instead of cleaning stale data over it. */
    SCB_InvalidateDCache_by_Addr((uint32_t *)cache_start, cache_size);

#else
    /* ========== CPU line-by-line memcpy (original mode) ========== */
    uint16_t *src = (uint16_t *)px_map;
    uint16_t *dst = (uint16_t *)(0xC0000000 + (area->y1 * MY_DISP_HOR_RES + area->x1) * 2);

    for(uint16_t y = 0; y < height; y++) {
        memcpy(dst, src, width * 2);
        src += width;
        dst += MY_DISP_HOR_RES;
    }

    /* Cache clean */
    uint32_t flush_addr = (uint32_t)(0xC0000000 + (area->y1 * MY_DISP_HOR_RES + area->x1) * 2);
    uint32_t flush_size = height * MY_DISP_HOR_RES * 2;
    SCB_CleanDCache_by_Addr((uint32_t *)flush_addr, flush_size);
#endif

    lv_display_flush_ready(disp);
#endif
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
