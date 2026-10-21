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
    text = (char *)malloc((size_t)size + 1U);
    if (text == NULL || fread(text, 1U, (size_t)size, file) != (size_t)size) {
        free(text);
        fclose(file);
        return NULL;
    }
    text[size] = '\0';
    fclose(file);
    return text;
}

static int expect_true(const char *name, int value)
{
    if (value) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

static int function_has_no_delete(const char *source)
{
    const char *start = strstr(source, "static void lv_ime_pinyin_cand_panel_event(lv_event_t * e)\n{");
    const char *end;
    if (start == NULL) return 0;
    end = strstr(start + 1, "\nstatic void pinyin_input_proc");
    return end != NULL &&
           strstr(start, "lv_textarea_add_text(pinyin_ime->textarea, txt);") < end &&
           (strstr(start, "lv_textarea_delete_char") == NULL ||
            strstr(start, "lv_textarea_delete_char") >= end);
}

int main(void)
{
    char *header = read_source("lvgl/src/widgets/ime/lv_ime_pinyin.h");
    char *core = read_source("lvgl/src/widgets/ime/lv_ime_pinyin.c");
    char *gui = read_source("lvgl_GUI/generated/gui_guider.c");
    char *widgets = read_source("lvgl_GUI/generated/widgets_init.c");
    char *selection = read_source("lvgl_GUI/custom/selection_keyboard.c");
    int failures = 0;

    failures += expect_true("read_sources", header && core && gui && widgets && selection);
    if (header && core && gui && widgets && selection) {
        const char *raw_map = strstr(core,
            "lv_btnm_def_pinyin_sel_map[IME_PREEDIT_ID] = pinyin_ime->preedit_display;");
        const char *cn_map = strstr(core,
            "lv_btnm_def_pinyin_sel_map[IME_CHINESE_FIRST_ID + btnm_i]");
        failures += expect_true("preedit_capacity",
            strstr(header, "#define LV_IME_PINYIN_PREEDIT_MAX   63") != NULL);
        failures += expect_true("default_english",
            strstr(gui, "LV_IME_PINYIN_LANGUAGE_ENGLISH") != NULL &&
            strstr(core, "pinyin_ime->language = LV_IME_PINYIN_LANGUAGE_ENGLISH;") != NULL);
        failures += expect_true("single_language_toggle",
            strstr(core, "#define IME_LANGUAGE_ID") != NULL &&
            strstr(core, "#define IME_MODE_CHINESE_ID") == NULL &&
            strstr(core, "#define IME_MODE_ENGLISH_ID") == NULL &&
            strstr(core, "pinyin_ime->language == LV_IME_PINYIN_LANGUAGE_ENGLISH ?") != NULL &&
            strstr(core, "LV_IME_PINYIN_LANGUAGE_CHINESE :") != NULL);
        failures += expect_true("keyboard_mode_cycle_restored",
            strstr(core, "if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26)") != NULL &&
            strstr(core, "lv_ime_pinyin_set_mode(obj, LV_IME_PINYIN_MODE_K9);") != NULL &&
            strstr(core, "else if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9)") != NULL &&
            strstr(core, "lv_ime_pinyin_set_mode(obj, LV_IME_PINYIN_MODE_K26);") != NULL &&
            strstr(core, "LV_IME_PINYIN_MODE_K9_NUMBER") != NULL &&
            strstr(core, "lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);") != NULL);
        failures += expect_true("k9_input_path_restored",
            strstr(core, "pinyin_k9_get_legal_py(obj, pinyin_ime->k9_input_str, k9_py_map);") != NULL &&
            strstr(core, "pinyin_k9_fill_cand(obj);") != NULL &&
            strstr(core, "pinyin_k9_cand_page_proc(obj, 0U);") != NULL &&
            strstr(core, "pinyin_k9_cand_page_proc(obj, 1U);") != NULL);
        failures += expect_true("candidate_order", raw_map != NULL && cn_map != NULL && raw_map < cn_map);
        failures += expect_true("candidate_commit_only", function_has_no_delete(core));
        failures += expect_true("chinese_detaches_textarea",
            strstr(core, "static void pinyin_update_keyboard_target") != NULL &&
            strstr(core, "pinyin_ime->mode == LV_IME_PINYIN_MODE_K9_NUMBER") != NULL &&
            strstr(core, "pinyin_ime->language == LV_IME_PINYIN_LANGUAGE_ENGLISH") != NULL &&
            strstr(core, "direct_input ? pinyin_ime->textarea : NULL") != NULL);
        failures += expect_true("long_english_elision",
            strstr(core, "lv_memcpy(&pinyin_ime->preedit_display[10], \"...\", 3U);") != NULL &&
            strstr(core, "txt = pinyin_ime->preedit;") != NULL);
        failures += expect_true("empty_preedit_does_not_terminate_map",
            strstr(core, "pinyin_ime->preedit_display[0] = ' ';") != NULL);
        failures += expect_true("idempotent_keyboard_binding",
            strstr(core, "lv_obj_remove_event_cb_with_user_data") != NULL);
        failures += expect_true("candidate_row_style",
            strstr(gui, "lv_obj_set_height(cand, 58);") != NULL &&
            strstr(gui, "LV_PART_ITEMS | LV_STATE_CHECKED") != NULL &&
            strstr(gui, "lv_color_hex(0x2195f6)") != NULL);
        failures += expect_true("focus_defaults_and_shows",
            strstr(widgets, "LV_IME_PINYIN_LANGUAGE_ENGLISH") != NULL &&
            strstr(widgets, "lv_ime_pinyin_set_textarea(g_kb_ime, ta);") != NULL &&
            strstr(widgets, "lv_ime_pinyin_reset(g_kb_ime)") != NULL);
        failures += expect_true("blur_hides",
            strstr(widgets, "lv_ime_pinyin_set_textarea(g_kb_ime, NULL);") != NULL);
        failures += expect_true("selection_hides_and_restores",
            strstr(selection, "lv_ime_pinyin_get_cand_panel") != NULL &&
            strstr(selection, "LV_IME_PINYIN_LANGUAGE_ENGLISH") != NULL &&
            strstr(selection, "lv_ime_pinyin_set_textarea(g_kb_ime, g_target);") != NULL);
    }

    free(header);
    free(core);
    free(gui);
    free(widgets);
    free(selection);
    if (failures != 0) {
        printf("global_ime_contract failures=%d\n", failures);
        return 1;
    }
    puts("PASS global_ime_contract");
    return 0;
}
