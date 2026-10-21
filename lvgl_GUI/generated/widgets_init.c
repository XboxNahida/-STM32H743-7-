/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include "gui_guider.h"
#include "widgets_init.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "rtc.h"
/* STAGE6 NOTES USER PATCH: restore after GUI Guider regeneration. */
#include "selection_keyboard.h"

#if LV_USE_IME_PINYIN
extern lv_obj_t *g_kb_ime;
#endif

__attribute__((unused)) void kb_event_cb (lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);
#if LV_USE_IME_PINYIN
    static uint8_t skip_hide = 0;
#endif
    if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
#if LV_USE_IME_PINYIN
        if (skip_hide) {
            skip_hide = 0;
            return;
        }
        if(g_kb_ime != NULL) {
            lv_ime_pinyin_set_textarea(g_kb_ime, NULL);
        }
#endif
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
#if LV_USE_IME_PINYIN
    else if(code == LV_EVENT_VALUE_CHANGED) {
        /* IME toggle (LV_SYMBOL_KEYBOARD / 123) fires VALUE_CHANGED then READY.
         * Skip the next READY/CANCEL to prevent keyboard from closing. */
        skip_hide = 1;
    }
#endif
}

__attribute__((unused)) void ta_event_cb (lv_event_t *e) {
#if LV_USE_KEYBOARD
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = lv_event_get_user_data(e);

    if(code == LV_EVENT_FOCUSED) {
        if(lv_indev_get_type(lv_indev_active()) != LV_INDEV_TYPE_KEYPAD) {
            lv_keyboard_set_textarea(kb, ta);
#if LV_USE_IME_PINYIN
            if(g_kb_ime != NULL) {
                lv_ime_pinyin_set_keyboard(g_kb_ime, kb);
                lv_ime_pinyin_set_mode(g_kb_ime, LV_IME_PINYIN_MODE_K26);
                lv_ime_pinyin_set_language(
                    g_kb_ime, LV_IME_PINYIN_LANGUAGE_ENGLISH);
                lv_ime_pinyin_set_textarea(g_kb_ime, ta);
            }
#endif
            /* STAGE6 NOTES USER PATCH: every editable shared-keyboard target. */
            (void)selection_keyboard_register_target(ta, NULL, NULL);
            selection_keyboard_focus(ta);
            lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
#if LV_USE_IME_PINYIN
            if(g_kb_ime != NULL) lv_ime_pinyin_reset(g_kb_ime);
#endif
        }
    } else if(code == LV_EVENT_READY) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_state(ta, LV_STATE_FOCUSED);
        lv_indev_reset(NULL, ta);
    } else if(code == LV_EVENT_DEFOCUSED) {
        /* STAGE6 NOTES USER PATCH: clear selection before keyboard unbind. */
        selection_keyboard_blur(ta);
#if LV_USE_IME_PINYIN
        if(g_kb_ime != NULL) lv_ime_pinyin_set_textarea(g_kb_ime, NULL);
#endif
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    } else if(code == LV_EVENT_DELETE) {
        /* STAGE6 NOTES USER PATCH: release target, keep global clipboard. */
#if LV_USE_IME_PINYIN
        if(g_kb_ime != NULL &&
           lv_ime_pinyin_get_textarea(g_kb_ime) == ta) {
            lv_ime_pinyin_set_textarea(g_kb_ime, NULL);
        }
#endif
        selection_keyboard_unregister_target(ta);
    }
#endif
}

void clock_count(int *hour, int *min, int *sec)
{
    (*sec)++;
    if(*sec == 60)
    {
        *sec = 0;
        (*min)++;
    }
    if(*min == 60)
    {
        *min = 0;
        if(*hour < 12)
        {
            (*hour)++;
        } else {
            (*hour)++;
            *hour = *hour %12;
        }
    }
}

void digital_clock_count(int * hour, int * minute, int * seconds, char * meridiem)
{

    (*seconds)++;
    if(*seconds == 60) {
        *seconds = 0;
        (*minute)++;
    }
    if(*minute == 60) {
        *minute = 0;
        if(*hour < 12) {
            (*hour)++;
        }
        else {
            (*hour)++;
            (*hour) = (*hour) % 12;
        }
    }
    if(*hour == 12 && *seconds == 0 && *minute == 0) {
        if((lv_strcmp(meridiem, "PM") == 0)) {
            lv_strcpy(meridiem, "AM");
        }
        else {
            lv_strcpy(meridiem, "PM");
        }
    }
}


extern int lock_screen_digital_clock_1_hour_value;
extern int lock_screen_digital_clock_1_min_value;
extern int lock_screen_digital_clock_1_sec_value;

