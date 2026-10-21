#ifndef MUSIC_PLAYER_UI_H
#define MUSIC_PLAYER_UI_H

#include <stdint.h>
#include "lvgl.h"

void music_player_ui_init(void);
void music_player_ui_attach(lv_obj_t *screen);
void music_player_ui_request_open(void);
void music_player_ui_open(void);
void music_player_ui_poll(uint32_t now_ms);
void music_player_ui_set_navigation_return(int enabled);
int music_player_ui_take_navigation_return(void);

#endif
