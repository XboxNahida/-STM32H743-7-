/*
 * STAGE6 UI USER PATCH
 * Static drawing interface based on the approved 1024x600 mockup.
 */
#include "lvgl.h"
#include "gui_guider.h"
#include "events_init.h"
#include "drawing_app_ui.h"

static const uint32_t g_drawing_colors[6] = {
    0x1f2325U, 0xe3ad37U, 0xd95d58U,
    0x3d9a6bU, 0x2990b7U, 0xf4f2edU
};

static lv_obj_t *drawing_label(lv_obj_t *parent, const char *text,
                               const lv_font_t *font)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(0xf4f6f7),
                                LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER,
                                LV_PART_MAIN|LV_STATE_DEFAULT);
    return label;
}

static lv_obj_t *drawing_button(lv_obj_t *parent, const char *text)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_t *label = drawing_label(button, text, &lv_font_myyh_20);
    lv_obj_center(label);
    lv_obj_set_style_radius(button, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x252a2d),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(button, 1,
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(button, lv_color_hex(0x434a4e),
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(button, 0,
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
    return button;
}

static void stage6_create_drawing_navigation(lv_ui *ui)
{
    ui->screen_drawing_cont_1 = lv_obj_create(ui->screen_drawing);
    lv_obj_set_pos(ui->screen_drawing_cont_1, 0, 0);
    lv_obj_set_size(ui->screen_drawing_cont_1, 1024, 35);
    lv_obj_set_scrollbar_mode(ui->screen_drawing_cont_1,
                              LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(ui->screen_drawing_cont_1, 0,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_drawing_cont_1, 0,
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_drawing_cont_1,
                              lv_color_hex(0x111416),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_drawing_cont_1, 255,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->screen_drawing_cont_1, 0,
                             LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->screen_drawing_digital_clock_1 = drawing_label(
        ui->screen_drawing_cont_1, "15:08", &lv_font_montserratMedium_20);
    lv_obj_set_pos(ui->screen_drawing_digital_clock_1, 0, 5);
    lv_obj_set_size(ui->screen_drawing_digital_clock_1, 100, 30);

    ui->screen_drawing_title = drawing_label(
        ui->screen_drawing_cont_1, "画板", &lv_font_myyh_20);
    lv_obj_set_pos(ui->screen_drawing_title, 462, 5);
    lv_obj_set_size(ui->screen_drawing_title, 100, 30);

    ui->screen_drawing_label_2 = drawing_label(
        ui->screen_drawing_cont_1,
        "   " LV_SYMBOL_WIFI "  " LV_SYMBOL_VOLUME_MAX " "
        LV_SYMBOL_USB " " LV_SYMBOL_BATTERY_FULL "  100%",
        &lv_font_montserratMedium_20);
    lv_obj_set_pos(ui->screen_drawing_label_2, 800, 5);
    lv_obj_set_size(ui->screen_drawing_label_2, 220, 30);

    ui->screen_drawing_label_1 = drawing_label(
        ui->screen_drawing_cont_1, "", &lv_font_myyh_20);
    lv_obj_set_pos(ui->screen_drawing_label_1, 462, -141);
    lv_obj_set_size(ui->screen_drawing_label_1, 100, 32);
    lv_label_set_long_mode(ui->screen_drawing_label_1, LV_LABEL_LONG_WRAP);
}

static void stage6_create_drawing_tools(
    lv_ui *ui, lv_obj_t *swatches[DRAWING_COLOR_COUNT])
{
    uint32_t index;

    ui->screen_drawing_tools = lv_obj_create(ui->screen_drawing);
    lv_obj_set_pos(ui->screen_drawing_tools, 0, 35);
    lv_obj_set_size(ui->screen_drawing_tools, 128, 565);
    lv_obj_set_scrollbar_mode(ui->screen_drawing_tools,
                              LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(ui->screen_drawing_tools, 0,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_drawing_tools, 0,
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_drawing_tools,
                              lv_color_hex(0x121517),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_drawing_tools, 255,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->screen_drawing_tools, 0,
                             LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->screen_drawing_btn_back = drawing_button(
        ui->screen_drawing_tools, "返回");
    lv_obj_set_pos(ui->screen_drawing_btn_back, 9, 14);
    lv_obj_set_size(ui->screen_drawing_btn_back, 110, 50);

    ui->screen_drawing_pen_btn = drawing_button(
        ui->screen_drawing_tools, "画笔");
    lv_obj_set_pos(ui->screen_drawing_pen_btn, 9, 78);
    lv_obj_set_size(ui->screen_drawing_pen_btn, 110, 54);
    lv_obj_set_style_bg_color(ui->screen_drawing_pen_btn,
                              lv_color_hex(0xe8bd4b),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lv_obj_get_child(
        ui->screen_drawing_pen_btn, 0), lv_color_hex(0x171a1c),
        LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->screen_drawing_eraser_btn = drawing_button(
        ui->screen_drawing_tools, "橡皮");
    lv_obj_set_pos(ui->screen_drawing_eraser_btn, 9, 142);
    lv_obj_set_size(ui->screen_drawing_eraser_btn, 110, 54);

    for (index = 0U; index < 6U; index++)
    {
        lv_obj_t *swatch = lv_button_create(ui->screen_drawing_tools);
        int32_t x = 16 + (int32_t)(index % 2U) * 58;
        int32_t y = 224 + (int32_t)(index / 2U) * 64;
        lv_obj_set_pos(swatch, x, y);
        lv_obj_set_size(swatch, 42, 42);
        lv_obj_set_style_radius(swatch, LV_RADIUS_CIRCLE,
                                LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(swatch,
            lv_color_hex(g_drawing_colors[index]),
            LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(swatch, (index == 0U) ? 3 : 1,
                                      LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(swatch, lv_color_hex(0xf4f6f7),
                                      LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(swatch, 0,
                                      LV_PART_MAIN|LV_STATE_DEFAULT);
        swatches[index] = swatch;
    }
}

static void stage6_create_drawing_canvas(lv_ui *ui)
{
    ui->screen_drawing_canvas = lv_canvas_create(ui->screen_drawing);
    lv_obj_set_pos(ui->screen_drawing_canvas, 143, 50);
    lv_obj_set_size(ui->screen_drawing_canvas, 866, 454);
    lv_obj_set_scrollbar_mode(ui->screen_drawing_canvas,
                              LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(ui->screen_drawing_canvas, 3,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_drawing_canvas, 1,
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_drawing_canvas,
                                  lv_color_hex(0xd7dadb),
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_drawing_canvas,
                              lv_color_hex(0xf4f2ed),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_drawing_canvas, 255,
                            LV_PART_MAIN|LV_STATE_DEFAULT);

}

static void stage6_create_drawing_actions(lv_ui *ui)
{
    lv_obj_t *label;

    ui->screen_drawing_actions = lv_obj_create(ui->screen_drawing);
    lv_obj_set_pos(ui->screen_drawing_actions, 143, 515);
    lv_obj_set_size(ui->screen_drawing_actions, 866, 70);
    lv_obj_set_scrollbar_mode(ui->screen_drawing_actions,
                              LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(ui->screen_drawing_actions, 7,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_drawing_actions, 0,
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_drawing_actions,
                              lv_color_hex(0x14181a),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_drawing_actions, 245,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->screen_drawing_actions, 0,
                             LV_PART_MAIN|LV_STATE_DEFAULT);

    label = drawing_label(ui->screen_drawing_actions,
                          "笔画", &lv_font_myyh_20);
    lv_obj_set_pos(label, 12, 17);
    lv_obj_set_size(label, 60, 30);

    ui->screen_drawing_width_slider = lv_slider_create(
        ui->screen_drawing_actions);
    lv_obj_set_pos(ui->screen_drawing_width_slider, 82, 24);
    lv_obj_set_size(ui->screen_drawing_width_slider, 220, 18);
    lv_slider_set_range(ui->screen_drawing_width_slider, 1, 32);
    lv_slider_set_value(ui->screen_drawing_width_slider, 8,
                        LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ui->screen_drawing_width_slider,
                              lv_color_hex(0x535b60),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_drawing_width_slider,
                              lv_color_hex(0xe8bd4b),
                              LV_PART_INDICATOR|LV_STATE_DEFAULT);

    ui->screen_drawing_undo_btn = drawing_button(
        ui->screen_drawing_actions, "撤销");
    lv_obj_set_pos(ui->screen_drawing_undo_btn, 356, 9);
    lv_obj_set_size(ui->screen_drawing_undo_btn, 110, 48);

    ui->screen_drawing_redo_btn = drawing_button(
        ui->screen_drawing_actions, "重做");
    lv_obj_set_pos(ui->screen_drawing_redo_btn, 478, 9);
    lv_obj_set_size(ui->screen_drawing_redo_btn, 110, 48);

    ui->screen_drawing_clear_btn = drawing_button(
        ui->screen_drawing_actions, "清空");
    lv_obj_set_pos(ui->screen_drawing_clear_btn, 600, 9);
    lv_obj_set_size(ui->screen_drawing_clear_btn, 110, 48);

    ui->screen_drawing_save_btn = drawing_button(
        ui->screen_drawing_actions, "保存");
    lv_obj_set_pos(ui->screen_drawing_save_btn, 722, 9);
    lv_obj_set_size(ui->screen_drawing_save_btn, 110, 48);
    lv_obj_set_style_bg_color(ui->screen_drawing_save_btn,
                              lv_color_hex(0x69c592),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lv_obj_get_child(
        ui->screen_drawing_save_btn, 0), lv_color_hex(0x171a1c),
        LV_PART_MAIN|LV_STATE_DEFAULT);
}

void setup_scr_screen_drawing(lv_ui *ui)
{
    lv_obj_t *swatches[DRAWING_COLOR_COUNT] = {NULL};

    ui->screen_drawing = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_drawing, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->screen_drawing, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(ui->screen_drawing, lv_color_hex(0x202427),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_drawing, 255,
                            LV_PART_MAIN|LV_STATE_DEFAULT);

    stage6_create_drawing_navigation(ui);
    stage6_create_drawing_tools(ui, swatches);
    stage6_create_drawing_canvas(ui);
    stage6_create_drawing_actions(ui);
    /* STAGE6 DRAWING APP USER PATCH BEGIN */
    (void)drawing_app_ui_attach(ui, swatches);
    /* STAGE6 DRAWING APP USER PATCH END */
    lv_obj_update_layout(ui->screen_drawing);
    events_init_screen_drawing(ui);
}
