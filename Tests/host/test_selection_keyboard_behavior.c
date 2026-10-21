#include <stdio.h>
#include <string.h>
#include <process.h>

#include "selection_keyboard.h"
#include "selection_keyboard_maps.h"

fake_lvgl_state_t g_fake_lvgl;
lv_font_t lv_font_myyh_18 = {18U};
lv_font_t g_fake_default_font = {18U};
static int g_failures;

#define CHECK(name, condition) do { if (!(condition)) { \
    printf("FAIL %s\n", name); g_failures++; } } while (0)

static void reset_fake(void)
{
    memset(&g_fake_lvgl, 0, sizeof(g_fake_lvgl));
    g_fake_lvgl.top_layer.font = &g_fake_default_font;
}

void lv_keyboard_def_event_cb(lv_event_t *event)
{
    lv_obj_t *keyboard = lv_event_get_current_target(event);
    const char *key = lv_buttonmatrix_get_button_text(
        keyboard, lv_buttonmatrix_get_selected_button(keyboard));
    g_fake_lvgl.default_calls++;
    if (keyboard->textarea == NULL || key == NULL) return;
    if (strcmp(key, LV_SYMBOL_BACKSPACE) == 0) lv_textarea_delete_char(keyboard->textarea);
    else lv_textarea_add_text(keyboard->textarea, key);
}

static void init_keyboard(lv_obj_t *keyboard)
{
    static const char *const old_map[] = {"OLD", ""};
    memset(keyboard, 0, sizeof(*keyboard));
    keyboard->font = &g_fake_default_font;
    keyboard->mode = LV_KEYBOARD_MODE_TEXT_UPPER;
    keyboard->maps[LV_KEYBOARD_MODE_TEXT_UPPER] = old_map;
    (void)lv_obj_add_event_cb(keyboard, lv_keyboard_def_event_cb,
                              LV_EVENT_VALUE_CHANGED, NULL);
    g_fake_lvgl.add_calls = 0U;
}

static void init_textarea(lv_obj_t *textarea, lv_obj_t *label,
                          const char *text, uint32_t cursor)
{
    memset(textarea, 0, sizeof(*textarea));
    memset(label, 0, sizeof(*label));
    strcpy(textarea->text, text);
    strcpy(label->text, text);
    textarea->label = label;
    textarea->cursor = cursor;
    label->selection_start = LV_LABEL_TEXT_SELECTION_OFF;
    label->selection_end = LV_LABEL_TEXT_SELECTION_OFF;
}

static bool press_key(lv_obj_t *keyboard, const char *key)
{
    uint32_t button = 0U;
    const char *text;
    while ((text = fake_map_button(keyboard->maps[keyboard->mode], button)) != NULL) {
        if (strcmp(text, key) == 0) {
            keyboard->selected_button = button;
            fake_send_event(keyboard, LV_EVENT_VALUE_CHANGED, NULL);
            return true;
        }
        button++;
    }
    return false;
}

static void bind_and_select_first(lv_obj_t *keyboard, lv_obj_t *textarea)
{
    lv_keyboard_set_textarea(keyboard, textarea);
    selection_keyboard_focus(textarea);
    if (lv_keyboard_get_mode(keyboard) == LV_KEYBOARD_MODE_USER_2)
        CHECK("return upper", press_key(keyboard, "ABC"));
    CHECK("enter select", press_key(keyboard, SELECTION_KEY_ENTRY));
    CHECK("anchor", press_key(keyboard, "OK"));
    CHECK("move right", press_key(keyboard, LV_SYMBOL_RIGHT));
    CHECK("fix", press_key(keyboard, "OK"));
}

static void check_failed_insert(lv_obj_t *textarea, lv_obj_t *label,
                                const char *replacement, const char *name)
{
    char old_text[256];
    uint32_t old_cursor = textarea->cursor;
    uint32_t old_start = label->selection_start;
    uint32_t old_end = label->selection_end;
    strcpy(old_text, textarea->text);
    lv_textarea_add_text(textarea, replacement);
    CHECK(name, strcmp(textarea->text, old_text) == 0 && textarea->cursor == old_cursor &&
          label->selection_start == old_start && label->selection_end == old_end);
}

typedef struct {
    uint8_t text[64];
    uint32_t length;
    uint32_t cursor;
    uint32_t replace_calls;
    bool fail_replace;
} custom_target_t;

