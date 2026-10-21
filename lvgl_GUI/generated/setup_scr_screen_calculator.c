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
#include "calculator_controller.h"

/* STAGE6 CALCULATOR USER PATCH BEGIN */
static calculator_controller_t g_calculator;
static const char *g_calculator_button_map[] = {
    "2^nd", "(", ")", "10^x", "mc", "m+", "m-", "mr", "\n",
    "1/X", "X^2", "X^3", "X^y", "AC", "<-", "+/-", "/", "\n",
    "X!", "sqr(X;2)", "sqr(X;Y)", "Lg", "7", "8", "9", "*", "\n",
    "sin", "cos", "tan", "In", "4", "5", "6", "-", "\n",
    "sinh", "cosh", "tanh", "e^x", "1", "2", "3", "+", "\n",
    "Rad", "Pi", "EE", "Rand", "%", "0", ".", "=", ""
};

static void calculator_sync_display(lv_ui *ui)
{
    if ((ui == NULL) || (ui->screen_calculator_ta_1 == NULL) ||
        (ui->screen_calculator_ta_2 == NULL)) return;
    lv_textarea_set_text(ui->screen_calculator_ta_1,
                         g_calculator.expression);
    lv_textarea_set_text(ui->screen_calculator_ta_2,
                         g_calculator.result_text);
    lv_textarea_set_cursor_pos(ui->screen_calculator_ta_1,
                               LV_TEXTAREA_CURSOR_LAST);
    lv_textarea_set_cursor_pos(ui->screen_calculator_ta_2,
                               LV_TEXTAREA_CURSOR_LAST);
}

static void calculator_update_button_map(lv_ui *ui)
{
    if ((ui == NULL) || (ui->screen_calculator_btnm_1 == NULL)) return;
    g_calculator_button_map[3] = (g_calculator.second_mode != 0U) ?
        "2^x" : "10^x";
    g_calculator_button_map[27] = (g_calculator.second_mode != 0U) ?
        "asin" : "sin";
    g_calculator_button_map[28] = (g_calculator.second_mode != 0U) ?
        "acos" : "cos";
    g_calculator_button_map[29] = (g_calculator.second_mode != 0U) ?
        "atan" : "tan";
    g_calculator_button_map[36] = (g_calculator.second_mode != 0U) ?
        "asinh" : "sinh";
    g_calculator_button_map[37] = (g_calculator.second_mode != 0U) ?
        "acosh" : "cosh";
    g_calculator_button_map[38] = (g_calculator.second_mode != 0U) ?
        "atanh" : "tanh";
    g_calculator_button_map[45] =
        (g_calculator.angle_mode == CALCULATOR_ANGLE_RAD) ? "Deg" : "Rad";
    lv_buttonmatrix_set_map(ui->screen_calculator_btnm_1,
                            g_calculator_button_map);
    if (g_calculator.second_mode != 0U)
    {
        lv_buttonmatrix_set_button_ctrl(ui->screen_calculator_btnm_1, 0U,
                                        LV_BUTTONMATRIX_CTRL_CHECKED);
    }
    else
    {
        lv_buttonmatrix_clear_button_ctrl(ui->screen_calculator_btnm_1, 0U,
                                          LV_BUTTONMATRIX_CTRL_CHECKED);
    }
}

static void screen_calculator_button_event_handler(lv_event_t *event)
{
    lv_ui *ui;
    lv_obj_t *button_matrix;
    uint32_t button_id;
    const char *button_text;

    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) return;
    ui = (lv_ui *)lv_event_get_user_data(event);
    button_matrix = lv_event_get_target_obj(event);
    if ((ui == NULL) || (button_matrix == NULL)) return;
    button_id = lv_buttonmatrix_get_selected_button(button_matrix);
    if (button_id == LV_BUTTONMATRIX_BUTTON_NONE) return;
    button_text = lv_buttonmatrix_get_button_text(button_matrix, button_id);
    if ((button_text == NULL) || (button_text[0] == '\0')) return;
    (void)calculator_controller_press(&g_calculator, button_text);
    calculator_update_button_map(ui);
    calculator_sync_display(ui);
}

