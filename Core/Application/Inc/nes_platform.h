#ifndef NES_PLATFORM_H
#define NES_PLATFORM_H

#include <stdint.h>

int nes_platform_workspace_acquire(uint8_t **address, uint32_t *size);
void nes_platform_workspace_release(void);
int nes_platform_rom_open(const char *path, uint32_t *size);
int nes_platform_rom_read(uint8_t *data, uint32_t requested,
                          uint32_t *actual);
int nes_platform_rom_close(void);
int nes_platform_get_video_buffers(uint16_t **buffer_a,
                                   uint16_t **buffer_b,
                                   uint32_t *pixels_per_buffer);

#endif
