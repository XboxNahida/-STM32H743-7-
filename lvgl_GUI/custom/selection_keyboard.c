#include "selection_keyboard.h"

#include <string.h>

#include "selection_keyboard_maps.h"
#include "selection_textarea_adapter.h"

#define SELECTION_TARGET_COUNT 16U

LV_FONT_DECLARE(lv_font_myyh_18);

#if LV_USE_IME_PINYIN
extern lv_obj_t *g_kb_ime;
#endif

typedef struct {
    lv_obj_t *textarea;
    void *context;
    const text_selection_target_ops_t *ops;
} selection_target_t;

static lv_obj_t *g_keyboard;
static lv_obj_t *g_target;
static lv_obj_t *g_message;
static lv_timer_t *g_message_timer;
static const lv_font_t *g_original_font;
static selection_target_t g_targets[SELECTION_TARGET_COUNT];
static text_selection_controller_t g_controller;
static bool g_install_attempted;

static selection_target_t *find_target(lv_obj_t *textarea)
{
    uint32_t index;
    for (index = 0U; index < SELECTION_TARGET_COUNT; index++)
        if (g_targets[index].textarea == textarea) return &g_targets[index];
    return NULL;
}

static bool has_fixed_selection(void)
{
    text_selection_status_t status;
    text_selection_get_status(&g_controller, &status);
    return status.phase == TEXT_SELECTION_PHASE_FIXED &&
           status.selection_start != status.selection_end;
}

static void message_timeout(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    if (g_message != NULL) lv_obj_delete(g_message);
    g_message = NULL;
    g_message_timer = NULL;
}

void selection_keyboard_show_message(const char *message)
{
    if (message == NULL) return;
    if (g_message_timer != NULL) lv_timer_delete(g_message_timer);
    if (g_message == NULL) g_message = lv_label_create(lv_layer_top());
    lv_label_set_text(g_message, message);
    lv_obj_set_style_text_font(g_message, &lv_font_myyh_18, 0);
    lv_obj_set_style_bg_opa(g_message, LV_OPA_80, 0);
    lv_obj_set_style_pad_all(g_message, 8, 0);
    lv_obj_align(g_message, LV_ALIGN_TOP_MID, 0, 12);
    g_message_timer = lv_timer_create(message_timeout, 1800U, NULL);
    if (g_message_timer != NULL) lv_timer_set_repeat_count(g_message_timer, 1);
}

static void report_result(text_selection_action_t action,
                          text_selection_result_t result)
{
    if (result == TEXT_SELECTION_OK) return;
    if (result == TEXT_SELECTION_CAPACITY)
        selection_keyboard_show_message("\xE5\xA4\x8D\xE5\x88\xB6\xE5\x86\x85\xE5\xAE\xB9\xE8\xB6\x85\xE8\xBF\x87" "64KB");
    else if (action == TEXT_SELECT_PASTE && result == TEXT_SELECTION_EMPTY)
        selection_keyboard_show_message("\xE5\x89\xAA\xE8\xB4\xB4\xE6\x9D\xBF\xE4\xB8\xBA\xE7\xA9\xBA");
    else if (action == TEXT_SELECT_COPY && result == TEXT_SELECTION_EMPTY)
        selection_keyboard_show_message("\xE8\xAF\xB7\xE5\x85\x88\xE8\xAE\xBE\xE7\xBD\xAE\xE9\x80\x89\xE6\x8B\xA9\xE7\xBB\x88\xE7\x82\xB9");
    else
        selection_keyboard_show_message("\xE6\x96\x87\xE6\x9C\xAC\xE5\xAE\xB9\xE9\x87\x8F\xE5\xB7\xB2\xE6\xBB\xA1");
}

static bool key_action(const char *key, text_selection_action_t *action)
{
    if (strcmp(key, "|<") == 0) *action = TEXT_SELECT_HOME;
    else if (strcmp(key, ">|") == 0) *action = TEXT_SELECT_END;
    else if (strcmp(key, LV_SYMBOL_UP) == 0) *action = TEXT_SELECT_UP;
    else if (strcmp(key, LV_SYMBOL_DOWN) == 0) *action = TEXT_SELECT_DOWN;
    else if (strcmp(key, LV_SYMBOL_LEFT) == 0) *action = TEXT_SELECT_LEFT;
    else if (strcmp(key, LV_SYMBOL_RIGHT) == 0) *action = TEXT_SELECT_RIGHT;
    else if (strcmp(key, "OK") == 0) *action = TEXT_SELECT_OK;
    else if (strcmp(key, SELECTION_KEY_COPY) == 0) *action = TEXT_SELECT_COPY;
    else if (strcmp(key, SELECTION_KEY_PASTE) == 0) *action = TEXT_SELECT_PASTE;
    else if (strcmp(key, SELECTION_KEY_CANCEL) == 0) *action = TEXT_SELECT_CANCEL;
    else return false;
    return true;
}