void lock_screen_digital_clock_1_timer(lv_timer_t *timer)
{
    /* by rtc_clock_1s_timer_cb */
}
static lv_obj_t * lock_screen_datetext_1_calendar;

void lock_screen_datetext_1_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_FOCUSED) {
        char * s = lv_label_get_text(btn);
        if(lock_screen_datetext_1_calendar == NULL) {
            lock_screen_datetext_1_init_calendar(btn, s);
        }
    }
}

void lock_screen_datetext_1_init_calendar(lv_obj_t *obj, char * s)
{
    if (lock_screen_datetext_1_calendar == NULL) {
        lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        lock_screen_datetext_1_calendar = lv_calendar_create(lv_layer_top());
        lv_obj_t * scr = lv_obj_get_screen(obj);
        int32_t scr_height = lv_obj_get_height(scr);
        int32_t scr_width = lv_obj_get_width(scr);
        lv_obj_set_size(lock_screen_datetext_1_calendar, scr_width * 0.8, scr_height * 0.8);
        char * year = strtok(s, "/");
        char * month = strtok(NULL, "/");
        char * day = strtok(NULL, "/");
        lv_calendar_set_showed_date(lock_screen_datetext_1_calendar, atoi(year), atoi(month));
        lv_calendar_date_t highlighted_days[1];       /*Only its pointer will be saved so should be static*/
        highlighted_days[0].year = atoi(year);
        highlighted_days[0].month = atoi(month);
        highlighted_days[0].day = atoi(day);
        lv_calendar_set_highlighted_dates(lock_screen_datetext_1_calendar, highlighted_days, 1);
        lv_obj_align(lock_screen_datetext_1_calendar,LV_ALIGN_CENTER, 0, 0);

        lv_obj_add_event_cb(lock_screen_datetext_1_calendar, lock_screen_datetext_1_calendar_event_handler, LV_EVENT_ALL,NULL);
        lv_calendar_header_arrow_create(lock_screen_datetext_1_calendar);
        lv_obj_update_layout(scr);
    }
}

void lock_screen_datetext_1_calendar_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_calendar_date_t date;
        lv_calendar_get_pressed_date(obj,&date);
        char buf[16];
        lv_snprintf(buf,sizeof(buf),"%d/%02d/%02d", date.year, date.month,date.day);
        lv_label_set_text(guider_ui.lock_screen_datetext_1, buf);
        lv_obj_remove_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_TRANSP, 0);
        lv_obj_delete(lock_screen_datetext_1_calendar);
        lock_screen_datetext_1_calendar = NULL;
    }
}

void DeskTop_calendar_1_draw_part_begin_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_draw_task_t * draw_task = lv_event_get_param(e);
    lv_draw_label_dsc_t * label_dsc = lv_draw_task_get_label_dsc(draw_task);
    if(label_dsc == NULL || label_dsc->base.part != LV_PART_ITEMS) return;
    lv_draw_border_dsc_t * border_dsc = lv_draw_task_get_border_dsc(draw_task);
    lv_draw_fill_dsc_t * fill_dsc = lv_draw_task_get_fill_dsc(draw_task);

    if(label_dsc->base.id1 < 7) {
        if(label_dsc) label_dsc->color = lv_color_hex(0x0D3055);
        if(label_dsc) label_dsc->font = &lv_font_montserratMedium_12;
    } else if (lv_buttonmatrix_has_button_ctrl(obj, label_dsc->base.id1, LV_BUTTONMATRIX_CTRL_DISABLED)) {
        if(label_dsc) label_dsc->color = lv_color_hex(0xA9A2A2);
        if(label_dsc) label_dsc->font = &lv_font_montserratMedium_12;
        if(fill_dsc) fill_dsc->opa = 255;
        if(fill_dsc) fill_dsc->color = lv_color_hex(0xffffff);
    }
    if(lv_buttonmatrix_has_button_ctrl(obj, label_dsc->base.id1, LV_BUTTONMATRIX_CTRL_CUSTOM_1)) {
        if(label_dsc) label_dsc->color = lv_color_hex(0x0D3055);
        if(label_dsc) label_dsc->font = &lv_font_montserratMedium_12;
        if(fill_dsc) fill_dsc->opa = 255;
        if(fill_dsc) fill_dsc->color = lv_color_hex(0x01a2b1);
        if(border_dsc) border_dsc->opa = 255;
        if(border_dsc) border_dsc->width = 1;
        if(border_dsc) border_dsc->color = lv_color_hex(0xc0c0c0);
    }
    if(lv_buttonmatrix_has_button_ctrl(obj, label_dsc->base.id1, LV_BUTTONMATRIX_CTRL_CUSTOM_2)) {
        if(label_dsc) label_dsc->color = lv_color_hex(0x0D3055);
        if(label_dsc) label_dsc->font = &lv_font_montserratMedium_12;
        if(fill_dsc) fill_dsc->opa = 255;
        if(fill_dsc) fill_dsc->color = lv_color_hex(0x2195f6);
    } else {
    }
}

