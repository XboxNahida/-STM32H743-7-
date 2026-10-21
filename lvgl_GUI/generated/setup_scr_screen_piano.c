/*
 * STAGE6 UI USER PATCH
 * Static piano interface based on the approved 1024x600 mockup.
 */
#include "lvgl.h"
#include "gui_guider.h"
#include "events_init.h"
#include "lv_port_indev.h"
#include "music_player_service.h"
#include "piano_audio_service.h"
#include "piano_touch_mapper.h"

#define STAGE6_PIANO_WHITE_KEY_COUNT 14U
#define STAGE6_PIANO_BLACK_KEY_COUNT 10U
#define STAGE6_PIANO_PAD_COUNT       15U

static const char *const g_white_note_names[STAGE6_PIANO_WHITE_KEY_COUNT] = {
    "Do", "Re", "Mi", "Fa", "Sol", "La", "Xi",
    "Do", "Re", "Mi", "Fa", "Sol", "La", "Xi"
};

static const uint8_t g_black_key_after[STAGE6_PIANO_BLACK_KEY_COUNT] = {
    0U, 1U, 3U, 4U, 5U, 7U, 8U, 10U, 11U, 12U
};

static const char *const g_pad_note_names[STAGE6_PIANO_PAD_COUNT] = {
    "Do", "Re", "Mi", "Fa", "Sol",
    "La", "Xi", "Do", "Re", "Mi",
    "Fa", "Sol", "La", "Xi", "Do"
};

static const uint32_t g_pad_accent[5] = {
    0xdf6761U, 0xe7b950U, 0x65bc8aU, 0x57a8caU, 0xb18bc2U
};

static lv_timer_t *g_piano_touch_timer;
static int8_t g_piano_transpose;
static uint8_t g_piano_touch_armed;

static void screen_piano_update_tune_label(lv_ui *ui)
{
    char text[5];

    if (g_piano_transpose == 0)
    {
        lv_label_set_text(ui->screen_piano_tune_value, "C");
    }
    else
    {
        (void)lv_snprintf(text, sizeof(text), "%+d",
                          (int)g_piano_transpose);
        lv_label_set_text(ui->screen_piano_tune_value, text);
    }
}

static void screen_piano_tune_down_event_handler(lv_event_t *event)
{
    lv_ui *ui;

    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    ui = (lv_ui *)lv_event_get_user_data(event);
    if ((ui == NULL) || (g_piano_transpose <= -12)) return;
    (void)piano_audio_apply_note_mask(0U);
    g_piano_transpose--;
    screen_piano_update_tune_label(ui);
}

static void screen_piano_tune_up_event_handler(lv_event_t *event)
{
    lv_ui *ui;

    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    ui = (lv_ui *)lv_event_get_user_data(event);
    if ((ui == NULL) || (g_piano_transpose >= 12)) return;
    (void)piano_audio_apply_note_mask(0U);
    g_piano_transpose++;
    screen_piano_update_tune_label(ui);
}

static void screen_piano_sustain_event_handler(lv_event_t *event)
{
    lv_obj_t *switch_obj;
    uint8_t enabled;

    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) return;
    switch_obj = lv_event_get_target_obj(event);
    if (switch_obj == NULL) return;
    enabled = lv_obj_has_state(switch_obj, LV_STATE_CHECKED) ? 1U : 0U;
    if (piano_audio_set_sustain(enabled) != PIANO_AUDIO_OK)
    {
        if (enabled != 0U)
        {
            lv_obj_remove_state(switch_obj, LV_STATE_CHECKED);
        }
        else
        {
            lv_obj_add_state(switch_obj, LV_STATE_CHECKED);
        }
    }
}

