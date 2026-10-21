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
/* STAGE6 NOTES USER PATCH: restore after GUI Guider regeneration. */
#include "notes_document.h"
#include "notes_editor_ui.h"



int screen_notes_digital_clock_2_min_value = 8;
int screen_notes_digital_clock_2_hour_value = 15;
int screen_notes_digital_clock_2_sec_value = 6;
void setup_scr_screen_notes(lv_ui *ui)
{
    //Write codes screen_notes
    ui->screen_notes = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_notes, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->screen_notes, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_notes, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_notes, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_cont_1
    ui->screen_notes_cont_1 = lv_obj_create(ui->screen_notes);
    lv_obj_set_pos(ui->screen_notes_cont_1, 0, 0);
    lv_obj_set_size(ui->screen_notes_cont_1, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->screen_notes_cont_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_notes_cont_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_notes_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_notes_cont_1, lv_color_hex(0xefefef), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_notes_cont_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_notes_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_notes_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_notes_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_notes_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_ta_note
    ui->screen_notes_ta_note = lv_textarea_create(ui->screen_notes_cont_1);
    lv_obj_set_pos(ui->screen_notes_ta_note, 373, 70);
    lv_obj_set_size(ui->screen_notes_ta_note, 500, 500);
    lv_textarea_set_text(ui->screen_notes_ta_note, "在此编辑可直接保存为新文件");
    lv_textarea_set_placeholder_text(ui->screen_notes_ta_note, "");
    lv_textarea_set_password_bullet(ui->screen_notes_ta_note, "*");
    lv_textarea_set_password_mode(ui->screen_notes_ta_note, false);
    lv_textarea_set_one_line(ui->screen_notes_ta_note, false);
    lv_textarea_set_accepted_chars(ui->screen_notes_ta_note, "");
    /* STAGE6 NOTES USER PATCH: textarea is a 32KB window over the full note. */
    lv_textarea_set_max_length(ui->screen_notes_ta_note,
                               NOTES_WINDOW_MAX_BYTES);
    (void)notes_editor_ui_attach(ui->screen_notes_ta_note);
#if LV_USE_KEYBOARD
    lv_obj_add_event_cb(ui->screen_notes_ta_note, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif

    //Write style for screen_notes_ta_note, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_notes_ta_note, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_ta_note, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_ta_note, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_notes_ta_note, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_ta_note, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_ta_note, 181, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_notes_ta_note, lv_color_hex(0xcecece), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_notes_ta_note, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_notes_ta_note, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_notes_ta_note, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_notes_ta_note, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_notes_ta_note, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_ta_note, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_notes_ta_note, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_notes_ta_note, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_notes_ta_note, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_ta_note, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* Scroll: same logic as Serial_Port_ta_display */
    lv_obj_set_scrollbar_mode(ui->screen_notes_ta_note, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(ui->screen_notes_ta_note, LV_DIR_ALL);
    lv_obj_set_scroll_snap_y(ui->screen_notes_ta_note, LV_SCROLL_SNAP_NONE);
    lv_obj_set_style_anim_time(ui->screen_notes_ta_note, 0, LV_PART_SCROLLBAR);
    lv_obj_add_flag(ui->screen_notes_ta_note, LV_OBJ_FLAG_SCROLLABLE);

    //Write codes screen_notes_btn_return
    ui->screen_notes_btn_return = lv_button_create(ui->screen_notes_cont_1);
    lv_obj_set_pos(ui->screen_notes_btn_return, 902, 138);
    lv_obj_set_size(ui->screen_notes_btn_return, 100, 60);
    ui->screen_notes_btn_return_label = lv_label_create(ui->screen_notes_btn_return);
    lv_label_set_text(ui->screen_notes_btn_return_label, "退出");
    lv_label_set_long_mode(ui->screen_notes_btn_return_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_notes_btn_return_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_notes_btn_return, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_notes_btn_return_label, LV_PCT(100));

    //Write style for screen_notes_btn_return, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_notes_btn_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_notes_btn_return, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_notes_btn_return, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_notes_btn_return, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_notes_btn_return, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_btn_return, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_btn_return, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_btn_return, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_btn_return, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_btn_return, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_btn_return, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_btn_2
    ui->screen_notes_btn_2 = lv_button_create(ui->screen_notes_cont_1);
    lv_obj_set_pos(ui->screen_notes_btn_2, 902, 207);
    lv_obj_set_size(ui->screen_notes_btn_2, 100, 60);
    ui->screen_notes_btn_2_label = lv_label_create(ui->screen_notes_btn_2);
    lv_label_set_text(ui->screen_notes_btn_2_label, "编辑");
    lv_label_set_long_mode(ui->screen_notes_btn_2_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_notes_btn_2_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_notes_btn_2, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_notes_btn_2_label, LV_PCT(100));

    //Write style for screen_notes_btn_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_notes_btn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_notes_btn_2, lv_color_hex(0x90EE90), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_notes_btn_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_notes_btn_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_notes_btn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_notes_btn_2, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_notes_btn_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_btn_2, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_btn_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_btn_2, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_btn_2, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_btn_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_btn_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_list_note
    ui->screen_notes_list_note = lv_list_create(ui->screen_notes_cont_1);
    lv_obj_set_pos(ui->screen_notes_list_note, 40, 73);
    lv_obj_set_size(ui->screen_notes_list_note, 300, 500);
    lv_obj_set_scrollbar_mode(ui->screen_notes_list_note, LV_SCROLLBAR_MODE_OFF);
    ui->screen_notes_list_note_item0 = lv_list_add_button(ui->screen_notes_list_note, LV_SYMBOL_SAVE, "save");

    //Write style state: LV_STATE_DEFAULT for &style_screen_notes_list_note_main_main_default
    static lv_style_t style_screen_notes_list_note_main_main_default;
    ui_init_style(&style_screen_notes_list_note_main_main_default);

    lv_style_set_pad_top(&style_screen_notes_list_note_main_main_default, 5);
    lv_style_set_pad_left(&style_screen_notes_list_note_main_main_default, 5);
    lv_style_set_pad_right(&style_screen_notes_list_note_main_main_default, 5);
    lv_style_set_pad_bottom(&style_screen_notes_list_note_main_main_default, 5);
    lv_style_set_bg_opa(&style_screen_notes_list_note_main_main_default, 255);
    lv_style_set_bg_color(&style_screen_notes_list_note_main_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_screen_notes_list_note_main_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_screen_notes_list_note_main_main_default, 1);
    lv_style_set_border_opa(&style_screen_notes_list_note_main_main_default, 255);
    lv_style_set_border_color(&style_screen_notes_list_note_main_main_default, lv_color_hex(0xe1e6ee));
    lv_style_set_border_side(&style_screen_notes_list_note_main_main_default, LV_BORDER_SIDE_FULL);
    lv_style_set_radius(&style_screen_notes_list_note_main_main_default, 3);
    lv_style_set_shadow_width(&style_screen_notes_list_note_main_main_default, 0);
    lv_obj_add_style(ui->screen_notes_list_note, &style_screen_notes_list_note_main_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_screen_notes_list_note_main_scrollbar_default
    static lv_style_t style_screen_notes_list_note_main_scrollbar_default;
    ui_init_style(&style_screen_notes_list_note_main_scrollbar_default);

    lv_style_set_radius(&style_screen_notes_list_note_main_scrollbar_default, 3);
    lv_style_set_bg_opa(&style_screen_notes_list_note_main_scrollbar_default, 255);
    lv_style_set_bg_color(&style_screen_notes_list_note_main_scrollbar_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_screen_notes_list_note_main_scrollbar_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->screen_notes_list_note, &style_screen_notes_list_note_main_scrollbar_default, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_screen_notes_list_note_extra_btns_main_default
    static lv_style_t style_screen_notes_list_note_extra_btns_main_default;
    ui_init_style(&style_screen_notes_list_note_extra_btns_main_default);

    lv_style_set_pad_top(&style_screen_notes_list_note_extra_btns_main_default, 5);
    lv_style_set_pad_left(&style_screen_notes_list_note_extra_btns_main_default, 5);
    lv_style_set_pad_right(&style_screen_notes_list_note_extra_btns_main_default, 5);
    lv_style_set_pad_bottom(&style_screen_notes_list_note_extra_btns_main_default, 5);
    lv_style_set_border_width(&style_screen_notes_list_note_extra_btns_main_default, 0);
    lv_style_set_text_color(&style_screen_notes_list_note_extra_btns_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_screen_notes_list_note_extra_btns_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_screen_notes_list_note_extra_btns_main_default, 255);
    lv_style_set_radius(&style_screen_notes_list_note_extra_btns_main_default, 3);
    lv_style_set_bg_opa(&style_screen_notes_list_note_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_screen_notes_list_note_extra_btns_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_screen_notes_list_note_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->screen_notes_list_note_item0, &style_screen_notes_list_note_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_screen_notes_list_note_extra_texts_main_default
    static lv_style_t style_screen_notes_list_note_extra_texts_main_default;
    ui_init_style(&style_screen_notes_list_note_extra_texts_main_default);

    lv_style_set_pad_top(&style_screen_notes_list_note_extra_texts_main_default, 5);
    lv_style_set_pad_left(&style_screen_notes_list_note_extra_texts_main_default, 5);
    lv_style_set_pad_right(&style_screen_notes_list_note_extra_texts_main_default, 5);
    lv_style_set_pad_bottom(&style_screen_notes_list_note_extra_texts_main_default, 5);
    lv_style_set_border_width(&style_screen_notes_list_note_extra_texts_main_default, 0);
    lv_style_set_text_color(&style_screen_notes_list_note_extra_texts_main_default, lv_color_hex(0x0D3055));
    lv_style_set_text_font(&style_screen_notes_list_note_extra_texts_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_screen_notes_list_note_extra_texts_main_default, 255);
    lv_style_set_radius(&style_screen_notes_list_note_extra_texts_main_default, 3);
    lv_style_set_transform_width(&style_screen_notes_list_note_extra_texts_main_default, 0);
    lv_style_set_bg_opa(&style_screen_notes_list_note_extra_texts_main_default, 255);
    lv_style_set_bg_color(&style_screen_notes_list_note_extra_texts_main_default, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_dir(&style_screen_notes_list_note_extra_texts_main_default, LV_GRAD_DIR_NONE);

    //Write codes screen_notes_btn_1
    ui->screen_notes_btn_1 = lv_button_create(ui->screen_notes_cont_1);
    lv_obj_set_pos(ui->screen_notes_btn_1, 902, 71);
    lv_obj_set_size(ui->screen_notes_btn_1, 100, 60);
    ui->screen_notes_btn_1_label = lv_label_create(ui->screen_notes_btn_1);
    lv_label_set_text(ui->screen_notes_btn_1_label, "保存并退出");
    lv_label_set_long_mode(ui->screen_notes_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_notes_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_notes_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_notes_btn_1_label, LV_PCT(100));

    //Write style for screen_notes_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_notes_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_notes_btn_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_notes_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_notes_btn_1, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_notes_btn_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_btn_1, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_btn_1, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_btn_1, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_tileview_note
    ui->screen_notes_tileview_note = lv_tileview_create(ui->screen_notes);
    lv_obj_set_pos(ui->screen_notes_tileview_note, 271, 83);
    lv_obj_set_size(ui->screen_notes_tileview_note, 500, 260);
    lv_obj_set_scrollbar_mode(ui->screen_notes_tileview_note, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->screen_notes_tileview_note, LV_OBJ_FLAG_HIDDEN);
    ui->screen_notes_tileview_note_tile = lv_tileview_add_tile(ui->screen_notes_tileview_note, 0, 0, LV_DIR_RIGHT);

    //Write style for screen_notes_tileview_note, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_notes_tileview_note, 243, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_notes_tileview_note, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_notes_tileview_note, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_tileview_note, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_tileview_note, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_notes_tileview_note, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_notes_tileview_note, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_notes_tileview_note, lv_color_hex(0xeaeff3), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_notes_tileview_note, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_tileview_note, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);



    //Write codes screen_notes_btn_4
    ui->screen_notes_btn_4 = lv_button_create(ui->screen_notes_tileview_note_tile);
    lv_obj_set_pos(ui->screen_notes_btn_4, 438, 9);
    lv_obj_set_size(ui->screen_notes_btn_4, 50, 40);
    ui->screen_notes_btn_4_label = lv_label_create(ui->screen_notes_btn_4);
    lv_label_set_text(ui->screen_notes_btn_4_label, " " LV_SYMBOL_CLOSE " ");
    lv_label_set_long_mode(ui->screen_notes_btn_4_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_notes_btn_4_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_notes_btn_4, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_notes_btn_4_label, LV_PCT(100));

    //Write style for screen_notes_btn_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_notes_btn_4, 139, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_notes_btn_4, lv_color_hex(0xf44444), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_notes_btn_4, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_notes_btn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_btn_4, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_btn_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_btn_4, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_btn_4, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_btn_4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_btn_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_btn_3
    ui->screen_notes_btn_3 = lv_button_create(ui->screen_notes_tileview_note_tile);
    lv_obj_set_pos(ui->screen_notes_btn_3, 133, 185);
    lv_obj_set_size(ui->screen_notes_btn_3, 100, 50);
    ui->screen_notes_btn_3_label = lv_label_create(ui->screen_notes_btn_3);
    lv_label_set_text(ui->screen_notes_btn_3_label, "保存");
    lv_label_set_long_mode(ui->screen_notes_btn_3_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_notes_btn_3_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_notes_btn_3, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_notes_btn_3_label, LV_PCT(100));

    //Write style for screen_notes_btn_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_notes_btn_3, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_notes_btn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_notes_btn_3, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_notes_btn_3, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_btn_3, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_btn_3, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_btn_3, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_btn_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_btn_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_btn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_btn_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_label_5
    ui->screen_notes_label_5 = lv_label_create(ui->screen_notes_tileview_note_tile);
    lv_obj_set_pos(ui->screen_notes_label_5, 206, 16);
    lv_obj_set_size(ui->screen_notes_label_5, 80, 30);
    lv_label_set_text(ui->screen_notes_label_5, "保存文件");
    lv_label_set_long_mode(ui->screen_notes_label_5, LV_LABEL_LONG_WRAP);

    //Write style for screen_notes_label_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_notes_label_5, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_label_5, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_label_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_notes_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_notes_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_label_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_notes_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_notes_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_notes_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_notes_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_notes_label_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_ta_1
    ui->screen_notes_ta_1 = lv_textarea_create(ui->screen_notes_tileview_note_tile);
    lv_obj_set_pos(ui->screen_notes_ta_1, 169, 59);
    lv_obj_set_size(ui->screen_notes_ta_1, 200, 40);
    lv_textarea_set_text(ui->screen_notes_ta_1, "Note1");
    lv_textarea_set_placeholder_text(ui->screen_notes_ta_1, "");
    lv_textarea_set_password_bullet(ui->screen_notes_ta_1, "*");
    lv_textarea_set_password_mode(ui->screen_notes_ta_1, false);
    lv_textarea_set_one_line(ui->screen_notes_ta_1, false);
    lv_textarea_set_accepted_chars(ui->screen_notes_ta_1, "");
    lv_textarea_set_max_length(ui->screen_notes_ta_1, 32);
#if LV_USE_KEYBOARD
    lv_obj_add_event_cb(ui->screen_notes_ta_1, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif

    //Write style for screen_notes_ta_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_notes_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_notes_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_notes_ta_1, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_notes_ta_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_ta_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_ta_1, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_notes_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_ta_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_notes_ta_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_notes_ta_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_notes_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_notes_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_notes_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_ta_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_notes_ta_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_notes_ta_1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_notes_ta_1, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_notes_ta_1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_ta_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes screen_notes_label_6
    ui->screen_notes_label_6 = lv_label_create(ui->screen_notes_tileview_note_tile);
    lv_obj_set_pos(ui->screen_notes_label_6, 65, 64);
    lv_obj_set_size(ui->screen_notes_label_6, 95, 30);
    lv_label_set_text(ui->screen_notes_label_6, "文件名称");
    lv_label_set_long_mode(ui->screen_notes_label_6, LV_LABEL_LONG_WRAP);

    //Write style for screen_notes_label_6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_notes_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_label_6, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_label_6, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_label_6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_notes_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_notes_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_label_6, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_notes_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_notes_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_notes_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_notes_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_label_6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_ta_2
    ui->screen_notes_ta_2 = lv_textarea_create(ui->screen_notes_tileview_note_tile);
    lv_obj_set_pos(ui->screen_notes_ta_2, 171, 123);
    lv_obj_set_size(ui->screen_notes_ta_2, 200, 40);
    lv_textarea_set_text(ui->screen_notes_ta_2, "0：/Note");
    lv_textarea_set_placeholder_text(ui->screen_notes_ta_2, "");
    lv_textarea_set_password_bullet(ui->screen_notes_ta_2, "*");
    lv_textarea_set_password_mode(ui->screen_notes_ta_2, false);
    lv_textarea_set_one_line(ui->screen_notes_ta_2, false);
    lv_textarea_set_accepted_chars(ui->screen_notes_ta_2, "");
    lv_textarea_set_max_length(ui->screen_notes_ta_2, 32);
#if LV_USE_KEYBOARD
    lv_obj_add_event_cb(ui->screen_notes_ta_2, ta_event_cb, LV_EVENT_ALL, ui->g_kb_top_layer);
#endif

    //Write style for screen_notes_ta_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_notes_ta_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_notes_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_notes_ta_2, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_notes_ta_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_ta_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_ta_2, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_notes_ta_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_ta_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_notes_ta_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_notes_ta_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_notes_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_notes_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_notes_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_ta_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_notes_ta_2, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_notes_ta_2, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_notes_ta_2, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_notes_ta_2, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_ta_2, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes screen_notes_label_7
    ui->screen_notes_label_7 = lv_label_create(ui->screen_notes_tileview_note_tile);
    lv_obj_set_pos(ui->screen_notes_label_7, 65, 130);
    lv_obj_set_size(ui->screen_notes_label_7, 95, 30);
    lv_label_set_text(ui->screen_notes_label_7, "文件路径");
    lv_label_set_long_mode(ui->screen_notes_label_7, LV_LABEL_LONG_WRAP);

    //Write style for screen_notes_label_7, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_notes_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_label_7, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_label_7, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_label_7, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_notes_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_notes_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_label_7, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_notes_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_notes_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_notes_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_notes_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_btn_5
    ui->screen_notes_btn_5 = lv_button_create(ui->screen_notes_tileview_note_tile);
    lv_obj_set_pos(ui->screen_notes_btn_5, 273, 185);
    lv_obj_set_size(ui->screen_notes_btn_5, 100, 50);
    ui->screen_notes_btn_5_label = lv_label_create(ui->screen_notes_btn_5);
    lv_label_set_text(ui->screen_notes_btn_5_label, "不保存");
    lv_label_set_long_mode(ui->screen_notes_btn_5_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_notes_btn_5_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_notes_btn_5, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_notes_btn_5_label, LV_PCT(100));

    //Write style for screen_notes_btn_5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_notes_btn_5, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_notes_btn_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_notes_btn_5, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_notes_btn_5, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_btn_5, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_btn_5, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_btn_5, &lv_font_myyh_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_btn_5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_btn_5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_btn_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_btn_5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_cont_2
    ui->screen_notes_cont_2 = lv_obj_create(ui->screen_notes);
    lv_obj_set_pos(ui->screen_notes_cont_2, 0, 0);
    lv_obj_set_size(ui->screen_notes_cont_2, 1024, 35);
    lv_obj_set_scrollbar_mode(ui->screen_notes_cont_2, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_notes_cont_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_notes_cont_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_notes_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_notes_cont_2, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_notes_cont_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_notes_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_notes_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_notes_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_notes_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_label_3
    ui->screen_notes_label_3 = lv_label_create(ui->screen_notes_cont_2);
    lv_obj_set_pos(ui->screen_notes_label_3, 810, 5);
    lv_obj_set_size(ui->screen_notes_label_3, 210, 30);
    lv_label_set_text(ui->screen_notes_label_3, "   " LV_SYMBOL_WIFI "  " LV_SYMBOL_VOLUME_MAX " " LV_SYMBOL_USB " " LV_SYMBOL_BATTERY_FULL "  100%");
    lv_label_set_long_mode(ui->screen_notes_label_3, LV_LABEL_LONG_WRAP);

    //Write style for screen_notes_label_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_notes_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_label_3, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_label_3, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_label_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_notes_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_notes_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_label_3, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_notes_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_notes_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_notes_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_notes_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_label_2
    ui->screen_notes_label_2 = lv_label_create(ui->screen_notes_cont_2);
    lv_obj_set_pos(ui->screen_notes_label_2, 462, -141);
    lv_obj_set_size(ui->screen_notes_label_2, 100, 32);
    lv_label_set_text(ui->screen_notes_label_2, "");
    lv_label_set_long_mode(ui->screen_notes_label_2, LV_LABEL_LONG_WRAP);

    //Write style for screen_notes_label_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_notes_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_notes_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_label_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_label_2, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_label_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_notes_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_notes_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_label_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_notes_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_notes_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_notes_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_notes_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_notes_digital_clock_2
    static bool screen_notes_digital_clock_2_timer_enabled = false;
    ui->screen_notes_digital_clock_2 = lv_label_create(ui->screen_notes_cont_2);
    lv_obj_set_pos(ui->screen_notes_digital_clock_2, 0, 0);
    lv_obj_set_size(ui->screen_notes_digital_clock_2, 100, 35);
    lv_label_set_text(ui->screen_notes_digital_clock_2, "15:08");
    if (!screen_notes_digital_clock_2_timer_enabled) {
        lv_timer_create(screen_notes_digital_clock_2_timer, 1000, NULL);
        screen_notes_digital_clock_2_timer_enabled = true;
    }

    //Write style for screen_notes_digital_clock_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_notes_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_notes_digital_clock_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_notes_digital_clock_2, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_notes_digital_clock_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_notes_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_notes_digital_clock_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_notes_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_notes_digital_clock_2, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_notes_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_notes_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_notes_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_notes_digital_clock_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen_notes.


    //Update current screen layout.
    lv_obj_update_layout(ui->screen_notes);

    //Init events for screen.
    events_init_screen_notes(ui);
}