void DeskTop_calendar_1_event_handler(lv_event_t * e)
{
    lv_calendar_date_t date;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t * obj = lv_event_get_current_target(e);
        lv_calendar_get_pressed_date(obj,&date);
        lv_calendar_set_highlighted_dates(obj, &date, 1);
    }
}

extern int DeskTop_analog_clock_1_hour_value;
extern int DeskTop_analog_clock_1_min_value;
extern int DeskTop_analog_clock_1_sec_value;

void DeskTop_analog_clock_1_timer(lv_timer_t *timer)
{
    /* by rtc_clock_1s_timer_cb */
}
static lv_obj_t * DeskTop_datetext_1_calendar;

void DeskTop_datetext_1_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_FOCUSED) {
        char * s = lv_label_get_text(btn);
        if(DeskTop_datetext_1_calendar == NULL) {
            DeskTop_datetext_1_init_calendar(btn, s);
        }
    }
}

void DeskTop_datetext_1_init_calendar(lv_obj_t *obj, char * s)
{
    if (DeskTop_datetext_1_calendar == NULL) {
        lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        DeskTop_datetext_1_calendar = lv_calendar_create(lv_layer_top());
        lv_obj_t * scr = lv_obj_get_screen(obj);
        int32_t scr_height = lv_obj_get_height(scr);
        int32_t scr_width = lv_obj_get_width(scr);
        lv_obj_set_size(DeskTop_datetext_1_calendar, scr_width * 0.8, scr_height * 0.8);
        char * year = strtok(s, "/");
        char * month = strtok(NULL, "/");
        char * day = strtok(NULL, "/");
        lv_calendar_set_showed_date(DeskTop_datetext_1_calendar, atoi(year), atoi(month));
        lv_calendar_date_t highlighted_days[1];       /*Only its pointer will be saved so should be static*/
        highlighted_days[0].year = atoi(year);
        highlighted_days[0].month = atoi(month);
        highlighted_days[0].day = atoi(day);
        lv_calendar_set_highlighted_dates(DeskTop_datetext_1_calendar, highlighted_days, 1);
        lv_obj_align(DeskTop_datetext_1_calendar,LV_ALIGN_CENTER, 0, 0);

        lv_obj_add_event_cb(DeskTop_datetext_1_calendar, DeskTop_datetext_1_calendar_event_handler, LV_EVENT_ALL,NULL);
        lv_calendar_header_arrow_create(DeskTop_datetext_1_calendar);
        lv_obj_update_layout(scr);
    }
}

void DeskTop_datetext_1_calendar_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_calendar_date_t date;
        lv_calendar_get_pressed_date(obj,&date);
        char buf[16];
        lv_snprintf(buf,sizeof(buf),"%d/%02d/%02d", date.year, date.month,date.day);
        lv_label_set_text(guider_ui.DeskTop_datetext_1, buf);
        lv_obj_remove_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_TRANSP, 0);
        lv_obj_delete(DeskTop_datetext_1_calendar);
        DeskTop_datetext_1_calendar = NULL;
    }
}


extern int DeskTop_digital_clock_2_hour_value;
extern int DeskTop_digital_clock_2_min_value;
extern int DeskTop_digital_clock_2_sec_value;

void DeskTop_digital_clock_2_timer(lv_timer_t *timer)
{
    /* by rtc_clock_1s_timer_cb */
}
static lv_obj_t * DeskTop_datetext_2_calendar;

void DeskTop_datetext_2_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_FOCUSED) {
        char * s = lv_label_get_text(btn);
        if(DeskTop_datetext_2_calendar == NULL) {
            DeskTop_datetext_2_init_calendar(btn, s);
        }
    }
}

