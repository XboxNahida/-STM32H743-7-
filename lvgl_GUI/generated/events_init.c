/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"
#include <string.h>
#include <math.h>
#include "usart.h"  // 引用 UART 句柄
#include "tim.h"
#include "widgets_init.h"
#include "ff.h"
#include "main.h"
#include "hander.h"
#include "command_parser.h"
#include "mjpeg.h"	//串口指令处理
#include "avi.h"
#include "dma2d.h"
#include "lv_port_disp.h"
#include "ltdc_video_output.h"
#include "custom.h"		// g_printf, g_force_scroll_bottom
/* MUSIC APP USER PATCH BEGIN: generated event calls custom music UI */
#include "music_player_ui.h"
#include "nes_player_ui.h"
#include "music_player_service.h"
/* MUSIC APP USER PATCH END */
/* AVI AUDIO USER PATCH BEGIN: generated AVI events feed shared audio output. */
#include "audio_playback_service.h"
/* AVI AUDIO USER PATCH END */
#include "image_viewer_ui.h"
#include "file_manager_ui.h"
#include "sdcard_manager.h"
#include "camera_ui_bridge.h"
#include "rtc.h"
/* STAGE6 NOTES USER PATCH: windowed full-document notes service. */
#include "notes_editor_ui.h"
#include "notes_storage_service.h"
#include "gt9xxx.h"   /* touch_result_t for pinch-zoom */
extern touch_result_t shared_touch_data;  /* from lv_port_indev.c */
uint8_t g_viewer_active = 0;               /* 1 when image viewer screen is shown */

#define AVI_SD_SESSION_OWNER 0x0504U

static uint8_t g_avi_sd_session_held;

static int avi_sd_session_acquire(void)
{
    if (g_avi_sd_session_held != 0U)
    {
        return 0;
    }
    if (sdcard_manager_session_acquire(AVI_SD_SESSION_OWNER) !=
        SDCARD_MANAGER_OK)
    {
        return 1;
    }
    g_avi_sd_session_held = 1U;
    return 0;
}

static void avi_sd_session_release(void)
{
    if (g_avi_sd_session_held == 0U)
    {
        return;
    }
    (void)sdcard_manager_session_release(AVI_SD_SESSION_OWNER);
    g_avi_sd_session_held = 0U;
}


/* Forward declaration */
static uint32_t gbk_to_utf8(const char *gbk, char *utf8, uint32_t utf8_max);
#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif

// ====== 外部变量声明（通过 command_parser.h / custom.h / tim.h 统一引用）======

/* countdown timer globals (defined in widgets_init.c) */
extern int32_t g_countdown_h, g_countdown_m, g_countdown_s;
extern int32_t g_countdown_preset_h, g_countdown_preset_m, g_countdown_preset_s;
extern uint8_t g_countdown_state;
extern int32_t g_lap_h[64], g_lap_m[64], g_lap_s[64];
extern uint8_t g_lap_count;
#define MAX_LAPS 64
extern char g_opened_file_path[300];  /* from sd_file_open_handler */
extern UART_HandleTypeDef huart1;      // 串口句柄
extern TIM_HandleTypeDef htim3;
int32_t saved_brightness = 50;		//滑块值

/* LOCK RETURN USER PATCH BEGIN: preserve the page behind navigation/lock. */
typedef enum
{
    NAV_RETURN_DESKTOP = 0,
    NAV_RETURN_NOTES,
    NAV_RETURN_SIGNAL_PROC,
    NAV_RETURN_YUANSHEN,
    NAV_RETURN_MUSIC,
    /* STAGE6 UI USER PATCH BEGIN: new application return sources. */
    NAV_RETURN_CALCULATOR,
    NAV_RETURN_PIANO,
    NAV_RETURN_DRAWING
    /* STAGE6 UI USER PATCH END */
} navigation_return_target_t;

static navigation_return_target_t g_navigation_return_target =
    NAV_RETURN_DESKTOP;

static void navigation_set_return_target(navigation_return_target_t target)
{
    g_navigation_return_target = target;
}

static void navigation_restore_saved_screen(bool *old_screen_del,
                                             lv_screen_load_anim_t anim_type,
                                             uint32_t time,
                                             uint32_t delay)
{
    music_player_ui_set_navigation_return(0);
    switch (g_navigation_return_target)
    {
    case NAV_RETURN_NOTES:
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_notes,
            guider_ui.screen_notes_del, old_screen_del,
            setup_scr_screen_notes, anim_type, time, delay, true, true);
        break;
    case NAV_RETURN_SIGNAL_PROC:
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_Signal_Proc,
            guider_ui.screen_Signal_Proc_del, old_screen_del,
            setup_scr_screen_Signal_Proc, anim_type, time, delay, true, true);
        break;
    case NAV_RETURN_YUANSHEN:
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_yuanshen,
            guider_ui.screen_yuanshen_del, old_screen_del,
            setup_scr_screen_yuanshen, anim_type, time, delay, true, true);
        break;
    case NAV_RETURN_MUSIC:
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_music,
            guider_ui.screen_music_del, old_screen_del,
            setup_scr_screen_music, anim_type, time, delay, true, true);
        break;
    /* STAGE6 UI USER PATCH BEGIN: restore calculator after navigation. */
    case NAV_RETURN_CALCULATOR:
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_calculator,
            guider_ui.screen_calculator_del, old_screen_del,
            setup_scr_screen_calculator, anim_type, time, delay, true, true);
        break;
    case NAV_RETURN_PIANO:
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_piano,
            guider_ui.screen_piano_del, old_screen_del,
            setup_scr_screen_piano, anim_type, time, delay, true, true);
        break;
    case NAV_RETURN_DRAWING:
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_drawing,
            guider_ui.screen_drawing_del, old_screen_del,
            setup_scr_screen_drawing, anim_type, time, delay, true, true);
        break;
    /* STAGE6 UI USER PATCH END */
    case NAV_RETURN_DESKTOP:
    default:
        ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop,
            guider_ui.DeskTop_del, old_screen_del,
            setup_scr_DeskTop, anim_type, time, delay, true, true);
        break;
    }
}
/* LOCK RETURN USER PATCH END */


extern volatile uint8_t g_auto_measure_enable;	// 自动处理数据标志
extern volatile uint8_t g_auto_measure_flag;  // 定时器自动测量标志
extern State_Machine g_meas_state;             // 测量状态机状态
extern void Signal_Measurement_NonBlocking(void); // 状态机处理函数

// 滑块回调函数
static void Menu_slider_LCD_BL_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t * slider = lv_event_get_target(e);
        int32_t val = lv_slider_get_value(slider);              // 0 - 100
				// 保存当前值
        saved_brightness = val;
				// 设置最小亮度 1%（值范围 10-999）
        if(val < 1) val = 1;
        uint16_t pwm = (uint16_t)(val * 999 / 100);           // 映射到 10 - 999
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwm);  // CH2 输出 PWM
    }
}
// ==============================================================================================


static void lock_screen_cont_lock_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCROLL_END:
    {
        /* LOCK RETURN USER PATCH: unlock to the page before navigation. */
        navigation_restore_saved_screen(&guider_ui.lock_screen_del,
            LV_SCR_LOAD_ANIM_OVER_TOP, 200U, 50U);
        break;
    }
    default:
        break;
    }
}

static void lock_screen_label_navigation_bar1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_RELEASED:
    {
        break;
    }
    case LV_EVENT_SCROLL_END:
    {
        break;
    }
    default:
        break;
    }
}

void events_init_lock_screen (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->lock_screen_cont_lock, lock_screen_cont_lock_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->lock_screen_label_navigation_bar1, lock_screen_label_navigation_bar1_event_handler, LV_EVENT_ALL, ui);
}

static void DeskTop_img_noteAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        g_opened_file_path[0] = 0;  /* new file from desktop icon */
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_notes, guider_ui.screen_notes_del, &guider_ui.DeskTop_del, setup_scr_screen_notes, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_img_settingAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_remove_flag(guider_ui.DeskTop_cont_setting, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_img_wetherAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        char buf[8];
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_alarm_h, buf, sizeof(buf));
        int hour = atoi(buf);
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_1, buf, sizeof(buf));
        int min = atoi(buf);
        alarm_add((uint8_t)hour, (uint8_t)min);
        /* Update label immediately */
        if (lv_obj_is_valid(guider_ui.DeskTop_label_23))
            lv_label_set_text_fmt(guider_ui.DeskTop_label_23,
                "闹钟：%02d:%02d", hour, min);
        lv_obj_add_flag(guider_ui.DeskTop_tileview_alarm, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_img_edgeAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        char buf[8];
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_alarm_h, buf, sizeof(buf));
        int hour = atoi(buf);
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_1, buf, sizeof(buf));
        int min = atoi(buf);
        alarm_add((uint8_t)hour, (uint8_t)min);
        /* Update label immediately */
        if (lv_obj_is_valid(guider_ui.DeskTop_label_23))
            lv_label_set_text_fmt(guider_ui.DeskTop_label_23,
                "闹钟：%02d:%02d", hour, min);
        lv_obj_add_flag(guider_ui.DeskTop_tileview_alarm, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_img_calendarAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        char buf[8];
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_alarm_h, buf, sizeof(buf));
        int hour = atoi(buf);
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_1, buf, sizeof(buf));
        int min = atoi(buf);
        alarm_add((uint8_t)hour, (uint8_t)min);
        /* Update label immediately */
        if (lv_obj_is_valid(guider_ui.DeskTop_label_23))
            lv_label_set_text_fmt(guider_ui.DeskTop_label_23,
                "闹钟：%02d:%02d", hour, min);
        lv_obj_add_flag(guider_ui.DeskTop_tileview_alarm, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void image_viewer_open_album(void);

static void DeskTop_img_albumAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        image_viewer_open_album();
        break;
    }
    default:
        break;
    }
}

static uint8_t avi_player_direct_play(void);  /* forward, 0=ok 1=fail */

static void DeskTop_img_yuanshenAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        if (avi_player_direct_play() != 0) {
            ui_load_scr_animation(&guider_ui, &guider_ui.screen_yuanshen, guider_ui.screen_yuanshen_del, &guider_ui.DeskTop_del, setup_scr_screen_yuanshen, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
        }
        break;
    }
    default:
        break;
    }
}

static void DeskTop_img_alarmAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_remove_flag(guider_ui.DeskTop_cont_setting, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(guider_ui.DeskTop_tileview_alarm, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(guider_ui.DeskTop_tileview_timekeeping, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_img_calculatorAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        /* STAGE6 UI USER PATCH: calculator entry opens its own screen. */
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_calculator,
            guider_ui.screen_calculator_del, &guider_ui.DeskTop_del,
            setup_scr_screen_calculator, LV_SCR_LOAD_ANIM_OVER_TOP,
            200, 200, true, true);
        break;
    }
    default:
        break;
    }
}

/* STAGE6 UI USER PATCH BEGIN: calculator navigation events. */
/* STAGE6 UI USER PATCH: notes-template container scroll. */
static void screen_calculator_top_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCROLL_END) return;
    navigation_set_return_target(NAV_RETURN_CALCULATOR);
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_navigation_bar,
        guider_ui.screen_navigation_bar_del,
        &guider_ui.screen_calculator_del,
        setup_scr_screen_navigation_bar, LV_SCR_LOAD_ANIM_OVER_BOTTOM,
        50, 50, true, true);
}

static void screen_calculator_back_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop,
        guider_ui.DeskTop_del, &guider_ui.screen_calculator_del,
        setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP,
        200, 200, true, true);
}

void events_init_screen_calculator(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_calculator_cont_1,
        screen_calculator_top_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_calculator_btn_1,
        screen_calculator_back_event_handler, LV_EVENT_ALL, ui);
}
/* STAGE6 UI USER PATCH END */

static void DeskTop_img_folderAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        file_manager_ui_request_open();
        break;
    }
    default:
        break;
    }
}

/* MUSIC APP USER PATCH BEGIN: generated-style desktop entry event */
static void DeskTop_img_musicAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        /* MUSIC APP USER PATCH: defer cleanup until event dispatch ends. */
        music_player_ui_request_open();
        break;
    }
    default:
        break;
    }
}
/* MUSIC APP USER PATCH END */

/* STAGE6 UI USER PATCH BEGIN: piano desktop entry and navigation. */
static void DeskTop_img_draw_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_drawing,
        guider_ui.screen_drawing_del, &guider_ui.DeskTop_del,
        setup_scr_screen_drawing, LV_SCR_LOAD_ANIM_OVER_TOP,
        200, 200, true, true);
}

static void DeskTop_img_piano_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_piano,
        guider_ui.screen_piano_del, &guider_ui.DeskTop_del,
        setup_scr_screen_piano, LV_SCR_LOAD_ANIM_OVER_TOP,
        200, 200, true, true);
}

static void DeskTop_img_nes_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    nes_player_ui_request_open();
}

static void screen_piano_top_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCROLL_END) return;
    navigation_set_return_target(NAV_RETURN_PIANO);
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_navigation_bar,
        guider_ui.screen_navigation_bar_del, &guider_ui.screen_piano_del,
        setup_scr_screen_navigation_bar, LV_SCR_LOAD_ANIM_OVER_BOTTOM,
        50, 50, true, true);
}

static void screen_piano_back_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop,
        guider_ui.DeskTop_del, &guider_ui.screen_piano_del,
        setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP,
        200, 200, true, true);
}

void events_init_screen_piano(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_piano_cont_1,
        screen_piano_top_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_piano_btn_back,
        screen_piano_back_event_handler, LV_EVENT_ALL, ui);
}

static void screen_drawing_top_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCROLL_END) return;
    navigation_set_return_target(NAV_RETURN_DRAWING);
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_navigation_bar,
        guider_ui.screen_navigation_bar_del, &guider_ui.screen_drawing_del,
        setup_scr_screen_navigation_bar, LV_SCR_LOAD_ANIM_OVER_BOTTOM,
        50, 50, true, true);
}

static void screen_drawing_back_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop,
        guider_ui.DeskTop_del, &guider_ui.screen_drawing_del,
        setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP,
        200, 200, true, true);
}

void events_init_screen_drawing(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_drawing_cont_1,
        screen_drawing_top_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_drawing_btn_back,
        screen_drawing_back_event_handler, LV_EVENT_ALL, ui);
}
/* STAGE6 UI USER PATCH END */

