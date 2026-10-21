#ifndef CAMERA_UI_STYLE_H
#define CAMERA_UI_STYLE_H

#include "camera_ui_bridge.h"

static void camera_style_label(lv_obj_t *label, uint32_t color)
{
    lv_obj_set_style_text_font(label, &lv_font_myyh_20, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_letter_space(label, 0, 0);
}

static void camera_style_button(lv_obj_t *button)
{
    lv_obj_t *label = lv_obj_get_child(button, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x242424), 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x383838),
                              LV_STATE_PRESSED);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_radius(button, 6, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_set_style_pad_all(button, 0, 0);
    if (label != NULL)
    {
        camera_style_label(label, 0xF2F2F2);
        lv_obj_center(label);
    }
}

static lv_obj_t *camera_make_button(lv_obj_t *parent, int32_t x, int32_t y,
                                    int32_t width, int32_t height,
                                    const char *text, lv_event_cb_t callback)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_t *label = lv_label_create(button);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, width, height);
    lv_label_set_text(label, text);
    camera_style_button(button);
    if (callback != NULL)
    {
        lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, NULL);
    }
    return button;
}

static lv_obj_t *camera_make_separator(lv_obj_t *parent, int32_t x, int32_t y,
                                       int32_t width, int32_t height)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_set_pos(line, x, y);
    lv_obj_set_size(line, width, height);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x303030), 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    return line;
}

static void camera_create_shutter(lv_obj_t *screen, lv_event_cb_t callback,
                                  lv_obj_t **outer, lv_obj_t **inner)
{
    *outer = lv_obj_create(screen);
    lv_obj_set_pos(*outer, 810, 220);
    lv_obj_set_size(*outer, 104, 104);
    lv_obj_set_style_bg_opa(*outer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(*outer, 4, 0);
    lv_obj_set_style_border_color(*outer, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(*outer, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(*outer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(*outer, callback, LV_EVENT_CLICKED, NULL);

    *inner = lv_obj_create(*outer);
    lv_obj_set_size(*inner, 76, 76);
    lv_obj_center(*inner);
    lv_obj_set_style_bg_opa(*inner, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(*inner, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_color(*inner, lv_color_hex(0xD0D0D0),
                              LV_STATE_PRESSED);
    lv_obj_set_style_border_width(*inner, 0, 0);
    lv_obj_set_style_radius(*inner, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(*inner, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(*inner, LV_OBJ_FLAG_CLICKABLE);
}

#endif
