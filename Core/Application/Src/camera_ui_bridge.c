#include "camera_ui_bridge.h"

#include <string.h>

#include "camera_preview_service.h"
#include "camera_resource_service.h"
#include "music_player_service.h"
#include "ov5640_service.h"
#include "camera_ui_style.h"
#include "system_settings_service.h"

static lv_ui *g_camera_ui;
static lv_obj_t *g_camera_settings;
static lv_obj_t *g_camera_flash_button;
static lv_obj_t *g_camera_status_label;
static lv_obj_t *g_camera_frame_label;
static lv_obj_t *g_camera_preview_message;
static lv_obj_t *g_camera_shutter_outer;
static lv_obj_t *g_camera_shutter_inner;
static lv_image_dsc_t g_camera_image;
static lv_timer_t *g_camera_flash_timer;
static camera_flash_mode_t g_camera_flash_mode = CAMERA_FLASH_MODE_OFF;
static uint8_t g_camera_mirror;
static uint8_t g_camera_flip;
static uint32_t g_camera_generation;
static uint8_t g_camera_leave_requested;

static void camera_button_set_text(lv_obj_t *button, const char *text)
{
    lv_obj_t *label = (button != NULL) ? lv_obj_get_child(button, 0) : NULL;
    if (label != NULL)
    {
        lv_label_set_text(label, text);
    }
}

static void camera_ui_set_status(const char *text)
{
    if (g_camera_status_label != NULL)
    {
        lv_label_set_text(g_camera_status_label, text);
    }
}

static void camera_flash_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    (void)ov5640_set_flash(0U);
    if (g_camera_flash_timer != NULL)
    {
        lv_timer_del(g_camera_flash_timer);
        g_camera_flash_timer = NULL;
    }
}

static void camera_flash_apply(void)
{
    (void)ov5640_set_flash(
        (uint8_t)(g_camera_flash_mode == CAMERA_FLASH_MODE_ON));
}

static void camera_flash_update_text(void)
{
    if (g_camera_flash_mode == CAMERA_FLASH_MODE_OFF)
        camera_button_set_text(g_camera_flash_button,
            "\xE9\x97\xAA\xE5\x85\x89\xE7\x81\xAF\xEF\xBC\x9A\xE5\x85\xB3\xE9\x97\xAD");
    else if (g_camera_flash_mode == CAMERA_FLASH_MODE_ON)
        camera_button_set_text(g_camera_flash_button,
            "\xE9\x97\xAA\xE5\x85\x89\xE7\x81\xAF\xEF\xBC\x9A\xE5\xB8\xB8\xE4\xBA\xAE");
    else
        camera_button_set_text(g_camera_flash_button,
            "\xE9\x97\xAA\xE5\x85\x89\xE7\x81\xAF\xEF\xBC\x9A\xE8\x87\xAA\xE5\x8A\xA8");
}

static void camera_store_preferences(void)
{
    system_settings_update_camera(g_camera_mirror, g_camera_flip,
                                  (uint8_t)g_camera_flash_mode,
                                  lv_tick_get());
}

static void camera_apply_preferences(void)
{
    (void)ov5640_set_mirror(g_camera_mirror);
    (void)ov5640_set_flip(g_camera_flip);
    camera_flash_apply();
}

static void camera_flash_button_event(lv_event_t *event)
{
    (void)event;
    g_camera_flash_mode = (camera_flash_mode_t)
        (((uint32_t)g_camera_flash_mode + 1U) % 3U);
    camera_flash_apply();
    camera_flash_update_text();
    camera_store_preferences();
}

static void camera_shutter_event(lv_event_t *event)
{
    (void)event;
    if (g_camera_flash_mode == CAMERA_FLASH_MODE_AUTO)
    {
        (void)ov5640_set_flash(1U);
        if (g_camera_flash_timer != NULL) lv_timer_del(g_camera_flash_timer);
        g_camera_flash_timer = lv_timer_create(camera_flash_timer_cb, 80U, NULL);
    }
    camera_ui_set_status(
        "\xE7\x8A\xB6\xE6\x80\x81\xEF\xBC\x9A\xE5\xBF\xAB\xE9\x97\xA8\xE5\x8A\x9F\xE8\x83\xBD\xE5\xB7\xB2\xE9\xA2\x84\xE7\x95\x99");
}