static void DeskTop_img_phoneAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        char buf[8];
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_alarm_h, buf, sizeof(buf));
        int hour = atoi(buf);
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_1, buf, sizeof(buf));
        int min = atoi(buf);
        alarm_add((uint8_t)hour, (uint8_t)min);
        /* Update label immediately */
        if (lv_obj_is_valid(guider_ui.DeskTop_label_23))
            lv_label_set_text_fmt(guider_ui.DeskTop_label_23,
                "闹钟：%02d:%02d", hour, min);
        lv_obj_add_flag(guider_ui.DeskTop_tileview_alarm, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_img_messageAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        char buf[8];
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_alarm_h, buf, sizeof(buf));
        int hour = atoi(buf);
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_1, buf, sizeof(buf));
        int min = atoi(buf);
        alarm_add((uint8_t)hour, (uint8_t)min);
        /* Update label immediately */
        if (lv_obj_is_valid(guider_ui.DeskTop_label_23))
            lv_label_set_text_fmt(guider_ui.DeskTop_label_23,
                "闹钟：%02d:%02d", hour, min);
        lv_obj_add_flag(guider_ui.DeskTop_tileview_alarm, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_img_cameraAPP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_camera, guider_ui.screen_camera_del, &guider_ui.DeskTop_del, setup_scr_screen_camera, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_custom_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_remove_flag(guider_ui.DeskTop_tileview_WiFi1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(guider_ui.DeskTop_spinner_1, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_Signal_Proc, guider_ui.screen_Signal_Proc_del, &guider_ui.DeskTop_del, setup_scr_screen_Signal_Proc, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_Debug_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        char buf[8];
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_alarm_h, buf, sizeof(buf));
        int hour = atoi(buf);
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_1, buf, sizeof(buf));
        int min = atoi(buf);
        alarm_add((uint8_t)hour, (uint8_t)min);
        /* Update label immediately */
        if (lv_obj_is_valid(guider_ui.DeskTop_label_23))
            lv_label_set_text_fmt(guider_ui.DeskTop_label_23,
                "闹钟：%02d:%02d", hour, min);
        lv_obj_add_flag(guider_ui.DeskTop_tileview_alarm, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_6_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_remove_flag(guider_ui.DeskTop_tileview_Time, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_7_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_remove_flag(guider_ui.DeskTop_tileview_alarm, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_8_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_remove_flag(guider_ui.DeskTop_tileview_timekeeping, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}


void image_viewer_show_jpeg(uint8_t *buf, uint32_t size, const char *name);

/* ---- SD card file browser ---- */
static char g_sd_path[256] = "0:/";
#define G_FILE_CONTENT_SIZE 262144
static char *const g_file_content = (char *)0xC0460000;  /* 256KB, SDRAM (after IMG3, before video bufs) */
static uint32_t g_file_content_len = 0;
static char g_opened_file_path[300] = "";  /* currently opened file */
static FATFS g_sd_fs;
static uint8_t g_sd_mounted = 0;

/* AVI AUDIO USER PATCH BEGIN: shared by file-browser and direct AVI paths. */
static uint8_t g_avi_audio_active;

static void avi_player_audio_stop(void)
{
    if (g_avi_audio_active != 0U)
    {
        (void)audio_stream_stop();
        g_avi_audio_active = 0U;
    }
}

static void avi_player_audio_start(void)
{
    audio_stream_format_t format;
    int result;

    avi_player_audio_stop();
    if ((g_avix.SampleRate == 0U) || (g_avix.Channels == 0U))
    {
        return;
    }
    if (g_avix.AudioType == AVI_AUDIO_FORMAT_PCM)
    {
        format = AUDIO_STREAM_FORMAT_PCM;
    }
    else if (g_avix.AudioType == AVI_AUDIO_FORMAT_MP3)
    {
        format = AUDIO_STREAM_FORMAT_MP3;
    }
    else
    {
        printf("[AVI-AUDIO] unsupported format=0x%04X\r\n",
               (unsigned int)g_avix.AudioType);
        return;
    }
    result = audio_stream_start(format, g_avix.SampleRate,
                                (uint8_t)g_avix.Channels,
                                (uint8_t)g_avix.BitsPerSample);
    if (result != AUDIO_OK)
    {
        printf("[AVI-AUDIO] start failed result=%d format=0x%04X rate=%lu channels=%u bits=%u\r\n",
               result, (unsigned int)g_avix.AudioType,
               (unsigned long)g_avix.SampleRate,
               (unsigned int)g_avix.Channels,
               (unsigned int)g_avix.BitsPerSample);
        return;
    }
    g_avi_audio_active = 1U;
}

static uint8_t avi_player_audio_read(FIL *file, uint32_t payload_size)
{
    UINT bytes_read = 0U;
    FRESULT result;
    int audio_result;

    if ((file == NULL) || (payload_size > G_FILE_CONTENT_SIZE))
    {
        return 1U;
    }
    if (g_avi_audio_active == 0U)
    {
        return (f_lseek(file, f_tell(file) + payload_size +
                       (payload_size & 1U)) == FR_OK) ? 0U : 1U;
    }
    result = f_read(file, (uint8_t *)g_file_content,
                    payload_size, &bytes_read);
    if ((result != FR_OK) || (bytes_read != payload_size))
    {
        return 1U;
    }
    if ((payload_size & 1U) != 0U)
    {
        if (f_lseek(file, f_tell(file) + 1U) != FR_OK)
        {
            return 1U;
        }
    }
    audio_result = audio_stream_push((const uint8_t *)g_file_content,
                                     payload_size);
    if (audio_result == AUDIO_BUSY)
    {
        audio_poll();
        audio_result = audio_stream_push((const uint8_t *)g_file_content,
                                         payload_size);
    }
    if (audio_result != AUDIO_OK)
    {
        printf("[AVI-AUDIO] push failed result=%d size=%lu\r\n",
               audio_result, (unsigned long)payload_size);
        avi_player_audio_stop();
    }
    audio_poll();
    return 0U;
}
/* AVI AUDIO USER PATCH END */

/* ---- SD card file open callback ---- */
static int avi_stricmp(const char *a, const char *b);
static void avi_player_open(const char *path, const char *name);

int events_open_notes_file(const char *path)
{
    size_t length;
    if (path == NULL) return 1;
    length = strlen(path);
    if ((length == 0U) || (length >= sizeof(g_opened_file_path))) return 1;
    memcpy(g_opened_file_path, path, length + 1U);
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_notes,
        guider_ui.screen_notes_del, &guider_ui.DeskTop_del,
        setup_scr_screen_notes, LV_SCR_LOAD_ANIM_OVER_TOP,
        200, 0, true, true);
    return 0;
}

int events_open_avi_file(const char *path)
{
    const char *name;
    const char *cursor;
    if ((path == NULL) || (path[0] == '\0')) return 1;
    name = path;
    for (cursor = path; *cursor != '\0'; cursor++)
        if (*cursor == '/') name = cursor + 1;
    avi_player_open(path, name);
    return 0;
}

static void sd_file_open_handler(lv_event_t *e)
{
    const char *name;
    char full_path[300];
    FIL fil;
    FRESULT res;
    UINT br;
    uint32_t total;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    name = (const char*)lv_obj_get_user_data(lv_event_get_current_target(e));
    if (!name || name[0] == (char)0) return;
        /* ---- Determine file type by extension ---- */
    {
        const char *ext = NULL;
        uint32_t nlen = strlen(name);
        int32_t i;
        for (i = (int32_t)nlen - 1; i >= 0; i--) {
            if (name[i] == '.') { ext = &name[i]; break; }
        }

        /* Build full path */
        snprintf(full_path, sizeof(full_path), "%s%s", g_sd_path, name);
        strcpy(g_opened_file_path, full_path);

        /* === BLOCK: Non-openable files === */
        if (ext) {
            if (
                avi_stricmp(ext, ".bin") == 0 || avi_stricmp(ext, ".exe") == 0 ||
                avi_stricmp(ext, ".sys") == 0 || avi_stricmp(ext, ".dll") == 0 ||
                avi_stricmp(ext, ".dat") == 0 || avi_stricmp(ext, ".hex") == 0 ||
                avi_stricmp(ext, ".axf") == 0 || avi_stricmp(ext, ".o")   == 0 ||
                avi_stricmp(ext, ".elf") == 0 || avi_stricmp(ext, ".map") == 0 ||
                avi_stricmp(ext, ".so")  == 0 || avi_stricmp(ext, ".lib") == 0
            ) {
                lv_label_set_text(guider_ui.DeskTop_label_21,
                    "无法打开此类型文件");
                return;
            }
        }

        /* === AVI: Open in video player === */
        if (ext && avi_stricmp(ext, ".avi") == 0) {
            avi_player_open(full_path, name);
            return;
        }

        /* === Image files: JPEG via image viewer === */
        if (ext && (avi_stricmp(ext, ".jpg") == 0 || avi_stricmp(ext, ".jpeg") == 0)) {
            if (image_viewer_ui_open_file(full_path) != IMAGE_VIEWER_OK) {
                lv_label_set_text(guider_ui.DeskTop_label_21, "打开失败");
            }
            return;
        }
    }

    /* === Text files: Open in notes ===
       (.txt, .c, .h, .md, .cfg, .ini, .log, .json, .xml, .csv)
       Also fallback: unrecognized extension -> try as text */

    snprintf(full_path, sizeof(full_path), "%s%s", g_sd_path, name);
    strcpy(g_opened_file_path, full_path);

    res = f_open(&fil, full_path, FA_READ);
    if (res != FR_OK) {
        char err[48];
        snprintf(err, sizeof(err), "打开失败[%d]", (int)res);
        lv_label_set_text(guider_ui.DeskTop_label_21, err);
        return;
    }

    total = f_size(&fil);
    if (total > G_FILE_CONTENT_SIZE - 1) total = G_FILE_CONTENT_SIZE - 1;

    res = f_read(&fil, g_file_content, (UINT)total, &br);
    f_close(&fil);

    if (res != FR_OK || br == 0) {
        char err[48];
        snprintf(err, sizeof(err), "读取失败[%d]", (int)res);
        lv_label_set_text(guider_ui.DeskTop_label_21, err);
        return;
    }

    g_file_content[br] = (char)0;
    g_file_content_len = br;

    /* Set filename label before screen transition */
    {
        char utf8_name[256];
        gbk_to_utf8(name, utf8_name, sizeof(utf8_name));
        lv_label_set_text(guider_ui.DeskTop_label_21, utf8_name);
    }

    /* Navigate to notes screen and display content */
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_notes,
        guider_ui.screen_notes_del, &guider_ui.DeskTop_del,
        setup_scr_screen_notes, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);

}

/* GBK filename pool for SD list items (stored via user_data) */
#define SD_MAX_LIST_ENTRIES 128
static char g_sd_gbk_names[SD_MAX_LIST_ENTRIES][256];
static int g_sd_gbk_name_count = 0;

/* ---- SD card list item click callback ---- */
static void sd_list_item_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn;
    lv_obj_t *label;
    const char *name;
    void *ud;
    uint32_t is_parent;
    FRESULT res;
    DIR dir;
    FILINFO fno;
    uint32_t cur_len;
    char *slash;
    lv_obj_t *item;
    const char *icon;

    if (code != LV_EVENT_CLICKED) return;

    ud = lv_event_get_user_data(e);
    is_parent = (ud == (void *)1) ? 1 : 0;
    btn = lv_event_get_target(e);

    if (is_parent) {
        /* ".." parent directory */
        cur_len = strlen(g_sd_path);
        if (cur_len > 3) {
            g_sd_path[cur_len - 1] = (char)0;
            slash = strrchr(g_sd_path, (char)'/');
            if (slash) slash[1] = (char)0;
        }
    } else {
        /* Regular directory entry */
        name = (const char*)lv_obj_get_user_data(lv_event_get_current_target(e));
        if (!name || name[0] == (char)0) return;
            cur_len = strlen(g_sd_path);
        snprintf(g_sd_path + cur_len, sizeof(g_sd_path) - cur_len, "%s/", name);
    }
    /* Refresh list */
    lv_obj_clean(guider_ui.DeskTop_list_2);

    res = f_opendir(&dir, g_sd_path);
    if (res != FR_OK) {
        lv_label_set_text(guider_ui.DeskTop_label_21, "\u6253\u5f00\u76ee\u5f55\u5931\u8d25");
        return;
    }

    /* Add ".." parent entry (user_data=1 marks it) */
    item = lv_list_add_btn(guider_ui.DeskTop_list_2, LV_SYMBOL_UP, "..");
    lv_obj_set_style_text_font(item, &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lv_obj_get_child(item, 0), &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_event_cb(item, sd_list_item_handler, LV_EVENT_CLICKED, (void *)1);

    g_sd_gbk_name_count = 0;
    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
         if (fno.fname[0] == (char)'.' && fno.fname[1] == (char)0) continue;
         if (fno.fname[0] == '.') continue;
         if (fno.fattrib & (AM_HID | AM_SYS)) continue;
         {
             char utf8_lname[256];
             gbk_to_utf8(fno.fname, utf8_lname, sizeof(utf8_lname));
             icon = (fno.fattrib & AM_DIR) ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE;
             item = lv_list_add_btn(guider_ui.DeskTop_list_2, icon, utf8_lname);
             lv_obj_set_style_text_font(item, &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lv_obj_get_child(item, 0), &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
             /* Store original GBK name for file operations */
             if (g_sd_gbk_name_count < SD_MAX_LIST_ENTRIES) {
                 strncpy(g_sd_gbk_names[g_sd_gbk_name_count], fno.fname, 255);
                 g_sd_gbk_names[g_sd_gbk_name_count][255] = (char)0;
                 lv_obj_set_user_data(item, g_sd_gbk_names[g_sd_gbk_name_count]);
                 g_sd_gbk_name_count++;
             }
         }
        if (fno.fattrib & AM_DIR)
            lv_obj_add_event_cb(item, sd_list_item_handler, LV_EVENT_CLICKED, NULL);
        else
            lv_obj_add_event_cb(item, sd_file_open_handler, LV_EVENT_CLICKED, NULL);
    }
    f_closedir(&dir);

    lv_label_set_text(guider_ui.DeskTop_label_21, g_sd_path);
}

/* ---- btn_SD: mount SD and show root directory ---- */
static void DeskTop_btn_SD_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    file_manager_ui_request_readonly(guider_ui.DeskTop_cont_folder);
}
static void DeskTop_btn_5_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_add_flag(guider_ui.DeskTop_tileview_WiFi1, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_4_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_remove_flag(guider_ui.DeskTop_spinner_1, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_add_flag(guider_ui.DeskTop_cont_setting, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_11_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_add_flag(guider_ui.DeskTop_tileview_Time, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_Time_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        /* 从滚轮读取时间并写入RTC */
        char buf[8];
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_2, buf, sizeof(buf));
        uint8_t hour = (uint8_t)atoi(buf);
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_3, buf, sizeof(buf));
        uint8_t min  = (uint8_t)atoi(buf);
        rtc_set_time(hour, min, 0);

        lv_obj_add_flag(guider_ui.DeskTop_tileview_Time, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_13_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_add_flag(guider_ui.DeskTop_tileview_timekeeping, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}


static void DeskTop_btn_pause_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        if (g_countdown_state == 1) {
            g_countdown_state = 2;  /* pause */
            if (lv_obj_is_valid(guider_ui.DeskTop_btn_pause_label))
                lv_label_set_text(guider_ui.DeskTop_btn_pause_label, "\xe7\xbb\xa7\xe7\xbb\xad");
        } else if (g_countdown_state == 2) {
            g_countdown_state = 1;  /* resume */
            if (lv_obj_is_valid(guider_ui.DeskTop_btn_pause_label))
                lv_label_set_text(guider_ui.DeskTop_btn_pause_label, "\xe6\x9a\x82\xe5\x81\x9c");
        } else {
            /* state == 0: start countdown */
            g_countdown_state = 1;
            if (lv_obj_is_valid(guider_ui.DeskTop_btn_pause_label))
                lv_label_set_text(guider_ui.DeskTop_btn_pause_label, "\xe6\x9a\x82\xe5\x81\x9c");
        }
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_reset_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        g_countdown_state = 0;
        g_countdown_h = g_countdown_preset_h;
        g_countdown_m = g_countdown_preset_m;
        g_countdown_s = g_countdown_preset_s;
        /* clear lap records */
        if (lv_obj_is_valid(guider_ui.DeskTop_btn_pause_label))
            lv_label_set_text(guider_ui.DeskTop_btn_pause_label, "\xe6\x9a\x82\xe5\x81\x9c");
        g_lap_count = 0;
        if (lv_obj_is_valid(guider_ui.DeskTop_label_34))
            lv_label_set_text(guider_ui.DeskTop_label_34, "\xe8\xae\xb0\xe5\xbd\x95\xe7\x82\xb9\xef\xbc\x9a");
        break;
    }
    default:
        break;
    }
}
static void DeskTop_btn_record_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        if (g_lap_count < MAX_LAPS) {
            g_lap_h[g_lap_count] = g_countdown_h;
            g_lap_m[g_lap_count] = g_countdown_m;
            g_lap_s[g_lap_count] = g_countdown_s;
            g_lap_count++;
            /* update lap display */
            if (lv_obj_is_valid(guider_ui.DeskTop_label_34)) {
                static char lap_buf[4096];
                lap_buf[0] = '\0';
                for (int i = 0; i < g_lap_count; i++) {
                    char line[32];
                    snprintf(line, sizeof(line), "%2d - > %02d : %02d : %02d\n",
                             i + 1, g_lap_h[i], g_lap_m[i], g_lap_s[i]);
                    strncat(lap_buf, line, sizeof(lap_buf) - strlen(lap_buf) - 1);
                }
                lv_label_set_text(guider_ui.DeskTop_label_34, lap_buf);
                lv_obj_scroll_to_y(guider_ui.DeskTop_label_34_scroll, LV_COORD_MAX, LV_ANIM_ON);
            }
        }
        break;
    }
    default:
        break;
    }
}
static void DeskTop_btn_confirm_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        char buf[8];
        /* read rollers for hour/min/sec preset */
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_5, buf, sizeof(buf));
        g_countdown_preset_h = atoi(buf);
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_4, buf, sizeof(buf));
        g_countdown_preset_m = atoi(buf);
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_6, buf, sizeof(buf));
        g_countdown_preset_s = atoi(buf);
        /* set countdown to preset and start */
        g_countdown_h = g_countdown_preset_h;
        g_countdown_m = g_countdown_preset_m;
        g_countdown_s = g_countdown_preset_s;
        g_countdown_state = 1;
        if (lv_obj_is_valid(guider_ui.DeskTop_btn_pause_label))
            lv_label_set_text(guider_ui.DeskTop_btn_pause_label, "\xe6\x9a\x82\xe5\x81\x9c");
        g_lap_count = 0;
        /* update last preset label */
        if (lv_obj_is_valid(guider_ui.DeskTop_label_30))
            lv_label_set_text_fmt(guider_ui.DeskTop_label_30,
                "\xe4\xb8\x8a\xe6\xac\xa1\xe9\xa2\x84\xe8\xae\xbe\xe8\xae\xa1\xe6\x97\xb6\xef\xbc\x9a%02d:%02d:%02d",
                g_countdown_preset_h, g_countdown_preset_m, g_countdown_preset_s);
        /* clear lap display */
        if (lv_obj_is_valid(guider_ui.DeskTop_label_34))
            lv_label_set_text(guider_ui.DeskTop_label_34,
                "\xe8\xae\xb0\xe5\xbd\x95\xe7\x82\xb9\xef\xbc\x9a");
        break;
    }
    default:
        break;
    }

}
static void DeskTop_btn_15_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_add_flag(guider_ui.DeskTop_tileview_alarm, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_alarm_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        char buf[8];
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_alarm_h, buf, sizeof(buf));
        int hour = atoi(buf);
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_1, buf, sizeof(buf));
        int min = atoi(buf);
        alarm_add((uint8_t)hour, (uint8_t)min);
        /* Update label immediately */
        if (lv_obj_is_valid(guider_ui.DeskTop_label_23))
            lv_label_set_text_fmt(guider_ui.DeskTop_label_23,
                "闹钟：%02d:%02d", hour, min);
                if (lv_obj_is_valid(guider_ui.DeskTop_label_alarm))
            lv_label_set_text_fmt(guider_ui.DeskTop_label_alarm,
                "\xe5\xb7\xb2\xe8\xae\xbe\xe7\xbd\xae: %02d:%02d", hour, min);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_btn_alarm_del_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        alarm_remove_nearest();
        break;
    }
    default:
        break;
    }
}

