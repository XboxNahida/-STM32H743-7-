/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"


typedef struct
{
  
	lv_obj_t *lock_screen;
	bool lock_screen_del;
	lv_obj_t *lock_screen_cont_lock;
	lv_obj_t *lock_screen_digital_clock_1;
	lv_obj_t *lock_screen_datetext_1;
	lv_obj_t *lock_screen_label_lock_flag;
	lv_obj_t *lock_screen_img_1;
	lv_obj_t *lock_screen_label_navigation_bar1;
	lv_obj_t *DeskTop;
	bool DeskTop_del;
	lv_obj_t *DeskTop_tileview_DeskTop;
	lv_obj_t *DeskTop_tileview_DeskTop_tile;
	lv_obj_t *DeskTop_tileview_DeskTop_tile_1;
	lv_obj_t *DeskTop_img_noteAPP;
	lv_obj_t *DeskTop_label_8;
	lv_obj_t *DeskTop_label_7;
	lv_obj_t *DeskTop_calendar_1;
	lv_obj_t *DeskTop_img_settingAPP;
	lv_obj_t *DeskTop_analog_clock_1;
	lv_obj_t *DeskTop_analog_clock_1_hour_needle;
	lv_obj_t *DeskTop_analog_clock_1_min_needle;
	lv_obj_t *DeskTop_analog_clock_1_sec_needle;
	lv_obj_t *DeskTop_datetext_1;
	lv_obj_t *DeskTop_digital_clock_2;
	lv_obj_t *DeskTop_img_wetherAPP;
	lv_obj_t *DeskTop_img_edgeAPP;
	lv_obj_t *DeskTop_img_calendarAPP;
	lv_obj_t *DeskTop_img_albumAPP;
	lv_obj_t *DeskTop_label_11;
	lv_obj_t *DeskTop_label_12;
	lv_obj_t *DeskTop_label_13;
	lv_obj_t *DeskTop_label_14;
	lv_obj_t *DeskTop_label_15;
	lv_obj_t *DeskTop_label_16;
	lv_obj_t *DeskTop_label_10;
	lv_obj_t *DeskTop_label_9;
	lv_obj_t *DeskTop_img_yuanshenAPP;
	lv_obj_t *DeskTop_img_alarmAPP;
	lv_obj_t *DeskTop_img_calculatorAPP;
	lv_obj_t *DeskTop_img_folderAPP;
	/* MUSIC APP USER PATCH BEGIN: desktop page 2 generated-style entry */
	lv_obj_t *DeskTop_img_musicAPP;
	/* MUSIC APP USER PATCH END */
	/* STAGE6 UI USER PATCH BEGIN: desktop page 2 entries */
	lv_obj_t *DeskTop_img_draw;
	lv_obj_t *DeskTop_img_piano;
	lv_obj_t *DeskTop_img_nes;
	/* STAGE6 UI USER PATCH END */
	lv_obj_t *DeskTop_label_17;
	lv_obj_t *DeskTop_label_18;
	lv_obj_t *DeskTop_label_19;
	lv_obj_t *DeskTop_label_20;
	/* MUSIC APP USER PATCH BEGIN: desktop page 2 generated-style label */
	lv_obj_t *DeskTop_label_musicAPP;
	/* MUSIC APP USER PATCH END */
	/* STAGE6 UI USER PATCH BEGIN: desktop page 2 labels */
	lv_obj_t *DeskTop_label_draw;
	lv_obj_t *DeskTop_label_piano;
	lv_obj_t *DeskTop_label_nes;
	/* STAGE6 UI USER PATCH END */
	lv_obj_t *DeskTop_tileview_menu;
	lv_obj_t *DeskTop_tileview_menu_tile;
	lv_obj_t *DeskTop_img_phoneAPP;
	lv_obj_t *DeskTop_img_messageAPP;
	lv_obj_t *DeskTop_img_cameraAPP;
	lv_obj_t *DeskTop_cont_setting;
	lv_obj_t *DeskTop_menu_1;
	lv_obj_t *DeskTop_menu_1_sidebar_page;
	lv_obj_t *DeskTop_menu_1_subpage_1_cont;
	lv_obj_t *DeskTop_menu_1_cont_1;
	lv_obj_t *DeskTop_menu_1_label_1;
	lv_obj_t *DeskTop_menu_1_subpage_2_cont;
	lv_obj_t *DeskTop_menu_1_cont_2;
	lv_obj_t *DeskTop_menu_1_label_2;
	lv_obj_t *DeskTop_menu_1_subpage_3_cont;
	lv_obj_t *DeskTop_menu_1_cont_3;
	lv_obj_t *DeskTop_menu_1_label_3;
	lv_obj_t *DeskTop_menu_1_subpage_4_cont;
	lv_obj_t *DeskTop_menu_1_cont_4;
	lv_obj_t *DeskTop_menu_1_label_4;
	lv_obj_t *DeskTop_menu_1_subpage_5_cont;
	lv_obj_t *DeskTop_menu_1_cont_5;
	lv_obj_t *DeskTop_menu_1_label_5;
	lv_obj_t *DeskTop_menu_1_subpage_6_cont;
	lv_obj_t *DeskTop_menu_1_cont_6;
	lv_obj_t *DeskTop_menu_1_label_6;
	lv_obj_t *DeskTop_menu_1_subpage_7_cont;
	lv_obj_t *DeskTop_menu_1_cont_7;
	lv_obj_t *DeskTop_menu_1_label_7;
	lv_obj_t *DeskTop_cont_WiFi_list;
	lv_obj_t *DeskTop_list_1;
	lv_obj_t *DeskTop_list_1_item0;
	lv_obj_t *DeskTop_list_1_item1;
	lv_obj_t *DeskTop_btn_custom;
	lv_obj_t *DeskTop_btn_custom_label;
	lv_obj_t *DeskTop_btn_connect;
	lv_obj_t *DeskTop_btn_connect_label;
	lv_obj_t *DeskTop_cont_blutooth;
	lv_obj_t *DeskTop_cont_volume;
	lv_obj_t *DeskTop_cont_other_setting;
	lv_obj_t *DeskTop_btn_2;
	lv_obj_t *DeskTop_btn_2_label;
	lv_obj_t *DeskTop_btn_Debug;
	lv_obj_t *DeskTop_btn_Debug_label;
	lv_obj_t *DeskTop_cont_chart;
	lv_obj_t *DeskTop_cont_chart2;
	lv_obj_t *DeskTop_scale_2;
	lv_obj_t *DeskTop_chart_2;
	lv_chart_series_t *DeskTop_chart_2_0;
	lv_obj_t *DeskTop_cont_chart1;
	lv_obj_t *DeskTop_scale_x1;
	lv_obj_t *DeskTop_chart_1;
	lv_chart_series_t *DeskTop_chart_1_0;
	lv_obj_t *DeskTop_scale_y2;
	lv_obj_t *DeskTop_scale_y1;
	lv_obj_t *DeskTop_cont_time;
	lv_obj_t *DeskTop_label_22;
	lv_obj_t *DeskTop_datetext_2;
	lv_obj_t *DeskTop_btn_6;
	lv_obj_t *DeskTop_btn_6_label;
	lv_obj_t *DeskTop_btn_7;
	lv_obj_t *DeskTop_btn_7_label;
	lv_obj_t *DeskTop_btn_8;
	lv_obj_t *DeskTop_btn_8_label;
	lv_obj_t *DeskTop_cont_folder;
	lv_obj_t *DeskTop_btn_SD;
	lv_obj_t *DeskTop_btn_SD_label;
	lv_obj_t *DeskTop_label_21;
	lv_obj_t *DeskTop_list_2;
	lv_obj_t *DeskTop_list_2_item0;
	lv_obj_t *DeskTop_tileview_WiFi1;
	lv_obj_t *DeskTop_tileview_WiFi1_tile;
	lv_obj_t *DeskTop_ta_2;
	lv_obj_t *DeskTop_ta_1;
	lv_obj_t *DeskTop_label_6;
	lv_obj_t *DeskTop_label_5;
	lv_obj_t *DeskTop_btn_5;
	lv_obj_t *DeskTop_btn_5_label;
	lv_obj_t *DeskTop_spinner_1;
	lv_obj_t *DeskTop_btn_4;
	lv_obj_t *DeskTop_btn_4_label;
	lv_obj_t *DeskTop_btn_3;
	lv_obj_t *DeskTop_btn_3_label;
	lv_obj_t *DeskTop_btn_1;
	lv_obj_t *DeskTop_btn_1_label;
	lv_obj_t *DeskTop_tileview_Time;
	lv_obj_t *DeskTop_tileview_Time_tile;
	lv_obj_t *DeskTop_btn_11;
	lv_obj_t *DeskTop_btn_11_label;
	lv_obj_t *DeskTop_btn_Time;
	lv_obj_t *DeskTop_btn_Time_label;
	lv_obj_t *DeskTop_roller_3;
	lv_obj_t *DeskTop_roller_2;
	lv_obj_t *DeskTop_label_28;
	lv_obj_t *DeskTop_label_27;
	lv_obj_t *DeskTop_label_29;
	lv_obj_t *DeskTop_tileview_timekeeping;
	lv_obj_t *DeskTop_tileview_timekeeping_tile;
	lv_obj_t *DeskTop_btn_13;
	lv_obj_t *DeskTop_btn_13_label;
	lv_obj_t *DeskTop_spinner_3;
	lv_obj_t *DeskTop_btn_confirm;
	lv_obj_t *DeskTop_btn_confirm_label;
	lv_obj_t *DeskTop_label_32;
	lv_obj_t *DeskTop_label_31;
	lv_obj_t *DeskTop_roller_5;
	lv_obj_t *DeskTop_roller_4;
	lv_obj_t *DeskTop_label_30;
	lv_obj_t *DeskTop_label_33;
	lv_obj_t *DeskTop_roller_6;
	lv_obj_t *DeskTop_btn_pause;
	lv_obj_t *DeskTop_btn_pause_label;
	lv_obj_t *DeskTop_btn_reset;
	lv_obj_t *DeskTop_btn_reset_label;
	lv_obj_t *DeskTop_btn_record;
	lv_obj_t *DeskTop_btn_record_label;
	lv_obj_t *DeskTop_label_timeupdate;
	lv_obj_t *DeskTop_label_34_scroll;
	lv_obj_t *DeskTop_label_34;
	lv_obj_t *DeskTop_tileview_alarm;
	lv_obj_t *DeskTop_tileview_alarm_tile;
	lv_obj_t *DeskTop_btn_15;
	lv_obj_t *DeskTop_btn_15_label;
	lv_obj_t *DeskTop_btn_alarm;
	lv_obj_t *DeskTop_btn_alarm_label;
	lv_obj_t *DeskTop_btn_alarm_del;
	lv_obj_t *DeskTop_btn_alarm_del_label;
	lv_obj_t *DeskTop_roller_alarm_h;
	lv_obj_t *DeskTop_label_25;
	lv_obj_t *DeskTop_label_26;
	lv_obj_t *DeskTop_label_alarm;
	lv_obj_t *DeskTop_roller_1;
	lv_obj_t *DeskTop_cont_navigation_bar;
	lv_obj_t *DeskTop_label_navigation_bar2;
	lv_obj_t *DeskTop_label_navigation_bar_flag;
	lv_obj_t *DeskTop_digital_clock_1;
	lv_obj_t *DeskTop_label_23;
	lv_obj_t *DeskTop_label_24;
	lv_obj_t *screen_navigation_bar;
	bool screen_navigation_bar_del;
	lv_obj_t *screen_navigation_bar_cont_navigation_bar_return;
	lv_obj_t *screen_navigation_bar_tileview_2;
	lv_obj_t *screen_navigation_bar_tileview_2_tile;
	lv_obj_t *screen_navigation_bar_btn_2;
	lv_obj_t *screen_navigation_bar_btn_2_label;
	lv_obj_t *screen_navigation_bar_btn_3;
	lv_obj_t *screen_navigation_bar_btn_3_label;
	lv_obj_t *screen_navigation_bar_btn_4;
	lv_obj_t *screen_navigation_bar_btn_4_label;
	lv_obj_t *screen_navigation_bar_btn_5;
	lv_obj_t *screen_navigation_bar_btn_5_label;
	lv_obj_t *screen_navigation_bar_tileview_1;
	lv_obj_t *screen_navigation_bar_tileview_1_tile;
	lv_obj_t *screen_navigation_bar_datetext_1;
	lv_obj_t *screen_navigation_bar_digital_clock_1;
	lv_obj_t *screen_navigation_bar_label_week;
	lv_obj_t *screen_navigation_bar_label_navigation_bar_return_flag;
	lv_obj_t *screen_navigation_bar_slider_2;
	lv_obj_t *screen_navigation_bar_slider_1;
	lv_obj_t *screen_navigation_bar_label_5;
	lv_obj_t *screen_navigation_bar_label_6;
	lv_obj_t *screen_navigation_bar_tileview_3;
	lv_obj_t *screen_navigation_bar_tileview_3_tile;
	lv_obj_t *screen_navigation_bar_btn_9;
	lv_obj_t *screen_navigation_bar_btn_9_label;
	lv_obj_t *screen_navigation_bar_btn_8;
	lv_obj_t *screen_navigation_bar_btn_8_label;
	lv_obj_t *screen_navigation_bar_btn_7;
	lv_obj_t *screen_navigation_bar_btn_7_label;
	lv_obj_t *screen_navigation_bar_btn_6;
	lv_obj_t *screen_navigation_bar_btn_6_label;
	lv_obj_t *screen_navigation_bar_tileview_4;
	lv_obj_t *screen_navigation_bar_tileview_4_tile;
	lv_obj_t *screen_navigation_bar_btn_12;
	lv_obj_t *screen_navigation_bar_btn_12_label;
	lv_obj_t *screen_navigation_bar_btn_11;
	lv_obj_t *screen_navigation_bar_btn_11_label;
	lv_obj_t *screen_navigation_bar_btn_10;
	lv_obj_t *screen_navigation_bar_btn_10_label;
	lv_obj_t *screen_navigation_bar_btn_14;
	lv_obj_t *screen_navigation_bar_btn_14_label;
	lv_obj_t *screen_navigation_bar_label_9;
	lv_obj_t *screen_navigation_bar_tileview_WiFi;
	lv_obj_t *screen_navigation_bar_tileview_WiFi_tile;
	lv_obj_t *screen_navigation_bar_ta_2;
	lv_obj_t *screen_navigation_bar_ta_1;
	lv_obj_t *screen_navigation_bar_label_11;
	lv_obj_t *screen_navigation_bar_label_10;
	lv_obj_t *screen_navigation_bar_btn_17;
	lv_obj_t *screen_navigation_bar_btn_17_label;
	lv_obj_t *screen_navigation_bar_spinner_1;
	lv_obj_t *screen_navigation_bar_btn_16;
	lv_obj_t *screen_navigation_bar_btn_16_label;
	lv_obj_t *screen_navigation_bar_btn_15;
	lv_obj_t *screen_navigation_bar_btn_15_label;
	lv_obj_t *screen_notes;
	bool screen_notes_del;
	lv_obj_t *screen_notes_cont_1;
	lv_obj_t *screen_notes_ta_note;
	lv_obj_t *screen_notes_cont_2;
	lv_obj_t *screen_notes_label_3;
	lv_obj_t *screen_notes_label_2;
	lv_obj_t *screen_notes_digital_clock_2;
	lv_obj_t *screen_notes_btn_1;
	lv_obj_t *screen_notes_btn_1_label;
	lv_obj_t *screen_notes_btn_return;
	lv_obj_t *screen_notes_btn_return_label;
	lv_obj_t *screen_notes_btn_2;
	lv_obj_t *screen_notes_btn_2_label;
	lv_obj_t *screen_notes_btn_3;
	lv_obj_t *screen_notes_btn_3_label;
	lv_obj_t *screen_notes_btn_4;
	lv_obj_t *screen_notes_btn_4_label;
	lv_obj_t *screen_notes_btn_5;
	lv_obj_t *screen_notes_btn_5_label;
	lv_obj_t *screen_notes_list_note;
	lv_obj_t *screen_notes_list_note_item0;
	lv_obj_t *screen_notes_tileview_note;
	lv_obj_t *screen_notes_tileview_note_tile;
	lv_obj_t *screen_notes_label_4;
	lv_obj_t *screen_notes_label_5;
	lv_obj_t *screen_notes_label_6;
	lv_obj_t *screen_notes_label_7;
	lv_obj_t *screen_notes_ta_1;
	lv_obj_t *screen_notes_ta_2;
	lv_obj_t *screen_Signal_Proc;
	bool screen_Signal_Proc_del;
	lv_obj_t *screen_Signal_Proc_btn_return;
	lv_obj_t *screen_Signal_Proc_btn_return_label;
	lv_obj_t *screen_Signal_Proc_btn_printf;
	lv_obj_t *screen_Signal_Proc_btn_printf_label;
	lv_obj_t *screen_Signal_Proc_btn_clear;
	lv_obj_t *screen_Signal_Proc_btn_clear_label;
	lv_obj_t *screen_Signal_Proc_ta_1;
	lv_obj_t *screen_Signal_Proc_btn_1;
	lv_obj_t *screen_Signal_Proc_btn_1_label;
	lv_obj_t *screen_Signal_Proc_label_3;
	lv_obj_t *screen_Signal_Proc_cont_2;
	lv_obj_t *screen_Signal_Proc_label_2;
	lv_obj_t *screen_Signal_Proc_label_1;
	lv_obj_t *screen_Signal_Proc_digital_clock_1;
	lv_obj_t *Serial_Port_ta_display;  // 用于显示的只读文本区域
	lv_obj_t *screen_yuanshen;
	bool screen_yuanshen_del;
	lv_obj_t *screen_yuanshen_cont_1;
	lv_obj_t *screen_yuanshen_img_1;
	lv_obj_t *screen_yuanshen_img_2;
	lv_obj_t *screen_yuanshen_img_3;
	lv_obj_t *screen_yuanshen_cont_2;
	lv_obj_t *screen_yuanshen_label_2;
	lv_obj_t *screen_yuanshen_label_1;
	lv_obj_t *screen_yuanshen_digital_clock_1;
	lv_obj_t *screen_camera;
	bool screen_camera_del;
	lv_obj_t *screen_camera_btn_1;
	lv_obj_t *screen_camera_btn_1_label;
	lv_obj_t *screen_camera_img_1;
	/* STAGE6 UI USER PATCH BEGIN: calculator screen */
	lv_obj_t *screen_calculator;
	bool screen_calculator_del;
	lv_obj_t *screen_calculator_cont_1;
	lv_obj_t *screen_calculator_label_2;
	lv_obj_t *screen_calculator_label_1;
	lv_obj_t *screen_calculator_digital_clock_1;
	lv_obj_t *screen_calculator_btnm_1;
	lv_obj_t *screen_calculator_ta_1;
	lv_obj_t *screen_calculator_ta_2;
	lv_obj_t *screen_calculator_btn_1;
	lv_obj_t *screen_calculator_btn_1_label;
	/* STAGE6 UI USER PATCH END */
	/* STAGE6 UI USER PATCH BEGIN: piano screen */
	lv_obj_t *screen_piano;
	bool screen_piano_del;
	lv_obj_t *screen_piano_cont_1;
	lv_obj_t *screen_piano_label_1;
	lv_obj_t *screen_piano_label_2;
	lv_obj_t *screen_piano_digital_clock_1;
	lv_obj_t *screen_piano_title;
	lv_obj_t *screen_piano_keys_panel;
	lv_obj_t *screen_piano_pads_panel;
	lv_obj_t *screen_piano_control_panel;
	lv_obj_t *screen_piano_btn_back;
	lv_obj_t *screen_piano_tune_down_btn;
	lv_obj_t *screen_piano_tune_up_btn;
	lv_obj_t *screen_piano_tune_value;
	lv_obj_t *screen_piano_mode_btn;
	lv_obj_t *screen_piano_mode_btn_label;
	/* STAGE6 UI USER PATCH END */
	/* STAGE6 UI USER PATCH BEGIN: drawing screen */
	lv_obj_t *screen_drawing;
	bool screen_drawing_del;
	lv_obj_t *screen_drawing_cont_1;
	lv_obj_t *screen_drawing_label_1;
	lv_obj_t *screen_drawing_label_2;
	lv_obj_t *screen_drawing_digital_clock_1;
	lv_obj_t *screen_drawing_title;
	lv_obj_t *screen_drawing_tools;
	lv_obj_t *screen_drawing_btn_back;
	lv_obj_t *screen_drawing_pen_btn;
	lv_obj_t *screen_drawing_eraser_btn;
	lv_obj_t *screen_drawing_canvas;
	lv_obj_t *screen_drawing_actions;
	lv_obj_t *screen_drawing_width_slider;
	lv_obj_t *screen_drawing_undo_btn;
	lv_obj_t *screen_drawing_redo_btn;
	lv_obj_t *screen_drawing_clear_btn;
	lv_obj_t *screen_drawing_save_btn;
	/* STAGE6 UI USER PATCH END */
	lv_obj_t *screen_image_viewer;
	bool screen_image_viewer_del;
	lv_obj_t *screen_image_viewer_img;
	lv_obj_t *screen_image_viewer_cont_2;
	lv_obj_t *screen_image_viewer_label_2;
	lv_obj_t *screen_image_viewer_label_3;
	lv_obj_t *screen_image_viewer_digital_clock_2;
	lv_obj_t *screen_image_viewer_btn_1;
	lv_obj_t *screen_image_viewer_btn_1_label;
	lv_obj_t *screen_avi_player;
	bool screen_avi_player_del;
	lv_obj_t *screen_avi_player_img;
	lv_obj_t *screen_avi_player_cont_2;
	lv_obj_t *screen_avi_player_label_2;
	lv_obj_t *screen_avi_player_label_3;
	lv_obj_t *screen_avi_player_digital_clock_2;
	lv_obj_t *screen_avi_player_btn_1;
	lv_obj_t *screen_avi_player_btn_1_label;
	/* MUSIC APP USER PATCH BEGIN: independent music screen */
	lv_obj_t *screen_music;
	bool screen_music_del;
	lv_obj_t *screen_music_cont_navigation_bar;
	lv_obj_t *screen_music_label_navigation_bar2;
	lv_obj_t *screen_music_label_navigation_bar_flag;
	lv_obj_t *screen_music_digital_clock;
	lv_obj_t *screen_music_btn_back;
	lv_obj_t *screen_music_btn_back_label;
	/* MUSIC APP USER PATCH END */
	lv_obj_t *g_kb_top_layer;
}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void ui_init_style(lv_style_t * style);

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del);

