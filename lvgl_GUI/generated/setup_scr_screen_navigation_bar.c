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



int screen_navigation_bar_digital_clock_1_min_value = 8;
int screen_navigation_bar_digital_clock_1_hour_value = 15;
int screen_navigation_bar_digital_clock_1_sec_value = 6;
void setup_scr_screen_navigation_bar(lv_ui *ui)
{
    //Write codes screen_navigation_bar
    ui->screen_navigation_bar = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_navigation_bar, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->screen_navigation_bar, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_navigation_bar, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(ui->screen_navigation_bar, norflash_get_wallpaper_dsc(), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_opa(ui->screen_navigation_bar, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_recolor(ui->screen_navigation_bar, lv_color_hex(0x252525), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_recolor_opa(ui->screen_navigation_bar, 94, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_cont_navigation_bar_return
    ui->screen_navigation_bar_cont_navigation_bar_return = lv_obj_create(ui->screen_navigation_bar);
    lv_obj_set_pos(ui->screen_navigation_bar_cont_navigation_bar_return, 10, 10);
    lv_obj_set_size(ui->screen_navigation_bar_cont_navigation_bar_return, 1000, 580);
    lv_obj_set_scrollbar_mode(ui->screen_navigation_bar_cont_navigation_bar_return, LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_scroll_dir(ui->screen_navigation_bar_cont_navigation_bar_return, LV_DIR_BOTTOM | LV_DIR_HOR);
    //Write style for screen_navigation_bar_cont_navigation_bar_return, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_navigation_bar_cont_navigation_bar_return, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_cont_navigation_bar_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_cont_navigation_bar_return, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_cont_navigation_bar_return, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_cont_navigation_bar_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_cont_navigation_bar_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_cont_navigation_bar_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_navigation_bar_cont_navigation_bar_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_cont_navigation_bar_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_cont_navigation_bar_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_cont_navigation_bar_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_tileview_2
    ui->screen_navigation_bar_tileview_2 = lv_tileview_create(ui->screen_navigation_bar_cont_navigation_bar_return);
    lv_obj_set_pos(ui->screen_navigation_bar_tileview_2, 391, 40);
    lv_obj_set_size(ui->screen_navigation_bar_tileview_2, 300, 200);
    lv_obj_set_scrollbar_mode(ui->screen_navigation_bar_tileview_2, LV_SCROLLBAR_MODE_OFF);
    ui->screen_navigation_bar_tileview_2_tile = lv_tileview_add_tile(ui->screen_navigation_bar_tileview_2, 0, 0, LV_DIR_RIGHT);

    //Write style for screen_navigation_bar_tileview_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_tileview_2, 77, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_tileview_2, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_tileview_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_tileview_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_tileview_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_navigation_bar_tileview_2, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_tileview_2, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_tileview_2, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_tileview_2, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_tileview_2, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes screen_navigation_bar_btn_2
    ui->screen_navigation_bar_btn_2 = lv_button_create(ui->screen_navigation_bar_tileview_2_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_2, 30, 14);
    lv_obj_set_size(ui->screen_navigation_bar_btn_2, 80, 80);
    ui->screen_navigation_bar_btn_2_label = lv_label_create(ui->screen_navigation_bar_btn_2);
    lv_label_set_text(ui->screen_navigation_bar_btn_2_label, " " LV_SYMBOL_WIFI " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_2_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_2_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_2, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_2_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_2, lv_color_hex(0x0098f2), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_2, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_2, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_btn_3
    ui->screen_navigation_bar_btn_3 = lv_button_create(ui->screen_navigation_bar_tileview_2_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_3, 155, 15);
    lv_obj_set_size(ui->screen_navigation_bar_btn_3, 80, 80);
    ui->screen_navigation_bar_btn_3_label = lv_label_create(ui->screen_navigation_bar_btn_3);
    lv_label_set_text(ui->screen_navigation_bar_btn_3_label, " " LV_SYMBOL_BLUETOOTH " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_3_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_3_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_3, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_3_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_3, lv_color_hex(0xa9a9a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_3, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_btn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_btn_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_btn_3, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_3, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_3, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_btn_4
    ui->screen_navigation_bar_btn_4 = lv_button_create(ui->screen_navigation_bar_tileview_2_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_4, 31, 107);
    lv_obj_set_size(ui->screen_navigation_bar_btn_4, 80, 80);
    ui->screen_navigation_bar_btn_4_label = lv_label_create(ui->screen_navigation_bar_btn_4);
    lv_label_set_text(ui->screen_navigation_bar_btn_4_label, " " LV_SYMBOL_POWER " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_4_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_4_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_4, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_4_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_4, lv_color_hex(0xa9a9a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_4, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_4, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_btn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_btn_4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_btn_4, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_4, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_4, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_4, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_btn_5
    ui->screen_navigation_bar_btn_5 = lv_button_create(ui->screen_navigation_bar_tileview_2_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_5, 153, 109);
    lv_obj_set_size(ui->screen_navigation_bar_btn_5, 80, 80);
    ui->screen_navigation_bar_btn_5_label = lv_label_create(ui->screen_navigation_bar_btn_5);
    lv_label_set_text(ui->screen_navigation_bar_btn_5_label, " " LV_SYMBOL_MUTE " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_5_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_5_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_5, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_5_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_5, lv_color_hex(0x0098f2), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_5, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_5, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_btn_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_btn_5, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_btn_5, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_5, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_5, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_5, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_tileview_1
    ui->screen_navigation_bar_tileview_1 = lv_tileview_create(ui->screen_navigation_bar_cont_navigation_bar_return);
    lv_obj_set_pos(ui->screen_navigation_bar_tileview_1, 60, 40);
    lv_obj_set_size(ui->screen_navigation_bar_tileview_1, 300, 200);
    lv_obj_set_scrollbar_mode(ui->screen_navigation_bar_tileview_1, LV_SCROLLBAR_MODE_OFF);
    ui->screen_navigation_bar_tileview_1_tile = lv_tileview_add_tile(ui->screen_navigation_bar_tileview_1, 0, 0, LV_DIR_RIGHT);

    //Write style for screen_navigation_bar_tileview_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_tileview_1, 77, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_tileview_1, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_tileview_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_tileview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_tileview_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_navigation_bar_tileview_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_tileview_1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_tileview_1, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_tileview_1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_tileview_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes screen_navigation_bar_datetext_1
    ui->screen_navigation_bar_datetext_1 = lv_label_create(ui->screen_navigation_bar_tileview_1_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_datetext_1, 50, 40);
    lv_obj_set_size(ui->screen_navigation_bar_datetext_1, 200, 35);
    lv_label_set_text(ui->screen_navigation_bar_datetext_1, "2026/05/09");
    lv_obj_set_style_text_align(ui->screen_navigation_bar_datetext_1, LV_TEXT_ALIGN_CENTER, 0);    /* CLICKABLE removed - date synced from RTC */
    lv_obj_add_event_cb(ui->screen_navigation_bar_datetext_1, screen_navigation_bar_datetext_1_event_handler, LV_EVENT_ALL, NULL);

    //Write style for screen_navigation_bar_datetext_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_navigation_bar_datetext_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_datetext_1, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_datetext_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_navigation_bar_datetext_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_datetext_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_datetext_1, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_digital_clock_1
    static bool screen_navigation_bar_digital_clock_1_timer_enabled = false;
    ui->screen_navigation_bar_digital_clock_1 = lv_label_create(ui->screen_navigation_bar_tileview_1_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_digital_clock_1, 94, 102);
    lv_obj_set_size(ui->screen_navigation_bar_digital_clock_1, 100, 50);
    lv_label_set_text(ui->screen_navigation_bar_digital_clock_1, "15:08");
    /* 删除旧时钟定时器，创建新定时器（屏幕重建时重新绑定正确的label对象） */
    {
        static lv_timer_t * clock_timer = NULL;
        if (clock_timer) { lv_timer_delete(clock_timer); }
        clock_timer = lv_timer_create(screen_navigation_bar_digital_clock_1_timer, 1000, NULL);
    }

    //Write style for screen_navigation_bar_digital_clock_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_navigation_bar_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_digital_clock_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_digital_clock_1, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_digital_clock_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_navigation_bar_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_digital_clock_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_digital_clock_1, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_navigation_bar_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_label_week
    ui->screen_navigation_bar_label_week = lv_label_create(ui->screen_navigation_bar_tileview_1_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_label_week, 67, 159);
    lv_obj_set_size(ui->screen_navigation_bar_label_week, 150, 30);
    lv_label_set_text(ui->screen_navigation_bar_label_week, "\xE6\x98\x9F\xE6\x9C\x9F\xE5\x85\xAD");
    lv_label_set_long_mode(ui->screen_navigation_bar_label_week, LV_LABEL_LONG_WRAP);

    //Write style for screen_navigation_bar_label_week, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_navigation_bar_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_label_week, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_label_week, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_label_week, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_navigation_bar_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_navigation_bar_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_label_week, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_navigation_bar_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_label_week, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_label_navigation_bar_return_flag
    ui->screen_navigation_bar_label_navigation_bar_return_flag = lv_label_create(ui->screen_navigation_bar_cont_navigation_bar_return);
    lv_obj_set_pos(ui->screen_navigation_bar_label_navigation_bar_return_flag, 423, 712);
    lv_obj_set_size(ui->screen_navigation_bar_label_navigation_bar_return_flag, 100, 32);
    lv_label_set_text(ui->screen_navigation_bar_label_navigation_bar_return_flag, "");
    lv_label_set_long_mode(ui->screen_navigation_bar_label_navigation_bar_return_flag, LV_LABEL_LONG_WRAP);

    //Write style for screen_navigation_bar_label_navigation_bar_return_flag, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_navigation_bar_label_navigation_bar_return_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_label_navigation_bar_return_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_label_navigation_bar_return_flag, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_label_navigation_bar_return_flag, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_label_navigation_bar_return_flag, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_navigation_bar_label_navigation_bar_return_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_navigation_bar_label_navigation_bar_return_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_label_navigation_bar_return_flag, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_label_navigation_bar_return_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_label_navigation_bar_return_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_label_navigation_bar_return_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_navigation_bar_label_navigation_bar_return_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_label_navigation_bar_return_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_label_navigation_bar_return_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_slider_2
    ui->screen_navigation_bar_slider_2 = lv_slider_create(ui->screen_navigation_bar_cont_navigation_bar_return);
    lv_obj_set_pos(ui->screen_navigation_bar_slider_2, 913, 78);
    lv_obj_set_size(ui->screen_navigation_bar_slider_2, 60, 429);
    lv_slider_set_range(ui->screen_navigation_bar_slider_2, 0, 100);
    lv_slider_set_mode(ui->screen_navigation_bar_slider_2, LV_SLIDER_MODE_NORMAL);
    lv_slider_set_value(ui->screen_navigation_bar_slider_2, 50, LV_ANIM_OFF);

    //Write style for screen_navigation_bar_slider_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_slider_2, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_slider_2, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_slider_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_slider_2, 50, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->screen_navigation_bar_slider_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_slider_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_navigation_bar_slider_2, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_slider_2, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_slider_2, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_slider_2, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_slider_2, 50, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for screen_navigation_bar_slider_2, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_slider_2, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_slider_2, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_slider_2, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_slider_2, 50, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_slider_1
    ui->screen_navigation_bar_slider_1 = lv_slider_create(ui->screen_navigation_bar_cont_navigation_bar_return);
    lv_obj_set_pos(ui->screen_navigation_bar_slider_1, 815, 78);
    lv_obj_set_size(ui->screen_navigation_bar_slider_1, 60, 429);
    lv_slider_set_range(ui->screen_navigation_bar_slider_1, 0, 100);
    lv_slider_set_mode(ui->screen_navigation_bar_slider_1, LV_SLIDER_MODE_NORMAL);
        /* 从当前PWM值恢复滑块位置 */
    {
        extern TIM_HandleTypeDef htim3;
        uint16_t pwm = __HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_2);
        int32_t pct = (int32_t)(pwm * 100 / 999);
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        lv_slider_set_value(ui->screen_navigation_bar_slider_1, pct, LV_ANIM_OFF);
    }

    //Write style for screen_navigation_bar_slider_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_navigation_bar_slider_1, 50, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_slider_1, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_slider_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_slider_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_slider_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->screen_navigation_bar_slider_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_navigation_bar_slider_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_slider_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_slider_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_slider_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_slider_1, 50, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for screen_navigation_bar_slider_1, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_slider_1, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_slider_1, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_slider_1, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_slider_1, 50, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_label_5
    ui->screen_navigation_bar_label_5 = lv_label_create(ui->screen_navigation_bar_cont_navigation_bar_return);
    lv_obj_set_pos(ui->screen_navigation_bar_label_5, 805, 66);
    lv_obj_set_size(ui->screen_navigation_bar_label_5, 80, 80);
    lv_label_set_text(ui->screen_navigation_bar_label_5, "\n " LV_SYMBOL_CHARGE " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_label_5, LV_LABEL_LONG_WRAP);

    //Write style for screen_navigation_bar_label_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_navigation_bar_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_label_5, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_label_5, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_label_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_navigation_bar_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_navigation_bar_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_label_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_navigation_bar_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_label_6
    ui->screen_navigation_bar_label_6 = lv_label_create(ui->screen_navigation_bar_cont_navigation_bar_return);
    lv_obj_set_pos(ui->screen_navigation_bar_label_6, 907, 66);
    lv_obj_set_size(ui->screen_navigation_bar_label_6, 80, 80);
    lv_label_set_text(ui->screen_navigation_bar_label_6, "\n" LV_SYMBOL_AUDIO " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_label_6, LV_LABEL_LONG_WRAP);

    //Write style for screen_navigation_bar_label_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_navigation_bar_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_label_6, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_label_6, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_label_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_navigation_bar_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_navigation_bar_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_label_6, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_navigation_bar_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_tileview_3
    ui->screen_navigation_bar_tileview_3 = lv_tileview_create(ui->screen_navigation_bar_cont_navigation_bar_return);
    lv_obj_set_pos(ui->screen_navigation_bar_tileview_3, 57, 270);
    lv_obj_set_size(ui->screen_navigation_bar_tileview_3, 300, 200);
    lv_obj_set_scrollbar_mode(ui->screen_navigation_bar_tileview_3, LV_SCROLLBAR_MODE_OFF);
    ui->screen_navigation_bar_tileview_3_tile = lv_tileview_add_tile(ui->screen_navigation_bar_tileview_3, 0, 0, LV_DIR_RIGHT);

    //Write style for screen_navigation_bar_tileview_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_navigation_bar_tileview_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_tileview_3, 77, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_tileview_3, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_tileview_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_tileview_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_navigation_bar_tileview_3, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_navigation_bar_tileview_3, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_tileview_3, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_tileview_3, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_tileview_3, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes screen_navigation_bar_btn_9
    ui->screen_navigation_bar_btn_9 = lv_button_create(ui->screen_navigation_bar_tileview_3_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_9, 30, 14);
    lv_obj_set_size(ui->screen_navigation_bar_btn_9, 80, 80);
    ui->screen_navigation_bar_btn_9_label = lv_label_create(ui->screen_navigation_bar_btn_9);
    lv_label_set_text(ui->screen_navigation_bar_btn_9_label, " " LV_SYMBOL_BATTERY_FULL " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_9_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_9_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_9, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_9_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_9, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_9, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_9, lv_color_hex(0xa9a9a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_9, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_9, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_9, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_9, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_9, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_9, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_btn_8
    ui->screen_navigation_bar_btn_8 = lv_button_create(ui->screen_navigation_bar_tileview_3_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_8, 155, 15);
    lv_obj_set_size(ui->screen_navigation_bar_btn_8, 80, 80);
    ui->screen_navigation_bar_btn_8_label = lv_label_create(ui->screen_navigation_bar_btn_8);
    lv_label_set_text(ui->screen_navigation_bar_btn_8_label, " " LV_SYMBOL_EYE_OPEN " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_8_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_8_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_8, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_8_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_8, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_8, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_8, lv_color_hex(0xa9a9a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_8, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_8, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_btn_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_btn_8, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_btn_8, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_8, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_8, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_8, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_8, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_8, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_btn_7
    ui->screen_navigation_bar_btn_7 = lv_button_create(ui->screen_navigation_bar_tileview_3_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_7, 30, 106);
    lv_obj_set_size(ui->screen_navigation_bar_btn_7, 80, 80);
    ui->screen_navigation_bar_btn_7_label = lv_label_create(ui->screen_navigation_bar_btn_7);
    lv_label_set_text(ui->screen_navigation_bar_btn_7_label, " " LV_SYMBOL_TINT " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_7_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_7_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_7, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_7_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_7, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_7, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_7, lv_color_hex(0xa9a9a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_7, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_7, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_btn_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_btn_7, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_btn_7, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_7, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_7, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_7, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_7, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_7, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_btn_6
    ui->screen_navigation_bar_btn_6 = lv_button_create(ui->screen_navigation_bar_tileview_3_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_6, 153, 109);
    lv_obj_set_size(ui->screen_navigation_bar_btn_6, 80, 80);
    ui->screen_navigation_bar_btn_6_label = lv_label_create(ui->screen_navigation_bar_btn_6);
    lv_label_set_text(ui->screen_navigation_bar_btn_6_label, " " LV_SYMBOL_SETTINGS " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_6_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_6_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_6, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_6_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_6, lv_color_hex(0xa9a9a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_6, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_6, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_btn_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_btn_6, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_btn_6, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_6, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_6, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_6, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_6, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_tileview_4
    ui->screen_navigation_bar_tileview_4 = lv_tileview_create(ui->screen_navigation_bar_cont_navigation_bar_return);
    lv_obj_set_pos(ui->screen_navigation_bar_tileview_4, 394, 272);
    lv_obj_set_size(ui->screen_navigation_bar_tileview_4, 300, 200);
    lv_obj_set_scrollbar_mode(ui->screen_navigation_bar_tileview_4, LV_SCROLLBAR_MODE_OFF);
    ui->screen_navigation_bar_tileview_4_tile = lv_tileview_add_tile(ui->screen_navigation_bar_tileview_4, 0, 0, LV_DIR_RIGHT);

    //Write style for screen_navigation_bar_tileview_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_navigation_bar_tileview_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_tileview_4, 77, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_tileview_4, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_tileview_4, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_tileview_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_navigation_bar_tileview_4, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_navigation_bar_tileview_4, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_tileview_4, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_tileview_4, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_tileview_4, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes screen_navigation_bar_btn_12
    ui->screen_navigation_bar_btn_12 = lv_button_create(ui->screen_navigation_bar_tileview_4_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_12, 214, 130);
    lv_obj_set_size(ui->screen_navigation_bar_btn_12, 60, 60);
    ui->screen_navigation_bar_btn_12_label = lv_label_create(ui->screen_navigation_bar_btn_12);
    lv_label_set_text(ui->screen_navigation_bar_btn_12_label, " " LV_SYMBOL_NEXT " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_12_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_12_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_12, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_12_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_12, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_12, lv_color_hex(0xa9a9a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_12, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_12, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_btn_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_btn_12, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_btn_12, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_12, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_12, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_12, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_12, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_btn_11
    ui->screen_navigation_bar_btn_11 = lv_button_create(ui->screen_navigation_bar_tileview_4_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_11, 19, 128);
    lv_obj_set_size(ui->screen_navigation_bar_btn_11, 60, 60);
    ui->screen_navigation_bar_btn_11_label = lv_label_create(ui->screen_navigation_bar_btn_11);
    lv_label_set_text(ui->screen_navigation_bar_btn_11_label, " " LV_SYMBOL_PREV " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_11_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_11_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_11, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_11_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_11, lv_color_hex(0xa9a9a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_11, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_11, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_btn_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_btn_11, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_btn_11, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_11, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_11, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_11, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_11, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_btn_10
    ui->screen_navigation_bar_btn_10 = lv_button_create(ui->screen_navigation_bar_tileview_4_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_10, 18, 11);
    lv_obj_set_size(ui->screen_navigation_bar_btn_10, 80, 80);
    ui->screen_navigation_bar_btn_10_label = lv_label_create(ui->screen_navigation_bar_btn_10);
    lv_label_set_text(ui->screen_navigation_bar_btn_10_label, "" LV_SYMBOL_AUDIO " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_10_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_10_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_10, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_10_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_10, lv_color_hex(0xa9a9a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_10, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_10, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_btn_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_btn_10, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_btn_10, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_10, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_10, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_10, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_10, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_btn_14
    ui->screen_navigation_bar_btn_14 = lv_button_create(ui->screen_navigation_bar_tileview_4_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_14, 118, 128);
    lv_obj_set_size(ui->screen_navigation_bar_btn_14, 60, 60);
    ui->screen_navigation_bar_btn_14_label = lv_label_create(ui->screen_navigation_bar_btn_14);
    lv_label_set_text(ui->screen_navigation_bar_btn_14_label, " " LV_SYMBOL_PLAY " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_14_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_14_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_14, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_14_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_14, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_14, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_14, lv_color_hex(0xa9a9a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_14, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_14, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_btn_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_btn_14, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_btn_14, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_14, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_14, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_14, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_14, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_14, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_label_9
    ui->screen_navigation_bar_label_9 = lv_label_create(ui->screen_navigation_bar_tileview_4_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_label_9, 116, 37);
    lv_obj_set_size(ui->screen_navigation_bar_label_9, 125, 40);
    lv_label_set_text(ui->screen_navigation_bar_label_9, "\xE6\x9C\xAA\xE5\x9C\xA8\xE6\x92\xAD\xE6\x94\xBE");
    lv_label_set_long_mode(ui->screen_navigation_bar_label_9, LV_LABEL_LONG_WRAP);

    //Write style for screen_navigation_bar_label_9, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_navigation_bar_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_label_9, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    /* MUSIC APP USER PATCH: SD track names require complete 20px CJK font. */
    lv_obj_set_style_text_font(ui->screen_navigation_bar_label_9, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_label_9, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_navigation_bar_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_navigation_bar_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_label_9, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_navigation_bar_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_tileview_WiFi
    ui->screen_navigation_bar_tileview_WiFi = lv_tileview_create(ui->screen_navigation_bar_cont_navigation_bar_return);
    lv_obj_set_pos(ui->screen_navigation_bar_tileview_WiFi, 236, 35);
    lv_obj_set_size(ui->screen_navigation_bar_tileview_WiFi, 500, 260);
    lv_obj_set_scrollbar_mode(ui->screen_navigation_bar_tileview_WiFi, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->screen_navigation_bar_tileview_WiFi, LV_OBJ_FLAG_HIDDEN);
    ui->screen_navigation_bar_tileview_WiFi_tile = lv_tileview_add_tile(ui->screen_navigation_bar_tileview_WiFi, 0, 0, LV_DIR_RIGHT);

    //Write style for screen_navigation_bar_tileview_WiFi, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_navigation_bar_tileview_WiFi, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_tileview_WiFi, 243, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_tileview_WiFi, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_tileview_WiFi, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_tileview_WiFi, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_navigation_bar_tileview_WiFi, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_navigation_bar_tileview_WiFi, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_tileview_WiFi, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_tileview_WiFi, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_tileview_WiFi, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes screen_navigation_bar_ta_2
    ui->screen_navigation_bar_ta_2 = lv_textarea_create(ui->screen_navigation_bar_tileview_WiFi_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_ta_2, 78, 141);
    lv_obj_set_size(ui->screen_navigation_bar_ta_2, 300, 40);
    lv_textarea_set_text(ui->screen_navigation_bar_ta_2, "cqh442666");
    lv_textarea_set_placeholder_text(ui->screen_navigation_bar_ta_2, "");
    lv_textarea_set_password_bullet(ui->screen_navigation_bar_ta_2, "*");
    lv_textarea_set_password_mode(ui->screen_navigation_bar_ta_2, true);
    lv_textarea_set_one_line(ui->screen_navigation_bar_ta_2, true);
    lv_textarea_set_accepted_chars(ui->screen_navigation_bar_ta_2, "");
    lv_textarea_set_max_length(ui->screen_navigation_bar_ta_2, 128);
#if LV_USE_KEYBOARD
    lv_obj_add_event_cb(ui->screen_navigation_bar_ta_2, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif

    //Write style for screen_navigation_bar_ta_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_navigation_bar_ta_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_ta_2, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_navigation_bar_ta_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_ta_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_ta_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_ta_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_ta_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_ta_2, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_ta_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_ta_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_navigation_bar_ta_2, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_ta_2, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_ta_2, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_ta_2, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_ta_2, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_ta_1
    ui->screen_navigation_bar_ta_1 = lv_textarea_create(ui->screen_navigation_bar_tileview_WiFi_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_ta_1, 81, 65);
    lv_obj_set_size(ui->screen_navigation_bar_ta_1, 300, 40);
    lv_textarea_set_text(ui->screen_navigation_bar_ta_1, "iQOO Neo9");
    lv_textarea_set_placeholder_text(ui->screen_navigation_bar_ta_1, "");
    lv_textarea_set_password_bullet(ui->screen_navigation_bar_ta_1, "*");
    lv_textarea_set_password_mode(ui->screen_navigation_bar_ta_1, false);
    lv_textarea_set_one_line(ui->screen_navigation_bar_ta_1, true);
    lv_textarea_set_accepted_chars(ui->screen_navigation_bar_ta_1, "");
    lv_textarea_set_max_length(ui->screen_navigation_bar_ta_1, 128);
#if LV_USE_KEYBOARD
    lv_obj_add_event_cb(ui->screen_navigation_bar_ta_1, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif

    //Write style for screen_navigation_bar_ta_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_navigation_bar_ta_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_ta_1, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_navigation_bar_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_ta_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_ta_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_ta_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_ta_1, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_ta_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_ta_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_navigation_bar_ta_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_ta_1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_ta_1, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_ta_1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_ta_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_label_11
    ui->screen_navigation_bar_label_11 = lv_label_create(ui->screen_navigation_bar_tileview_WiFi_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_label_11, 131, 40);
    lv_obj_set_size(ui->screen_navigation_bar_label_11, 200, 32);
    lv_label_set_text(ui->screen_navigation_bar_label_11, "\x57\x69\x46\x69\x20\xE5\x90\x8D\xE5\xAD\x97");
    lv_label_set_long_mode(ui->screen_navigation_bar_label_11, LV_LABEL_LONG_WRAP);

    //Write style for screen_navigation_bar_label_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_navigation_bar_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_label_11, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_label_11, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_label_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_navigation_bar_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_navigation_bar_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_label_11, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_navigation_bar_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_label_10
    ui->screen_navigation_bar_label_10 = lv_label_create(ui->screen_navigation_bar_tileview_WiFi_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_label_10, 128, 117);
    lv_obj_set_size(ui->screen_navigation_bar_label_10, 200, 32);
    lv_label_set_text(ui->screen_navigation_bar_label_10, "\x57\x69\x46\x69\x20\xE5\xAF\x86\xE7\xA0\x81");
    lv_label_set_long_mode(ui->screen_navigation_bar_label_10, LV_LABEL_LONG_WRAP);

    //Write style for screen_navigation_bar_label_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_navigation_bar_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_label_10, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_label_10, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_label_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_navigation_bar_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_navigation_bar_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_label_10, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_navigation_bar_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_btn_17
    ui->screen_navigation_bar_btn_17 = lv_button_create(ui->screen_navigation_bar_tileview_WiFi_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_17, 438, 9);
    lv_obj_set_size(ui->screen_navigation_bar_btn_17, 50, 40);
    ui->screen_navigation_bar_btn_17_label = lv_label_create(ui->screen_navigation_bar_btn_17);
    lv_label_set_text(ui->screen_navigation_bar_btn_17_label, " " LV_SYMBOL_CLOSE " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_17_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_17_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_17, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_17_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_17, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_17, 139, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_17, lv_color_hex(0xf44444), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_btn_17, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_17, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_17, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_17, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_17, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_17, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_spinner_1
    ui->screen_navigation_bar_spinner_1 = lv_spinner_create(ui->screen_navigation_bar_tileview_WiFi_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_spinner_1, 163, 63);
    lv_obj_set_size(ui->screen_navigation_bar_spinner_1, 100, 100);
    lv_obj_add_flag(ui->screen_navigation_bar_spinner_1, LV_OBJ_FLAG_HIDDEN);
    lv_spinner_set_anim_params(ui->screen_navigation_bar_spinner_1, 2000, 200);

    //Write style for screen_navigation_bar_spinner_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_pad_top(ui->screen_navigation_bar_spinner_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_navigation_bar_spinner_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_navigation_bar_spinner_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_navigation_bar_spinner_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_spinner_1, 169, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_navigation_bar_spinner_1, lv_color_hex(0xeeeef6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_navigation_bar_spinner_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui->screen_navigation_bar_spinner_1, 12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui->screen_navigation_bar_spinner_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui->screen_navigation_bar_spinner_1, lv_color_hex(0xd5d6de), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(ui->screen_navigation_bar_spinner_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_spinner_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_navigation_bar_spinner_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_arc_width(ui->screen_navigation_bar_spinner_1, 12, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui->screen_navigation_bar_spinner_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(ui->screen_navigation_bar_spinner_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(ui->screen_navigation_bar_spinner_1, true, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_btn_16
    ui->screen_navigation_bar_btn_16 = lv_button_create(ui->screen_navigation_bar_tileview_WiFi_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_16, 181, 193);
    lv_obj_set_size(ui->screen_navigation_bar_btn_16, 100, 50);
    ui->screen_navigation_bar_btn_16_label = lv_label_create(ui->screen_navigation_bar_btn_16);
    lv_label_set_text(ui->screen_navigation_bar_btn_16_label, "\xE8\xBF\x9E\xE6\x8E\xA5");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_16_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_16_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_16, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_16_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_16, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_16, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_navigation_bar_btn_16, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_navigation_bar_btn_16, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_navigation_bar_btn_16, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_16, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_16, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_16, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_16, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_16, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_navigation_bar_btn_15
    ui->screen_navigation_bar_btn_15 = lv_button_create(ui->screen_navigation_bar_tileview_WiFi_tile);
    lv_obj_set_pos(ui->screen_navigation_bar_btn_15, 324, 143);
    lv_obj_set_size(ui->screen_navigation_bar_btn_15, 50, 40);
    ui->screen_navigation_bar_btn_15_label = lv_label_create(ui->screen_navigation_bar_btn_15);
    lv_label_set_text(ui->screen_navigation_bar_btn_15_label, " " LV_SYMBOL_EYE_CLOSE " ");
    lv_label_set_long_mode(ui->screen_navigation_bar_btn_15_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_navigation_bar_btn_15_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_navigation_bar_btn_15, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_navigation_bar_btn_15_label, LV_PCT(100));

    //Write style for screen_navigation_bar_btn_15, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_navigation_bar_btn_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_navigation_bar_btn_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_navigation_bar_btn_15, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_navigation_bar_btn_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_navigation_bar_btn_15, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_navigation_bar_btn_15, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_navigation_bar_btn_15, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_navigation_bar_btn_15, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen_navigation_bar.


    //Update current screen layout.
    lv_obj_update_layout(ui->screen_navigation_bar);

    //Init events for screen.

    /* Restore toggle button states from g_nav_btn_state */
    if (g_nav_btn_state & NAV_BTN_3)
        lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_3, lv_color_hex(0x0098f2), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (!(g_nav_btn_state & NAV_BTN_5))
        lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_5, lv_color_hex(0xa9a9a9), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (g_nav_btn_state & NAV_BTN_7)
        lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_7, lv_color_hex(0x0098f2), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (g_nav_btn_state & NAV_BTN_8)
        lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_8, lv_color_hex(0x0098f2), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (g_nav_btn_state & NAV_BTN_9)
        lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_9, lv_color_hex(0x0098f2), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (g_nav_btn_state & NAV_BTN_10)
        lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_10, lv_color_hex(0x0098f2), LV_PART_MAIN | LV_STATE_DEFAULT);
    if (g_nav_btn_state & NAV_BTN_14) {
        lv_obj_set_style_bg_color(ui->screen_navigation_bar_btn_14, lv_color_hex(0x0098f2), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui->screen_navigation_bar_btn_14_label, " " LV_SYMBOL_PAUSE " ");
    }
    /* STAGE6F SETTINGS USER PATCH: controls are created after boot restore. */
    system_settings_sync_navigation_controls(ui);
    events_init_screen_navigation_bar(ui);
}
