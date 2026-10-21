#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nes_ppu_output.h"

static uint16_t g_frame[520U * 480U];
static uint8_t g_indices[256U];
static uint16_t g_palette[64U];

static int expect_true(const char *name, int condition)
{
    if (condition == 0) { printf("FAIL %s\n", name); return 1; }
    return 0;
}

int main(void)
{
    nes_ppu_output_t output;
    uint32_t i;
    int failures = 0;

    for (i = 0U; i < 64U; ++i)
    {
        g_palette[i] = (uint16_t)(0x1000U + i);
    }
    for (i = 0U; i < 256U; ++i)
    {
        g_indices[i] = (uint8_t)i;
    }
    memset(g_frame, 0xA5, sizeof(g_frame));

    failures += expect_true("bind_null_output",
        nes_ppu_output_bind(NULL, g_frame, 520U) ==
        NES_PPU_OUTPUT_INVALID_ARG);
    failures += expect_true("bind_null_pixels",
        nes_ppu_output_bind(&output, NULL, 520U) ==
        NES_PPU_OUTPUT_INVALID_ARG);
    failures += expect_true("bind_short_stride",
        nes_ppu_output_bind(&output, g_frame, 511U) ==
        NES_PPU_OUTPUT_INVALID_ARG);
    failures += expect_true("bind",
        nes_ppu_output_bind(&output, g_frame, 520U) == NES_PPU_OUTPUT_OK);
    failures += expect_true("write_line",
        nes_ppu_output_write_line(&output, 7U, g_indices, g_palette) ==
        NES_PPU_OUTPUT_OK);
    failures += expect_true("first_pixel",
                            g_frame[14U * 520U] == g_palette[0] &&
                            g_frame[14U * 520U + 1U] == g_palette[0] &&
                            g_frame[15U * 520U] == g_palette[0]);
    failures += expect_true("masked_palette_index",
                            g_frame[14U * 520U + 510U] == g_palette[63] &&
                            g_frame[14U * 520U + 511U] == g_palette[63] &&
                            g_frame[15U * 520U + 511U] == g_palette[63]);
    failures += expect_true("stride_guard",
        g_frame[14U * 520U + 512U] == 0xA5A5U &&
        g_frame[15U * 520U + 512U] == 0xA5A5U);
    failures += expect_true("last_line",
        nes_ppu_output_write_line(&output, 239U, g_indices, g_palette) ==
        NES_PPU_OUTPUT_OK);
    failures += expect_true("line_oob",
        nes_ppu_output_write_line(&output, 240U, g_indices, g_palette) ==
        NES_PPU_OUTPUT_INVALID_ARG);
    failures += expect_true("null_indices",
        nes_ppu_output_write_line(&output, 0U, NULL, g_palette) ==
        NES_PPU_OUTPUT_INVALID_ARG);
    failures += expect_true("null_palette",
        nes_ppu_output_write_line(&output, 0U, g_indices, NULL) ==
        NES_PPU_OUTPUT_INVALID_ARG);

    nes_ppu_output_unbind(&output);
    failures += expect_true("write_unbound",
        nes_ppu_output_write_line(&output, 0U, g_indices, g_palette) ==
        NES_PPU_OUTPUT_NOT_BOUND);

    if (failures != 0) {
        printf("FAIL nes_ppu_output failures=%d\n", failures);
        return 1;
    }
    printf("PASS nes_ppu_output\n");
    return 0;
}