void DeskTop_datetext_2_init_calendar(lv_obj_t *obj, char * s)
{
    if (DeskTop_datetext_2_calendar == NULL) {
        lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        DeskTop_datetext_2_calendar = lv_calendar_create(lv_layer_top());
        lv_obj_t * scr = lv_obj_get_screen(obj);
        int32_t scr_height = lv_obj_get_height(scr);
        int32_t scr_width = lv_obj_get_width(scr);
        lv_obj_set_size(DeskTop_datetext_2_calendar, scr_width * 0.8, scr_height * 0.8);
        char * year = strtok(s, "/");
        char * month = strtok(NULL, "/");
        char * day = strtok(NULL, "/");
        lv_calendar_set_showed_date(DeskTop_datetext_2_calendar, atoi(year), atoi(month));
        lv_calendar_date_t highlighted_days[1];       /*Only its pointer will be saved so should be static*/
        highlighted_days[0].year = atoi(year);
        highlighted_days[0].month = atoi(month);
        highlighted_days[0].day = atoi(day);
        lv_calendar_set_highlighted_dates(DeskTop_datetext_2_calendar, highlighted_days, 1);
        lv_obj_align(DeskTop_datetext_2_calendar,LV_ALIGN_CENTER, 0, 0);

        lv_obj_add_event_cb(DeskTop_datetext_2_calendar, DeskTop_datetext_2_calendar_event_handler, LV_EVENT_ALL,NULL);
        lv_calendar_header_arrow_create(DeskTop_datetext_2_calendar);
        lv_obj_update_layout(scr);
    }
}

void DeskTop_datetext_2_calendar_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_calendar_date_t date;
        lv_calendar_get_pressed_date(obj,&date);
        char buf[16];
        lv_snprintf(buf,sizeof(buf),"%d/%02d/%02d", date.year, date.month,date.day);
        lv_label_set_text(guider_ui.DeskTop_datetext_2, buf);
        /* д��RTCӲ�� */
        rtc_set_date((uint8_t)(date.year % 100), (uint8_t)date.month, (uint8_t)date.day);
        lv_obj_remove_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_TRANSP, 0);
        lv_obj_delete(DeskTop_datetext_2_calendar);
        DeskTop_datetext_2_calendar = NULL;
    }
}


extern int DeskTop_digital_clock_1_hour_value;
extern int DeskTop_digital_clock_1_min_value;
extern int DeskTop_digital_clock_1_sec_value;

void DeskTop_digital_clock_1_timer(lv_timer_t *timer)
{
    /* by rtc_clock_1s_timer_cb */
}
static lv_obj_t * screen_navigation_bar_datetext_1_calendar;

void screen_navigation_bar_datetext_1_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_FOCUSED) {
        char * s = lv_label_get_text(btn);
        if(screen_navigation_bar_datetext_1_calendar == NULL) {
            screen_navigation_bar_datetext_1_init_calendar(btn, s);
        }
    }
}

void screen_navigation_bar_datetext_1_init_calendar(lv_obj_t *obj, char * s)
{
    if (screen_navigation_bar_datetext_1_calendar == NULL) {
        lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        screen_navigation_bar_datetext_1_calendar = lv_calendar_create(lv_layer_top());
        lv_obj_t * scr = lv_obj_get_screen(obj);
        int32_t scr_height = lv_obj_get_height(scr);
        int32_t scr_width = lv_obj_get_width(scr);
        lv_obj_set_size(screen_navigation_bar_datetext_1_calendar, scr_width * 0.8, scr_height * 0.8);
        char * year = strtok(s, "/");
        char * month = strtok(NULL, "/");
        char * day = strtok(NULL, "/");
        lv_calendar_set_showed_date(screen_navigation_bar_datetext_1_calendar, atoi(year), atoi(month));
        lv_calendar_date_t highlighted_days[1];       /*Only its pointer will be saved so should be static*/
        highlighted_days[0].year = atoi(year);
        highlighted_days[0].month = atoi(month);
        highlighted_days[0].day = atoi(day);
        lv_calendar_set_highlighted_dates(screen_navigation_bar_datetext_1_calendar, highlighted_days, 1);
        lv_obj_align(screen_navigation_bar_datetext_1_calendar,LV_ALIGN_CENTER, 0, 0);

        lv_obj_add_event_cb(screen_navigation_bar_datetext_1_calendar, screen_navigation_bar_datetext_1_calendar_event_handler, LV_EVENT_ALL,NULL);
        lv_calendar_header_arrow_create(screen_navigation_bar_datetext_1_calendar);
        lv_obj_update_layout(scr);
    }
}

void screen_navigation_bar_datetext_1_calendar_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_calendar_date_t date;
        lv_calendar_get_pressed_date(obj,&date);
        char buf[16];
        lv_snprintf(buf,sizeof(buf),"%d/%02d/%02d", date.year, date.month,date.day);
        lv_label_set_text(guider_ui.screen_navigation_bar_datetext_1, buf);
        lv_obj_remove_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_TRANSP, 0);
        lv_obj_delete(screen_navigation_bar_datetext_1_calendar);
        screen_navigation_bar_datetext_1_calendar = NULL;
    }
}