void ui_animation(void * var, uint32_t duration, int32_t delay, int32_t start_value, int32_t end_value, lv_anim_path_cb_t path_cb,
                  uint32_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                  lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb, lv_anim_completed_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);


void init_scr_del_flag(lv_ui *ui);

void setup_bottom_layer(void);

void setup_ui(lv_ui *ui);

void video_play(lv_ui *ui);

void init_keyboard(lv_ui *ui);

extern lv_ui guider_ui;


void setup_scr_lock_screen(lv_ui *ui);
void setup_scr_DeskTop(lv_ui *ui);
void setup_scr_screen_navigation_bar(lv_ui *ui);
void setup_scr_screen_notes(lv_ui *ui);
void setup_scr_screen_Signal_Proc(lv_ui *ui);
void setup_scr_screen_yuanshen(lv_ui *ui);
void setup_scr_screen_camera(lv_ui *ui);
/* STAGE6 UI USER PATCH BEGIN: new application screens */
void setup_scr_screen_calculator(lv_ui *ui);
void setup_scr_screen_piano(lv_ui *ui);
void setup_scr_screen_drawing(lv_ui *ui);
/* STAGE6 UI USER PATCH END */
void setup_scr_screen_image_viewer(lv_ui *ui);
void setup_scr_screen_avi_player(lv_ui *ui);
/* MUSIC APP USER PATCH BEGIN: independent music screen */
void setup_scr_screen_music(lv_ui *ui);
/* MUSIC APP USER PATCH END */