static void calculator_make_display_only(lv_obj_t *textarea)
{
    lv_textarea_set_cursor_click_pos(textarea, false);
    lv_obj_remove_flag(textarea, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(textarea, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_set_style_bg_opa(textarea, LV_OPA_TRANSP,
                            LV_PART_CURSOR|LV_STATE_DEFAULT);
}
/* STAGE6 CALCULATOR USER PATCH END */


/* STAGE6 UI USER PATCH: this screen uses the existing shared RTC updater. */
void setup_scr_screen_calculator(lv_ui *ui)
{
    //Write codes screen_calculator
    ui->screen_calculator = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_calculator, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->screen_calculator, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_calculator, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_calculator, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_calculator, lv_color_hex(0xd7d7d7), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_calculator, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_calculator_cont_1
    ui->screen_calculator_cont_1 = lv_obj_create(ui->screen_calculator);
    lv_obj_set_pos(ui->screen_calculator_cont_1, 0, 0);
    lv_obj_set_size(ui->screen_calculator_cont_1, 1024, 35);
    lv_obj_set_scrollbar_mode(ui->screen_calculator_cont_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_calculator_cont_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_calculator_cont_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_calculator_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_calculator_cont_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_calculator_cont_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_calculator_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_calculator_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_calculator_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_calculator_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_calculator_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_calculator_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_calculator_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_calculator_label_2
    ui->screen_calculator_label_2 = lv_label_create(ui->screen_calculator_cont_1);
    lv_obj_set_pos(ui->screen_calculator_label_2, 810, 5);
    lv_obj_set_size(ui->screen_calculator_label_2, 210, 30);
    lv_label_set_text(ui->screen_calculator_label_2, "   " LV_SYMBOL_WIFI "  " LV_SYMBOL_VOLUME_MAX " " LV_SYMBOL_USB " " LV_SYMBOL_BATTERY_FULL "  100%");
    lv_label_set_long_mode(ui->screen_calculator_label_2, LV_LABEL_LONG_WRAP);

    //Write style for screen_calculator_label_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_calculator_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_calculator_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_calculator_label_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_calculator_label_2, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_calculator_label_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_calculator_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_calculator_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_calculator_label_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_calculator_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_calculator_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_calculator_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_calculator_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_calculator_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_calculator_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_calculator_label_1
    ui->screen_calculator_label_1 = lv_label_create(ui->screen_calculator_cont_1);
    lv_obj_set_pos(ui->screen_calculator_label_1, 462, -141);
    lv_obj_set_size(ui->screen_calculator_label_1, 100, 32);
    lv_label_set_text(ui->screen_calculator_label_1, "");
    lv_label_set_long_mode(ui->screen_calculator_label_1, LV_LABEL_LONG_WRAP);

    //Write style for screen_calculator_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_calculator_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_calculator_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_calculator_label_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_calculator_label_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_calculator_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_calculator_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_calculator_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_calculator_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_calculator_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_calculator_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_calculator_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_calculator_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_calculator_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_calculator_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_calculator_digital_clock_1
    ui->screen_calculator_digital_clock_1 = lv_label_create(ui->screen_calculator_cont_1);
    lv_obj_set_pos(ui->screen_calculator_digital_clock_1, 0, 0);
    lv_obj_set_size(ui->screen_calculator_digital_clock_1, 100, 35);
    lv_label_set_text(ui->screen_calculator_digital_clock_1, "15:08");

    //Write style for screen_calculator_digital_clock_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_calculator_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_calculator_digital_clock_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_calculator_digital_clock_1, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_calculator_digital_clock_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_calculator_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_calculator_digital_clock_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_calculator_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_calculator_digital_clock_1, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_calculator_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_calculator_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_calculator_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_calculator_digital_clock_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_calculator_btnm_1
    ui->screen_calculator_btnm_1 = lv_buttonmatrix_create(ui->screen_calculator);
    lv_obj_set_pos(ui->screen_calculator_btnm_1, 10, 239);
    lv_obj_set_size(ui->screen_calculator_btnm_1, 1000, 350);
    lv_buttonmatrix_set_map(ui->screen_calculator_btnm_1,
                            g_calculator_button_map);

    //Write style for screen_calculator_btnm_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_calculator_btnm_1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_calculator_btnm_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_calculator_btnm_1, lv_color_hex(0xc9c9c9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_calculator_btnm_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_calculator_btnm_1, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_calculator_btnm_1, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_calculator_btnm_1, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_calculator_btnm_1, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(ui->screen_calculator_btnm_1, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(ui->screen_calculator_btnm_1, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_calculator_btnm_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_calculator_btnm_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_calculator_btnm_1, lv_color_hex(0xb8b8b8), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_calculator_btnm_1, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->screen_calculator_btnm_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->screen_calculator_btnm_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->screen_calculator_btnm_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_calculator_btnm_1, Part: LV_PART_ITEMS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_calculator_btnm_1, 1, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_calculator_btnm_1, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_calculator_btnm_1, lv_color_hex(0xc9c9c9), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_calculator_btnm_1, LV_BORDER_SIDE_FULL, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_calculator_btnm_1, lv_color_hex(0x860b1e), LV_PART_ITEMS|LV_STATE_DEFAULT);
    /* STAGE6 UI USER PATCH: use the project's existing ASCII font. */
    lv_obj_set_style_text_font(ui->screen_calculator_btnm_1, &lv_font_montserratMedium_20, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_calculator_btnm_1, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_calculator_btnm_1, 4, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_calculator_btnm_1, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_calculator_btnm_1, lv_color_hex(0xa4a4a4), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_calculator_btnm_1, LV_GRAD_DIR_HOR, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->screen_calculator_btnm_1, lv_color_hex(0xeeeeee), LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->screen_calculator_btnm_1, 0, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->screen_calculator_btnm_1, 255, LV_PART_ITEMS|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_calculator_btnm_1, 0, LV_PART_ITEMS|LV_STATE_DEFAULT);

    //Write style for screen_calculator_btnm_1, Part: LV_PART_ITEMS, State: LV_STATE_PRESSED.
    lv_obj_set_style_border_width(ui->screen_calculator_btnm_1, 1, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(ui->screen_calculator_btnm_1, 255, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_border_color(ui->screen_calculator_btnm_1, lv_color_hex(0xc9c9c9), LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_border_side(ui->screen_calculator_btnm_1, LV_BORDER_SIDE_FULL, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_text_color(ui->screen_calculator_btnm_1, lv_color_hex(0xffffff), LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_text_font(ui->screen_calculator_btnm_1, &lv_font_montserratMedium_20, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_text_opa(ui->screen_calculator_btnm_1, 255, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_radius(ui->screen_calculator_btnm_1, 4, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui->screen_calculator_btnm_1, 255, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(ui->screen_calculator_btnm_1, lv_color_hex(0xb3a9cc), LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_bg_grad_dir(ui->screen_calculator_btnm_1, LV_GRAD_DIR_NONE, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(ui->screen_calculator_btnm_1, 4, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_color(ui->screen_calculator_btnm_1, lv_color_hex(0xb3a9cc), LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_opa(ui->screen_calculator_btnm_1, 255, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_spread(ui->screen_calculator_btnm_1, 2, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_offset_x(ui->screen_calculator_btnm_1, 0, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_shadow_offset_y(ui->screen_calculator_btnm_1, 2, LV_PART_ITEMS|LV_STATE_PRESSED);
    lv_obj_set_style_text_color(ui->screen_calculator_btnm_1,
                                lv_color_hex(0xffffff),
                                LV_PART_ITEMS|LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(ui->screen_calculator_btnm_1,
                              lv_color_hex(0x7b5fa8),
                              LV_PART_ITEMS|LV_STATE_CHECKED);

    //Write codes screen_calculator_ta_1
    ui->screen_calculator_ta_1 = lv_textarea_create(ui->screen_calculator);
    lv_obj_set_pos(ui->screen_calculator_ta_1, 10, 44);
    lv_obj_set_size(ui->screen_calculator_ta_1, 1000, 115);
    lv_textarea_set_text(ui->screen_calculator_ta_1, "0");
    lv_textarea_set_placeholder_text(ui->screen_calculator_ta_1, "");
    lv_textarea_set_password_bullet(ui->screen_calculator_ta_1, "*");
    lv_textarea_set_password_mode(ui->screen_calculator_ta_1, false);
    lv_textarea_set_one_line(ui->screen_calculator_ta_1, false);
    lv_textarea_set_accepted_chars(ui->screen_calculator_ta_1, "");
    lv_textarea_set_max_length(ui->screen_calculator_ta_1, 512);
    calculator_make_display_only(ui->screen_calculator_ta_1);

    //Write style for screen_calculator_ta_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_calculator_ta_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_calculator_ta_1, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_calculator_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_calculator_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_calculator_ta_1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_calculator_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_calculator_ta_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_calculator_ta_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_calculator_ta_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_calculator_ta_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_calculator_ta_1, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_calculator_ta_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_calculator_ta_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_calculator_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_calculator_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_calculator_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_calculator_ta_1, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_calculator_ta_1, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_calculator_ta_1, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_calculator_ta_1, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_calculator_ta_1, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_calculator_ta_1, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes screen_calculator_ta_2
    ui->screen_calculator_ta_2 = lv_textarea_create(ui->screen_calculator);
    lv_obj_set_pos(ui->screen_calculator_ta_2, 510, 173);
    lv_obj_set_size(ui->screen_calculator_ta_2, 500, 60);
    lv_textarea_set_text(ui->screen_calculator_ta_2, "");
    lv_textarea_set_placeholder_text(ui->screen_calculator_ta_2, "");
    lv_textarea_set_password_bullet(ui->screen_calculator_ta_2, "*");
    lv_textarea_set_password_mode(ui->screen_calculator_ta_2, false);
    lv_textarea_set_one_line(ui->screen_calculator_ta_2, false);
    lv_textarea_set_accepted_chars(ui->screen_calculator_ta_2, "");
    lv_textarea_set_max_length(ui->screen_calculator_ta_2, 512);
    calculator_make_display_only(ui->screen_calculator_ta_2);

    //Write style for screen_calculator_ta_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_calculator_ta_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_calculator_ta_2, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_calculator_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_calculator_ta_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_calculator_ta_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_calculator_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_calculator_ta_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_calculator_ta_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_calculator_ta_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_calculator_ta_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_calculator_ta_2, lv_color_hex(0xe6e6e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_calculator_ta_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_calculator_ta_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_calculator_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_calculator_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_calculator_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_calculator_ta_2, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for screen_calculator_ta_2, Part: LV_PART_SCROLLBAR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_calculator_ta_2, 255, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_calculator_ta_2, lv_color_hex(0x2195f6), LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_calculator_ta_2, LV_GRAD_DIR_NONE, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_calculator_ta_2, 0, LV_PART_SCROLLBAR|LV_STATE_DEFAULT);

    //Write codes screen_calculator_btn_1
    ui->screen_calculator_btn_1 = lv_button_create(ui->screen_calculator);
    lv_obj_set_pos(ui->screen_calculator_btn_1, 20, 173);
    lv_obj_set_size(ui->screen_calculator_btn_1, 100, 50);
    ui->screen_calculator_btn_1_label = lv_label_create(ui->screen_calculator_btn_1);
    lv_label_set_text(ui->screen_calculator_btn_1_label, "返回");
    lv_label_set_long_mode(ui->screen_calculator_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_calculator_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_calculator_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_calculator_btn_1_label, LV_PCT(100));

    //Write style for screen_calculator_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_calculator_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_calculator_btn_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_calculator_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_calculator_btn_1, lv_color_hex(0x5482a9), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_calculator_btn_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_calculator_btn_1, 25, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_calculator_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_calculator_btn_1, lv_color_hex(0x12548b), LV_PART_MAIN|LV_STATE_DEFAULT);
    /* STAGE6 UI USER PATCH: visible Chinese uses the complete 20px font. */
    lv_obj_set_style_text_font(ui->screen_calculator_btn_1, &lv_font_myyh_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_calculator_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_calculator_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen_calculator.


    //Update current screen layout.
    lv_obj_update_layout(ui->screen_calculator);

    /* STAGE6 UI USER PATCH: bind return and top navigation events. */
    events_init_screen_calculator(ui);
    calculator_controller_init(&g_calculator);
    calculator_update_button_map(ui);
    calculator_sync_display(ui);
    lv_obj_add_event_cb(ui->screen_calculator_btnm_1,
                        screen_calculator_button_event_handler,
                        LV_EVENT_VALUE_CHANGED, ui);

}
