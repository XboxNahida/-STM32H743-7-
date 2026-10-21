#include "file_manager_ui.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "file_manager_service.h"
#include "gui_guider.h"
#include "rtc.h"

typedef struct
{
    lv_obj_t *screen;
    lv_obj_t *clock_label;
    lv_obj_t *path_label;
    lv_obj_t *list;
    lv_obj_t *page_label;
    lv_obj_t *status_label;
    lv_obj_t *open_button;
    lv_obj_t *prev_button;
    lv_obj_t *next_button;
    lv_obj_t *modal;
    lv_obj_t *name_input;
    lv_obj_t *progress_bar;
    lv_obj_t *progress_label;
    file_manager_page_t page;
    char path[FILE_MANAGER_PATH_CAPACITY];
    char selected_path[FILE_MANAGER_PATH_CAPACITY];
    uint32_t offset;
    uint32_t last_click_tick;
    uint32_t last_clock_tick;
    uint16_t selected_index;
    uint16_t last_click_index;
    uint8_t open_pending;
    uint8_t refresh_pending;
    uint8_t readonly_pending;
    uint8_t compact;
    lv_obj_t *readonly_parent;
    file_manager_entry_type_t create_type;
} file_manager_ui_context_t;

static file_manager_ui_context_t g_file_ui;
static file_manager_ui_open_cb_t g_open_callback;
static char g_clipboard_path[FILE_MANAGER_PATH_CAPACITY];
static uint8_t g_clipboard_move;
static uint8_t g_task_active;
static uint8_t g_task_background;
static uint8_t g_show_progress_pending;
static lv_obj_t *g_navigation_indicator;

static lv_obj_t *file_ui_button(lv_obj_t *parent, const char *text,
                                int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_t *label = lv_label_create(button);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, w, h);
    lv_obj_set_style_radius(button, 6, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x2f7f88),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x23666e),
                              LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x2f7f88),
                              LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x2f7f88),
                              LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_color(button, lv_color_hex(0xd7e1e3),
                              LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x1f6870), 0);
    lv_obj_set_style_outline_width(button, 0,
                                   LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(button, 0,
                                   LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_myyh_20, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(0x87969a),
                                LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_center(label);
    return button;
}

static void file_ui_update_clock(uint8_t force)
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    char text[16];
    uint32_t now;

    if ((g_file_ui.clock_label == NULL) ||
        !lv_obj_is_valid(g_file_ui.clock_label)) return;
    now = HAL_GetTick();
    if ((force == 0U) &&
        ((uint32_t)(now - g_file_ui.last_clock_tick) < 1000U)) return;
    g_file_ui.last_clock_tick = now;
    if (HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) return;
    (void)HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
    (void)snprintf(text, sizeof(text), "%02u:%02u",
                   (unsigned int)time.Hours,
                   (unsigned int)time.Minutes);
    lv_label_set_text(g_file_ui.clock_label, text);
}

static void file_ui_create_status_bar(lv_obj_t *screen)
{
    lv_obj_t *bar;
    lv_obj_t *status;

    bar = lv_obj_create(screen);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_size(bar, 1024, 35);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xf1f6f7), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);

    g_file_ui.clock_label = lv_label_create(bar);
    lv_obj_set_pos(g_file_ui.clock_label, 0, 2);
    lv_obj_set_size(g_file_ui.clock_label, 110, 31);
    lv_label_set_text(g_file_ui.clock_label, "--:--:--");
    lv_obj_set_style_text_color(g_file_ui.clock_label,
                                lv_color_hex(0x17383d), 0);
    lv_obj_set_style_text_font(g_file_ui.clock_label,
                               &lv_font_montserratMedium_20, 0);
    lv_obj_set_style_text_align(g_file_ui.clock_label,
                                LV_TEXT_ALIGN_CENTER, 0);

    status = lv_label_create(bar);
    lv_obj_set_pos(status, 805, 2);
    lv_obj_set_size(status, 215, 31);
    lv_label_set_text(status, "   " LV_SYMBOL_WIFI "  "
        LV_SYMBOL_VOLUME_MAX " " LV_SYMBOL_USB " "
        LV_SYMBOL_BATTERY_FULL "  100%");
    lv_obj_set_style_text_color(status, lv_color_hex(0x17383d), 0);
    lv_obj_set_style_text_font(status, &lv_font_montserratMedium_20, 0);
    lv_obj_move_foreground(bar);
    file_ui_update_clock(1U);
}

static void file_ui_set_page_button_state(void)
{
    if (g_file_ui.prev_button != NULL)
    {
        if (g_file_ui.offset == 0U)
            lv_obj_add_state(g_file_ui.prev_button, LV_STATE_DISABLED);
        else
            lv_obj_remove_state(g_file_ui.prev_button, LV_STATE_DISABLED);
    }
    if (g_file_ui.next_button != NULL)
    {
        if (g_file_ui.page.has_more == 0U)
            lv_obj_add_state(g_file_ui.next_button, LV_STATE_DISABLED);
        else
            lv_obj_remove_state(g_file_ui.next_button, LV_STATE_DISABLED);
    }
}

