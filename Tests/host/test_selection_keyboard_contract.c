#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    char *data;
    long size;
    if (file == NULL) return NULL;
    if (fseek(file, 0L, SEEK_END) != 0 || (size = ftell(file)) < 0L ||
        fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return NULL;
    }
    data = (char *)malloc((size_t)size + 1U);
    if (data == NULL || fread(data, 1U, (size_t)size, file) != (size_t)size) {
        free(data);
        (void)fclose(file);
        return NULL;
    }
    data[size] = '\0';
    (void)fclose(file);
    return data;
}

static int require_text(const char *source, const char *name, const char *text)
{
    if (strstr(source, text) != NULL) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

static int forbid_text(const char *source, const char *name, const char *text)
{
    if (strstr(source, text) == NULL) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

static int require_order(const char *source, const char *name,
                         const char *first, const char *second)
{
    const char *left = strstr(source, first);
    const char *right = strstr(source, second);
    if (left != NULL && right != NULL && left < right) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

static int require_count(const char *source, const char *name,
                         const char *text, unsigned int expected)
{
    const char *cursor = source;
    unsigned int count = 0U;
    while ((cursor = strstr(cursor, text)) != NULL) {
        count++;
        cursor += strlen(text);
    }
    if (count == expected) return 0;
    printf("FAIL %s count=%u\n", name, count);
    return 1;
}

int main(void)
{
    static const char *const header_api[] = {
        "selection_textarea_adapter_ops", "selection_textarea_adapter_set_callbacks",
        "selection_textarea_adapter_attach", "selection_textarea_adapter_detach",
        "selection_textarea_adapter_focus", "selection_textarea_adapter_blur"
    };
    static const char *const source_contract[] = {
        "utf8_char_to_byte", "utf8_byte_to_char", "lv_textarea_get_label(textarea)",
        "lv_label_get_letter_pos", "lv_label_get_letter_on",
        "lv_label_set_text_selection_start", "lv_label_set_text_selection_end",
        "end - start >= capacity", "LV_EVENT_INSERT", "LV_EVENT_DELETE",
        "if (fixed) lv_textarea_set_insert_replace(textarea, \"\")",
        "text_selection_replace_active", "g_active_controller->target = NULL",
        "g_active_controller->ops = NULL", "replacement_is_valid",
        "lv_textarea_get_one_line", "lv_textarea_get_accepted_chars",
        "utf8_character_allowed", "old_chars - (last - first) + added > max",
        "lv_obj_remove_event_dsc(textarea, insert_event)"
    };
    static const char *const keyboard_api[] = {
        "selection_keyboard_install", "selection_keyboard_focus",
        "selection_keyboard_blur", "selection_keyboard_register_target",
        "selection_keyboard_unregister_target", "selection_keyboard_show_message"
    };
    static const char *const selection_keys[] = {
        "\"|<\"", "\">|\"", "\"OK\"", "\"ABC\"",
        "LV_SYMBOL_UP", "LV_SYMBOL_DOWN", "LV_SYMBOL_LEFT", "LV_SYMBOL_RIGHT",
        "\\xE5\\xA4\\x8D\\xE5\\x88\\xB6", "\\xE7\\xB2\\x98\\xE8\\xB4\\xB4",
        "\\xE5\\x8F\\x96\\xE6\\xB6\\x88\\xE9\\x80\\x89\\xE4\\xB8\\xAD"
    };
    static const char *const upper_rows[] = {
        "\"1#\", \"Q\", \"W\", \"E\", \"R\", \"T\", \"Y\", \"U\", \"I\", \"O\", \"P\", LV_SYMBOL_BACKSPACE",
        "\"abc\", \"A\", \"S\", \"D\", \"F\", \"G\", \"H\", \"J\", \"K\", \"L\", LV_SYMBOL_NEW_LINE",
        "\"_\", \":\", \"Z\", \"X\", \"C\", \"V\", \"B\", \"N\", \"M\", \"\\\"\", \",\", \";\"",
        "SELECTION_KEY_ENTRY,", "LV_SYMBOL_LEFT, \" \", LV_SYMBOL_RIGHT, LV_SYMBOL_OK"
    };
    static const char *const selection_rows[] = {
        "\"|<\", LV_SYMBOL_UP, \">|\", SELECTION_KEY_CANCEL, \"\\n\"",
        "LV_SYMBOL_LEFT, \"OK\", LV_SYMBOL_RIGHT, \" \", \"\\n\"",
        "SELECTION_KEY_COPY, LV_SYMBOL_DOWN, SELECTION_KEY_PASTE, \"ABC\", \"\""
    };
    char *header = read_file("lvgl_GUI/custom/selection_textarea_adapter.h");
    char *source = read_file("lvgl_GUI/custom/selection_textarea_adapter.c");
    char *keyboard_header = read_file("lvgl_GUI/custom/selection_keyboard.h");
    char *keyboard_source = read_file("lvgl_GUI/custom/selection_keyboard.c");
    char *maps_header = read_file("lvgl_GUI/custom/selection_keyboard_maps.h");
    char *maps_source = read_file("lvgl_GUI/custom/selection_keyboard_maps.c");
    unsigned int index;
    int failures = 0;
    if (header == NULL || source == NULL || keyboard_header == NULL ||
        keyboard_source == NULL || maps_header == NULL || maps_source == NULL) {
        free(header);
        free(source);
        free(keyboard_header);
        free(keyboard_source);
        free(maps_header);
        free(maps_source);
        printf("FAIL production_files_missing\n");
        return 1;
    }
    for (index = 0U; index < sizeof(header_api) / sizeof(header_api[0]); index++)
        failures += require_text(header, "adapter_api", header_api[index]);
    for (index = 0U; index < sizeof(source_contract) / sizeof(source_contract[0]); index++)
        failures += require_text(source, "adapter_contract", source_contract[index]);
    failures += require_order(source, "internal_guard_before_delete",
                              "g_internal_insert = true", "lv_textarea_delete_char(textarea)");
    failures += require_order(source, "internal_guard_after_insert",
                              "lv_textarea_add_text(textarea", "g_internal_insert = false");
    failures += require_order(source, "precheck_before_delete",
                              "if (!replacement_is_valid", "g_internal_insert = true");
    failures += forbid_text(source, "no_private_textarea", "lv_textarea_private.h");
    failures += forbid_text(source, "no_generic_child_lookup", "lv_obj_get_child(textarea");
    failures += forbid_text(source, "no_unbounded_copy", "strcpy(");
    for (index = 0U; index < sizeof(keyboard_api) / sizeof(keyboard_api[0]); index++)
        failures += require_text(keyboard_header, "keyboard_api", keyboard_api[index]);
    for (index = 0U; index < 8U; index++)
        failures += require_text(maps_source, "selection_key", selection_keys[index]);
    for (index = 8U; index < sizeof(selection_keys) / sizeof(selection_keys[0]); index++)
        failures += require_text(maps_header, "selection_key_macro", selection_keys[index]);
    for (index = 0U; index < sizeof(upper_rows) / sizeof(upper_rows[0]); index++)
        failures += require_text(maps_source, "uppercase_map", upper_rows[index]);
    for (index = 0U; index < sizeof(selection_rows) / sizeof(selection_rows[0]); index++)
        failures += require_text(maps_source, "selection_layout", selection_rows[index]);
    failures += require_text(maps_source, "selection_hidden_spacer",
                             "LV_BUTTONMATRIX_CTRL_HIDDEN | 2");
    failures += require_text(maps_header, "ascii_entry", "#define SELECTION_KEY_ENTRY \"SEL\"");
    failures += require_text(keyboard_source, "selection_font", "&lv_font_myyh_18");
    failures += require_count(keyboard_source, "single_clipboard_allocation",
                              "lv_malloc(TEXT_SELECTION_CLIPBOARD_BYTES + 1U)", 1U);
    failures += require_order(keyboard_source, "extension_before_default",
                              "extension_event = lv_obj_add_event_cb(kb, keyboard_event_cb",
                              "new_default = lv_obj_add_event_cb(kb, lv_keyboard_def_event_cb");
    failures += require_order(keyboard_source, "remove_old_after_new_default",
                              "new_default = lv_obj_add_event_cb", "lv_obj_remove_event(kb, old_index)");
    failures += require_order(keyboard_source, "maps_after_old_removal",
                              "lv_obj_remove_event(kb, old_index)", "lv_keyboard_set_map(kb");
    failures += require_text(keyboard_source, "find_default_count", "lv_obj_get_event_count(kb)");
    failures += require_text(keyboard_source, "find_default_dsc", "lv_obj_get_event_dsc(kb, index)");
    failures += require_text(keyboard_source, "find_default_cb", "lv_event_dsc_get_cb(event)");
    failures += require_text(keyboard_source, "extension_checked", "if (extension_event == NULL)");
    failures += require_text(keyboard_source, "new_default_checked", "if (new_default == NULL)");
    failures += require_text(keyboard_source, "new_default_rollback",
                             "lv_obj_remove_event_dsc(kb, new_default)");
    failures += require_text(keyboard_source, "extension_rollback",
                             "lv_obj_remove_event_dsc(kb, extension_event)");
    failures += require_text(keyboard_source, "stop_custom_key", "lv_event_stop_processing(e)");
    failures += require_text(keyboard_source, "adapter_include", "selection_textarea_adapter.h");
    failures += require_text(keyboard_source, "adapter_builtin_ops", "selection_textarea_adapter_ops()");
    failures += require_text(keyboard_source, "adapter_attach", "selection_textarea_adapter_attach(ta)");
    failures += require_text(keyboard_source, "adapter_focus", "selection_textarea_adapter_focus(ta, &g_controller)");
    failures += require_text(keyboard_source, "custom_context", "entry->context, entry->ops");
    failures += require_text(keyboard_source, "editable_only", "LV_STATE_DISABLED");
    failures += require_text(keyboard_source, "maps_include", "selection_keyboard_maps.h");
    failures += require_text(keyboard_source, "upper_map_getter", "selection_keyboard_maps_upper()");
    failures += require_text(keyboard_source, "selection_map_getter", "selection_keyboard_maps_selection()");
    failures += require_text(keyboard_source, "delete_unbind", "lv_keyboard_set_textarea(g_keyboard, NULL)");
    failures += require_text(keyboard_source, "delete_context_clear", "g_targets[index].context = NULL");
    failures += require_text(keyboard_source, "delete_ops_clear", "g_targets[index].ops = NULL");
    failures += require_order(keyboard_source, "attach_before_registry_publish",
                              "selection_textarea_adapter_attach(ta)", "g_targets[index].textarea = ta");
    failures += forbid_text(keyboard_source, "no_duplicate_utf8", "utf8_char_to_byte");
    failures += forbid_text(keyboard_source, "no_private_keyboard", "lv_keyboard_private.h");
    failures += forbid_text(keyboard_source, "no_callback_bulk_remove", "lv_obj_remove_event_cb(");
    free(header);
    free(source);
    free(keyboard_header);
    free(keyboard_source);
    free(maps_header);
    free(maps_source);
    if (failures != 0) {
        printf("FAIL selection_keyboard_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS selection_keyboard_contract\n");
    return 0;
}