extern int screen_navigation_bar_digital_clock_1_hour_value;
extern int screen_navigation_bar_digital_clock_1_min_value;
extern int screen_navigation_bar_digital_clock_1_sec_value;

void screen_navigation_bar_digital_clock_1_timer(lv_timer_t *timer)
{
    /* by rtc_clock_1s_timer_cb */
}

extern int screen_notes_digital_clock_2_hour_value;
extern int screen_notes_digital_clock_2_min_value;
extern int screen_notes_digital_clock_2_sec_value;

void screen_notes_digital_clock_2_timer(lv_timer_t *timer)
{
    /* by rtc_clock_1s_timer_cb */
}

extern int screen_Signal_Proc_digital_clock_1_hour_value;
extern int screen_Signal_Proc_digital_clock_1_min_value;
extern int screen_Signal_Proc_digital_clock_1_sec_value;

void screen_Signal_Proc_digital_clock_1_timer(lv_timer_t *timer)
{
    /* by rtc_clock_1s_timer_cb */
}

extern int screen_yuanshen_digital_clock_1_hour_value;
extern int screen_yuanshen_digital_clock_1_min_value;
extern int screen_yuanshen_digital_clock_1_sec_value;

/* alarm state */
volatile uint8_t g_alarm_triggered = 0;
static lv_obj_t *alarm_popup = NULL;
/* countdown timer state */
int32_t g_countdown_h = 0, g_countdown_m = 0, g_countdown_s = 0;
int32_t g_countdown_preset_h = 0, g_countdown_preset_m = 0, g_countdown_preset_s = 0;
uint8_t g_countdown_state = 0;  /* 0=stopped, 1=running, 2=paused */
int32_t g_lap_h[64], g_lap_m[64], g_lap_s[64];
uint8_t g_lap_count = 0;

/* ---- navigation bar toggle buttons ---- */
uint8_t g_nav_btn_state = NAV_BTN_5;  /* btn_5 starts blue */
/* MAX_LAPS defined in widgets_init.h */

void screen_yuanshen_digital_clock_1_timer(lv_timer_t *timer)
{
    /* by rtc_clock_1s_timer_cb */
}



/* ================================================================
 *  1s RTC timer: updates all clock/date/weekday labels
 * ================================================================ */
static const char *week_str[] = {
    "",
    "\xE6\x98\x9F\xE6\x9C\x9F\xE4\xB8\x80",
    "\xE6\x98\x9F\xE6\x9C\x9F\xE4\xBA\x8C",
    "\xE6\x98\x9F\xE6\x9C\x9F\xE4\xB8\x89",
    "\xE6\x98\x9F\xE6\x9C\x9F\xE5\x9B\x9B",
    "\xE6\x98\x9F\xE6\x9C\x9F\xE4\xBA\x94",
    "\xE6\x98\x9F\xE6\x9C\x9F\xE5\x85\xAD",
    "\xE6\x98\x9F\xE6\x9C\x9F\xE6\x97\xA5"
};

/* RTC OBJECT OWNERSHIP USER PATCH BEGIN */
static bool rtc_object_on_active_screen(lv_obj_t *object, lv_obj_t *screen)
{
    if ((object == NULL) || (screen == NULL)) return false;
    if (lv_screen_active() != screen) return false;
    if (!lv_obj_is_valid(screen) || !lv_obj_is_valid(object)) return false;
    return lv_obj_get_screen(object) == screen;
}
/* RTC OBJECT OWNERSHIP USER PATCH END */