static void screen_piano_touch_timer_cb(lv_timer_t *timer)
{
    lv_ui *ui = (lv_ui *)lv_timer_get_user_data(timer);
    lv_port_touch_snapshot_t snapshot;
    piano_touch_point_t points[PIANO_TOUCH_MAX_POINTS];
    piano_touch_mode_t mode;
    uint64_t note_mask;
    uint8_t index;

    if ((ui == NULL) || (ui->screen_piano == NULL)) return;
    lv_port_indev_get_touch_snapshot(&snapshot);
    if (g_piano_touch_armed == 0U)
    {
        if (snapshot.count == 0U) g_piano_touch_armed = 1U;
        (void)piano_audio_apply_note_mask(0U);
        return;
    }
    for (index = 0U; index < snapshot.count; index++)
    {
        points[index].x = snapshot.x[index];
        points[index].y = snapshot.y[index];
    }
    mode = lv_obj_has_flag(ui->screen_piano_pads_panel,
                           LV_OBJ_FLAG_HIDDEN) ?
           PIANO_TOUCH_MODE_KEYS : PIANO_TOUCH_MODE_MATRIX;
    if (piano_touch_map(mode, points, snapshot.count,
                        g_piano_transpose, &note_mask) == PIANO_TOUCH_OK)
    {
        (void)piano_audio_apply_note_mask(note_mask);
    }
}

static void screen_piano_delete_event_handler(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_DELETE) return;
    if (g_piano_touch_timer != NULL)
    {
        lv_timer_delete(g_piano_touch_timer);
        g_piano_touch_timer = NULL;
    }
    (void)piano_audio_apply_note_mask(0U);
    (void)piano_audio_stop();
}

