#include "selection_keyboard_maps.h"

#define KB_CTRL(value) ((lv_buttonmatrix_ctrl_t)(value))
#define KB_BTN(width) KB_CTRL(LV_BUTTONMATRIX_CTRL_POPOVER | (width))
#define KB_FLAGS(width) KB_CTRL(LV_KEYBOARD_CTRL_BUTTON_FLAGS | (width))
#define KB_CHECKED(width) KB_CTRL(LV_BUTTONMATRIX_CTRL_CHECKED | (width))
#define KB_CHECKED_BTN(width) \
    KB_CTRL(LV_BUTTONMATRIX_CTRL_CHECKED | LV_BUTTONMATRIX_CTRL_POPOVER | (width))

static const char *const g_upper_map[] = {
    "1#", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", LV_SYMBOL_BACKSPACE, "\n",
    "abc", "A", "S", "D", "F", "G", "H", "J", "K", "L", LV_SYMBOL_NEW_LINE, "\n",
    "_", ":", "Z", "X", "C", "V", "B", "N", "M", "\"", ",", ";", "\n",
    SELECTION_KEY_ENTRY,
#if LV_USE_ARABIC_PERSIAN_CHARS
    "أب",
#endif
    LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t g_upper_ctrl[] = {
    KB_FLAGS(5), KB_BTN(4), KB_BTN(4), KB_BTN(4), KB_BTN(4), KB_BTN(4), KB_BTN(4), KB_BTN(4), KB_BTN(4), KB_BTN(4), KB_BTN(4), KB_CHECKED(7),
    KB_FLAGS(6), KB_BTN(3), KB_BTN(3), KB_BTN(3), KB_BTN(3), KB_BTN(3), KB_BTN(3), KB_BTN(3), KB_BTN(3), KB_BTN(3), KB_CHECKED(7),
    KB_CHECKED_BTN(1), KB_CHECKED_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_BTN(1), KB_CHECKED_BTN(1), KB_CHECKED_BTN(1), KB_CHECKED_BTN(1),
    KB_FLAGS(2),
#if LV_USE_ARABIC_PERSIAN_CHARS
    KB_FLAGS(2),
#endif
    KB_CHECKED(2), KB_CTRL(6), KB_CHECKED(2), KB_FLAGS(2)
};

static const char *const g_selection_map[] = {
    "|<", LV_SYMBOL_UP, ">|", SELECTION_KEY_CANCEL, "\n",
    LV_SYMBOL_LEFT, "OK", LV_SYMBOL_RIGHT, " ", "\n",
    SELECTION_KEY_COPY, LV_SYMBOL_DOWN, SELECTION_KEY_PASTE, "ABC", ""
};

static const lv_buttonmatrix_ctrl_t g_selection_ctrl[] = {
    KB_CTRL(3), KB_CTRL(3), KB_CTRL(3), KB_CTRL(2),
    KB_CTRL(3), KB_CTRL(3), KB_CTRL(3),
    KB_CTRL(LV_BUTTONMATRIX_CTRL_HIDDEN | 2),
    KB_CTRL(3), KB_CTRL(3), KB_CTRL(3), KB_FLAGS(2)
};

const char *const *selection_keyboard_maps_upper(void)
{
    return g_upper_map;
}

const lv_buttonmatrix_ctrl_t *selection_keyboard_maps_upper_ctrl(void)
{
    return g_upper_ctrl;
}

const char *const *selection_keyboard_maps_selection(void)
{
    return g_selection_map;
}

const lv_buttonmatrix_ctrl_t *selection_keyboard_maps_selection_ctrl(void)
{
    return g_selection_ctrl;
}
