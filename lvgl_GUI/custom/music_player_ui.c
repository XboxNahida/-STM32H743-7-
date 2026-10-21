#include "music_player_ui.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "gui_guider.h"
#include "music_player_service.h"
#include "music_player_visual.h"

#define MUSIC_UI_CONTENT_Y 35
#define MUSIC_UI_CONTENT_HEIGHT 565
#define MUSIC_UI_PAGE_HEIGHT 565
#define MUSIC_UI_INTRO_TIME_MS 2000U
#define MUSIC_UI_SPECTRUM_HEIGHT 250
#define MUSIC_UI_ACTIVE_SYNC_INTERVAL_MS 33U
#define MUSIC_UI_IDLE_SYNC_INTERVAL_MS 100U
#define MUSIC_UI_VISIBLE_TRACK_ROWS 9U
#define MUSIC_UI_TRACK_ROW_HEIGHT 64

LV_IMAGE_DECLARE(img_lv_demo_music_icon_1);
LV_IMAGE_DECLARE(img_lv_demo_music_icon_2);
LV_IMAGE_DECLARE(img_lv_demo_music_icon_3);
LV_IMAGE_DECLARE(img_lv_demo_music_icon_4);
LV_IMAGE_DECLARE(img_lv_demo_music_btn_rnd);
LV_IMAGE_DECLARE(img_lv_demo_music_btn_loop);
LV_IMAGE_DECLARE(img_lv_demo_music_btn_prev);
LV_IMAGE_DECLARE(img_lv_demo_music_btn_next);
LV_IMAGE_DECLARE(img_lv_demo_music_btn_play);
LV_IMAGE_DECLARE(img_lv_demo_music_btn_pause);
LV_IMAGE_DECLARE(img_lv_demo_music_slider_knob);
LV_IMAGE_DECLARE(img_lv_demo_music_cover_1);
LV_IMAGE_DECLARE(img_lv_demo_music_btn_list_play);
LV_IMAGE_DECLARE(img_lv_demo_music_btn_list_pause);

static lv_obj_t *g_screen_root;
static lv_obj_t *g_app;
static lv_obj_t *g_scroll;
static lv_obj_t *g_title_label;
static lv_obj_t *g_state_label;
static lv_obj_t *g_elapsed_label;
static lv_obj_t *g_play_image;
static lv_obj_t *g_progress_slider;
static lv_obj_t *g_track_count_label;
static lv_obj_t *g_track_list;
static lv_obj_t *g_track_buttons[MUSIC_UI_VISIBLE_TRACK_ROWS];
static lv_obj_t *g_track_icons[MUSIC_UI_VISIBLE_TRACK_ROWS];
static lv_obj_t *g_track_labels[MUSIC_UI_VISIBLE_TRACK_ROWS];
static uint16_t g_track_row_indices[MUSIC_UI_VISIBLE_TRACK_ROWS];
static lv_obj_t *g_navigation_root;
static music_visual_t g_visual;
static uint8_t g_navigation_return_to_music;
static uint8_t g_list_created;
static uint8_t g_navigation_needs_sync;
static uint8_t g_open_requested;
static uint8_t g_last_volume_percent;
static uint16_t g_last_selected_index;
static uint16_t g_last_track_count;
static uint32_t g_last_playback_state;
static uint32_t g_last_elapsed_second;
static uint32_t g_last_duration_second;
static uint32_t g_next_sync_ms;

static int music_ui_time_reached(uint32_t now_ms, uint32_t deadline_ms)
{
    return ((int32_t)(now_ms - deadline_ms) >= 0) ? 1 : 0;
}

static int music_ui_object_valid(lv_obj_t *object)
{
    return ((object != NULL) && lv_obj_is_valid(object)) ? 1 : 0;
}

