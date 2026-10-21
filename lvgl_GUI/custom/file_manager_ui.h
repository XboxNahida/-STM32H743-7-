#ifndef FILE_MANAGER_UI_H
#define FILE_MANAGER_UI_H

#include "lvgl.h"

typedef int (*file_manager_ui_open_cb_t)(const char *path);

void file_manager_ui_init(void);
void file_manager_ui_set_open_callback(file_manager_ui_open_cb_t callback);
void file_manager_ui_request_open(void);
void file_manager_ui_request_readonly(lv_obj_t *parent);
void file_manager_ui_poll(void);

#endif