static void DeskTop_cont_navigation_bar_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCROLL_END:
    {
        navigation_set_return_target(NAV_RETURN_DESKTOP);
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_navigation_bar, guider_ui.screen_navigation_bar_del, &guider_ui.DeskTop_del, setup_scr_screen_navigation_bar, LV_SCR_LOAD_ANIM_OVER_BOTTOM, 50, 50, true, true);
        break;
    }
    default:
        break;
    }
}

static void DeskTop_label_navigation_bar2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_RELEASED:
    {
        break;
    }
    case LV_EVENT_SCROLL_END:
    {
        break;
    }
    default:
        break;
    }
}

void events_init_DeskTop (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->DeskTop_img_noteAPP, DeskTop_img_noteAPP_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_settingAPP, DeskTop_img_settingAPP_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_wetherAPP, DeskTop_img_wetherAPP_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_edgeAPP, DeskTop_img_edgeAPP_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_calendarAPP, DeskTop_img_calendarAPP_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_albumAPP, DeskTop_img_albumAPP_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_yuanshenAPP, DeskTop_img_yuanshenAPP_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_alarmAPP, DeskTop_img_alarmAPP_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_calculatorAPP, DeskTop_img_calculatorAPP_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_folderAPP, DeskTop_img_folderAPP_event_handler, LV_EVENT_ALL, ui);
    /* MUSIC APP USER PATCH BEGIN: generated-style desktop entry binding */
    lv_obj_add_event_cb(ui->DeskTop_img_musicAPP, DeskTop_img_musicAPP_event_handler, LV_EVENT_ALL, ui);
    /* MUSIC APP USER PATCH END */
    /* STAGE6 UI USER PATCH BEGIN: new desktop entry bindings */
    lv_obj_add_event_cb(ui->DeskTop_img_draw,
        DeskTop_img_draw_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_piano,
        DeskTop_img_piano_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_nes,
        DeskTop_img_nes_event_handler, LV_EVENT_ALL, ui);
    /* STAGE6 UI USER PATCH END */
    lv_obj_add_event_cb(ui->DeskTop_img_phoneAPP, DeskTop_img_phoneAPP_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_messageAPP, DeskTop_img_messageAPP_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_img_cameraAPP, DeskTop_img_cameraAPP_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_custom, DeskTop_btn_custom_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_2, DeskTop_btn_2_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_Debug, DeskTop_btn_Debug_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_6, DeskTop_btn_6_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_7, DeskTop_btn_7_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_8, DeskTop_btn_8_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_SD, DeskTop_btn_SD_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_5, DeskTop_btn_5_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_4, DeskTop_btn_4_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_1, DeskTop_btn_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_11, DeskTop_btn_11_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_Time, DeskTop_btn_Time_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_13, DeskTop_btn_13_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_confirm, DeskTop_btn_confirm_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_pause, DeskTop_btn_pause_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_reset, DeskTop_btn_reset_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_record, DeskTop_btn_record_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_15, DeskTop_btn_15_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_alarm, DeskTop_btn_alarm_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_btn_alarm_del, DeskTop_btn_alarm_del_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_cont_navigation_bar, DeskTop_cont_navigation_bar_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->DeskTop_label_navigation_bar2, DeskTop_label_navigation_bar2_event_handler, LV_EVENT_ALL, ui);
}

static void screen_navigation_bar_cont_navigation_bar_return_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCROLL_END:
    {
        /* LOCK RETURN USER PATCH: swipe up to the recorded source page. */
        navigation_restore_saved_screen(&guider_ui.screen_navigation_bar_del,
            LV_SCR_LOAD_ANIM_OVER_TOP, 50U, 50U);
        break;
    }
    default:
        break;
    }
}

/* MUSIC APP USER PATCH BEGIN: independent music screen events */
static void screen_music_btn_back_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        music_player_ui_set_navigation_return(0);
        ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop,
            guider_ui.DeskTop_del, &guider_ui.screen_music_del,
            setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP,
            200, 200, true, true);
    }
}

static void screen_music_navigation_event_handler(lv_event_t *e)
{
    /* MUSIC APP USER PATCH: notes-template container scroll. */
    if (lv_event_get_code(e) != LV_EVENT_SCROLL_END) return;
    navigation_set_return_target(NAV_RETURN_MUSIC);
    music_player_ui_set_navigation_return(1);
    ui_load_scr_animation(&guider_ui,
        &guider_ui.screen_navigation_bar,
        guider_ui.screen_navigation_bar_del,
        &guider_ui.screen_music_del,
        setup_scr_screen_navigation_bar,
        LV_SCR_LOAD_ANIM_OVER_BOTTOM, 50, 50, true, true);
}

void events_init_screen_music(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_music_btn_back,
                        screen_music_btn_back_event_handler,
                        LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_music_cont_navigation_bar,
                        screen_music_navigation_event_handler,
                        LV_EVENT_ALL, ui);
}
/* MUSIC APP USER PATCH END */

static void screen_navigation_bar_btn_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_remove_flag(guider_ui.screen_navigation_bar_tileview_WiFi, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(guider_ui.screen_navigation_bar_spinner_1, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void screen_navigation_bar_btn_4_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_RELEASED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.lock_screen, guider_ui.lock_screen_del, &guider_ui.screen_navigation_bar_del, setup_scr_lock_screen, LV_SCR_LOAD_ANIM_NONE, 50, 50, true, true);
        break;
    }
    default:
        break;
    }
}

static void screen_navigation_bar_btn_17_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_add_flag(guider_ui.screen_navigation_bar_tileview_WiFi, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void screen_navigation_bar_btn_16_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_remove_flag(guider_ui.screen_navigation_bar_spinner_1, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void screen_navigation_bar_btn_15_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        char buf[8];
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_alarm_h, buf, sizeof(buf));
        int hour = atoi(buf);
        lv_roller_get_selected_str(guider_ui.DeskTop_roller_1, buf, sizeof(buf));
        int min = atoi(buf);
        alarm_add((uint8_t)hour, (uint8_t)min);
        /* Update label immediately */
        if (lv_obj_is_valid(guider_ui.DeskTop_label_23))
            lv_label_set_text_fmt(guider_ui.DeskTop_label_23,
                "闹钟：%02d:%02d", hour, min);
        lv_obj_add_flag(guider_ui.DeskTop_tileview_alarm, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}


/* ---- navigation bar toggle: shared handler for btn_3/5/7/8/9/10/14 ---- */
static void nav_btn_toggle_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint8_t mask = 0;

    lv_obj_t *btn = lv_event_get_target(e);
    if (code != LV_EVENT_CLICKED) return;
    if      (btn == guider_ui.screen_navigation_bar_btn_3)  mask = NAV_BTN_3;
    else if (btn == guider_ui.screen_navigation_bar_btn_5)  mask = NAV_BTN_5;
    else if (btn == guider_ui.screen_navigation_bar_btn_7)  mask = NAV_BTN_7;
    else if (btn == guider_ui.screen_navigation_bar_btn_8)  mask = NAV_BTN_8;
    else if (btn == guider_ui.screen_navigation_bar_btn_9)  mask = NAV_BTN_9;
    else if (btn == guider_ui.screen_navigation_bar_btn_10) mask = NAV_BTN_10;
    else if (btn == guider_ui.screen_navigation_bar_btn_14) mask = NAV_BTN_14;
    else return;

    if (g_nav_btn_state & mask) {
        /* turn off -> gray */
        g_nav_btn_state &= ~mask;
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xa9a9a9), LV_PART_MAIN | LV_STATE_DEFAULT);
        if (mask == NAV_BTN_14)
            lv_label_set_text(guider_ui.screen_navigation_bar_btn_14_label, " " LV_SYMBOL_PLAY " ");
    } else {
        /* turn on -> blue */
        g_nav_btn_state |= mask;
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x0098f2), LV_PART_MAIN | LV_STATE_DEFAULT);
        if (mask == NAV_BTN_14)
            lv_label_set_text(guider_ui.screen_navigation_bar_btn_14_label, " " LV_SYMBOL_PAUSE " ");
        if (mask == NAV_BTN_5)
            lv_slider_set_value(guider_ui.screen_navigation_bar_slider_2, 0, LV_ANIM_OFF);
    }
}

