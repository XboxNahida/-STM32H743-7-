#ifndef SELECTION_KEYBOARD_MAPS_H
#define SELECTION_KEYBOARD_MAPS_H

#include "lvgl.h"

#define SELECTION_KEY_ENTRY "SEL"
#define SELECTION_KEY_COPY "\xE5\xA4\x8D\xE5\x88\xB6"
#define SELECTION_KEY_PASTE "\xE7\xB2\x98\xE8\xB4\xB4"
#define SELECTION_KEY_CANCEL "\xE5\x8F\x96\xE6\xB6\x88\xE9\x80\x89\xE4\xB8\xAD"

const char *const *selection_keyboard_maps_upper(void);
const lv_buttonmatrix_ctrl_t *selection_keyboard_maps_upper_ctrl(void);
const char *const *selection_keyboard_maps_selection(void);
const lv_buttonmatrix_ctrl_t *selection_keyboard_maps_selection_ctrl(void);

#endif