static uint32_t custom_length(void *context) { return ((custom_target_t *)context)->length; }
static uint32_t custom_cursor(void *context) { return ((custom_target_t *)context)->cursor; }
static int custom_set_cursor(void *context, uint32_t cursor)
{
    custom_target_t *target = context;
    if (cursor > target->length) return 1;
    target->cursor = cursor;
    return 0;
}
static int custom_move(void *context, text_selection_vertical_t direction, uint32_t *cursor)
{
    LV_UNUSED(context); LV_UNUSED(direction); LV_UNUSED(cursor); return 1;
}
static void custom_show(void *context, uint8_t visible, uint32_t start, uint32_t end)
{
    LV_UNUSED(context); LV_UNUSED(visible); LV_UNUSED(start); LV_UNUSED(end);
}
static int custom_read(void *context, uint32_t start, uint32_t end,
                       uint8_t *output, uint32_t capacity)
{
    custom_target_t *target = context;
    if (start > end || end > target->length || end - start >= capacity) return 1;
    memcpy(output, target->text + start, end - start);
    return 0;
}
static int custom_replace(void *context, uint32_t start, uint32_t end,
                          const uint8_t *replacement, uint32_t bytes, uint32_t *cursor)
{
    custom_target_t *target = context;
    uint32_t next = target->length - (end - start) + bytes;
    target->replace_calls++;
    if (target->fail_replace || start > end || end > target->length || next >= sizeof(target->text)) return 1;
    memmove(target->text + start + bytes, target->text + end, target->length - end + 1U);
    if (bytes != 0U) memcpy(target->text + start, replacement, bytes);
    target->length = next; target->cursor = start + bytes; *cursor = target->cursor;
    return 0;
}
static const text_selection_target_ops_t g_custom_ops = {
    custom_length, custom_cursor, custom_set_cursor, custom_move,
    custom_show, custom_read, custom_replace
};

static int scenario_validation(void)
{
    lv_obj_t keyboard, areas[4], labels[4];
    const char invalid[] = {(char)0xC3, '(', '\0'};
    uint32_t index;
    reset_fake(); init_keyboard(&keyboard);
    CHECK("install", selection_keyboard_install(&keyboard));
    for (index = 0U; index < 4U; index++) {
        init_textarea(&areas[index], &labels[index], index == 2U ? "abc" : "ab", 0U);
        CHECK("register", selection_keyboard_register_target(&areas[index], NULL, NULL));
    }
    areas[0].one_line = true; bind_and_select_first(&keyboard, &areas[0]);
    check_failed_insert(&areas[0], &labels[0], "\n", "one line atomic");
    areas[1].accepted = "abc"; bind_and_select_first(&keyboard, &areas[1]);
    check_failed_insert(&areas[1], &labels[1], "Z", "accepted atomic");
    areas[2].max_length = 3U; bind_and_select_first(&keyboard, &areas[2]);
    check_failed_insert(&areas[2], &labels[2], "ZZ", "max atomic");
    bind_and_select_first(&keyboard, &areas[3]);
    check_failed_insert(&areas[3], &labels[3], invalid, "utf8 atomic");
    return g_failures != 0;
}

static void init_custom(custom_target_t *target, bool fail)
{
    memset(target, 0, sizeof(*target));
    memcpy(target->text, "abc", 4U); target->length = 3U; target->fail_replace = fail;
}

static int scenario_custom(bool fail)
{
    lv_obj_t keyboard, textarea, label;
    custom_target_t target;
    uint32_t default_before;
    reset_fake(); init_keyboard(&keyboard); init_textarea(&textarea, &label, "ui", 0U);
    init_custom(&target, fail);
    CHECK("install", selection_keyboard_install(&keyboard));
    CHECK("custom register", selection_keyboard_register_target(&textarea, &target, &g_custom_ops));
    bind_and_select_first(&keyboard, &textarea);
    lv_textarea_add_text(&textarea, "Z");
    if (fail) {
        CHECK("failed insert suppressed", strcmp(textarea.text, "ui") == 0 && strcmp((char *)target.text, "abc") == 0);
        CHECK("return upper", press_key(&keyboard, "ABC")); default_before = g_fake_lvgl.default_calls;
        CHECK("backspace key", press_key(&keyboard, LV_SYMBOL_BACKSPACE));
        CHECK("failed backspace suppressed", strcmp(textarea.text, "ui") == 0 &&
              strcmp((char *)target.text, "abc") == 0 && g_fake_lvgl.default_calls == default_before);
    }
    else CHECK("custom insert routed", strcmp((char *)target.text, "Zbc") == 0 && strcmp(textarea.text, "ui") == 0);
    return g_failures != 0;
}