/* ---- slider_2: volume, interacts with btn_5 mute ---- */
static void screen_navigation_bar_slider_2_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        int32_t val = lv_slider_get_value(guider_ui.screen_navigation_bar_slider_2);
        audio_volume_slider_changed(val);
        if (val > 0) {
            g_nav_btn_state &= ~NAV_BTN_5;
            lv_obj_set_style_bg_color(guider_ui.screen_navigation_bar_btn_5, lv_color_hex(0xa9a9a9), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            g_nav_btn_state |= NAV_BTN_5;
            lv_obj_set_style_bg_color(guider_ui.screen_navigation_bar_btn_5, lv_color_hex(0x0098f2), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}
void events_init_screen_navigation_bar (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_navigation_bar_cont_navigation_bar_return, screen_navigation_bar_cont_navigation_bar_return_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_navigation_bar_btn_2, screen_navigation_bar_btn_2_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_navigation_bar_btn_4, screen_navigation_bar_btn_4_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_navigation_bar_btn_17, screen_navigation_bar_btn_17_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_navigation_bar_btn_16, screen_navigation_bar_btn_16_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_navigation_bar_btn_15, screen_navigation_bar_btn_15_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_navigation_bar_slider_1, Menu_slider_LCD_BL_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui->screen_navigation_bar_btn_3, nav_btn_toggle_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_navigation_bar_btn_5, nav_btn_toggle_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_navigation_bar_btn_7, nav_btn_toggle_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_navigation_bar_btn_8, nav_btn_toggle_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_navigation_bar_btn_9, nav_btn_toggle_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_navigation_bar_btn_10, nav_btn_toggle_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_navigation_bar_btn_14, nav_btn_toggle_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui->screen_navigation_bar_slider_2, screen_navigation_bar_slider_2_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
}

/* STAGE6 NOTES USER PATCH BEGIN: unified full-document lifecycle. */
static void notes_show_error(const char *message)
{
    if (guider_ui.screen_notes_label_3 != NULL)
        lv_label_set_text(guider_ui.screen_notes_label_3, message);
}

static uint8_t notes_has_unsaved_changes(void)
{
    notes_storage_status_t status;
    notes_storage_get_status(&status);
    return status.opened != 0U && status.modified != 0U;
}

static void notes_update_path_label(const char *path)
{
    char utf8_path[256];

    if (path == NULL || path[0] == '\0') {
        lv_label_set_text(guider_ui.screen_notes_label_3, "新建笔记");
    } else {
        gbk_to_utf8(path, utf8_path, sizeof(utf8_path));
        lv_label_set_text(guider_ui.screen_notes_label_3, utf8_path);
    }
    lv_obj_set_style_text_font(guider_ui.screen_notes_label_3, &lv_font_myyh_16,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
}

static uint8_t notes_open_path(const char *path)
{
    int result;
    notes_editor_ui_discard();
    result = notes_editor_ui_open(path);
    if (result != NOTES_STORAGE_OK) {
        (void)notes_editor_ui_new();
        notes_show_error("打开失败");
        return 0U;
    }
    strncpy(g_opened_file_path, path, sizeof(g_opened_file_path) - 1U);
    g_opened_file_path[sizeof(g_opened_file_path) - 1U] = '\0';
    notes_update_path_label(path);
    return 1U;
}

static uint8_t notes_save_path(const char *path)
{
    if (notes_editor_ui_save(path) != NOTES_STORAGE_OK) {
        notes_show_error("保存失败");
        return 0U;
    }
    strncpy(g_opened_file_path, path, sizeof(g_opened_file_path) - 1U);
    g_opened_file_path[sizeof(g_opened_file_path) - 1U] = '\0';
    notes_update_path_label(path);
    return 1U;
}

static uint8_t notes_detach_for_exit(uint8_t discard)
{
    if (discard != 0U) notes_editor_ui_discard();
    if (notes_editor_ui_detach() == NOTES_STORAGE_OK) {
        return 1U;
    } else {
        notes_show_error("正文同步失败");
        return 0U;
    }
}
/* STAGE6 NOTES USER PATCH END */

static void screen_notes_cont_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCROLL_END:
    {
        navigation_set_return_target(NAV_RETURN_NOTES);
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_navigation_bar, guider_ui.screen_navigation_bar_del, &guider_ui.screen_notes_del, setup_scr_screen_navigation_bar, LV_SCR_LOAD_ANIM_OVER_BOTTOM, 50, 50, true, true);
        break;
    }
    default:
        break;
    }
}

/* ---- screen_notes: Return/Exit (btn_return) ---- */
static void screen_notes_btn_return_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        /* Exit without saving */
        if (notes_detach_for_exit(1U))
            ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop, guider_ui.DeskTop_del, &guider_ui.screen_notes_del, setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
        break;
    }
    default:
        break;
    }
}

/* ---- screen_notes: Edit toggle (btn_2) ---- */
static int g_notes_editable = 1;  /* 1=editable, 0=readonly */
static void screen_notes_btn_2_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        g_notes_editable = !g_notes_editable;
        if (g_notes_editable) {
            /* Editable: light green, re-enable keyboard */
            lv_obj_set_style_bg_color(guider_ui.screen_notes_btn_2, lv_color_hex(0x90EE90), LV_PART_MAIN|LV_STATE_DEFAULT);

            lv_obj_add_event_cb(guider_ui.screen_notes_ta_note, ta_event_cb, LV_EVENT_ALL, guider_ui.g_kb_top_layer);
            lv_label_set_text(guider_ui.screen_notes_btn_2_label, "编辑");
        } else {
            /* Read-only: light red, remove keyboard callback */
            lv_obj_set_style_bg_color(guider_ui.screen_notes_btn_2, lv_color_hex(0xFFB6C1), LV_PART_MAIN|LV_STATE_DEFAULT);
            lv_obj_remove_event_cb(guider_ui.screen_notes_ta_note, ta_event_cb);
            lv_label_set_text(guider_ui.screen_notes_btn_2_label, "只读");
        }
        break;
    }
    default:
        break;
    }
}

/* ---- screen_notes: Save & Exit (list item or tileview save btn_3) ---- */
static void screen_notes_save_exit_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        /* Show save card (tileview), pre-fill if opening existing file */
        if (g_opened_file_path[0]) {
            /* Split full path into dir + filename */
            char dir_only[256];
            const char *p, *name = g_opened_file_path;
            for (p = g_opened_file_path; *p; p++) {
                if (*p == (char)0x2F) name = p + 1;
            }
            int dir_len = (int)(name - g_opened_file_path);
            if (dir_len > 0 && g_opened_file_path[dir_len-1] == (char)0x2F) dir_len--;
            if (dir_len <= 0) {
                strcpy(dir_only, "0:/");
            } else {
                memcpy(dir_only, g_opened_file_path, dir_len);
                dir_only[dir_len] = 0;
            }
            /* Strip extension so save handler can append .md */
            {
                const char *dot = name;
                const char *s;
                for (s = name; *s; s++) {
                    if (*s == (char)0x2E) dot = s;
                }
                if (dot != name) {
                    char name_noext[128];
                    int nlen = (int)(dot - name);
                    if (nlen < (int)sizeof(name_noext)) {
                        memcpy(name_noext, name, nlen);
                        name_noext[nlen] = 0;
                        lv_textarea_set_text(guider_ui.screen_notes_ta_1, name_noext);
                    }
                } else {
                    lv_textarea_set_text(guider_ui.screen_notes_ta_1, name);
                }
            }
            lv_textarea_set_text(guider_ui.screen_notes_ta_2, dir_only);
        } else {
            lv_textarea_set_text(guider_ui.screen_notes_ta_1, "Note1");
            lv_textarea_set_text(guider_ui.screen_notes_ta_2, "0:/Note");
        }
        lv_obj_remove_flag(guider_ui.screen_notes_tileview_note, LV_OBJ_FLAG_HIDDEN);

        break;
    }
    default:
        break;
    }
}

/* ---- Save card: execute save (btn_3) ---- */
static char g_save_path_pending[300];
static lv_obj_t *g_overwrite_dlg = NULL;

static void overwrite_confirm_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (g_save_path_pending[0]) {
        if (!notes_save_path(g_save_path_pending) ||
            !notes_detach_for_exit(0U)) return;
    }
    if (g_overwrite_dlg) { lv_obj_del(g_overwrite_dlg); g_overwrite_dlg = NULL; }
    lv_obj_add_flag(guider_ui.screen_notes_tileview_note, LV_OBJ_FLAG_HIDDEN);
    ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop, guider_ui.DeskTop_del,
        &guider_ui.screen_notes_del, setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
}

static void overwrite_cancel_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (g_overwrite_dlg) { lv_obj_del(g_overwrite_dlg); g_overwrite_dlg = NULL; }
    /* Return to save card (tileview stays visible) */
}

