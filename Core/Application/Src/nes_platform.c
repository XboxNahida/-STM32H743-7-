#include "nes_platform.h"

#include <stddef.h>

#include "sdcard_manager.h"
#include "sdram_layout.h"

#define NES_PLATFORM_SD_OWNER 0x4E45U

#ifdef NES_PLATFORM_HOST_TEST
typedef unsigned int UINT;
typedef uint32_t FSIZE_t;
typedef int FRESULT;
typedef struct { uint32_t unused; } FIL;

#define FR_OK   0
#define FA_READ 1U

extern FRESULT f_open(FIL *file, const char *path, unsigned int mode);
extern FRESULT f_read(FIL *file, void *data, UINT requested, UINT *actual);
extern FRESULT f_close(FIL *file);
extern FSIZE_t nes_platform_test_file_size(FIL *file);

static FSIZE_t nes_platform_file_size(FIL *file)
{
    return nes_platform_test_file_size(file);
}
#else
#include "ff.h"

static FSIZE_t nes_platform_file_size(FIL *file)
{
    return f_size(file);
}
#endif

static FIL g_nes_rom_file;
static uint8_t g_nes_workspace_owned;
static uint8_t g_nes_session_owned;
static uint8_t g_nes_file_opened;

int nes_platform_workspace_acquire(uint8_t **address, uint32_t *size)
{
    if ((address == NULL) || (size == NULL))
    {
        return 1;
    }
    *address = NULL;
    *size = 0U;
    if (g_nes_workspace_owned != 0U)
    {
        return 1;
    }
    g_nes_workspace_owned = 1U;
    *address = (uint8_t *)SDRAM_NES_WORK_ADDR;
    *size = SDRAM_NES_WORK_SIZE;
    return 0;
}

void nes_platform_workspace_release(void)
{
    g_nes_workspace_owned = 0U;
}

int nes_platform_rom_open(const char *path, uint32_t *size)
{
    FSIZE_t file_size;

    if ((path == NULL) || (path[0] == '\0') || (size == NULL) ||
        (g_nes_session_owned != 0U) || (g_nes_file_opened != 0U))
    {
        return 1;
    }
    *size = 0U;
    if (sdcard_manager_session_acquire(NES_PLATFORM_SD_OWNER) !=
        SDCARD_MANAGER_OK)
    {
        return 1;
    }
    g_nes_session_owned = 1U;
    if (f_open(&g_nes_rom_file, path, FA_READ) != FR_OK)
    {
        (void)sdcard_manager_session_release(NES_PLATFORM_SD_OWNER);
        g_nes_session_owned = 0U;
        return 1;
    }
    g_nes_file_opened = 1U;
    file_size = nes_platform_file_size(&g_nes_rom_file);
    if ((file_size == 0U) || (file_size > UINT32_MAX))
    {
        (void)nes_platform_rom_close();
        return 1;
    }
    *size = (uint32_t)file_size;
    return 0;
}

int nes_platform_rom_read(uint8_t *data, uint32_t requested,
                          uint32_t *actual)
{
    UINT bytes_read = 0U;
    FRESULT result;

    if ((data == NULL) || (requested == 0U) || (actual == NULL) ||
        (g_nes_file_opened == 0U))
    {
        return 1;
    }
    *actual = 0U;
    result = f_read(&g_nes_rom_file, data, (UINT)requested, &bytes_read);
    *actual = (uint32_t)bytes_read;
    return (result == FR_OK) ? 0 : 1;
}

int nes_platform_rom_close(void)
{
    int failed = 0;

    if (g_nes_file_opened != 0U)
    {
        if (f_close(&g_nes_rom_file) != FR_OK)
        {
            failed = 1;
        }
        g_nes_file_opened = 0U;
    }
    if (g_nes_session_owned != 0U)
    {
        if (sdcard_manager_session_release(NES_PLATFORM_SD_OWNER) !=
            SDCARD_MANAGER_OK)
        {
            failed = 1;
        }
        g_nes_session_owned = 0U;
    }
    return failed;
}

int nes_platform_get_video_buffers(uint16_t **buffer_a,
                                   uint16_t **buffer_b,
                                   uint32_t *pixels_per_buffer)
{
    if ((buffer_a == NULL) || (buffer_b == NULL) ||
        (pixels_per_buffer == NULL))
    {
        return 1;
    }
    *buffer_a = (uint16_t *)SDRAM_VIDEO_RGB_A_ADDR;
    *buffer_b = (uint16_t *)SDRAM_VIDEO_RGB_B_ADDR;
    *pixels_per_buffer = SDRAM_VIDEO_RGB_FRAME_SIZE / sizeof(uint16_t);
    return 0;
}