static void keyboard_event_cb(lv_event_t *e)
{
    lv_obj_t *kb = lv_event_get_current_target(e);
    text_selection_action_t action;
    text_selection_result_t result;
    const char *key;
    uint32_t id = lv_buttonmatrix_get_selected_button(kb);
    uint8_t handled = 0U;
    bool fixed;
    if (id == LV_BUTTONMATRIX_BUTTON_NONE) return;
    key = lv_buttonmatrix_get_button_text(kb, id);
    if (key == NULL) return;
    if (strcmp(key, SELECTION_KEY_ENTRY) == 0) {
        if (g_target == NULL) {
            selection_keyboard_show_message("\xE8\xAF\xB7\xE5\x85\x88\xE9\x80\x89\xE6\x8B\xA9\xE6\x96\x87\xE6\x9C\xAC\xE6\xA1\x86");
        }
        else {
#if LV_USE_IME_PINYIN
            if (g_kb_ime != NULL) {
                lv_ime_pinyin_reset(g_kb_ime);
                lv_ime_pinyin_set_language(
                    g_kb_ime, LV_IME_PINYIN_LANGUAGE_ENGLISH);
                lv_ime_pinyin_set_textarea(g_kb_ime, g_target);
                lv_obj_add_flag(lv_ime_pinyin_get_cand_panel(g_kb_ime),
                                LV_OBJ_FLAG_HIDDEN);
            }
#endif
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_2);
            lv_obj_set_style_text_font(kb, &lv_font_myyh_18,
                                       LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_event_stop_processing(e);
        return;
    }
    if (lv_keyboard_get_mode(kb) == LV_KEYBOARD_MODE_USER_2 &&
        strcmp(key, "ABC") == 0) {
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_UPPER);
        lv_obj_set_style_text_font(kb, g_original_font,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
#if LV_USE_IME_PINYIN
        if (g_kb_ime != NULL) {
            lv_ime_pinyin_set_language(
                g_kb_ime, LV_IME_PINYIN_LANGUAGE_ENGLISH);
            lv_ime_pinyin_set_textarea(g_kb_ime, g_target);
            lv_ime_pinyin_reset(g_kb_ime);
        }
#endif
        lv_event_stop_processing(e);
        return;
    }
    if (g_target == NULL || lv_keyboard_get_textarea(kb) != g_target) return;
    if (strcmp(key, LV_SYMBOL_BACKSPACE) == 0) {
        fixed = has_fixed_selection();
        result = text_selection_replace_active(&g_controller, (const uint8_t *)"",
                                               0U, &handled);
        if (fixed) {
            report_result(TEXT_SELECT_CANCEL, result);
            lv_event_stop_processing(e);
        }
        return;
    }
    if (lv_keyboard_get_mode(kb) != LV_KEYBOARD_MODE_USER_2 ||
        !key_action(key, &action)) return;
    report_result(action, text_selection_action(&g_controller, action));
    lv_event_stop_processing(e);
}

static void adapter_deleted(lv_obj_t *textarea, void *user_data)
{
    uint32_t index;
    LV_UNUSED(user_data);
    if (g_keyboard != NULL && lv_keyboard_get_textarea(g_keyboard) == textarea)
        lv_keyboard_set_textarea(g_keyboard, NULL);
    if (g_target == textarea) {
        g_target = NULL;
        g_controller.target = NULL;
        g_controller.ops = NULL;
    }
    for (index = 0U; index < SELECTION_TARGET_COUNT; index++) {
        if (g_targets[index].textarea != textarea) continue;
        g_targets[index].textarea = NULL;
        g_targets[index].context = NULL;
        g_targets[index].ops = NULL;
    }
}

static void adapter_result(text_selection_result_t result, void *user_data)
{
    LV_UNUSED(user_data);
    report_result(TEXT_SELECT_CANCEL, result);
}

bool selection_keyboard_install(lv_obj_t *kb)
{
    text_selection_controller_t controller;
    lv_event_dsc_t *old_default = NULL;
    lv_event_dsc_t *extension_event;
    lv_event_dsc_t *new_default;
    const lv_font_t *original_font;
    uint8_t *clipboard;
    uint32_t event_count;
    uint32_t old_index = 0U;
    uint32_t index;
    if (g_keyboard != NULL) return g_keyboard == kb;
    if (kb == NULL || g_install_attempted) return false;
    g_install_attempted = true;
    event_count = lv_obj_get_event_count(kb);
    for (index = 0U; index < event_count; index++) {
        lv_event_dsc_t *event = lv_obj_get_event_dsc(kb, index);
        if (event != NULL && lv_event_dsc_get_cb(event) == lv_keyboard_def_event_cb) {
            old_default = event;
            old_index = index;
            break;
        }
    }
    if (old_default == NULL) return false;
    clipboard = lv_malloc(TEXT_SELECTION_CLIPBOARD_BYTES + 1U);
    if (clipboard == NULL) {
        selection_keyboard_show_message("\xE9\x80\x89\xE6\x8B\xA9\xE8\xB5\x84\xE6\xBA\x90\xE4\xB8\x8D\xE8\xB6\xB3");
        return false;
    }
    if (text_selection_init(&controller, clipboard,
                            TEXT_SELECTION_CLIPBOARD_BYTES + 1U) != TEXT_SELECTION_OK) {
        lv_free(clipboard);
        return false;
    }
    extension_event = lv_obj_add_event_cb(kb, keyboard_event_cb,
                                          LV_EVENT_VALUE_CHANGED, NULL);
    if (extension_event == NULL) {
        lv_free(clipboard);
        return false;
    }
    new_default = lv_obj_add_event_cb(kb, lv_keyboard_def_event_cb,
                                      LV_EVENT_VALUE_CHANGED, NULL);
    if (new_default == NULL) {
        (void)lv_obj_remove_event_dsc(kb, extension_event);
        lv_free(clipboard);
        return false;
    }
    if (!lv_obj_remove_event(kb, old_index)) {
        (void)lv_obj_remove_event_dsc(kb, new_default);
        (void)lv_obj_remove_event_dsc(kb, extension_event);
        lv_free(clipboard);
        return false;
    }
    original_font = lv_obj_get_style_text_font(kb, LV_PART_MAIN);
    selection_textarea_adapter_set_callbacks(adapter_deleted, adapter_result, NULL);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_UPPER,
                        selection_keyboard_maps_upper(),
                        selection_keyboard_maps_upper_ctrl());
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_2,
                        selection_keyboard_maps_selection(),
                        selection_keyboard_maps_selection_ctrl());
    g_controller = controller;
    g_original_font = original_font;
    g_keyboard = kb;
    return true;
}