static void screen_notes_card_save_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    FILINFO fno;
    const char *fname, *fpath;
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        fname = lv_textarea_get_text(guider_ui.screen_notes_ta_1);
        fpath = lv_textarea_get_text(guider_ui.screen_notes_ta_2);
        if (fpath && fpath[0] && fname && fname[0]) {
            char save_path[300];
            int plen = strlen(fpath);
            if (plen > 0 && fpath[plen-1] == 0x2F)
                snprintf(save_path, sizeof(save_path), "%s%s.md", fpath, fname);
            else
                snprintf(save_path, sizeof(save_path), "%s/%s.md", fpath, fname);
            /* Check if file already exists */
            if (f_stat(save_path, &fno) == FR_OK) {
                /* File exists: show overwrite confirmation */
                strcpy(g_save_path_pending, save_path);

                /* Create confirmation dialog */
                g_overwrite_dlg = lv_obj_create(guider_ui.screen_notes);
                lv_obj_set_size(g_overwrite_dlg, 340, 160);
                lv_obj_align(g_overwrite_dlg, LV_ALIGN_CENTER, 0, 0);
                lv_obj_set_style_bg_color(g_overwrite_dlg, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(g_overwrite_dlg, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(g_overwrite_dlg, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_radius(g_overwrite_dlg, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_shadow_width(g_overwrite_dlg, 20, LV_PART_MAIN|LV_STATE_DEFAULT);

                /* Title label */
                lv_obj_t *dlg_label = lv_label_create(g_overwrite_dlg);
                lv_label_set_text(dlg_label, "\u6587\u4ef6\u5df2\u5b58\u5728\uff0c\u662f\u5426\u8986\u76d6\uff1f");
                lv_obj_set_style_text_font(dlg_label, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(dlg_label, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_align(dlg_label, LV_ALIGN_TOP_MID, 0, 15);

                /* Filename label */
                lv_obj_t *dlg_fname = lv_label_create(g_overwrite_dlg);
                lv_label_set_text(dlg_fname, g_save_path_pending);
                lv_label_set_long_mode(dlg_fname, LV_LABEL_LONG_SCROLL_CIRCULAR);
                lv_obj_set_size(dlg_fname, 300, 24);
                lv_obj_set_style_text_font(dlg_fname, &lv_font_myyh_14, LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(dlg_fname, lv_color_hex(0x666666), LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_align(dlg_fname, LV_ALIGN_TOP_MID, 0, 48);

                /* Overwrite button */
                lv_obj_t *btn_ow = lv_button_create(g_overwrite_dlg);
                lv_obj_set_size(btn_ow, 100, 36);
                lv_obj_align(btn_ow, LV_ALIGN_BOTTOM_LEFT, 50, -20);
                lv_obj_set_style_bg_color(btn_ow, lv_color_hex(0xFF6B6B), LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_radius(btn_ow, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_t *btn_ow_label = lv_label_create(btn_ow);
                lv_label_set_text(btn_ow_label, "\u8986\u76d6");
                lv_obj_set_style_text_font(btn_ow_label, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_center(btn_ow_label);
                lv_obj_add_event_cb(btn_ow, overwrite_confirm_handler, LV_EVENT_CLICKED, NULL);

                /* Cancel button */
                lv_obj_t *btn_cancel = lv_button_create(g_overwrite_dlg);
                lv_obj_set_size(btn_cancel, 100, 36);
                lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_RIGHT, -50, -20);
                lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0xCCCCCC), LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_set_style_radius(btn_cancel, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_t *btn_cancel_label = lv_label_create(btn_cancel);
                lv_label_set_text(btn_cancel_label, "\u53d6\u6d88");
                lv_obj_set_style_text_font(btn_cancel_label, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
                lv_obj_center(btn_cancel_label);
                lv_obj_add_event_cb(btn_cancel, overwrite_cancel_handler, LV_EVENT_CLICKED, NULL);

                return;  /* Wait for user choice */
            }
            /* File does not exist: save directly */
            if (!notes_save_path(save_path) ||
                !notes_detach_for_exit(0U)) return;
        }
        /* Hide card and go back */
        lv_obj_add_flag(guider_ui.screen_notes_tileview_note, LV_OBJ_FLAG_HIDDEN);
        ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop, guider_ui.DeskTop_del, &guider_ui.screen_notes_del, setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
        break;
    }
    default:
        break;
    }
}
/* ---- Save card: close (btn_4 X) ---- */
static void screen_notes_card_close_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_add_flag(guider_ui.screen_notes_tileview_note, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ---- Save card: exit without saving (btn_5) ---- */
static void screen_notes_card_nosave_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_add_flag(guider_ui.screen_notes_tileview_note, LV_OBJ_FLAG_HIDDEN);
        if (notes_detach_for_exit(1U))
            ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop, guider_ui.DeskTop_del, &guider_ui.screen_notes_del, setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
    }
}


/* ---- Notes file list: scan 0:/Note for .md files ---- */
static char g_notes_file_names[32][128];
static int g_notes_file_count = 0;

static lv_obj_t *g_notes_unsaved_dlg = NULL;
static char g_notes_pending_open[300];

/* Confirmation: discard unsaved changes */
static void notes_discard_confirm_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (g_notes_unsaved_dlg) { lv_obj_del(g_notes_unsaved_dlg); g_notes_unsaved_dlg = NULL; }
    if (g_notes_pending_open[0]) {
        notes_editor_ui_discard();
        (void)notes_open_path(g_notes_pending_open);
        g_notes_pending_open[0] = '\0';
    }
}

static void notes_discard_cancel_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (g_notes_unsaved_dlg) { lv_obj_del(g_notes_unsaved_dlg); g_notes_unsaved_dlg = NULL; }
    g_notes_pending_open[0] = 0;
}

/* Show unsaved changes warning dialog */
static void notes_show_unsaved_dialog(const char *file_path)
{
    if (file_path == NULL) return;
    strncpy(g_notes_pending_open, file_path, sizeof(g_notes_pending_open) - 1U);
    g_notes_pending_open[sizeof(g_notes_pending_open) - 1U] = '\0';
    g_notes_unsaved_dlg = lv_obj_create(guider_ui.screen_notes);
    lv_obj_set_size(g_notes_unsaved_dlg, 340, 150);
    lv_obj_align(g_notes_unsaved_dlg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(g_notes_unsaved_dlg, lv_color_hex(0xFFFFFF), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(g_notes_unsaved_dlg, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(g_notes_unsaved_dlg, lv_color_hex(0xE74C3C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(g_notes_unsaved_dlg, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(g_notes_unsaved_dlg, 20, LV_PART_MAIN|LV_STATE_DEFAULT);

    lv_obj_t *dl = lv_label_create(g_notes_unsaved_dlg);
    lv_label_set_text(dl, "\u5f53\u524d\u5185\u5bb9\u672a\u4fdd\u5b58\uff0c\u662f\u5426\u653e\u5f03\uff1f");
    lv_obj_set_style_text_font(dl, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(dl, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_align(dl, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *b1 = lv_button_create(g_notes_unsaved_dlg);
    lv_obj_set_size(b1, 100, 36);
    lv_obj_align(b1, LV_ALIGN_BOTTOM_LEFT, 50, -20);
    lv_obj_set_style_bg_color(b1, lv_color_hex(0xFF6B6B), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(b1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_t *bl1 = lv_label_create(b1);
    lv_label_set_text(bl1, "\u653e\u5f03");
    lv_obj_set_style_text_font(bl1, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_center(bl1);
    lv_obj_add_event_cb(b1, notes_discard_confirm_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *b2 = lv_button_create(g_notes_unsaved_dlg);
    lv_obj_set_size(b2, 100, 36);
    lv_obj_align(b2, LV_ALIGN_BOTTOM_RIGHT, -50, -20);
    lv_obj_set_style_bg_color(b2, lv_color_hex(0xCCCCCC), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(b2, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_t *bl2 = lv_label_create(b2);
    lv_label_set_text(bl2, "\u53d6\u6d88");
    lv_obj_set_style_text_font(bl2, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_center(bl2);
    lv_obj_add_event_cb(b2, notes_discard_cancel_handler, LV_EVENT_CLICKED, NULL);
}

/* Click handler for notes file list items */
static void notes_list_item_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    const char *fname = (const char *)lv_obj_get_user_data(lv_event_get_current_target(e));
    if (!fname) return;

    char full_path[300];
    snprintf(full_path, sizeof(full_path), "0:/Note/%s.md", fname);

    if (notes_has_unsaved_changes()) {
        notes_show_unsaved_dialog(full_path);
        return;
    }
    (void)notes_open_path(full_path);
}

/* Populate the notes file list from 0:/Note */
static void notes_populate_file_list(void)
{
    DIR dir;
    FILINFO fno;
    int i;

    /* Remove old dynamic list items (keep item0 = save button) */
    for (i = 1; i < g_notes_file_count; i++) {
        /* Items will be re-created, LVGL list manages children */
    }
    /* Clear the list by deleting all children except the first (save btn) */
    {
        lv_obj_t *child;
        int first = 1;
        while ((child = lv_obj_get_child(guider_ui.screen_notes_list_note, 0)) != NULL) {
            if (first && child == guider_ui.screen_notes_list_note_item0) {
                first = 0;
                /* skip: move to end to keep it, but lv_obj_get_child always gets first... */
                /* Instead just delete all and re-add item0 later */
            }
            lv_obj_del(child);
        }
    }
    /* Re-add save button */
    guider_ui.screen_notes_list_note_item0 = lv_list_add_button(guider_ui.screen_notes_list_note, LV_SYMBOL_SAVE, "\u4fdd\u5b58\u5e76\u9000\u51fa");
    lv_obj_add_event_cb(guider_ui.screen_notes_list_note_item0, screen_notes_save_exit_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_style_text_font(guider_ui.screen_notes_list_note_item0, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);

    g_notes_file_count = 0;

    if (f_opendir(&dir, "0:/Note") != FR_OK) return;

    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
        if (fno.fattrib & AM_DIR) continue;
        uint32_t nlen = strlen(fno.fname);
        /* Match .md files */
        if (nlen < 3) continue;
        if (fno.fname[nlen-3] != 0x2E || fno.fname[nlen-2] != 0x6D || fno.fname[nlen-1] != 0x64) continue;
        if (g_notes_file_count >= 32) break;

        /* Strip .md extension for display */
        char display_name[128];
        uint32_t dnlen = nlen - 3;
        if (dnlen >= sizeof(display_name)) dnlen = sizeof(display_name) - 1;
        memcpy(display_name, fno.fname, dnlen);
        display_name[dnlen] = 0;

        /* Store base name for click handler */
        strcpy(g_notes_file_names[g_notes_file_count], display_name);

        /* Convert to UTF-8 for display */
        char utf8_display[256];
        gbk_to_utf8(display_name, utf8_display, sizeof(utf8_display));

        lv_obj_t *item = lv_list_add_button(guider_ui.screen_notes_list_note, LV_SYMBOL_FILE, utf8_display);
        lv_obj_set_user_data(item, (void *)g_notes_file_names[g_notes_file_count]);
        lv_obj_add_event_cb(item, notes_list_item_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_text_font(item, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);

        g_notes_file_count++;
    }
    f_closedir(&dir);
}

void events_init_screen_notes(lv_ui *ui)
{
    notes_storage_status_t status;

    notes_storage_get_status(&status);
    if (status.opened == 0U) {
        if (g_opened_file_path[0] != '\0') {
            (void)notes_open_path(g_opened_file_path);
        } else if (notes_editor_ui_new() == NOTES_STORAGE_OK) {
            notes_update_path_label(NULL);
        } else {
            notes_show_error("新建失败");
        }
    }

    /* Navigation bar (unchanged) */
    lv_obj_add_event_cb(ui->screen_notes_cont_2, screen_notes_cont_2_event_handler, LV_EVENT_ALL, ui);
    /* Save&Exit button (btn_1) - shows save card */
    lv_obj_add_event_cb(ui->screen_notes_btn_1, screen_notes_save_exit_handler, LV_EVENT_ALL, ui);
    /* Return/Exit button (btn_return) - exit without saving */
    lv_obj_add_event_cb(ui->screen_notes_btn_return, screen_notes_btn_return_event_handler, LV_EVENT_ALL, ui);
    /* Keyboard toggle button */
    lv_obj_add_event_cb(ui->screen_notes_btn_2, screen_notes_btn_2_event_handler, LV_EVENT_ALL, ui);
    /* Save list item (alternative save&exit trigger) */
    lv_obj_add_event_cb(ui->screen_notes_list_note_item0, screen_notes_save_exit_handler, LV_EVENT_ALL, ui);
    /* Save card buttons */
    lv_obj_add_event_cb(ui->screen_notes_btn_3, screen_notes_card_save_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_notes_btn_4, screen_notes_card_close_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_notes_btn_5, screen_notes_card_nosave_handler, LV_EVENT_ALL, ui);
    /* Hide save card initially */
    /* Populate file list from 0:/Note */
    notes_populate_file_list();
    lv_obj_add_flag(ui->screen_notes_tileview_note, LV_OBJ_FLAG_HIDDEN);
}
static void screen_Signal_Proc_btn_return_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop, guider_ui.DeskTop_del, &guider_ui.screen_Signal_Proc_del, setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
        break;
    }
    default:
        break;
    }
}

static void screen_Signal_Proc_btn_printf_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        // 设置强制滚动标志
        g_force_scroll_bottom = 1;
        // 设置标志，在主循环中处理
        g_printf = 1;
        break;
    }
    default:
        break;
    }
}

static void screen_Signal_Proc_btn_clear_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        // 清空显示
        lv_textarea_set_text(guider_ui.Serial_Port_ta_display, "");
        
        // 同步清空缓冲区
        extern char uart_display_buf[];
        extern uint16_t uart_display_len;
        extern uint16_t uart_last_display_len;
        
        memset(uart_display_buf, 0, UART_DISPLAY_BUF_SIZE);
        uart_display_len = 0;
        uart_last_display_len = 0;  // 重置增量指针
        break;
    }
    default:
        break;
    }
}

static void screen_Signal_Proc_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        const char *text = lv_textarea_get_text(guider_ui.screen_Signal_Proc_ta_1);
        if(text == NULL || strlen(text) == 0) {
            break;
        }
        /* 该事件运行在LVGL主循环上下文，可直接提交完整命令行。 */
        (void)command_parser_execute_line(text);
        // 通过串口发送数据
        HAL_UART_Transmit(&huart1, (uint8_t *)text, strlen(text), 100);
        // 更新显示缓冲区
        int written = snprintf(uart_display_buf + uart_display_len, 
                              UART_DISPLAY_BUF_SIZE - uart_display_len, 
                              "\n[TX] %s", text);
        if(written > 0) {
            uart_display_len += written;
        }
        // 清空输入框
        lv_textarea_set_text(guider_ui.screen_Signal_Proc_ta_1, "");
        break;
    }
    default:
        break;
    }
}

static void screen_Signal_Proc_cont_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCROLL_END:
    {
        navigation_set_return_target(NAV_RETURN_SIGNAL_PROC);
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_navigation_bar, guider_ui.screen_navigation_bar_del, &guider_ui.screen_Signal_Proc_del, setup_scr_screen_navigation_bar, LV_SCR_LOAD_ANIM_OVER_BOTTOM, 50, 50, true, true);
        break;
    }
    default:
        break;
    }
}

void events_init_screen_Signal_Proc (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_Signal_Proc_btn_return, screen_Signal_Proc_btn_return_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_Signal_Proc_btn_printf, screen_Signal_Proc_btn_printf_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_Signal_Proc_btn_clear, screen_Signal_Proc_btn_clear_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_Signal_Proc_btn_1, screen_Signal_Proc_btn_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_Signal_Proc_cont_2, screen_Signal_Proc_cont_2_event_handler, LV_EVENT_ALL, ui);
}

static void screen_yuanshen_cont_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_remove_flag(guider_ui.screen_yuanshen_img_1, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void screen_yuanshen_img_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_remove_flag(guider_ui.screen_yuanshen_img_2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(guider_ui.screen_yuanshen_img_1, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void screen_yuanshen_img_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_remove_flag(guider_ui.screen_yuanshen_img_3, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(guider_ui.screen_yuanshen_img_2, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

static void screen_yuanshen_img_3_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_add_flag(guider_ui.screen_yuanshen_img_3, LV_OBJ_FLAG_HIDDEN);
        ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop, guider_ui.DeskTop_del, &guider_ui.screen_yuanshen_del, setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
        break;
    }
    default:
        break;
    }
}

static void screen_yuanshen_cont_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_SCROLL_END:
    {
        navigation_set_return_target(NAV_RETURN_YUANSHEN);
        ui_load_scr_animation(&guider_ui, &guider_ui.screen_navigation_bar, guider_ui.screen_navigation_bar_del, &guider_ui.screen_yuanshen_del, setup_scr_screen_navigation_bar, LV_SCR_LOAD_ANIM_OVER_BOTTOM, 200, 200, true, true);
        break;
    }
    default:
        break;
    }
}

void events_init_screen_yuanshen (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_yuanshen_cont_1, screen_yuanshen_cont_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_yuanshen_img_1, screen_yuanshen_img_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_yuanshen_img_2, screen_yuanshen_img_2_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_yuanshen_img_3, screen_yuanshen_img_3_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_yuanshen_cont_2, screen_yuanshen_cont_2_event_handler, LV_EVENT_ALL, ui);
}

static void screen_camera_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        camera_ui_bridge_request_leave();
        ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop, guider_ui.DeskTop_del, &guider_ui.screen_camera_del, setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
        break;
    }
    default:
        break;
    }
}

void events_init_screen_camera (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_camera_btn_1, screen_camera_btn_1_event_handler, LV_EVENT_ALL, ui);
}


void events_init(lv_ui *ui)
{

}


/* ==================== GBK to UTF-8 Converter ==================== */
/* Converts GBK (CP936) filename to UTF-8 for LVGL display.
 * Returns number of UTF-8 bytes written (not including null). */
/* GBK → UTF-8 using FatFS built-in cc936 conversion table.
/* GBK → UTF-8: simple strncpy pass-through.
 * With FF_LFN_UNICODE=0, filenames from FatFS are in GBK encoding.
 * Passed as-is to LVGL which may show garbled Chinese but is stable. */
static uint32_t gbk_to_utf8(const char *gbk, char *utf8, uint32_t utf8_max)
{
    uint32_t i;
    for (i = 0; i + 1 < utf8_max && gbk[i]; i++)
        utf8[i] = gbk[i];
    utf8[i] = (char)0;
    return i;
}
/* ==================== Image Viewer ==================== */

#define ALBUM_DIR          "0:/DCIM/"
#define ALBUM_MAX_FILES    64
#define ALBUM_PATH_LEN     256

lv_image_dsc_t g_img_dsc;
static char g_album_paths[ALBUM_MAX_FILES][ALBUM_PATH_LEN];
static int  g_album_count = 0;
int  g_album_index = 0;
static uint8_t g_album_mode = 0;  /* 1=album swipe mode, 0=single view */
uint32_t g_img_zoom = 256;        /* current image zoom (256=100%) */
uint32_t g_img_fit_zoom = 256;    /* zoom level for fit-to-screen */
uint32_t g_img_saved_zoom = 256;  /* last pinch zoom, committed on release */
int32_t  g_pan_x = 0;             /* pan offset X (pixels) */
int32_t  g_pan_y = 0;             /* pan offset Y (pixels) */

/* Forward */
void image_viewer_load_file(const char *path);

void image_viewer_reposition(void);  /* forward */

/* Decode JPEG buffer and display (shared by album and single-view) */
static uint8_t image_viewer_decode_jpeg(uint8_t *buf, uint32_t size)
{
    uint32_t w, h, aligned_w, aligned_h, orig_w, orig_h, i;
    uint8_t res;

    w = 0; h = 0;
    for (i = 0; i < size - 9; i++) {
        if (buf[i] == 0xFF) {
            uint8_t marker = buf[i+1];
            if (marker == 0xC0 || marker == 0xC1 || marker == 0xC2) {
                /* SOF0=baseline, SOF1=extended, SOF2=progressive */
                if (marker == 0xC2) {
                    /* Progressive JPEG not supported by hardware */
                    return 4;
                }
                h = (buf[i+5] << 8) | buf[i+6];
                w = (buf[i+7] << 8) | buf[i+8];
                orig_w = w; orig_h = h;
                /* Round up to MCU boundary (16) for H743 HW JPEG decoder */
                aligned_w = ((w + 15) / 16) * 16;
                aligned_h = ((h + 15) / 16) * 16;
                if (aligned_w != w || aligned_h != h) {
                    buf[i+5] = (aligned_h >> 8) & 0xFF;
                    buf[i+6] = aligned_h & 0xFF;
                    buf[i+7] = (aligned_w >> 8) & 0xFF;
                    buf[i+8] = aligned_w & 0xFF;
                    w = aligned_w; h = aligned_h;
                }
                /* Flush entire buffer after SOF patch so MDMA sees aligned dims */
                SCB_CleanInvalidateDCache_by_Addr((uint32_t *)(((uint32_t)buf) & ~0x1FUL), ((size + 31) & ~0x1FUL));
                break;
            }
        }
    }
    if (w == 0 || h == 0 || w > 2048 || h > 2048) return 1;

    res = mjpeg_init(0, 0, w, h);
    if (res) return 2;

    res = mjpeg_decode(buf, size);
    if (res || mjpeg.state != JPEG_STATE_FINISHED) {
        mjpeg_free();
        return 3;
    }

    g_img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    g_img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    g_img_dsc.header.w = (orig_w > 0) ? orig_w : w;
    g_img_dsc.header.h = (orig_h > 0) ? orig_h : h;
    g_img_dsc.data_size = w * h * 2;
    g_img_dsc.data = (uint8_t *)0xC1D00000UL;
    g_img_dsc.header.stride = w * 2;

    lv_image_set_src(guider_ui.screen_image_viewer_img, &g_img_dsc);

    /* Auto-scale to fit within 1024x600 screen */
    {
        uint32_t disp_w = 1024;
        uint32_t disp_h = 600;
        uint32_t scale_x = (disp_w * 256) / g_img_dsc.header.w;
        uint32_t scale_y = (disp_h * 256) / g_img_dsc.header.h;
        uint32_t fit_zoom = (scale_x < scale_y) ? scale_x : scale_y;
        g_img_zoom = (fit_zoom > 256) ? 256 : fit_zoom;
        g_img_fit_zoom = fit_zoom;
        g_pan_x = 0; g_pan_y = 0;
    }
    image_viewer_reposition();
    mjpeg_free();
    return 0;
}