static lv_obj_t *stage6_label(lv_obj_t *parent, const char *text,
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

static lv_obj_t *stage6_button(lv_obj_t *parent, const char *text,
                               const lv_font_t *font)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_t *label = stage6_label(button, text, font);
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

static void screen_piano_mode_event_handler(lv_event_t *event)
{
    lv_ui *ui;
    bool pads_visible;

    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    ui = (lv_ui *)lv_event_get_user_data(event);
    if ((ui == NULL) || (ui->screen_piano_keys_panel == NULL) ||
        (ui->screen_piano_pads_panel == NULL)) return;

    pads_visible = !lv_obj_has_flag(ui->screen_piano_pads_panel,
                                    LV_OBJ_FLAG_HIDDEN);
    (void)piano_audio_apply_note_mask(0U);
    if (pads_visible)
    {
        lv_obj_add_flag(ui->screen_piano_pads_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui->screen_piano_keys_panel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui->screen_piano_mode_btn_label,
                          "\xE5\x88\x87\xE6\x8D\xA2\xE7\x9F\xA9\xE9\x98\xB5");
    }
    else
    {
        lv_obj_add_flag(ui->screen_piano_keys_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui->screen_piano_pads_panel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui->screen_piano_mode_btn_label,
                          "\xE5\x88\x87\xE6\x8D\xA2\xE7\x90\xB4\xE9\x94\xAE");
    }
}

static void stage6_create_piano_navigation(lv_ui *ui)
{
    ui->screen_piano_cont_1 = lv_obj_create(ui->screen_piano);
    lv_obj_set_pos(ui->screen_piano_cont_1, 0, 0);
    lv_obj_set_size(ui->screen_piano_cont_1, 1024, 35);
    lv_obj_set_scrollbar_mode(ui->screen_piano_cont_1,
                              LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(ui->screen_piano_cont_1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui->screen_piano_cont_1, 0,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_piano_cont_1, 0,
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_piano_cont_1,
                              lv_color_hex(0x111416),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_piano_cont_1, 255,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->screen_piano_cont_1, 0,
                             LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->screen_piano_digital_clock_1 = stage6_label(
        ui->screen_piano_cont_1, "15:08", &lv_font_montserratMedium_20);
    lv_obj_set_pos(ui->screen_piano_digital_clock_1, 0, 5);
    lv_obj_set_size(ui->screen_piano_digital_clock_1, 100, 30);

    ui->screen_piano_title = stage6_label(
        ui->screen_piano_cont_1, "\xE9\x92\xA2\xE7\x90\xB4",
        &lv_font_myyh_20);
    lv_obj_set_pos(ui->screen_piano_title, 462, 5);
    lv_obj_set_size(ui->screen_piano_title, 100, 30);

    ui->screen_piano_label_2 = stage6_label(
        ui->screen_piano_cont_1,
        "   " LV_SYMBOL_WIFI "  " LV_SYMBOL_VOLUME_MAX " "
        LV_SYMBOL_USB " " LV_SYMBOL_BATTERY_FULL "  100%",
        &lv_font_montserratMedium_20);
    lv_obj_set_pos(ui->screen_piano_label_2, 800, 5);
    lv_obj_set_size(ui->screen_piano_label_2, 220, 30);

    ui->screen_piano_label_1 = stage6_label(
        ui->screen_piano_cont_1, "", &lv_font_myyh_20);
    lv_obj_set_pos(ui->screen_piano_label_1, 462, -141);
    lv_obj_set_size(ui->screen_piano_label_1, 100, 32);
    lv_label_set_long_mode(ui->screen_piano_label_1, LV_LABEL_LONG_WRAP);
}

static void stage6_create_white_keys(lv_ui *ui)
{
    uint32_t index;

    for (index = 0U; index < STAGE6_PIANO_WHITE_KEY_COUNT; index++)
    {
        lv_obj_t *key = lv_button_create(ui->screen_piano_keys_panel);
        lv_obj_t *label;
        lv_obj_set_pos(key, 14 + (int32_t)index * 54, 32);
        lv_obj_set_size(key, 52, 482);
        lv_obj_set_style_radius(key, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(key, lv_color_hex(0xf4f3ef),
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(key, 1,
                                      LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(key, lv_color_hex(0xaeb3b5),
                                      LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(key, 0,
                                      LV_PART_MAIN|LV_STATE_DEFAULT);
        label = stage6_label(key, g_white_note_names[index],
                             &lv_font_montserratMedium_18);
        lv_obj_set_style_text_color(label, lv_color_hex(0x303437),
                                    LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -20);
    }
}

static void stage6_create_black_keys(lv_ui *ui)
{
    uint32_t index;

    for (index = 0U; index < STAGE6_PIANO_BLACK_KEY_COUNT; index++)
    {
        lv_obj_t *key = lv_button_create(ui->screen_piano_keys_panel);
        int32_t x = 14 + ((int32_t)g_black_key_after[index] + 1) * 54 - 16;
        lv_obj_set_pos(key, x, 32);
        lv_obj_set_size(key, 32, 280);
        lv_obj_set_style_radius(key, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(key, lv_color_hex(0x23272a),
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(key, 1,
                                      LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(key, lv_color_hex(0x050606),
                                      LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(key, 4,
                                      LV_PART_MAIN|LV_STATE_DEFAULT);
    }
}

static void stage6_create_pads(lv_ui *ui)
{
    uint32_t index;

    for (index = 0U; index < STAGE6_PIANO_PAD_COUNT; index++)
    {
        uint32_t row = index / 5U;
        uint32_t column = index % 5U;
        lv_obj_t *pad = stage6_button(ui->screen_piano_pads_panel,
            g_pad_note_names[index], &lv_font_montserratMedium_20);
        lv_obj_set_pos(pad, 18 + (int32_t)column * 150,
                       18 + (int32_t)row * 176);
        lv_obj_set_size(pad, 135, 158);
        lv_obj_set_style_border_width(pad, 4,
                                      LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(pad, LV_BORDER_SIDE_TOP,
                                     LV_PART_MAIN|LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(pad, lv_color_hex(g_pad_accent[column]),
                                      LV_PART_MAIN|LV_STATE_DEFAULT);
    }
}

static void stage6_create_control_panel(lv_ui *ui)
{
    lv_obj_t *caption;
    lv_obj_t *sustain_switch;

    ui->screen_piano_control_panel = lv_obj_create(ui->screen_piano);
    lv_obj_set_pos(ui->screen_piano_control_panel, 786, 35);
    lv_obj_set_size(ui->screen_piano_control_panel, 238, 565);
    lv_obj_set_scrollbar_mode(ui->screen_piano_control_panel,
                              LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(ui->screen_piano_control_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui->screen_piano_control_panel, 0,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_piano_control_panel, 0,
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_piano_control_panel,
                              lv_color_hex(0x101315),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_piano_control_panel, 255,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    /* STAGE6 UI USER PATCH: fixed coordinates require no parent padding. */
    lv_obj_set_style_pad_all(ui->screen_piano_control_panel, 0,
                             LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->screen_piano_btn_back = stage6_button(
        ui->screen_piano_control_panel,
        LV_SYMBOL_LEFT " \xE8\xBF\x94\xE5\x9B\x9E",
        &lv_font_myyh_20);
    lv_obj_set_pos(ui->screen_piano_btn_back, 16, 20);
    lv_obj_set_size(ui->screen_piano_btn_back, 206, 60);

    caption = stage6_label(ui->screen_piano_control_panel,
                           "\xE9\x9F\xB3\xE8\x89\xB2\xEF\xBC\x9A"
                           "\xE5\x8E\x9F\xE5\xA3\xB0\xE9\x92\xA2\xE7\x90\xB4",
                           &lv_font_myyh_20);
    lv_obj_set_pos(caption, 12, 102);
    lv_obj_set_size(caption, 214, 30);
    lv_obj_set_style_text_color(caption, lv_color_hex(0x9fa8ad),
                                LV_PART_MAIN|LV_STATE_DEFAULT);

    ui->screen_piano_tune_down_btn = stage6_button(
        ui->screen_piano_control_panel, "-", &lv_font_montserratMedium_30);
    lv_obj_set_pos(ui->screen_piano_tune_down_btn, 16, 165);
    lv_obj_set_size(ui->screen_piano_tune_down_btn, 56, 76);
    lv_obj_add_event_cb(ui->screen_piano_tune_down_btn,
                        screen_piano_tune_down_event_handler,
                        LV_EVENT_ALL, ui);

    ui->screen_piano_tune_value = stage6_label(
        ui->screen_piano_control_panel, "C", &lv_font_montserratMedium_30);
    lv_obj_set_pos(ui->screen_piano_tune_value, 84, 166);
    lv_obj_set_size(ui->screen_piano_tune_value, 70, 38);
    lv_obj_set_style_text_color(ui->screen_piano_tune_value,
                                lv_color_hex(0xe8bd4b),
                                LV_PART_MAIN|LV_STATE_DEFAULT);

    caption = stage6_label(ui->screen_piano_control_panel,
                           "\xE6\xA0\x87\xE5\x87\x86\xE8\xB0\x83",
                           &lv_font_myyh_20);
    lv_obj_set_pos(caption, 76, 205);
    lv_obj_set_size(caption, 86, 30);

    ui->screen_piano_tune_up_btn = stage6_button(
        ui->screen_piano_control_panel, "+", &lv_font_montserratMedium_30);
    lv_obj_set_pos(ui->screen_piano_tune_up_btn, 166, 165);
    lv_obj_set_size(ui->screen_piano_tune_up_btn, 56, 76);
    lv_obj_add_event_cb(ui->screen_piano_tune_up_btn,
                        screen_piano_tune_up_event_handler,
                        LV_EVENT_ALL, ui);

    caption = stage6_label(ui->screen_piano_control_panel,
                           "\xE7\xA7\xBB\xE8\xB0\x83\xEF\xBC\x9A" "0",
                           &lv_font_myyh_20);
    lv_obj_set_pos(caption, 54, 260);
    lv_obj_set_size(caption, 130, 30);

    caption = stage6_label(ui->screen_piano_control_panel,
                           "\xE5\xBB\xB6\xE9\x9F\xB3",
                           &lv_font_myyh_20);
    lv_obj_set_pos(caption, 26, 326);
    lv_obj_set_size(caption, 80, 36);

    sustain_switch = lv_switch_create(ui->screen_piano_control_panel);
    lv_obj_set_pos(sustain_switch, 138, 320);
    lv_obj_set_size(sustain_switch, 72, 40);
    lv_obj_set_style_bg_color(sustain_switch, lv_color_hex(0x3b4246),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(sustain_switch, lv_color_hex(0x65bc8a),
                              LV_PART_INDICATOR|LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sustain_switch, lv_color_hex(0xf4f6f7),
                              LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_add_event_cb(sustain_switch,
                        screen_piano_sustain_event_handler,
                        LV_EVENT_VALUE_CHANGED, NULL);

    ui->screen_piano_mode_btn = stage6_button(
        ui->screen_piano_control_panel,
        "\xE5\x88\x87\xE6\x8D\xA2\xE7\x9F\xA9\xE9\x98\xB5",
        &lv_font_myyh_20);
    ui->screen_piano_mode_btn_label = lv_obj_get_child(
        ui->screen_piano_mode_btn, 0);
    lv_obj_set_pos(ui->screen_piano_mode_btn, 16, 426);
    lv_obj_set_size(ui->screen_piano_mode_btn, 206, 70);
    lv_obj_set_style_bg_color(ui->screen_piano_mode_btn,
                              lv_color_hex(0xe8bd4b),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_piano_mode_btn_label,
                                lv_color_hex(0x171a1c),
                                LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui->screen_piano_mode_btn,
                        screen_piano_mode_event_handler,
                        LV_EVENT_CLICKED, ui);

    caption = stage6_label(ui->screen_piano_control_panel,
                           "\xE6\x94\xAF\xE6\x8C\x81\xE6\x9C\x80\xE5\xA4\x9A"
                           "\xE4\xBA\x94\xE9\x94\xAE\xE5\x90\x8C\xE6\x8C\x89",
                           &lv_font_myyh_20);
    lv_obj_set_pos(caption, 12, 514);
    lv_obj_set_size(caption, 214, 30);
    lv_obj_set_style_text_color(caption, lv_color_hex(0x778187),
                                LV_PART_MAIN|LV_STATE_DEFAULT);
}

void setup_scr_screen_piano(lv_ui *ui)
{
    ui->screen_piano = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_piano, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->screen_piano, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(ui->screen_piano, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui->screen_piano, lv_color_hex(0x171a1c),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_piano, 255,
                            LV_PART_MAIN|LV_STATE_DEFAULT);

    stage6_create_piano_navigation(ui);

    ui->screen_piano_keys_panel = lv_obj_create(ui->screen_piano);
    lv_obj_set_pos(ui->screen_piano_keys_panel, 0, 35);
    lv_obj_set_size(ui->screen_piano_keys_panel, 786, 565);
    lv_obj_set_scrollbar_mode(ui->screen_piano_keys_panel,
                              LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(ui->screen_piano_keys_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui->screen_piano_keys_panel, 0,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_piano_keys_panel, 0,
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_piano_keys_panel,
                              lv_color_hex(0x171a1c),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->screen_piano_keys_panel, 0,
                             LV_PART_MAIN|LV_STATE_DEFAULT);
    stage6_create_white_keys(ui);
    stage6_create_black_keys(ui);

    ui->screen_piano_pads_panel = lv_obj_create(ui->screen_piano);
    lv_obj_set_pos(ui->screen_piano_pads_panel, 0, 35);
    lv_obj_set_size(ui->screen_piano_pads_panel, 786, 565);
    lv_obj_set_scrollbar_mode(ui->screen_piano_pads_panel,
                              LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(ui->screen_piano_pads_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui->screen_piano_pads_panel, 0,
                            LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_piano_pads_panel, 0,
                                  LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_piano_pads_panel,
                              lv_color_hex(0x171a1c),
                              LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui->screen_piano_pads_panel, 0,
                             LV_PART_MAIN|LV_STATE_DEFAULT);
    stage6_create_pads(ui);
    lv_obj_add_flag(ui->screen_piano_pads_panel, LV_OBJ_FLAG_HIDDEN);

    stage6_create_control_panel(ui);
    lv_obj_update_layout(ui->screen_piano);
    events_init_screen_piano(ui);
    g_piano_transpose = 0;
    g_piano_touch_armed = 0U;
    (void)music_player_stop();
    piano_audio_init();
    (void)piano_audio_start();
    g_piano_touch_timer = lv_timer_create(screen_piano_touch_timer_cb,
                                           5U, ui);
    lv_obj_add_event_cb(ui->screen_piano,
                        screen_piano_delete_event_handler,
                        LV_EVENT_DELETE, ui);
}
