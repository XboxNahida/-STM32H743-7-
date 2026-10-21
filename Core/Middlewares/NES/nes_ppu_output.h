#ifndef NES_PPU_OUTPUT_H
#define NES_PPU_OUTPUT_H

#include <stdint.h>

#define NES_PPU_OUTPUT_WIDTH   256U
#define NES_PPU_OUTPUT_HEIGHT  240U
#define NES_PPU_OUTPUT_SCALE   2U
#define NES_PPU_FRAME_WIDTH    (NES_PPU_OUTPUT_WIDTH * NES_PPU_OUTPUT_SCALE)
#define NES_PPU_FRAME_HEIGHT   (NES_PPU_OUTPUT_HEIGHT * NES_PPU_OUTPUT_SCALE)

typedef enum
{
    NES_PPU_OUTPUT_OK = 0,
    NES_PPU_OUTPUT_INVALID_ARG,
    NES_PPU_OUTPUT_NOT_BOUND
} nes_ppu_output_result_t;

typedef struct
{
    uint16_t *pixels;
    uint16_t stride_pixels;
} nes_ppu_output_t;

int nes_ppu_output_bind(nes_ppu_output_t *output,
                        uint16_t *pixels,
                        uint16_t stride_pixels);
void nes_ppu_output_unbind(nes_ppu_output_t *output);
int nes_ppu_output_write_line(nes_ppu_output_t *output,
                              uint16_t line,
                              const uint8_t indices[NES_PPU_OUTPUT_WIDTH],
                              const uint16_t palette[64]);

#endif