/* Load and display a JPEG file by path */
void image_viewer_load_file(const char *path)
{
    FIL fil;
    FRESULT fr;
    UINT br;
    uint32_t total;
    char buf[64];

    fr = f_open(&fil, path, FA_READ);
    if (fr != FR_OK) {
        snprintf(buf, sizeof(buf), "\u6253\u5f00\u5931\u8d25(%d)", (int)fr);
        lv_label_set_text(guider_ui.screen_image_viewer_label_2, buf);
        return;
    }
    total = f_size(&fil);
    if (total > 65535) total = 65535;

    fr = f_read(&fil, g_file_content, (UINT)total, &br);
    f_close(&fil);
    if (fr != FR_OK || br == 0) {
        lv_label_set_text(guider_ui.screen_image_viewer_label_2, "\u8bfb\u53d6\u5931\u8d25");
        return;
    }

    /* Flush DCache so JPEG HW decoder (DMA) sees the fresh data */
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)((uint32_t)g_file_content & ~0x1FUL), (br + 31) & ~0x1F);

    {
        uint8_t res;
        res = image_viewer_decode_jpeg(g_file_content, br);
        if (res == 0) {
            const char *name = path;
            const char *p;
            for (p = path; *p; p++) { if (*p == '/') name = p + 1; }
            {
                char utf8_name[256];
                gbk_to_utf8(name, utf8_name, sizeof(utf8_name));
                snprintf(buf, sizeof(buf), "%s (%luB)", utf8_name, (unsigned long)br);
            }
            lv_label_set_text(guider_ui.screen_image_viewer_label_2, buf);
        } else if (res == 1) {
            snprintf(buf, sizeof(buf), "\u5c3a\u5bf8\u9519\u8bef (%luB)", (unsigned long)br);
            lv_label_set_text(guider_ui.screen_image_viewer_label_2, buf);
        } else if (res == 4) {
            lv_label_set_text(guider_ui.screen_image_viewer_label_2, "\u4e0d\u652f\u6301\u6e10\u8fdbJPEG");
        } else if (res == 2) {
            lv_label_set_text(guider_ui.screen_image_viewer_label_2, "\u521d\u59cb\u5316\u5931\u8d25");
        } else {
            lv_label_set_text(guider_ui.screen_image_viewer_label_2, "\u89e3\u7801\u5931\u8d25");
        }
    }
}

/* Scan album directory and build file list */
uint8_t image_viewer_scan_album(void)
{
    DIR dir;
    FILINFO fno;
    FRESULT fr;
    char full[ALBUM_PATH_LEN];

    g_album_count = 0;
    g_album_index = 0;

    fr = f_opendir(&dir, ALBUM_DIR);
    if (fr != FR_OK) return 0;

    while (g_album_count < ALBUM_MAX_FILES) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) break;
        if (fno.fattrib & AM_DIR) continue;

        /* Check .jpg extension */
        {
            uint32_t nl = strlen(fno.fname);
            if (nl < 4) continue;
            if (!((fno.fname[nl-4] == '.' || fno.fname[nl-4] == '.') &&
                  (fno.fname[nl-3] == 'j' || fno.fname[nl-3] == 'J') &&
                  (fno.fname[nl-2] == 'p' || fno.fname[nl-2] == 'P') &&
                  (fno.fname[nl-1] == 'g' || fno.fname[nl-1] == 'G'))) {
                if (!(nl >= 5 &&
                      (fno.fname[nl-5] == '.' || fno.fname[nl-5] == '.') &&
                      (fno.fname[nl-4] == 'j' || fno.fname[nl-4] == 'J') &&
                      (fno.fname[nl-3] == 'p' || fno.fname[nl-3] == 'P') &&
                      (fno.fname[nl-2] == 'e' || fno.fname[nl-2] == 'E') &&
                      (fno.fname[nl-1] == 'g' || fno.fname[nl-1] == 'G')))
                    continue;
            }
        }

        snprintf(full, sizeof(full), "%s%s", ALBUM_DIR, fno.fname);
        strncpy(g_album_paths[g_album_count], full, ALBUM_PATH_LEN - 1);
        g_album_paths[g_album_count][ALBUM_PATH_LEN - 1] = (char)0;
        g_album_count++;
    }
    f_closedir(&dir);
    return (g_album_count > 0) ? 1 : 0;
}

/* Swipe handler for album mode */
static uint8_t  g_swipe_active = 0;
static uint32_t g_last_tap_ms = 0;     /* last tap timestamp for double-tap */
static uint32_t g_pinch_dist0 = 0;     /* initial pinch distance */
static uint8_t  g_pinch_active = 0;    /* pinch-zoom in progress */
static uint32_t g_pinch_zoom0 = 256;  /* zoom level at pinch start */
static uint8_t  g_pan_active = 0;     /* single-finger pan in progress */
static int32_t  g_pan_start_x = 0;    /* touch X when pan started */
static int32_t  g_pan_start_y = 0;    /* touch Y when pan started */
static int32_t  g_pan_orig_x = 0;     /* pan offset X at pan start */
static int32_t  g_pan_orig_y = 0;     /* pan offset Y at pan start */

/* Reposition: uses lv_obj_align(CENTER) + pan offset.
 * LVGL scales around pivot (center). Align center ensures
 * image pivot at screen center, zoom extends equally in all directions. */
void image_viewer_reposition(void)
{
    int32_t w0  = (int32_t)g_img_dsc.header.w;
    int32_t h0  = (int32_t)g_img_dsc.header.h;
    int32_t zw  = (w0 * (int32_t)g_img_zoom) / 256;
    int32_t zh  = (h0 * (int32_t)g_img_zoom) / 256;

    /* Clamp pan: keep at least some portion of image visible.
     * Image center = (512+g_pan_x, 300+g_pan_y); extends zw/2, zh/2 from center. */
    if (zw > 1024) {
        int32_t lim_l = 512 - zw/2;
        int32_t lim_r = zw/2 - 512;
        if (g_pan_x < lim_l) g_pan_x = lim_l;
        if (g_pan_x > lim_r) g_pan_x = lim_r;
    } else { g_pan_x = 0; }
    if (zh > 600) {
        int32_t lim_t = 300 - zh/2;
        int32_t lim_b = zh/2 - 300;
        if (g_pan_y < lim_t) g_pan_y = lim_t;
        if (g_pan_y > lim_b) g_pan_y = lim_b;
    } else { g_pan_y = 0; }

    lv_obj_align(guider_ui.screen_image_viewer_img, LV_ALIGN_CENTER, g_pan_x, g_pan_y);
    if ((uint16_t)g_img_zoom != lv_image_get_scale(guider_ui.screen_image_viewer_img))
        lv_image_set_scale(guider_ui.screen_image_viewer_img, (uint16_t)g_img_zoom);
}


/* Timer-based pinch/pan/swipe handler — runs at ~30 ms independent of which
 * LVGL object received the PRESSING event, so it works reliably even when
 * the image object is not yet hit-tested on first touch. */
static void image_viewer_gesture_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    static uint8_t  prev_num = 0;
    static int32_t  prev_y0;
    static int32_t  swipe_start_x, swipe_start_y;
    uint8_t cur_num = shared_touch_data.num;

    /* --- Released: commit or detect swipe --- */
    if (cur_num == 0 && prev_num > 0) {
        if (g_pinch_active) {
            g_pinch_active = 0;
            g_img_zoom = g_img_saved_zoom;
            image_viewer_reposition();
        }
        if (g_pan_active) {
            g_pan_active = 0;
        }
        if (g_swipe_active && g_album_mode) {
            /* GT9xxx: use prev_y0 for horizontal swipe direction */
            int32_t dx = prev_y0 - swipe_start_x;
            if (dx < -40) {
                g_pan_x = 0; g_pan_y = 0;
                (void)image_viewer_ui_next();
            } else if (dx > 40) {
                g_pan_x = 0; g_pan_y = 0;
                (void)image_viewer_ui_previous();
            }
            g_swipe_active = 0;
        }
        prev_num = 0;
        return;
    }
    if (cur_num == 0) { prev_num = 0; return; }

    /* --- Two-finger pinch --- */
    if (cur_num >= 2) {
        int32_t dx = (int32_t)shared_touch_data.x[0] - (int32_t)shared_touch_data.x[1];
        int32_t dy = (int32_t)shared_touch_data.y[0] - (int32_t)shared_touch_data.y[1];
        uint32_t dist = (uint32_t)sqrtf((float)(dx*dx + dy*dy));

        if (!g_pinch_active && dist > 20) {
            g_pinch_dist0 = dist;
            g_pinch_zoom0 = g_img_zoom;
            g_pinch_active = 1;
        } else if (g_pinch_active && g_pinch_dist0 > 0) {
            int32_t nz = (int32_t)((g_pinch_zoom0 * dist) / g_pinch_dist0);
            if (nz >= 32 && nz <= 1024) {
                g_img_zoom = (uint32_t)nz;
                g_img_saved_zoom = (uint32_t)nz;
                image_viewer_reposition();
            }
        }
        g_swipe_active = 0; g_pan_active = 0;
        prev_num = cur_num;
        prev_y0 = (int32_t)shared_touch_data.y[0];
        return;
    }

    /* --- Single finger: pan (if zoomed) or swipe tracking --- */
    {
        int32_t x0 = (int32_t)shared_touch_data.x[0];
        int32_t y0 = (int32_t)shared_touch_data.y[0];
        int32_t zw0 = ((int32_t)g_img_dsc.header.w * (int32_t)g_img_zoom) / 256;
        int32_t zh0 = ((int32_t)g_img_dsc.header.h * (int32_t)g_img_zoom) / 256;

        if (prev_num != 1) {
            /* Transition from 0 or 2+ → reset tracking.
             * This handles: first touch, and pinch→single-finger transition. */
            if (g_pinch_active) {
                g_pinch_active = 0;
                g_img_zoom = g_img_saved_zoom;
                image_viewer_reposition();
            }
            g_pan_active = 0;
            g_swipe_active = 0;
            /* GT9xxx axes swapped: x=vert, y=horiz */
            swipe_start_x = y0;
            swipe_start_y = x0;
        } else if (!g_pan_active && !g_swipe_active) {
            /* Second+ frame of single touch — decide pan vs swipe */
            /* GT9xxx axes swapped: x=vert, y=horiz */
            int32_t adx = y0 > swipe_start_x ? y0 - swipe_start_x : swipe_start_x - y0;
            int32_t ady = x0 > swipe_start_y ? x0 - swipe_start_y : swipe_start_y - x0;
            if (zw0 > 1024 || zh0 > 600) {
                g_pan_active = 1;
                /* GT9xxx axes swapped: x=vert, y=horiz */
                g_pan_start_x = y0;
                g_pan_start_y = x0;
                g_pan_orig_x = g_pan_x;
                g_pan_orig_y = g_pan_y;
            } else if (g_album_mode && (adx > 30 || ady > 30)) {
                g_swipe_active = 1;
            }
        } else if (g_pan_active) {
            /* GT9xxx axes: shared.x=screen-vertical, shared.y=screen-horizontal.
             * g_pan_start_x holds old y0, g_pan_start_y holds old x0. */
            int32_t nx = g_pan_orig_x + (y0 - g_pan_start_x);
            int32_t ny = g_pan_orig_y + (x0 - g_pan_start_y);
            if (nx != g_pan_x || ny != g_pan_y) {
                g_pan_x = nx;
                g_pan_y = ny;
                image_viewer_reposition();
            }
        }
        prev_y0 = y0;
    }

    prev_num = cur_num;
}

