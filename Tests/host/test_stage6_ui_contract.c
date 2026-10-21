#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file;
    long length;
    char *text;

    file = fopen(path, "rb");
    if (file == NULL) return NULL;
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        (void)fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)length + 1U);
    if (text == NULL)
    {
        (void)fclose(file);
        return NULL;
    }
    if (fread(text, 1U, (size_t)length, file) != (size_t)length)
    {
        free(text);
        (void)fclose(file);
        return NULL;
    }
    text[length] = '\0';
    (void)fclose(file);
    return text;
}

static int expect_true(const char *name, int condition)
{
    if (condition == 0)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

static unsigned int count_text(const char *text, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);

    while ((text = strstr(text, needle)) != NULL)
    {
        count++;
        text += length;
    }
    return count;
}

static int desktop_label_uses_font_20(const char *desktop,
                                      const char *label_name)
{
    char font[160];

    (void)snprintf(font, sizeof(font),
                   "lv_obj_set_style_text_font(ui->%s, &lv_font_myyh_20",
                   label_name);
    return strstr(desktop, font) != NULL;
}

int main(void)
{
    char *header = read_source("lvgl_GUI/generated/gui_guider.h");
    char *lifetime = read_source("lvgl_GUI/generated/gui_guider.c");
    char *desktop = read_source("lvgl_GUI/generated/setup_scr_DeskTop.c");
    char *events = read_source("lvgl_GUI/generated/events_init.c");
    char *widgets = read_source("lvgl_GUI/generated/widgets_init.c");
    char *calculator = read_source(
        "lvgl_GUI/generated/setup_scr_screen_calculator.c");
    char *piano = read_source("lvgl_GUI/generated/setup_scr_screen_piano.c");
    char *drawing = read_source(
        "lvgl_GUI/generated/setup_scr_screen_drawing.c");
    char *music = read_source(
        "lvgl_GUI/generated/setup_scr_screen_music.c");
    char *image_viewer = read_source(
        "lvgl_GUI/generated/setup_scr_screen_image_viewer.c");
    char *avi_player = read_source(
        "lvgl_GUI/generated/setup_scr_screen_avi_player.c");
    char *project = read_source("MDK-ARM/H743_LCD_Keil5.uvprojx");
    char *draw_icon = read_source(
        "lvgl_GUI/generated/images/_draw_RGB565A8_80x80.c");
    char *piano_icon = read_source(
        "lvgl_GUI/generated/images/_piano_RGB565A8_80x80.c");
    char *nes_icon = read_source(
        "lvgl_GUI/generated/images/_steam_RGB565A8_80x80.c");
    int failures = 0;

    failures += expect_true("read_header", header != NULL);
    failures += expect_true("read_lifetime", lifetime != NULL);
    failures += expect_true("read_desktop", desktop != NULL);
    failures += expect_true("read_events", events != NULL);
    failures += expect_true("read_widgets", widgets != NULL);
    failures += expect_true("read_calculator", calculator != NULL);
    failures += expect_true("read_piano", piano != NULL);
    failures += expect_true("read_drawing", drawing != NULL);
    failures += expect_true("read_music", music != NULL);
    failures += expect_true("read_image_viewer", image_viewer != NULL);
    failures += expect_true("read_avi_player", avi_player != NULL);
    failures += expect_true("read_project", project != NULL);
    failures += expect_true("read_draw_icon", draw_icon != NULL);
    failures += expect_true("read_piano_icon", piano_icon != NULL);
    failures += expect_true("read_nes_icon", nes_icon != NULL);
    if ((header == NULL) || (lifetime == NULL) || (desktop == NULL) ||
        (events == NULL) || (widgets == NULL) || (calculator == NULL) || (piano == NULL) ||
        (drawing == NULL) || (music == NULL) ||
        (image_viewer == NULL) || (avi_player == NULL) ||
        (project == NULL) || (draw_icon == NULL) ||
        (piano_icon == NULL) || (nes_icon == NULL))
    {
        free(header); free(lifetime); free(desktop); free(events); free(widgets);
        free(calculator); free(piano); free(drawing); free(music);
        free(image_viewer); free(avi_player); free(project);
        free(draw_icon); free(piano_icon); free(nes_icon);
        return 1;
    }

    failures += expect_true("calculator_screen",
        strstr(calculator, "ui->screen_calculator = lv_obj_create(NULL);") != NULL);
    failures += expect_true("calculator_entry_opens_screen",
        strstr(events, "setup_scr_screen_calculator") != NULL);
    failures += expect_true("calculator_no_alarm_side_effect",
        strstr(events, "STAGE6 UI USER PATCH: calculator entry") != NULL);
    failures += expect_true("desktop_draw_icon",
        strstr(desktop, "&_draw_RGB565A8_80x80") != NULL);
    failures += expect_true("desktop_piano_icon",
        strstr(desktop, "&_piano_RGB565A8_80x80") != NULL);
    failures += expect_true("desktop_nes_entry",
        strstr(header, "DeskTop_img_nes") != NULL &&
        strstr(header, "DeskTop_label_nes") != NULL &&
        strstr(desktop, "&_steam_RGB565A8_80x80") != NULL &&
        strstr(desktop, "lv_label_set_text(ui->DeskTop_label_nes, \"小游戏\")") != NULL &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_nes"));
    failures += expect_true("nes_icon_keil_include",
        strstr(nes_icon, "#include \"lvgl.h\"") != NULL &&
        strstr(nes_icon, "#include \"lvgl/lvgl.h\"") == NULL);
    failures += expect_true("desktop_app_names_font_20",
        desktop_label_uses_font_20(desktop, "DeskTop_label_11") &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_12") &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_13") &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_14") &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_15") &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_16") &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_17") &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_18") &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_19") &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_20") &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_musicAPP") &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_draw") &&
        desktop_label_uses_font_20(desktop, "DeskTop_label_piano"));
    failures += expect_true("piano_two_modes",
        strstr(piano, "screen_piano_keys_panel") != NULL &&
        strstr(piano, "screen_piano_pads_panel") != NULL);
    failures += expect_true("piano_uses_note_names",
        strstr(piano, "\"Do\"") != NULL &&
        strstr(piano, "\"Sol\"") != NULL &&
        strstr(piano, "\"Xi\"") != NULL &&
        strstr(piano, "\"15\"") == NULL);
    failures += expect_true("piano_has_14_white_keys",
        strstr(piano, "STAGE6_PIANO_WHITE_KEY_COUNT 14U") != NULL);
    failures += expect_true("piano_has_10_black_keys",
        strstr(piano, "STAGE6_PIANO_BLACK_KEY_COUNT 10U") != NULL);
    failures += expect_true("piano_structural_containers_do_not_scroll",
        strstr(piano,
            "lv_obj_remove_flag(ui->screen_piano, LV_OBJ_FLAG_SCROLLABLE);") != NULL &&
        strstr(piano,
            "lv_obj_remove_flag(ui->screen_piano_cont_1, LV_OBJ_FLAG_SCROLLABLE);") != NULL &&
        strstr(piano,
            "lv_obj_remove_flag(ui->screen_piano_keys_panel, LV_OBJ_FLAG_SCROLLABLE);") != NULL &&
        strstr(piano,
            "lv_obj_remove_flag(ui->screen_piano_pads_panel, LV_OBJ_FLAG_SCROLLABLE);") != NULL &&
        strstr(piano,
            "lv_obj_remove_flag(ui->screen_piano_control_panel, LV_OBJ_FLAG_SCROLLABLE);") != NULL);
    failures += expect_true("piano_control_panel_has_no_layout_padding",
        strstr(piano,
            "lv_obj_set_style_pad_all(ui->screen_piano_control_panel, 0,") != NULL);
    failures += expect_true("drawing_has_canvas",
        strstr(drawing, "screen_drawing_canvas") != NULL);
    failures += expect_true("drawing_has_tools",
        strstr(drawing, "screen_drawing_width_slider") != NULL &&
        strstr(drawing, "screen_drawing_save_btn") != NULL);
    failures += expect_true("drawing_actions_clear_default_padding",
        strstr(drawing,
            "lv_obj_set_style_pad_all(ui->screen_drawing_actions, 0,") != NULL);
    failures += expect_true("navigation_sources",
        strstr(events, "NAV_RETURN_CALCULATOR") != NULL &&
        strstr(events, "NAV_RETURN_PIANO") != NULL &&
        strstr(events, "NAV_RETURN_DRAWING") != NULL);
    failures += expect_true("navigation_uses_notes_scroll_template",
        strstr(events,
            "STAGE6 UI USER PATCH: notes-template container scroll") != NULL &&
        strstr(events,
            "MUSIC APP USER PATCH: notes-template container scroll") != NULL &&
        strstr(events, "stage6_navigation_swiped_down") == NULL);
    failures += expect_true("image_viewer_clock_updates_from_rtc",
        strstr(widgets,
            "guider_ui.screen_image_viewer_digital_clock_2,\n            guider_ui.screen_image_viewer))") != NULL &&
        strstr(widgets,
            "guider_ui.screen_image_viewer_digital_clock_2,\n            \"%d:%02d\"") != NULL);
    failures += expect_true("avi_player_clock_updates_from_rtc",
        strstr(widgets,
            "guider_ui.screen_avi_player_digital_clock_2,\n            guider_ui.screen_avi_player))") != NULL &&
        strstr(widgets,
            "guider_ui.screen_avi_player_digital_clock_2,\n            \"%d:%02d\"") != NULL);
    failures += expect_true("image_viewer_navigation_geometry",
        strstr(image_viewer,
            "lv_obj_set_pos(ui->screen_image_viewer_digital_clock_2, 700, 5);") != NULL &&
        strstr(image_viewer,
            "lv_obj_set_pos(ui->screen_image_viewer_label_2, 55, 5);") != NULL &&
        strstr(image_viewer,
            "lv_obj_set_size(ui->screen_image_viewer_label_2, 635, 30);") != NULL);
    failures += expect_true("avi_player_navigation_geometry",
        strstr(avi_player,
            "lv_obj_set_pos(ui->screen_avi_player_digital_clock_2, 700, 5);") != NULL &&
        strstr(avi_player,
            "lv_obj_set_pos(ui->screen_avi_player_label_2, 55, 5);") != NULL &&
        strstr(avi_player,
            "lv_obj_set_pos(ui->screen_avi_player_label_3, 450, 5);") != NULL &&
        strstr(avi_player, "screen_avi_player_status_label") != NULL &&
        strstr(avi_player,
            "lv_obj_set_pos(screen_avi_player_status_label, 810, 5);") != NULL);
    failures += expect_true("desktop_calendar_syncs_from_rtc",
        strstr(widgets,
            "lv_calendar_get_today_date(\n            guider_ui.DeskTop_calendar_1)") != NULL &&
        strstr(widgets,
            "lv_calendar_set_today_date(guider_ui.DeskTop_calendar_1") != NULL &&
        strstr(widgets,
            "lv_calendar_set_showed_date(guider_ui.DeskTop_calendar_1") != NULL &&
        strstr(widgets,
            "DeskTop_calendar_1_highlihted_days, 0U);") != NULL &&
        strstr(widgets,
            "lv_calendar_set_highlighted_dates(guider_ui.DeskTop_calendar_1,\n                                              NULL, 0U);") == NULL);
    failures += expect_true("music_has_scroll_sentinel",
        strstr(music,
            "lv_obj_set_pos(ui->screen_music_label_navigation_bar_flag, 462, -141);") != NULL &&
        strstr(music,
            "lv_label_set_long_mode(ui->screen_music_label_navigation_bar_flag,") != NULL);
    failures += expect_true("stage6_sentinels_match_notes_template",
        strstr(piano,
            "lv_label_set_long_mode(ui->screen_piano_label_1,") != NULL &&
        strstr(drawing,
            "lv_label_set_long_mode(ui->screen_drawing_label_1,") != NULL);
    failures += expect_true("screen_lifetime",
        strstr(lifetime, "ui->screen_calculator == screen") != NULL &&
        strstr(lifetime, "ui->screen_piano == screen") != NULL &&
        strstr(lifetime, "ui->screen_drawing == screen") != NULL);
    failures += expect_true("generated_patch_marked",
        strstr(desktop, "STAGE6 UI USER PATCH") != NULL &&
        strstr(events, "STAGE6 UI USER PATCH") != NULL);
    failures += expect_true("keil_calculator_once",
        count_text(project, "setup_scr_screen_calculator.c") == 2U);
    failures += expect_true("keil_piano_once",
        count_text(project, "setup_scr_screen_piano.c") == 2U);
    failures += expect_true("keil_drawing_once",
        count_text(project, "setup_scr_screen_drawing.c") == 2U);
    failures += expect_true("keil_draw_icon_once",
        count_text(project, "_draw_RGB565A8_80x80.c") == 2U);
    failures += expect_true("keil_piano_icon_once",
        count_text(project, "_piano_RGB565A8_80x80.c") == 2U);
    failures += expect_true("keil_nes_icon_once",
        count_text(project, "_steam_RGB565A8_80x80.c") == 2U);

    free(header); free(lifetime); free(desktop); free(events); free(widgets);
    free(calculator); free(piano); free(drawing); free(music);
    free(image_viewer); free(avi_player); free(project);
    free(draw_icon); free(piano_icon); free(nes_icon);
    if (failures != 0)
    {
        printf("stage6_ui_contract failures=%d\n", failures);
        return 1;
    }
    puts("PASS stage6_ui_contract");
    return 0;
}
