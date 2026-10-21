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

static unsigned int count_text(const char *source, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);
    while ((source = strstr(source, needle)) != NULL) {
        count++;
        source += length;
    }
    return count;
}

static int require_text(const char *source, const char *name, const char *needle)
{
    if (source != NULL && strstr(source, needle) != NULL) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

static int require_order(const char *source, const char *name,
                         const char *first, const char *second)
{
    const char *left = source != NULL ? strstr(source, first) : NULL;
    const char *right = source != NULL ? strstr(source, second) : NULL;
    if (left != NULL && right != NULL && left < right) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

int main(void)
{
    char *ime_header = read_file("lvgl/src/widgets/ime/lv_ime_pinyin.h");
    char *guider = read_file("lvgl_GUI/generated/gui_guider.c");
    char *widgets = read_file("lvgl_GUI/generated/widgets_init.c");
    char *calculator = read_file(
        "lvgl_GUI/generated/setup_scr_screen_calculator.c");
    char *signal = read_file(
        "lvgl_GUI/generated/setup_scr_screen_Signal_Proc.c");
    int failures = 0;

    failures += require_text(ime_header, "IME target getter",
                             "lv_ime_pinyin_get_textarea");
    failures += require_text(guider, "guider include", "selection_keyboard.h");
    failures += require_text(widgets, "widgets include", "selection_keyboard.h");
    failures += require_text(guider, "install", "selection_keyboard_install(");
    if (guider == NULL || count_text(guider, "selection_keyboard_install(") != 1U) {
        printf("FAIL install once\n");
        failures++;
    }
    failures += require_order(guider, "install before generated callback",
        "selection_keyboard_install(ui->g_kb_top_layer)",
        "lv_obj_add_event_cb(ui->g_kb_top_layer, kb_event_cb");
    failures += require_order(guider, "capture configured normal font",
        "lv_obj_set_style_text_font(ui->g_kb_top_layer",
        "selection_keyboard_install(ui->g_kb_top_layer)");
    failures += require_text(widgets, "register ordinary target",
                             "selection_keyboard_register_target(ta, NULL, NULL)");
    failures += require_order(widgets, "bind before focus",
        "lv_keyboard_set_textarea(kb, ta)", "selection_keyboard_focus(ta)");
    failures += require_order(widgets, "blur before unbind",
        "selection_keyboard_blur(ta)", "lv_keyboard_set_textarea(kb, NULL)");
    failures += require_text(widgets, "delete branch", "code == LV_EVENT_DELETE");
    failures += require_text(widgets, "delete unregister",
                             "selection_keyboard_unregister_target(ta)");
    failures += require_text(widgets, "delete clears matching IME target",
        "lv_ime_pinyin_get_textarea(g_kb_ime) == ta");
    failures += require_order(widgets, "delete clears IME before unregister",
        "lv_ime_pinyin_set_textarea(g_kb_ime, NULL);\n        }\n#endif\n        selection_keyboard_unregister_target(ta)",
        "selection_keyboard_unregister_target(ta);");
    if (calculator == NULL || strstr(calculator, "ta_event_cb") != NULL) {
        printf("FAIL calculator excluded\n");
        failures++;
    }
    if (signal == NULL || strstr(signal, "Serial_Port_ta_display, ta_event_cb") != NULL) {
        printf("FAIL readonly serial excluded\n");
        failures++;
    }
    failures += require_text(guider, "generated patch marker",
                             "STAGE6 NOTES USER PATCH");
    failures += require_text(widgets, "widgets patch marker",
                             "STAGE6 NOTES USER PATCH");

    free(ime_header);
    free(guider);
    free(widgets);
    free(calculator);
    free(signal);
    if (failures != 0) return 1;
    printf("PASS selection_keyboard_integration_contract\n");
    return 0;
}
