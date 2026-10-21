#ifndef OSCILLOSCOPE_UI_H
#define OSCILLOSCOPE_UI_H
#include "lvgl.h"
void oscilloscope_ui_init(void);
void oscilloscope_ui_poll(void);
void oscilloscope_ui_open(void);
/* 桌面重建后可重复调用；仅在指定桌面添加一次入口。 */
void oscilloscope_ui_attach_desktop(lv_obj_t *desktop);
#endif
