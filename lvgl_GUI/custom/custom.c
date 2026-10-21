/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <string.h>
#include "main.h"             /* UART_DISPLAY_BUF_SIZE, HAL_GetTick */
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "custom.h"
#include "widgets_init.h"
#include "../generated/events_init.h"
#include "command_parser.h"   /* 串口显示缓冲与清屏回调 */
#include "music_player_service.h"
#include "music_player_ui.h"
#include "usart.h"            /* huart1 */
#include "tim.h"              /* htim3 */
#include "image_viewer_ui.h"
#include "file_manager_ui.h"
#include "file_open_router.h"
#include "wav_playback_service.h"
#include "nes_player_ui.h"
#include "boot_progress_ui.h"
#include "sdram_layout.h"
#include "system_settings_service.h"
#include "oscilloscope_ui.h"

/**********************
 *  EXTERNS
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/
lv_ui guider_ui;                           /* Gui Guider UI 实例 */
volatile uint8_t g_force_scroll_bottom = 0; /* 强制滚动到底部标志 */
uint8_t g_printf = 0;                       /* Printf 按钮标志 */
static app_lvgl_boot_status_t g_lvgl_boot_status;
static uint8_t g_lvgl_bootstrap_ready;
static int32_t g_last_settings_brightness = 50;
extern int32_t saved_brightness;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void uart_display_timer_cb(lv_timer_t *timer);
static void uart_display_clear(void);

static void system_settings_apply_ui(void)
{
    system_settings_t settings;
    uint8_t brightness;
    uint16_t pwm;

    memset(&settings, 0, sizeof(settings));
    settings.brightness_percent = 50U;
    settings.volume_percent = 50U;
    system_settings_get(&settings);
    brightness = settings.brightness_percent;
    saved_brightness = (int32_t)brightness;
    g_last_settings_brightness = (int32_t)brightness;
    if (brightness < 1U)
    {
        brightness = 1U;
    }
    pwm = (uint16_t)(((uint32_t)brightness * 999U) / 100U);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwm);
    music_player_request_volume(settings.volume_percent, HAL_GetTick());

    if (guider_ui.screen_navigation_bar_slider_1 != NULL)
    {
        lv_slider_set_value(guider_ui.screen_navigation_bar_slider_1,
                            settings.brightness_percent, LV_ANIM_OFF);
    }
    if (guider_ui.screen_navigation_bar_slider_2 != NULL)
    {
        lv_slider_set_value(guider_ui.screen_navigation_bar_slider_2,
                            settings.volume_percent, LV_ANIM_OFF);
    }
}

void system_settings_sync_navigation_controls(lv_ui *ui)
{
    system_settings_t settings;

    if (ui == NULL)
    {
        return;
    }
    memset(&settings, 0, sizeof(settings));
    settings.brightness_percent = 50U;
    settings.volume_percent = 50U;
    system_settings_get(&settings);
    if (ui->screen_navigation_bar_slider_1 != NULL)
    {
        lv_slider_set_value(ui->screen_navigation_bar_slider_1,
                            settings.brightness_percent, LV_ANIM_OFF);
    }
    if (ui->screen_navigation_bar_slider_2 != NULL)
    {
        lv_slider_set_value(ui->screen_navigation_bar_slider_2,
                            settings.volume_percent, LV_ANIM_OFF);
    }
}

bool app_lvgl_bootstrap_init(void)
{
    if (g_lvgl_bootstrap_ready != 0U)
    {
        return true;
    }

    memset(&g_lvgl_boot_status, 0, sizeof(g_lvgl_boot_status));
    memset((void *)SDRAM_LVGL_POOL_ADDR, 0, SDRAM_LVGL_POOL_SIZE);

    /* verify SDRAM read-back */
    *(volatile uint32_t *)SDRAM_LVGL_POOL_ADDR = 0xA5A5A5A5;
    if (*(volatile uint32_t *)SDRAM_LVGL_POOL_ADDR != 0xA5A5A5A5) {
        printf("[BOOT] display FAIL stage=sdram_verify code=1\r\n");
        while(1);
    }
    g_lvgl_boot_status.sdram_ready = 1U;

    lv_init();
    g_lvgl_boot_status.lvgl_ready = 1U;
    lv_tick_set_cb(HAL_GetTick);
    lv_port_disp_init();
    g_lvgl_boot_status.display_ready = 1U;

    if (!boot_progress_ui_create())
    {
        printf("[BOOT] display FAIL stage=progress_ui code=2\r\n");
        return false;
    }
    g_lvgl_bootstrap_ready = 1U;
    return true;
}