static void music_ui_style_page(lv_obj_t *object, lv_color_t color)
{
    lv_obj_set_style_bg_color(object, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(object, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(object, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(object, 0, LV_PART_MAIN);
}

static void music_ui_reset_screen_objects(void)
{
    uint32_t index;

    g_screen_root = NULL;
    g_app = NULL;
    g_scroll = NULL;
    g_title_label = NULL;
    g_state_label = NULL;
    g_elapsed_label = NULL;
    g_play_image = NULL;
    g_progress_slider = NULL;
    g_track_count_label = NULL;
    g_track_list = NULL;
    g_list_created = 0U;
    g_last_selected_index = UINT16_MAX;
    g_last_track_count = UINT16_MAX;
    g_last_playback_state = UINT32_MAX;
    g_last_elapsed_second = UINT32_MAX;
    g_last_duration_second = UINT32_MAX;
    g_last_volume_percent = UINT8_MAX;
    music_visual_init(&g_visual);
    for (index = 0U; index < MUSIC_UI_VISIBLE_TRACK_ROWS; index++)
    {
        g_track_buttons[index] = NULL;
        g_track_icons[index] = NULL;
        g_track_labels[index] = NULL;
        g_track_row_indices[index] = UINT16_MAX;
    }
}

static void music_ui_screen_deleted(lv_event_t *event)
{
    (void)event;
    music_ui_reset_screen_objects();
}

static void music_ui_navigation_deleted(lv_event_t *event)
{
    (void)event;
    g_navigation_root = NULL;
    g_navigation_needs_sync = 0U;
}

static int music_ui_ensure_library(void)
{
    music_player_status_t status;

    music_player_get_status(&status);
    if ((status.scanned == 0U) || (status.track_count == 0U))
    {
        return music_player_scan();
    }
    return (status.track_count > 0U) ? MUSIC_PLAYER_OK : MUSIC_PLAYER_EMPTY;
}

static void music_ui_track_clicked(lv_event_t *event)
{
    uint32_t row = (uint32_t)(uintptr_t)lv_event_get_user_data(event);

    if ((lv_event_get_code(event) == LV_EVENT_CLICKED) &&
        (row < MUSIC_UI_VISIBLE_TRACK_ROWS) &&
        (g_track_row_indices[row] != UINT16_MAX))
    {
        (void)music_player_play_index(g_track_row_indices[row]);
    }
}

static void music_ui_previous_clicked(lv_event_t *event)
{
    if ((lv_event_get_code(event) == LV_EVENT_CLICKED) &&
        (music_ui_ensure_library() == MUSIC_PLAYER_OK))
    {
        (void)music_player_previous();
    }
}

static void music_ui_toggle_clicked(lv_event_t *event)
{
    if ((lv_event_get_code(event) == LV_EVENT_CLICKED) &&
        (music_ui_ensure_library() == MUSIC_PLAYER_OK))
    {
        (void)music_player_toggle();
    }
}

static void music_ui_next_clicked(lv_event_t *event)
{
    if ((lv_event_get_code(event) == LV_EVENT_CLICKED) &&
        (music_ui_ensure_library() == MUSIC_PLAYER_OK))
    {
        (void)music_player_next();
    }
}

static void music_ui_show_tracks(lv_event_t *event)
{
    if ((lv_event_get_code(event) == LV_EVENT_CLICKED) &&
        (music_ui_object_valid(g_scroll) != 0))
    {
        lv_obj_scroll_to_y(g_scroll, MUSIC_UI_PAGE_HEIGHT, LV_ANIM_ON);
    }
}

static void music_ui_show_player(lv_event_t *event)
{
    if ((lv_event_get_code(event) == LV_EVENT_CLICKED) &&
        (music_ui_object_valid(g_scroll) != 0))
    {
        lv_obj_scroll_to_y(g_scroll, 0, LV_ANIM_ON);
    }
}

static void music_ui_player_gesture(lv_event_t *event)
{
    lv_dir_t direction;

    if ((lv_event_get_code(event) != LV_EVENT_GESTURE) ||
        (music_ui_object_valid(g_scroll) == 0))
    {
        return;
    }
    direction = lv_indev_get_gesture_dir(lv_indev_active());
    if ((direction == LV_DIR_TOP) || (direction == LV_DIR_BOTTOM))
    {
        lv_obj_scroll_to_y(g_scroll, MUSIC_UI_PAGE_HEIGHT, LV_ANIM_ON);
    }
}

static void music_ui_update_visible_tracks(void)
{
    music_player_status_t status;
    int32_t scroll_y;
    uint32_t first_index;
    uint32_t row;

    if ((music_ui_object_valid(g_track_list) == 0) ||
        (g_list_created == 0U))
    {
        return;
    }
    music_player_get_status(&status);
    scroll_y = lv_obj_get_scroll_y(g_track_list);
    if (scroll_y < 0)
    {
        scroll_y = 0;
    }
    first_index = (uint32_t)scroll_y / MUSIC_UI_TRACK_ROW_HEIGHT;
    if (first_index > 0U)
    {
        first_index--;
    }
    for (row = 0U; row < MUSIC_UI_VISIBLE_TRACK_ROWS; row++)
    {
        uint32_t index = first_index + row;
        lv_obj_t *button = g_track_buttons[row];

        if ((index >= status.track_count) ||
            (music_ui_object_valid(button) == 0))
        {
            g_track_row_indices[row] = UINT16_MAX;
            if (music_ui_object_valid(button) != 0)
            {
                lv_obj_add_flag(button, LV_OBJ_FLAG_HIDDEN);
            }
            continue;
        }
        g_track_row_indices[row] = (uint16_t)index;
        lv_obj_set_y(button, (int32_t)index * MUSIC_UI_TRACK_ROW_HEIGHT);
        lv_label_set_text(g_track_labels[row],
                          music_player_get_track_name(index));
        lv_obj_remove_flag(button, LV_OBJ_FLAG_HIDDEN);
    }
}

static void music_ui_track_list_scrolled(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_SCROLL)
    {
        music_ui_update_visible_tracks();
    }
}

static void music_ui_build_track_list(void)
{
    music_player_status_t status;
    uint32_t index;
    char count_text[32];

    if ((music_ui_object_valid(g_track_list) == 0) ||
        (g_list_created != 0U))
    {
        return;
    }
    music_player_get_status(&status);
    lv_obj_clean(g_track_list);
    for (index = 0U; index < MUSIC_UI_VISIBLE_TRACK_ROWS; index++)
    {
        g_track_buttons[index] = NULL;
        g_track_icons[index] = NULL;
        g_track_labels[index] = NULL;
        g_track_row_indices[index] = UINT16_MAX;
    }
    if (status.skipped_count > 0U)
    {
        (void)snprintf(count_text, sizeof(count_text), "%u / SKIP %u",
                       (unsigned int)status.track_count,
                       (unsigned int)status.skipped_count);
        lv_obj_set_style_text_color(g_track_count_label,
                                    lv_color_hex(0xffd166), LV_PART_MAIN);
    }
    else
    {
        (void)snprintf(count_text, sizeof(count_text),
                       "%u TRACKS", (unsigned int)status.track_count);
        lv_obj_set_style_text_color(g_track_count_label,
                                    lv_color_hex(0xb1b0be), LV_PART_MAIN);
    }
    lv_label_set_text(g_track_count_label, count_text);

    if (status.track_count > 0U)
    {
        lv_obj_t *spacer = lv_obj_create(g_track_list);

        lv_obj_set_pos(spacer, 0, 0);
        lv_obj_set_size(spacer, 1,
                        (int32_t)status.track_count *
                        MUSIC_UI_TRACK_ROW_HEIGHT);
        lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(spacer, 0, LV_PART_MAIN);
        lv_obj_clear_flag(spacer, LV_OBJ_FLAG_CLICKABLE);
        for (index = 0U; index < MUSIC_UI_VISIBLE_TRACK_ROWS; index++)
        {
            lv_obj_t *button = lv_button_create(g_track_list);
            lv_obj_t *icon = lv_image_create(button);
            lv_obj_t *label = lv_label_create(button);

            lv_obj_set_pos(button, 0,
                           (int32_t)index * MUSIC_UI_TRACK_ROW_HEIGHT);
            lv_obj_set_size(button, 900, 60);
            lv_obj_set_style_bg_color(button, lv_color_hex(0x4c4965),
                                      LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(button, LV_OPA_COVER,
                                    LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
            lv_obj_set_style_radius(button, 4, LV_PART_MAIN);

            lv_image_set_src(icon, &img_lv_demo_music_btn_list_play);
            lv_obj_set_pos(icon, 0, 0);
            lv_label_set_text(label, "-");
            lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
            lv_obj_set_pos(label, 62, 13);
            lv_obj_set_size(label, 790, 34);
            lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_text_font(label, &lv_font_myyh_20, LV_PART_MAIN);
            lv_obj_add_event_cb(button, music_ui_track_clicked,
                                LV_EVENT_CLICKED,
                                (void *)(uintptr_t)index);
            g_track_buttons[index] = button;
            g_track_icons[index] = icon;
            g_track_labels[index] = label;
        }
    }
    else
    {
        lv_obj_t *label = lv_label_create(g_track_list);

        lv_label_set_text(label,
                          "\xE6\x9C\xAA\xE6\x89\xBE\xE5\x88\xB0MP3\xE9\x9F\xB3\xE4\xB9\x90");
        lv_obj_set_style_text_color(label, lv_color_hex(0xffd166),
                                    LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &lv_font_myyh_20, LV_PART_MAIN);
    }
    g_list_created = 1U;
    music_ui_update_visible_tracks();
}

static void music_ui_create_player_page(lv_obj_t *page)
{
    static const int32_t page_columns[] =
    {
        LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
    };
    static const int32_t page_rows[] =
    {
        20, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_FR(3),
        LV_GRID_CONTENT, LV_GRID_FR(3), MUSIC_UI_SPECTRUM_HEIGHT,
        LV_GRID_FR(3), LV_GRID_CONTENT, LV_GRID_FR(3),
        LV_GRID_CONTENT, LV_GRID_FR(1), 20, LV_GRID_TEMPLATE_LAST
    };
    static const int32_t control_columns[] =
    {
        LV_GRID_FR(2), LV_GRID_FR(3), LV_GRID_FR(5), LV_GRID_FR(5),
        LV_GRID_FR(5), LV_GRID_FR(3), LV_GRID_FR(2),
        LV_GRID_TEMPLATE_LAST
    };
    static const int32_t control_rows[] =
    {
        LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST
    };
    lv_obj_t *title_box;
    lv_obj_t *icon_box;
    lv_obj_t *control_box;
    lv_obj_t *handle_box;
    lv_obj_t *button;
    lv_obj_t *label;
    lv_obj_t *play;

    music_visual_create_background(page);
    lv_obj_add_event_cb(page, music_ui_player_gesture,
                        LV_EVENT_GESTURE, NULL);
    lv_obj_set_grid_dsc_array(page, page_columns, page_rows);
    lv_obj_set_style_grid_row_align(page, LV_GRID_ALIGN_SPACE_BETWEEN,
                                    LV_PART_MAIN);

    title_box = lv_obj_create(page);
    lv_obj_remove_style_all(title_box);
    lv_obj_set_width(title_box, LV_PCT(100));
    lv_obj_set_height(title_box, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(title_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(title_box, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_grid_cell(title_box, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_CENTER, 2, 1);
    g_title_label = lv_label_create(title_box);
    lv_obj_set_width(g_title_label, 700);
    lv_label_set_text(g_title_label, "\xE9\x9F\xB3\xE4\xB9\x90");
    lv_label_set_long_mode(g_title_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(g_title_label, LV_TEXT_ALIGN_CENTER,
                                LV_PART_MAIN);
    lv_obj_set_style_text_color(g_title_label, lv_color_hex(0x504d6d),
                                LV_PART_MAIN);
    lv_obj_set_style_text_font(g_title_label, &lv_font_myyh_20,
                               LV_PART_MAIN);
    g_state_label = lv_label_create(title_box);
    lv_label_set_text(g_state_label, "SD CARD MUSIC");
    lv_obj_set_style_text_color(g_state_label, lv_color_hex(0x504d6d),
                                LV_PART_MAIN);
    lv_obj_set_style_text_font(g_state_label, &lv_font_montserratMedium_12,
                               LV_PART_MAIN);
    label = lv_label_create(title_box);
    lv_label_set_text(label, "MP3 - SD CARD");
    lv_obj_set_style_text_color(label, lv_color_hex(0x8a86b8),
                                LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserratMedium_12,
                               LV_PART_MAIN);

    icon_box = lv_obj_create(page);
    lv_obj_remove_style_all(icon_box);
    lv_obj_set_width(icon_box, LV_PCT(100));
    lv_obj_set_height(icon_box, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(icon_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(icon_box, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_grid_cell(icon_box, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_CENTER, 4, 1);
    button = music_visual_create_image_button(
        icon_box, &img_lv_demo_music_icon_1, NULL);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_CLICKABLE);
    button = music_visual_create_image_button(
        icon_box, &img_lv_demo_music_icon_2, NULL);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_CLICKABLE);
    button = music_visual_create_image_button(
        icon_box, &img_lv_demo_music_icon_3, NULL);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_CLICKABLE);
    button = music_visual_create_image_button(
        icon_box, &img_lv_demo_music_icon_4, NULL);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_CLICKABLE);

    music_visual_create_spectrum(&g_visual, page,
                                 &img_lv_demo_music_cover_1);
    lv_obj_set_grid_cell(g_visual.spectrum, LV_GRID_ALIGN_CENTER, 0, 1,
                         LV_GRID_ALIGN_CENTER, 6, 1);

    control_box = lv_obj_create(page);
    lv_obj_remove_style_all(control_box);
    lv_obj_set_width(control_box, LV_PCT(100));
    lv_obj_set_height(control_box, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_bottom(control_box, 8, LV_PART_MAIN);
    lv_obj_set_grid_dsc_array(control_box, control_columns, control_rows);
    lv_obj_set_grid_cell(control_box, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_CENTER, 8, 1);
    button = music_visual_create_image_button(
        control_box, &img_lv_demo_music_btn_rnd, NULL);
    lv_obj_set_grid_cell(button, LV_GRID_ALIGN_START, 1, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_CLICKABLE);
    button = music_visual_create_image_button(
        control_box, &img_lv_demo_music_btn_prev, NULL);
    lv_obj_set_grid_cell(button, LV_GRID_ALIGN_CENTER, 2, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(button, music_ui_previous_clicked,
                        LV_EVENT_CLICKED, NULL);
    play = music_visual_create_image_button(
        control_box, &img_lv_demo_music_btn_play, &g_play_image);
    lv_obj_set_grid_cell(play, LV_GRID_ALIGN_CENTER, 3, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(play, music_ui_toggle_clicked,
                        LV_EVENT_CLICKED, NULL);
    button = music_visual_create_image_button(
        control_box, &img_lv_demo_music_btn_next, NULL);
    lv_obj_set_grid_cell(button, LV_GRID_ALIGN_CENTER, 4, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(button, music_ui_next_clicked,
                        LV_EVENT_CLICKED, NULL);
    button = music_visual_create_image_button(
        control_box, &img_lv_demo_music_btn_loop, NULL);
    lv_obj_set_grid_cell(button, LV_GRID_ALIGN_END, 5, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_CLICKABLE);

    g_progress_slider = lv_slider_create(control_box);
    lv_obj_set_height(g_progress_slider, 3);
    lv_obj_set_grid_cell(g_progress_slider, LV_GRID_ALIGN_STRETCH, 1, 4,
                         LV_GRID_ALIGN_CENTER, 1, 1);
    lv_slider_set_range(g_progress_slider, 0, 1);
    lv_obj_clear_flag(g_progress_slider, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_grad_dir(g_progress_slider, LV_GRAD_DIR_HOR,
                                 LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(g_progress_slider, lv_color_hex(0x569af8),
                              LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(g_progress_slider,
                                   lv_color_hex(0xa666f1),
                                   LV_PART_INDICATOR);
    lv_obj_set_style_bg_image_src(g_progress_slider,
                                  &img_lv_demo_music_slider_knob,
                                  LV_PART_KNOB);
    lv_obj_set_style_bg_opa(g_progress_slider, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(g_progress_slider, 20, LV_PART_KNOB);
    g_elapsed_label = lv_label_create(control_box);
    lv_label_set_text(g_elapsed_label, "0:00");
    lv_obj_set_grid_cell(g_elapsed_label, LV_GRID_ALIGN_END, 5, 1,
                         LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_style_text_color(g_elapsed_label, lv_color_hex(0x8a86b8),
                                LV_PART_MAIN);
    lv_obj_set_style_text_font(g_elapsed_label,
                               &lv_font_montserratMedium_12,
                               LV_PART_MAIN);

    handle_box = lv_obj_create(page);
    lv_obj_remove_style_all(handle_box);
    lv_obj_set_width(handle_box, LV_PCT(100));
    lv_obj_set_height(handle_box, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(handle_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(handle_box, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(handle_box, 8, LV_PART_MAIN);
    lv_obj_add_flag(handle_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_grid_cell(handle_box, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_CENTER, 10, 1);
    label = lv_label_create(handle_box);
    lv_label_set_text(label, "ALL TRACKS");
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0x8a86b8),
                                LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserratMedium_12,
                               LV_PART_MAIN);
    button = lv_obj_create(handle_box);
    lv_obj_set_size(button, 20, 2);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x8a86b8), LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(handle_box, music_ui_show_tracks,
                        LV_EVENT_CLICKED, NULL);

    lv_obj_update_layout(page);
    music_visual_start_intro(&g_visual, page, title_box, icon_box,
                             control_box, handle_box, lv_tick_get());
    (void)MUSIC_UI_INTRO_TIME_MS;
}

static void music_ui_create_list_page(lv_obj_t *page)
{
    lv_obj_t *back;
    lv_obj_t *title;

    back = lv_button_create(page);
    lv_obj_set_pos(back, 28, 22);
    lv_obj_set_size(back, 44, 44);
    lv_obj_set_style_radius(back, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x4c4965), LV_PART_MAIN);
    lv_obj_set_style_border_width(back, 0, LV_PART_MAIN);
    title = lv_label_create(back);
    lv_label_set_text(title, LV_SYMBOL_UP);
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(title);
    lv_obj_add_event_cb(back, music_ui_show_player, LV_EVENT_CLICKED, NULL);
    title = lv_label_create(page);
    lv_obj_set_pos(title, 96, 28);
    lv_label_set_text(title, "SD CARD MUSIC");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserratMedium_20,
                               LV_PART_MAIN);
    g_track_count_label = lv_label_create(page);
    lv_obj_set_pos(g_track_count_label, 820, 30);
    lv_obj_set_size(g_track_count_label, 160, 28);
    lv_obj_set_style_text_align(g_track_count_label, LV_TEXT_ALIGN_RIGHT,
                                LV_PART_MAIN);
    lv_obj_set_style_text_color(g_track_count_label,
                                lv_color_hex(0xb1b0be), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_track_count_label,
                               &lv_font_montserratMedium_16,
                               LV_PART_MAIN);

    g_track_list = lv_obj_create(page);
    lv_obj_set_pos(g_track_list, 54, 88);
    lv_obj_set_size(g_track_list, 916, 445);
    lv_obj_set_style_bg_opa(g_track_list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_track_list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_track_list, 8, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(g_track_list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(g_track_list, LV_DIR_VER);
    lv_obj_add_event_cb(g_track_list, music_ui_track_list_scrolled,
                        LV_EVENT_SCROLL, NULL);
}

static void music_ui_create_app(void)
{
    lv_obj_t *player_page;
    lv_obj_t *list_page;

    if ((music_ui_object_valid(g_screen_root) == 0) ||
        (music_ui_object_valid(g_app) != 0))
    {
        return;
    }
    g_app = lv_obj_create(g_screen_root);
    lv_obj_set_pos(g_app, 0, MUSIC_UI_CONTENT_Y);
    lv_obj_set_size(g_app, 1024, MUSIC_UI_CONTENT_HEIGHT);
    music_ui_style_page(g_app, lv_color_white());
    lv_obj_clear_flag(g_app, LV_OBJ_FLAG_SCROLLABLE);

    g_scroll = lv_obj_create(g_app);
    lv_obj_set_pos(g_scroll, 0, 0);
    lv_obj_set_size(g_scroll, 1024, MUSIC_UI_CONTENT_HEIGHT);
    music_ui_style_page(g_scroll, lv_color_white());
    lv_obj_set_scroll_dir(g_scroll, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(g_scroll, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(g_scroll, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(g_scroll, LV_OBJ_FLAG_SCROLL_ELASTIC);

    player_page = lv_obj_create(g_scroll);
    lv_obj_set_pos(player_page, 0, 0);
    lv_obj_set_size(player_page, 1024, MUSIC_UI_PAGE_HEIGHT);
    music_ui_style_page(player_page, lv_color_white());
    lv_obj_clear_flag(player_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(player_page, LV_OBJ_FLAG_SNAPPABLE);
    music_ui_create_player_page(player_page);

    list_page = lv_obj_create(g_scroll);
    lv_obj_set_pos(list_page, 0, MUSIC_UI_PAGE_HEIGHT);
    lv_obj_set_size(list_page, 1024, MUSIC_UI_PAGE_HEIGHT);
    music_ui_style_page(list_page, lv_color_hex(0x343247));
    lv_obj_clear_flag(list_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(list_page, LV_OBJ_FLAG_SNAPPABLE);
    music_ui_create_list_page(list_page);
}

void music_player_ui_attach(lv_obj_t *screen)
{
    if ((music_ui_object_valid(screen) == 0) || (g_screen_root == screen))
    {
        return;
    }
    music_ui_reset_screen_objects();
    g_screen_root = screen;
    lv_obj_add_event_cb(screen, music_ui_screen_deleted,
                        LV_EVENT_DELETE, NULL);
    music_ui_create_app();
    music_ui_build_track_list();
}

static void music_ui_nav_open_clicked(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED)
    {
        music_player_ui_request_open();
    }
}

static void music_ui_attach_navigation(void)
{
    lv_obj_t *root = guider_ui.screen_navigation_bar;

    if ((music_ui_object_valid(root) == 0) || (g_navigation_root == root))
    {
        return;
    }
    g_navigation_root = root;
    g_navigation_needs_sync = 1U;
    lv_obj_add_event_cb(root, music_ui_navigation_deleted,
                        LV_EVENT_DELETE, NULL);
    lv_obj_add_event_cb(guider_ui.screen_navigation_bar_btn_10,
                        music_ui_nav_open_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(guider_ui.screen_navigation_bar_btn_11,
                        music_ui_previous_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(guider_ui.screen_navigation_bar_btn_12,
                        music_ui_next_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(guider_ui.screen_navigation_bar_btn_14,
                        music_ui_toggle_clicked, LV_EVENT_CLICKED, NULL);
}

static void music_ui_sync_track_buttons(const music_player_status_t *status)
{
    uint32_t row;

    for (row = 0U; row < MUSIC_UI_VISIBLE_TRACK_ROWS; row++)
    {
        uint32_t index = g_track_row_indices[row];
        lv_obj_t *button = g_track_buttons[row];

        if ((index != UINT16_MAX) && (index < status->track_count) &&
            (music_ui_object_valid(button) != 0))
        {
            if (index == status->selected_index)
            {
                lv_obj_add_state(button, LV_STATE_CHECKED);
            }
            else
            {
                lv_obj_remove_state(button, LV_STATE_CHECKED);
            }
            if (music_ui_object_valid(g_track_icons[row]) != 0)
            {
                lv_image_set_src(
                    g_track_icons[row],
                    ((index == status->selected_index) &&
                     (status->playback_state == AUDIO_STATE_PLAYING)) ?
                    &img_lv_demo_music_btn_list_pause :
                    &img_lv_demo_music_btn_list_play);
            }
        }
    }
}

static const char *music_ui_state_text(audio_state_t state)
{
    switch (state)
    {
        case AUDIO_STATE_PLAYING:
            return "\xE6\xAD\xA3\xE5\x9C\xA8\xE6\x92\xAD\xE6\x94\xBE";
        case AUDIO_STATE_PAUSED:
            return "\xE5\xB7\xB2\xE6\x9A\x82\xE5\x81\x9C";
        case AUDIO_STATE_ERROR:
            return "\xE6\x92\xAD\xE6\x94\xBE\xE9\x94\x99\xE8\xAF\xAF";
        default:
            return "SD CARD MUSIC";
    }
}

static void music_ui_format_time(char *buffer, size_t size,
                                 uint32_t milliseconds)
{
    uint32_t seconds = milliseconds / 1000U;

    (void)snprintf(buffer, size, "%lu:%02lu",
                   (unsigned long)(seconds / 60U),
                   (unsigned long)(seconds % 60U));
}

static void music_ui_sync_progress(const music_player_status_t *status)
{
    char elapsed_text[16];
    uint32_t elapsed_seconds = status->elapsed_ms / 1000U;
    uint32_t duration_seconds = status->duration_ms / 1000U;

    if (duration_seconds == 0U)
    {
        duration_seconds = 1U;
    }
    if (duration_seconds > (uint32_t)INT32_MAX)
    {
        duration_seconds = (uint32_t)INT32_MAX;
    }
    if (elapsed_seconds > duration_seconds)
    {
        elapsed_seconds = duration_seconds;
    }
    lv_slider_set_range(g_progress_slider, 0, (int32_t)duration_seconds);
    lv_slider_set_value(g_progress_slider, (int32_t)elapsed_seconds,
                        LV_ANIM_ON);
    music_ui_format_time(elapsed_text, sizeof(elapsed_text),
                         status->elapsed_ms);
    lv_label_set_text(g_elapsed_label, elapsed_text);
}

static void music_ui_sync(uint32_t now_ms)
{
    music_player_status_t status;
    const char *name = NULL;
    uint32_t elapsed_second;
    uint32_t duration_second;
    int track_changed;
    int playback_state_changed;
    int elapsed_second_changed;
    int volume_changed;

    music_player_get_status(&status);
    elapsed_second = status.elapsed_ms / 1000U;
    duration_second = status.duration_ms / 1000U;
    track_changed = ((status.selected_index != g_last_selected_index) ||
                     (status.track_count != g_last_track_count)) ? 1 : 0;
    playback_state_changed =
        ((uint32_t)status.playback_state != g_last_playback_state) ? 1 : 0;
    elapsed_second_changed =
        ((elapsed_second != g_last_elapsed_second) ||
         (duration_second != g_last_duration_second)) ? 1 : 0;
    volume_changed = (status.volume_percent != g_last_volume_percent) ? 1 : 0;
    if ((track_changed != 0) || (g_navigation_needs_sync != 0U))
    {
        name = (status.track_count > 0U) ?
               music_player_get_track_name(status.selected_index) : NULL;
    }
    if (music_ui_object_valid(g_title_label) != 0)
    {
        if (track_changed != 0)
        {
            lv_label_set_text(g_title_label, (name != NULL) ? name :
                              "\xE9\x9F\xB3\xE4\xB9\x90");
        }
        if (playback_state_changed != 0)
        {
            lv_label_set_text(g_state_label,
                              music_ui_state_text(status.playback_state));
            lv_image_set_src(g_play_image,
                             (status.playback_state == AUDIO_STATE_PLAYING) ?
                             &img_lv_demo_music_btn_pause :
                             &img_lv_demo_music_btn_play);
        }
        if (elapsed_second_changed != 0)
        {
            music_ui_sync_progress(&status);
        }
        if ((track_changed != 0) || (playback_state_changed != 0))
        {
            music_ui_sync_track_buttons(&status);
        }
        if ((music_ui_object_valid(g_app) != 0) &&
            !lv_obj_has_flag(g_app, LV_OBJ_FLAG_HIDDEN))
        {
            music_visual_update_spectrum(
                &g_visual, status.elapsed_ms,
                status.playback_state == AUDIO_STATE_PLAYING,
                now_ms);
        }
    }

    if ((music_ui_object_valid(g_navigation_root) != 0) &&
        (music_ui_object_valid(guider_ui.screen_navigation_bar_label_9) != 0))
    {
        if ((track_changed != 0) || (g_navigation_needs_sync != 0U))
        {
            lv_label_set_text(guider_ui.screen_navigation_bar_label_9,
                              (name != NULL) ? name :
                              "\xE6\x9C\xAA\xE5\x9C\xA8\xE6\x92\xAD\xE6\x94\xBE");
        }
        if ((playback_state_changed != 0) ||
            (g_navigation_needs_sync != 0U))
        {
            lv_label_set_text(
                guider_ui.screen_navigation_bar_btn_14_label,
                (status.playback_state == AUDIO_STATE_PLAYING) ?
                " " LV_SYMBOL_PAUSE " " : " " LV_SYMBOL_PLAY " ");
            lv_obj_set_style_bg_color(
                guider_ui.screen_navigation_bar_btn_14,
                (status.playback_state == AUDIO_STATE_PLAYING) ?
                lv_color_hex(0x0098f2) : lv_color_hex(0xa9a9a9),
                LV_PART_MAIN);
            lv_obj_set_style_bg_color(
                guider_ui.screen_navigation_bar_btn_10,
                ((status.playback_state == AUDIO_STATE_PLAYING) ||
                 (status.playback_state == AUDIO_STATE_PAUSED)) ?
                lv_color_hex(0x0098f2) : lv_color_hex(0xa9a9a9),
                LV_PART_MAIN);
        }
        if (((volume_changed != 0) || (g_navigation_needs_sync != 0U)) &&
            !lv_obj_has_state(guider_ui.screen_navigation_bar_slider_2,
                              LV_STATE_PRESSED))
        {
            lv_slider_set_value(guider_ui.screen_navigation_bar_slider_2,
                                status.volume_percent, LV_ANIM_OFF);
        }
        g_navigation_needs_sync = 0U;
    }
    g_last_selected_index = status.selected_index;
    g_last_track_count = status.track_count;
    g_last_playback_state = (uint32_t)status.playback_state;
    g_last_elapsed_second = elapsed_second;
    g_last_duration_second = duration_second;
    g_last_volume_percent = status.volume_percent;
    g_next_sync_ms = now_ms +
        ((music_ui_object_valid(g_screen_root) != 0) ?
         MUSIC_UI_ACTIVE_SYNC_INTERVAL_MS :
         MUSIC_UI_IDLE_SYNC_INTERVAL_MS);
}

void music_player_ui_init(void)
{
    music_ui_reset_screen_objects();
    g_navigation_root = NULL;
    g_navigation_return_to_music = 0U;
    g_open_requested = 0U;
    g_next_sync_ms = 0U;
    music_player_service_init();
}

void music_player_ui_request_open(void)
{
    g_open_requested = 1U;
}

void music_player_ui_open(void)
{
    music_player_status_t status;
    lv_obj_t *active_screen = lv_screen_active();
    bool *old_screen_del;

    if ((music_ui_object_valid(guider_ui.screen_music) != 0) &&
        (active_screen == guider_ui.screen_music))
    {
        return;
    }
    if ((active_screen != guider_ui.DeskTop) &&
        (active_screen != guider_ui.screen_navigation_bar))
    {
        return;
    }
    music_player_ui_set_navigation_return(0);
    if (active_screen == guider_ui.screen_navigation_bar)
    {
        old_screen_del = &guider_ui.screen_navigation_bar_del;
    }
    else
    {
        old_screen_del = &guider_ui.DeskTop_del;
    }
    music_player_get_status(&status);
    if ((status.playback_state != AUDIO_STATE_PLAYING) &&
        (status.playback_state != AUDIO_STATE_PAUSED))
    {
        (void)music_player_scan();
    }
    else
    {
        (void)music_ui_ensure_library();
    }
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_music,
                          guider_ui.screen_music_del, old_screen_del,
                          setup_scr_screen_music,
                          LV_SCR_LOAD_ANIM_OVER_TOP,
                          200U, 200U, true, true);
}

void music_player_ui_poll(uint32_t now_ms)
{
    music_player_poll(now_ms);
    music_ui_attach_navigation();
    if (g_open_requested != 0U)
    {
        lv_display_t *display = lv_display_get_default();

        if ((display != NULL) &&
            (lv_display_get_screen_loading(display) == NULL))
        {
            g_open_requested = 0U;
            music_player_ui_open();
            return;
        }
    }
    if (music_ui_time_reached(now_ms, g_next_sync_ms) != 0)
    {
        music_ui_sync(now_ms);
    }
}

void music_player_ui_set_navigation_return(int enabled)
{
    g_navigation_return_to_music = (enabled != 0) ? 1U : 0U;
}

int music_player_ui_take_navigation_return(void)
{
    int result = (g_navigation_return_to_music != 0U) ? 1 : 0;

    g_navigation_return_to_music = 0U;
    return result;
}
