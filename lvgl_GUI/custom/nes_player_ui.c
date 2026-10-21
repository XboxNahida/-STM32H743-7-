#include "nes_player_ui.h"

#include <stdint.h>
#include <string.h>

#include "gui_guider.h"
#include "lv_port_indev.h"
#include "music_player_service.h"
#include "nes_controller_port.h"
#include "nes_input.h"
#include "nes_player_controls.h"
#include "nes_rom_catalog.h"
#include "nes_runtime.h"
#include "nes_video.h"
#define NES_ROM_DIRECTORY "0:/SYSTEM/NES"
#define NES_GAME_WIDTH 512
#define NES_GAME_HEIGHT 480

typedef enum { NES_UI_CLOSED = 0, NES_UI_LIST, NES_UI_STARTING,
               NES_UI_PLAYING, NES_UI_PAUSED, NES_UI_STOPPING } nes_ui_state_t;
typedef struct
{
    lv_obj_t *screen;
    lv_obj_t *list_panel;
    lv_obj_t *game_panel;
    lv_obj_t *status;
    lv_obj_t *image;
    lv_image_dsc_t image_desc;
    nes_ui_state_t state;
    uint32_t page_first;
    char selected_path[NES_ROM_CATALOG_PATH_SIZE];
    uint8_t open_pending;
    uint8_t start_pending;
    uint8_t exit_pending;
} nes_ui_context_t;
static nes_ui_context_t g_nes_ui;
static char g_nes_direct_path[NES_ROM_CATALOG_PATH_SIZE];
static lv_obj_t *nes_ui_button(lv_obj_t *parent, const char *text,
                               int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_t *label = lv_label_create(button);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, w, h);
    lv_obj_set_style_radius(button, 6, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x24272c), 0);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_myyh_20, 0);
    lv_obj_center(label);
    return button;
}
#include "nes_player_ui_layout.h"
static void nes_ui_set_status(const char *text)
{
    if (g_nes_ui.status != NULL)
    {
        lv_label_set_text(g_nes_ui.status, text);
    }
}

static const char *nes_ui_failure_text(int result)
{
    switch (result)
    {
        case NES_RUNTIME_UNSUPPORTED_ROM:
            return "\xE8\xAF\xA5\xE6\xB8\xB8\xE6\x88\x8F Mapper \xE4\xB8\x8D\xE6\x94\xAF\xE6\x8C\x81";
        case NES_RUNTIME_NO_MEMORY:
            return "NES \xE5\x86\x85\xE5\xAD\x98\xE4\xB8\x8D\xE8\xB6\xB3";
        case NES_RUNTIME_STORAGE_ERROR:
            return "\xE6\xB8\xB8\xE6\x88\x8F\xE6\x96\x87\xE4\xBB\xB6\xE8\xAF\xBB\xE5\x8F\x96\xE5\xA4\xB1\xE8\xB4\xA5";
        case NES_RUNTIME_INVALID_ROM:
            return "NES \xE6\x96\x87\xE4\xBB\xB6\xE6\x97\xA0\xE6\x95\x88";
        case NES_RUNTIME_CORE_ERROR:
            return "NES \xE6\xA0\xB8\xE5\xBF\x83\xE9\x94\x99\xE8\xAF\xAF";
        case NES_RUNTIME_BUSY:
            return "NES \xE6\xAD\xA3\xE5\x9C\xA8\xE8\xBF\x90\xE8\xA1\x8C";
        case NES_RUNTIME_INVALID_ARG:
            return "\xE6\xB8\xB8\xE6\x88\x8F\xE8\xB7\xAF\xE5\xBE\x84\xE6\x97\xA0\xE6\x95\x88";
        default:
            return "\xE6\xB8\xB8\xE6\x88\x8F\xE5\x8A\xA0\xE8\xBD\xBD\xE5\xA4\xB1\xE8\xB4\xA5";
    }
}

