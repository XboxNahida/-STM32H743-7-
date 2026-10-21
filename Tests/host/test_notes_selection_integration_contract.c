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
    if (source != NULL && strstr(source, text) != NULL) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

static int forbid_text(const char *source, const char *name, const char *text)
{
    if (source != NULL && strstr(source, text) == NULL) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

int main(void)
{
    char *setup = read_file("lvgl_GUI/generated/setup_scr_screen_notes.c");
    char *events = read_file("lvgl_GUI/generated/events_init.c");
    int failures = 0;

    failures += require_text(setup, "bridge include", "notes_editor_ui.h");
    failures += require_text(setup, "document include", "notes_document.h");
    failures += require_text(setup, "window maximum", "NOTES_WINDOW_MAX_BYTES");
    failures += require_text(setup, "attach", "notes_editor_ui_attach(");
    failures += forbid_text(setup, "no 4096 limit", "set_max_length(ui->screen_notes_ta_note, 4096)");
    failures += require_text(events, "events bridge include", "notes_editor_ui.h");
    failures += require_text(events, "new document", "notes_editor_ui_new(");
    failures += require_text(events, "open document", "notes_editor_ui_open(");
    failures += require_text(events, "save document", "notes_editor_ui_save(");
    failures += require_text(events, "discard document", "notes_editor_ui_discard(");
    failures += require_text(events, "fallible detach", "notes_editor_ui_detach() == NOTES_STORAGE_OK");
    failures += require_text(events, "status modified", "notes_storage_get_status(");
    failures += forbid_text(events, "no save text copy", "g_save_text_pending");
    failures += forbid_text(events, "no direct note textarea save",
                            "text = lv_textarea_get_text(guider_ui.screen_notes_ta_note)");
    failures += forbid_text(events, "no old note utf8 truncation",
                            "if (src_len > 16384) src_len = 16384");
    failures += forbid_text(events, "no old note gbk window",
                            "G_FILE_CONTENT_SIZE - 16384");
    failures += forbid_text(events, "no legacy modified flag", "g_notes_modified");
    failures += require_text(events, "image path preserved", "image_viewer_open_album(");
    failures += require_text(events, "avi path preserved", "avi_player_open(");
    failures += require_text(events, "music path preserved", "music_player_ui_request_open(");
    failures += require_text(setup, "setup patch marker", "STAGE6 NOTES USER PATCH");
    failures += require_text(events, "events patch marker", "STAGE6 NOTES USER PATCH");

    free(setup);
    free(events);
    if (failures != 0) return 1;
    printf("PASS notes_selection_integration_contract\n");
    return 0;
}