void app_lvgl_finish_init(void)
{
    if ((g_lvgl_bootstrap_ready == 0U) ||
        (g_lvgl_boot_status.ui_ready != 0U))
    {
        return;
    }

    lv_port_indev_init();
    g_lvgl_boot_status.input_ready = 1U;

    setup_ui(&guider_ui);
    events_init(&guider_ui);
    music_player_ui_init();
    system_settings_apply_ui();
    image_viewer_ui_init();
    file_manager_ui_init();
    file_open_router_init();
    file_manager_ui_set_open_callback(file_open_router_request);
    nes_player_ui_init();
    oscilloscope_ui_init();
    command_parser_set_image_callbacks(image_viewer_ui_open_file,
                                       image_viewer_ui_open_album);
    command_parser_set_clear_callback(uart_display_clear);

    lv_timer_create(rtc_clock_1s_timer_cb, 1000, NULL);
    lv_timer_create(uart_display_timer_cb, 100, NULL);
    HAL_UART_Receive_IT(&huart1, uart_rx_buf, 1);
    g_lvgl_boot_status.ui_ready = 1U;
}

void app_lvgl_get_boot_status(app_lvgl_boot_status_t *status)
{
    if (status != NULL)
    {
        *status = g_lvgl_boot_status;
    }
}

void audio_volume_slider_changed(int32_t value)
{
    if (value < 0)
    {
        value = 0;
    }
    else if (value > 100)
    {
        value = 100;
    }
    music_player_request_volume((uint8_t)value, HAL_GetTick());
    system_settings_update_volume((uint8_t)value, HAL_GetTick());
    system_settings_request_save();
}

/* ================================================================
 *  应用级LVGL每帧处理（在main循环中调用）
 * ================================================================ */
void app_lvgl_handler(void)
{
    int32_t brightness = saved_brightness;

    if (brightness < 0)
    {
        brightness = 0;
    }
    else if (brightness > 100)
    {
        brightness = 100;
    }
    if (brightness != g_last_settings_brightness)
    {
        g_last_settings_brightness = brightness;
        system_settings_update_brightness((uint8_t)brightness, HAL_GetTick());
    }
    music_player_ui_poll(HAL_GetTick());
    image_viewer_ui_poll();
    file_manager_ui_poll();
    file_open_router_poll();
    wav_playback_poll();
    nes_player_ui_poll();
    lv_port_indev_process_power_request();
    oscilloscope_ui_poll();
    lv_timer_handler();
    HAL_Delay(5);
}

/* ================================================================
 *  LVGL定时器：增量更新串口文本到界面
 * ================================================================ */
static void uart_display_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    /* 批量追加主循环已生成的新文本。 */
    if (uart_display_len > uart_last_display_len && guider_ui.Serial_Port_ta_display) {
        lv_obj_t *ta = guider_ui.Serial_Port_ta_display;
        int32_t scroll_bottom = lv_obj_get_scroll_bottom(ta);
        bool was_at_bottom = (scroll_bottom < 20);

        uint16_t new_len  = uart_display_len - uart_last_display_len;
        char    *new_text = &uart_display_buf[uart_last_display_len];
        uart_last_display_len = uart_display_len;

        char temp_char = new_text[new_len];
        new_text[new_len] = '\0';
        lv_textarea_add_text(ta, new_text);
        new_text[new_len] = temp_char;

        if (was_at_bottom || g_force_scroll_bottom) {
            lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
            g_force_scroll_bottom = 0;
        }
    }
}

static void uart_display_clear(void)
{
    if ((guider_ui.Serial_Port_ta_display != NULL) &&
        lv_obj_is_valid(guider_ui.Serial_Port_ta_display))
    {
        lv_textarea_set_text(guider_ui.Serial_Port_ta_display, "");
    }
    memset(uart_display_buf, 0, UART_DISPLAY_BUF_SIZE);
    uart_display_len = 0U;
    uart_last_display_len = 0U;
}

/* ================================================================
 *  Gui Guider 鍗犱綅鍑芥暟
 * ================================================================ */
void custom_init(lv_ui *ui)
{
    (void)ui;
}

void update_waveform_chart(void)  { }
void update_harmonic_chart(void)  { }
void update_measurement_display(void)
{
    char buf[512];
    (void)buf;
}