bool selection_keyboard_register_target(lv_obj_t *ta, void *context,
                                        const text_selection_target_ops_t *ops)
{
    uint32_t index;
    if (ta == NULL || (context == NULL) != (ops == NULL) || find_target(ta) != NULL)
        return false;
    for (index = 0U; index < SELECTION_TARGET_COUNT; index++) {
        if (g_targets[index].textarea != NULL) continue;
        if (!selection_textarea_adapter_attach(ta)) return false;
        g_targets[index].textarea = ta;
        g_targets[index].context = context != NULL ? context : ta;
        g_targets[index].ops = ops != NULL ? ops : selection_textarea_adapter_ops();
        return true;
    }
    return false;
}

void selection_keyboard_focus(lv_obj_t *ta)
{
    selection_target_t *entry = find_target(ta);
    if (g_keyboard == NULL || entry == NULL ||
        lv_obj_has_state(ta, LV_STATE_DISABLED) ||
        lv_keyboard_get_textarea(g_keyboard) != ta) return;
    if (text_selection_set_target(&g_controller, entry->context, entry->ops) ==
        TEXT_SELECTION_OK) {
        g_target = ta;
        selection_textarea_adapter_focus(ta, &g_controller);
    }
}

void selection_keyboard_blur(lv_obj_t *ta)
{
    if (ta == NULL || g_target != ta) return;
    selection_textarea_adapter_blur(ta);
    g_target = NULL;
}

void selection_keyboard_unregister_target(lv_obj_t *ta)
{
    selection_target_t *entry;
    if (ta == NULL) return;
    selection_keyboard_blur(ta);
    selection_textarea_adapter_detach(ta);
    entry = find_target(ta);
    if (entry != NULL) {
        entry->textarea = NULL;
        entry->context = NULL;
        entry->ops = NULL;
    }
}