void rtc_clock_1s_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    /* TIM6 ISR drives software RTC, sync to hardware RTC every 60s */
    static uint16_t sync_cnt = 0;
    sync_cnt++;
    if (sync_cnt >= 60) {
        sync_cnt = 0;
        rtc_set_time(g_rtc_hour, g_rtc_min, g_rtc_sec);
        rtc_set_date(g_rtc_year, g_rtc_month, g_rtc_day);
    }

    /* digital clocks */
    if (rtc_object_on_active_screen(guider_ui.lock_screen_digital_clock_1,
                                    guider_ui.lock_screen))
        lv_label_set_text_fmt(guider_ui.lock_screen_digital_clock_1,
            "%d:%02d:%02d", g_rtc_hour, g_rtc_min, g_rtc_sec);
    if (rtc_object_on_active_screen(guider_ui.DeskTop_digital_clock_2,
                                    guider_ui.DeskTop))
        lv_label_set_text_fmt(guider_ui.DeskTop_digital_clock_2,
            "%d:%02d", g_rtc_hour, g_rtc_min);
    if (rtc_object_on_active_screen(guider_ui.DeskTop_digital_clock_1,
                                    guider_ui.DeskTop))
        lv_label_set_text_fmt(guider_ui.DeskTop_digital_clock_1,
            "%d:%02d:%02d", g_rtc_hour, g_rtc_min, g_rtc_sec);
    if (rtc_object_on_active_screen(
            guider_ui.screen_navigation_bar_digital_clock_1,
            guider_ui.screen_navigation_bar))
        lv_label_set_text_fmt(guider_ui.screen_navigation_bar_digital_clock_1,
            "%d:%02d", g_rtc_hour, g_rtc_min);
    if (rtc_object_on_active_screen(guider_ui.screen_notes_digital_clock_2,
                                    guider_ui.screen_notes))
        lv_label_set_text_fmt(guider_ui.screen_notes_digital_clock_2,
            "%d:%02d", g_rtc_hour, g_rtc_min);
    if (rtc_object_on_active_screen(
            guider_ui.screen_Signal_Proc_digital_clock_1,
            guider_ui.screen_Signal_Proc))
        lv_label_set_text_fmt(guider_ui.screen_Signal_Proc_digital_clock_1,
            "%d:%02d", g_rtc_hour, g_rtc_min);
    if (rtc_object_on_active_screen(
            guider_ui.screen_yuanshen_digital_clock_1,
            guider_ui.screen_yuanshen))
        lv_label_set_text_fmt(guider_ui.screen_yuanshen_digital_clock_1,
            "%d:%02d", g_rtc_hour, g_rtc_min);
    /* STAGE6 UI USER PATCH BEGIN: new application clocks. */
    if (rtc_object_on_active_screen(
            guider_ui.screen_calculator_digital_clock_1,
            guider_ui.screen_calculator))
        lv_label_set_text_fmt(guider_ui.screen_calculator_digital_clock_1,
            "%d:%02d", g_rtc_hour, g_rtc_min);
    if (rtc_object_on_active_screen(
            guider_ui.screen_piano_digital_clock_1,
            guider_ui.screen_piano))
        lv_label_set_text_fmt(guider_ui.screen_piano_digital_clock_1,
            "%d:%02d", g_rtc_hour, g_rtc_min);
    if (rtc_object_on_active_screen(
            guider_ui.screen_drawing_digital_clock_1,
            guider_ui.screen_drawing))
        lv_label_set_text_fmt(guider_ui.screen_drawing_digital_clock_1,
            "%d:%02d", g_rtc_hour, g_rtc_min);
    /* STAGE6 UI USER PATCH END */
    /* MEDIA NAVIGATION USER PATCH BEGIN: keep media clocks synchronized. */
    if (rtc_object_on_active_screen(
            guider_ui.screen_image_viewer_digital_clock_2,
            guider_ui.screen_image_viewer))
        lv_label_set_text_fmt(
            guider_ui.screen_image_viewer_digital_clock_2,
            "%d:%02d", g_rtc_hour, g_rtc_min);
    if (rtc_object_on_active_screen(
            guider_ui.screen_avi_player_digital_clock_2,
            guider_ui.screen_avi_player))
        lv_label_set_text_fmt(
            guider_ui.screen_avi_player_digital_clock_2,
            "%d:%02d", g_rtc_hour, g_rtc_min);
    /* MEDIA NAVIGATION USER PATCH END */
    /* MUSIC APP USER PATCH BEGIN: keep music navigation clock synchronized */
    if (rtc_object_on_active_screen(guider_ui.screen_music_digital_clock,
                                    guider_ui.screen_music))
        lv_label_set_text_fmt(guider_ui.screen_music_digital_clock,
            "%d:%02d", g_rtc_hour, g_rtc_min);
    /* MUSIC APP USER PATCH END */

    /* analog clock */
    if (rtc_object_on_active_screen(guider_ui.DeskTop_analog_clock_1,
                                    guider_ui.DeskTop)) {
        lv_scale_set_line_needle_value(guider_ui.DeskTop_analog_clock_1,
            guider_ui.DeskTop_analog_clock_1_hour_needle, 30,
            (g_rtc_hour % 12) * 5 + g_rtc_min / 12);
        lv_scale_set_line_needle_value(guider_ui.DeskTop_analog_clock_1,
            guider_ui.DeskTop_analog_clock_1_min_needle, 40,
            g_rtc_min);
        lv_scale_set_line_needle_value(guider_ui.DeskTop_analog_clock_1,
            guider_ui.DeskTop_analog_clock_1_sec_needle, 60,
            g_rtc_sec);
    }

    /* date labels */
    if (rtc_object_on_active_screen(guider_ui.DeskTop_datetext_1,
                                    guider_ui.DeskTop))
        lv_label_set_text_fmt(guider_ui.DeskTop_datetext_1,
            "20%02d/%02d/%02d", g_rtc_year, g_rtc_month, g_rtc_day);
    if (rtc_object_on_active_screen(guider_ui.DeskTop_datetext_2,
                                    guider_ui.DeskTop))
        lv_label_set_text_fmt(guider_ui.DeskTop_datetext_2,
            "20%02d/%02d/%02d", g_rtc_year, g_rtc_month, g_rtc_day);
    /* DESKTOP CALENDAR USER PATCH BEGIN: replace GUI test date on boot. */
    if (rtc_object_on_active_screen(guider_ui.DeskTop_calendar_1,
                                    guider_ui.DeskTop))
    {
        const lv_calendar_date_t *today = lv_calendar_get_today_date(
            guider_ui.DeskTop_calendar_1);
        uint32_t rtc_year = 2000U + (uint32_t)g_rtc_year;
        if ((today == NULL) || (today->year != rtc_year) ||
            (today->month != (uint32_t)g_rtc_month) ||
            (today->day != (uint32_t)g_rtc_day))
        {
            lv_calendar_set_today_date(guider_ui.DeskTop_calendar_1,
                                       rtc_year,
                                       (uint32_t)g_rtc_month,
                                       (uint32_t)g_rtc_day);
            lv_calendar_set_showed_date(guider_ui.DeskTop_calendar_1,
                                        rtc_year,
                                        (uint32_t)g_rtc_month);
            /* LVGL asserts even when date_num is zero, so keep the pointer valid. */
            lv_calendar_set_highlighted_dates(
                guider_ui.DeskTop_calendar_1,
                DeskTop_calendar_1_highlihted_days, 0U);
        }
    }
    /* DESKTOP CALENDAR USER PATCH END */
    if (rtc_object_on_active_screen(guider_ui.lock_screen_datetext_1,
                                    guider_ui.lock_screen))
        lv_label_set_text_fmt(guider_ui.lock_screen_datetext_1,
            "20%02d/%02d/%02d", g_rtc_year, g_rtc_month, g_rtc_day);
    if (rtc_object_on_active_screen(
            guider_ui.screen_navigation_bar_datetext_1,
            guider_ui.screen_navigation_bar))
        lv_label_set_text_fmt(guider_ui.screen_navigation_bar_datetext_1,
            "20%02d/%02d/%02d", g_rtc_year, g_rtc_month, g_rtc_day);

    /* weekday */
    if (rtc_object_on_active_screen(
            guider_ui.screen_navigation_bar_label_week,
            guider_ui.screen_navigation_bar)) {
        uint8_t wk = g_rtc_week;
        if (wk < 1) wk = 1;
        if (wk > 7) wk = 7;
        lv_label_set_text(guider_ui.screen_navigation_bar_label_week, week_str[wk]);
    }
    /* ---- alarm label (navigation bar: nearest) ---- */
    if (rtc_object_on_active_screen(guider_ui.DeskTop_label_23,
                                    guider_ui.DeskTop)) {
        if (g_alarm_hour != 255)
            lv_label_set_text_fmt(guider_ui.DeskTop_label_23,
                "\xe9\x97\xb9\xe9\x92\x9f\xef\xbc\x9a%02d:%02d",
                g_alarm_hour, g_alarm_min);
        else
            lv_label_set_text(guider_ui.DeskTop_label_23,
                "\xe9\x97\xb9\xe9\x92\x9f\xef\xbc\x9a--:--");
    }

    /* ---- alarm list label (settings: all active alarms) ---- */
    if (rtc_object_on_active_screen(guider_ui.DeskTop_label_alarm,
                                    guider_ui.DeskTop)) {
        static char alarm_buf[256];
        alarm_buf[0] = '\0';
        int active = 0;
        /* alarms are already sorted by alarm_update_nearest */
        for (int i = 0; i < MAX_ALARMS; i++) {
            if (g_alarms[i].hour == 255) continue;
            active++;
            char line[32];
            snprintf(line, sizeof(line), "%02d:%02d\n", g_alarms[i].hour, g_alarms[i].min);
            strncat(alarm_buf, line, sizeof(alarm_buf) - strlen(alarm_buf) - 1);
        }
        if (active == 0)
            lv_label_set_text(guider_ui.DeskTop_label_alarm,
                "\xe6\x9c\xaa\xe8\xae\xbe\xe7\xbd\xae\xe9\x97\xb9\xe9\x92\x9f");
        else
            lv_label_set_text(guider_ui.DeskTop_label_alarm, alarm_buf);
    }
    /* ---- alarm trigger check ---- */
    if (g_alarm_hour != 255 && g_alarm_triggered == 0) {
        if (g_rtc_hour == g_alarm_hour && g_rtc_min == g_alarm_min && g_rtc_sec == 0) {
            g_alarm_triggered = 1;
            alarm_popup_show();
        }
    }
    if (g_alarm_triggered && (g_rtc_min != g_alarm_min || g_rtc_hour != g_alarm_hour)) {
        g_alarm_triggered = 0;
    }

    /* ---- countdown timer tick ---- */
    if (g_countdown_state == 1) {
        if (g_countdown_s > 0) {
            g_countdown_s--;
        } else if (g_countdown_m > 0) {
            g_countdown_m--;
            g_countdown_s = 59;
        } else if (g_countdown_h > 0) {
            g_countdown_h--;
            g_countdown_m = 59;
            g_countdown_s = 59;
        } else {
            /* countdown finished */
            g_countdown_state = 0;
        }
    }
    /* update timekeeping display */
    if (rtc_object_on_active_screen(guider_ui.DeskTop_label_timeupdate,
                                    guider_ui.DeskTop)) {
        lv_label_set_text_fmt(guider_ui.DeskTop_label_timeupdate,
            "%02d : %02d : %02d", g_countdown_h, g_countdown_m, g_countdown_s);
    }
    /* update label_24 in navigation bar */
    if (rtc_object_on_active_screen(guider_ui.DeskTop_label_24,
                                    guider_ui.DeskTop)) {
        if (g_countdown_state >= 1)
            lv_label_set_text_fmt(guider_ui.DeskTop_label_24,
                "\xe7\xa7\x92\xe8\xa1\xa8\xef\xbc\x9a%02d:%02d:%02d",
                g_countdown_h, g_countdown_m, g_countdown_s);
        else
            lv_label_set_text(guider_ui.DeskTop_label_24,
                "\xe7\xa7\x92\xe8\xa1\xa8\xef\xbc\x9a--:--:--");
    }
}  /* end rtc_clock_1s_timer_cb */

