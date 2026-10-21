#ifndef NOTES_EDITOR_UI_H
#define NOTES_EDITOR_UI_H

#include <stdbool.h>

#include "lvgl.h"

bool notes_editor_ui_attach(lv_obj_t *textarea);
int notes_editor_ui_open(const char *path);
int notes_editor_ui_new(void);
int notes_editor_ui_save(const char *path);
void notes_editor_ui_discard(void);
int notes_editor_ui_detach(void);

#endif
