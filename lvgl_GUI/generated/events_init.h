/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/


#ifndef EVENTS_INIT_H_
#define EVENTS_INIT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "gui_guider.h"

void events_init(lv_ui *ui);

void events_init_lock_screen(lv_ui *ui);
void events_init_DeskTop(lv_ui *ui);
void events_init_screen_navigation_bar(lv_ui *ui);
void events_init_screen_notes(lv_ui *ui);
void events_init_screen_Signal_Proc(lv_ui *ui);
void events_init_screen_yuanshen(lv_ui *ui);
void events_init_screen_camera(lv_ui *ui);
/* STAGE6 UI USER PATCH BEGIN: new application screens */
void events_init_screen_calculator(lv_ui *ui);
void events_init_screen_piano(lv_ui *ui);
void events_init_screen_drawing(lv_ui *ui);
/* STAGE6 UI USER PATCH END */
void events_init_screen_image_viewer(lv_ui *ui);
void events_init_screen_avi_player(lv_ui *ui);
/* MUSIC APP USER PATCH BEGIN: independent music screen */
void events_init_screen_music(lv_ui *ui);
int events_open_notes_file(const char *path);
int events_open_avi_file(const char *path);
/* MUSIC APP USER PATCH END */

#ifdef __cplusplus
}
#endif
#endif /* EVENT_CB_H_ */