static void file_ui_update_selection(void)
{
    uint16_t index;

    if ((g_file_ui.list == NULL) || !lv_obj_is_valid(g_file_ui.list)) return;
    for (index = 0U; index < g_file_ui.page.count; index++)
    {
        lv_obj_t *row = lv_obj_get_child(g_file_ui.list, (int32_t)index);
        uint8_t selected = ((index + 1U) == g_file_ui.selected_index);
        if (row == NULL) continue;
        lv_obj_set_style_bg_color(row,
            lv_color_hex(selected ? 0xcde9ec : 0xffffff),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(row, selected ? 2 : 1,
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(row,
            lv_color_hex(selected ? 0x2f7f88 : 0xb7c9cc),
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void file_ui_set_status(const char *text)
{
    if (g_file_ui.status_label != NULL)
        lv_label_set_text(g_file_ui.status_label, text);
}

static void file_ui_close_modal(void)
{
    if ((g_file_ui.modal != NULL) && lv_obj_is_valid(g_file_ui.modal))
        lv_obj_delete(g_file_ui.modal);
    g_file_ui.modal = NULL;
    g_file_ui.name_input = NULL;
    g_file_ui.progress_bar = NULL;
    g_file_ui.progress_label = NULL;
}

static lv_obj_t *file_ui_modal_create(const char *title, int32_t height)
{
    lv_obj_t *panel;
    lv_obj_t *label;

    file_ui_close_modal();
    g_file_ui.modal = lv_obj_create(g_file_ui.screen);
    lv_obj_set_pos(g_file_ui.modal, 0, 0);
    lv_obj_set_size(g_file_ui.modal, g_file_ui.compact ? 600 : 1024,
                    g_file_ui.compact ? 520 : 600);
    lv_obj_set_style_bg_color(g_file_ui.modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_file_ui.modal, 170, 0);
    lv_obj_remove_flag(g_file_ui.modal, LV_OBJ_FLAG_SCROLLABLE);
    panel = lv_obj_create(g_file_ui.modal);
    lv_obj_set_size(panel, g_file_ui.compact ? 560 : 620, height);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0xf8fbfb), 0);
    lv_obj_set_style_text_color(panel, lv_color_hex(0x17383d), 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x86adb1), 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    label = lv_label_create(panel);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, &lv_font_myyh_20, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 8);
    return panel;
}

static void file_ui_parent_path(void)
{
    char *slash;
    size_t length = strlen(g_file_ui.path);

    if (length <= 3U) return;
    if (g_file_ui.path[length - 1U] == '/') g_file_ui.path[length - 1U] = '\0';
    slash = strrchr(g_file_ui.path, '/');
    if (slash == NULL) return;
    slash[1] = '\0';
    g_file_ui.offset = 0U;
    g_file_ui.refresh_pending = 1U;
}

static const char *file_ui_route_name(file_manager_kind_t kind)
{
    switch (kind)
    {
    case FILE_MANAGER_KIND_TEXT: return "笔记";
    case FILE_MANAGER_KIND_JPEG:
    case FILE_MANAGER_KIND_IMAGE_UNSUPPORTED: return "图片查看器";
    case FILE_MANAGER_KIND_DRAWING_BMP: return "画板";
    case FILE_MANAGER_KIND_AVI: return "视频播放器";
    case FILE_MANAGER_KIND_MP3:
    case FILE_MANAGER_KIND_WAV: return "音乐";
    case FILE_MANAGER_KIND_NES: return "小游戏";
    default: return NULL;
    }
}

static void file_ui_route_cancel_event(lv_event_t *event)
{ if (lv_event_get_code(event) == LV_EVENT_CLICKED) file_ui_close_modal(); }

static void file_ui_route_confirm_event(lv_event_t *event)
{
    int result;
    uint8_t compact;

    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    compact = g_file_ui.compact;
    file_ui_close_modal();
    result = (g_open_callback != NULL) ?
             g_open_callback(g_file_ui.selected_path) : 1;
    if (result != 0) { file_ui_set_status("无法打开此文件"); return; }
    if (compact == 0U)
    {
        lv_screen_load_anim(guider_ui.DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP,
                            120U, 0U, true);
        memset(&g_file_ui, 0, sizeof(g_file_ui));
    }
}

static void file_ui_show_route_dialog(file_manager_kind_t kind)
{
    const char *application = file_ui_route_name(kind);
    lv_obj_t *panel;
    lv_obj_t *label;
    lv_obj_t *button;

    if (application == NULL) { file_ui_set_status("没有可打开此类型文件的应用"); return; }
    panel = file_ui_modal_create("打开文件", 210);
    label = lv_label_create(panel);
    lv_label_set_text_fmt(label, "是否使用%s打开？", application);
    lv_obj_set_pos(label, 30, 62);
    lv_obj_set_size(label, g_file_ui.compact ? 500 : 560, 40);
    lv_obj_set_style_text_font(label, &lv_font_myyh_20, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    button = file_ui_button(panel, "取消", 110, 140, 150, 44);
    lv_obj_add_event_cb(button, file_ui_route_cancel_event, LV_EVENT_CLICKED, NULL);
    button = file_ui_button(panel, "打开", 350, 140, 150, 44);
    lv_obj_add_event_cb(button, file_ui_route_confirm_event, LV_EVENT_CLICKED, NULL);
}

static void file_ui_open_selected(void)
{
    file_manager_entry_t *entry;
    file_manager_result_t result;

    if ((g_file_ui.selected_index == 0U) ||
        (g_file_ui.selected_index > g_file_ui.page.count))
    {
        file_ui_set_status("请先选择文件或文件夹");
        return;
    }
    entry = &g_file_ui.page.entries[g_file_ui.selected_index - 1U];
    if (entry->type == FILE_MANAGER_ENTRY_DIRECTORY)
    {
        result = file_manager_join_path(g_file_ui.path, entry->name,
                                        g_file_ui.path,
                                        sizeof(g_file_ui.path));
        if (result != FILE_MANAGER_OK)
        {
            file_ui_set_status("目录路径过长");
            return;
        }
        g_file_ui.offset = 0U;
        g_file_ui.refresh_pending = 1U;
        return;
    }
    file_ui_show_route_dialog(file_manager_classify(g_file_ui.selected_path));
}

static void file_ui_row_event(lv_event_t *event)
{
    uint16_t index;
    uint32_t now;

    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    index = (uint16_t)(uintptr_t)lv_event_get_user_data(event);
    if ((index == 0U) || (index > g_file_ui.page.count)) return;
    now = lv_tick_get();
    if ((g_file_ui.last_click_index == index) &&
        ((uint32_t)(now - g_file_ui.last_click_tick) <= 450U))
    {
        g_file_ui.selected_index = index;
        file_ui_open_selected();
        g_file_ui.last_click_index = 0U;
        return;
    }
    g_file_ui.selected_index = index;
    g_file_ui.last_click_index = index;
    g_file_ui.last_click_tick = now;
    file_ui_update_selection();
    if (file_manager_join_path(g_file_ui.path,
            g_file_ui.page.entries[index - 1U].name,
            g_file_ui.selected_path, sizeof(g_file_ui.selected_path)) !=
        FILE_MANAGER_OK)
    {
        g_file_ui.selected_path[0] = '\0';
        file_ui_set_status("文件路径过长");
        return;
    }
    file_ui_set_status(g_file_ui.page.entries[index - 1U].name);
}

static void file_ui_refresh(void)
{
    char page_text[48];
    uint16_t index;
    file_manager_result_t result;

    g_file_ui.refresh_pending = 0U;
    g_file_ui.selected_index = 0U;
    g_file_ui.selected_path[0] = '\0';
    result = file_manager_list_page(g_file_ui.path, g_file_ui.offset,
                                    &g_file_ui.page);
    if (result != FILE_MANAGER_OK)
    {
        lv_obj_clean(g_file_ui.list);
        memset(&g_file_ui.page, 0, sizeof(g_file_ui.page));
        file_ui_set_page_button_state();
        file_ui_set_status((result == FILE_MANAGER_NOT_READY) ?
                           "SD卡尚未就绪" : "目录读取失败");
        return;
    }
    lv_obj_clean(g_file_ui.list);
    for (index = 0U; index < g_file_ui.page.count; index++)
    {
        file_manager_entry_t *entry = &g_file_ui.page.entries[index];
        lv_obj_t *button = lv_button_create(g_file_ui.list);
        lv_obj_t *name = lv_label_create(button);
        lv_obj_t *detail = lv_label_create(button);
        lv_obj_set_width(button, LV_PCT(100));
        lv_obj_set_height(button, 48);
        lv_obj_set_style_radius(button, 4, 0);
        lv_obj_set_style_bg_color(button, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_bg_color(button, lv_color_hex(0xe2f1f2),
                                  LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_width(button, 1, 0);
        lv_obj_set_style_border_color(button, lv_color_hex(0xb7c9cc), 0);
        lv_obj_set_style_text_color(button, lv_color_hex(0x17383d), 0);
        lv_label_set_text_fmt(name, "%s  %s",
            (entry->type == FILE_MANAGER_ENTRY_DIRECTORY) ? "[目录]" : "[文件]",
            entry->name);
        lv_obj_set_pos(name, 12, 10);
        lv_obj_set_width(name, g_file_ui.compact ? 340 : 650);
        lv_label_set_long_mode(name, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_font(name, &lv_font_myyh_20, 0);
        if (entry->type == FILE_MANAGER_ENTRY_DIRECTORY)
            lv_label_set_text(detail, "双击进入");
        else
            lv_label_set_text_fmt(detail, "%llu 字节",
                                  (unsigned long long)entry->size);
        lv_obj_set_pos(detail, g_file_ui.compact ? 355 : 700, 11);
        lv_obj_set_width(detail, g_file_ui.compact ? 170 : 210);
        lv_obj_set_style_text_align(detail, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_font(detail, &lv_font_myyh_20, 0);
        lv_obj_add_event_cb(button, file_ui_row_event, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)(index + 1U));
    }
    lv_label_set_text(g_file_ui.path_label, g_file_ui.path);
    snprintf(page_text, sizeof(page_text), "第 %lu 页  本页 %u 项",
             (unsigned long)(g_file_ui.offset / FILE_MANAGER_PAGE_SIZE + 1U),
             (unsigned int)g_file_ui.page.count);
    lv_label_set_text(g_file_ui.page_label, page_text);
    file_ui_set_page_button_state();
    file_ui_set_status("单击选择，双击进入目录");
}

static void file_ui_back_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    lv_screen_load_anim(guider_ui.DeskTop, LV_SCR_LOAD_ANIM_OVER_TOP,
                        180U, 0U, true);
    memset(&g_file_ui, 0, sizeof(g_file_ui));
}

static void file_ui_parent_event(lv_event_t *event)
{ if (lv_event_get_code(event) == LV_EVENT_CLICKED) file_ui_parent_path(); }
static void file_ui_open_event(lv_event_t *event)
{ if (lv_event_get_code(event) == LV_EVENT_CLICKED) file_ui_open_selected(); }
static void file_ui_prev_event(lv_event_t *event)
{
    if ((lv_event_get_code(event) == LV_EVENT_CLICKED) && (g_file_ui.offset != 0U))
    { g_file_ui.offset -= FILE_MANAGER_PAGE_SIZE; g_file_ui.refresh_pending = 1U; }
}
static void file_ui_next_event(lv_event_t *event)
{
    if ((lv_event_get_code(event) == LV_EVENT_CLICKED) && (g_file_ui.page.has_more != 0U))
    { g_file_ui.offset = g_file_ui.page.next_offset; g_file_ui.refresh_pending = 1U; }
}

static void file_ui_name_cancel_event(lv_event_t *event)
{ if (lv_event_get_code(event) == LV_EVENT_CLICKED) file_ui_close_modal(); }

static void file_ui_name_confirm_event(lv_event_t *event)
{
    const char *name;
    file_manager_result_t result;

    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    name = lv_textarea_get_text(g_file_ui.name_input);
    result = file_manager_create(g_file_ui.path, name, g_file_ui.create_type);
    if (result == FILE_MANAGER_OK)
    {
        file_ui_close_modal();
        g_file_ui.refresh_pending = 1U;
        file_ui_set_status("新建成功");
    }
    else
        file_ui_set_status((result == FILE_MANAGER_EXISTS) ?
                           "名称已存在" : "新建失败，请检查名称");
}

static void file_ui_show_name_dialog(file_manager_entry_type_t type)
{
    lv_obj_t *panel = file_ui_modal_create(
        (type == FILE_MANAGER_ENTRY_DIRECTORY) ? "新建文件夹" : "新建文件", 430);
    lv_obj_t *keyboard;
    lv_obj_t *button;

    g_file_ui.create_type = type;
    g_file_ui.name_input = lv_textarea_create(panel);
    lv_obj_set_pos(g_file_ui.name_input, 30, 48);
    lv_obj_set_size(g_file_ui.name_input, 560, 48);
    lv_textarea_set_one_line(g_file_ui.name_input, true);
    lv_obj_set_style_text_font(g_file_ui.name_input, &lv_font_myyh_20, 0);
    keyboard = lv_keyboard_create(panel);
    lv_obj_set_pos(keyboard, 20, 108);
    lv_obj_set_size(keyboard, 580, 250);
    lv_obj_set_style_text_font(keyboard, &lv_font_myyh_18, 0);
    lv_keyboard_set_textarea(keyboard, g_file_ui.name_input);
    button = file_ui_button(panel, "取消", 120, 368, 140, 42);
    lv_obj_add_event_cb(button, file_ui_name_cancel_event, LV_EVENT_CLICKED, NULL);
    button = file_ui_button(panel, "确定", 350, 368, 140, 42);
    lv_obj_add_event_cb(button, file_ui_name_confirm_event, LV_EVENT_CLICKED, NULL);
}

static void file_ui_new_file_event(lv_event_t *event)
{ if (lv_event_get_code(event) == LV_EVENT_CLICKED) file_ui_show_name_dialog(FILE_MANAGER_ENTRY_FILE); }
static void file_ui_new_dir_event(lv_event_t *event)
{ if (lv_event_get_code(event) == LV_EVENT_CLICKED) file_ui_show_name_dialog(FILE_MANAGER_ENTRY_DIRECTORY); }

static void file_ui_delete_cancel_event(lv_event_t *event)
{ if (lv_event_get_code(event) == LV_EVENT_CLICKED) file_ui_close_modal(); }

static void file_ui_show_progress(void);

static void file_ui_delete_confirm_event(lv_event_t *event)
{
    file_manager_result_t result;
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    result = file_manager_delete_start(g_file_ui.selected_path, 1U);
    if (result != FILE_MANAGER_OK) { file_ui_set_status("删除任务启动失败"); return; }
    file_ui_close_modal();
    g_task_active = 1U;
    g_task_background = 0U;
    file_ui_show_progress();
}

static void file_ui_delete_event(lv_event_t *event)
{
    lv_obj_t *panel;
    lv_obj_t *label;
    lv_obj_t *button;
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    if (g_file_ui.selected_path[0] == '\0') { file_ui_set_status("请先选择项目"); return; }
    panel = file_ui_modal_create("确认删除", 210);
    label = lv_label_create(panel);
    lv_label_set_text_fmt(label, "将删除：%s\n非空文件夹会递归删除", g_file_ui.selected_path);
    lv_obj_set_pos(label, 25, 52);
    lv_obj_set_size(label, 570, 70);
    lv_obj_set_style_text_font(label, &lv_font_myyh_20, 0);
    button = file_ui_button(panel, "取消", 110, 145, 150, 42);
    lv_obj_add_event_cb(button, file_ui_delete_cancel_event, LV_EVENT_CLICKED, NULL);
    button = file_ui_button(panel, "删除", 350, 145, 150, 42);
    lv_obj_add_event_cb(button, file_ui_delete_confirm_event, LV_EVENT_CLICKED, NULL);
}

static void file_ui_copy_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    if (g_file_ui.selected_path[0] == '\0') { file_ui_set_status("请先选择项目"); return; }
    memcpy(g_clipboard_path, g_file_ui.selected_path, strlen(g_file_ui.selected_path) + 1U);
    g_clipboard_move = 0U;
    file_ui_set_status("已复制，进入目标目录后点击粘贴");
}

static void file_ui_move_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    if (g_file_ui.selected_path[0] == '\0') { file_ui_set_status("请先选择项目"); return; }
    memcpy(g_clipboard_path, g_file_ui.selected_path, strlen(g_file_ui.selected_path) + 1U);
    g_clipboard_move = 1U;
    file_ui_set_status("已剪切，进入目标目录后点击粘贴");
}

static void file_ui_start_paste(file_manager_conflict_t conflict)
{
    file_manager_result_t result = g_clipboard_move ?
        file_manager_move_start(g_clipboard_path, g_file_ui.path, conflict) :
        file_manager_copy_start(g_clipboard_path, g_file_ui.path, conflict);
    file_ui_close_modal();
    if (result != FILE_MANAGER_OK)
    {
        file_ui_set_status((result == FILE_MANAGER_CANCELLED) ? "已取消" : "任务启动失败");
        return;
    }
    g_task_active = 1U;
    g_task_background = 0U;
    file_ui_show_progress();
}

static void file_ui_conflict_overwrite_event(lv_event_t *event)
{ if (lv_event_get_code(event) == LV_EVENT_CLICKED) file_ui_start_paste(FILE_MANAGER_CONFLICT_OVERWRITE); }
static void file_ui_conflict_rename_event(lv_event_t *event)
{ if (lv_event_get_code(event) == LV_EVENT_CLICKED) file_ui_start_paste(FILE_MANAGER_CONFLICT_RENAME); }
static void file_ui_conflict_cancel_event(lv_event_t *event)
{ if (lv_event_get_code(event) == LV_EVENT_CLICKED) file_ui_close_modal(); }

static void file_ui_paste_event(lv_event_t *event)
{
    lv_obj_t *panel;
    lv_obj_t *button;
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    if (g_clipboard_path[0] == '\0') { file_ui_set_status("剪贴板为空"); return; }
    panel = file_ui_modal_create("同名文件处理", 190);
    button = file_ui_button(panel, "覆盖", 30, 80, 160, 52);
    lv_obj_add_event_cb(button, file_ui_conflict_overwrite_event, LV_EVENT_CLICKED, NULL);
    button = file_ui_button(panel, "自动重命名", 220, 80, 180, 52);
    lv_obj_add_event_cb(button, file_ui_conflict_rename_event, LV_EVENT_CLICKED, NULL);
    button = file_ui_button(panel, "取消", 430, 80, 160, 52);
    lv_obj_add_event_cb(button, file_ui_conflict_cancel_event, LV_EVENT_CLICKED, NULL);
}

static void file_ui_progress_cancel_event(lv_event_t *event)
{
    file_manager_result_t result;
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    result = file_manager_cancel();
    if (result == FILE_MANAGER_BUSY)
    {
        lv_label_set_text(g_file_ui.progress_label, "删除阶段不能取消");
        return;
    }
    file_ui_close_modal();
}

static void file_ui_progress_background_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    g_task_background = 1U;
    file_ui_close_modal();
}

static void file_ui_show_progress(void)
{
    lv_obj_t *panel;
    lv_obj_t *button;
    if ((g_file_ui.screen == NULL) || !lv_obj_is_valid(g_file_ui.screen)) return;
    panel = file_ui_modal_create("文件操作", 230);
    g_file_ui.progress_label = lv_label_create(panel);
    lv_obj_set_pos(g_file_ui.progress_label, 25, 55);
    lv_obj_set_size(g_file_ui.progress_label, 570, 32);
    lv_obj_set_style_text_font(g_file_ui.progress_label, &lv_font_myyh_20, 0);
    g_file_ui.progress_bar = lv_bar_create(panel);
    lv_obj_set_pos(g_file_ui.progress_bar, 25, 100);
    lv_obj_set_size(g_file_ui.progress_bar, 570, 24);
    lv_bar_set_range(g_file_ui.progress_bar, 0, 100);
    button = file_ui_button(panel, "取消", 110, 155, 150, 44);
    lv_obj_add_event_cb(button, file_ui_progress_cancel_event, LV_EVENT_CLICKED, NULL);
    button = file_ui_button(panel, "后台运行", 350, 155, 150, 44);
    lv_obj_add_event_cb(button, file_ui_progress_background_event, LV_EVENT_CLICKED, NULL);
}

static void file_ui_navigation_indicator_event(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    g_show_progress_pending = 1U;
    file_manager_ui_request_open();
}

static void file_ui_update_navigation_indicator(
    const file_manager_status_t *status)
{
    lv_obj_t *label;
    lv_obj_t *navigation_parent =
        guider_ui.screen_navigation_bar_cont_navigation_bar_return;
    if ((guider_ui.screen_navigation_bar == NULL) ||
        !lv_obj_is_valid(guider_ui.screen_navigation_bar) ||
        (navigation_parent == NULL) ||
        !lv_obj_is_valid(navigation_parent))
    {
        g_navigation_indicator = NULL;
        return;
    }
    if ((g_navigation_indicator == NULL) ||
        !lv_obj_is_valid(g_navigation_indicator) ||
        (lv_obj_get_parent(g_navigation_indicator) != navigation_parent))
    {
        g_navigation_indicator = file_ui_button(
            navigation_parent,
            "文件操作", 700, 520, 270, 44);
        lv_obj_add_event_cb(g_navigation_indicator,
            file_ui_navigation_indicator_event, LV_EVENT_CLICKED, NULL);
    }
    label = lv_obj_get_child(g_navigation_indicator, 0);
    if (label != NULL)
        lv_label_set_text_fmt(label, "%s  %u%%",
            status->current_name[0] ? status->current_name : "文件操作",
            (unsigned int)status->percent);
}

static void file_ui_create(void)
{
    lv_obj_t *button;

    memset(&g_file_ui, 0, sizeof(g_file_ui));
    memcpy(g_file_ui.path, "0:/", 4U);
    g_file_ui.screen = lv_obj_create(NULL);
    lv_obj_set_size(g_file_ui.screen, 1024, 600);
    lv_obj_remove_flag(g_file_ui.screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(g_file_ui.screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(g_file_ui.screen, lv_color_hex(0xf1f6f7), 0);
    lv_obj_set_style_text_color(g_file_ui.screen, lv_color_hex(0x17383d), 0);
    lv_obj_set_style_text_font(g_file_ui.screen, &lv_font_myyh_20, 0);
    file_ui_create_status_bar(g_file_ui.screen);
    button = file_ui_button(g_file_ui.screen, "返回", 12, 50, 86, 40);
    lv_obj_add_event_cb(button, file_ui_back_event, LV_EVENT_CLICKED, NULL);
    g_file_ui.path_label = lv_label_create(g_file_ui.screen);
    lv_obj_set_pos(g_file_ui.path_label, 112, 57);
    lv_obj_set_size(g_file_ui.path_label, 665, 32);
    lv_label_set_long_mode(g_file_ui.path_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    button = file_ui_button(g_file_ui.screen, "上级", 798, 50, 86, 40);
    lv_obj_add_event_cb(button, file_ui_parent_event, LV_EVENT_CLICKED, NULL);
    g_file_ui.open_button = file_ui_button(g_file_ui.screen, "打开", 900, 50, 86, 40);
    lv_obj_add_event_cb(g_file_ui.open_button, file_ui_open_event, LV_EVENT_CLICKED, NULL);
    button = file_ui_button(g_file_ui.screen, "新建文件", 12, 100, 145, 42);
    lv_obj_add_event_cb(button, file_ui_new_file_event, LV_EVENT_CLICKED, NULL);
    button = file_ui_button(g_file_ui.screen, "新建文件夹", 169, 100, 145, 42);
    lv_obj_add_event_cb(button, file_ui_new_dir_event, LV_EVENT_CLICKED, NULL);
    button = file_ui_button(g_file_ui.screen, "删除", 326, 100, 145, 42);
    lv_obj_add_event_cb(button, file_ui_delete_event, LV_EVENT_CLICKED, NULL);
    button = file_ui_button(g_file_ui.screen, "复制", 483, 100, 145, 42);
    lv_obj_add_event_cb(button, file_ui_copy_event, LV_EVENT_CLICKED, NULL);
    button = file_ui_button(g_file_ui.screen, "移动", 640, 100, 145, 42);
    lv_obj_add_event_cb(button, file_ui_move_event, LV_EVENT_CLICKED, NULL);
    button = file_ui_button(g_file_ui.screen, "粘贴", 797, 100, 145, 42);
    lv_obj_add_event_cb(button, file_ui_paste_event, LV_EVENT_CLICKED, NULL);
    g_file_ui.list = lv_obj_create(g_file_ui.screen);
    lv_obj_set_pos(g_file_ui.list, 12, 150);
    lv_obj_set_size(g_file_ui.list, 1000, 352);
    lv_obj_set_scroll_dir(g_file_ui.list, LV_DIR_VER);
    lv_obj_set_flex_flow(g_file_ui.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_file_ui.list, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(g_file_ui.list, 6, 0);
    lv_obj_set_style_pad_row(g_file_ui.list, 4, 0);
    lv_obj_set_style_bg_color(g_file_ui.list, lv_color_hex(0xe8f0f1), 0);
    lv_obj_set_style_border_color(g_file_ui.list, lv_color_hex(0x9db9bd), 0);
    g_file_ui.prev_button = file_ui_button(g_file_ui.screen, "上一页", 12, 518, 104, 46);
    lv_obj_add_event_cb(g_file_ui.prev_button, file_ui_prev_event, LV_EVENT_CLICKED, NULL);
    g_file_ui.page_label = lv_label_create(g_file_ui.screen);
    lv_obj_set_pos(g_file_ui.page_label, 132, 530);
    lv_obj_set_size(g_file_ui.page_label, 250, 30);
    g_file_ui.next_button = file_ui_button(g_file_ui.screen, "下一页", 390, 518, 104, 46);
    lv_obj_add_event_cb(g_file_ui.next_button, file_ui_next_event, LV_EVENT_CLICKED, NULL);
    g_file_ui.status_label = lv_label_create(g_file_ui.screen);
    lv_obj_set_pos(g_file_ui.status_label, 520, 530);
    lv_obj_set_size(g_file_ui.status_label, 480, 30);
    lv_label_set_long_mode(g_file_ui.status_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(g_file_ui.status_label, LV_TEXT_ALIGN_RIGHT, 0);
    g_file_ui.refresh_pending = 1U;
    lv_screen_load_anim(g_file_ui.screen, LV_SCR_LOAD_ANIM_OVER_TOP,
                        180U, 0U, false);
}

static void file_ui_create_readonly(lv_obj_t *parent)
{
    lv_obj_t *button;
    lv_obj_t *outer;

    if ((parent == NULL) || !lv_obj_is_valid(parent)) return;
    memset(&g_file_ui, 0, sizeof(g_file_ui));
    memcpy(g_file_ui.path, "0:/", 4U);
    g_file_ui.compact = 1U;
    g_file_ui.screen = parent;
    lv_obj_clean(parent);
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);
    outer = lv_obj_get_parent(parent);
    if (outer != NULL)
    {
        lv_obj_remove_flag(outer, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(outer, LV_SCROLLBAR_MODE_OFF);
        outer = lv_obj_get_parent(outer);
        if (outer != NULL)
        {
            lv_obj_remove_flag(outer, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_scrollbar_mode(outer, LV_SCROLLBAR_MODE_OFF);
        }
    }
    lv_obj_set_style_bg_color(parent, lv_color_hex(0xf7fafb), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_70, 0);
    lv_obj_set_style_text_color(parent, lv_color_hex(0x17383d), 0);
    lv_obj_set_style_text_font(parent, &lv_font_myyh_20, 0);
    g_file_ui.path_label = lv_label_create(parent);
    lv_obj_set_pos(g_file_ui.path_label, 10, 12);
    lv_obj_set_size(g_file_ui.path_label, 360, 32);
    lv_label_set_long_mode(g_file_ui.path_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    button = file_ui_button(parent, "上级", 382, 6, 92, 42);
    lv_obj_add_event_cb(button, file_ui_parent_event, LV_EVENT_CLICKED, NULL);
    g_file_ui.open_button = file_ui_button(parent, "打开", 486, 6, 92, 42);
    lv_obj_add_event_cb(g_file_ui.open_button, file_ui_open_event, LV_EVENT_CLICKED, NULL);
    g_file_ui.list = lv_obj_create(parent);
    lv_obj_set_pos(g_file_ui.list, 10, 56);
    lv_obj_set_size(g_file_ui.list, 570, 396);
    lv_obj_set_scroll_dir(g_file_ui.list, LV_DIR_VER);
    lv_obj_set_flex_flow(g_file_ui.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_file_ui.list, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(g_file_ui.list, 6, 0);
    lv_obj_set_style_pad_row(g_file_ui.list, 4, 0);
    lv_obj_set_style_bg_color(g_file_ui.list, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_color(g_file_ui.list, lv_color_hex(0x9db9bd), 0);
    g_file_ui.prev_button = file_ui_button(parent, "上一页", 10, 462, 100, 42);
    lv_obj_add_event_cb(g_file_ui.prev_button, file_ui_prev_event, LV_EVENT_CLICKED, NULL);
    g_file_ui.page_label = lv_label_create(parent);
    lv_obj_set_pos(g_file_ui.page_label, 120, 472);
    lv_obj_set_size(g_file_ui.page_label, 250, 30);
    g_file_ui.next_button = file_ui_button(parent, "下一页", 480, 462, 100, 42);
    lv_obj_add_event_cb(g_file_ui.next_button, file_ui_next_event, LV_EVENT_CLICKED, NULL);
    g_file_ui.status_label = lv_label_create(parent);
    lv_obj_set_pos(g_file_ui.status_label, 280, 472);
    lv_obj_set_size(g_file_ui.status_label, 190, 30);
    lv_label_set_long_mode(g_file_ui.status_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    g_file_ui.refresh_pending = 1U;
}

void file_manager_ui_init(void)
{
    memset(&g_file_ui, 0, sizeof(g_file_ui));
    memset(g_clipboard_path, 0, sizeof(g_clipboard_path));
    g_clipboard_move = 0U;
    g_task_active = 0U;
    g_task_background = 0U;
    g_show_progress_pending = 0U;
    g_navigation_indicator = NULL;
    g_open_callback = NULL;
}

void file_manager_ui_set_open_callback(file_manager_ui_open_cb_t callback)
{ g_open_callback = callback; }

void file_manager_ui_request_open(void)
{ g_file_ui.open_pending = 1U; }

void file_manager_ui_request_readonly(lv_obj_t *parent)
{
    g_file_ui.readonly_parent = parent;
    g_file_ui.readonly_pending = 1U;
}

void file_manager_ui_poll(void)
{
    file_manager_status_t status;

    if (g_file_ui.open_pending != 0U)
    {
        g_file_ui.open_pending = 0U;
        file_ui_create();
        if ((g_show_progress_pending != 0U) && (g_task_active != 0U))
        {
            g_show_progress_pending = 0U;
            file_ui_show_progress();
        }
    }
    if (g_file_ui.readonly_pending != 0U)
    {
        lv_obj_t *parent = g_file_ui.readonly_parent;
        g_file_ui.readonly_pending = 0U;
        file_ui_create_readonly(parent);
    }
    if ((g_file_ui.screen != NULL) && (g_file_ui.refresh_pending != 0U))
        file_ui_refresh();
    if (g_file_ui.compact == 0U) file_ui_update_clock(0U);
    if (g_task_active == 0U) return;
    (void)file_manager_poll();
    memset(&status, 0, sizeof(status));
    file_manager_get_status(&status);
    file_ui_update_navigation_indicator(&status);
    if ((g_file_ui.progress_bar != NULL) && lv_obj_is_valid(g_file_ui.progress_bar))
    {
        lv_bar_set_value(g_file_ui.progress_bar, status.percent, LV_ANIM_OFF);
        lv_label_set_text_fmt(g_file_ui.progress_label, "%s  %u%%",
            status.current_name[0] ? status.current_name : "正在处理",
            (unsigned int)status.percent);
    }
    else if ((g_task_background != 0U) && (g_file_ui.status_label != NULL) &&
             lv_obj_is_valid(g_file_ui.status_label))
    {
        lv_label_set_text_fmt(g_file_ui.status_label, "文件操作  %u%%",
                              (unsigned int)status.percent);
    }
    if ((status.state == FILE_MANAGER_TASK_DONE) ||
        (status.state == FILE_MANAGER_TASK_CANCELLED) ||
        (status.state == FILE_MANAGER_TASK_ERROR))
    {
        g_task_active = 0U;
        g_task_background = 0U;
        if ((g_navigation_indicator != NULL) &&
            lv_obj_is_valid(g_navigation_indicator))
            lv_obj_delete(g_navigation_indicator);
        g_navigation_indicator = NULL;
        if ((status.state == FILE_MANAGER_TASK_DONE) && (g_clipboard_move != 0U))
        {
            g_clipboard_path[0] = '\0';
            g_clipboard_move = 0U;
        }
        file_ui_close_modal();
        if ((g_file_ui.screen != NULL) && lv_obj_is_valid(g_file_ui.screen))
        {
            g_file_ui.refresh_pending = 1U;
            file_ui_set_status((status.state == FILE_MANAGER_TASK_DONE) ?
                "操作完成" : ((status.state == FILE_MANAGER_TASK_CANCELLED) ?
                "操作已取消" : "文件操作失败"));
        }
    }
}