static void camera_mirror_event(lv_event_t *event)
{
    (void)event;
    g_camera_mirror = (uint8_t)(g_camera_mirror == 0U);
    (void)ov5640_set_mirror(g_camera_mirror);
    camera_store_preferences();
    camera_ui_set_status(g_camera_mirror != 0U
        ? "\xE7\x8A\xB6\xE6\x80\x81\xEF\xBC\x9A\xE6\xB0\xB4\xE5\xB9\xB3\xE9\x95\x9C\xE5\x83\x8F\xE5\xBC\x80"
        : "\xE7\x8A\xB6\xE6\x80\x81\xEF\xBC\x9A\xE6\xB0\xB4\xE5\xB9\xB3\xE9\x95\x9C\xE5\x83\x8F\xE5\x85\xB3");
}

static void camera_flip_event(lv_event_t *event)
{
    (void)event;
    g_camera_flip = (uint8_t)(g_camera_flip == 0U);
    (void)ov5640_set_flip(g_camera_flip);
    camera_store_preferences();
    camera_ui_set_status(g_camera_flip != 0U
        ? "\xE7\x8A\xB6\xE6\x80\x81\xEF\xBC\x9A\xE5\x9E\x82\xE7\x9B\xB4\xE7\xBF\xBB\xE8\xBD\xAC\xE5\xBC\x80"
        : "\xE7\x8A\xB6\xE6\x80\x81\xEF\xBC\x9A\xE5\x9E\x82\xE7\x9B\xB4\xE7\xBF\xBB\xE8\xBD\xAC\xE5\x85\xB3");
}

static void camera_pattern_event(lv_event_t *event)
{
    ov5640_status_t status;
    (void)event;
    ov5640_get_status(&status);
    (void)ov5640_set_test_pattern((uint8_t)(status.test_pattern == 0U));
    camera_ui_set_status(status.test_pattern == 0U
        ? "\xE7\x8A\xB6\xE6\x80\x81\xEF\xBC\x9A\xE6\xB5\x8B\xE8\xAF\x95\xE5\xBD\xA9\xE6\x9D\xA1\xE5\xBC\x80"
        : "\xE7\x8A\xB6\xE6\x80\x81\xEF\xBC\x9A\xE6\xB5\x8B\xE8\xAF\x95\xE5\xBD\xA9\xE6\x9D\xA1\xE5\x85\xB3");
}

static void camera_settings_event(lv_event_t *event)
{
    (void)event;
    if (g_camera_settings != NULL)
        lv_obj_clear_flag(g_camera_settings, LV_OBJ_FLAG_HIDDEN);
}

static void camera_settings_close_event(lv_event_t *event)
{
    (void)event;
    if (g_camera_settings != NULL)
        lv_obj_add_flag(g_camera_settings, LV_OBJ_FLAG_HIDDEN);
}