static int scenario_delete_clipboard(void)
{
    lv_obj_t keyboard, first, second, first_label, second_label;
    reset_fake(); init_keyboard(&keyboard);
    init_textarea(&first, &first_label, "abc", 0U); init_textarea(&second, &second_label, "X", 1U);
    CHECK("install", selection_keyboard_install(&keyboard));
    CHECK("register first", selection_keyboard_register_target(&first, NULL, NULL));
    bind_and_select_first(&keyboard, &first); CHECK("copy", press_key(&keyboard, SELECTION_KEY_COPY));
    fake_send_event(&first, LV_EVENT_DELETE, NULL);
    CHECK("delete unbind", lv_keyboard_get_textarea(&keyboard) == NULL);
    CHECK("register second", selection_keyboard_register_target(&second, NULL, NULL));
    lv_keyboard_set_textarea(&keyboard, &second); selection_keyboard_focus(&second);
    CHECK("paste retained", press_key(&keyboard, SELECTION_KEY_PASTE));
    CHECK("clipboard content", strcmp(second.text, "Xa") == 0);
    return g_failures != 0;
}

static int scenario_attach_fail(void)
{
    lv_obj_t textarea, label;
    reset_fake(); init_textarea(&textarea, &label, "a", 0U);
    g_fake_lvgl.fail_add_call = 2U;
    CHECK("attach fail", !selection_keyboard_register_target(&textarea, NULL, NULL));
    CHECK("attach rollback", textarea.event_count == 0U);
    return g_failures != 0;
}

static int scenario_install(const char *mode)
{
    lv_obj_t keyboard;
    const char *const *old_map;
    bool result;
    reset_fake(); init_keyboard(&keyboard); old_map = keyboard.maps[LV_KEYBOARD_MODE_TEXT_UPPER];
    if (strcmp(mode, "alloc") == 0) g_fake_lvgl.fail_malloc = true;
    if (strcmp(mode, "ext") == 0) g_fake_lvgl.fail_add_call = 1U;
    if (strcmp(mode, "def") == 0) g_fake_lvgl.fail_add_call = 2U;
    if (strcmp(mode, "remove") == 0) g_fake_lvgl.fail_remove_event = true;
    result = selection_keyboard_install(&keyboard);
    if (strcmp(mode, "success") != 0) {
        CHECK("install rejected", !result);
        CHECK("old event kept", keyboard.event_count == 1U && keyboard.events[0]->callback == lv_keyboard_def_event_cb);
        CHECK("old map kept", keyboard.maps[LV_KEYBOARD_MODE_TEXT_UPPER] == old_map);
    }
    else {
        CHECK("install success", result && keyboard.event_count == 2U);
        CHECK("event order", keyboard.events[0]->callback != lv_keyboard_def_event_cb &&
              keyboard.events[1]->callback == lv_keyboard_def_event_cb);
        CHECK("repeat idempotent", selection_keyboard_install(&keyboard) &&
              keyboard.event_count == 2U && g_fake_lvgl.malloc_calls == 1U);
    }
    return g_failures != 0;
}

static int run_named(const char *name)
{
    if (strcmp(name, "validation") == 0) return scenario_validation();
    if (strcmp(name, "custom_fail") == 0) return scenario_custom(true);
    if (strcmp(name, "custom_ok") == 0) return scenario_custom(false);
    if (strcmp(name, "delete") == 0) return scenario_delete_clipboard();
    if (strcmp(name, "attach") == 0) return scenario_attach_fail();
    return scenario_install(name);
}

int main(int argc, char **argv)
{
    static const char *const cases[] = {
        "validation", "custom_fail", "custom_ok", "delete", "attach",
        "alloc", "ext", "def", "remove", "success"
    };
    uint32_t index;
    if (argc == 2) return run_named(argv[1]);
    for (index = 0U; index < sizeof(cases) / sizeof(cases[0]); index++) {
        if (_spawnl(_P_WAIT, argv[0], argv[0], cases[index], NULL) != 0) {
            printf("FAIL child_%s\n", cases[index]);
            return 1;
        }
    }
    printf("PASS selection_keyboard_behavior\n");
    return 0;
}