static void image_viewer_swipe_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    /* CLICKED: double-tap zoom + single-tap toggle bar (both modes) */
    if (code == LV_EVENT_CLICKED) {
        if (g_pinch_active || g_pan_active) return;
        uint32_t now = lv_tick_get();
        if (now - g_last_tap_ms < 400) {
            /* Double tap: toggle between fit-screen and 100% */
            g_pan_x = 0; g_pan_y = 0;
            if (g_img_zoom == 256)
                g_img_zoom = g_img_fit_zoom;
            else
                g_img_zoom = 256;
            image_viewer_reposition();
            g_last_tap_ms = 0;
        } else {
            g_last_tap_ms = now;
            /* Toggle bar only — image position is independent */
            if (lv_obj_has_flag(guider_ui.screen_image_viewer_cont_2, LV_OBJ_FLAG_HIDDEN))
                lv_obj_remove_flag(guider_ui.screen_image_viewer_cont_2, LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(guider_ui.screen_image_viewer_cont_2, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    /* Pinch / pan / swipe (all modes; swipe only in album mode) */
    switch (code) {
    case LV_EVENT_PRESSING: {
        /* Pinch-to-zoom: once active, keep it alive even if num drops
         * to 1 briefly (GT9xxx sometimes reports one finger before the
         * other updates).  Also use Euclidean distance for accurate scale. */
        if (shared_touch_data.num >= 2 || g_pinch_active) {
            if (shared_touch_data.num >= 2) {
                int32_t dx = (int32_t)shared_touch_data.x[0] - (int32_t)shared_touch_data.x[1];
                int32_t dy = (int32_t)shared_touch_data.y[0] - (int32_t)shared_touch_data.y[1];
                uint32_t dist = (uint32_t)sqrtf((float)(dx*dx + dy*dy));
                if (!g_pinch_active && dist > 20) {
                    g_pinch_dist0 = dist;
                    g_pinch_zoom0 = g_img_zoom;
                    g_pinch_active = 1;
                } else if (g_pinch_active && g_pinch_dist0 > 0) {
                    int32_t new_zoom = (int32_t)((g_pinch_zoom0 * dist) / g_pinch_dist0);
                    if (new_zoom >= 32 && new_zoom <= 1024) {
                        g_img_zoom = (uint32_t)new_zoom;
                        g_img_saved_zoom = (uint32_t)new_zoom;
                        image_viewer_reposition();
                    } else {
                    }
                }
            }
            g_swipe_active = 0; g_pan_active = 0;
            break;
        }

        /* Single finger: pan if zoomed image > screen, else swipe */
        {
            int32_t zw0 = ((int32_t)g_img_dsc.header.w * (int32_t)g_img_zoom) / 256;
            int32_t zh0 = ((int32_t)g_img_dsc.header.h * (int32_t)g_img_zoom) / 256;
            lv_point_t pt;
            lv_indev_get_vect(lv_indev_active(), &pt);
            int32_t adx = pt.x > 0 ? pt.x : -pt.x;
            int32_t ady = pt.y > 0 ? pt.y : -pt.y;

            if ((zw0 > 1024 || zh0 > 600) && !g_pan_active && !g_swipe_active) {
                /* Start pan: record current clamped pan + touch origin */
                g_pan_active = 1;
                lv_indev_get_point(lv_indev_active(), &pt);
                g_pan_start_x = pt.x;
                g_pan_start_y = pt.y;
                g_pan_orig_x = g_pan_x;
                g_pan_orig_y = g_pan_y;
            } else if (g_pan_active) {
                /* Continue pan: delta from touch start, reposition clamps result.
                 * Only update when pan actually changed to avoid redraw on every poll. */
                lv_indev_get_point(lv_indev_active(), &pt);
                int32_t nx = g_pan_orig_x + (pt.x - g_pan_start_x);
                int32_t ny = g_pan_orig_y + (pt.y - g_pan_start_y);
                if (nx != g_pan_x || ny != g_pan_y) {
                    g_pan_x = nx;
                    g_pan_y = ny;
                    image_viewer_reposition();
                }
            } else if (g_album_mode && !g_swipe_active && (adx > 30 || ady > 30)) {
                g_swipe_active = 1;
            }
        }
        break;
    }
    case LV_EVENT_RELEASED: {
        if (g_pinch_active) {
            g_pinch_active = 0;
            g_img_zoom = g_img_saved_zoom;  /* ensure zoom sticks */
            image_viewer_reposition();
        } else if (g_pan_active) {
            g_pan_active = 0;
        } else if (g_swipe_active && g_album_mode) {
            lv_point_t pt;
            lv_indev_get_vect(lv_indev_active(), &pt);
            if (pt.x < -40) {
                g_pan_x = 0; g_pan_y = 0;
                (void)image_viewer_ui_next();
            } else if (pt.x > 40) {
                g_pan_x = 0; g_pan_y = 0;
                (void)image_viewer_ui_previous();
            }
            g_swipe_active = 0;
        }
        break;
    }
    case LV_EVENT_CLICKED:
        /* Ignore clicks during active gestures */
        if (g_pinch_active || g_pan_active) break;
        /* Double-tap: toggle between fit-screen and 100% zoom */
        {
            uint32_t now = lv_tick_get();
            if (now - g_last_tap_ms < 400) {
                /* Double tap: toggle zoom, reset pan */
                g_pan_x = 0; g_pan_y = 0;
                if (g_img_zoom == 256)
                    g_img_zoom = g_img_fit_zoom;
                else
                    g_img_zoom = 256;
                image_viewer_reposition();
                g_last_tap_ms = 0;
            } else {
                /* Single tap: toggle bar only — image position is independent */
                g_last_tap_ms = now;
                if (lv_obj_has_flag(guider_ui.screen_image_viewer_cont_2, LV_OBJ_FLAG_HIDDEN))
                    lv_obj_remove_flag(guider_ui.screen_image_viewer_cont_2, LV_OBJ_FLAG_HIDDEN);
                else
                    lv_obj_add_flag(guider_ui.screen_image_viewer_cont_2, LV_OBJ_FLAG_HIDDEN);
            }
        }
        break;
    default:
        break;
    }
}

/* Single image display (from SD file browser) */
void image_viewer_show_jpeg(uint8_t *buf, uint32_t size, const char *name)
{
    g_album_mode = 0;

    ui_load_scr_animation(&guider_ui, &guider_ui.screen_image_viewer,
        guider_ui.screen_image_viewer_del, &guider_ui.DeskTop_del,
        setup_scr_screen_image_viewer, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);

    {
        char utf8_name[256];
        gbk_to_utf8(name, utf8_name, sizeof(utf8_name));
        lv_label_set_text(guider_ui.screen_image_viewer_label_2, utf8_name);
    }
    lv_obj_remove_flag(guider_ui.screen_image_viewer_cont_2, LV_OBJ_FLAG_HIDDEN);

    {
        uint8_t r = image_viewer_decode_jpeg(buf, size);
        char tmp[48];
        if (r == 1) {
            snprintf(tmp, sizeof(tmp), "\u5c3a\u5bf8\u9519\u8bef (%luB)", (unsigned long)size);
            lv_label_set_text(guider_ui.screen_image_viewer_label_2, tmp);
        } else if (r == 4) {
            lv_label_set_text(guider_ui.screen_image_viewer_label_2, "\u4e0d\u652f\u6301\u6e10\u8fdbJPEG");
        } else if (r == 2) {
            lv_label_set_text(guider_ui.screen_image_viewer_label_2, "\u521d\u59cb\u5316\u5931\u8d25");
        } else if (r != 0) {
            lv_label_set_text(guider_ui.screen_image_viewer_label_2, "\u89e3\u7801\u5931\u8d25");
        }
    }
}

/* Album mode: open gallery from ALBUM_DIR */
static void image_viewer_open_album(void)
{
    g_album_mode = 1;
    if (image_viewer_ui_open_album(ALBUM_DIR) != IMAGE_VIEWER_OK) {
        g_album_mode = 0;
    }
}

static void screen_image_viewer_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        g_album_mode = 0;
        image_viewer_ui_stop();
        ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop,
            guider_ui.DeskTop_del, &guider_ui.screen_image_viewer_del,
            setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
        break;
    }
    default:
        break;
    }
}



/* ========================================================================
 *  AVI / MJPEG Direct Playback (for yuanshen icon)
 *  Stops LVGL, plays AVI full-screen at max speed, then restores LVGL.
 * ======================================================================== */

enum {
    AVI_CP_OPENED = 1,
    AVI_CP_FRAME_HEADER,
    AVI_CP_FRAME_READ,
    AVI_CP_CACHE_CLEAN,
    AVI_CP_JPEG_INIT,
    AVI_CP_JPEG_START,
    AVI_CP_YUV_READY,
    AVI_CP_DMA2D_DONE,
    AVI_CP_FRAME_READY,
    AVI_CP_LTDC_SWITCH
};

volatile uint8_t g_avi_checkpoint = 0;
volatile uint8_t g_avi_diag_active = 0;
volatile uint8_t g_avi_diag_first_frame = 0;

void avi_diag_checkpoint(uint8_t checkpoint)
{
    if (g_avi_diag_active == 0U || g_avi_diag_first_frame == 0U) return;

    g_avi_checkpoint = checkpoint;
    printf("[AVI-CP%02u] tick=%lu jpeg_sr=%08lX jpeg_cr=%08lX mdma_gisr=%08lX dma2d_isr=%08lX\r\n",
           (unsigned int)checkpoint,
           (unsigned long)HAL_GetTick(),
           (unsigned long)JPEG->SR,
           (unsigned long)JPEG->CR,
           (unsigned long)MDMA->GISR0,
           (unsigned long)DMA2D->ISR);
}

static uint8_t avi_player_decode_frame_direct(uint8_t *buf, uint32_t size)
{
    uint16_t w = 0, h = 0;
    uint32_t i;
    uint32_t decode_target;
    mjpeg_result_t res;

    for (i = 0; i + 9 < size; i++) {
        if (buf[i] == 0xFF && buf[i+1] == 0xC0) {
            h = ((uint16_t)buf[i+5] << 8) | buf[i+6];
            w = ((uint16_t)buf[i+7] << 8) | buf[i+8];
            break;
        }
    }
    if (w == 0 || h == 0 || w > 2048 || h > 2048) {
        printf("[AVI-FIRST] invalid JPEG dimensions: %ux%u size=%lu checkpoint=%u\r\n",
               (unsigned int)w, (unsigned int)h,
               (unsigned long)size, (unsigned int)g_avi_checkpoint);
        return 1U;
    }

    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)((uint32_t)buf & ~0x1FUL), (size + 31) & ~0x1F);
    avi_diag_checkpoint(AVI_CP_CACHE_CLEAN);

    decode_target = ltdc_video_output_acquire_decode_target();
    if (decode_target == 0U) {
        printf("[AVI] no LTDC decode buffer\r\n");
        return 2U;
    }

    res = mjpeg_session_decode(buf, size, decode_target, 1000U);
    if (res != MJPEG_OK || mjpeg.state != JPEG_STATE_FINISHED) {
        printf("[AVI] mjpeg_decode failed: res=%u state=%u checkpoint=%u jpeg_sr=%08lX mdma_gisr=%08lX dma2d_isr=%08lX\r\n",
               (unsigned int)res, (unsigned int)mjpeg.state,
               (unsigned int)g_avi_checkpoint,
               (unsigned long)JPEG->SR,
               (unsigned long)MDMA->GISR0,
               (unsigned long)DMA2D->ISR);
        return 3U;
    }
    avi_diag_checkpoint(AVI_CP_FRAME_READY);

    if (g_avi_diag_first_frame != 0U) {
        if (!ltdc_video_output_activate_first_frame(50U)) {
            printf("[AVI] first VBlank activation timeout\r\n");
            return 4U;
        }
        printf("[AVI] first frame activated at VBlank\r\n");
        avi_diag_checkpoint(AVI_CP_LTDC_SWITCH);
    } else {
        if (!ltdc_video_output_present(50U)) {
            printf("[AVI] VBlank present timeout\r\n");
            return 5U;
        }
    }

    g_avi_diag_first_frame = 0U;
    return 0U;
}

static uint8_t avi_player_direct_play(void)
{
    static FIL      dfil;
    uint8_t         hdr_buf[16384];
    UINT            br;
    FRESULT         fr;
    AVISTATUS       avr;
    uint32_t        movi_off;
    uint32_t        frame_idx = 0;
    uint32_t        total_frames;

    const char *avi_path = "0:SYSTEM/AVI/yuanshen_ launch.avi";

    g_avi_checkpoint = 0U;
    g_avi_diag_active = 1U;
    g_avi_diag_first_frame = 1U;
    printf("[AVI] direct play start, path: %s\r\n", avi_path);

    /* AVI AUDIO USER PATCH: video owns SAI and must not mix with music. */
    (void)music_player_stop();
    if (avi_sd_session_acquire() != 0) {
        g_avi_diag_active = 0U;
        return 1U;
    }
    fr = f_open(&dfil, avi_path, FA_READ);
    if (fr != FR_OK) {
        printf("[AVI] f_open failed: %d\r\n", (int)fr);
        g_avi_diag_active = 0U;
        avi_sd_session_release();
        return 1;
    }
    printf("[AVI] file opened OK\r\n");
    avi_diag_checkpoint(AVI_CP_OPENED);

    fr = f_read(&dfil, hdr_buf, sizeof(hdr_buf), &br);
    if (fr != FR_OK || br < 256) {
        printf("[AVI] f_read hdr failed: fr=%d br=%d\r\n", (int)fr, (int)br);
        f_close(&dfil);
        g_avi_diag_active = 0U;
        avi_sd_session_release();
        return 1;
    }
    printf("[AVI] header read OK, %d bytes\r\n", (int)br);

    avr = avi_init(hdr_buf, br);
    if (avr == AVI_STREAM_ERR) {
        /* AVI AUDIO USER PATCH: metadata is valid even when the first
         * audio chunk lies beyond the 16KB header window. */
        printf("[AVI] audio chunk is outside header window\r\n");
        total_frames = g_avix.TotalFrame;
    } else if (avr != AVI_OK) {
        printf("[AVI] avi_init failed: %d\r\n", (int)avr);
        f_close(&dfil);
        g_avi_diag_active = 0U;
        avi_sd_session_release();
        return 1;
    } else {
        total_frames = g_avix.TotalFrame;
    }

    printf("[AVI] parsed OK: %lux%lu, %lu frames, period=%luus\r\n",
        (unsigned long)g_avix.Width, (unsigned long)g_avix.Height,
        (unsigned long)total_frames, (unsigned long)g_avix.SecPerFrame);

    movi_off = avi_srarch_id(hdr_buf, br, "movi");
    if (movi_off == 0) {
        printf("[AVI] movi not found\r\n");
        f_close(&dfil);
        g_avi_diag_active = 0U;
        avi_sd_session_release();
        return 1;
    }
    printf("[AVI] movi offset: %lu\r\n", (unsigned long)movi_off);

    f_lseek(&dfil, movi_off + 4);
    avi_diag_checkpoint(AVI_CP_JPEG_INIT);
    if (mjpeg_session_open((uint16_t)g_avix.Width, (uint16_t)g_avix.Height) != MJPEG_OK) {
        printf("[AVI] JPEG session open failed: %u\r\n",
               (unsigned int)mjpeg_session_get_last_error());
        f_close(&dfil);
        g_avi_diag_active = 0U;
        avi_sd_session_release();
        return 1U;
    }
    if (!ltdc_video_output_prepare((uint16_t)g_avix.Width,
                                   (uint16_t)g_avix.Height)) {
        printf("[AVI] LTDC video output prepare failed\r\n");
        mjpeg_session_close();
        f_close(&dfil);
        g_avi_diag_active = 0U;
        avi_sd_session_release();
        return 1U;
    }
    avi_player_audio_start();

    while (1) {
        uint8_t  hdr[8];
        uint32_t cid, csize;
        uint32_t payload_size;

        if (frame_idx == 2U) {
            printf("[AVI-F3-01] tick=%lu outer_pos=%lu\r\n",
                   (unsigned long)HAL_GetTick(),
                   (unsigned long)f_tell(&dfil));
        }
        if (f_read(&dfil, hdr, 8, &br) != FR_OK || br != 8) {
            printf("[AVI] chunk hdr fail at %lu\r\n", (unsigned long)frame_idx);
            break;
        }

        cid   = hdr[0] | ((uint32_t)hdr[1] << 8) | ((uint32_t)hdr[2] << 16) | ((uint32_t)hdr[3] << 24);
        csize = hdr[4] | ((uint32_t)hdr[5] << 8) | ((uint32_t)hdr[6] << 16) | ((uint32_t)hdr[7] << 24);
        payload_size = csize;
        if (csize & 1) csize++;
        if (frame_idx == 2U) {
            printf("[AVI-F3-02] tick=%lu cid=%08lX size=%lu pos=%lu\r\n",
                   (unsigned long)HAL_GetTick(),
                   (unsigned long)cid, (unsigned long)csize,
                   (unsigned long)f_tell(&dfil));
        }

        if (cid == 0x63643030) {
            if (csize > G_FILE_CONTENT_SIZE || csize < 16) break;
            avi_diag_checkpoint(AVI_CP_FRAME_HEADER);
            if (frame_idx == 2U) {
                printf("[AVI-F3-07] tick=%lu payload=%lu\r\n",
                       (unsigned long)HAL_GetTick(), (unsigned long)csize);
            }
            if (f_read(&dfil, (uint8_t *)g_file_content, csize, &br) != FR_OK || br != csize) break;
            if (frame_idx == 2U) {
                printf("[AVI-F3-08] tick=%lu br=%lu pos=%lu\r\n",
                       (unsigned long)HAL_GetTick(), (unsigned long)br,
                       (unsigned long)f_tell(&dfil));
                printf("[AVI-F3-09] tick=%lu decode_start\r\n",
                       (unsigned long)HAL_GetTick());
            }
            avi_diag_checkpoint(AVI_CP_FRAME_READ);

            if (avi_player_decode_frame_direct((uint8_t *)g_file_content, csize) != 0U) goto done;
            if (frame_idx == 2U) {
                printf("[AVI-F3-10] tick=%lu decode_done\r\n",
                       (unsigned long)HAL_GetTick());
            }
            frame_idx++;


            if (frame_idx >= total_frames) break;
        } else if (cid == 0x62773030 || cid == 0x62773031 || cid == 0x62773130) {
            if (avi_player_audio_read(&dfil, payload_size) != 0U) goto done;
            while (1) {
                uint8_t nh[8]; UINT nb;
                uint32_t next_payload_size;
                if (frame_idx == 2U) {
                    printf("[AVI-F3-05] tick=%lu inner_pos=%lu\r\n",
                           (unsigned long)HAL_GetTick(),
                           (unsigned long)f_tell(&dfil));
                }
                if (f_read(&dfil, nh, 8, &nb) != FR_OK || nb != 8) goto done;
                uint32_t nid = nh[0] | ((uint32_t)nh[1] << 8) | ((uint32_t)nh[2] << 16) | ((uint32_t)nh[3] << 24);
                uint32_t nsz = nh[4] | ((uint32_t)nh[5] << 8) | ((uint32_t)nh[6] << 16) | ((uint32_t)nh[7] << 24);
                next_payload_size = nsz;
                if (frame_idx == 2U) {
                    printf("[AVI-F3-06] tick=%lu nid=%08lX size=%lu pos=%lu\r\n",
                           (unsigned long)HAL_GetTick(),
                           (unsigned long)nid, (unsigned long)nsz,
                           (unsigned long)f_tell(&dfil));
                }
                if (nid == 0x63643030) {
                    if (nsz & 1) nsz++;
                    if (nsz > G_FILE_CONTENT_SIZE || nsz < 16) goto done;
                    avi_diag_checkpoint(AVI_CP_FRAME_HEADER);
                    if (frame_idx == 2U) {
                        printf("[AVI-F3-07] tick=%lu payload=%lu\r\n",
                               (unsigned long)HAL_GetTick(),
                               (unsigned long)nsz);
                    }
                    if (f_read(&dfil, (uint8_t *)g_file_content, nsz, &nb) != FR_OK || nb != nsz) goto done;
                    if (frame_idx == 2U) {
                        printf("[AVI-F3-08] tick=%lu br=%lu pos=%lu\r\n",
                               (unsigned long)HAL_GetTick(),
                               (unsigned long)nb,
                               (unsigned long)f_tell(&dfil));
                        printf("[AVI-F3-09] tick=%lu decode_start\r\n",
                               (unsigned long)HAL_GetTick());
                    }
                    avi_diag_checkpoint(AVI_CP_FRAME_READ);

                    if (avi_player_decode_frame_direct((uint8_t *)g_file_content, nsz) != 0U) goto done;
                    if (frame_idx == 2U) {
                        printf("[AVI-F3-10] tick=%lu decode_done\r\n",
                               (unsigned long)HAL_GetTick());
                    }
                    frame_idx++;

                    if (frame_idx >= total_frames) goto done;
                    break;
                } else if (nid == 0x78647831) {
                    goto done;
                } else if (nid == 0x62773030 || nid == 0x62773031 ||
                           nid == 0x62773130) {
                    if (avi_player_audio_read(&dfil,
                                              next_payload_size) != 0U) {
                        goto done;
                    }
                } else {
                    if (nsz & 1) nsz++;
                    f_lseek(&dfil, f_tell(&dfil) + nsz);
                }
            }
        } else if (cid == 0x78647831) {
            break;
        } else {
            f_lseek(&dfil, f_tell(&dfil) + csize);
        }
        audio_poll();
    }

done:
    g_avi_diag_active = 0U;
    avi_player_audio_stop();
    printf("[AVI] playback done, %lu frames shown\r\n", (unsigned long)frame_idx);
    f_close(&dfil);
    /* Clean DMA2D state before LVGL HAL resumes */
    DMA2D->CR      = 0x00000000U;
    DMA2D->IFCR    = 0x0000003FU;
    mjpeg_session_close();
    ltdc_video_output_abort();
    lv_obj_invalidate(lv_scr_act());
    printf("[AVI] LVGL restored, desktop refreshed\r\n");
    avi_sd_session_release();
    return 0;
}


