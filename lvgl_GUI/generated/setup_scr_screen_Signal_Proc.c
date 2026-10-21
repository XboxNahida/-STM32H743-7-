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



int screen_Signal_Proc_digital_clock_1_min_value = 8;
int screen_Signal_Proc_digital_clock_1_hour_value = 15;
int screen_Signal_Proc_digital_clock_1_sec_value = 6;
void setup_scr_screen_Signal_Proc(lv_ui *ui)
{
    //Write codes screen_Signal_Proc
    ui->screen_Signal_Proc = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_Signal_Proc, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->screen_Signal_Proc, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_Signal_Proc, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_Signal_Proc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(ui->screen_Signal_Proc, norflash_get_wallpaper_dsc(), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_opa(ui->screen_Signal_Proc, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_recolor_opa(ui->screen_Signal_Proc, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_Signal_Proc_btn_return
    ui->screen_Signal_Proc_btn_return = lv_button_create(ui->screen_Signal_Proc);
    lv_obj_set_pos(ui->screen_Signal_Proc_btn_return, 17, 466);
    lv_obj_set_size(ui->screen_Signal_Proc_btn_return, 250, 50);
    ui->screen_Signal_Proc_btn_return_label = lv_label_create(ui->screen_Signal_Proc_btn_return);
    lv_label_set_text(ui->screen_Signal_Proc_btn_return_label, "\xE8\xBF\x94\xE5\x9B\x9E\x20" LV_SYMBOL_HOME " ");
    lv_label_set_long_mode(ui->screen_Signal_Proc_btn_return_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_Signal_Proc_btn_return_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_Signal_Proc_btn_return, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_Signal_Proc_btn_return_label, LV_PCT(100));

    //Write style for screen_Signal_Proc_btn_return, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_Signal_Proc_btn_return, 90, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_Signal_Proc_btn_return, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_Signal_Proc_btn_return, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_Signal_Proc_btn_return, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_Signal_Proc_btn_return, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_Signal_Proc_btn_return, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_Signal_Proc_btn_return, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_Signal_Proc_btn_return, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_Signal_Proc_btn_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_Signal_Proc_btn_return, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_Signal_Proc_btn_return, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_Signal_Proc_btn_return, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_Signal_Proc_btn_return, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_Signal_Proc_btn_printf
    ui->screen_Signal_Proc_btn_printf = lv_button_create(ui->screen_Signal_Proc);
    lv_obj_set_pos(ui->screen_Signal_Proc_btn_printf, 17, 411);
    lv_obj_set_size(ui->screen_Signal_Proc_btn_printf, 250, 50);
    ui->screen_Signal_Proc_btn_printf_label = lv_label_create(ui->screen_Signal_Proc_btn_printf);
    lv_label_set_text(ui->screen_Signal_Proc_btn_printf_label, "\xE6\x89\x93\xE5\x8D\xB0");
    lv_label_set_long_mode(ui->screen_Signal_Proc_btn_printf_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_Signal_Proc_btn_printf_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_Signal_Proc_btn_printf, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_Signal_Proc_btn_printf_label, LV_PCT(100));

    //Write style for screen_Signal_Proc_btn_printf, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_Signal_Proc_btn_printf, 90, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_Signal_Proc_btn_printf, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_Signal_Proc_btn_printf, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_Signal_Proc_btn_printf, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_Signal_Proc_btn_printf, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_Signal_Proc_btn_printf, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_Signal_Proc_btn_printf, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_Signal_Proc_btn_printf, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_Signal_Proc_btn_printf, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_Signal_Proc_btn_printf, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_Signal_Proc_btn_printf, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_Signal_Proc_btn_printf, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_Signal_Proc_btn_printf, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_Signal_Proc_btn_clear
    ui->screen_Signal_Proc_btn_clear = lv_button_create(ui->screen_Signal_Proc);
    lv_obj_set_pos(ui->screen_Signal_Proc_btn_clear, 17, 354);
    lv_obj_set_size(ui->screen_Signal_Proc_btn_clear, 250, 50);
    ui->screen_Signal_Proc_btn_clear_label = lv_label_create(ui->screen_Signal_Proc_btn_clear);
    lv_label_set_text(ui->screen_Signal_Proc_btn_clear_label, "\xE6\xB8\x85\xE7\xA9\xBA");
    lv_label_set_long_mode(ui->screen_Signal_Proc_btn_clear_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_Signal_Proc_btn_clear_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_Signal_Proc_btn_clear, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_Signal_Proc_btn_clear_label, LV_PCT(100));

    //Write style for screen_Signal_Proc_btn_clear, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_Signal_Proc_btn_clear, 90, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_Signal_Proc_btn_clear, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_Signal_Proc_btn_clear, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_Signal_Proc_btn_clear, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_Signal_Proc_btn_clear, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_Signal_Proc_btn_clear, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_Signal_Proc_btn_clear, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_Signal_Proc_btn_clear, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_Signal_Proc_btn_clear, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_Signal_Proc_btn_clear, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_Signal_Proc_btn_clear, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_Signal_Proc_btn_clear, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_Signal_Proc_btn_clear, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_Signal_Proc_ta_1
    ui->screen_Signal_Proc_ta_1 = lv_textarea_create(ui->screen_Signal_Proc);
    lv_obj_set_pos(ui->screen_Signal_Proc_ta_1, 390, 47);
    lv_obj_set_size(ui->screen_Signal_Proc_ta_1, 600, 60);
    lv_textarea_set_text(ui->screen_Signal_Proc_ta_1, "help");
    lv_textarea_set_placeholder_text(ui->screen_Signal_Proc_ta_1, "");
    lv_textarea_set_password_bullet(ui->screen_Signal_Proc_ta_1, "*");
    lv_textarea_set_password_mode(ui->screen_Signal_Proc_ta_1, false);
    lv_textarea_set_one_line(ui->screen_Signal_Proc_ta_1, false);
    lv_textarea_set_accepted_chars(ui->screen_Signal_Proc_ta_1, "");
    lv_textarea_set_max_length(ui->screen_Signal_Proc_ta_1, 512);
#if LV_USE_KEYBOARD
    lv_obj_add_event_cb(ui->screen_Signal_Proc_ta_1, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif

    //Write style for screen_Signal_Proc_ta_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_Signal_Proc_ta_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_Signal_Proc_ta_1, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_Signal_Proc_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_Signal_Proc_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_Signal_Proc_ta_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_Signal_Proc_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_Signal_Proc_ta_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_Signal_Proc_ta_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_Signal_Proc_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_Signal_Proc_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_Signal_Proc_ta_1, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_Signal_Proc_ta_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_Signal_Proc_ta_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_Signal_Proc_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_Signal_Proc_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_Signal_Proc_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_Signal_Proc_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_Signal_Proc_ta_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_Signal_Proc_ta_1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_Signal_Proc_ta_1, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_Signal_Proc_ta_1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_Signal_Proc_ta_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes screen_Signal_Proc_btn_1
    ui->screen_Signal_Proc_btn_1 = lv_button_create(ui->screen_Signal_Proc);
    lv_obj_set_pos(ui->screen_Signal_Proc_btn_1, 275, 51);
    lv_obj_set_size(ui->screen_Signal_Proc_btn_1, 100, 50);
    ui->screen_Signal_Proc_btn_1_label = lv_label_create(ui->screen_Signal_Proc_btn_1);
    lv_label_set_text(ui->screen_Signal_Proc_btn_1_label, "\xE5\x8F\x91\xE9\x80\x81");
    lv_label_set_long_mode(ui->screen_Signal_Proc_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_Signal_Proc_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_Signal_Proc_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_Signal_Proc_btn_1_label, LV_PCT(100));

    //Write style for screen_Signal_Proc_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_Signal_Proc_btn_1, 90, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_Signal_Proc_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_Signal_Proc_btn_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_Signal_Proc_btn_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_Signal_Proc_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_Signal_Proc_btn_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_Signal_Proc_btn_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_Signal_Proc_btn_1, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_Signal_Proc_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_Signal_Proc_btn_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_Signal_Proc_btn_1, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_Signal_Proc_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_Signal_Proc_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_Signal_Proc_label_3
    ui->screen_Signal_Proc_label_3 = lv_label_create(ui->screen_Signal_Proc);
    lv_obj_set_pos(ui->screen_Signal_Proc_label_3, 390, 137);
    lv_obj_set_size(ui->screen_Signal_Proc_label_3, 600, 60);
    lv_label_set_text(ui->screen_Signal_Proc_label_3, "Signal_Proc");
    lv_label_set_long_mode(ui->screen_Signal_Proc_label_3, LV_LABEL_LONG_WRAP);

    //Write style for screen_Signal_Proc_label_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_Signal_Proc_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_Signal_Proc_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_Signal_Proc_label_3, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_Signal_Proc_label_3, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_Signal_Proc_label_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_Signal_Proc_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_Signal_Proc_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_Signal_Proc_label_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_Signal_Proc_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_Signal_Proc_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_Signal_Proc_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_Signal_Proc_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_Signal_Proc_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_Signal_Proc_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_Signal_Proc_cont_2
    ui->screen_Signal_Proc_cont_2 = lv_obj_create(ui->screen_Signal_Proc);
    lv_obj_set_pos(ui->screen_Signal_Proc_cont_2, 0, 0);
    lv_obj_set_size(ui->screen_Signal_Proc_cont_2, 1024, 35);
    lv_obj_set_scrollbar_mode(ui->screen_Signal_Proc_cont_2, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_Signal_Proc_cont_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_Signal_Proc_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_Signal_Proc_cont_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_Signal_Proc_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_Signal_Proc_cont_2, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_Signal_Proc_cont_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_Signal_Proc_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_Signal_Proc_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_Signal_Proc_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_Signal_Proc_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_Signal_Proc_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_Signal_Proc_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_Signal_Proc_label_2
    ui->screen_Signal_Proc_label_2 = lv_label_create(ui->screen_Signal_Proc_cont_2);
    lv_obj_set_pos(ui->screen_Signal_Proc_label_2, 810, 5);
    lv_obj_set_size(ui->screen_Signal_Proc_label_2, 210, 30);
    lv_label_set_text(ui->screen_Signal_Proc_label_2, "   " LV_SYMBOL_WIFI "  " LV_SYMBOL_VOLUME_MAX " " LV_SYMBOL_USB " " LV_SYMBOL_BATTERY_FULL "  100%");
    lv_label_set_long_mode(ui->screen_Signal_Proc_label_2, LV_LABEL_LONG_WRAP);

    //Write style for screen_Signal_Proc_label_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_Signal_Proc_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_Signal_Proc_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_Signal_Proc_label_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_Signal_Proc_label_2, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_Signal_Proc_label_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_Signal_Proc_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_Signal_Proc_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_Signal_Proc_label_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_Signal_Proc_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_Signal_Proc_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_Signal_Proc_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_Signal_Proc_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_Signal_Proc_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_Signal_Proc_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_Signal_Proc_label_1
    ui->screen_Signal_Proc_label_1 = lv_label_create(ui->screen_Signal_Proc_cont_2);
    lv_obj_set_pos(ui->screen_Signal_Proc_label_1, 462, -141);
    lv_obj_set_size(ui->screen_Signal_Proc_label_1, 100, 32);
    lv_label_set_text(ui->screen_Signal_Proc_label_1, "");
    lv_label_set_long_mode(ui->screen_Signal_Proc_label_1, LV_LABEL_LONG_WRAP);

    //Write style for screen_Signal_Proc_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_Signal_Proc_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_Signal_Proc_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_Signal_Proc_label_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_Signal_Proc_label_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_Signal_Proc_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_Signal_Proc_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_Signal_Proc_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_Signal_Proc_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_Signal_Proc_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_Signal_Proc_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_Signal_Proc_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_Signal_Proc_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_Signal_Proc_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_Signal_Proc_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_Signal_Proc_digital_clock_1
    static bool screen_Signal_Proc_digital_clock_1_timer_enabled = false;
    ui->screen_Signal_Proc_digital_clock_1 = lv_label_create(ui->screen_Signal_Proc_cont_2);
    lv_obj_set_pos(ui->screen_Signal_Proc_digital_clock_1, 0, 0);
    lv_obj_set_size(ui->screen_Signal_Proc_digital_clock_1, 100, 35);
    lv_label_set_text(ui->screen_Signal_Proc_digital_clock_1, "15:08");
    if (!screen_Signal_Proc_digital_clock_1_timer_enabled) {
        lv_timer_create(screen_Signal_Proc_digital_clock_1_timer, 1000, NULL);
        screen_Signal_Proc_digital_clock_1_timer_enabled = true;
    }

    //Write style for screen_Signal_Proc_digital_clock_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_Signal_Proc_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_Signal_Proc_digital_clock_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_Signal_Proc_digital_clock_1, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_Signal_Proc_digital_clock_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_Signal_Proc_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_Signal_Proc_digital_clock_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_Signal_Proc_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_Signal_Proc_digital_clock_1, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_Signal_Proc_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_Signal_Proc_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_Signal_Proc_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_Signal_Proc_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    // ====== 优化版 textarea 配置 ======
    ui->Serial_Port_ta_display = lv_textarea_create(ui->screen_Signal_Proc);
    lv_obj_set_pos(ui->Serial_Port_ta_display, 362, 199);
    lv_obj_set_size(ui->Serial_Port_ta_display, 625, 369);
    // 基础配置
    lv_textarea_set_text(ui->Serial_Port_ta_display, "");
    lv_textarea_set_placeholder_text(ui->Serial_Port_ta_display, "No data...");
    lv_textarea_set_password_mode(ui->Serial_Port_ta_display, false);
    lv_textarea_set_one_line(ui->Serial_Port_ta_display, false);
    lv_textarea_set_max_length(ui->Serial_Port_ta_display, 65535);
    // === 核心优化：提取内部 label 并设置长文本模式 ===
    lv_obj_t * inner_label = lv_textarea_get_label(ui->Serial_Port_ta_display);
    if(inner_label) {
        lv_label_set_long_mode(inner_label, LV_LABEL_LONG_MODE_WRAP);  // SCROLLABLE 模式
        lv_obj_set_size(inner_label, 625 - 16, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(inner_label, 0, LV_PART_MAIN);
    }
    // 滚动配置
    lv_obj_set_scrollbar_mode(ui->Serial_Port_ta_display, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(ui->Serial_Port_ta_display, LV_DIR_ALL);
    lv_obj_set_scroll_snap_y(ui->Serial_Port_ta_display, LV_SCROLL_SNAP_NONE);
    lv_obj_set_style_anim_time(ui->Serial_Port_ta_display, 0, LV_PART_SCROLLBAR);
    // === 恢复滚动手势 ===
    lv_obj_add_flag(ui->Serial_Port_ta_display, LV_OBJ_FLAG_SCROLLABLE);
    // 外观样式
    lv_obj_set_style_text_color(ui->Serial_Port_ta_display, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->Serial_Port_ta_display, &lv_font_myyh_14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->Serial_Port_ta_display, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->Serial_Port_ta_display, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->Serial_Port_ta_display, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->Serial_Port_ta_display, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->Serial_Port_ta_display, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->Serial_Port_ta_display, 8, LV_PART_MAIN|LV_STATE_DEFAULT);



    //The custom code of screen_Signal_Proc.


    //Update current screen layout.
    lv_obj_update_layout(ui->screen_Signal_Proc);

    //Init events for screen.
    events_init_screen_Signal_Proc(ui);
}
