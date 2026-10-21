#ifndef NOTES_STORAGE_PLATFORM_H
#define NOTES_STORAGE_PLATFORM_H

#include <stdint.h>

int notes_platform_acquire(uint8_t **address, uint32_t *size);
void notes_platform_release(void);
int notes_platform_open_read(const char *path);
int notes_platform_open_write(const char *path);
uint32_t notes_platform_size(void);
int notes_platform_read(uint8_t *data, uint32_t requested, uint32_t *actual);
int notes_platform_write(const uint8_t *data, uint32_t requested,
                         uint32_t *actual);
int notes_platform_close(void);
uint16_t notes_platform_oem2uni(uint16_t value);

#endif
