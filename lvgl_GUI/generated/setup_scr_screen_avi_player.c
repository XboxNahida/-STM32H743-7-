/*
* Copyright 2026 NXP
* NXP Proprietary.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_screen_avi_player(lv_ui *ui)
{
    lv_obj_t *screen_avi_player_status_label;

    /* ---- Main screen (black background) ---- */
    ui->screen_avi_player = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_avi_player, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->screen_avi_player, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(ui->screen_avi_player, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_avi_player, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->screen_avi_player, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* ---- Video display image ---- */
    ui->screen_avi_player_img = lv_image_create(ui->screen_avi_player);
    lv_obj_align(ui->screen_avi_player_img, LV_ALIGN_CENTER, 0, -15);
    lv_obj_set_style_bg_opa(ui->screen_avi_player_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* ---- Top status bar ---- */
    ui->screen_avi_player_cont_2 = lv_obj_create(ui->screen_avi_player);
    lv_obj_set_pos(ui->screen_avi_player_cont_2, 0, 0);
    lv_obj_set_size(ui->screen_avi_player_cont_2, 1024, 35);
    lv_obj_set_scrollbar_mode(ui->screen_avi_player_cont_2, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(ui->screen_avi_player_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_avi_player_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_avi_player_cont_2, 160, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_avi_player_cont_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_avi_player_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->screen_avi_player_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* ---- Back button ---- */
    ui->screen_avi_player_btn_1 = lv_button_create(ui->screen_avi_player);
    lv_obj_set_pos(ui->screen_avi_player_btn_1, 0, 0);
    lv_obj_set_size(ui->screen_avi_player_btn_1, 50, 35);
    lv_obj_set_style_bg_opa(ui->screen_avi_player_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_avi_player_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_avi_player_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_avi_player_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    ui->screen_avi_player_btn_1_label = lv_label_create(ui->screen_avi_player_btn_1);
    lv_label_set_text(ui->screen_avi_player_btn_1_label, LV_SYMBOL_CLOSE);
    lv_obj_center(ui->screen_avi_player_btn_1_label);
    lv_obj_set_style_text_color(ui->screen_avi_player_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_avi_player_btn_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* ---- Filename label ---- */
    ui->screen_avi_player_label_2 = lv_label_create(ui->screen_avi_player_cont_2);
    /* MEDIA NAVIGATION USER PATCH: fixed non-overlapping media zones. */
    lv_obj_set_pos(ui->screen_avi_player_label_2, 55, 5);
    lv_obj_set_size(ui->screen_avi_player_label_2, 385, 30);
    lv_label_set_long_mode(ui->screen_avi_player_label_2, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(ui->screen_avi_player_label_2, "AVI \u64ad\u653e\u5668");
    lv_obj_set_style_text_font(ui->screen_avi_player_label_2, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_avi_player_label_2, lv_color_hex(0xcccccc), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_avi_player_label_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* ---- Frame counter / status label ---- */
    ui->screen_avi_player_label_3 = lv_label_create(ui->screen_avi_player_cont_2);
    lv_obj_set_pos(ui->screen_avi_player_label_3, 450, 5);
    lv_obj_set_size(ui->screen_avi_player_label_3, 240, 30);
    lv_label_set_text(ui->screen_avi_player_label_3, "");
    lv_obj_set_style_text_font(ui->screen_avi_player_label_3, &lv_font_myyh_14, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_avi_player_label_3, lv_color_hex(0xcccccc), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_avi_player_label_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_avi_player_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* ---- Time display ---- */
    ui->screen_avi_player_digital_clock_2 = lv_label_create(ui->screen_avi_player_cont_2);
    lv_obj_set_pos(ui->screen_avi_player_digital_clock_2, 700, 5);
    lv_obj_set_size(ui->screen_avi_player_digital_clock_2, 100, 30);
    lv_label_set_text(ui->screen_avi_player_digital_clock_2, "15:08");
    lv_obj_set_style_text_font(ui->screen_avi_player_digital_clock_2, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_avi_player_digital_clock_2, lv_color_hex(0xcccccc), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_avi_player_digital_clock_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_avi_player_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* ---- Status icons (right) ---- */
    screen_avi_player_status_label = lv_label_create(ui->screen_avi_player_cont_2);
    lv_obj_set_pos(screen_avi_player_status_label, 810, 5);
    lv_obj_set_size(screen_avi_player_status_label, 210, 30);
    lv_label_set_text(screen_avi_player_status_label,
        "   " LV_SYMBOL_WIFI "  " LV_SYMBOL_VOLUME_MAX " "
        LV_SYMBOL_USB " " LV_SYMBOL_BATTERY_FULL "  100%");
    lv_obj_set_style_text_color(screen_avi_player_status_label,
        lv_color_hex(0xcccccc), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(screen_avi_player_status_label,
        &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(screen_avi_player_status_label, 0,
        LV_PART_MAIN|LV_STATE_DEFAULT);

    lv_obj_update_layout(ui->screen_avi_player);
    events_init_screen_avi_player(ui);
}
