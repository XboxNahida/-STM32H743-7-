/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef WIDGET_INIT_H
#define WIDGET_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "gui_guider.h"

__attribute__((unused)) void kb_event_cb(lv_event_t *e);
__attribute__((unused)) void ta_event_cb(lv_event_t *e);
#if LV_USE_ANALOGCLOCK != 0
void clock_count(int *hour, int *min, int *sec);
void digital_clock_count(int * hour, int * minute, int * seconds, char * meridiem);
#endif


void lock_screen_digital_clock_1_timer(lv_timer_t *timer);
void lock_screen_datetext_1_event_handler(lv_event_t *e);
void lock_screen_datetext_1_calendar_event_handler(lv_event_t *e);
void lock_screen_datetext_1_init_calendar(lv_obj_t *obj, char * s);
extern lv_calendar_date_t DeskTop_calendar_1_today;
extern lv_calendar_date_t DeskTop_calendar_1_highlihted_days[1];
void DeskTop_calendar_1_event_handler(lv_event_t * e);
void DeskTop_calendar_1_draw_part_begin_event_cb(lv_event_t * e);
void DeskTop_analog_clock_1_timer(lv_timer_t *timer);
void DeskTop_datetext_1_event_handler(lv_event_t *e);
void DeskTop_datetext_1_calendar_event_handler(lv_event_t *e);
void DeskTop_datetext_1_init_calendar(lv_obj_t *obj, char * s);
void DeskTop_digital_clock_2_timer(lv_timer_t *timer);
void DeskTop_datetext_2_event_handler(lv_event_t *e);
void DeskTop_datetext_2_calendar_event_handler(lv_event_t *e);
void DeskTop_datetext_2_init_calendar(lv_obj_t *obj, char * s);
void DeskTop_digital_clock_1_timer(lv_timer_t *timer);
void screen_navigation_bar_datetext_1_event_handler(lv_event_t *e);
void screen_navigation_bar_datetext_1_calendar_event_handler(lv_event_t *e);
void screen_navigation_bar_datetext_1_init_calendar(lv_obj_t *obj, char * s);
void screen_navigation_bar_digital_clock_1_timer(lv_timer_t *timer);
void screen_notes_digital_clock_2_timer(lv_timer_t *timer);
void screen_Signal_Proc_digital_clock_1_timer(lv_timer_t *timer);
void screen_yuanshen_digital_clock_1_timer(lv_timer_t *timer);
void rtc_clock_1s_timer_cb(lv_timer_t *timer);
void alarm_popup_show(void);


/* navigation bar toggle button bitmask */
#define NAV_BTN_3   (1 << 0)
#define NAV_BTN_5   (1 << 1)
#define NAV_BTN_7   (1 << 2)
#define NAV_BTN_8   (1 << 3)
#define NAV_BTN_9   (1 << 4)
#define NAV_BTN_10  (1 << 5)
#define NAV_BTN_14  (1 << 6)
extern uint8_t g_nav_btn_state;

#if LV_USE_IME_PINYIN
extern lv_pinyin_dict_t gg_pinyin_dict[];
#endif
#ifdef __cplusplus
}
#endif
#endif
