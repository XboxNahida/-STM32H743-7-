#include <stdio.h>
#include <string.h>

#include "nes_platform.h"
#include "sdcard_manager.h"
#include "sdram_layout.h"
#include "shared_buffer.h"

typedef unsigned int UINT;
typedef uint32_t FSIZE_t;
typedef int FRESULT;
typedef struct { uint32_t unused; } FIL;

static uint8_t g_read_buffer[16];
static int g_session_result;
static int g_open_result;
static int g_read_result;
static int g_close_result;
static int g_release_result;
static uint32_t g_file_size;
static uint32_t g_read_size;
static int g_session_owned;
static int g_buffer_owned;

shared_buffer_result_t shared_buffer_acquire(shared_buffer_owner_t owner,
                                             uint32_t size, void **address)
{
    if ((owner != SHARED_BUFFER_OWNER_NES) ||
        (size != SDRAM_SHARED_WORK_SIZE) || g_buffer_owned)
    {
        return SHARED_BUFFER_BUSY;
    }
    g_buffer_owned = 1;
    *address = g_read_buffer;
    return SHARED_BUFFER_OK;
}

shared_buffer_result_t shared_buffer_release(shared_buffer_owner_t owner)
{
    if ((owner != SHARED_BUFFER_OWNER_NES) || !g_buffer_owned)
    {
        return SHARED_BUFFER_NOT_OWNER;
    }
    g_buffer_owned = 0;
    return SHARED_BUFFER_OK;
}

uint32_t shared_buffer_get_size(void) { return SDRAM_SHARED_WORK_SIZE; }

int sdcard_manager_session_acquire(uint16_t owner)
{
    (void)owner;
    if (g_session_result != 0)
    {
        return g_session_result;
    }
    g_session_owned = 1;
    return SDCARD_MANAGER_OK;
}

int sdcard_manager_session_release(uint16_t owner)
{
    (void)owner;
    g_session_owned = 0;
    return g_release_result;
}

FRESULT f_open(FIL *file, const char *path, unsigned int mode)
{
    (void)file; (void)path; (void)mode;
    return g_open_result;
}

FRESULT f_read(FIL *file, void *data, UINT requested, UINT *actual)
{
    (void)file;
    memset(data, 0xA5, requested);
    *actual = (UINT)g_read_size;
    return g_read_result;
}

FRESULT f_close(FIL *file)
{
    (void)file;
    return g_close_result;
}

FSIZE_t nes_platform_test_file_size(FIL *file)
{
    (void)file;
    return g_file_size;
}

static void reset_fixture(void)
{
    g_session_result = 0; g_open_result = 0; g_read_result = 0;
    g_close_result = 0; g_release_result = 0; g_file_size = 32784U;
    g_read_size = 16U; g_session_owned = 0; g_buffer_owned = 0;
}

static int expect(const char *name, int condition)
{
    if (!condition)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    uint8_t *workspace;
    uint16_t *video_a;
    uint16_t *video_b;
    uint32_t size;
    uint32_t actual;
    int failures = 0;

    reset_fixture();
    failures += expect("workspace acquire",
        nes_platform_workspace_acquire(&workspace, &size) == 0 &&
        (uintptr_t)workspace == SDRAM_NES_WORK_ADDR &&
        size == SDRAM_NES_WORK_SIZE && !g_buffer_owned);
    failures += expect("workspace repeat rejected",
        nes_platform_workspace_acquire(&workspace, &size) != 0);
    nes_platform_workspace_release();
    failures += expect("workspace reacquire after release",
        nes_platform_workspace_acquire(&workspace, &size) == 0 &&
        (uintptr_t)workspace == SDRAM_NES_WORK_ADDR &&
        size == SDRAM_NES_WORK_SIZE);
    nes_platform_workspace_release();

    g_session_result = SDCARD_MANAGER_NOT_READY;
    failures += expect("session failure",
        nes_platform_rom_open("x", &size) != 0 && !g_session_owned);
    reset_fixture();
    g_open_result = 1;
    failures += expect("open releases session",
        nes_platform_rom_open("x", &size) != 0 && !g_session_owned);

    reset_fixture();
    failures += expect("open success",
        nes_platform_rom_open("x", &size) == 0 &&
        size == g_file_size && g_session_owned);
    failures += expect("read success",
        nes_platform_rom_read(g_read_buffer, 16U, &actual) == 0 &&
        actual == 16U && g_read_buffer[0] == 0xA5U);
    g_close_result = 1;
    failures += expect("close failure still releases",
        nes_platform_rom_close() != 0 && !g_session_owned);

    failures += expect("video buffers",
        nes_platform_get_video_buffers(&video_a, &video_b, &size) == 0 &&
        (uintptr_t)video_a == SDRAM_VIDEO_RGB_A_ADDR &&
        (uintptr_t)video_b == SDRAM_VIDEO_RGB_B_ADDR &&
        size >= (256U * 240U));

    if (failures == 0)
    {
        printf("PASS nes_platform\n");
    }
    return failures;
}