static void nes_ui_handle_failure(int result)
{
    const char *message = nes_ui_failure_text(result);

    (void)nes_runtime_stop();
    (void)nes_input_set_source(NES_INPUT_SOURCE_TOUCH, 0U);
    nes_controller_port_init();
    nes_player_controls_init();
    g_nes_ui.state = NES_UI_LIST;
    lv_obj_add_flag(g_nes_ui.game_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(g_nes_ui.list_panel, LV_OBJ_FLAG_HIDDEN);
    nes_ui_set_status(message);
}

static void nes_ui_lock_view(lv_obj_t *object)
{
    lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(object, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(object, 0, 0);
    lv_obj_set_style_border_width(object, 0, 0);
}
static void nes_ui_show_list(void);
static void nes_ui_back_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    if ((g_nes_ui.state == NES_UI_PLAYING) ||
        (g_nes_ui.state == NES_UI_PAUSED) ||
        (g_nes_ui.state == NES_UI_STARTING))
    {
        g_nes_ui.exit_pending = 1U;
        nes_ui_set_status("\xE6\xAD\xA3\xE5\x9C\xA8\xE9\x80\x80\xE5\x87\xBA");
    }
    else if (g_nes_ui.state == NES_UI_LIST)
    {
        lv_screen_load_anim(guider_ui.DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP,
                            200U, 0U, true);
        memset(&g_nes_ui, 0, sizeof(g_nes_ui));
    }
}
static void nes_ui_rom_event(lv_event_t *event)
{
    const nes_rom_catalog_entry_t *entry;
    uint8_t index;
    size_t length;

    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    index = (uint8_t)(uintptr_t)lv_obj_get_user_data(
        lv_event_get_current_target(event));
    entry = nes_rom_catalog_get(index);
    if (entry == NULL) return;
    length = strlen(entry->path);
    if (length >= sizeof(g_nes_ui.selected_path)) return;
    memcpy(g_nes_ui.selected_path, entry->path, length + 1U);
    g_nes_ui.start_pending = 1U;
    g_nes_ui.state = NES_UI_STARTING;
    lv_obj_add_flag(g_nes_ui.list_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(g_nes_ui.game_panel, LV_OBJ_FLAG_HIDDEN);
    nes_ui_set_status("\xE6\xAD\xA3\xE5\x9C\xA8\xE5\x8A\xA0\xE8\xBD\xBD");
}
static void nes_ui_page_event(lv_event_t *event)
{
    intptr_t direction;
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    direction = (intptr_t)lv_obj_get_user_data(
        lv_event_get_current_target(event));
    if (direction < 0)
    {
        g_nes_ui.page_first = (g_nes_ui.page_first >=
            NES_ROM_CATALOG_PAGE_SIZE) ?
            g_nes_ui.page_first - NES_ROM_CATALOG_PAGE_SIZE : 0U;
    }
    else
    {
        g_nes_ui.page_first += NES_ROM_CATALOG_PAGE_SIZE;
    }
    nes_ui_show_list();
}

static void nes_ui_add_pagination(const nes_rom_catalog_status_t *status)
{
    lv_obj_t *button;
    lv_obj_t *label;
    uint32_t current_page;
    uint32_t total_pages;

    current_page = g_nes_ui.page_first / NES_ROM_CATALOG_PAGE_SIZE + 1U;
    total_pages = status->total_count / NES_ROM_CATALOG_PAGE_SIZE;
    if ((status->total_count % NES_ROM_CATALOG_PAGE_SIZE) != 0U)
    {
        ++total_pages;
    }
    if (total_pages == 0U)
    {
        total_pages = 1U;
    }

    button = nes_ui_button(g_nes_ui.list_panel,
        "\xE4\xB8\x8A\xE4\xB8\x80\xE9\xA1\xB5", 280, 420, 180, 48);
    lv_obj_set_user_data(button, (void *)(intptr_t)-1);
    lv_obj_add_event_cb(button, nes_ui_page_event, LV_EVENT_CLICKED, NULL);
    if (g_nes_ui.page_first == 0U)
    {
        lv_obj_add_state(button, LV_STATE_DISABLED);
    }

    label = lv_label_create(g_nes_ui.list_panel);
    lv_obj_set_pos(label, 468, 430);
    lv_obj_set_size(label, 88, 30);
    lv_obj_set_style_text_font(label, &lv_font_myyh_20, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(label, "%lu / %lu",
                          (unsigned long)current_page,
                          (unsigned long)total_pages);

    button = nes_ui_button(g_nes_ui.list_panel,
        "\xE4\xB8\x8B\xE4\xB8\x80\xE9\xA1\xB5", 564, 420, 180, 48);
    lv_obj_set_user_data(button, (void *)(intptr_t)1);
    lv_obj_add_event_cb(button, nes_ui_page_event, LV_EVENT_CLICKED, NULL);
    if (status->has_more == 0U)
    {
        lv_obj_add_state(button, LV_STATE_DISABLED);
    }
}

static void nes_ui_show_list(void)
{
    nes_rom_catalog_status_t status;
    const nes_rom_catalog_entry_t *entry;
    lv_obj_t *button;
    uint8_t index;

    lv_obj_clean(g_nes_ui.list_panel);
    if (nes_rom_catalog_scan_page(NES_ROM_DIRECTORY,
                                  g_nes_ui.page_first) != 0)
    {
        nes_ui_set_status("SD\xE5\x8D\xA1\xE6\x88\x96\xE6\xB8\xB8\xE6\x88\x8F\xE7\x9B\xAE\xE5\xBD\x95\xE4\xB8\x8D\xE5\x8F\xAF\xE7\x94\xA8");
        return;
    }
    nes_rom_catalog_get_status(&status);
    for (index = 0U; index < status.count; ++index)
    {
        entry = nes_rom_catalog_get(index);
        button = nes_ui_button(g_nes_ui.list_panel, entry->name,
            12 + (index % 2U) * 492, 8 + (index / 2U) * 68,
            480, 58);
        lv_obj_set_user_data(button, (void *)(uintptr_t)index);
        lv_obj_add_event_cb(button, nes_ui_rom_event, LV_EVENT_CLICKED, NULL);
    }
    nes_ui_add_pagination(&status);
    nes_ui_set_status((status.count != 0U) ?
        "\xE9\x80\x89\xE6\x8B\xA9\xE6\xB8\xB8\xE6\x88\x8F" :
        "\xE6\xB2\xA1\xE6\x9C\x89\xE6\x89\xBE\xE5\x88\xB0NES\xE6\xB8\xB8\xE6\x88\x8F");
}
static void nes_ui_open_async(void *unused)
{
    lv_obj_t *back;
    (void)unused;
    g_nes_ui.open_pending = 0U;
    if (g_nes_ui.screen != NULL) return;
    g_nes_ui.screen = lv_obj_create(NULL);
    nes_ui_lock_view(g_nes_ui.screen);
    lv_obj_set_style_bg_color(g_nes_ui.screen, lv_color_black(), 0);
    back = nes_ui_button(g_nes_ui.screen, LV_SYMBOL_LEFT, 10, 8, 72, 40);
    lv_obj_add_event_cb(back, nes_ui_back_event, LV_EVENT_CLICKED, NULL);
    g_nes_ui.status = lv_label_create(g_nes_ui.screen);
    lv_obj_set_pos(g_nes_ui.status, 100, 14);
    lv_obj_set_size(g_nes_ui.status, 824, 32);
    lv_obj_set_style_text_font(g_nes_ui.status, &lv_font_myyh_20, 0);
    lv_obj_set_style_text_color(g_nes_ui.status, lv_color_white(), 0);
    lv_obj_set_style_text_align(g_nes_ui.status, LV_TEXT_ALIGN_CENTER, 0);
    g_nes_ui.list_panel = lv_obj_create(g_nes_ui.screen);
    nes_ui_lock_view(g_nes_ui.list_panel);
    lv_obj_set_pos(g_nes_ui.list_panel, 0, 52);
    lv_obj_set_size(g_nes_ui.list_panel, 1024, 548);
    lv_obj_set_style_bg_opa(g_nes_ui.list_panel, LV_OPA_TRANSP, 0);
    g_nes_ui.game_panel = lv_obj_create(g_nes_ui.screen);
    nes_ui_lock_view(g_nes_ui.game_panel);
    lv_obj_set_pos(g_nes_ui.game_panel, 0, 48);
    lv_obj_set_size(g_nes_ui.game_panel, 1024, 552);
    lv_obj_set_style_bg_opa(g_nes_ui.game_panel, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(g_nes_ui.game_panel, LV_OBJ_FLAG_HIDDEN);
    g_nes_ui.image = lv_image_create(g_nes_ui.game_panel);
    lv_obj_center(g_nes_ui.image);
    lv_image_set_scale(g_nes_ui.image, 256U);
    nes_ui_build_game_controls(g_nes_ui.game_panel);
    nes_ui_create_help(g_nes_ui.game_panel);
    g_nes_ui.state = NES_UI_LIST;
    nes_ui_show_list();
    if (g_nes_direct_path[0] != '\0')
    {
        memcpy(g_nes_ui.selected_path, g_nes_direct_path,
               strlen(g_nes_direct_path) + 1U);
        g_nes_direct_path[0] = '\0';
        g_nes_ui.start_pending = 1U;
        g_nes_ui.state = NES_UI_STARTING;
        lv_obj_add_flag(g_nes_ui.list_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(g_nes_ui.game_panel, LV_OBJ_FLAG_HIDDEN);
        nes_ui_set_status("正在加载");
    }
    lv_screen_load_anim(g_nes_ui.screen, LV_SCR_LOAD_ANIM_OVER_TOP,
                        200U, 0U, false);
}

void nes_player_ui_init(void)
{
    memset(&g_nes_ui, 0, sizeof(g_nes_ui));
    nes_player_controls_init();
}

void nes_player_ui_request_open(void)
{
    if ((g_nes_ui.screen == NULL) && (g_nes_ui.open_pending == 0U))
    {
        g_nes_ui.open_pending = 1U;
        lv_async_call(nes_ui_open_async, NULL);
    }
}

int nes_player_ui_request_open_file(const char *path)
{
    size_t length;
    if ((path == NULL) || (g_nes_ui.screen != NULL) ||
        (g_nes_ui.open_pending != 0U)) return 1;
    length = strlen(path);
    if ((length == 0U) || (length >= sizeof(g_nes_direct_path))) return 1;
    memcpy(g_nes_direct_path, path, length + 1U);
    nes_player_ui_request_open();
    return 0;
}

void nes_player_ui_poll(void)
{
    nes_runtime_status_t runtime;
    nes_video_frame_t frame;
    lv_port_touch_snapshot_t snapshot;
    nes_touch_point_t points[5];
    nes_button_mask_t buttons = 0U;
    uint32_t now_ms;
    int start_result = NES_RUNTIME_OK;
    uint8_t pause_toggle = 0U;
    uint8_t index;

    if (g_nes_ui.screen == NULL) return;
    if (g_nes_ui.start_pending != 0U)
    {
        g_nes_ui.start_pending = 0U;
        nes_player_controls_init();
        nes_controller_port_init();
        (void)music_player_stop();
        start_result = nes_runtime_start(g_nes_ui.selected_path);
        if (start_result != NES_RUNTIME_OK)
        {
            nes_ui_handle_failure(start_result);
            return;
        }
    }
    if (g_nes_ui.exit_pending != 0U)
    {
        g_nes_ui.exit_pending = 0U;
        g_nes_ui.state = NES_UI_STOPPING;
        (void)nes_runtime_stop();
        (void)nes_input_set_source(NES_INPUT_SOURCE_TOUCH, 0U);
        nes_controller_port_init();
        nes_player_controls_init();
        g_nes_ui.state = NES_UI_LIST;
        lv_obj_add_flag(g_nes_ui.game_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(g_nes_ui.list_panel, LV_OBJ_FLAG_HIDDEN);
        nes_ui_set_status("\xE9\x80\x89\xE6\x8B\xA9\xE6\xB8\xB8\xE6\x88\x8F");
        return;
    }
    nes_runtime_get_status(&runtime);
    if (runtime.state == NES_RUNTIME_ERROR)
    {
        nes_ui_handle_failure(runtime.result);
        return;
    }
    if ((runtime.state != NES_RUNTIME_RUNNING) &&
        (runtime.state != NES_RUNTIME_PAUSED))
    {
        return;
    }
    now_ms = lv_tick_get();
    nes_controller_port_poll(now_ms);
    lv_port_indev_get_touch_snapshot(&snapshot);
    for (index = 0U; index < snapshot.count; ++index)
    {
        points[index].x = snapshot.x[index]; points[index].y = snapshot.y[index];
    }
    (void)nes_player_controls_update(points, snapshot.count, now_ms,
                                     &buttons, &pause_toggle);
    if (nes_controller_port_take_pause_toggle() != 0U)
    {
        pause_toggle = 1U;
    }
    if (pause_toggle != 0U)
    {
        uint8_t pause = (runtime.state == NES_RUNTIME_RUNNING) ? 1U : 0U;
        if (nes_runtime_set_paused(pause) == NES_RUNTIME_OK)
        {
            runtime.state = (pause != 0U) ?
                NES_RUNTIME_PAUSED : NES_RUNTIME_RUNNING;
        }
    }
    if (runtime.state == NES_RUNTIME_PAUSED)
    {
        g_nes_ui.state = NES_UI_PAUSED;
        nes_ui_set_status("\xE5\xB7\xB2\xE6\x9A\x82\xE5\x81\x9C");
        (void)nes_input_set_source(NES_INPUT_SOURCE_TOUCH, 0U);
        return;
    }
    if (g_nes_ui.state != NES_UI_PLAYING)
    {
        nes_ui_set_status("\xE8\xBF\x90\xE8\xA1\x8C\xE4\xB8\xAD");
    }
    g_nes_ui.state = NES_UI_PLAYING;
    (void)nes_input_set_source(NES_INPUT_SOURCE_TOUCH, buttons);
    if (nes_video_get_frame(&frame) == NES_VIDEO_OK)
    {
        g_nes_ui.image_desc.header.magic = LV_IMAGE_HEADER_MAGIC;
        g_nes_ui.image_desc.header.cf = LV_COLOR_FORMAT_RGB565;
        g_nes_ui.image_desc.header.w = frame.width;
        g_nes_ui.image_desc.header.h = frame.height;
        g_nes_ui.image_desc.header.stride = frame.stride_pixels * 2U;
        g_nes_ui.image_desc.data_size = g_nes_ui.image_desc.header.stride * frame.height;
        g_nes_ui.image_desc.data = (const uint8_t *)frame.pixels;
        lv_image_set_src(g_nes_ui.image, &g_nes_ui.image_desc);
        lv_obj_invalidate(g_nes_ui.image);
        (void)nes_video_mark_consumed(frame.generation);
    }
}
