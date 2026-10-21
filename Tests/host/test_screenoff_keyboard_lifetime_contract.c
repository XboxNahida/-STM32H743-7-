#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *text;

    if (file == NULL) return NULL;
    if (fseek(file, 0L, SEEK_END) != 0 || (size = ftell(file)) < 0L ||
        fseek(file, 0L, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    text = malloc((size_t)size + 1U);
    if (text == NULL || fread(text, 1U, (size_t)size, file) != (size_t)size) {
        free(text);
        fclose(file);
        return NULL;
    }
    text[size] = '\0';
    fclose(file);
    return text;
}

static int expect(const char *name, int condition)
{
    if (condition) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

int main(void)
{
    char *source = read_source("lvgl/porting/lv_port_indev.c");
    char *header = read_source("lvgl/porting/lv_port_indev.h");
    char *custom = read_source("lvgl_GUI/custom/custom.c");
    int failures = 0;
    char *wake_handler;
    char *wake_handler_end;

    failures += expect("read_source", source != NULL);
    failures += expect("read_header", header != NULL);
    failures += expect("read_custom", custom != NULL);
    if (source != NULL) {
        wake_handler = strstr(source,
            "static void touchpad_read(lv_indev_t * indev_drv");
        wake_handler_end = (wake_handler != NULL) ?
            strstr(wake_handler, "/* \xE6\xA3\x80\xE6\xB5\x8B\xE6\x89\x8B\xE5\x8A\xBF */") : NULL;
        failures += expect("wake_handler_found",
            wake_handler != NULL && wake_handler_end != NULL);
        if (wake_handler != NULL && wake_handler_end != NULL) {
            char saved = *wake_handler_end;
            *wake_handler_end = '\0';
            failures += expect("input_callback_does_not_modify_ui",
                strstr(wake_handler, "g_lock_screen_request") == NULL &&
                strstr(wake_handler, "lv_obj_clean(") == NULL &&
                strstr(wake_handler, "ui_load_scr_animation(") == NULL);
            *wake_handler_end = saved;
        }
        failures += expect("screenoff_queues_lock_before_wake",
            strstr(source, "g_lock_screen_request = 1U") != NULL &&
            strstr(source, "case 1: /* \xE4\xBD\x8E\xE4\xBA\xAE\xE4\xB8\xAD") != NULL);
        failures += expect("screenoff_hides_global_keyboard",
            strstr(source, "screen_off_hide_input_overlays") != NULL &&
            strstr(source, "lv_keyboard_set_textarea(keyboard, NULL)") != NULL &&
            strstr(source, "lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN)") != NULL);
        failures += expect("screenoff_clears_ime_target",
            strstr(source, "lv_ime_pinyin_set_textarea(g_kb_ime, NULL)") != NULL &&
            strstr(source, "lv_ime_pinyin_get_cand_panel(g_kb_ime)") != NULL);
    }
    if (header != NULL)
        failures += expect("public_power_processor",
            strstr(header, "lv_port_indev_process_power_request") != NULL);
    if (custom != NULL)
        failures += expect("power_processed_before_lvgl_timer",
            strstr(custom, "lv_port_indev_process_power_request();") != NULL);

    free(source);
    free(header);
    free(custom);
    if (failures != 0) {
        printf("screenoff_keyboard_lifetime_contract failures=%d\n", failures);
        return 1;
    }
    puts("PASS screenoff_keyboard_lifetime_contract");
    return 0;
}
