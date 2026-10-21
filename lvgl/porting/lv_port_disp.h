/**
 * @file lv_port_disp_templ.h
 *
 */

/*Copy this file as "lv_port_disp.h" and set this value to "1" to enable content*/
#if 1

#ifndef LV_PORT_DISP_TEMPL_H
#define LV_PORT_DISP_TEMPL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "../lvgl/lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct
{
    uint32_t reload_request_count;
    uint32_t reload_callback_count;
    uint32_t reload_timeout_count;
    uint32_t reload_failure_count;
    uint32_t active_frame_addr;
    uint32_t pending_frame_addr;
    uint32_t programmed_frame_addr;
    uint32_t ltdc_srcr;
    uint32_t ltdc_isr;
    uint32_t fifo_underrun_count;
    uint32_t transfer_error_count;
    uint32_t last_error_code;
    uint32_t last_error_cpsr;
    uint32_t last_error_frame_addr;
    uint8_t reload_pending;
    uint8_t flush_enabled;
} disp_diagnostics_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/* Initialize low level display driver */
void lv_port_disp_init(void);

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void);

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void);

/* Return the LVGL frame currently selected by LTDC. */
uint32_t disp_get_frame_address(void);

/* Notify the display port after the LTDC vertical-blank reload interrupt. */
void disp_on_ltdc_reload(void);

/* Copy display reload diagnostics without changing display state. */
void disp_get_diagnostics(disp_diagnostics_t *diagnostics);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_DISP_TEMPL_H*/

#endif /*Disable/Enable content*/