/* ========================================================================
 *  AVI / MJPEG Video Player
 * ======================================================================== */
extern AVI_INFO g_avix;  /* from avi.c */

static FIL       g_avi_fil;
static uint8_t   g_avi_open = 0;
static uint32_t  g_avi_frame_idx;
static uint32_t  g_avi_frame_period_ms;
static lv_timer_t *g_avi_timer = NULL;

static void avi_player_timer_cb(lv_timer_t *timer);
static void avi_player_stop(void);
static uint8_t avi_player_decode_frame(uint8_t *buf, uint32_t size);

static int avi_stricmp(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return (int)(unsigned char)ca - (int)(unsigned char)cb;
        a++; b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static void avi_player_open(const char *path, const char *name)
{
    uint8_t  hdr_buf[16384];
    UINT     br;
    FRESULT  fr;
    AVISTATUS avr;
    uint32_t movi_off;

    /* AVI AUDIO USER PATCH: stop background music before taking SAI. */
    (void)music_player_stop();
    if (avi_sd_session_acquire() != 0) {
        lv_label_set_text(guider_ui.DeskTop_label_21, "SD BUSY");
        return;
    }
    fr = f_open(&g_avi_fil, path, FA_READ);
    if (fr != FR_OK) {
        lv_label_set_text(guider_ui.DeskTop_label_21, "??????");
        avi_sd_session_release();
        return;
    }

    fr = f_read(&g_avi_fil, hdr_buf, sizeof(hdr_buf), &br);
    if (fr != FR_OK || br < 256) {
        f_close(&g_avi_fil);
        avi_sd_session_release();
        lv_label_set_text(guider_ui.DeskTop_label_21, "???????");
        return;
    }

    avr = avi_init(hdr_buf, br);
    if (avr == AVI_STREAM_ERR) {
        /* AVI AUDIO USER PATCH: keep parsed metadata; only the first audio
         * payload was outside the header buffer. */
    } else if (avr != AVI_OK) {
        f_close(&g_avi_fil);
        avi_sd_session_release();
        lv_label_set_text(guider_ui.DeskTop_label_21, "???????");
        return;
    }

    movi_off = avi_srarch_id(hdr_buf, br, "movi");
    if (movi_off == 0) {
        f_close(&g_avi_fil);
        avi_sd_session_release();
        lv_label_set_text(guider_ui.DeskTop_label_21, "MOVI???");
        return;
    }

    f_lseek(&g_avi_fil, movi_off + 4);

    if (mjpeg_session_open((uint16_t)g_avix.Width, (uint16_t)g_avix.Height) != MJPEG_OK) {
        f_close(&g_avi_fil);
        avi_sd_session_release();
        lv_label_set_text(guider_ui.DeskTop_label_21, "JPEG ERR");
        return;
    }
    avi_player_audio_start();

    g_avi_frame_idx = 0;
    g_avi_frame_period_ms = g_avix.SecPerFrame / 1000;
    if (g_avi_frame_period_ms < 20)  g_avi_frame_period_ms = 20;
    if (g_avi_frame_period_ms > 500) g_avi_frame_period_ms = 500;
    g_avi_open = 1;

    ui_load_scr_animation(&guider_ui, &guider_ui.screen_avi_player,
        guider_ui.screen_avi_player_del, &guider_ui.DeskTop_del,
        setup_scr_screen_avi_player, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);

    {
        char utf8_name[256];
        gbk_to_utf8(name, utf8_name, sizeof(utf8_name));
        lv_label_set_text(guider_ui.screen_avi_player_label_2, utf8_name);
    }

    if (g_avi_timer) lv_timer_del(g_avi_timer);
    g_avi_timer = lv_timer_create(avi_player_timer_cb, g_avi_frame_period_ms, NULL);
}

static void avi_player_timer_cb(lv_timer_t *timer)
{
    uint8_t  hdr[8];
    UINT     br;
    uint32_t cid, csize, payload_size;
    char     info[64];

    if (!g_avi_open) {
        lv_timer_del(timer);
        g_avi_timer = NULL;
        return;
    }

    if (f_read(&g_avi_fil, hdr, 8, &br) != FR_OK || br != 8) goto stop;

    cid   = hdr[0] | ((uint32_t)hdr[1] << 8) | ((uint32_t)hdr[2] << 16) | ((uint32_t)hdr[3] << 24);
    csize = hdr[4] | ((uint32_t)hdr[5] << 8) | ((uint32_t)hdr[6] << 16) | ((uint32_t)hdr[7] << 24);
    payload_size = csize;
    if (csize & 1) csize++;  /* pad to even for chunk alignment */

    if (cid == 0x63643030) {
        if (csize > G_FILE_CONTENT_SIZE || csize < 16) goto stop;
        if (f_read(&g_avi_fil, g_file_content, csize, &br) != FR_OK || br != csize) goto stop;
        if (avi_player_decode_frame(g_file_content, csize) != 0U) goto stop;
        g_avi_frame_idx++;
        snprintf(info, sizeof(info), "?: %lu / %lu",
            (unsigned long)g_avi_frame_idx, (unsigned long)g_avix.TotalFrame);
        lv_label_set_text(guider_ui.screen_avi_player_label_3, info);
        if (g_avi_frame_idx >= g_avix.TotalFrame) goto stop;
        lv_timer_set_period(timer, g_avi_frame_period_ms);
    } else if (cid == 0x62773030 || cid == 0x62773031 || cid == 0x62773130) {
        if (avi_player_audio_read(&g_avi_fil, payload_size) != 0U) goto stop;
        while (1) {
            uint8_t nh[8]; UINT nb;
            uint32_t next_payload_size;
            if (f_read(&g_avi_fil, nh, 8, &nb) != FR_OK || nb != 8) goto stop;
            uint32_t nid = nh[0] | ((uint32_t)nh[1] << 8) | ((uint32_t)nh[2] << 16) | ((uint32_t)nh[3] << 24);
            uint32_t nsz = nh[4] | ((uint32_t)nh[5] << 8) | ((uint32_t)nh[6] << 16) | ((uint32_t)nh[7] << 24);
            next_payload_size = nsz;
            if (nid == 0x63643030) {
                /* Found next video frame, process it now */
                if (nsz & 1) nsz++;
                if (nsz > G_FILE_CONTENT_SIZE || nsz < 16) goto stop;
                if (f_read(&g_avi_fil, g_file_content, nsz, &nb) != FR_OK || nb != nsz) goto stop;
                if (avi_player_decode_frame(g_file_content, nsz) != 0U) goto stop;
                g_avi_frame_idx++;
                snprintf(info, sizeof(info), "?: %lu / %lu",
                    (unsigned long)g_avi_frame_idx, (unsigned long)g_avix.TotalFrame);
                lv_label_set_text(guider_ui.screen_avi_player_label_3, info);
                if (g_avi_frame_idx >= g_avix.TotalFrame) goto stop;
                lv_timer_set_period(timer, g_avi_frame_period_ms);
                return;
            } else if (nid == 0x78647831) {
                goto stop;
            } else if (nid == 0x62773030 || nid == 0x62773031 ||
                       nid == 0x62773130) {
                if (avi_player_audio_read(&g_avi_fil,
                                          next_payload_size) != 0U) {
                    goto stop;
                }
            } else {
                /* Skip unknown/audio chunk */
                if (nsz & 1) nsz++;
                f_lseek(&g_avi_fil, f_tell(&g_avi_fil) + nsz);
            }
        }
    } else if (cid == 0x78647831) {
        goto stop;
    } else {
        if (csize & 1) csize++;
        f_lseek(&g_avi_fil, f_tell(&g_avi_fil) + csize);
        lv_timer_set_period(timer, 5);
    }
    return;

stop:
    avi_player_stop();
}

static void avi_player_stop(void)
{
    avi_player_audio_stop();
    if (g_avi_open) {
        f_close(&g_avi_fil);
        g_avi_open = 0;
    }
    if (g_avi_timer) {
        lv_timer_del(g_avi_timer);
        g_avi_timer = NULL;
    }
    avi_sd_session_release();
    mjpeg_session_close();
    lv_label_set_text(guider_ui.screen_avi_player_label_2, "????");
}

static uint8_t avi_player_decode_frame(uint8_t *buf, uint32_t size)
{
    uint16_t w = 0, h = 0;
    uint32_t i;
    mjpeg_result_t res;

    for (i = 0; i + 9 < size; i++) {
        if (buf[i] == 0xFF && buf[i+1] == 0xC0) {
            h = ((uint16_t)buf[i+5] << 8) | buf[i+6];
            w = ((uint16_t)buf[i+7] << 8) | buf[i+8];
            break;
        }
    }
    if (w == 0 || h == 0 || w > 2048 || h > 2048) return 1;


    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)((uint32_t)buf & ~0x1FUL), (size + 31) & ~0x1F);

    res = mjpeg_session_decode(buf, size, 0xC1D00000UL, 1000U);
    if (res != MJPEG_OK || mjpeg.state != JPEG_STATE_FINISHED) {
        return 3;
    }

    g_img_dsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
    g_img_dsc.header.cf     = LV_COLOR_FORMAT_RGB565;
    g_img_dsc.header.w      = w;
    g_img_dsc.header.h      = h;
    g_img_dsc.data_size     = w * h * 2;
    g_img_dsc.data          = (uint8_t *)0xC1D00000UL;
    g_img_dsc.header.stride = w * 2;

    lv_image_set_src(guider_ui.screen_avi_player_img, &g_img_dsc);
    /* LVGL AVI playback uses native pixels; scaling every frame is too costly. */
    lv_image_set_scale(guider_ui.screen_avi_player_img, 256U);
    lv_obj_align(guider_ui.screen_avi_player_img, LV_ALIGN_CENTER, 0, 0);

    return 0;
}

static void screen_avi_player_btn_1_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        avi_player_stop();
        ui_load_scr_animation(&guider_ui, &guider_ui.DeskTop,
            guider_ui.DeskTop_del, &guider_ui.screen_avi_player_del,
            setup_scr_DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
    }
}

void events_init_screen_avi_player(lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_avi_player_btn_1,
        screen_avi_player_btn_1_event_handler, LV_EVENT_ALL, ui);
}
void events_init_screen_image_viewer (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_image_viewer_btn_1,
        screen_image_viewer_btn_1_event_handler, LV_EVENT_ALL, ui);
    /* Event handler for CLICKED (double-tap / bar toggle) */
    lv_obj_add_event_cb(ui->screen_image_viewer_img,
        image_viewer_swipe_handler, LV_EVENT_CLICKED, ui);
    /* Timer-based gesture handler: pinch, pan, swipe.
     * Runs at 30 ms independent of which object received the touch,
     * so it works reliably even on first touch before the image is hit-tested.
     * Create only once (static guard prevents duplicates across re-entries). */
    {
        static lv_timer_t *gesture_timer = NULL;
        if (gesture_timer == NULL)
            gesture_timer = lv_timer_create(image_viewer_gesture_timer_cb, 30, NULL);
    }
}
