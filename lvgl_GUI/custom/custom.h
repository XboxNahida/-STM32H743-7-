#ifndef __CUSTOM_H_
#define __CUSTOM_H_
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "gui_guider.h"

#define WAVEFORM_POINTS     500
#define HARMONIC_COUNT      5

/* ---- 全局数据 ---- */
extern lv_ui guider_ui;
extern uint16_t g_waveform_buffer[WAVEFORM_POINTS];
extern volatile uint8_t g_waveform_ready;
extern volatile uint8_t g_force_scroll_bottom;
extern uint8_t g_printf;

typedef struct
{
    uint8_t sdram_ready;
    uint8_t lvgl_ready;
    uint8_t display_ready;
    uint8_t input_ready;
    uint8_t ui_ready;
} app_lvgl_boot_status_t;

/* ---- 应用级 LVGL 接口（由 main 调用）---- */
bool app_lvgl_bootstrap_init(void);
void app_lvgl_finish_init(void);
void app_lvgl_handler(void);    /* 每帧调用刷新  */
void app_lvgl_get_boot_status(app_lvgl_boot_status_t *status);
void audio_volume_slider_changed(int32_t value);
void system_settings_sync_navigation_controls(lv_ui *ui);

/* ---- 业务回调 ---- */
void custom_init(lv_ui *ui);
void update_waveform_chart(void);
void update_harmonic_chart(void);
void update_measurement_display(void);

#ifdef __cplusplus
}
#endif
#endif /* __CUSTOM_H_ */
