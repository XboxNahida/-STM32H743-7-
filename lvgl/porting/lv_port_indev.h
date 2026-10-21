
/**
 * @file lv_port_indev_templ.h
 *
 */

/*Copy this file as "lv_port_indev.h" and set this value to "1" to enable content*/
#if 1

#ifndef LV_PORT_INDEV_TEMPL_H
#define LV_PORT_INDEV_TEMPL_H

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
    uint8_t count;
    int16_t x[5];
    int16_t y[5];
} lv_port_touch_snapshot_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_port_indev_init(void);
void lv_port_indev_get_touch_snapshot(lv_port_touch_snapshot_t *snapshot);
void lv_port_indev_process_power_request(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_INDEV_TEMPL_H*/

#endif /*Disable/Enable content*/
