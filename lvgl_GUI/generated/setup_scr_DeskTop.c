/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"
#include "norflash.h"

/* MUSIC APP USER PATCH BEGIN: GUI Guider generated music asset */
LV_IMAGE_DECLARE(_music_RGB565A8_80x80);
/* MUSIC APP USER PATCH END */


lv_calendar_date_t DeskTop_calendar_1_today;
lv_calendar_date_t DeskTop_calendar_1_highlihted_days[1];
int DeskTop_analog_clock_1_hour_value = 3;
int DeskTop_analog_clock_1_min_value = 20;
int DeskTop_analog_clock_1_sec_value = 50;
int DeskTop_digital_clock_2_min_value = 8;
int DeskTop_digital_clock_2_hour_value = 15;
int DeskTop_digital_clock_2_sec_value = 6;
int DeskTop_digital_clock_1_min_value = 8;
int DeskTop_digital_clock_1_hour_value = 15;
int DeskTop_digital_clock_1_sec_value = 6;
void setup_scr_DeskTop(lv_ui *ui)
{
    //Write codes DeskTop
    ui->DeskTop = lv_obj_create(NULL);
    lv_obj_set_size(ui->DeskTop, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->DeskTop, LV_SCROLLBAR_MODE_OFF);

    //Write style for DeskTop, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(ui->DeskTop, norflash_get_wallpaper_dsc(), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_opa(ui->DeskTop, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_recolor_opa(ui->DeskTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_tileview_DeskTop
    ui->DeskTop_tileview_DeskTop = lv_tileview_create(ui->DeskTop);
    lv_obj_set_pos(ui->DeskTop_tileview_DeskTop, 12, 35);
    lv_obj_set_size(ui->DeskTop_tileview_DeskTop, 1000, 565);
    lv_obj_set_scrollbar_mode(ui->DeskTop_tileview_DeskTop, LV_SCROLLBAR_MODE_OFF);
    ui->DeskTop_tileview_DeskTop_tile = lv_tileview_add_tile(ui->DeskTop_tileview_DeskTop, 0, 0, LV_DIR_RIGHT);
    ui->DeskTop_tileview_DeskTop_tile_1 = lv_tileview_add_tile(ui->DeskTop_tileview_DeskTop, 1, 0, LV_DIR_LEFT);

    //Write style for DeskTop_tileview_DeskTop, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_tileview_DeskTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_tileview_DeskTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_tileview_DeskTop, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_tileview_DeskTop, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_tileview_DeskTop, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_tileview_DeskTop, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_tileview_DeskTop, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_tileview_DeskTop, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes DeskTop_img_noteAPP
    ui->DeskTop_img_noteAPP = lv_image_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_img_noteAPP, 163, 255);
    lv_obj_set_size(ui->DeskTop_img_noteAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_noteAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_noteAPP, &_note_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_noteAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_noteAPP, 0);

    //Write style for DeskTop_img_noteAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_noteAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_noteAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_8
    ui->DeskTop_label_8 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_label_8, 448, 527);
    lv_obj_set_size(ui->DeskTop_label_8, 50, 32);
    lv_label_set_text(ui->DeskTop_label_8, " " LV_SYMBOL_HOME " ");
    lv_label_set_long_mode(ui->DeskTop_label_8, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_8, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_8, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_8, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_8, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_8, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_7
    ui->DeskTop_label_7 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_label_7, 479, 530);
    lv_obj_set_size(ui->DeskTop_label_7, 50, 32);
    lv_label_set_text(ui->DeskTop_label_7, "" LV_SYMBOL_STOP " ");
    lv_label_set_long_mode(ui->DeskTop_label_7, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_7, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_7, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_7, &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_7, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_7, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_calendar_1
    ui->DeskTop_calendar_1 = lv_calendar_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_calendar_1, 576, 31);
    lv_obj_set_size(ui->DeskTop_calendar_1, 400, 300);
    DeskTop_calendar_1_today.year = 2026;
    DeskTop_calendar_1_today.month = 6;
    DeskTop_calendar_1_today.day = 8;
    lv_calendar_set_today_date(ui->DeskTop_calendar_1, DeskTop_calendar_1_today.year, DeskTop_calendar_1_today.month, DeskTop_calendar_1_today.day);
    lv_calendar_set_showed_date(ui->DeskTop_calendar_1, DeskTop_calendar_1_today.year, DeskTop_calendar_1_today.month);
    DeskTop_calendar_1_highlihted_days[0].year = 2026;
    DeskTop_calendar_1_highlihted_days[0].month = 6;
    DeskTop_calendar_1_highlihted_days[0].day = 9;
    lv_calendar_set_highlighted_dates(ui->DeskTop_calendar_1, DeskTop_calendar_1_highlihted_days, 1);
    lv_obj_t *DeskTop_calendar_1_header = lv_calendar_header_arrow_create(ui->DeskTop_calendar_1);
    lv_calendar_t *DeskTop_calendar_1 = (lv_calendar_t *)ui->DeskTop_calendar_1;
    lv_obj_add_event_cb(lv_calendar_get_btnmatrix(ui->DeskTop_calendar_1), DeskTop_calendar_1_draw_part_begin_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_event_cb(ui->DeskTop_calendar_1, DeskTop_calendar_1_event_handler, LV_EVENT_ALL, NULL);

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_calendar_1_main_main_default
    static lv_style_t style_DeskTop_calendar_1_main_main_default;
    ui_init_style(&style_DeskTop_calendar_1_main_main_default);

    lv_style_set_border_width(&style_DeskTop_calendar_1_main_main_default, 1);
    lv_style_set_border_opa(&style_DeskTop_calendar_1_main_main_default, 255);
    lv_style_set_border_color(&style_DeskTop_calendar_1_main_main_default, lv_color_hex(0xc0c0c0));
    lv_style_set_border_side(&style_DeskTop_calendar_1_main_main_default, LV_BORDER_SIDE_FULL);
    lv_style_set_radius(&style_DeskTop_calendar_1_main_main_default, 0);
    lv_style_set_bg_opa(&style_DeskTop_calendar_1_main_main_default, 255);
    lv_style_set_bg_color(&style_DeskTop_calendar_1_main_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_DeskTop_calendar_1_main_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_shadow_width(&style_DeskTop_calendar_1_main_main_default, 0);
    lv_obj_add_style(ui->DeskTop_calendar_1, &style_DeskTop_calendar_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_calendar_1_extra_header_main_default
    static lv_style_t style_DeskTop_calendar_1_extra_header_main_default;
    ui_init_style(&style_DeskTop_calendar_1_extra_header_main_default);

    lv_style_set_text_color(&style_DeskTop_calendar_1_extra_header_main_default, lv_color_hex(0xffffff));
    lv_style_set_text_font(&style_DeskTop_calendar_1_extra_header_main_default, &lv_font_myyh_12);
    lv_style_set_text_opa(&style_DeskTop_calendar_1_extra_header_main_default, 255);
    lv_style_set_bg_opa(&style_DeskTop_calendar_1_extra_header_main_default, 255);
    lv_style_set_bg_color(&style_DeskTop_calendar_1_extra_header_main_default, lv_color_hex(0x2195f6));
    lv_style_set_bg_grad_dir(&style_DeskTop_calendar_1_extra_header_main_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(DeskTop_calendar_1_header, &style_DeskTop_calendar_1_extra_header_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_calendar_1_main_items_default
    static lv_style_t style_DeskTop_calendar_1_main_items_default;
    ui_init_style(&style_DeskTop_calendar_1_main_items_default);

    lv_style_set_bg_opa(&style_DeskTop_calendar_1_main_items_default, 255);
    lv_style_set_bg_color(&style_DeskTop_calendar_1_main_items_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_DeskTop_calendar_1_main_items_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_DeskTop_calendar_1_main_items_default, 1);
    lv_style_set_border_opa(&style_DeskTop_calendar_1_main_items_default, 255);
    lv_style_set_border_color(&style_DeskTop_calendar_1_main_items_default, lv_color_hex(0xc0c0c0));
    lv_style_set_border_side(&style_DeskTop_calendar_1_main_items_default, LV_BORDER_SIDE_FULL);
    lv_style_set_text_color(&style_DeskTop_calendar_1_main_items_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_DeskTop_calendar_1_main_items_default, &lv_font_myyh_12);
    lv_style_set_text_opa(&style_DeskTop_calendar_1_main_items_default, 255);
    lv_obj_add_style(lv_calendar_get_btnmatrix(ui->DeskTop_calendar_1), &style_DeskTop_calendar_1_main_items_default, LV_PART_ITEMS|LV_STATE_DEFAULT);

    //Write codes DeskTop_img_settingAPP
    ui->DeskTop_img_settingAPP = lv_image_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_img_settingAPP, 63, 255);
    lv_obj_set_size(ui->DeskTop_img_settingAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_settingAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_settingAPP, &_setting_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_settingAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_settingAPP, 0);

    //Write style for DeskTop_img_settingAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_settingAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_settingAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_analog_clock_1
    static bool DeskTop_analog_clock_1_timer_enabled = false;
    static const char * DeskTop_analog_clock_1_hour_ticks[] = {"12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", NULL};
    ui->DeskTop_analog_clock_1 = lv_scale_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_analog_clock_1, 292, 23);
    lv_obj_set_size(ui->DeskTop_analog_clock_1, 200, 200);
    lv_scale_set_mode(ui->DeskTop_analog_clock_1, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_angle_range(ui->DeskTop_analog_clock_1, 360U);
    lv_scale_set_range(ui->DeskTop_analog_clock_1, 0U, 60U);
    lv_scale_set_rotation(ui->DeskTop_analog_clock_1, 270U);
    lv_obj_set_style_radius(ui->DeskTop_analog_clock_1, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(ui->DeskTop_analog_clock_1, true, LV_PART_MAIN);
    lv_obj_set_style_arc_width(ui->DeskTop_analog_clock_1, 0, LV_PART_MAIN);

    lv_scale_set_total_tick_count(ui->DeskTop_analog_clock_1, 61);

    lv_scale_set_major_tick_every(ui->DeskTop_analog_clock_1, 5);
    lv_scale_set_text_src(ui->DeskTop_analog_clock_1, DeskTop_analog_clock_1_hour_ticks);
    lv_obj_update_layout(ui->DeskTop_analog_clock_1);
    ui->DeskTop_analog_clock_1_hour_needle = lv_line_create(ui->DeskTop_analog_clock_1);
    lv_obj_set_style_line_width(ui->DeskTop_analog_clock_1_hour_needle, 4, LV_PART_MAIN);
    lv_obj_set_style_line_color(ui->DeskTop_analog_clock_1_hour_needle, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_line_rounded(ui->DeskTop_analog_clock_1_hour_needle, true, LV_PART_MAIN);
    lv_scale_set_line_needle_value(ui->DeskTop_analog_clock_1, ui->DeskTop_analog_clock_1_hour_needle, 30, DeskTop_analog_clock_1_hour_value * 5);
    ui->DeskTop_analog_clock_1_min_needle = lv_line_create(ui->DeskTop_analog_clock_1);
    lv_obj_set_style_line_width(ui->DeskTop_analog_clock_1_min_needle, 3, LV_PART_MAIN);
    lv_obj_set_style_line_color(ui->DeskTop_analog_clock_1_min_needle, lv_color_hex(0x004eff), LV_PART_MAIN);
    lv_obj_set_style_line_rounded(ui->DeskTop_analog_clock_1_min_needle, true, LV_PART_MAIN);
    lv_scale_set_line_needle_value(ui->DeskTop_analog_clock_1, ui->DeskTop_analog_clock_1_min_needle, 40, DeskTop_analog_clock_1_min_value);
    ui->DeskTop_analog_clock_1_sec_needle = lv_line_create(ui->DeskTop_analog_clock_1);
    lv_obj_set_style_line_width(ui->DeskTop_analog_clock_1_sec_needle, 2, LV_PART_MAIN);
    lv_obj_set_style_line_color(ui->DeskTop_analog_clock_1_sec_needle, lv_color_hex(0xffa300), LV_PART_MAIN);
    lv_obj_set_style_line_rounded(ui->DeskTop_analog_clock_1_sec_needle, true, LV_PART_MAIN);
    lv_scale_set_line_needle_value(ui->DeskTop_analog_clock_1, ui->DeskTop_analog_clock_1_sec_needle, 60, DeskTop_analog_clock_1_sec_value);
    // create timer
    if (!DeskTop_analog_clock_1_timer_enabled) {
        lv_timer_create(DeskTop_analog_clock_1_timer, 1000, NULL);
        DeskTop_analog_clock_1_timer_enabled = true;
    }

    //Write style for DeskTop_analog_clock_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_analog_clock_1, 82, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_analog_clock_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_analog_clock_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_analog_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_analog_clock_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->DeskTop_analog_clock_1, lv_color_hex(0x000000), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_analog_clock_1, &lv_font_montserratMedium_14, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_analog_clock_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_analog_clock_1, 2, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_analog_clock_1, lv_color_hex(0x000000), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_analog_clock_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_analog_clock_1, true, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_length(ui->DeskTop_analog_clock_1, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for DeskTop_analog_clock_1, Part: LV_PART_ITEMS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_line_width(ui->DeskTop_analog_clock_1, 2, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_analog_clock_1, lv_color_hex(0x000000), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_analog_clock_1, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_analog_clock_1, true, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_length(ui->DeskTop_analog_clock_1, 6, LV_PART_ITEMS|LV_STATE_DEFAULT);

    //Write codes DeskTop_datetext_1
    ui->DeskTop_datetext_1 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_datetext_1, 4, 110);
    lv_obj_set_size(ui->DeskTop_datetext_1, 300, 50);
    lv_label_set_text(ui->DeskTop_datetext_1, "2026/05/09");
    lv_obj_set_style_text_align(ui->DeskTop_datetext_1, LV_TEXT_ALIGN_CENTER, 0);    /* CLICKABLE removed - date synced from RTC */
    lv_obj_add_event_cb(ui->DeskTop_datetext_1, DeskTop_datetext_1_event_handler, LV_EVENT_ALL, NULL);

    //Write style for DeskTop_datetext_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_datetext_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_datetext_1, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_datetext_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_datetext_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_datetext_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_datetext_1, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_digital_clock_2
    static bool DeskTop_digital_clock_2_timer_enabled = false;
    ui->DeskTop_digital_clock_2 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_digital_clock_2, 48, 57);
    lv_obj_set_size(ui->DeskTop_digital_clock_2, 200, 50);
    lv_label_set_text(ui->DeskTop_digital_clock_2, "15:08");
    if (!DeskTop_digital_clock_2_timer_enabled) {
        lv_timer_create(DeskTop_digital_clock_2_timer, 1000, NULL);
        DeskTop_digital_clock_2_timer_enabled = true;
    }

    //Write style for DeskTop_digital_clock_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_digital_clock_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_digital_clock_2, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_digital_clock_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_digital_clock_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_digital_clock_2, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_img_wetherAPP
    ui->DeskTop_img_wetherAPP = lv_image_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_img_wetherAPP, 263, 255);
    lv_obj_set_size(ui->DeskTop_img_wetherAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_wetherAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_wetherAPP, &_weather_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_wetherAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_wetherAPP, 0);

    //Write style for DeskTop_img_wetherAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_wetherAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_wetherAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_img_edgeAPP
    ui->DeskTop_img_edgeAPP = lv_image_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_img_edgeAPP, 60, 377);
    lv_obj_set_size(ui->DeskTop_img_edgeAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_edgeAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_edgeAPP, &_Edge_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_edgeAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_edgeAPP, 0);

    //Write style for DeskTop_img_edgeAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_edgeAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_edgeAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_img_calendarAPP
    ui->DeskTop_img_calendarAPP = lv_image_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_img_calendarAPP, 160, 377);
    lv_obj_set_size(ui->DeskTop_img_calendarAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_calendarAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_calendarAPP, &_calendar_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_calendarAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_calendarAPP, 0);

    //Write style for DeskTop_img_calendarAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_calendarAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_calendarAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_img_albumAPP
    ui->DeskTop_img_albumAPP = lv_image_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_img_albumAPP, 363, 255);
    lv_obj_set_size(ui->DeskTop_img_albumAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_albumAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_albumAPP, &_photo_album_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_albumAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_albumAPP, 0);

    //Write style for DeskTop_img_albumAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_albumAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_albumAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_11
    ui->DeskTop_label_11 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_label_11, 60, 343);
    lv_obj_set_size(ui->DeskTop_label_11, 80, 30);
    lv_label_set_text(ui->DeskTop_label_11, "\xE8\xAE\xBE\xE7\xBD\xAE");
    lv_label_set_long_mode(ui->DeskTop_label_11, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_11, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_11, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_11, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_12
    ui->DeskTop_label_12 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_label_12, 160, 342);
    lv_obj_set_size(ui->DeskTop_label_12, 80, 30);
    lv_label_set_text(ui->DeskTop_label_12, "\xE8\xAE\xB0\xE4\xBA\x8B\xE6\x9C\xAC");
    lv_label_set_long_mode(ui->DeskTop_label_12, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_12, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_12, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_12, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_12, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_13
    ui->DeskTop_label_13 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_label_13, 260, 342);
    lv_obj_set_size(ui->DeskTop_label_13, 80, 30);
    lv_label_set_text(ui->DeskTop_label_13, "\xE5\xA4\xA9\xE6\xB0\x94\xE9\xA2\x84\xE6\x8A\xA5");
    lv_label_set_long_mode(ui->DeskTop_label_13, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_13, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_13, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_13, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_13, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_14
    ui->DeskTop_label_14 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_label_14, 362, 341);
    lv_obj_set_size(ui->DeskTop_label_14, 80, 30);
    lv_label_set_text(ui->DeskTop_label_14, "\xE7\x9B\xB8\xE5\x86\x8C");
    lv_label_set_long_mode(ui->DeskTop_label_14, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_14, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_14, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_14, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_14, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_14, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_15
    ui->DeskTop_label_15 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_label_15, 57, 466);
    lv_obj_set_size(ui->DeskTop_label_15, 80, 30);
    lv_label_set_text(ui->DeskTop_label_15, "\xE6\xB5\x8F\xE8\xA7\x88\xE5\x99\xA8");
    lv_label_set_long_mode(ui->DeskTop_label_15, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_15, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_15, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_15, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_15, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_15, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_16
    ui->DeskTop_label_16 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile);
    lv_obj_set_pos(ui->DeskTop_label_16, 160, 465);
    lv_obj_set_size(ui->DeskTop_label_16, 80, 30);
    lv_label_set_text(ui->DeskTop_label_16, "\xE6\x97\xA5\xE5\x8E\x86");
    lv_label_set_long_mode(ui->DeskTop_label_16, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_16, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_16, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_16, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_16, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_16, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);



    //Write codes DeskTop_label_10
    ui->DeskTop_label_10 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_label_10, 459, 531);
    lv_obj_set_size(ui->DeskTop_label_10, 50, 32);
    lv_label_set_text(ui->DeskTop_label_10, " " LV_SYMBOL_HOME " ");
    lv_label_set_long_mode(ui->DeskTop_label_10, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_10, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_10, &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_10, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_9
    ui->DeskTop_label_9 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_label_9, 486, 528);
    lv_obj_set_size(ui->DeskTop_label_9, 50, 32);
    lv_label_set_text(ui->DeskTop_label_9, "" LV_SYMBOL_STOP " ");
    lv_label_set_long_mode(ui->DeskTop_label_9, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_9, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_9, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_9, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_9, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_9, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_img_yuanshenAPP
    ui->DeskTop_img_yuanshenAPP = lv_image_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_img_yuanshenAPP, 50, 35);
    lv_obj_set_size(ui->DeskTop_img_yuanshenAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_yuanshenAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_yuanshenAPP, &_yuanshen_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_yuanshenAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_yuanshenAPP, 0);

    //Write style for DeskTop_img_yuanshenAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_yuanshenAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_yuanshenAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_img_alarmAPP
    ui->DeskTop_img_alarmAPP = lv_image_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_img_alarmAPP, 150, 35);
    lv_obj_set_size(ui->DeskTop_img_alarmAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_alarmAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_alarmAPP, &_alarm_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_alarmAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_alarmAPP, 0);

    //Write style for DeskTop_img_alarmAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_alarmAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_alarmAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_img_calculatorAPP
    ui->DeskTop_img_calculatorAPP = lv_image_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_img_calculatorAPP, 250, 35);
    lv_obj_set_size(ui->DeskTop_img_calculatorAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_calculatorAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_calculatorAPP, &_calculator_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_calculatorAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_calculatorAPP, 0);

    //Write style for DeskTop_img_calculatorAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_calculatorAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_calculatorAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_img_folderAPP
    ui->DeskTop_img_folderAPP = lv_image_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_img_folderAPP, 350, 35);
    lv_obj_set_size(ui->DeskTop_img_folderAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_folderAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_folderAPP, &_folder_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_folderAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_folderAPP, 0);

    //Write style for DeskTop_img_folderAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_folderAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_folderAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* MUSIC APP USER PATCH BEGIN: generated-style desktop page 2 entry */
    //Write codes DeskTop_img_musicAPP
    ui->DeskTop_img_musicAPP = lv_image_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_img_musicAPP, 450, 35);
    lv_obj_set_size(ui->DeskTop_img_musicAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_musicAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_musicAPP, &_music_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_musicAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_musicAPP, 0);

    //Write style for DeskTop_img_musicAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_musicAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_musicAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    /* MUSIC APP USER PATCH END */

    /* STAGE6 UI USER PATCH BEGIN: generated-style desktop entries */
    ui->DeskTop_img_draw = lv_image_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_img_draw, 550, 35);
    lv_obj_set_size(ui->DeskTop_img_draw, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_draw, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_draw, &_draw_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_draw, 40, 40);
    lv_image_set_rotation(ui->DeskTop_img_draw, 0);
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_draw, 0,
        LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_draw, 255,
        LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->DeskTop_img_piano = lv_image_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_img_piano, 650, 35);
    lv_obj_set_size(ui->DeskTop_img_piano, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_piano, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_piano, &_piano_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_piano, 40, 40);
    lv_image_set_rotation(ui->DeskTop_img_piano, 0);
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_piano, 0,
        LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_piano, 255,
        LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->DeskTop_img_nes = lv_image_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_img_nes, 750, 35);
    lv_obj_set_size(ui->DeskTop_img_nes, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_nes, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_nes, &_steam_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_nes, 40, 40);
    lv_image_set_rotation(ui->DeskTop_img_nes, 0);
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_nes, 0,
        LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_nes, 255,
        LV_PART_MAIN|LV_STATE_DEFAULT);
    /* STAGE6 UI USER PATCH END */

    //Write codes DeskTop_label_17
    ui->DeskTop_label_17 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_label_17, 50, 119);
    lv_obj_set_size(ui->DeskTop_label_17, 80, 30);
    lv_label_set_text(ui->DeskTop_label_17, "\xE5\x8E\x9F\xE7\xA5\x9E");
    lv_label_set_long_mode(ui->DeskTop_label_17, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_17, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_17, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_17, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_17, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_17, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_18
    ui->DeskTop_label_18 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_label_18, 148, 119);
    lv_obj_set_size(ui->DeskTop_label_18, 80, 30);
    lv_label_set_text(ui->DeskTop_label_18, "\xE9\x97\xB9\xE9\x92\x9F");
    lv_label_set_long_mode(ui->DeskTop_label_18, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_18, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_18, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_18, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_18, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_18, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_19
    ui->DeskTop_label_19 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_label_19, 250, 119);
    lv_obj_set_size(ui->DeskTop_label_19, 80, 30);
    lv_label_set_text(ui->DeskTop_label_19, "\xE8\xAE\xA1\xE7\xAE\x97\xE5\x99\xA8");
    lv_label_set_long_mode(ui->DeskTop_label_19, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_19, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_19, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_19, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_19, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_19, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_20
    ui->DeskTop_label_20 = lv_label_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_label_20, 352, 120);
    lv_obj_set_size(ui->DeskTop_label_20, 80, 30);
    lv_label_set_text(ui->DeskTop_label_20, "\xE6\x96\x87\xE4\xBB\xB6");
    lv_label_set_long_mode(ui->DeskTop_label_20, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_20, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_20, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_20, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_20, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_20, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* MUSIC APP USER PATCH BEGIN: generated-style desktop page 2 label */
    //Write codes DeskTop_label_musicAPP
    ui->DeskTop_label_musicAPP = lv_label_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_label_musicAPP, 450, 119);
    lv_obj_set_size(ui->DeskTop_label_musicAPP, 80, 30);
    lv_label_set_text(ui->DeskTop_label_musicAPP, "\xE9\x9F\xB3\xE4\xB9\x90");
    lv_label_set_long_mode(ui->DeskTop_label_musicAPP, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_musicAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_musicAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_musicAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_musicAPP, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_musicAPP, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_musicAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_musicAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_musicAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_musicAPP, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_musicAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_musicAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_musicAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_musicAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_musicAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_musicAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    /* MUSIC APP USER PATCH END */

    /* STAGE6 UI USER PATCH BEGIN: 20px Chinese desktop labels */
    ui->DeskTop_label_draw = lv_label_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_label_draw, 550, 119);
    lv_obj_set_size(ui->DeskTop_label_draw, 80, 30);
    lv_label_set_text(ui->DeskTop_label_draw, "画板");
    lv_obj_set_style_text_color(ui->DeskTop_label_draw,
        lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_draw, &lv_font_myyh_20,
        LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_draw, LV_TEXT_ALIGN_CENTER,
        LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_draw, 0,
        LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->DeskTop_label_piano = lv_label_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_label_piano, 650, 119);
    lv_obj_set_size(ui->DeskTop_label_piano, 80, 30);
    lv_label_set_text(ui->DeskTop_label_piano, "钢琴");
    lv_obj_set_style_text_color(ui->DeskTop_label_piano,
        lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_piano, &lv_font_myyh_20,
        LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_piano, LV_TEXT_ALIGN_CENTER,
        LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_piano, 0,
        LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->DeskTop_label_nes = lv_label_create(ui->DeskTop_tileview_DeskTop_tile_1);
    lv_obj_set_pos(ui->DeskTop_label_nes, 750, 119);
    lv_obj_set_size(ui->DeskTop_label_nes, 80, 30);
    lv_label_set_text(ui->DeskTop_label_nes, "小游戏");
    lv_obj_set_style_text_color(ui->DeskTop_label_nes,
        lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_nes, &lv_font_myyh_20,
        LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_nes, LV_TEXT_ALIGN_CENTER,
        LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_nes, 0,
        LV_PART_MAIN|LV_STATE_DEFAULT);
    /* STAGE6 UI USER PATCH END */

    //Write codes DeskTop_tileview_menu
    ui->DeskTop_tileview_menu = lv_tileview_create(ui->DeskTop);
    lv_obj_set_pos(ui->DeskTop_tileview_menu, 112, 470);
    lv_obj_set_size(ui->DeskTop_tileview_menu, 800, 100);
    lv_obj_set_scrollbar_mode(ui->DeskTop_tileview_menu, LV_SCROLLBAR_MODE_OFF);
    ui->DeskTop_tileview_menu_tile = lv_tileview_add_tile(ui->DeskTop_tileview_menu, 0, 0, LV_DIR_RIGHT);

    //Write style for DeskTop_tileview_menu, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_tileview_menu, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_tileview_menu, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_tileview_menu, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_tileview_menu, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_tileview_menu, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_tileview_menu, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_tileview_menu, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_tileview_menu, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes DeskTop_img_phoneAPP
    ui->DeskTop_img_phoneAPP = lv_image_create(ui->DeskTop_tileview_menu_tile);
    lv_obj_set_pos(ui->DeskTop_img_phoneAPP, 222, 8);
    lv_obj_set_size(ui->DeskTop_img_phoneAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_phoneAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_phoneAPP, &_phone_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_phoneAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_phoneAPP, 0);

    //Write style for DeskTop_img_phoneAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_phoneAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_phoneAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_img_messageAPP
    ui->DeskTop_img_messageAPP = lv_image_create(ui->DeskTop_tileview_menu_tile);
    lv_obj_set_pos(ui->DeskTop_img_messageAPP, 352, 8);
    lv_obj_set_size(ui->DeskTop_img_messageAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_messageAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_messageAPP, &_message_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_messageAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_messageAPP, 0);

    //Write style for DeskTop_img_messageAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_messageAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_messageAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_img_cameraAPP
    ui->DeskTop_img_cameraAPP = lv_image_create(ui->DeskTop_tileview_menu_tile);
    lv_obj_set_pos(ui->DeskTop_img_cameraAPP, 482, 8);
    lv_obj_set_size(ui->DeskTop_img_cameraAPP, 80, 80);
    lv_obj_add_flag(ui->DeskTop_img_cameraAPP, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->DeskTop_img_cameraAPP, &_camera_RGB565A8_80x80);
    lv_image_set_pivot(ui->DeskTop_img_cameraAPP, 50,50);
    lv_image_set_rotation(ui->DeskTop_img_cameraAPP, 0);

    //Write style for DeskTop_img_cameraAPP, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->DeskTop_img_cameraAPP, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->DeskTop_img_cameraAPP, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_cont_setting
    ui->DeskTop_cont_setting = lv_obj_create(ui->DeskTop);
    lv_obj_set_pos(ui->DeskTop_cont_setting, 0, 0);
    lv_obj_set_size(ui->DeskTop_cont_setting, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->DeskTop_cont_setting, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->DeskTop_cont_setting, LV_OBJ_FLAG_HIDDEN);

    //Write style for DeskTop_cont_setting, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_cont_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_cont_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_cont_setting, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_cont_setting, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_cont_setting, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_cont_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_cont_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_cont_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_cont_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(ui->DeskTop_cont_setting, norflash_get_wallpaper_dsc(), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_opa(ui->DeskTop_cont_setting, 192, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_recolor_opa(ui->DeskTop_cont_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_cont_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_menu_1
    ui->DeskTop_menu_1 = lv_menu_create(ui->DeskTop_cont_setting);
    lv_obj_set_pos(ui->DeskTop_menu_1, 12, 35);
    lv_obj_set_size(ui->DeskTop_menu_1, 1000, 550);
    lv_obj_set_scrollbar_mode(ui->DeskTop_menu_1, LV_SCROLLBAR_MODE_OFF);

    //Create sidebar page for menu DeskTop_menu_1
    ui->DeskTop_menu_1_sidebar_page = lv_menu_page_create(ui->DeskTop_menu_1, "menu");
    lv_menu_set_sidebar_page(ui->DeskTop_menu_1, ui->DeskTop_menu_1_sidebar_page);
    lv_obj_set_scrollbar_mode(ui->DeskTop_menu_1_sidebar_page, LV_SCROLLBAR_MODE_OFF);

    //Create subpage for DeskTop_menu_1
    lv_obj_t * DeskTop_menu_1_subpage_1 = lv_menu_page_create(ui->DeskTop_menu_1, NULL);
    ui->DeskTop_menu_1_subpage_1_cont = lv_menu_cont_create(DeskTop_menu_1_subpage_1);
    lv_obj_set_layout(ui->DeskTop_menu_1_subpage_1_cont, LV_LAYOUT_NONE);
    ui->DeskTop_menu_1_cont_1 = lv_menu_cont_create(ui->DeskTop_menu_1_sidebar_page);
    ui->DeskTop_menu_1_label_1 = lv_label_create(ui->DeskTop_menu_1_cont_1);
    lv_label_set_text(ui->DeskTop_menu_1_label_1, "WiFi");
    lv_obj_set_size(ui->DeskTop_menu_1_label_1, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(ui->DeskTop_menu_1_label_1, LV_LABEL_LONG_CLIP);
    lv_obj_set_scrollbar_mode(DeskTop_menu_1_subpage_1, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->DeskTop_menu_1, ui->DeskTop_menu_1_cont_1, DeskTop_menu_1_subpage_1);

    //Create subpage for DeskTop_menu_1
    lv_obj_t * DeskTop_menu_1_subpage_2 = lv_menu_page_create(ui->DeskTop_menu_1, NULL);
    ui->DeskTop_menu_1_subpage_2_cont = lv_menu_cont_create(DeskTop_menu_1_subpage_2);
    lv_obj_set_layout(ui->DeskTop_menu_1_subpage_2_cont, LV_LAYOUT_NONE);
    ui->DeskTop_menu_1_cont_2 = lv_menu_cont_create(ui->DeskTop_menu_1_sidebar_page);
    ui->DeskTop_menu_1_label_2 = lv_label_create(ui->DeskTop_menu_1_cont_2);
    lv_label_set_text(ui->DeskTop_menu_1_label_2, "Bluetooth");
    lv_obj_set_size(ui->DeskTop_menu_1_label_2, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(ui->DeskTop_menu_1_label_2, LV_LABEL_LONG_CLIP);
    lv_obj_set_scrollbar_mode(DeskTop_menu_1_subpage_2, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->DeskTop_menu_1, ui->DeskTop_menu_1_cont_2, DeskTop_menu_1_subpage_2);

    //Create subpage for DeskTop_menu_1
    lv_obj_t * DeskTop_menu_1_subpage_3 = lv_menu_page_create(ui->DeskTop_menu_1, NULL);
    ui->DeskTop_menu_1_subpage_3_cont = lv_menu_cont_create(DeskTop_menu_1_subpage_3);
    lv_obj_set_layout(ui->DeskTop_menu_1_subpage_3_cont, LV_LAYOUT_NONE);
    ui->DeskTop_menu_1_cont_3 = lv_menu_cont_create(ui->DeskTop_menu_1_sidebar_page);
    ui->DeskTop_menu_1_label_3 = lv_label_create(ui->DeskTop_menu_1_cont_3);
    lv_label_set_text(ui->DeskTop_menu_1_label_3, "Volume");
    lv_obj_set_size(ui->DeskTop_menu_1_label_3, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(ui->DeskTop_menu_1_label_3, LV_LABEL_LONG_CLIP);
    lv_obj_set_scrollbar_mode(DeskTop_menu_1_subpage_3, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->DeskTop_menu_1, ui->DeskTop_menu_1_cont_3, DeskTop_menu_1_subpage_3);

    //Create subpage for DeskTop_menu_1
    lv_obj_t * DeskTop_menu_1_subpage_4 = lv_menu_page_create(ui->DeskTop_menu_1, NULL);
    ui->DeskTop_menu_1_subpage_4_cont = lv_menu_cont_create(DeskTop_menu_1_subpage_4);
    lv_obj_set_layout(ui->DeskTop_menu_1_subpage_4_cont, LV_LAYOUT_NONE);
    ui->DeskTop_menu_1_cont_4 = lv_menu_cont_create(ui->DeskTop_menu_1_sidebar_page);
    ui->DeskTop_menu_1_label_4 = lv_label_create(ui->DeskTop_menu_1_cont_4);
    lv_label_set_text(ui->DeskTop_menu_1_label_4, "Other Settings");
    lv_obj_set_size(ui->DeskTop_menu_1_label_4, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(ui->DeskTop_menu_1_label_4, LV_LABEL_LONG_CLIP);
    lv_obj_set_scrollbar_mode(DeskTop_menu_1_subpage_4, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->DeskTop_menu_1, ui->DeskTop_menu_1_cont_4, DeskTop_menu_1_subpage_4);

    //Create subpage for DeskTop_menu_1
    lv_obj_t * DeskTop_menu_1_subpage_5 = lv_menu_page_create(ui->DeskTop_menu_1, NULL);
    ui->DeskTop_menu_1_subpage_5_cont = lv_menu_cont_create(DeskTop_menu_1_subpage_5);
    lv_obj_set_layout(ui->DeskTop_menu_1_subpage_5_cont, LV_LAYOUT_NONE);
    ui->DeskTop_menu_1_cont_5 = lv_menu_cont_create(ui->DeskTop_menu_1_sidebar_page);
    ui->DeskTop_menu_1_label_5 = lv_label_create(ui->DeskTop_menu_1_cont_5);
    lv_label_set_text(ui->DeskTop_menu_1_label_5, "Chart");
    lv_obj_set_size(ui->DeskTop_menu_1_label_5, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(ui->DeskTop_menu_1_label_5, LV_LABEL_LONG_CLIP);
    lv_obj_set_scrollbar_mode(DeskTop_menu_1_subpage_5, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->DeskTop_menu_1, ui->DeskTop_menu_1_cont_5, DeskTop_menu_1_subpage_5);

    //Create subpage for DeskTop_menu_1
    lv_obj_t * DeskTop_menu_1_subpage_6 = lv_menu_page_create(ui->DeskTop_menu_1, NULL);
    ui->DeskTop_menu_1_subpage_6_cont = lv_menu_cont_create(DeskTop_menu_1_subpage_6);
    lv_obj_set_layout(ui->DeskTop_menu_1_subpage_6_cont, LV_LAYOUT_NONE);
    ui->DeskTop_menu_1_cont_6 = lv_menu_cont_create(ui->DeskTop_menu_1_sidebar_page);
    ui->DeskTop_menu_1_label_6 = lv_label_create(ui->DeskTop_menu_1_cont_6);
    lv_label_set_text(ui->DeskTop_menu_1_label_6, "Time");
    lv_obj_set_size(ui->DeskTop_menu_1_label_6, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(ui->DeskTop_menu_1_label_6, LV_LABEL_LONG_CLIP);
    lv_obj_set_scrollbar_mode(DeskTop_menu_1_subpage_6, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->DeskTop_menu_1, ui->DeskTop_menu_1_cont_6, DeskTop_menu_1_subpage_6);

    //Create subpage for DeskTop_menu_1
    lv_obj_t * DeskTop_menu_1_subpage_7 = lv_menu_page_create(ui->DeskTop_menu_1, NULL);
    ui->DeskTop_menu_1_subpage_7_cont = lv_menu_cont_create(DeskTop_menu_1_subpage_7);
    lv_obj_set_layout(ui->DeskTop_menu_1_subpage_7_cont, LV_LAYOUT_NONE);
    ui->DeskTop_menu_1_cont_7 = lv_menu_cont_create(ui->DeskTop_menu_1_sidebar_page);
    ui->DeskTop_menu_1_label_7 = lv_label_create(ui->DeskTop_menu_1_cont_7);
    lv_label_set_text(ui->DeskTop_menu_1_label_7, "Folder");
    lv_obj_set_size(ui->DeskTop_menu_1_label_7, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(ui->DeskTop_menu_1_label_7, LV_LABEL_LONG_CLIP);
    lv_obj_set_scrollbar_mode(DeskTop_menu_1_subpage_7, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->DeskTop_menu_1, ui->DeskTop_menu_1_cont_7, DeskTop_menu_1_subpage_7);

    //Write style for DeskTop_menu_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_menu_1, 159, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_menu_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_menu_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_menu_1, 21, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_menu_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_menu_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_margin_hor(ui->DeskTop_menu_1_sidebar_page, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_margin_ver(ui->DeskTop_menu_1_sidebar_page, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_menu_1_sidebar_page, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_menu_1_sidebar_page, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_menu_1_sidebar_page, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_menu_1_sidebar_page, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_menu_1_sidebar_page, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_menu_1_extra_option_btns_main_default
    static lv_style_t style_DeskTop_menu_1_extra_option_btns_main_default;
    ui_init_style(&style_DeskTop_menu_1_extra_option_btns_main_default);

    lv_style_set_text_color(&style_DeskTop_menu_1_extra_option_btns_main_default, lv_color_hex(0x151212));
    lv_style_set_text_font(&style_DeskTop_menu_1_extra_option_btns_main_default, &lv_font_myyh_12);
    lv_style_set_text_opa(&style_DeskTop_menu_1_extra_option_btns_main_default, 255);
    lv_style_set_text_align(&style_DeskTop_menu_1_extra_option_btns_main_default, LV_TEXT_ALIGN_CENTER);
    lv_style_set_pad_top(&style_DeskTop_menu_1_extra_option_btns_main_default, 10);
    lv_style_set_pad_bottom(&style_DeskTop_menu_1_extra_option_btns_main_default, 10);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_7, &style_DeskTop_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_6, &style_DeskTop_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_5, &style_DeskTop_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_4, &style_DeskTop_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_3, &style_DeskTop_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_2, &style_DeskTop_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_1, &style_DeskTop_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_CHECKED for &style_DeskTop_menu_1_extra_option_btns_main_checked
    static lv_style_t style_DeskTop_menu_1_extra_option_btns_main_checked;
    ui_init_style(&style_DeskTop_menu_1_extra_option_btns_main_checked);

    lv_style_set_bg_opa(&style_DeskTop_menu_1_extra_option_btns_main_checked, 60);
    lv_style_set_bg_color(&style_DeskTop_menu_1_extra_option_btns_main_checked, lv_color_hex(0x19a5ff));
    lv_style_set_bg_grad_dir(&style_DeskTop_menu_1_extra_option_btns_main_checked, LV_GRAD_DIR_NONE);
    lv_style_set_radius(&style_DeskTop_menu_1_extra_option_btns_main_checked, 5);
    lv_style_set_text_color(&style_DeskTop_menu_1_extra_option_btns_main_checked, lv_color_hex(0x9ab700));
    lv_style_set_text_font(&style_DeskTop_menu_1_extra_option_btns_main_checked, &lv_font_myyh_12);
    lv_style_set_text_opa(&style_DeskTop_menu_1_extra_option_btns_main_checked, 255);
    lv_style_set_text_align(&style_DeskTop_menu_1_extra_option_btns_main_checked, LV_TEXT_ALIGN_CENTER);
    lv_style_set_border_width(&style_DeskTop_menu_1_extra_option_btns_main_checked, 0);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_7, &style_DeskTop_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_6, &style_DeskTop_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_5, &style_DeskTop_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_4, &style_DeskTop_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_3, &style_DeskTop_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_2, &style_DeskTop_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_add_style(ui->DeskTop_menu_1_cont_1, &style_DeskTop_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_menu_1_extra_main_title_main_default
    static lv_style_t style_DeskTop_menu_1_extra_main_title_main_default;
    ui_init_style(&style_DeskTop_menu_1_extra_main_title_main_default);

    lv_style_set_bg_opa(&style_DeskTop_menu_1_extra_main_title_main_default, 0);
    lv_style_set_text_color(&style_DeskTop_menu_1_extra_main_title_main_default, lv_color_hex(0x41485a));
    lv_style_set_text_font(&style_DeskTop_menu_1_extra_main_title_main_default, &lv_font_montserratMedium_14);
    lv_style_set_text_opa(&style_DeskTop_menu_1_extra_main_title_main_default, 255);
    lv_style_set_text_align(&style_DeskTop_menu_1_extra_main_title_main_default, LV_TEXT_ALIGN_CENTER);
    lv_style_set_pad_hor(&style_DeskTop_menu_1_extra_main_title_main_default, 5);
    lv_style_set_pad_ver(&style_DeskTop_menu_1_extra_main_title_main_default, 5);
    lv_menu_t * DeskTop_menu_1_menu= (lv_menu_t *)ui->DeskTop_menu_1;
    lv_obj_t * DeskTop_menu_1_title = lv_menu_get_sidebar_header(ui->DeskTop_menu_1);
    lv_obj_set_size(DeskTop_menu_1_title, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_add_style(lv_menu_get_sidebar_header(ui->DeskTop_menu_1), &style_DeskTop_menu_1_extra_main_title_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);



    //Write codes DeskTop_cont_WiFi_list
    ui->DeskTop_cont_WiFi_list = lv_obj_create(ui->DeskTop_menu_1_subpage_1_cont);
    lv_obj_set_pos(ui->DeskTop_cont_WiFi_list, 61, 11);
    lv_obj_set_size(ui->DeskTop_cont_WiFi_list, 600, 530);
    lv_obj_set_scrollbar_mode(ui->DeskTop_cont_WiFi_list, LV_SCROLLBAR_MODE_OFF);

    //Write style for DeskTop_cont_WiFi_list, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_cont_WiFi_list, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_cont_WiFi_list, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_cont_WiFi_list, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_cont_WiFi_list, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_cont_WiFi_list, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_cont_WiFi_list, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_cont_WiFi_list, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_cont_WiFi_list, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_list_1
    ui->DeskTop_list_1 = lv_list_create(ui->DeskTop_cont_WiFi_list);
    lv_obj_set_pos(ui->DeskTop_list_1, 300, 10);
    lv_obj_set_size(ui->DeskTop_list_1, 300, 500);
    lv_obj_set_scrollbar_mode(ui->DeskTop_list_1, LV_SCROLLBAR_MODE_AUTO);
    ui->DeskTop_list_1_item0 = lv_list_add_button(ui->DeskTop_list_1, LV_SYMBOL_WIFI, "iQOO Neo9");
    ui->DeskTop_list_1_item1 = lv_list_add_button(ui->DeskTop_list_1, LV_SYMBOL_WIFI, "442");

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_list_1_main_main_default
    static lv_style_t style_DeskTop_list_1_main_main_default;
    ui_init_style(&style_DeskTop_list_1_main_main_default);

    lv_style_set_pad_top(&style_DeskTop_list_1_main_main_default, 9);
    lv_style_set_pad_left(&style_DeskTop_list_1_main_main_default, 8);
    lv_style_set_pad_right(&style_DeskTop_list_1_main_main_default, 8);
    lv_style_set_pad_bottom(&style_DeskTop_list_1_main_main_default, 13);
    lv_style_set_bg_opa(&style_DeskTop_list_1_main_main_default, 0);
    lv_style_set_border_width(&style_DeskTop_list_1_main_main_default, 0);
    lv_style_set_radius(&style_DeskTop_list_1_main_main_default, 0);
    lv_style_set_shadow_width(&style_DeskTop_list_1_main_main_default, 2);
    lv_style_set_shadow_color(&style_DeskTop_list_1_main_main_default, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&style_DeskTop_list_1_main_main_default, 255);
    lv_style_set_shadow_spread(&style_DeskTop_list_1_main_main_default, 1);
    lv_style_set_shadow_offset_x(&style_DeskTop_list_1_main_main_default, 1);
    lv_style_set_shadow_offset_y(&style_DeskTop_list_1_main_main_default, 1);
    lv_obj_add_style(ui->DeskTop_list_1, &style_DeskTop_list_1_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_list_1_main_scrollbar_default
    static lv_style_t style_DeskTop_list_1_main_scrollbar_default;
    ui_init_style(&style_DeskTop_list_1_main_scrollbar_default);

    lv_style_set_radius(&style_DeskTop_list_1_main_scrollbar_default, 3);
    lv_style_set_bg_opa(&style_DeskTop_list_1_main_scrollbar_default, 255);
    lv_style_set_bg_color(&style_DeskTop_list_1_main_scrollbar_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_DeskTop_list_1_main_scrollbar_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->DeskTop_list_1, &style_DeskTop_list_1_main_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_list_1_extra_btns_main_default
    static lv_style_t style_DeskTop_list_1_extra_btns_main_default;
    ui_init_style(&style_DeskTop_list_1_extra_btns_main_default);

    lv_style_set_pad_top(&style_DeskTop_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_left(&style_DeskTop_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_right(&style_DeskTop_list_1_extra_btns_main_default, 5);
    lv_style_set_pad_bottom(&style_DeskTop_list_1_extra_btns_main_default, 5);
    lv_style_set_border_width(&style_DeskTop_list_1_extra_btns_main_default, 0);
    lv_style_set_text_color(&style_DeskTop_list_1_extra_btns_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_DeskTop_list_1_extra_btns_main_default, &lv_font_myyh_12);
    lv_style_set_text_opa(&style_DeskTop_list_1_extra_btns_main_default, 255);
    lv_style_set_radius(&style_DeskTop_list_1_extra_btns_main_default, 3);
    lv_style_set_bg_opa(&style_DeskTop_list_1_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_DeskTop_list_1_extra_btns_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_DeskTop_list_1_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->DeskTop_list_1_item1, &style_DeskTop_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->DeskTop_list_1_item0, &style_DeskTop_list_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_list_1_extra_texts_main_default
    static lv_style_t style_DeskTop_list_1_extra_texts_main_default;
    ui_init_style(&style_DeskTop_list_1_extra_texts_main_default);

    lv_style_set_pad_top(&style_DeskTop_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_left(&style_DeskTop_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_right(&style_DeskTop_list_1_extra_texts_main_default, 5);
    lv_style_set_pad_bottom(&style_DeskTop_list_1_extra_texts_main_default, 5);
    lv_style_set_border_width(&style_DeskTop_list_1_extra_texts_main_default, 0);
    lv_style_set_text_color(&style_DeskTop_list_1_extra_texts_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_DeskTop_list_1_extra_texts_main_default, &lv_font_myyh_12);
    lv_style_set_text_opa(&style_DeskTop_list_1_extra_texts_main_default, 255);
    lv_style_set_radius(&style_DeskTop_list_1_extra_texts_main_default, 3);
    lv_style_set_transform_width(&style_DeskTop_list_1_extra_texts_main_default, 0);
    lv_style_set_bg_opa(&style_DeskTop_list_1_extra_texts_main_default, 255);
    lv_style_set_bg_color(&style_DeskTop_list_1_extra_texts_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_DeskTop_list_1_extra_texts_main_default, LV_GRAD_DIR_NONE);

    //Write codes DeskTop_btn_custom
    ui->DeskTop_btn_custom = lv_button_create(ui->DeskTop_cont_WiFi_list);
    lv_obj_set_pos(ui->DeskTop_btn_custom, 43, 23);
    lv_obj_set_size(ui->DeskTop_btn_custom, 200, 50);
    ui->DeskTop_btn_custom_label = lv_label_create(ui->DeskTop_btn_custom);
    lv_label_set_text(ui->DeskTop_btn_custom_label, "\xE8\x87\xAA\xE5\xAE\x9A\xE4\xB9\x89\xE8\xBF\x9E\xE6\x8E\xA5");
    lv_label_set_long_mode(ui->DeskTop_btn_custom_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_custom_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_custom, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_custom_label, LV_PCT(100));

    //Write style for DeskTop_btn_custom, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_custom, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_custom, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_custom, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_custom, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_custom, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_custom, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_custom, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_custom, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_custom, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_custom, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_custom, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_connect
    ui->DeskTop_btn_connect = lv_button_create(ui->DeskTop_cont_WiFi_list);
    lv_obj_set_pos(ui->DeskTop_btn_connect, 45, 100);
    lv_obj_set_size(ui->DeskTop_btn_connect, 200, 50);
    ui->DeskTop_btn_connect_label = lv_label_create(ui->DeskTop_btn_connect);
    lv_label_set_text(ui->DeskTop_btn_connect_label, "\xE8\xBF\x9E\xE6\x8E\xA5");
    lv_label_set_long_mode(ui->DeskTop_btn_connect_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_connect_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_connect, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_connect_label, LV_PCT(100));

    //Write style for DeskTop_btn_connect, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_connect, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_connect, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_connect, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_connect, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_connect, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_connect, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_connect, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_connect, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_connect, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_connect, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_connect, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);



    //Write codes DeskTop_cont_blutooth
    ui->DeskTop_cont_blutooth = lv_obj_create(ui->DeskTop_menu_1_subpage_2_cont);
    lv_obj_set_pos(ui->DeskTop_cont_blutooth, 61, 11);
    lv_obj_set_size(ui->DeskTop_cont_blutooth, 600, 530);
    lv_obj_set_scrollbar_mode(ui->DeskTop_cont_blutooth, LV_SCROLLBAR_MODE_OFF);

    //Write style for DeskTop_cont_blutooth, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_cont_blutooth, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_cont_blutooth, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_cont_blutooth, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_cont_blutooth, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_cont_blutooth, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_cont_blutooth, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_cont_blutooth, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_cont_blutooth, 0, LV_PART_MAIN|LV_STATE_DEFAULT);



    //Write codes DeskTop_cont_volume
    ui->DeskTop_cont_volume = lv_obj_create(ui->DeskTop_menu_1_subpage_3_cont);
    lv_obj_set_pos(ui->DeskTop_cont_volume, 61, 11);
    lv_obj_set_size(ui->DeskTop_cont_volume, 600, 530);
    lv_obj_set_scrollbar_mode(ui->DeskTop_cont_volume, LV_SCROLLBAR_MODE_OFF);

    //Write style for DeskTop_cont_volume, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_cont_volume, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_cont_volume, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_cont_volume, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_cont_volume, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_cont_volume, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_cont_volume, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_cont_volume, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_cont_volume, 0, LV_PART_MAIN|LV_STATE_DEFAULT);



    //Write codes DeskTop_cont_other_setting
    ui->DeskTop_cont_other_setting = lv_obj_create(ui->DeskTop_menu_1_subpage_4_cont);
    lv_obj_set_pos(ui->DeskTop_cont_other_setting, 61, 11);
    lv_obj_set_size(ui->DeskTop_cont_other_setting, 600, 530);
    lv_obj_set_scrollbar_mode(ui->DeskTop_cont_other_setting, LV_SCROLLBAR_MODE_OFF);

    //Write style for DeskTop_cont_other_setting, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_cont_other_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_cont_other_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_cont_other_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_cont_other_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_cont_other_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_cont_other_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_cont_other_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_cont_other_setting, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_2
    ui->DeskTop_btn_2 = lv_button_create(ui->DeskTop_cont_other_setting);
    lv_obj_set_pos(ui->DeskTop_btn_2, 48, 100);
    lv_obj_set_size(ui->DeskTop_btn_2, 500, 50);
    ui->DeskTop_btn_2_label = lv_label_create(ui->DeskTop_btn_2);
    lv_label_set_text(ui->DeskTop_btn_2_label, "\xE4\xB8\xB2\xE5\x8F\xA3\xE7\x95\x8C\xE9\x9D\xA2");
    lv_label_set_long_mode(ui->DeskTop_btn_2_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_2_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_2, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_2_label, LV_PCT(100));

    //Write style for DeskTop_btn_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_2, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_2, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_2, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_2, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_Debug
    ui->DeskTop_btn_Debug = lv_button_create(ui->DeskTop_cont_other_setting);
    lv_obj_set_pos(ui->DeskTop_btn_Debug, 48, 27);
    lv_obj_set_size(ui->DeskTop_btn_Debug, 500, 50);
    ui->DeskTop_btn_Debug_label = lv_label_create(ui->DeskTop_btn_Debug);
    lv_label_set_text(ui->DeskTop_btn_Debug_label, "\xE8\xB0\x83\xE8\xAF\x95");
    lv_label_set_long_mode(ui->DeskTop_btn_Debug_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_Debug_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_Debug, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_Debug_label, LV_PCT(100));

    //Write style for DeskTop_btn_Debug, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_Debug, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_Debug, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_Debug, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_Debug, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_Debug, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_Debug, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_Debug, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_Debug, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_Debug, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_Debug, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_Debug, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);



    //Write codes DeskTop_cont_chart
    ui->DeskTop_cont_chart = lv_obj_create(ui->DeskTop_menu_1_subpage_5_cont);
    lv_obj_set_pos(ui->DeskTop_cont_chart, 61, 11);
    lv_obj_set_size(ui->DeskTop_cont_chart, 600, 520);
    lv_obj_set_scrollbar_mode(ui->DeskTop_cont_chart, LV_SCROLLBAR_MODE_OFF);

    //Write style for DeskTop_cont_chart, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_cont_chart, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_cont_chart, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_cont_chart, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_cont_chart, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_cont_chart, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_cont_chart, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_cont_chart, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_cont_chart, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_cont_chart2
    ui->DeskTop_cont_chart2 = lv_obj_create(ui->DeskTop_cont_chart);
    lv_obj_set_pos(ui->DeskTop_cont_chart2, 86, 400);
    lv_obj_set_size(ui->DeskTop_cont_chart2, 500, 350);
    lv_obj_set_scrollbar_mode(ui->DeskTop_cont_chart2, LV_SCROLLBAR_MODE_AUTO);

    //Write style for DeskTop_cont_chart2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_cont_chart2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_cont_chart2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_cont_chart2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_cont_chart2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_cont_chart2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_cont_chart2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_cont_chart2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_cont_chart2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_scale_2
    ui->DeskTop_scale_2 = lv_scale_create(ui->DeskTop_cont_chart2);
    lv_obj_set_pos(ui->DeskTop_scale_2, 76, 270);
    lv_obj_set_size(ui->DeskTop_scale_2, 450, 30);
    lv_scale_set_mode(ui->DeskTop_scale_2, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
    lv_scale_set_total_tick_count(ui->DeskTop_scale_2, 8);
    lv_scale_set_major_tick_every(ui->DeskTop_scale_2, 1);
    lv_scale_set_label_show(ui->DeskTop_scale_2, true);
    lv_scale_set_range(ui->DeskTop_scale_2, 0, 7);
    lv_scale_set_post_draw(ui->DeskTop_scale_2, true);


    //Write style for DeskTop_scale_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_scale_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_scale_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_scale_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_scale_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_scale_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_scale_2, lv_color_hex(0x757575), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_scale_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_scale_2, true, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_scale_2, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_scale_2, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_scale_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_scale_2, Part: LV_PART_ITEMS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_length(ui->DeskTop_scale_2, 5, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_scale_2, 2, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_scale_2, lv_color_hex(0x757575), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_scale_2, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_scale_2, true, LV_PART_ITEMS|LV_STATE_DEFAULT);

    //Write style for DeskTop_scale_2, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_length(ui->DeskTop_scale_2, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_scale_2, 2, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_scale_2, lv_color_hex(0x757575), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_scale_2, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_scale_2, true, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes DeskTop_chart_2
    ui->DeskTop_chart_2 = lv_chart_create(ui->DeskTop_cont_chart2);
    lv_obj_set_pos(ui->DeskTop_chart_2, 77, 19);
    lv_obj_set_size(ui->DeskTop_chart_2, 600, 250);
    lv_obj_set_scrollbar_mode(ui->DeskTop_chart_2, LV_SCROLLBAR_MODE_OFF);
    lv_chart_set_type(ui->DeskTop_chart_2, LV_CHART_TYPE_BAR);
    lv_chart_set_div_line_count(ui->DeskTop_chart_2, 2, 8);
    lv_chart_set_point_count(ui->DeskTop_chart_2, 9);
    lv_chart_set_range(ui->DeskTop_chart_2, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_range(ui->DeskTop_chart_2, LV_CHART_AXIS_SECONDARY_Y, 0, 100);
    ui->DeskTop_chart_2_0 = lv_chart_add_series(ui->DeskTop_chart_2, lv_color_hex(0x00d9ff), LV_CHART_AXIS_PRIMARY_Y);
#if LV_USE_FREEMASTER == 0
    lv_chart_set_next_value(ui->DeskTop_chart_2, ui->DeskTop_chart_2_0, 1);
    lv_chart_set_next_value(ui->DeskTop_chart_2, ui->DeskTop_chart_2_0, 20);
    lv_chart_set_next_value(ui->DeskTop_chart_2, ui->DeskTop_chart_2_0, 30);
    lv_chart_set_next_value(ui->DeskTop_chart_2, ui->DeskTop_chart_2_0, 40);
    lv_chart_set_next_value(ui->DeskTop_chart_2, ui->DeskTop_chart_2_0, 60);
    lv_chart_set_next_value(ui->DeskTop_chart_2, ui->DeskTop_chart_2_0, 91);
    lv_chart_set_next_value(ui->DeskTop_chart_2, ui->DeskTop_chart_2_0, 85);
    lv_chart_set_next_value(ui->DeskTop_chart_2, ui->DeskTop_chart_2_0, 75);
    lv_chart_set_next_value(ui->DeskTop_chart_2, ui->DeskTop_chart_2_0, 32);
#endif

    //Write style for DeskTop_chart_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_chart_2, 128, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_chart_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_chart_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_chart_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_chart_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_chart_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_chart_2, lv_color_hex(0x949494), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_chart_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_chart_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_cont_chart1
    ui->DeskTop_cont_chart1 = lv_obj_create(ui->DeskTop_cont_chart);
    lv_obj_set_pos(ui->DeskTop_cont_chart1, 91, 38);
    lv_obj_set_size(ui->DeskTop_cont_chart1, 500, 350);
    lv_obj_set_scrollbar_mode(ui->DeskTop_cont_chart1, LV_SCROLLBAR_MODE_AUTO);

    //Write style for DeskTop_cont_chart1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_cont_chart1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_cont_chart1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_cont_chart1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_cont_chart1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_cont_chart1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_cont_chart1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_cont_chart1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_cont_chart1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_scale_x1
    ui->DeskTop_scale_x1 = lv_scale_create(ui->DeskTop_cont_chart1);
    lv_obj_set_pos(ui->DeskTop_scale_x1, 76, 270);
    lv_obj_set_size(ui->DeskTop_scale_x1, 800, 30);
    lv_scale_set_mode(ui->DeskTop_scale_x1, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
    lv_scale_set_total_tick_count(ui->DeskTop_scale_x1, 8);
    lv_scale_set_major_tick_every(ui->DeskTop_scale_x1, 1);
    lv_scale_set_label_show(ui->DeskTop_scale_x1, true);
    lv_scale_set_range(ui->DeskTop_scale_x1, 0, 7);
    lv_scale_set_post_draw(ui->DeskTop_scale_x1, true);


    //Write style for DeskTop_scale_x1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_scale_x1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_scale_x1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_scale_x1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_scale_x1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_scale_x1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_scale_x1, lv_color_hex(0x757575), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_scale_x1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_scale_x1, true, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_scale_x1, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_scale_x1, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_scale_x1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_scale_x1, Part: LV_PART_ITEMS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_length(ui->DeskTop_scale_x1, 5, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_scale_x1, 2, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_scale_x1, lv_color_hex(0x757575), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_scale_x1, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_scale_x1, true, LV_PART_ITEMS|LV_STATE_DEFAULT);

    //Write style for DeskTop_scale_x1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_length(ui->DeskTop_scale_x1, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_scale_x1, 2, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_scale_x1, lv_color_hex(0x757575), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_scale_x1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_scale_x1, true, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes DeskTop_chart_1
    ui->DeskTop_chart_1 = lv_chart_create(ui->DeskTop_cont_chart1);
    lv_obj_set_pos(ui->DeskTop_chart_1, 76, 20);
    lv_obj_set_size(ui->DeskTop_chart_1, 800, 250);
    lv_obj_set_scrollbar_mode(ui->DeskTop_chart_1, LV_SCROLLBAR_MODE_OFF);
    lv_chart_set_type(ui->DeskTop_chart_1, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(ui->DeskTop_chart_1, 3, 8);
    lv_chart_set_point_count(ui->DeskTop_chart_1, 5);
    lv_chart_set_range(ui->DeskTop_chart_1, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_range(ui->DeskTop_chart_1, LV_CHART_AXIS_SECONDARY_Y, 0, 100);
    ui->DeskTop_chart_1_0 = lv_chart_add_series(ui->DeskTop_chart_1, lv_color_hex(0x0039ff), LV_CHART_AXIS_PRIMARY_Y);
#if LV_USE_FREEMASTER == 0
    lv_chart_set_next_value(ui->DeskTop_chart_1, ui->DeskTop_chart_1_0, 1);
    lv_chart_set_next_value(ui->DeskTop_chart_1, ui->DeskTop_chart_1_0, 20);
    lv_chart_set_next_value(ui->DeskTop_chart_1, ui->DeskTop_chart_1_0, 30);
    lv_chart_set_next_value(ui->DeskTop_chart_1, ui->DeskTop_chart_1_0, 40);
    lv_chart_set_next_value(ui->DeskTop_chart_1, ui->DeskTop_chart_1_0, 5);
#endif

    //Write style for DeskTop_chart_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_chart_1, 128, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_chart_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_chart_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_chart_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_chart_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_chart_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_chart_1, lv_color_hex(0x949494), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_chart_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_chart_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_scale_y2
    ui->DeskTop_scale_y2 = lv_scale_create(ui->DeskTop_cont_chart);
    lv_obj_set_pos(ui->DeskTop_scale_y2, 117, 420);
    lv_obj_set_size(ui->DeskTop_scale_y2, 45, 250);
    lv_scale_set_mode(ui->DeskTop_scale_y2, LV_SCALE_MODE_VERTICAL_LEFT);
    lv_scale_set_total_tick_count(ui->DeskTop_scale_y2, 6);
    lv_scale_set_major_tick_every(ui->DeskTop_scale_y2, 1);
    lv_scale_set_label_show(ui->DeskTop_scale_y2, true);
    lv_scale_set_range(ui->DeskTop_scale_y2, 0, 100);
    lv_scale_set_post_draw(ui->DeskTop_scale_y2, true);


    //Write style for DeskTop_scale_y2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_scale_y2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_scale_y2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_scale_y2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_scale_y2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_scale_y2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_scale_y2, lv_color_hex(0x757575), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_scale_y2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_scale_y2, true, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_scale_y2, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_scale_y2, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_scale_y2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_scale_y2, Part: LV_PART_ITEMS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_length(ui->DeskTop_scale_y2, 5, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_scale_y2, 2, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_scale_y2, lv_color_hex(0x757575), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_scale_y2, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_scale_y2, true, LV_PART_ITEMS|LV_STATE_DEFAULT);

    //Write style for DeskTop_scale_y2, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_length(ui->DeskTop_scale_y2, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_scale_y2, 2, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_scale_y2, lv_color_hex(0x757575), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_scale_y2, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_scale_y2, true, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes DeskTop_scale_y1
    ui->DeskTop_scale_y1 = lv_scale_create(ui->DeskTop_cont_chart);
    lv_obj_set_pos(ui->DeskTop_scale_y1, 122, 57);
    lv_obj_set_size(ui->DeskTop_scale_y1, 45, 250);
    lv_scale_set_mode(ui->DeskTop_scale_y1, LV_SCALE_MODE_VERTICAL_LEFT);
    lv_scale_set_total_tick_count(ui->DeskTop_scale_y1, 6);
    lv_scale_set_major_tick_every(ui->DeskTop_scale_y1, 1);
    lv_scale_set_label_show(ui->DeskTop_scale_y1, true);
    lv_scale_set_range(ui->DeskTop_scale_y1, 0, 100);
    lv_scale_set_post_draw(ui->DeskTop_scale_y1, true);


    //Write style for DeskTop_scale_y1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_scale_y1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_scale_y1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_scale_y1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_scale_y1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_scale_y1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_scale_y1, lv_color_hex(0x757575), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_scale_y1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_scale_y1, true, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_scale_y1, lv_color_hex(0x0D3055), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_scale_y1, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_scale_y1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_scale_y1, Part: LV_PART_ITEMS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_length(ui->DeskTop_scale_y1, 5, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_scale_y1, 2, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_scale_y1, lv_color_hex(0x757575), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_scale_y1, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_scale_y1, true, LV_PART_ITEMS|LV_STATE_DEFAULT);

    //Write style for DeskTop_scale_y1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_length(ui->DeskTop_scale_y1, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->DeskTop_scale_y1, 2, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->DeskTop_scale_y1, lv_color_hex(0x757575), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->DeskTop_scale_y1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->DeskTop_scale_y1, true, LV_PART_INDICATOR|LV_STATE_DEFAULT);



    //Write codes DeskTop_cont_time
    ui->DeskTop_cont_time = lv_obj_create(ui->DeskTop_menu_1_subpage_6_cont);
    lv_obj_set_pos(ui->DeskTop_cont_time, 60, 10);
    lv_obj_set_size(ui->DeskTop_cont_time, 600, 520);
    lv_obj_set_scrollbar_mode(ui->DeskTop_cont_time, LV_SCROLLBAR_MODE_OFF);

    //Write style for DeskTop_cont_time, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_cont_time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_cont_time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_cont_time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_cont_time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_cont_time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_cont_time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_cont_time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_cont_time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_22
    ui->DeskTop_label_22 = lv_label_create(ui->DeskTop_cont_time);
    lv_obj_set_pos(ui->DeskTop_label_22, 94, 90);
    lv_obj_set_size(ui->DeskTop_label_22, 150, 30);
    lv_label_set_text(ui->DeskTop_label_22, "\xE6\x97\xA5\xE6\x9C\x9F\xE8\xAE\xBE\xE7\xBD\xAE");
    lv_label_set_long_mode(ui->DeskTop_label_22, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_22, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_22, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_22, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_22, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_22, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_datetext_2
    ui->DeskTop_datetext_2 = lv_label_create(ui->DeskTop_cont_time);
    lv_obj_set_pos(ui->DeskTop_datetext_2, 178, 77);
    lv_obj_set_size(ui->DeskTop_datetext_2, 300, 40);
    lv_label_set_text(ui->DeskTop_datetext_2, "2024/04/22");
    lv_obj_set_style_text_align(ui->DeskTop_datetext_2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_flag(ui->DeskTop_datetext_2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui->DeskTop_datetext_2, DeskTop_datetext_2_event_handler, LV_EVENT_ALL, NULL);

    //Write style for DeskTop_datetext_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->DeskTop_datetext_2, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_datetext_2, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_datetext_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_datetext_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_datetext_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_datetext_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_datetext_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_datetext_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_datetext_2, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_datetext_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_datetext_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_datetext_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_6
    ui->DeskTop_btn_6 = lv_button_create(ui->DeskTop_cont_time);
    lv_obj_set_pos(ui->DeskTop_btn_6, 171, 140);
    lv_obj_set_size(ui->DeskTop_btn_6, 200, 50);
    ui->DeskTop_btn_6_label = lv_label_create(ui->DeskTop_btn_6);
    lv_label_set_text(ui->DeskTop_btn_6_label, "\xE6\x97\xB6\xE9\x97\xB4\xE8\xAE\xBE\xE7\xBD\xAE");
    lv_label_set_long_mode(ui->DeskTop_btn_6_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_6_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_6, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_6_label, LV_PCT(100));

    //Write style for DeskTop_btn_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_btn_6, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_6, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_6, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_6, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_6, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_6, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_6, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_7
    ui->DeskTop_btn_7 = lv_button_create(ui->DeskTop_cont_time);
    lv_obj_set_pos(ui->DeskTop_btn_7, 172, 205);
    lv_obj_set_size(ui->DeskTop_btn_7, 200, 50);
    ui->DeskTop_btn_7_label = lv_label_create(ui->DeskTop_btn_7);
    lv_label_set_text(ui->DeskTop_btn_7_label, "\xE9\x97\xB9\xE9\x92\x9F\xE8\xAE\xBE\xE7\xBD\xAE");
    lv_label_set_long_mode(ui->DeskTop_btn_7_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_7_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_7, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_7_label, LV_PCT(100));

    //Write style for DeskTop_btn_7, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_btn_7, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_7, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_7, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_7, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_7, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_7, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_7, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_7, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_7, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_8
    ui->DeskTop_btn_8 = lv_button_create(ui->DeskTop_cont_time);
    lv_obj_set_pos(ui->DeskTop_btn_8, 170, 272);
    lv_obj_set_size(ui->DeskTop_btn_8, 200, 50);
    ui->DeskTop_btn_8_label = lv_label_create(ui->DeskTop_btn_8);
    lv_label_set_text(ui->DeskTop_btn_8_label, "\xE8\xAE\xA1\xE6\x97\xB6");
    lv_label_set_long_mode(ui->DeskTop_btn_8_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_8_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_8, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_8_label, LV_PCT(100));

    //Write style for DeskTop_btn_8, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_btn_8, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_8, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_8, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_8, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_8, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_8, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_8, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_8, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_8, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);



    //Write codes DeskTop_cont_folder
    ui->DeskTop_cont_folder = lv_obj_create(ui->DeskTop_menu_1_subpage_7_cont);
    lv_obj_set_pos(ui->DeskTop_cont_folder, 61, 11);
    lv_obj_set_size(ui->DeskTop_cont_folder, 600, 520);
    lv_obj_set_scrollbar_mode(ui->DeskTop_cont_folder, LV_SCROLLBAR_MODE_OFF);

    //Write style for DeskTop_cont_folder, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_cont_folder, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_cont_folder, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_cont_folder, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_cont_folder, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_cont_folder, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_cont_folder, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_cont_folder, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_cont_folder, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_SD
    ui->DeskTop_btn_SD = lv_button_create(ui->DeskTop_cont_folder);
    lv_obj_set_pos(ui->DeskTop_btn_SD, 19, 38);
    lv_obj_set_size(ui->DeskTop_btn_SD, 150, 50);
    ui->DeskTop_btn_SD_label = lv_label_create(ui->DeskTop_btn_SD);
    lv_label_set_text(ui->DeskTop_btn_SD_label, "\xE6\x9F\xA5\xE7\x9C\x8B\x53\x44\xE5\x8D\xA1\xE6\x96\x87\xE4\xBB\xB6" LV_SYMBOL_SD_CARD " ");
    lv_label_set_long_mode(ui->DeskTop_btn_SD_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_SD_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_SD, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_SD_label, LV_PCT(100));

    //Write style for DeskTop_btn_SD, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_btn_SD, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_SD, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_SD, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_SD, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_SD, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_SD, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_SD, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_SD, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_SD, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_SD, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_SD, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_21
    ui->DeskTop_label_21 = lv_label_create(ui->DeskTop_cont_folder);
    lv_obj_set_pos(ui->DeskTop_label_21, 280, 11);
    lv_obj_set_size(ui->DeskTop_label_21, 210, 30);
    lv_label_set_text(ui->DeskTop_label_21, "\xE6\x96\x87\xE4\xBB\xB6\xE5\x88\x97\xE8\xA1\xA8");
    lv_label_set_long_mode(ui->DeskTop_label_21, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_21, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_21, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_21, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_21, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_21, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_list_2
    ui->DeskTop_list_2 = lv_list_create(ui->DeskTop_cont_folder);
    lv_obj_set_style_text_font(ui->DeskTop_list_2, &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_pos(ui->DeskTop_list_2, 183, 47);
    lv_obj_set_size(ui->DeskTop_list_2, 400, 400);
    lv_obj_set_scrollbar_mode(ui->DeskTop_list_2, LV_SCROLLBAR_MODE_AUTO);
    ui->DeskTop_list_2_item0 = lv_list_add_button(ui->DeskTop_list_2, LV_SYMBOL_SAVE, "save_1");

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_list_2_main_main_default
    static lv_style_t style_DeskTop_list_2_main_main_default;
    ui_init_style(&style_DeskTop_list_2_main_main_default);

    lv_style_set_pad_top(&style_DeskTop_list_2_main_main_default, 9);
    lv_style_set_pad_left(&style_DeskTop_list_2_main_main_default, 8);
    lv_style_set_pad_right(&style_DeskTop_list_2_main_main_default, 8);
    lv_style_set_pad_bottom(&style_DeskTop_list_2_main_main_default, 13);
    lv_style_set_bg_opa(&style_DeskTop_list_2_main_main_default, 0);
    lv_style_set_border_width(&style_DeskTop_list_2_main_main_default, 0);
    lv_style_set_radius(&style_DeskTop_list_2_main_main_default, 0);
    lv_style_set_shadow_width(&style_DeskTop_list_2_main_main_default, 2);
    lv_style_set_shadow_color(&style_DeskTop_list_2_main_main_default, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&style_DeskTop_list_2_main_main_default, 255);
    lv_style_set_shadow_spread(&style_DeskTop_list_2_main_main_default, 1);
    lv_style_set_shadow_offset_x(&style_DeskTop_list_2_main_main_default, 1);
    lv_style_set_shadow_offset_y(&style_DeskTop_list_2_main_main_default, 1);
    lv_obj_add_style(ui->DeskTop_list_2, &style_DeskTop_list_2_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_list_2_main_scrollbar_default
    static lv_style_t style_DeskTop_list_2_main_scrollbar_default;
    ui_init_style(&style_DeskTop_list_2_main_scrollbar_default);

    lv_style_set_radius(&style_DeskTop_list_2_main_scrollbar_default, 3);
    lv_style_set_bg_opa(&style_DeskTop_list_2_main_scrollbar_default, 255);
    lv_style_set_bg_color(&style_DeskTop_list_2_main_scrollbar_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_DeskTop_list_2_main_scrollbar_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->DeskTop_list_2, &style_DeskTop_list_2_main_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_list_2_extra_btns_main_default
    static lv_style_t style_DeskTop_list_2_extra_btns_main_default;
    ui_init_style(&style_DeskTop_list_2_extra_btns_main_default);

    lv_style_set_pad_top(&style_DeskTop_list_2_extra_btns_main_default, 5);
    lv_style_set_pad_left(&style_DeskTop_list_2_extra_btns_main_default, 5);
    lv_style_set_pad_right(&style_DeskTop_list_2_extra_btns_main_default, 5);
    lv_style_set_pad_bottom(&style_DeskTop_list_2_extra_btns_main_default, 5);
    lv_style_set_border_width(&style_DeskTop_list_2_extra_btns_main_default, 0);
    lv_style_set_text_color(&style_DeskTop_list_2_extra_btns_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_DeskTop_list_2_extra_btns_main_default, &lv_font_myyh_12);
    lv_style_set_text_opa(&style_DeskTop_list_2_extra_btns_main_default, 255);
    lv_style_set_radius(&style_DeskTop_list_2_extra_btns_main_default, 3);
    lv_style_set_bg_opa(&style_DeskTop_list_2_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_DeskTop_list_2_extra_btns_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_DeskTop_list_2_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->DeskTop_list_2_item0, &style_DeskTop_list_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_DeskTop_list_2_extra_texts_main_default
    static lv_style_t style_DeskTop_list_2_extra_texts_main_default;
    ui_init_style(&style_DeskTop_list_2_extra_texts_main_default);

    lv_style_set_pad_top(&style_DeskTop_list_2_extra_texts_main_default, 5);
    lv_style_set_pad_left(&style_DeskTop_list_2_extra_texts_main_default, 5);
    lv_style_set_pad_right(&style_DeskTop_list_2_extra_texts_main_default, 5);
    lv_style_set_pad_bottom(&style_DeskTop_list_2_extra_texts_main_default, 5);
    lv_style_set_border_width(&style_DeskTop_list_2_extra_texts_main_default, 0);
    lv_style_set_text_color(&style_DeskTop_list_2_extra_texts_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_DeskTop_list_2_extra_texts_main_default, &lv_font_myyh_12);
    lv_style_set_text_opa(&style_DeskTop_list_2_extra_texts_main_default, 255);
    lv_style_set_radius(&style_DeskTop_list_2_extra_texts_main_default, 3);
    lv_style_set_transform_width(&style_DeskTop_list_2_extra_texts_main_default, 0);
    lv_style_set_bg_opa(&style_DeskTop_list_2_extra_texts_main_default, 255);
    lv_style_set_bg_color(&style_DeskTop_list_2_extra_texts_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_DeskTop_list_2_extra_texts_main_default, LV_GRAD_DIR_NONE);

    //Write codes DeskTop_tileview_WiFi1
    ui->DeskTop_tileview_WiFi1 = lv_tileview_create(ui->DeskTop_cont_setting);
    lv_obj_set_pos(ui->DeskTop_tileview_WiFi1, 236, 35);
    lv_obj_set_size(ui->DeskTop_tileview_WiFi1, 500, 260);
    lv_obj_set_scrollbar_mode(ui->DeskTop_tileview_WiFi1, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->DeskTop_tileview_WiFi1, LV_OBJ_FLAG_HIDDEN);
    ui->DeskTop_tileview_WiFi1_tile = lv_tileview_add_tile(ui->DeskTop_tileview_WiFi1, 0, 0, LV_DIR_RIGHT);

    //Write style for DeskTop_tileview_WiFi1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_tileview_WiFi1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_tileview_WiFi1, 243, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_tileview_WiFi1, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_tileview_WiFi1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_tileview_WiFi1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_tileview_WiFi1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_tileview_WiFi1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_tileview_WiFi1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_tileview_WiFi1, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_tileview_WiFi1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes DeskTop_ta_2
    ui->DeskTop_ta_2 = lv_textarea_create(ui->DeskTop_tileview_WiFi1_tile);
    lv_obj_set_pos(ui->DeskTop_ta_2, 78, 141);
    lv_obj_set_size(ui->DeskTop_ta_2, 300, 40);
    lv_textarea_set_text(ui->DeskTop_ta_2, "cqh442666");
    lv_textarea_set_placeholder_text(ui->DeskTop_ta_2, "");
    lv_textarea_set_password_bullet(ui->DeskTop_ta_2, "*");
    lv_textarea_set_password_mode(ui->DeskTop_ta_2, true);
    lv_textarea_set_one_line(ui->DeskTop_ta_2, true);
    lv_textarea_set_accepted_chars(ui->DeskTop_ta_2, "");
    lv_textarea_set_max_length(ui->DeskTop_ta_2, 128);
#if LV_USE_KEYBOARD
    lv_obj_add_event_cb(ui->DeskTop_ta_2, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif

    //Write style for DeskTop_ta_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->DeskTop_ta_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_ta_2, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_ta_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_ta_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_ta_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_ta_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_ta_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_ta_2, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_ta_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_ta_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_ta_2, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_ta_2, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_ta_2, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_ta_2, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_ta_2, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes DeskTop_ta_1
    ui->DeskTop_ta_1 = lv_textarea_create(ui->DeskTop_tileview_WiFi1_tile);
    lv_obj_set_pos(ui->DeskTop_ta_1, 81, 65);
    lv_obj_set_size(ui->DeskTop_ta_1, 300, 40);
    lv_textarea_set_text(ui->DeskTop_ta_1, "iQOO Neo9");
    lv_textarea_set_placeholder_text(ui->DeskTop_ta_1, "");
    lv_textarea_set_password_bullet(ui->DeskTop_ta_1, "*");
    lv_textarea_set_password_mode(ui->DeskTop_ta_1, false);
    lv_textarea_set_one_line(ui->DeskTop_ta_1, true);
    lv_textarea_set_accepted_chars(ui->DeskTop_ta_1, "");
    lv_textarea_set_max_length(ui->DeskTop_ta_1, 128);
#if LV_USE_KEYBOARD
    lv_obj_add_event_cb(ui->DeskTop_ta_1, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif

    //Write style for DeskTop_ta_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->DeskTop_ta_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_ta_1, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_ta_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_ta_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_ta_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_ta_1, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_ta_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_ta_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_ta_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_ta_1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_ta_1, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_ta_1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_ta_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_6
    ui->DeskTop_label_6 = lv_label_create(ui->DeskTop_tileview_WiFi1_tile);
    lv_obj_set_pos(ui->DeskTop_label_6, 131, 40);
    lv_obj_set_size(ui->DeskTop_label_6, 200, 32);
    lv_label_set_text(ui->DeskTop_label_6, "\x57\x69\x46\x69\x20\xE5\x90\x8D\xE5\xAD\x97");
    lv_label_set_long_mode(ui->DeskTop_label_6, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_6, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_6, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_6, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_5
    ui->DeskTop_label_5 = lv_label_create(ui->DeskTop_tileview_WiFi1_tile);
    lv_obj_set_pos(ui->DeskTop_label_5, 128, 117);
    lv_obj_set_size(ui->DeskTop_label_5, 200, 32);
    lv_label_set_text(ui->DeskTop_label_5, "\x57\x69\x46\x69\x20\xE5\xAF\x86\xE7\xA0\x81");
    lv_label_set_long_mode(ui->DeskTop_label_5, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_5, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_5, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_5
    ui->DeskTop_btn_5 = lv_button_create(ui->DeskTop_tileview_WiFi1_tile);
    lv_obj_set_pos(ui->DeskTop_btn_5, 438, 9);
    lv_obj_set_size(ui->DeskTop_btn_5, 50, 40);
    ui->DeskTop_btn_5_label = lv_label_create(ui->DeskTop_btn_5);
    lv_label_set_text(ui->DeskTop_btn_5_label, " " LV_SYMBOL_CLOSE " ");
    lv_label_set_long_mode(ui->DeskTop_btn_5_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_5_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_5, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_5_label, LV_PCT(100));

    //Write style for DeskTop_btn_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_5, 139, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_btn_5, lv_color_hex(0xf44444), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_btn_5, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_5, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_5, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_5, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_spinner_1
    ui->DeskTop_spinner_1 = lv_spinner_create(ui->DeskTop_tileview_WiFi1_tile);
    lv_obj_set_pos(ui->DeskTop_spinner_1, 163, 63);
    lv_obj_set_size(ui->DeskTop_spinner_1, 100, 100);
    lv_obj_add_flag(ui->DeskTop_spinner_1, LV_OBJ_FLAG_HIDDEN);
    lv_spinner_set_anim_params(ui->DeskTop_spinner_1, 2000, 200);

    //Write style for DeskTop_spinner_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_pad_top(ui->DeskTop_spinner_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_spinner_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_spinner_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_spinner_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_spinner_1, 169, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_spinner_1, lv_color_hex(0xeeeef6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_spinner_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui->DeskTop_spinner_1, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui->DeskTop_spinner_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui->DeskTop_spinner_1, lv_color_hex(0xd5d6de), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(ui->DeskTop_spinner_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_spinner_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_spinner_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_arc_width(ui->DeskTop_spinner_1, 12, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui->DeskTop_spinner_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui->DeskTop_spinner_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(ui->DeskTop_spinner_1, true, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_4
    ui->DeskTop_btn_4 = lv_button_create(ui->DeskTop_tileview_WiFi1_tile);
    lv_obj_set_pos(ui->DeskTop_btn_4, 181, 193);
    lv_obj_set_size(ui->DeskTop_btn_4, 100, 50);
    ui->DeskTop_btn_4_label = lv_label_create(ui->DeskTop_btn_4);
    lv_label_set_text(ui->DeskTop_btn_4_label, "\xE8\xBF\x9E\xE6\x8E\xA5");
    lv_label_set_long_mode(ui->DeskTop_btn_4_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_4_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_4, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_4_label, LV_PCT(100));

    //Write style for DeskTop_btn_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_4, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_4, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_4, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_4, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_4, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_4, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_3
    ui->DeskTop_btn_3 = lv_button_create(ui->DeskTop_tileview_WiFi1_tile);
    lv_obj_set_pos(ui->DeskTop_btn_3, 324, 143);
    lv_obj_set_size(ui->DeskTop_btn_3, 50, 40);
    ui->DeskTop_btn_3_label = lv_label_create(ui->DeskTop_btn_3);
    lv_label_set_text(ui->DeskTop_btn_3_label, " " LV_SYMBOL_EYE_CLOSE " ");
    lv_label_set_long_mode(ui->DeskTop_btn_3_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_3_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_3, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_3_label, LV_PCT(100));

    //Write style for DeskTop_btn_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_3, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_3, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_3, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_1
    ui->DeskTop_btn_1 = lv_button_create(ui->DeskTop_cont_setting);
    lv_obj_set_pos(ui->DeskTop_btn_1, 55, 516);
    lv_obj_set_size(ui->DeskTop_btn_1, 200, 50);
    ui->DeskTop_btn_1_label = lv_label_create(ui->DeskTop_btn_1);
    lv_label_set_text(ui->DeskTop_btn_1_label, "\xE8\xBF\x94\xE5\x9B\x9E\x20" LV_SYMBOL_HOME " ");
    lv_label_set_long_mode(ui->DeskTop_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_1_label, LV_PCT(100));

    //Write style for DeskTop_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_btn_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_1, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_1, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_1, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_1, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_tileview_Time
    ui->DeskTop_tileview_Time = lv_tileview_create(ui->DeskTop_cont_setting);
    lv_obj_set_pos(ui->DeskTop_tileview_Time, 387, 71);
    lv_obj_set_size(ui->DeskTop_tileview_Time, 500, 260);
    lv_obj_set_scrollbar_mode(ui->DeskTop_tileview_Time, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->DeskTop_tileview_Time, LV_OBJ_FLAG_HIDDEN);
    ui->DeskTop_tileview_Time_tile = lv_tileview_add_tile(ui->DeskTop_tileview_Time, 0, 0, LV_DIR_RIGHT);

    //Write style for DeskTop_tileview_Time, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_tileview_Time, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_tileview_Time, 243, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_tileview_Time, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_tileview_Time, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_tileview_Time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_tileview_Time, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_tileview_Time, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_tileview_Time, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_tileview_Time, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_tileview_Time, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes DeskTop_btn_11
    ui->DeskTop_btn_11 = lv_button_create(ui->DeskTop_tileview_Time_tile);
    lv_obj_set_pos(ui->DeskTop_btn_11, 438, 9);
    lv_obj_set_size(ui->DeskTop_btn_11, 50, 40);
    ui->DeskTop_btn_11_label = lv_label_create(ui->DeskTop_btn_11);
    lv_label_set_text(ui->DeskTop_btn_11_label, " " LV_SYMBOL_CLOSE " ");
    lv_label_set_long_mode(ui->DeskTop_btn_11_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_11_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_11, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_11_label, LV_PCT(100));

    //Write style for DeskTop_btn_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_11, 139, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_btn_11, lv_color_hex(0xf44444), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_btn_11, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_11, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_11, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_11, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_11, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_Time
    ui->DeskTop_btn_Time = lv_button_create(ui->DeskTop_tileview_Time_tile);
    lv_obj_set_pos(ui->DeskTop_btn_Time, 94, 177);
    lv_obj_set_size(ui->DeskTop_btn_Time, 100, 50);
    ui->DeskTop_btn_Time_label = lv_label_create(ui->DeskTop_btn_Time);
    lv_label_set_text(ui->DeskTop_btn_Time_label, "\xE7\xA1\xAE\xE8\xAE\xA4");
    lv_label_set_long_mode(ui->DeskTop_btn_Time_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_Time_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_Time, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_Time_label, LV_PCT(100));

    //Write style for DeskTop_btn_Time, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_Time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_Time, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_Time, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_Time, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_Time, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_Time, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_Time, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_Time, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_Time, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_Time, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_Time, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_roller_3
    ui->DeskTop_roller_3 = lv_roller_create(ui->DeskTop_tileview_Time_tile);
    lv_obj_set_pos(ui->DeskTop_roller_3, 163, 63);
    lv_obj_set_width(ui->DeskTop_roller_3, 80);
    lv_roller_set_options(ui->DeskTop_roller_3, "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59\n00", LV_ROLLER_MODE_INFINITE);

    //Write style for DeskTop_roller_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_roller_3, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_3, lv_color_hex(0x333333), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_3, &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_roller_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_roller_3, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_roller_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_roller_3, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_roller_3, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_roller_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_roller_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_roller_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_roller_3, Part: LV_PART_SELECTED, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_3, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_3, lv_color_hex(0x2195f6), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_3, LV_GRAD_DIR_NONE, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_3, lv_color_hex(0xFFFFFF), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_3, &lv_font_myyh_12, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_3, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);

    lv_roller_set_visible_row_count(ui->DeskTop_roller_3, 3);
    //Write codes DeskTop_roller_2
    ui->DeskTop_roller_2 = lv_roller_create(ui->DeskTop_tileview_Time_tile);
    lv_obj_set_pos(ui->DeskTop_roller_2, 62, 63);
    lv_obj_set_width(ui->DeskTop_roller_2, 80);
    lv_roller_set_options(ui->DeskTop_roller_2, "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n00", LV_ROLLER_MODE_INFINITE);

    //Write style for DeskTop_roller_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_roller_2, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_2, lv_color_hex(0x333333), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_2, &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_roller_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_roller_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_roller_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_roller_2, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_roller_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_roller_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_roller_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_roller_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_roller_2, Part: LV_PART_SELECTED, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_2, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_2, lv_color_hex(0x2195f6), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_2, LV_GRAD_DIR_NONE, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_2, lv_color_hex(0xFFFFFF), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_2, &lv_font_myyh_12, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_2, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);

    lv_roller_set_visible_row_count(ui->DeskTop_roller_2, 3);
    //Write codes DeskTop_label_28
    ui->DeskTop_label_28 = lv_label_create(ui->DeskTop_tileview_Time_tile);
    lv_obj_set_pos(ui->DeskTop_label_28, 159, 30);
    lv_obj_set_size(ui->DeskTop_label_28, 80, 30);
    lv_label_set_text(ui->DeskTop_label_28, "\xE5\x88\x86\xE9\x92\x9F");
    lv_label_set_long_mode(ui->DeskTop_label_28, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_28, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_28, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_28, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_28, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_28, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_28, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_28, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_28, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_28, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_28, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_28, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_28, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_28, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_28, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_28, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_27
    ui->DeskTop_label_27 = lv_label_create(ui->DeskTop_tileview_Time_tile);
    lv_obj_set_pos(ui->DeskTop_label_27, 63, 30);
    lv_obj_set_size(ui->DeskTop_label_27, 80, 30);
    lv_label_set_text(ui->DeskTop_label_27, "\xE5\xB0\x8F\xE6\x97\xB6");
    lv_label_set_long_mode(ui->DeskTop_label_27, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_27, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_27, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_27, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_27, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_27, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_27, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_27, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_27, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_27, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_27, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_27, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_27, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_27, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_27, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_27, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_29
    ui->DeskTop_label_29 = lv_label_create(ui->DeskTop_tileview_Time_tile);
    lv_obj_set_pos(ui->DeskTop_label_29, 264, 66);
    lv_obj_set_size(ui->DeskTop_label_29, 200, 150);
    lv_label_set_text(ui->DeskTop_label_29, "\xE7\x82\xB9\xE5\x87\xBB\xE7\xA1\xAE\xE8\xAE\xA4\xE5\x90\x8E\xE5\xB0\x86\xE4\xBB\x8E\xE6\xBB\x9A\xE8\xBD\xAE\xE8\xAE\xBE\xE7\xBD\xAE\xE7\x9A\x84\x5C\x6E\x5C\x6E\xE5\xB0\x8F\xE6\x97\xB6\xE5\x92\x8C\xE5\x88\x86\xE9\x92\x9F\xE5\xBC\x80\xE5\xA7\x8B\xEF\xBC\x8C\xE7\xA7\x92\xE9\x92\x88\xE5\xBD\x92\xE9\x9B\xB6");
    lv_label_set_long_mode(ui->DeskTop_label_29, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_29, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_29, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_29, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_29, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_29, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_29, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_29, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_29, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_29, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_29, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_29, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_29, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_29, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_29, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_29, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_tileview_timekeeping
    ui->DeskTop_tileview_timekeeping = lv_tileview_create(ui->DeskTop_cont_setting);
    lv_obj_set_pos(ui->DeskTop_tileview_timekeeping, 135, 41);
    lv_obj_set_size(ui->DeskTop_tileview_timekeeping, 800, 500);
    lv_obj_set_scrollbar_mode(ui->DeskTop_tileview_timekeeping, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->DeskTop_tileview_timekeeping, LV_OBJ_FLAG_HIDDEN);
    ui->DeskTop_tileview_timekeeping_tile = lv_tileview_add_tile(ui->DeskTop_tileview_timekeeping, 0, 0, LV_DIR_RIGHT);

    //Write style for DeskTop_tileview_timekeeping, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_tileview_timekeeping, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_tileview_timekeeping, 243, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_tileview_timekeeping, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_tileview_timekeeping, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_tileview_timekeeping, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_tileview_timekeeping, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_tileview_timekeeping, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_tileview_timekeeping, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_tileview_timekeeping, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_tileview_timekeeping, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes DeskTop_btn_13
    ui->DeskTop_btn_13 = lv_button_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_btn_13, 744, 7);
    lv_obj_set_size(ui->DeskTop_btn_13, 50, 40);
    ui->DeskTop_btn_13_label = lv_label_create(ui->DeskTop_btn_13);
    lv_label_set_text(ui->DeskTop_btn_13_label, " " LV_SYMBOL_CLOSE " ");
    lv_label_set_long_mode(ui->DeskTop_btn_13_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_13_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_13, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_13_label, LV_PCT(100));

    //Write style for DeskTop_btn_13, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_13, 139, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_btn_13, lv_color_hex(0xf44444), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_btn_13, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_13, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_13, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_13, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_13, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_spinner_3
    ui->DeskTop_spinner_3 = lv_spinner_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_spinner_3, 163, 63);
    lv_obj_set_size(ui->DeskTop_spinner_3, 100, 100);
    lv_obj_add_flag(ui->DeskTop_spinner_3, LV_OBJ_FLAG_HIDDEN);
    lv_spinner_set_anim_params(ui->DeskTop_spinner_3, 2000, 200);

    //Write style for DeskTop_spinner_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_spinner_3, 169, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_spinner_3, lv_color_hex(0xeeeef6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_spinner_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_spinner_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_spinner_3, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_spinner_3, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_spinner_3, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_spinner_3, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui->DeskTop_spinner_3, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui->DeskTop_spinner_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui->DeskTop_spinner_3, lv_color_hex(0xd5d6de), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(ui->DeskTop_spinner_3, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_spinner_3, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_arc_width(ui->DeskTop_spinner_3, 12, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui->DeskTop_spinner_3, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui->DeskTop_spinner_3, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(ui->DeskTop_spinner_3, true, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_confirm
    ui->DeskTop_btn_confirm = lv_button_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_btn_confirm, 117, 173);
    lv_obj_set_size(ui->DeskTop_btn_confirm, 100, 50);
    ui->DeskTop_btn_confirm_label = lv_label_create(ui->DeskTop_btn_confirm);
    lv_label_set_text(ui->DeskTop_btn_confirm_label, "\xE7\xA1\xAE\xE8\xAE\xA4");
    lv_label_set_long_mode(ui->DeskTop_btn_confirm_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_confirm_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_confirm, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_confirm_label, LV_PCT(100));

    //Write style for DeskTop_btn_confirm, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_btn_confirm, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_confirm, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_confirm, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_confirm, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_confirm, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_confirm, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_confirm, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_confirm, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_confirm, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_confirm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_confirm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_32
    ui->DeskTop_label_32 = lv_label_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_label_32, 159, 30);
    lv_obj_set_size(ui->DeskTop_label_32, 80, 30);
    lv_label_set_text(ui->DeskTop_label_32, "\xE5\x88\x86");
    lv_label_set_long_mode(ui->DeskTop_label_32, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_32, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_32, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_32, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_32, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_32, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_32, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_32, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_32, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_32, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_32, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_32, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_32, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_32, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_32, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_32, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_31
    ui->DeskTop_label_31 = lv_label_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_label_31, 61, 30);
    lv_obj_set_size(ui->DeskTop_label_31, 80, 30);
    lv_label_set_text(ui->DeskTop_label_31, "\xE6\x97\xB6");
    lv_label_set_long_mode(ui->DeskTop_label_31, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_31, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_label_31, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_31, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_31, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_31, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_31, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_31, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_31, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_31, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_label_31, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_31, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_31, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_31, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_31, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_31, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_roller_5
    ui->DeskTop_roller_5 = lv_roller_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_roller_5, 62, 61);
    lv_obj_set_width(ui->DeskTop_roller_5, 80);
    lv_roller_set_options(ui->DeskTop_roller_5, "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n00", LV_ROLLER_MODE_INFINITE);

    //Write style for DeskTop_roller_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_roller_5, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_5, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_5, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_5, lv_color_hex(0x333333), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_5, &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_roller_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_roller_5, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_roller_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_roller_5, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_roller_5, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_roller_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_roller_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_roller_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_roller_5, Part: LV_PART_SELECTED, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_5, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_5, lv_color_hex(0x2195f6), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_5, LV_GRAD_DIR_NONE, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_5, lv_color_hex(0xFFFFFF), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_5, &lv_font_myyh_12, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_5, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);

    lv_roller_set_visible_row_count(ui->DeskTop_roller_5, 3);
    //Write codes DeskTop_roller_4
    ui->DeskTop_roller_4 = lv_roller_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_roller_4, 161, 61);
    lv_obj_set_width(ui->DeskTop_roller_4, 80);
    lv_roller_set_options(ui->DeskTop_roller_4, "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59\n00", LV_ROLLER_MODE_INFINITE);

    //Write style for DeskTop_roller_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_roller_4, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_roller_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_roller_4, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_roller_4, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_roller_4, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_4, lv_color_hex(0x333333), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_4, &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_roller_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_4, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_roller_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_roller_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_roller_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_roller_4, Part: LV_PART_SELECTED, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_4, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_4, lv_color_hex(0x2195f6), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_4, LV_GRAD_DIR_NONE, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_4, lv_color_hex(0xFFFFFF), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_4, &lv_font_myyh_12, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_4, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);

    lv_roller_set_visible_row_count(ui->DeskTop_roller_4, 3);
    //Write codes DeskTop_label_30
    ui->DeskTop_label_30 = lv_label_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_label_30, 375, 84);
    lv_obj_set_size(ui->DeskTop_label_30, 200, 40);
    lv_label_set_text(ui->DeskTop_label_30, "\xE4\xB8\x8A\xE6\xAC\xA1\xE9\xA2\x84\xE8\xAE\xBE\xE8\xAE\xA1\xE6\x97\xB6\xEF\xBC\x9A\x30\x30\x3A\x30\x31\x3A\x30\x30\x5C\x6E");
    lv_label_set_long_mode(ui->DeskTop_label_30, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_30, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_30, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_30, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_30, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_30, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_30, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_30, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_30, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_30, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_30, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_30, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_30, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_30, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_30, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_30, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_33
    ui->DeskTop_label_33 = lv_label_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_label_33, 260, 32);
    lv_obj_set_size(ui->DeskTop_label_33, 80, 30);
    lv_label_set_text(ui->DeskTop_label_33, "\xE7\xA7\x92");
    lv_label_set_long_mode(ui->DeskTop_label_33, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_33, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_label_33, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_33, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_33, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_label_33, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_33, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_33, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_33, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_33, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_33, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_33, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_33, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_33, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_33, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_33, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_roller_6
    ui->DeskTop_roller_6 = lv_roller_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_roller_6, 260, 63);
    lv_obj_set_width(ui->DeskTop_roller_6, 80);
    lv_roller_set_options(ui->DeskTop_roller_6, "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59\n00", LV_ROLLER_MODE_INFINITE);

    //Write style for DeskTop_roller_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_roller_6, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_roller_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_roller_6, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_roller_6, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_roller_6, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_6, lv_color_hex(0x333333), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_6, &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_roller_6, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_6, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_6, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_roller_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_roller_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_roller_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_roller_6, Part: LV_PART_SELECTED, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_6, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_6, lv_color_hex(0x2195f6), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_6, LV_GRAD_DIR_NONE, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_6, lv_color_hex(0xFFFFFF), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_6, &lv_font_myyh_12, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_6, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);

    lv_roller_set_visible_row_count(ui->DeskTop_roller_6, 3);
    //Write codes DeskTop_btn_pause
    ui->DeskTop_btn_pause = lv_button_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_btn_pause, 118, 236);
    lv_obj_set_size(ui->DeskTop_btn_pause, 100, 50);
    ui->DeskTop_btn_pause_label = lv_label_create(ui->DeskTop_btn_pause);
    lv_label_set_text(ui->DeskTop_btn_pause_label, "\xE6\x9A\x82\xE5\x81\x9C");
    lv_label_set_long_mode(ui->DeskTop_btn_pause_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_pause_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_pause, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_pause_label, LV_PCT(100));

    //Write style for DeskTop_btn_pause, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_btn_pause, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_pause, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_pause, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_pause, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_pause, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_pause, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_pause, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_pause, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_pause, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_pause, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_pause, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_reset
    ui->DeskTop_btn_reset = lv_button_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_btn_reset, 115, 301);
    lv_obj_set_size(ui->DeskTop_btn_reset, 100, 50);
    ui->DeskTop_btn_reset_label = lv_label_create(ui->DeskTop_btn_reset);
    lv_label_set_text(ui->DeskTop_btn_reset_label, "\xE9\x87\x8D\xE7\xBD\xAE");
    lv_label_set_long_mode(ui->DeskTop_btn_reset_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_reset_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_reset, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_reset_label, LV_PCT(100));

    //Write style for DeskTop_btn_reset, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_btn_reset, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_reset, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_reset, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_reset, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_reset, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_reset, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_reset, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_reset, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_reset, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_reset, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_reset, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_record
    ui->DeskTop_btn_record = lv_button_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_btn_record, 116, 369);
    lv_obj_set_size(ui->DeskTop_btn_record, 100, 50);
    ui->DeskTop_btn_record_label = lv_label_create(ui->DeskTop_btn_record);
    lv_label_set_text(ui->DeskTop_btn_record_label, "\xE6\xA0\x87\xE8\xAE\xB0");
    lv_label_set_long_mode(ui->DeskTop_btn_record_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_record_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_record, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_record_label, LV_PCT(100));

    //Write style for DeskTop_btn_record, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_btn_record, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_record, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_record, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_record, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_record, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_record, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_record, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_record, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_record, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_record, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_record, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_timeupdate
    ui->DeskTop_label_timeupdate = lv_label_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_label_timeupdate, 364, 175);
    lv_obj_set_size(ui->DeskTop_label_timeupdate, 200, 40);
    lv_label_set_text(ui->DeskTop_label_timeupdate, "00 : 01 : 00");
    lv_label_set_long_mode(ui->DeskTop_label_timeupdate, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_timeupdate, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_timeupdate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_timeupdate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_timeupdate, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_timeupdate, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_timeupdate, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_timeupdate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_timeupdate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_timeupdate, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_timeupdate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_timeupdate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_timeupdate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_timeupdate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_timeupdate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_timeupdate, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_34_scroll (scrollable container for lap records)
    ui->DeskTop_label_34_scroll = lv_obj_create(ui->DeskTop_tileview_timekeeping_tile);
    lv_obj_set_pos(ui->DeskTop_label_34_scroll, 315, 230);
    lv_obj_set_size(ui->DeskTop_label_34_scroll, 400, 250);
    lv_obj_set_scrollbar_mode(ui->DeskTop_label_34_scroll, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_scroll_dir(ui->DeskTop_label_34_scroll, LV_DIR_VER);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_34_scroll, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_label_34_scroll, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->DeskTop_label_34_scroll, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_34_scroll, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_34
    ui->DeskTop_label_34 = lv_label_create(ui->DeskTop_label_34_scroll);
    lv_obj_set_width(ui->DeskTop_label_34, 380);
    lv_label_set_long_mode(ui->DeskTop_label_34, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(ui->DeskTop_label_34, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_34, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_34, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_label_set_text(ui->DeskTop_label_34, "\xe8\xae\xb0\xe5\xbd\x95\xe7\x82\xb9\xef\xbc\x9a");


    //Write codes DeskTop_tileview_alarm
    ui->DeskTop_tileview_alarm = lv_tileview_create(ui->DeskTop_cont_setting);
    lv_obj_set_pos(ui->DeskTop_tileview_alarm, 371, 85);
    lv_obj_set_size(ui->DeskTop_tileview_alarm, 500, 260);
    lv_obj_set_scrollbar_mode(ui->DeskTop_tileview_alarm, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->DeskTop_tileview_alarm, LV_OBJ_FLAG_HIDDEN);
    ui->DeskTop_tileview_alarm_tile = lv_tileview_add_tile(ui->DeskTop_tileview_alarm, 0, 0, LV_DIR_RIGHT);

    //Write style for DeskTop_tileview_alarm, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_tileview_alarm, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_tileview_alarm, 243, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_tileview_alarm, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_tileview_alarm, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_tileview_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_tileview_alarm, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_tileview_alarm, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_tileview_alarm, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_tileview_alarm, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_tileview_alarm, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes DeskTop_btn_15
    ui->DeskTop_btn_15 = lv_button_create(ui->DeskTop_tileview_alarm_tile);
    lv_obj_set_pos(ui->DeskTop_btn_15, 438, 9);
    lv_obj_set_size(ui->DeskTop_btn_15, 50, 40);
    ui->DeskTop_btn_15_label = lv_label_create(ui->DeskTop_btn_15);
    lv_label_set_text(ui->DeskTop_btn_15_label, " " LV_SYMBOL_CLOSE " ");
    lv_label_set_long_mode(ui->DeskTop_btn_15_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_15_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_15, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_15_label, LV_PCT(100));

    //Write style for DeskTop_btn_15, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_15, 139, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_btn_15, lv_color_hex(0xf44444), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_btn_15, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_15, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_15, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_15, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_15, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_15, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_alarm
    ui->DeskTop_btn_alarm = lv_button_create(ui->DeskTop_tileview_alarm_tile);
lv_obj_set_pos(ui->DeskTop_btn_alarm, 46, 176);
    lv_obj_set_size(ui->DeskTop_btn_alarm, 100, 50);
    ui->DeskTop_btn_alarm_label = lv_label_create(ui->DeskTop_btn_alarm);
    lv_label_set_text(ui->DeskTop_btn_alarm_label, "\xE7\xA1\xAE\xE8\xAE\xA4");
    lv_label_set_long_mode(ui->DeskTop_btn_alarm_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_alarm_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_alarm, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_alarm_label, LV_PCT(100));

    //Write style for DeskTop_btn_alarm, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_alarm, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_alarm, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_alarm, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_alarm, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_alarm, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_alarm, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_alarm, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_alarm, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_alarm, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_btn_alarm_del
    ui->DeskTop_btn_alarm_del = lv_button_create(ui->DeskTop_tileview_alarm_tile);
    lv_obj_set_pos(ui->DeskTop_btn_alarm_del, 163, 176);
    lv_obj_set_size(ui->DeskTop_btn_alarm_del, 100, 50);
    ui->DeskTop_btn_alarm_del_label = lv_label_create(ui->DeskTop_btn_alarm_del);
    lv_label_set_text(ui->DeskTop_btn_alarm_del_label, "\xe5\x88\xa0\xe9\x99\xa4");
    lv_label_set_long_mode(ui->DeskTop_btn_alarm_del_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DeskTop_btn_alarm_del_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DeskTop_btn_alarm_del, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DeskTop_btn_alarm_del_label, LV_PCT(100));

    //Write style for DeskTop_btn_alarm_del, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_btn_alarm_del, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_btn_alarm_del, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_btn_alarm_del, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_btn_alarm_del, lv_color_hex(0xff4444), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_btn_alarm_del, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_btn_alarm_del, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_btn_alarm_del, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_btn_alarm_del, lv_color_hex(0xff4444), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_btn_alarm_del, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_btn_alarm_del, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_btn_alarm_del, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_roller_alarm_h
    ui->DeskTop_roller_alarm_h = lv_roller_create(ui->DeskTop_tileview_alarm_tile);
    lv_obj_set_pos(ui->DeskTop_roller_alarm_h, 62, 63);
    lv_obj_set_width(ui->DeskTop_roller_alarm_h, 80);
    lv_roller_set_options(ui->DeskTop_roller_alarm_h, "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n00", LV_ROLLER_MODE_INFINITE);

    //Write style for DeskTop_roller_alarm_h, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_roller_alarm_h, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_alarm_h, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_alarm_h, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_alarm_h, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_alarm_h, lv_color_hex(0x333333), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_alarm_h, &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_alarm_h, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_roller_alarm_h, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_roller_alarm_h, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_roller_alarm_h, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_roller_alarm_h, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_roller_alarm_h, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_roller_alarm_h, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_roller_alarm_h, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_roller_alarm_h, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_roller_alarm_h, Part: LV_PART_SELECTED, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_alarm_h, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_alarm_h, lv_color_hex(0x2195f6), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_alarm_h, LV_GRAD_DIR_NONE, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_alarm_h, lv_color_hex(0xFFFFFF), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_alarm_h, &lv_font_myyh_12, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_alarm_h, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);

    lv_roller_set_visible_row_count(ui->DeskTop_roller_alarm_h, 3);
    //Write codes DeskTop_label_25
    ui->DeskTop_label_25 = lv_label_create(ui->DeskTop_tileview_alarm_tile);
    lv_obj_set_pos(ui->DeskTop_label_25, 63, 30);
    lv_obj_set_size(ui->DeskTop_label_25, 80, 30);
    lv_label_set_text(ui->DeskTop_label_25, "\xE5\xB0\x8F\xE6\x97\xB6");
    lv_label_set_long_mode(ui->DeskTop_label_25, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_25, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_25, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_25, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_25, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_25, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_26
    ui->DeskTop_label_26 = lv_label_create(ui->DeskTop_tileview_alarm_tile);
    lv_obj_set_pos(ui->DeskTop_label_26, 159, 30);
    lv_obj_set_size(ui->DeskTop_label_26, 80, 30);
    lv_label_set_text(ui->DeskTop_label_26, "\xE5\x88\x86\xE9\x92\x9F");
    lv_label_set_long_mode(ui->DeskTop_label_26, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_26, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_26, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_26, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_26, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_26, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_alarm
    ui->DeskTop_label_alarm = lv_label_create(ui->DeskTop_tileview_alarm_tile);
    lv_obj_set_pos(ui->DeskTop_label_alarm, 269, 60);
    lv_obj_set_size(ui->DeskTop_label_alarm, 200, 150);
    lv_label_set_text(ui->DeskTop_label_alarm, "\xE6\xAD\xA3\xE5\x9C\xA8\xE8\xAE\xA1\xE6\x97\xB6\xE7\x9A\x84\xE9\x97\xB9\xE9\x92\x9F\xE5\x88\x97\xE8\xA1\xA8\xEF\xBC\x9A\x5C\x6E\x2D\x2D\xEF\xBC\x9A\x2D\x2D\x5C\x6E\x2D\x2D\xEF\xBC\x9A\x2D\x2D\x5C\x6E");
    lv_label_set_long_mode(ui->DeskTop_label_alarm, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_alarm, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_alarm, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_alarm, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_alarm, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_alarm, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_alarm, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_roller_1
    ui->DeskTop_roller_1 = lv_roller_create(ui->DeskTop_tileview_alarm_tile);
    lv_obj_set_pos(ui->DeskTop_roller_1, 163, 63);
    lv_obj_set_width(ui->DeskTop_roller_1, 80);
    lv_roller_set_options(ui->DeskTop_roller_1, "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59\n00", LV_ROLLER_MODE_INFINITE);

    //Write style for DeskTop_roller_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_roller_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_1, lv_color_hex(0x333333), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_1, &lv_font_myyh_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_roller_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_roller_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_roller_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_roller_1, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_roller_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_roller_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_roller_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_roller_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for DeskTop_roller_1, Part: LV_PART_SELECTED, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_roller_1, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DeskTop_roller_1, lv_color_hex(0x2195f6), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DeskTop_roller_1, LV_GRAD_DIR_NONE, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_roller_1, lv_color_hex(0xFFFFFF), LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_roller_1, &lv_font_myyh_12, LV_PART_SELECTED|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_roller_1, 255, LV_PART_SELECTED|LV_STATE_DEFAULT);

    lv_roller_set_visible_row_count(ui->DeskTop_roller_1, 3);
    //Write codes DeskTop_cont_navigation_bar
    ui->DeskTop_cont_navigation_bar = lv_obj_create(ui->DeskTop);
    lv_obj_set_pos(ui->DeskTop_cont_navigation_bar, 0, 0);
    lv_obj_set_size(ui->DeskTop_cont_navigation_bar, 1024, 35);
    lv_obj_set_scrollbar_mode(ui->DeskTop_cont_navigation_bar, LV_SCROLLBAR_MODE_OFF);

    //Write style for DeskTop_cont_navigation_bar, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DeskTop_cont_navigation_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_cont_navigation_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_cont_navigation_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_cont_navigation_bar, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->DeskTop_cont_navigation_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->DeskTop_cont_navigation_bar, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->DeskTop_cont_navigation_bar, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_cont_navigation_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_cont_navigation_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_cont_navigation_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_cont_navigation_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_navigation_bar2
    ui->DeskTop_label_navigation_bar2 = lv_label_create(ui->DeskTop_cont_navigation_bar);
    lv_obj_set_pos(ui->DeskTop_label_navigation_bar2, 810, 5);
    lv_obj_set_size(ui->DeskTop_label_navigation_bar2, 210, 30);
    lv_label_set_text(ui->DeskTop_label_navigation_bar2, "   " LV_SYMBOL_WIFI "  " LV_SYMBOL_VOLUME_MAX " " LV_SYMBOL_USB " " LV_SYMBOL_BATTERY_FULL "  100%");
    lv_label_set_long_mode(ui->DeskTop_label_navigation_bar2, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_navigation_bar2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_navigation_bar2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_navigation_bar2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_navigation_bar2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_navigation_bar2, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_navigation_bar2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_navigation_bar2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_navigation_bar2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_navigation_bar2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_navigation_bar2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_navigation_bar2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_navigation_bar2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_navigation_bar2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_navigation_bar2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_navigation_bar2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_navigation_bar_flag
    ui->DeskTop_label_navigation_bar_flag = lv_label_create(ui->DeskTop_cont_navigation_bar);
    lv_obj_set_pos(ui->DeskTop_label_navigation_bar_flag, 462, -141);
    lv_obj_set_size(ui->DeskTop_label_navigation_bar_flag, 100, 32);
    lv_label_set_text(ui->DeskTop_label_navigation_bar_flag, "");
    lv_label_set_long_mode(ui->DeskTop_label_navigation_bar_flag, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_navigation_bar_flag, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_label_navigation_bar_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_navigation_bar_flag, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_navigation_bar_flag, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_navigation_bar_flag, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_navigation_bar_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_navigation_bar_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_navigation_bar_flag, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_navigation_bar_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_navigation_bar_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_navigation_bar_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_navigation_bar_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_navigation_bar_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_navigation_bar_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_label_navigation_bar_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_digital_clock_1
    static bool DeskTop_digital_clock_1_timer_enabled = false;
    ui->DeskTop_digital_clock_1 = lv_label_create(ui->DeskTop_cont_navigation_bar);
    lv_obj_set_pos(ui->DeskTop_digital_clock_1, 0, 0);
    lv_obj_set_size(ui->DeskTop_digital_clock_1, 100, 35);
    lv_label_set_text(ui->DeskTop_digital_clock_1, "15:08:06");
    if (!DeskTop_digital_clock_1_timer_enabled) {
        lv_timer_create(DeskTop_digital_clock_1_timer, 1000, NULL);
        DeskTop_digital_clock_1_timer_enabled = true;
    }

    //Write style for DeskTop_digital_clock_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_digital_clock_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_digital_clock_1, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_digital_clock_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_digital_clock_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_digital_clock_1, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_23
    ui->DeskTop_label_23 = lv_label_create(ui->DeskTop_cont_navigation_bar);
    lv_obj_set_pos(ui->DeskTop_label_23, 496, 5);
    lv_obj_set_size(ui->DeskTop_label_23, 150, 30);
    lv_label_set_text(ui->DeskTop_label_23, "\xE9\x97\xB9\xE9\x92\x9F\xEF\xBC\x9A\x2D\x2D\xEF\xBC\x9A\x2D\x2D");
    lv_label_set_long_mode(ui->DeskTop_label_23, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_23, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DeskTop_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DeskTop_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_23, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_23, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_23, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_23, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DeskTop_label_24
    ui->DeskTop_label_24 = lv_label_create(ui->DeskTop_cont_navigation_bar);
    lv_obj_set_pos(ui->DeskTop_label_24, 650, 5);
    lv_obj_set_size(ui->DeskTop_label_24, 150, 30);
    lv_label_set_text(ui->DeskTop_label_24, "\xE7\xA7\x92\xE8\xA1\xA8\xEF\xBC\x9A\x2D\x2D\xEF\xBC\x9A\x2D\x2D\xEF\xBC\x9A\x2D\x2D");
    lv_label_set_long_mode(ui->DeskTop_label_24, LV_LABEL_LONG_WRAP);

    //Write style for DeskTop_label_24, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->DeskTop_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DeskTop_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DeskTop_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DeskTop_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DeskTop_label_24, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DeskTop_label_24, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DeskTop_label_24, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DeskTop_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DeskTop_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DeskTop_label_24, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DeskTop_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DeskTop_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DeskTop_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DeskTop_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of DeskTop.


    //Update current screen layout.
    lv_obj_update_layout(ui->DeskTop);

    //Init events for screen.
    events_init_DeskTop(ui);
}
