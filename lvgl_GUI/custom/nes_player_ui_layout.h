#ifndef NES_PLAYER_UI_LAYOUT_H
#define NES_PLAYER_UI_LAYOUT_H

static void nes_ui_build_game_controls(lv_obj_t *parent)
{
    static const char *const labels[NES_PLAYER_CONTROL_COUNT] =
    {
        "\xE9\x80\x89\xE6\x8B\xA9",
        "\xE5\xBC\x80\xE5\xA7\x8B",
        LV_SYMBOL_UP, LV_SYMBOL_DOWN, LV_SYMBOL_LEFT,
        "\xE6\x9A\x82\xE5\x81\x9C",
        LV_SYMBOL_RIGHT, "Y", "X", "B", "A",
        "\xE5\xB7\xA6\xE4\xB8\x8A",
        "\xE5\x8F\xB3\xE4\xB8\x8A",
        "\xE5\xB7\xA6\xE4\xB8\x8B",
        "\xE5\x8F\xB3\xE4\xB8\x8B"
    };
    const nes_player_control_region_t *layout;
    uint8_t count;
    uint8_t index;

    layout = nes_player_controls_get_layout(&count);
    for (index = 0U; index < count; ++index)
    {
        (void)nes_ui_button(parent, labels[index], layout[index].x,
            layout[index].y - 48, layout[index].width,
            layout[index].height);
    }
}

static void nes_ui_create_help(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, 774, 68);
    lv_obj_set_size(label, 240, 78);
    lv_label_set_text(label,
        "A/B \xE6\x99\xAE\xE9\x80\x9A\n"
        "X=B\xE8\xBF\x9E\xE5\x8F\x91 Y=A\xE8\xBF\x9E\xE5\x8F\x91\n"
        "\xE6\x9A\x82\xE5\x81\x9C/\xE7\xBB\xA7\xE7\xBB\xAD");
    lv_obj_set_style_text_font(label, &lv_font_myyh_20, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xaeb4bd), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
}

#endif
