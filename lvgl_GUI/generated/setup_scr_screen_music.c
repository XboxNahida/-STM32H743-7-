/*
 * MUSIC APP USER PATCH
 * 独立音乐屏幕。GUI Guider 重新生成后需要按文档恢复此文件及字段。
 */

#include "lvgl.h"
#include "gui_guider.h"
#include "events_init.h"
#include "music_player_ui.h"

void setup_scr_screen_music(lv_ui *ui)
{
    ui->screen_music = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_music, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->screen_music, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(ui->screen_music, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->screen_music, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->screen_music, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui->screen_music, 0, LV_PART_MAIN);

    music_player_ui_attach(ui->screen_music);

    ui->screen_music_cont_navigation_bar = lv_obj_create(ui->screen_music);
    lv_obj_set_pos(ui->screen_music_cont_navigation_bar, 0, 0);
    lv_obj_set_size(ui->screen_music_cont_navigation_bar, 1024, 35);
    lv_obj_set_scrollbar_mode(ui->screen_music_cont_navigation_bar,
                              LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(ui->screen_music_cont_navigation_bar,
                            LV_OPA_COVER, LV_PART_MAIN);
    /* MUSIC APP USER PATCH: music screen and its top bar stay plain white. */
    lv_obj_set_style_bg_color(ui->screen_music_cont_navigation_bar,
                              lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->screen_music_cont_navigation_bar,
                                  0, LV_PART_MAIN);
    lv_obj_set_style_radius(ui->screen_music_cont_navigation_bar,
                            0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui->screen_music_cont_navigation_bar,
                             0, LV_PART_MAIN);

    ui->screen_music_btn_back = lv_button_create(
        ui->screen_music_cont_navigation_bar);
    lv_obj_set_pos(ui->screen_music_btn_back, 0, 0);
    lv_obj_set_size(ui->screen_music_btn_back, 48, 35);
    lv_obj_set_style_bg_opa(ui->screen_music_btn_back,
                            LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->screen_music_btn_back, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(ui->screen_music_btn_back, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(ui->screen_music_btn_back, 0, LV_PART_MAIN);
    ui->screen_music_btn_back_label = lv_label_create(ui->screen_music_btn_back);
    lv_label_set_text(ui->screen_music_btn_back_label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(ui->screen_music_btn_back_label,
                                lv_color_hex(0x504d6d), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->screen_music_btn_back_label,
                               &lv_font_montserratMedium_20, LV_PART_MAIN);
    lv_obj_center(ui->screen_music_btn_back_label);

    ui->screen_music_digital_clock = lv_label_create(
        ui->screen_music_cont_navigation_bar);
    lv_obj_set_pos(ui->screen_music_digital_clock, 52, 0);
    lv_obj_set_size(ui->screen_music_digital_clock, 100, 35);
    /* MUSIC APP USER PATCH: music pages show hour/minute only. */
    lv_label_set_text(ui->screen_music_digital_clock, "--:--");
    lv_obj_set_style_text_color(ui->screen_music_digital_clock,
                                lv_color_hex(0x504d6d), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->screen_music_digital_clock,
                               &lv_font_montserratMedium_20, LV_PART_MAIN);
    lv_obj_set_style_text_align(ui->screen_music_digital_clock,
                                LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);

    ui->screen_music_label_navigation_bar_flag = lv_label_create(
        ui->screen_music_cont_navigation_bar);
    lv_obj_set_pos(ui->screen_music_label_navigation_bar_flag, 462, -141);
    lv_obj_set_size(ui->screen_music_label_navigation_bar_flag, 100, 32);
    /* 空 label 复制笔记页模板，用于形成顶部容器的纵向滚动范围。 */
    lv_label_set_text(ui->screen_music_label_navigation_bar_flag, "");
    lv_label_set_long_mode(ui->screen_music_label_navigation_bar_flag,
                           LV_LABEL_LONG_WRAP);

    ui->screen_music_label_navigation_bar2 = lv_label_create(
        ui->screen_music_cont_navigation_bar);
    lv_obj_set_pos(ui->screen_music_label_navigation_bar2, 805, 2);
    lv_obj_set_size(ui->screen_music_label_navigation_bar2, 215, 31);
    lv_label_set_text(ui->screen_music_label_navigation_bar2,
                      "   " LV_SYMBOL_WIFI "  " LV_SYMBOL_VOLUME_MAX
                      " " LV_SYMBOL_USB " " LV_SYMBOL_BATTERY_FULL "  100%");
    lv_obj_set_style_text_color(ui->screen_music_label_navigation_bar2,
                                lv_color_hex(0x504d6d), LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->screen_music_label_navigation_bar2,
                               &lv_font_montserratMedium_20, LV_PART_MAIN);

    lv_obj_move_foreground(ui->screen_music_cont_navigation_bar);
    events_init_screen_music(ui);
}
