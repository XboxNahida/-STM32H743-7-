/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "widgets_init.h"
/* STAGE6 NOTES USER PATCH: restore after GUI Guider regeneration. */
#include "selection_keyboard.h"

#if LV_USE_IME_PINYIN
lv_obj_t *g_kb_ime = NULL;
#endif

void ui_init_style(lv_style_t * style)
{
    if (style->prop_cnt > 1)
        lv_style_reset(style);
    else
        lv_style_init(style);
}

/* SCREEN LIFETIME USER PATCH BEGIN */
static void ui_clear_deleted_screen_reference(lv_event_t *event)
{
    lv_ui *ui = (lv_ui *)lv_event_get_user_data(event);
    lv_obj_t *screen = lv_event_get_target(event);

    if ((ui == NULL) || (screen == NULL)) return;
    if (ui->lock_screen == screen) ui->lock_screen = NULL;
    if (ui->DeskTop == screen) ui->DeskTop = NULL;
    if (ui->screen_navigation_bar == screen)
        ui->screen_navigation_bar = NULL;
    if (ui->screen_notes == screen) ui->screen_notes = NULL;
    if (ui->screen_Signal_Proc == screen) ui->screen_Signal_Proc = NULL;
    if (ui->screen_yuanshen == screen) ui->screen_yuanshen = NULL;
    if (ui->screen_camera == screen) ui->screen_camera = NULL;
    /* STAGE6 UI USER PATCH BEGIN: clear deleted application screens. */
    if (ui->screen_calculator == screen) ui->screen_calculator = NULL;
    if (ui->screen_piano == screen) ui->screen_piano = NULL;
    if (ui->screen_drawing == screen) ui->screen_drawing = NULL;
    /* STAGE6 UI USER PATCH END */
    if (ui->screen_image_viewer == screen) ui->screen_image_viewer = NULL;
    if (ui->screen_avi_player == screen) ui->screen_avi_player = NULL;
    if (ui->screen_music == screen) ui->screen_music = NULL;
}
/* SCREEN LIFETIME USER PATCH END */

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del)
{
    lv_obj_t * act_scr = lv_screen_active();
    lv_display_t *display = lv_display_get_default();
    lv_obj_t *loading_screen = (display != NULL) ?
                               lv_display_get_screen_loading(display) : NULL;

    /* SCREEN LIFETIME USER PATCH: never force-complete an active load. */
    if (loading_screen != NULL) {
        return;
    }

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "gg_external_data.h"

    if(auto_del) {
        gg_edata_task_clear(act_scr);
    }
#endif
    (void)is_clean;
    if ((auto_del != false) && (act_scr != NULL)) {
        /* Keep children alive until SCREEN_UNLOADED; event callbacks may
         * still be using the current target while this function runs. */
        lv_obj_add_event_cb(act_scr, ui_clear_deleted_screen_reference,
                            LV_EVENT_DELETE, ui);
    }
    if (new_scr_del) {
        setup_scr(ui);
    }
    lv_screen_load_anim(*new_scr, anim_type, time, delay, auto_del);
    *old_scr_del = auto_del;
}

void ui_animation(void * var, uint32_t duration, int32_t delay, int32_t start_value, int32_t end_value, lv_anim_path_cb_t path_cb,
                  uint32_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                  lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb, lv_anim_completed_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb)
{
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, var);
    lv_anim_set_exec_cb(&anim, exec_cb);
    lv_anim_set_values(&anim, start_value, end_value);
    lv_anim_set_time(&anim, duration);
    lv_anim_set_delay(&anim, delay);
    lv_anim_set_path_cb(&anim, path_cb);
    lv_anim_set_repeat_count(&anim, repeat_cnt);
    lv_anim_set_repeat_delay(&anim, repeat_delay);
    lv_anim_set_playback_time(&anim, playback_time);
    lv_anim_set_playback_delay(&anim, playback_delay);
    if (start_cb) {
        lv_anim_set_start_cb(&anim, start_cb);
    }
    if (ready_cb) {
        lv_anim_set_completed_cb(&anim, ready_cb);
    }
    if (deleted_cb) {
        lv_anim_set_deleted_cb(&anim, deleted_cb);
    }
    lv_anim_start(&anim);
}

