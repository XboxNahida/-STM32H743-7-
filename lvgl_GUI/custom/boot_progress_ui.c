#include "boot_progress_ui.h"
#include <stdint.h>
#define BOOT_UI_SCREEN_WIDTH 1024
#define BOOT_UI_SCREEN_HEIGHT 600
#define BOOT_UI_LEFT_X 40
#define BOOT_UI_LEFT_Y 105
#define BOOT_UI_LEFT_WIDTH 528
#define BOOT_UI_LEFT_HEIGHT 390
#define BOOT_UI_RIGHT_X 584
#define BOOT_UI_RIGHT_Y 105
#define BOOT_UI_RIGHT_WIDTH 400
#define BOOT_UI_RIGHT_HEIGHT 390
#define BOOT_UI_ERROR_X 40
#define BOOT_UI_ERROR_Y 515
#define BOOT_UI_ERROR_WIDTH 944
#define BOOT_UI_ERROR_HEIGHT 40
#define BOOT_UI_PANEL_OPA 166
#define BOOT_UI_BAR_WIDTH 456
#define BOOT_UI_BAR_HEIGHT 18
#define BOOT_UI_DEVICE_COLUMNS 2
#define BOOT_UI_DEVICE_ROWS 5

#define BOOT_UI_COLOR_SUCCESS 0x35C46A
#define BOOT_UI_COLOR_WARNING 0xF2C94C
#define BOOT_UI_COLOR_ERROR 0xFF5A5F
#define BOOT_UI_COLOR_PENDING 0xB8C0CC
#define BOOT_UI_COLOR_PRIMARY 0xF5F7FA
#define BOOT_UI_COLOR_SECONDARY 0xD4DAE3
#define BOOT_UI_COLOR_BAR_BG 0x3A414C
#define BOOT_UI_COLOR_BAR_FILL 0x3A9DFF
#define BOOT_UI_ZH_SYSTEM "\xE7\xB3\xBB\xE7\xBB\x9F\xE5\x90\xAF\xE5\x8A\xA8"
#define BOOT_UI_ZH_PERIPHERALS "\xE5\xA4\x96\xE8\xAE\xBE\xE7\x8A\xB6\xE6\x80\x81"
#define BOOT_UI_ZH_PENDING "\xE7\xAD\x89\xE5\xBE\x85"
#define BOOT_UI_ZH_NO_ERROR "\xE6\x9A\x82\xE6\x97\xA0\xE5\x90\xAF\xE5\x8A\xA8\xE9\x94\x99\xE8\xAF\xAF"
#define BOOT_UI_ZH_SUCCESS "\xE6\x88\x90\xE5\x8A\x9F"
#define BOOT_UI_ZH_WARNING "\xE8\xAD\xA6\xE5\x91\x8A"
#define BOOT_UI_ZH_ERROR "\xE9\x94\x99\xE8\xAF\xAF"
static lv_obj_t *g_screen;
static lv_obj_t *g_wallpaper;
static lv_obj_t *g_title_label;
static lv_obj_t *g_stage_label;
static lv_obj_t *g_detail_label;
static lv_obj_t *g_percent_label;
static lv_obj_t *g_item_label;
static lv_obj_t *g_elapsed_label;
static lv_obj_t *g_progress_bar;
static lv_obj_t *g_device_title;
static lv_obj_t *g_device_labels[BOOT_DEVICE_COUNT];
static lv_obj_t *g_error_label;
static bool g_use_chinese;
static void set_geometry(lv_obj_t *object, int32_t x, int32_t y,
                         int32_t width, int32_t height)
{
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, height);
}
static void style_panel(lv_obj_t *panel)
{
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(panel, BOOT_UI_PANEL_OPA, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 8, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
}
static lv_obj_t *create_label(lv_obj_t *parent, int32_t x, int32_t y,
                              int32_t width, int32_t height,
                              uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    set_geometry(label, x, y, width, height);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(label, 0, 0);
    return label;
}
static uint32_t level_color(boot_progress_level_t level)
{
    if (level == BOOT_PROGRESS_SUCCESS)
    {
        return BOOT_UI_COLOR_SUCCESS;
    }
    if (level == BOOT_PROGRESS_WARNING)
    {
        return BOOT_UI_COLOR_WARNING;
    }
    return BOOT_UI_COLOR_ERROR;
}
static const char *level_text(boot_progress_level_t level)
{
    if (level == BOOT_PROGRESS_SUCCESS)
    {
        return g_use_chinese ? BOOT_UI_ZH_SUCCESS : "OK";
    }
    if (level == BOOT_PROGRESS_WARNING)
    {
        return g_use_chinese ? BOOT_UI_ZH_WARNING : "WARN";
    }
    return g_use_chinese ? BOOT_UI_ZH_ERROR : "ERROR";
}
static void reset_objects(void)
{
    uint32_t index;
    g_screen = NULL;
    g_wallpaper = NULL;
    g_title_label = NULL;
    g_stage_label = NULL;
    g_detail_label = NULL;
    g_percent_label = NULL;
    g_item_label = NULL;
    g_elapsed_label = NULL;
    g_progress_bar = NULL;
    g_device_title = NULL;
    g_error_label = NULL;
    g_use_chinese = false;
    for (index = 0U; index < (uint32_t)BOOT_DEVICE_COUNT; index++)
    {
        g_device_labels[index] = NULL;
    }
}
bool boot_progress_ui_create(void)
{
    lv_obj_t *left_panel;
    lv_obj_t *right_panel;
    uint32_t index;
    if ((g_screen != NULL) && lv_obj_is_valid(g_screen))
    {
        return true;
    }
    reset_objects();
    g_screen = lv_obj_create(NULL);
    if (g_screen == NULL)
    {
        return false;
    }
    set_geometry(g_screen, 0, 0, BOOT_UI_SCREEN_WIDTH, BOOT_UI_SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(g_screen, lv_color_hex(0x11151B), 0);
    lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_screen, 0, 0);
    lv_obj_set_style_pad_all(g_screen, 0, 0);
    lv_obj_remove_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE);
    g_wallpaper = lv_image_create(g_screen);
    set_geometry(g_wallpaper, 0, 0, BOOT_UI_SCREEN_WIDTH, BOOT_UI_SCREEN_HEIGHT);
    g_title_label = create_label(g_screen, 40, 34, 944, 44,
                                 BOOT_UI_COLOR_PRIMARY);
    lv_label_set_text(g_title_label, "SYSTEM STARTUP");
    left_panel = lv_obj_create(g_screen);
    set_geometry(left_panel, BOOT_UI_LEFT_X, BOOT_UI_LEFT_Y,
                 BOOT_UI_LEFT_WIDTH, BOOT_UI_LEFT_HEIGHT);
    style_panel(left_panel);
    g_stage_label = create_label(left_panel, 28, 24, 472, 34,
                                 BOOT_UI_COLOR_PRIMARY);
    g_detail_label = create_label(left_panel, 28, 68, 472, 52,
                                  BOOT_UI_COLOR_SECONDARY);
    g_progress_bar = lv_bar_create(left_panel);
    set_geometry(g_progress_bar, 36, 142, BOOT_UI_BAR_WIDTH,
                 BOOT_UI_BAR_HEIGHT);
    lv_bar_set_range(g_progress_bar, 0, 100);
    lv_obj_set_style_bg_color(g_progress_bar,
                              lv_color_hex(BOOT_UI_COLOR_BAR_BG), 0);
    lv_obj_set_style_bg_color(g_progress_bar,
                              lv_color_hex(BOOT_UI_COLOR_BAR_FILL),
                              LV_PART_INDICATOR);
    g_percent_label = create_label(left_panel, 28, 178, 472, 40,
                                   BOOT_UI_COLOR_PRIMARY);
    g_item_label = create_label(left_panel, 28, 238, 472, 54,
                                BOOT_UI_COLOR_SECONDARY);
    g_elapsed_label = create_label(left_panel, 28, 328, 472, 30,
                                   BOOT_UI_COLOR_SECONDARY);
    right_panel = lv_obj_create(g_screen);
    set_geometry(right_panel, BOOT_UI_RIGHT_X, BOOT_UI_RIGHT_Y,
                 BOOT_UI_RIGHT_WIDTH, BOOT_UI_RIGHT_HEIGHT);
    style_panel(right_panel);
    g_device_title = create_label(right_panel, 20, 20, 360, 34,
                                  BOOT_UI_COLOR_PRIMARY);
    lv_label_set_text(g_device_title, "PERIPHERALS");
    for (index = 0U; index < (uint32_t)BOOT_DEVICE_COUNT; index++)
    {
        int32_t column = (int32_t)(index / BOOT_UI_DEVICE_ROWS);
        int32_t row = (int32_t)(index % BOOT_UI_DEVICE_ROWS);

        g_device_labels[index] = create_label(
            right_panel, 20 + (column * 190), 66 + (row * 60), 178, 52,
            BOOT_UI_COLOR_PENDING);
        lv_label_set_text(g_device_labels[index], "Pending");
    }
    g_error_label = create_label(g_screen, BOOT_UI_ERROR_X, BOOT_UI_ERROR_Y,
                                 BOOT_UI_ERROR_WIDTH, BOOT_UI_ERROR_HEIGHT,
                                 BOOT_UI_COLOR_PENDING);
    lv_obj_set_style_bg_color(g_error_label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(g_error_label, BOOT_UI_PANEL_OPA, 0);
    lv_obj_set_style_pad_left(g_error_label, 14, 0);
    lv_obj_set_style_pad_top(g_error_label, 9, 0);
    lv_label_set_text(g_error_label, "No startup errors");
    lv_screen_load(g_screen);
    return true;
}
void boot_progress_ui_set_wallpaper(const lv_image_dsc_t *wallpaper)
{
    if ((g_wallpaper != NULL) && (wallpaper != NULL))
    {
        lv_image_set_src(g_wallpaper, wallpaper);
    }
}
void boot_progress_ui_set_font(const lv_font_t *font)
{
    uint32_t index;
    if ((g_screen == NULL) || (font == NULL))
    {
        return;
    }
    lv_obj_set_style_text_font(g_screen, font, 0);
    for (index = 0U; index < (uint32_t)BOOT_DEVICE_COUNT; index++)
    {
        lv_obj_set_style_text_font(g_device_labels[index], font, 0);
    }
}
void boot_progress_ui_set_chinese(bool enabled)
{
    g_use_chinese = enabled;
    if (g_title_label == NULL) { return; }
    lv_label_set_text(g_title_label, enabled ? BOOT_UI_ZH_SYSTEM : "SYSTEM STARTUP");
    lv_label_set_text(g_device_title, enabled ? BOOT_UI_ZH_PERIPHERALS : "PERIPHERALS");
    lv_label_set_text(g_error_label, enabled ? BOOT_UI_ZH_NO_ERROR : "No startup errors");
}
void boot_progress_ui_refresh(const boot_progress_snapshot_t *snapshot)
{
    uint32_t index;
    if ((snapshot == NULL) || (g_screen == NULL))
    {
        return;
    }
    lv_label_set_text(g_stage_label, snapshot->stage);
    lv_label_set_text(g_detail_label, snapshot->detail);
    lv_bar_set_value(g_progress_bar, snapshot->total_percent, LV_ANIM_OFF);
    lv_label_set_text_fmt(g_percent_label, g_use_chinese ?
                          "\xE6\x80\xBB\xE8\xBF\x9B\xE5\xBA\xA6 %u%%  \xE5\xBD\x93\xE5\x89\x8D %u%%" :
                          "%u%%  Item %u%%",
                          (unsigned int)snapshot->total_percent,
                          (unsigned int)snapshot->item_percent);
    lv_label_set_text_fmt(g_item_label, g_use_chinese ?
                          "\xE5\xB7\xB2\xE5\xAE\x8C\xE6\x88\x90 %lu \xE9\xA1\xB9  \xE4\xB8\x8A\xE4\xB8\x80\xE9\xA1\xB9: %s" :
                          "Completed %lu  Last: %s",
                          (unsigned long)snapshot->completed_items,
                          snapshot->last_item);
    lv_label_set_text_fmt(g_elapsed_label, g_use_chinese ?
                          "\xE7\x94\xA8\xE6\x97\xB6 %lu ms" : "Elapsed %lu ms",
                          (unsigned long)snapshot->elapsed_ms);
    for (index = 0U; index < (uint32_t)BOOT_DEVICE_COUNT; index++)
    {
        const boot_progress_device_t *device = &snapshot->devices[index];

        if (device->updated == 0U)
        {
            lv_obj_set_style_text_color(g_device_labels[index],
                                        lv_color_hex(BOOT_UI_COLOR_PENDING), 0);
            lv_label_set_text_fmt(g_device_labels[index], g_use_chinese ?
                                  "%s\n" BOOT_UI_ZH_PENDING : "%s\nPending",
                                  device->name);
        }
        else
        {
            lv_obj_set_style_text_color(g_device_labels[index],
                                        lv_color_hex(level_color(device->level)), 0);
            lv_label_set_text_fmt(g_device_labels[index], "%s\n%s %s",
                                  device->name, level_text(device->level),
                                  device->detail);
        }
    }

    if (snapshot->first_error_valid != 0U)
    {
        lv_obj_set_style_text_color(g_error_label,
                                    lv_color_hex(BOOT_UI_COLOR_ERROR), 0);
        lv_label_set_text_fmt(g_error_label, g_use_chinese ?
                              "\xE9\xA6\x96\xE4\xB8\xAA\xE9\x94\x99\xE8\xAF\xAF: %s" :
                              "FIRST ERROR: %s",
                              snapshot->first_error);
    }
    else
    {
        lv_obj_set_style_text_color(g_error_label,
                                    lv_color_hex(BOOT_UI_COLOR_PENDING), 0);
        lv_label_set_text(g_error_label, g_use_chinese ?
                          BOOT_UI_ZH_NO_ERROR : "No startup errors");
    }
    lv_refr_now(NULL);
}

void boot_progress_ui_destroy(void)
{
    if ((g_screen != NULL) && lv_obj_is_valid(g_screen))
    {
        lv_obj_delete(g_screen);
    }
    reset_objects();
}
