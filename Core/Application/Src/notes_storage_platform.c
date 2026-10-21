#include "notes_storage_platform.h"

#include <stddef.h>

#ifdef NOTES_STORAGE_HOST_TEST
extern int notes_storage_host_acquire(uint8_t **address, uint32_t *size);
extern void notes_storage_host_release(void);
extern int notes_storage_host_open_read(const char *path);
extern int notes_storage_host_open_write(const char *path);
extern uint32_t notes_storage_host_size(void);
extern int notes_storage_host_read(uint8_t *data, uint32_t requested,
                                   uint32_t *actual);
extern int notes_storage_host_write(const uint8_t *data, uint32_t requested,
                                    uint32_t *actual);
extern int notes_storage_host_close(void);
extern uint16_t notes_storage_host_oem2uni(uint16_t value);

int notes_platform_acquire(uint8_t **address, uint32_t *size)
{
    return notes_storage_host_acquire(address, size);
}

void notes_platform_release(void)
{
    notes_storage_host_release();
}

int notes_platform_open_read(const char *path)
{
    return notes_storage_host_open_read(path);
}

int notes_platform_open_write(const char *path)
{
    return notes_storage_host_open_write(path);
}

uint32_t notes_platform_size(void)
{
    return notes_storage_host_size();
}

int notes_platform_read(uint8_t *data, uint32_t requested, uint32_t *actual)
{
    return notes_storage_host_read(data, requested, actual);
}

int notes_platform_write(const uint8_t *data, uint32_t requested,
                         uint32_t *actual)
{
    return notes_storage_host_write(data, requested, actual);
}

int notes_platform_close(void)
{
    return notes_storage_host_close();
}

uint16_t notes_platform_oem2uni(uint16_t value)
{
    return notes_storage_host_oem2uni(value);
}
#else
#include "ff.h"
#include "shared_buffer.h"
#include "sdram_layout.h"

static FIL g_notes_file;

int notes_platform_acquire(uint8_t **address, uint32_t *size)
{
    void *buffer = NULL;
    shared_buffer_result_t result = shared_buffer_acquire(
        SHARED_BUFFER_OWNER_NOTES, SDRAM_SHARED_WORK_SIZE, &buffer);
    if (result != SHARED_BUFFER_OK)
    {
        return 1;
    }
    *address = (uint8_t *)buffer;
    *size = shared_buffer_get_size();
    return 0;
}

void notes_platform_release(void)
{
    (void)shared_buffer_release(SHARED_BUFFER_OWNER_NOTES);
}

int notes_platform_open_read(const char *path)
{
    return (f_open(&g_notes_file, path, FA_READ) == FR_OK) ? 0 : 1;
}

int notes_platform_open_write(const char *path)
{
    return (f_open(&g_notes_file, path, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) ?
        0 : 1;
}

uint32_t notes_platform_size(void)
{
    return (uint32_t)f_size(&g_notes_file);
}

int notes_platform_read(uint8_t *data, uint32_t requested, uint32_t *actual)
{
    UINT bytes_read = 0U;
    FRESULT result = f_read(&g_notes_file, data, (UINT)requested, &bytes_read);
    *actual = (uint32_t)bytes_read;
    return (result == FR_OK) ? 0 : 1;
}

int notes_platform_write(const uint8_t *data, uint32_t requested,
                         uint32_t *actual)
{
    UINT bytes_written = 0U;
    FRESULT result = f_write(&g_notes_file, data, (UINT)requested,
                             &bytes_written);
    *actual = (uint32_t)bytes_written;
    return (result == FR_OK) ? 0 : 1;
}

int notes_platform_close(void)
{
    return (f_close(&g_notes_file) == FR_OK) ? 0 : 1;
}

uint16_t notes_platform_oem2uni(uint16_t value)
{
    return (uint16_t)ff_oem2uni((WCHAR)value, 936U);
}
#endif