void init_scr_del_flag(lv_ui *ui)
{

    ui->lock_screen_del = true;
    ui->DeskTop_del = true;
    ui->screen_navigation_bar_del = true;
    ui->screen_notes_del = true;
    ui->screen_Signal_Proc_del = true;
    ui->screen_yuanshen_del = true;
    ui->screen_camera_del = true;
    /* STAGE6 UI USER PATCH BEGIN: independent application screens. */
    ui->screen_calculator_del = true;
    ui->screen_piano_del = true;
    ui->screen_drawing_del = true;
    /* STAGE6 UI USER PATCH END */
    ui->screen_image_viewer_del = true;
    ui->screen_avi_player_del = true;
    /* MUSIC APP USER PATCH BEGIN: independent music screen */
    ui->screen_music_del = true;
    /* MUSIC APP USER PATCH END */
}

void setup_bottom_layer(void)
{
    lv_theme_apply(lv_layer_bottom());
}

void setup_ui(lv_ui *ui)
{
    setup_bottom_layer();
    init_scr_del_flag(ui);
    init_keyboard(ui);
    setup_scr_lock_screen(ui);
    lv_screen_load(ui->lock_screen);
}

void video_play(lv_ui *ui)
{
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_avi_player,
        guider_ui.screen_avi_player_del, &guider_ui.DeskTop_del,
        setup_scr_screen_avi_player, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 200, true, true);
}

void init_keyboard(lv_ui *ui)
{
#if LV_USE_IME_PINYIN
    extern lv_pinyin_dict_t gg_pinyin_dict[];
    lv_obj_t * ime;
    lv_obj_t * cand;
#endif

    ui->g_kb_top_layer = lv_keyboard_create(lv_layer_top());
    lv_obj_set_size(ui->g_kb_top_layer, 1024, 245);  /* keyboard: 1024x245, btn~61px */
    lv_obj_add_flag(ui->g_kb_top_layer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_font(ui->g_kb_top_layer, &lv_font_montserratMedium_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    /* STAGE6 NOTES USER PATCH: capture the normal font, then install once. */
    (void)selection_keyboard_install(ui->g_kb_top_layer);
    lv_obj_add_event_cb(ui->g_kb_top_layer, kb_event_cb, LV_EVENT_ALL, NULL);

#if LV_USE_IME_PINYIN
    /* Attach pinyin IME so ALL keyboards get Chinese candidate panel */
    ime = lv_ime_pinyin_create(lv_layer_top());
    lv_ime_pinyin_set_keyboard(ime, ui->g_kb_top_layer);
    lv_ime_pinyin_set_dict(ime, gg_pinyin_dict);
    /* GLOBAL IME USER PATCH: direct English is the default input mode. */
    lv_ime_pinyin_set_mode(ime, LV_IME_PINYIN_MODE_K26);
    lv_ime_pinyin_set_language(ime, LV_IME_PINYIN_LANGUAGE_ENGLISH);
    /* Set Chinese font on candidate panel */
    cand = lv_ime_pinyin_get_cand_panel(ime);

    lv_obj_set_height(cand, 58);
    lv_obj_set_style_text_font(cand, &lv_font_myyh_20, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cand, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cand, lv_color_hex(0xf5f7f9), LV_PART_MAIN);
    lv_obj_set_style_border_width(cand, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cand, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(cand, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(cand, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(cand, &lv_font_myyh_20,
                               LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(cand, lv_color_hex(0x202428),
                                LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cand, LV_OPA_COVER,
                            LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cand, lv_color_hex(0xe7ebef),
                              LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cand, 1,
                                  LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(cand, lv_color_hex(0xc7cdd3),
                                  LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cand, 5, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cand, lv_color_hex(0xd5dce3),
                              LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(cand, LV_OPA_COVER,
                            LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(cand, lv_color_hex(0x2195f6),
                              LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(cand, LV_OPA_COVER,
                            LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(cand, lv_color_hex(0xffffff),
                                LV_PART_ITEMS | LV_STATE_CHECKED);
    /* GLOBAL IME USER PATCH END */
    /* Store for kb_event_cb to toggle mode */
    g_kb_ime = ime;
#endif
}