static void camera_create_settings(lv_obj_t *screen)
{
    g_camera_settings = lv_obj_create(screen);
    lv_obj_set_pos(g_camera_settings, 720, 76);
    lv_obj_set_size(g_camera_settings, 280, 430);
    lv_obj_set_style_bg_opa(g_camera_settings, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(g_camera_settings, lv_color_hex(0x121212), 0);
    lv_obj_set_style_border_width(g_camera_settings, 1, 0);
    lv_obj_set_style_border_color(g_camera_settings, lv_color_hex(0x3A3A3A), 0);
    lv_obj_set_style_radius(g_camera_settings, 6, 0);
    lv_obj_clear_flag(g_camera_settings, LV_OBJ_FLAG_SCROLLABLE);
    camera_make_button(g_camera_settings, 16, 16, 248, 44,
        "\xE5\x85\xB3\xE9\x97\xAD\xE8\xAE\xBE\xE7\xBD\xAE",
        camera_settings_close_event);
    camera_make_button(g_camera_settings, 16, 80, 248, 44,
        "\xE6\xB0\xB4\xE5\xB9\xB3\xE9\x95\x9C\xE5\x83\x8F",
        camera_mirror_event);
    camera_make_button(g_camera_settings, 16, 136, 248, 44,
        "\xE5\x9E\x82\xE7\x9B\xB4\xE7\xBF\xBB\xE8\xBD\xAC",
        camera_flip_event);
    g_camera_flash_button = camera_make_button(g_camera_settings, 16, 192,
        248, 44,
        "\xE9\x97\xAA\xE5\x85\x89\xE7\x81\xAF\xEF\xBC\x9A\xE5\x85\xB3\xE9\x97\xAD",
        camera_flash_button_event);
    camera_make_button(g_camera_settings, 16, 248, 248, 44,
        "\xE6\xB5\x8B\xE8\xAF\x95\xE5\xBD\xA9\xE6\x9D\xA1",
        camera_pattern_event);
    lv_obj_add_flag(g_camera_settings, LV_OBJ_FLAG_HIDDEN);
}

void camera_ui_bridge_attach(lv_ui *ui)
{
    system_settings_t settings;
    lv_obj_t *screen;
    lv_obj_t *label;
    if ((ui == NULL) || (ui->screen_camera == NULL)) return;

    g_camera_ui = ui;
    g_camera_generation = 0U;
    g_camera_leave_requested = 0U;
    memset(&settings, 0, sizeof(settings));
    system_settings_get(&settings);
    g_camera_mirror = settings.camera_mirror;
    g_camera_flip = settings.camera_flip;
    g_camera_flash_mode = (camera_flash_mode_t)settings.camera_flash_mode;
    screen = ui->screen_camera;
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

    lv_obj_set_pos(ui->screen_camera_btn_1, 24, 16);
    lv_obj_set_size(ui->screen_camera_btn_1, 124, 44);
    camera_style_button(ui->screen_camera_btn_1);

    label = lv_label_create(screen);
    lv_obj_set_pos(label, 180, 26);
    lv_label_set_text(label, "\xE7\x9B\xB8\xE6\x9C\xBA");
    camera_style_label(label, 0xF2F2F2);
    label = lv_label_create(screen);
    lv_obj_set_pos(label, 548, 26);
    lv_label_set_text(label, "OV5640   600 x 400");
    camera_style_label(label, 0x9A9A9A);
    (void)camera_make_separator(screen, 0, 72, 1024, 1);

    lv_obj_set_pos(ui->screen_camera_img_1, 72, 88);
    lv_obj_set_size(ui->screen_camera_img_1, 600, 400);
    lv_obj_set_style_bg_opa(ui->screen_camera_img_1, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(ui->screen_camera_img_1,
                              lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(ui->screen_camera_img_1, 1, 0);
    lv_obj_set_style_border_color(ui->screen_camera_img_1,
                                  lv_color_hex(0x3A3A3A), 0);
    g_camera_preview_message = lv_label_create(screen);
    lv_obj_set_pos(g_camera_preview_message, 172, 270);
    lv_obj_set_size(g_camera_preview_message, 400, 80);
    lv_label_set_text(g_camera_preview_message,
        "\xE6\xAD\xA3\xE5\x9C\xA8\xE5\x90\xAF\xE5\x8A\xA8");
    lv_obj_set_style_text_align(g_camera_preview_message, LV_TEXT_ALIGN_CENTER, 0);
    camera_style_label(g_camera_preview_message, 0x8A8A8A);

    (void)camera_make_separator(screen, 704, 88, 1, 400);
    (void)camera_make_button(screen, 780, 112, 160, 48,
        "\xE8\xAE\xBE\xE7\xBD\xAE", camera_settings_event);
    camera_create_shutter(screen, camera_shutter_event,
                          &g_camera_shutter_outer,
                          &g_camera_shutter_inner);
    (void)camera_make_button(screen, 780, 420, 160, 48,
        "\xE7\x9B\xB8\xE5\x86\x8C", NULL);

    (void)camera_make_separator(screen, 0, 518, 1024, 1);
    g_camera_status_label = lv_label_create(screen);
    lv_obj_set_pos(g_camera_status_label, 72, 542);
    lv_label_set_text(g_camera_status_label,
        "\xE7\x8A\xB6\xE6\x80\x81\xEF\xBC\x9A\xE6\xAD\xA3\xE5\x9C\xA8\xE5\x90\xAF\xE5\x8A\xA8");
    camera_style_label(g_camera_status_label, 0xB0B0B0);
    g_camera_frame_label = lv_label_create(screen);
    lv_obj_set_pos(g_camera_frame_label, 790, 542);
    lv_label_set_text(g_camera_frame_label,
        "\xE5\xB8\xA7\x20\x30\x30\x30\x30");
    camera_style_label(g_camera_frame_label, 0xB0B0B0);
    camera_create_settings(screen);
    camera_flash_update_text();
    if (music_player_suspend_for_camera() != MUSIC_PLAYER_OK)
    {
        camera_ui_set_status(
            "\xE7\x8A\xB6\xE6\x80\x81\xEF\xBC\x9A\xE9\x9F\xB3\xE4\xB9\x90\xE6\x8C\x82\xE8\xB5\xB7\xE5\xA4\xB1\xE8\xB4\xA5");
        return;
    }
    if (camera_preview_start() == CAMERA_PREVIEW_OK)
    {
        camera_apply_preferences();
    }
}

void camera_ui_bridge_poll(void)
{
    camera_preview_frame_t frame;
    camera_preview_status_t status;
    camera_resource_status_t resource;
    if (g_camera_ui == NULL) return;

    camera_preview_get_status(&status);
    if (status.state == CAMERA_PREVIEW_ERROR)
    {
        camera_resource_get_status(&resource);
        lv_obj_clear_flag(g_camera_preview_message, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(g_camera_preview_message,
            "\xE7\x9B\xB8\xE6\x9C\xBA\xE5\x90\xAF\xE5\x8A\xA8\xE5\xA4\xB1\xE8\xB4\xA5\n"
            "stage=%d result=%d", resource.stage, status.last_result);
        camera_ui_set_status(
            "\xE7\x8A\xB6\xE6\x80\x81\xEF\xBC\x9A\xE5\x90\xAF\xE5\x8A\xA8\xE5\xA4\xB1\xE8\xB4\xA5");
        return;
    }
    if ((camera_preview_get_frame(&frame) != CAMERA_PREVIEW_OK) ||
        (frame.generation == g_camera_generation)) return;

    g_camera_generation = frame.generation;
    g_camera_image.header.magic = LV_IMAGE_HEADER_MAGIC;
    g_camera_image.header.cf = LV_COLOR_FORMAT_RGB565;
    g_camera_image.header.w = frame.width;
    g_camera_image.header.h = frame.height;
    g_camera_image.header.stride = (uint32_t)frame.width * 2U;
    g_camera_image.data_size = frame.bytes;
    g_camera_image.data = (const uint8_t *)frame.rgb_address;
    lv_image_set_src(g_camera_ui->screen_camera_img_1, &g_camera_image);
    lv_obj_add_flag(g_camera_preview_message, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text_fmt(g_camera_frame_label,
        "\xE5\xB8\xA7\x20\x25\x30\x34\x6C\x75",
        (unsigned long)frame.generation);
    camera_ui_set_status(
        "\xE7\x8A\xB6\xE6\x80\x81\xEF\xBC\x9A\xE5\xAE\x9E\xE6\x97\xB6\xE9\xA2\x84\xE8\xA7\x88");
}

void camera_ui_bridge_request_leave(void)
{
    if (g_camera_leave_requested == 0U)
    {
        g_camera_leave_requested = 1U;
        (void)camera_preview_stop();
        camera_flash_timer_cb(NULL);
        music_player_request_restore_after_camera();
        g_camera_ui = NULL;
    }
}

camera_flash_mode_t camera_ui_bridge_get_flash_mode(void)
{
    return g_camera_flash_mode;
}
