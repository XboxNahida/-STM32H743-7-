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



int screen_yuanshen_digital_clock_1_min_value = 8;
int screen_yuanshen_digital_clock_1_hour_value = 15;
int screen_yuanshen_digital_clock_1_sec_value = 6;
void setup_scr_screen_yuanshen(lv_ui *ui)
{
    //Write codes screen_yuanshen
    ui->screen_yuanshen = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_yuanshen, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->screen_yuanshen, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_yuanshen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_yuanshen, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_yuanshen, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_yuanshen, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_yuanshen_cont_1
    ui->screen_yuanshen_cont_1 = lv_obj_create(ui->screen_yuanshen);
    lv_obj_set_pos(ui->screen_yuanshen_cont_1, 0, 0);
    lv_obj_set_size(ui->screen_yuanshen_cont_1, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->screen_yuanshen_cont_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_yuanshen_cont_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_yuanshen_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_yuanshen_cont_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_yuanshen_cont_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_yuanshen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_yuanshen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_yuanshen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_yuanshen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_yuanshen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_yuanshen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_yuanshen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_yuanshen_img_1
    ui->screen_yuanshen_img_1 = lv_image_create(ui->screen_yuanshen_cont_1);
    lv_obj_set_pos(ui->screen_yuanshen_img_1, 369, 257);
    lv_obj_set_size(ui->screen_yuanshen_img_1, 285, 107);
    lv_obj_add_flag(ui->screen_yuanshen_img_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->screen_yuanshen_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->screen_yuanshen_img_1, norflash_get_img1_dsc());
    lv_image_set_pivot(ui->screen_yuanshen_img_1, 50,50);
    lv_image_set_rotation(ui->screen_yuanshen_img_1, 0);

    //Write style for screen_yuanshen_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_yuanshen_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_yuanshen_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_yuanshen_img_2
    ui->screen_yuanshen_img_2 = lv_image_create(ui->screen_yuanshen_cont_1);
    lv_obj_set_pos(ui->screen_yuanshen_img_2, 162, 216);
    lv_obj_set_size(ui->screen_yuanshen_img_2, 700, 370);
    lv_obj_add_flag(ui->screen_yuanshen_img_2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->screen_yuanshen_img_2, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->screen_yuanshen_img_2, norflash_get_img2_dsc());
    lv_image_set_pivot(ui->screen_yuanshen_img_2, 50,50);
    lv_image_set_rotation(ui->screen_yuanshen_img_2, 0);

    //Write style for screen_yuanshen_img_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_yuanshen_img_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_yuanshen_img_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_yuanshen_img_3
    ui->screen_yuanshen_img_3 = lv_image_create(ui->screen_yuanshen_cont_1);
    lv_obj_set_pos(ui->screen_yuanshen_img_3, 212, 166);
    lv_obj_set_size(ui->screen_yuanshen_img_3, 600, 220);
    lv_obj_add_flag(ui->screen_yuanshen_img_3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->screen_yuanshen_img_3, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->screen_yuanshen_img_3, norflash_get_img3_dsc());
    lv_image_set_pivot(ui->screen_yuanshen_img_3, 50,50);
    lv_image_set_rotation(ui->screen_yuanshen_img_3, 0);

    //Write style for screen_yuanshen_img_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_yuanshen_img_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_yuanshen_img_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_yuanshen_cont_2
    ui->screen_yuanshen_cont_2 = lv_obj_create(ui->screen_yuanshen);
    lv_obj_set_pos(ui->screen_yuanshen_cont_2, 0, 0);
    lv_obj_set_size(ui->screen_yuanshen_cont_2, 1024, 35);
    lv_obj_set_scrollbar_mode(ui->screen_yuanshen_cont_2, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_yuanshen_cont_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_yuanshen_cont_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_yuanshen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_yuanshen_cont_2, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_yuanshen_cont_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_yuanshen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_yuanshen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_yuanshen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_yuanshen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_yuanshen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_yuanshen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_yuanshen_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_yuanshen_label_2
    ui->screen_yuanshen_label_2 = lv_label_create(ui->screen_yuanshen_cont_2);
    lv_obj_set_pos(ui->screen_yuanshen_label_2, 810, 5);
    lv_obj_set_size(ui->screen_yuanshen_label_2, 210, 30);
    lv_label_set_text(ui->screen_yuanshen_label_2, "   " LV_SYMBOL_WIFI "  " LV_SYMBOL_VOLUME_MAX " " LV_SYMBOL_USB " " LV_SYMBOL_BATTERY_FULL "  100%");
    lv_label_set_long_mode(ui->screen_yuanshen_label_2, LV_LABEL_LONG_WRAP);

    //Write style for screen_yuanshen_label_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_yuanshen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_yuanshen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_yuanshen_label_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_yuanshen_label_2, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_yuanshen_label_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_yuanshen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_yuanshen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_yuanshen_label_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_yuanshen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_yuanshen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_yuanshen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_yuanshen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_yuanshen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_yuanshen_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_yuanshen_label_1
    ui->screen_yuanshen_label_1 = lv_label_create(ui->screen_yuanshen_cont_2);
    lv_obj_set_pos(ui->screen_yuanshen_label_1, 462, -141);
    lv_obj_set_size(ui->screen_yuanshen_label_1, 100, 32);
    lv_label_set_text(ui->screen_yuanshen_label_1, "");
    lv_label_set_long_mode(ui->screen_yuanshen_label_1, LV_LABEL_LONG_WRAP);

    //Write style for screen_yuanshen_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_yuanshen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_yuanshen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_yuanshen_label_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_yuanshen_label_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_yuanshen_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_yuanshen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_yuanshen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_yuanshen_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_yuanshen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_yuanshen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_yuanshen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_yuanshen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_yuanshen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_yuanshen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_yuanshen_digital_clock_1
    static bool screen_yuanshen_digital_clock_1_timer_enabled = false;
    ui->screen_yuanshen_digital_clock_1 = lv_label_create(ui->screen_yuanshen_cont_2);
    lv_obj_set_pos(ui->screen_yuanshen_digital_clock_1, 0, 0);
    lv_obj_set_size(ui->screen_yuanshen_digital_clock_1, 100, 35);
    lv_label_set_text(ui->screen_yuanshen_digital_clock_1, "15:08");
    if (!screen_yuanshen_digital_clock_1_timer_enabled) {
        lv_timer_create(screen_yuanshen_digital_clock_1_timer, 1000, NULL);
        screen_yuanshen_digital_clock_1_timer_enabled = true;
    }

    //Write style for screen_yuanshen_digital_clock_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_yuanshen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_yuanshen_digital_clock_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_yuanshen_digital_clock_1, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_yuanshen_digital_clock_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_yuanshen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_yuanshen_digital_clock_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_yuanshen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_yuanshen_digital_clock_1, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_yuanshen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_yuanshen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_yuanshen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_yuanshen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen_yuanshen.


    //Update current screen layout.
    lv_obj_update_layout(ui->screen_yuanshen);

    //Init events for screen.
    events_init_screen_yuanshen(ui);
}