LV_IMAGE_DECLARE(_PPC_lock_RGB565_1024x600_RLE);
LV_IMAGE_DECLARE(_fingerprint_RGB565A8_70x70);
LV_IMAGE_DECLARE(_note_RGB565A8_80x80);
LV_IMAGE_DECLARE(_setting_RGB565A8_80x80);
LV_IMAGE_DECLARE(_weather_RGB565A8_80x80);
LV_IMAGE_DECLARE(_Edge_RGB565A8_80x80);
LV_IMAGE_DECLARE(_calendar_RGB565A8_80x80);
LV_IMAGE_DECLARE(_photo_album_RGB565A8_80x80);
LV_IMAGE_DECLARE(_yuanshen_RGB565A8_80x80);
LV_IMAGE_DECLARE(_alarm_RGB565A8_80x80);
LV_IMAGE_DECLARE(_calculator_RGB565A8_80x80);
LV_IMAGE_DECLARE(_folder_RGB565A8_80x80);
/* STAGE6 UI USER PATCH BEGIN: new desktop image assets */
LV_IMAGE_DECLARE(_draw_RGB565A8_80x80);
LV_IMAGE_DECLARE(_piano_RGB565A8_80x80);
LV_IMAGE_DECLARE(_steam_RGB565A8_80x80);
/* STAGE6 UI USER PATCH END */
LV_IMAGE_DECLARE(_phone_RGB565A8_80x80);
LV_IMAGE_DECLARE(_message_RGB565A8_80x80);
LV_IMAGE_DECLARE(_camera_RGB565A8_80x80);

LV_IMAGE_DECLARE(_PPC_lock_RGB565A8_1024x600);
LV_IMAGE_DECLARE(_1_RGB565A8_285x107);
LV_IMAGE_DECLARE(_2_RGB565A8_700x370);
LV_IMAGE_DECLARE(_3_RGB565A8_600x220);

LV_FONT_DECLARE(lv_font_montserratMedium_30)
LV_FONT_DECLARE(lv_font_montserratMedium_16)
LV_FONT_DECLARE(lv_font_montserratMedium_20)
LV_FONT_DECLARE(lv_font_montserratMedium_18)
LV_FONT_DECLARE(lv_font_montserratMedium_12)
LV_FONT_DECLARE(lv_font_montserratMedium_14)
LV_FONT_DECLARE(lv_font_myyh_16)
LV_FONT_DECLARE(lv_font_myyh_12)
LV_FONT_DECLARE(lv_font_myyh_14)
LV_FONT_DECLARE(lv_font_myyh_16)
LV_FONT_DECLARE(lv_font_myyh_18)
LV_FONT_DECLARE(lv_font_myyh_20)
LV_FONT_DECLARE(lv_font_myyh_30)


#ifdef __cplusplus
}
#endif
#endif