/* ================================================================
 *  Alarm popup: shown on top layer when alarm triggers
 * ================================================================ */
static void alarm_popup_close_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (alarm_popup && lv_obj_is_valid(alarm_popup)) {
            lv_obj_delete(alarm_popup);
            alarm_popup = NULL;
        }
        lv_obj_remove_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_TRANSP, 0);
    }
}

void alarm_popup_show(void)
{
    if (alarm_popup) return;  /* already showing */

    lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_50, 0);
    lv_obj_set_style_bg_color(lv_layer_top(), lv_color_hex(0x000000), 0);

    alarm_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(alarm_popup, 400, 200);
    lv_obj_align(alarm_popup, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(alarm_popup, 20, 0);
    lv_obj_set_style_bg_color(alarm_popup, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(alarm_popup, 2, 0);
    lv_obj_set_style_border_color(alarm_popup, lv_color_hex(0xff6600), 0);

    /* Title */
    lv_obj_t *title = lv_label_create(alarm_popup);
    lv_label_set_text(title, "\xe2\x8f\xb0 \xe9\x97\xb9\xe9\x92\x9f");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title, &lv_font_myyh_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xff6600), 0);
    lv_obj_set_style_text_font(title, &lv_font_myyh_16, 0);

    /* Time */
    lv_obj_t *time_label = lv_label_create(alarm_popup);
    lv_label_set_text_fmt(time_label, "%02d:%02d", g_alarm_hour, g_alarm_min);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_text_font(time_label, &lv_font_montserratMedium_30, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x333333), 0);

    /* Dismiss button */
    lv_obj_t *btn = lv_button_create(alarm_popup);
    lv_obj_set_size(btn, 120, 40);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xff6600), 0);
    lv_obj_set_style_radius(btn, 20, 0);
    lv_obj_add_event_cb(btn, alarm_popup_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "\xe5\x85\xb3\xe9\x97\xad");
    lv_obj_set_style_text_font(btn_label, &lv_font_myyh_16, 0);
    lv_obj_center(btn_label);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xffffff), 0);
}
