#ifndef SELECTION_KEYBOARD_H
#define SELECTION_KEYBOARD_H

#include "lvgl.h"
#include "text_selection_controller.h"

bool selection_keyboard_install(lv_obj_t *keyboard);
bool selection_keyboard_register_target(lv_obj_t *textarea, void *context,
                                        const text_selection_target_ops_t *ops);
void selection_keyboard_unregister_target(lv_obj_t *textarea);
void selection_keyboard_focus(lv_obj_t *textarea);
void selection_keyboard_blur(lv_obj_t *textarea);
void selection_keyboard_show_message(const char *message);

#endif
