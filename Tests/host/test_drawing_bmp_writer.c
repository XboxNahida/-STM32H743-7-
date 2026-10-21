#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "drawing_bmp_writer.h"

static uint8_t g_file_data[256];
static uint32_t g_file_size;
static uint32_t g_stat_calls;
static uint32_t g_close_calls;
static uint32_t g_release_calls;
static uint32_t g_remove_calls;
static int g_acquire_result;
static int g_stat_error;
static int g_write_fail_after;

int drawing_bmp_test_session_acquire(void)
{
    return g_acquire_result;
}

void drawing_bmp_test_session_release(void)
{
    g_release_calls++;
}

int drawing_bmp_test_mkdir(const char *path)
{
    return (strcmp(path, "0:/PICTURE") == 0) ? 0 : 1;
}

int drawing_bmp_test_stat(const char *path)
{
    (void)path;
    g_stat_calls++;
    if (g_stat_error != 0) return 2;
    return (g_stat_calls == 1U) ? 0 : 1;
}

int drawing_bmp_test_open(const char *path)
{
    return (strcmp(path, "0:/PICTURE/DRAW0002.BMP") == 0) ? 0 : 1;
}

int drawing_bmp_test_write(const void *data, uint32_t size,
                           uint32_t *written)
{
    if ((g_write_fail_after >= 0) &&
        (g_file_size >= (uint32_t)g_write_fail_after))
    {
        *written = 0U;
        return 1;
    }
    if ((g_file_size + size) > sizeof(g_file_data))
    {
        *written = 0U;
        return 1;
    }
    (void)memcpy(&g_file_data[g_file_size], data, size);
    g_file_size += size;
    *written = size;
    return 0;
}

int drawing_bmp_test_close(void)
{
    g_close_calls++;
    return 0;
}

void drawing_bmp_test_remove(const char *path)
{
    if (strcmp(path, "0:/PICTURE/DRAW0002.BMP") == 0)
    {
        g_remove_calls++;
    }
}

static void reset_mocks(void)
{
    (void)memset(g_file_data, 0, sizeof(g_file_data));
    g_file_size = 0U;
    g_stat_calls = 0U;
    g_close_calls = 0U;
    g_release_calls = 0U;
    g_remove_calls = 0U;
    g_acquire_result = 0;
    g_stat_error = 0;
    g_write_fail_after = -1;
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n", name,
               (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

static int expect_bytes(const char *name, const uint8_t *actual,
                        const uint8_t *expected, uint32_t size)
{
    if (memcmp(actual, expected, size) != 0)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    static const uint8_t expected_pixels[16] = {
        0xFFU, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U,
        0x00U, 0x00U, 0xFFU, 0x00U, 0xFFU, 0x00U, 0x00U, 0x00U
    };
    static const uint8_t pixels[8] = {
        0x00U, 0xF8U, 0xE0U, 0x07U,
        0x1FU, 0x00U, 0xFFU, 0xFFU
    };
    char path[DRAWING_BMP_PATH_CAPACITY];
    int failures = 0;
    int result;

    reset_mocks();
    result = drawing_bmp_save_next(pixels, 2U, 2U, 4U,
                                   path, sizeof(path));
    failures += expect_u32("save_result", (uint32_t)result,
                           DRAWING_BMP_OK);
    failures += expect_u32("file_size", g_file_size, 70U);
    failures += expect_u32("signature", read_le32(g_file_data) & 0xFFFFU,
                           0x4D42U);
    failures += expect_u32("header_file_size", read_le32(&g_file_data[2]),
                           70U);
    failures += expect_u32("pixel_offset", read_le32(&g_file_data[10]),
                           54U);
    failures += expect_u32("width", read_le32(&g_file_data[18]), 2U);
    failures += expect_u32("height", read_le32(&g_file_data[22]), 2U);
    failures += expect_bytes("bottom_up_bgr", &g_file_data[54],
                             expected_pixels, sizeof(expected_pixels));
    failures += expect_u32("close_success", g_close_calls, 1U);
    failures += expect_u32("release_success", g_release_calls, 1U);
    if (strcmp(path, "0:/PICTURE/DRAW0002.BMP") != 0)
    {
        printf("FAIL selected_path actual=%s\n", path);
        failures++;
    }

    reset_mocks();
    g_acquire_result = 1;
    result = drawing_bmp_save_next(pixels, 2U, 2U, 4U,
                                   path, sizeof(path));
    failures += expect_u32("not_ready", (uint32_t)result,
                           DRAWING_BMP_NOT_READY);
    failures += expect_u32("no_release_without_session", g_release_calls, 0U);

    reset_mocks();
    g_stat_error = 1;
    result = drawing_bmp_save_next(pixels, 2U, 2U, 4U,
                                   path, sizeof(path));
    failures += expect_u32("stat_error", (uint32_t)result,
                           DRAWING_BMP_IO_ERROR);
    failures += expect_u32("release_stat_error", g_release_calls, 1U);

    reset_mocks();
    g_write_fail_after = 54;
    result = drawing_bmp_save_next(pixels, 2U, 2U, 4U,
                                   path, sizeof(path));
    failures += expect_u32("write_error", (uint32_t)result,
                           DRAWING_BMP_IO_ERROR);
    failures += expect_u32("close_write_error", g_close_calls, 1U);
    failures += expect_u32("release_write_error", g_release_calls, 1U);
    failures += expect_u32("remove_partial", g_remove_calls, 1U);

    result = drawing_bmp_save_next(NULL, 2U, 2U, 4U,
                                   path, sizeof(path));
    failures += expect_u32("invalid_pixels", (uint32_t)result,
                           DRAWING_BMP_INVALID_ARG);

    if (failures != 0)
    {
        printf("FAIL drawing_bmp_writer failures=%d\n", failures);
        return 1;
    }
    printf("PASS drawing_bmp_writer\n");
    return 0;
}
