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



int lock_screen_digital_clock_1_min_value = 8;
int lock_screen_digital_clock_1_hour_value = 15;
int lock_screen_digital_clock_1_sec_value = 6;
void setup_scr_lock_screen(lv_ui *ui)
{
    //Write codes lock_screen
    ui->lock_screen = lv_obj_create(NULL);
    lv_obj_set_size(ui->lock_screen, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->lock_screen, LV_SCROLLBAR_MODE_OFF);

    //Write style for lock_screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->lock_screen, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(ui->lock_screen, norflash_get_wallpaper_dsc(), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_opa(ui->lock_screen, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_recolor_opa(ui->lock_screen, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes lock_screen_cont_lock
    ui->lock_screen_cont_lock = lv_obj_create(ui->lock_screen);
    lv_obj_set_pos(ui->lock_screen_cont_lock, 9, 35);
    lv_obj_set_size(ui->lock_screen_cont_lock, 1000, 550);
    lv_obj_set_scrollbar_mode(ui->lock_screen_cont_lock, LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_scroll_dir(ui->lock_screen_cont_lock, LV_DIR_BOTTOM | LV_DIR_HOR);
    //Write style for lock_screen_cont_lock, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->lock_screen_cont_lock, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->lock_screen_cont_lock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->lock_screen_cont_lock, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->lock_screen_cont_lock, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->lock_screen_cont_lock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->lock_screen_cont_lock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->lock_screen_cont_lock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->lock_screen_cont_lock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->lock_screen_cont_lock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->lock_screen_cont_lock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->lock_screen_cont_lock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes lock_screen_digital_clock_1
    static bool lock_screen_digital_clock_1_timer_enabled = false;
    ui->lock_screen_digital_clock_1 = lv_label_create(ui->lock_screen_cont_lock);
    lv_obj_set_pos(ui->lock_screen_digital_clock_1, 310, 110);
    lv_obj_set_size(ui->lock_screen_digital_clock_1, 400, 50);
    lv_label_set_text(ui->lock_screen_digital_clock_1, "15:08:06");
    if (!lock_screen_digital_clock_1_timer_enabled) {
        lv_timer_create(lock_screen_digital_clock_1_timer, 1000, NULL);
        lock_screen_digital_clock_1_timer_enabled = true;
    }

    //Write style for lock_screen_digital_clock_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->lock_screen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->lock_screen_digital_clock_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->lock_screen_digital_clock_1, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->lock_screen_digital_clock_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->lock_screen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->lock_screen_digital_clock_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->lock_screen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->lock_screen_digital_clock_1, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->lock_screen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->lock_screen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->lock_screen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->lock_screen_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes lock_screen_datetext_1
    ui->lock_screen_datetext_1 = lv_label_create(ui->lock_screen_cont_lock);
    lv_obj_set_pos(ui->lock_screen_datetext_1, 312, 159);
    lv_obj_set_size(ui->lock_screen_datetext_1, 400, 50);
    lv_label_set_text(ui->lock_screen_datetext_1, "2026/05/09");
    lv_obj_set_style_text_align(ui->lock_screen_datetext_1, LV_TEXT_ALIGN_CENTER, 0);    /* CLICKABLE removed - date synced from RTC */
    lv_obj_add_event_cb(ui->lock_screen_datetext_1, lock_screen_datetext_1_event_handler, LV_EVENT_ALL, NULL);

    //Write style for lock_screen_datetext_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->lock_screen_datetext_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->lock_screen_datetext_1, &lv_font_montserratMedium_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->lock_screen_datetext_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->lock_screen_datetext_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->lock_screen_datetext_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->lock_screen_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->lock_screen_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->lock_screen_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->lock_screen_datetext_1, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->lock_screen_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->lock_screen_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->lock_screen_datetext_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes lock_screen_label_lock_flag
    ui->lock_screen_label_lock_flag = lv_label_create(ui->lock_screen_cont_lock);
    lv_obj_set_pos(ui->lock_screen_label_lock_flag, 448, 900);
    lv_obj_set_size(ui->lock_screen_label_lock_flag, 100, 32);
    lv_label_set_text(ui->lock_screen_label_lock_flag, "");
    lv_label_set_long_mode(ui->lock_screen_label_lock_flag, LV_LABEL_LONG_WRAP);

    //Write style for lock_screen_label_lock_flag, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->lock_screen_label_lock_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->lock_screen_label_lock_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->lock_screen_label_lock_flag, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->lock_screen_label_lock_flag, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->lock_screen_label_lock_flag, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->lock_screen_label_lock_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->lock_screen_label_lock_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->lock_screen_label_lock_flag, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->lock_screen_label_lock_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->lock_screen_label_lock_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->lock_screen_label_lock_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->lock_screen_label_lock_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->lock_screen_label_lock_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->lock_screen_label_lock_flag, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes lock_screen_img_1
    ui->lock_screen_img_1 = lv_image_create(ui->lock_screen_cont_lock);
    lv_obj_set_pos(ui->lock_screen_img_1, 463, 424);
    lv_obj_set_size(ui->lock_screen_img_1, 70, 70);
    lv_obj_add_flag(ui->lock_screen_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ui->lock_screen_img_1, &_fingerprint_RGB565A8_70x70);
    lv_image_set_pivot(ui->lock_screen_img_1, 50,50);
    lv_image_set_rotation(ui->lock_screen_img_1, 0);

    //Write style for lock_screen_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->lock_screen_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor(ui->lock_screen_img_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->lock_screen_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes lock_screen_label_navigation_bar1
    ui->lock_screen_label_navigation_bar1 = lv_label_create(ui->lock_screen);
    lv_obj_set_pos(ui->lock_screen_label_navigation_bar1, 809, 5);
    lv_obj_set_size(ui->lock_screen_label_navigation_bar1, 210, 30);
    lv_label_set_text(ui->lock_screen_label_navigation_bar1, "   " LV_SYMBOL_WIFI "  " LV_SYMBOL_VOLUME_MAX " " LV_SYMBOL_USB " " LV_SYMBOL_BATTERY_FULL "  100%");
    lv_label_set_long_mode(ui->lock_screen_label_navigation_bar1, LV_LABEL_LONG_WRAP);

    //Write style for lock_screen_label_navigation_bar1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->lock_screen_label_navigation_bar1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->lock_screen_label_navigation_bar1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->lock_screen_label_navigation_bar1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->lock_screen_label_navigation_bar1, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->lock_screen_label_navigation_bar1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->lock_screen_label_navigation_bar1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->lock_screen_label_navigation_bar1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->lock_screen_label_navigation_bar1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->lock_screen_label_navigation_bar1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->lock_screen_label_navigation_bar1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->lock_screen_label_navigation_bar1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->lock_screen_label_navigation_bar1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->lock_screen_label_navigation_bar1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->lock_screen_label_navigation_bar1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of lock_screen.


    //Update current screen layout.
    lv_obj_update_layout(ui->lock_screen);

    //Init events for screen.
    events_init_lock_screen(ui);
}
