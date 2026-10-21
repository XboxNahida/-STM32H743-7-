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

int main(void)
{
    char *header = read_file("lvgl_GUI/custom/notes_editor_ui.h");
    char *source = read_file("lvgl_GUI/custom/notes_editor_ui.c");
    char *target_header = read_file(
        "lvgl_GUI/custom/notes_editor_selection_target.h");
    char *target_source = read_file(
        "lvgl_GUI/custom/notes_editor_selection_target.c");
    char *guard_header = read_file(
        "lvgl_GUI/custom/notes_editor_refresh_guard.h");
    char *guard_source = read_file(
        "lvgl_GUI/custom/notes_editor_refresh_guard.c");
    int failures = 0;
    if (header == NULL || source == NULL || target_header == NULL ||
        target_source == NULL || guard_header == NULL || guard_source == NULL) {
        printf("FAIL production_files_missing\n");
        free(header);
        free(source);
        free(target_header);
        free(target_source);
        free(guard_header);
        free(guard_source);
        return 1;
    }

    failures += require_text(header, "attach api", "notes_editor_ui_attach");
    failures += require_text(header, "open api", "notes_editor_ui_open");
    failures += require_text(header, "save api", "notes_editor_ui_save");
    failures += require_text(header, "discard api", "notes_editor_ui_discard");
    failures += require_text(header, "fallible detach api",
                             "int notes_editor_ui_detach(void)");
    failures += require_text(source, "storage boundary", "notes_storage_open(");
    failures += require_text(source, "storage save", "notes_storage_save(");
    failures += require_text(source, "document window", "notes_document_build_window(");
    failures += require_text(source, "immediate commit", "notes_document_commit_window(");
    failures += require_order(source, "unchanged window skips commit",
                              "memcmp(text, g_editor.target.window.text",
                              "notes_document_commit_window(");
    failures += require_text(source, "window size contract", "NOTES_WINDOW_MAX_BYTES");
    failures += require_text(target_source, "overlap threshold", "NOTES_WINDOW_EDGE_BYTES");
    failures += require_text(target_source, "explicit empty document length",
                             "if (target == NULL || target->document == NULL) return 0U;");
    failures += require_text(guard_source, "async refresh retry", "lv_async_call(");
    failures += require_text(guard_source, "cancel stale async",
                             "lv_async_call_cancel(");
    failures += require_text(guard_source, "freeze pending textarea",
                             "lv_obj_add_state(");
    failures += require_text(guard_source, "restore textarea state",
                             "lv_obj_remove_state(");
    failures += require_text(source, "value event", "LV_EVENT_VALUE_CHANGED");
    failures += require_text(source, "programmatic guard", "programmatic_update");
    failures += require_order(source, "guard set text order",
                               "programmatic_update = true", "lv_textarea_set_text(");
    failures += require_text(source, "bulk load clears accepted chars",
                             "lv_textarea_set_accepted_chars(textarea, NULL)");
    failures += require_order(source, "bulk load disables character limit",
                              "lv_textarea_set_max_length(g_editor.textarea, 0U)",
                              "lv_textarea_set_text(g_editor.textarea, text)");
    failures += require_order(source, "bulk load restores character limit",
                              "lv_textarea_set_text(g_editor.textarea, text)",
                              "lv_textarea_set_max_length(g_editor.textarea, max_length)");
    failures += require_text(source, "custom target registration",
                             "selection_keyboard_register_target(");
    failures += require_text(target_header, "full target ops", "text_selection_target_ops_t");
    failures += require_text(target_source, "full read", "notes_document_read_range(");
    failures += require_text(target_source, "full replace", "notes_document_replace_range(");
    failures += require_text(target_source, "public label", "lv_textarea_get_label(");
    failures += require_text(target_source, "visual position", "lv_label_get_letter_pos(");
    failures += require_text(target_source, "visual hit test", "lv_label_get_letter_on(");
    failures += require_text(target_source, "selection intersection", "window_start");
    failures += require_text(target_source, "selection start", "lv_label_set_text_selection_start(");
    failures += require_text(target_source, "selection end", "lv_label_set_text_selection_end(");
    failures += require_order(source, "save commits first",
                              "static int commit_window", "notes_storage_save(");
    failures += require_text(source, "detach preserves commit failure",
                             "g_editor.target.document != NULL && commit_window() != 0");
    failures += require_text(source, "detach failure result",
                             "return NOTES_STORAGE_CAPACITY;");
    failures += forbid_text(source, "no FatFs include", "ff.h");
    failures += forbid_text(source, "no direct FatFs", "f_open(");
    failures += forbid_text(source, "no private textarea", "lv_textarea_private.h");
    failures += forbid_text(source, "no private textarea type", "lv_textarea_t");
    failures += forbid_text(target_source, "target no storage", "notes_storage_");
    failures += forbid_text(target_source, "target no private textarea",
                            "lv_textarea_private.h");
    failures += forbid_text(source, "ui no target callbacks", "target_move_vertical");

    free(header);
    free(source);
    free(target_header);
    free(target_source);
    free(guard_header);
    free(guard_source);
    if (failures != 0) return 1;
    printf("PASS notes_editor_ui_contract\n");
    return 0;
}
