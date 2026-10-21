#include "nes_ppu_output.h"

#include <stddef.h>

int nes_ppu_output_bind(nes_ppu_output_t *output,
                        uint16_t *pixels,
                        uint16_t stride_pixels)
{
    if ((output == NULL) || (pixels == NULL) ||
        (stride_pixels < NES_PPU_FRAME_WIDTH))
    {
        return NES_PPU_OUTPUT_INVALID_ARG;
    }
    output->pixels = pixels;
    output->stride_pixels = stride_pixels;
    return NES_PPU_OUTPUT_OK;
}

void nes_ppu_output_unbind(nes_ppu_output_t *output)
{
    if (output != NULL)
    {
        output->pixels = NULL;
        output->stride_pixels = 0U;
    }
}

int nes_ppu_output_write_line(nes_ppu_output_t *output,
                              uint16_t line,
                              const uint8_t indices[NES_PPU_OUTPUT_WIDTH],
                              const uint16_t palette[64])
{
    uint16_t *destination;
    uint32_t column;

    if ((output == NULL) || (indices == NULL) || (palette == NULL) ||
        (line >= NES_PPU_OUTPUT_HEIGHT))
    {
        return NES_PPU_OUTPUT_INVALID_ARG;
    }
    if ((output->pixels == NULL) ||
        (output->stride_pixels < NES_PPU_FRAME_WIDTH))
    {
        return NES_PPU_OUTPUT_NOT_BOUND;
    }

    destination = output->pixels +
                  (uint32_t)line * NES_PPU_OUTPUT_SCALE *
                  output->stride_pixels;
    for (column = 0U; column < NES_PPU_OUTPUT_WIDTH; ++column)
    {
        uint16_t color = palette[indices[column] & 0x3FU];
        uint32_t output_column = column * NES_PPU_OUTPUT_SCALE;

        destination[output_column] = color;
        destination[output_column + 1U] = color;
        destination[output->stride_pixels + output_column] = color;
        destination[output->stride_pixels + output_column + 1U] = color;
    }
    return NES_PPU_OUTPUT_OK;
}
