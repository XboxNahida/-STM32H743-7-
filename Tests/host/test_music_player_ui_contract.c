#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file;
    long length;
    char *text;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }
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

int main(void)
{
    char *ui_source = read_source("lvgl_GUI/custom/music_player_ui.c");
    char *ui_header = read_source("lvgl_GUI/custom/music_player_ui.h");
    char *visual_source = read_source(
        "lvgl_GUI/custom/music_player_visual.c");
    char *visual_header = read_source(
        "lvgl_GUI/custom/music_player_visual.h");
    char *demo_assets = read_source(
        "lvgl_GUI/custom/music_demo_assets.c");
    char *gui_music_image = read_source(
        "lvgl_GUI/generated/images/_music_RGB565A8_80x80.c");
    char *custom = read_source("lvgl_GUI/custom/custom.c");
    char *desktop = read_source("lvgl_GUI/generated/setup_scr_DeskTop.c");
    char *events = read_source("lvgl_GUI/generated/events_init.c");
    char *gui_header = read_source("lvgl_GUI/generated/gui_guider.h");
    char *gui_source = read_source("lvgl_GUI/generated/gui_guider.c");
    char *navigation = read_source(
        "lvgl_GUI/generated/setup_scr_screen_navigation_bar.c");
    char *music_service = read_source(
        "Core/Application/Src/music_player_service.c");
    char *music_service_header = read_source(
        "Core/Application/Inc/music_player_service.h");
    char *music_screen = read_source(
        "lvgl_GUI/generated/setup_scr_screen_music.c");
    char *widgets = read_source("lvgl_GUI/generated/widgets_init.c");
    int failures = 0;

    failures += expect_true("read_music_ui", ui_source != NULL);
    failures += expect_true("read_music_ui_header", ui_header != NULL);
    failures += expect_true("read_music_visual", visual_source != NULL);
    failures += expect_true("read_music_visual_header", visual_header != NULL);
    failures += expect_true("read_music_demo_assets", demo_assets != NULL);
    failures += expect_true("read_gui_music_image", gui_music_image != NULL);
    failures += expect_true("read_custom", custom != NULL);
    failures += expect_true("read_generated_desktop", desktop != NULL);
    failures += expect_true("read_generated_events", events != NULL);
    failures += expect_true("read_generated_gui_header", gui_header != NULL);
    failures += expect_true("read_generated_gui_source", gui_source != NULL);
    failures += expect_true("read_generated_navigation", navigation != NULL);
    failures += expect_true("read_music_service", music_service != NULL);
    failures += expect_true("read_music_service_header",
                            music_service_header != NULL);
    failures += expect_true("read_music_screen", music_screen != NULL);
    failures += expect_true("read_generated_widgets", widgets != NULL);
    if ((ui_source == NULL) || (ui_header == NULL) ||
        (visual_source == NULL) || (visual_header == NULL) ||
        (demo_assets == NULL) || (gui_music_image == NULL) ||
        (custom == NULL) ||
        (desktop == NULL) || (events == NULL) || (gui_header == NULL) ||
        (gui_source == NULL) ||
        (navigation == NULL) || (music_service == NULL) ||
        (music_screen == NULL) || (music_service_header == NULL) ||
        (widgets == NULL))
    {
        free(ui_source);
        free(ui_header);
        free(visual_source);
        free(visual_header);
        free(demo_assets);
        free(gui_music_image);
        free(custom);
        free(desktop);
        free(events);
        free(gui_header);
        free(gui_source);
        free(navigation);
        free(music_service);
        free(music_screen);
        free(music_service_header);
        free(widgets);
        return 1;
    }

    failures += expect_true(
        "generated_empty_navigation_label_preserved",
        strstr(desktop,
               "lv_label_set_text(ui->DeskTop_label_navigation_bar_flag, \"\");") != NULL);
    failures += expect_true(
        "generated_navigation_swipe_preserved",
        strstr(events, "&guider_ui.screen_navigation_bar") != NULL &&
        strstr(events, "LV_SCR_LOAD_ANIM_OVER_BOTTOM") != NULL);
    failures += expect_true(
        "generated_music_image_field",
        strstr(gui_header, "DeskTop_img_musicAPP") != NULL);
    failures += expect_true(
        "generated_music_label_field",
        strstr(gui_header, "DeskTop_label_musicAPP") != NULL);
    failures += expect_true(
        "generated_music_entry_on_second_page",
        strstr(desktop,
               "ui->DeskTop_img_musicAPP = lv_image_create(ui->DeskTop_tileview_DeskTop_tile_1);") != NULL);
    failures += expect_true("generated_music_entry_uses_gui_icon",
                            strstr(desktop,
                                   "&_music_RGB565A8_80x80") != NULL &&
                            strstr(desktop, "&music_icon_80") == NULL);
    failures += expect_true("generated_music_entry_event",
                            strstr(events,
                                   "DeskTop_img_musicAPP_event_handler") != NULL);
    failures += expect_true(
        "generated_music_entry_defers_screen_transition",
        strstr(events, "music_player_ui_request_open();") != NULL &&
        strstr(events, "music_player_ui_open();") == NULL);
    failures += expect_true("generated_patch_is_marked",
                            strstr(desktop, "MUSIC APP USER PATCH") != NULL &&
                            strstr(events, "MUSIC APP USER PATCH") != NULL);
    failures += expect_true(
        "music_uses_independent_screen",
        strstr(gui_header, "lv_obj_t *screen_music;") != NULL &&
        strstr(music_screen, "ui->screen_music = lv_obj_create(NULL);") != NULL &&
        strstr(ui_source, "g_desktop_root") == NULL);
    failures += expect_true(
        "music_screen_is_white_and_keeps_empty_swipe_label",
        strstr(music_screen, "lv_color_white()") != NULL &&
        strstr(music_screen,
               "lv_label_set_text(ui->screen_music_label_navigation_bar_flag, \"\");") != NULL);
    failures += expect_true(
        "music_list_supports_512_without_512_rows",
        strstr(music_service_header, "MUSIC_PLAYER_MAX_TRACKS 512U") != NULL &&
        strstr(ui_source, "MUSIC_UI_VISIBLE_TRACK_ROWS") != NULL &&
        strstr(ui_source,
               "g_track_buttons[MUSIC_UI_VISIBLE_TRACK_ROWS]") != NULL);
    failures += expect_true("ui_uses_original_demo_cover",
                            strstr(ui_source,
                                   "&img_lv_demo_music_cover_1") != NULL);
    failures += expect_true("ui_scans_real_sd_music",
                            strstr(ui_source, "music_player_scan()") != NULL);
    failures += expect_true("ui_reports_skipped_tracks",
                            strstr(ui_source, "status.skipped_count") != NULL);
    failures += expect_true("ui_has_previous",
                            strstr(ui_source, "music_player_previous()") != NULL);
    failures += expect_true("ui_has_toggle",
                            strstr(ui_source, "music_player_toggle()") != NULL);
    failures += expect_true("ui_has_next",
                            strstr(ui_source, "music_player_next()") != NULL);
    failures += expect_true("ui_volume_is_queued",
                            strstr(custom,
                                   "music_player_request_volume") != NULL);
    failures += expect_true(
        "ui_has_real_playback_progress",
        strstr(ui_source, "g_progress_slider") != NULL &&
        strstr(ui_source, "status->duration_ms") != NULL &&
        strstr(ui_source, "lv_slider_set_range(g_progress_slider") != NULL);
    failures += expect_true(
        "ui_scrolls_to_sd_track_page",
        strstr(ui_source, "lv_obj_set_scroll_snap_y") != NULL &&
        strstr(ui_source, "lv_obj_scroll_to_y") != NULL &&
        strstr(ui_source, "MUSIC_UI_PAGE_HEIGHT") != NULL);
    failures += expect_true(
        "visual_matches_original_round_controls",
        strstr(ui_source, "&img_lv_demo_music_btn_play") != NULL &&
        strstr(ui_source, "&img_lv_demo_music_btn_prev") != NULL &&
        strstr(ui_source, "&img_lv_demo_music_btn_next") != NULL &&
        strstr(visual_source, "music_visual_create_round_button") == NULL);
    failures += expect_true(
        "visual_has_animated_ring_spectrum",
        strstr(visual_header, "MUSIC_VISUAL_BAR_COUNT 20U") != NULL &&
        strstr(visual_header, "MUSIC_VISUAL_SPECTRUM_SIZE 250") != NULL &&
        strstr(visual_header, "MUSIC_VISUAL_INNER_RADIUS 77") != NULL &&
        strstr(visual_source, "lv_draw_triangle(layer") != NULL &&
        strstr(visual_source, "lv_arc_set_bg_angles") == NULL &&
        strstr(visual_source, "music_visual_update_spectrum") != NULL);
    failures += expect_true(
        "ui_matches_reference_small_grid_layout",
        strstr(ui_source, "MUSIC_UI_INTRO_TIME_MS 2000U") != NULL &&
        strstr(ui_source, "lv_obj_set_grid_dsc_array") != NULL &&
        strstr(ui_source, "MUSIC_UI_SPECTRUM_HEIGHT 250") != NULL &&
        strstr(ui_source, "MUSIC_UI_LANDSCAPE_LAYOUT") == NULL);
    failures += expect_true(
        "ui_has_reference_logo_intro",
        strstr(visual_source, "&img_lv_demo_music_logo") != NULL &&
        strstr(visual_header, "intro_bars_visible") != NULL &&
        strstr(ui_source, "music_visual_start_intro") != NULL &&
        strstr(demo_assets, "const lv_image_dsc_t img_lv_demo_music_logo") != NULL);
    failures += expect_true(
        "intro_logo_is_not_repositioned_by_player_grid",
        strstr(visual_source,
               "lv_obj_add_flag(visual->logo, LV_OBJ_FLAG_FLOATING);") != NULL &&
        strstr(visual_source,
               "lv_obj_add_flag(visual->logo_title, LV_OBJ_FLAG_FLOATING);") != NULL);
    failures += expect_true(
        "spectrum_follows_round_cover_without_rectangular_clipping",
        strstr(visual_source, "lv_image_get_scale(visual->cover)") != NULL &&
        strstr(visual_source, "MUSIC_VISUAL_MAX_RADIUS") != NULL &&
        strstr(visual_source,
               "outer_radius > MUSIC_VISUAL_MAX_RADIUS") != NULL &&
        strstr(visual_source, "code == LV_EVENT_DRAW_POST") != NULL &&
        strstr(visual_source, "code == LV_EVENT_DRAW_MAIN") != NULL &&
        strstr(visual_source,
               "(code == LV_EVENT_DRAW_MAIN) &&") != NULL);
    failures += expect_true(
        "player_has_plain_white_background_without_scaled_wallpaper",
        strstr(visual_source, "img_lv_demo_music_wave_top_large") == NULL &&
        strstr(visual_source, "img_lv_demo_music_wave_bottom_large") == NULL &&
        strstr(visual_source, "lv_image_set_scale(image, 548U)") == NULL);
    failures += expect_true(
        "ui_does_not_refresh_static_widgets_at_spectrum_rate",
        strstr(ui_source, "g_last_selected_index") != NULL &&
        strstr(ui_source, "track_changed") != NULL &&
        strstr(ui_source, "elapsed_second_changed") != NULL &&
        strstr(ui_source, "MUSIC_UI_ACTIVE_SYNC_INTERVAL_MS 33U") != NULL);
    failures += expect_true(
        "music_open_is_deferred_out_of_button_callback",
        strstr(ui_header, "void music_player_ui_request_open(void);") != NULL &&
        strstr(ui_source, "static uint8_t g_open_requested;") != NULL &&
        strstr(ui_source, "g_open_requested = 1U;") != NULL &&
        strstr(ui_source, "if (g_open_requested != 0U)") != NULL &&
        strstr(ui_source, "lv_display_get_screen_loading") != NULL &&
        strstr(ui_source, "active_screen != guider_ui.DeskTop") != NULL &&
        count_text(ui_source, "music_player_ui_open();") == 1U);
    failures += expect_true(
        "music_navigation_clock_has_no_seconds",
        strstr(music_screen,
               "lv_label_set_text(ui->screen_music_digital_clock, \"--:--\");") != NULL &&
        strstr(music_screen, "--:--:--") == NULL &&
        strstr(events, "screen_music_navigation_event_handler") != NULL);
    failures += expect_true(
        "navigation_records_all_supported_source_screens",
        strstr(events,
               "navigation_set_return_target(NAV_RETURN_DESKTOP)") != NULL &&
        strstr(events,
               "navigation_set_return_target(NAV_RETURN_NOTES)") != NULL &&
        strstr(events,
               "navigation_set_return_target(NAV_RETURN_SIGNAL_PROC)") != NULL &&
        strstr(events,
               "navigation_set_return_target(NAV_RETURN_YUANSHEN)") != NULL &&
        strstr(events,
               "navigation_set_return_target(NAV_RETURN_MUSIC)") != NULL);
    failures += expect_true(
        "unlock_and_navigation_swipe_restore_saved_screen",
        strstr(events,
               "navigation_restore_saved_screen(&guider_ui.lock_screen_del") != NULL &&
        strstr(events,
               "navigation_restore_saved_screen(&guider_ui.screen_navigation_bar_del") != NULL &&
        strstr(events,
               "&guider_ui.DeskTop, guider_ui.DeskTop_del, &guider_ui.lock_screen_del") == NULL);
    failures += expect_true(
        "screen_transition_reentry_is_blocked_before_cleanup",
        strstr(gui_source, "lv_display_get_screen_loading") != NULL &&
        strstr(gui_source, "if (loading_screen != NULL)") != NULL &&
        strstr(gui_source, "lv_obj_clean(act_scr)") == NULL &&
        strstr(gui_source, "ui_clear_deleted_screen_reference") != NULL &&
        strstr(gui_source,
               "lv_obj_add_event_cb(act_scr, ui_clear_deleted_screen_reference") != NULL);
    failures += expect_true(
        "music_navigation_bar_is_white_without_wallpaper_color",
        strstr(music_screen,
               "lv_obj_set_style_bg_color(ui->screen_music_cont_navigation_bar,") != NULL &&
        strstr(music_screen, "lv_color_white(), LV_PART_MAIN);") != NULL &&
        strstr(music_screen, "lv_color_hex(0x31558a)") == NULL);
    failures += expect_true(
        "rtc_updates_only_objects_owned_by_active_screen",
        strstr(widgets, "rtc_object_on_active_screen") != NULL &&
        strstr(widgets,
               "rtc_object_on_active_screen(guider_ui.lock_screen_digital_clock_1,") != NULL &&
        strstr(widgets,
               "rtc_object_on_active_screen(guider_ui.DeskTop_digital_clock_1,") != NULL &&
        strstr(widgets,
               "guider_ui.screen_navigation_bar_digital_clock_1,") != NULL &&
        strstr(widgets,
               "rtc_object_on_active_screen(guider_ui.screen_music_digital_clock,") != NULL &&
        strstr(widgets,
               "if (lv_obj_is_valid(guider_ui.DeskTop_digital_clock_1))") == NULL);
    failures += expect_true(
        "ui_uses_original_demo_assets",
        strstr(ui_source, "&img_lv_demo_music_btn_play") != NULL &&
        strstr(ui_source, "&img_lv_demo_music_slider_knob") != NULL &&
        strstr(visual_source, "&img_lv_demo_music_logo") != NULL &&
        strstr(demo_assets,
               "const lv_image_dsc_t img_lv_demo_music_cover_1") != NULL);
    failures += expect_true(
        "gui_music_image_armcc_include",
        strstr(gui_music_image, "MUSIC APP USER PATCH") != NULL &&
        strstr(gui_music_image, "#include \"lvgl/lvgl.h\"") == NULL);
    failures += expect_true(
        "sd_track_names_use_font20",
        strstr(ui_source,
               "lv_obj_set_style_text_font(label, &lv_font_myyh_20") != NULL);
    failures += expect_true(
        "navbar_track_name_uses_font20",
        strstr(navigation,
               "screen_navigation_bar_label_9, &lv_font_myyh_20") != NULL);
    failures += expect_true(
        "music_sd_owner_does_not_collide_with_avi",
        strstr(events, "#define AVI_SD_SESSION_OWNER 0x0504U") != NULL &&
        strstr(music_service, "#define MUSIC_PLAYER_SD_OWNER 0x0505U") != NULL);

    failures += expect_true(
        "navbar_music_icon_opens_app",
        strstr(ui_source, "screen_navigation_bar_btn_10") != NULL &&
        strstr(ui_source, "music_player_ui_open") != NULL);
    failures += expect_true(
        "navbar_previous_connected",
        strstr(ui_source, "screen_navigation_bar_btn_11") != NULL);
    failures += expect_true(
        "navbar_next_connected",
        strstr(ui_source, "screen_navigation_bar_btn_12") != NULL);
    failures += expect_true(
        "navbar_toggle_connected",
        strstr(ui_source, "screen_navigation_bar_btn_14") != NULL);
    failures += expect_true(
        "navbar_title_synchronized",
        strstr(ui_source, "screen_navigation_bar_label_9") != NULL);

    failures += expect_true(
        "custom_initializes_music",
        strstr(custom, "music_player_ui_init();") != NULL);
    failures += expect_true(
        "custom_polls_music",
        strstr(custom, "music_player_ui_poll(HAL_GetTick());") != NULL);
    failures += expect_true(
        "old_volume_direct_write_removed",
        strstr(custom, "audio_set_volume_percent((uint8_t)value)") == NULL);

    free(ui_source);
    free(ui_header);
    free(visual_source);
    free(visual_header);
    free(demo_assets);
    free(gui_music_image);
    free(custom);
    free(desktop);
    free(events);
    free(gui_header);
    free(gui_source);
    free(navigation);
    free(music_service);
    free(music_screen);
    free(music_service_header);
    free(widgets);
    if (failures != 0)
    {
        printf("FAIL music_player_ui_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS music_player_ui_contract\n");
    return 0;
}
