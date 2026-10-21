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
#include "camera_ui_bridge.h"



void setup_scr_screen_camera(lv_ui *ui)
{
    //Write codes screen_camera
    ui->screen_camera = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_camera, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->screen_camera, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_camera, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_camera, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_camera, lv_color_hex(0x2d2d2d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_camera, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_camera_btn_1
    ui->screen_camera_btn_1 = lv_button_create(ui->screen_camera);
    lv_obj_set_pos(ui->screen_camera_btn_1, 26, 516);
    lv_obj_set_size(ui->screen_camera_btn_1, 200, 50);
    ui->screen_camera_btn_1_label = lv_label_create(ui->screen_camera_btn_1);
    lv_label_set_text(ui->screen_camera_btn_1_label, "\xE8\xBF\x94\xE5\x9B\x9E\x20" LV_SYMBOL_HOME " ");
    lv_label_set_long_mode(ui->screen_camera_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_camera_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_camera_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_camera_btn_1_label, LV_PCT(100));

    //Write style for screen_camera_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_camera_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_camera_btn_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_camera_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_camera_btn_1, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_camera_btn_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_camera_btn_1, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_camera_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_camera_btn_1, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_camera_btn_1, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_camera_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_camera_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_camera_img_1
    ui->screen_camera_img_1 = lv_image_create(ui->screen_camera);
    lv_obj_set_pos(ui->screen_camera_img_1, 270, 27);
    lv_obj_set_size(ui->screen_camera_img_1, 720, 540);
    lv_obj_add_flag(ui->screen_camera_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_pivot(ui->screen_camera_img_1, 50,50);
    lv_image_set_rotation(ui->screen_camera_img_1, 0);

    //Write style for screen_camera_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_camera_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_camera_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen_camera.


    //Update current screen layout.
    lv_obj_update_layout(ui->screen_camera);

    //Init events for screen.
    events_init_screen_camera(ui);
    camera_ui_bridge_attach(ui);
}
