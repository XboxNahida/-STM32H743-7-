#include "test_notes_storage_fixture.h"

#include <stdio.h>
#include <string.h>

#include "notes_storage_service.h"

uint8_t g_shared[TEST_SHARED_BYTES];
uint8_t g_file_data[TEST_FILE_BYTES];
uint8_t g_written[TEST_FILE_BYTES];
uint32_t g_file_size;
uint32_t g_shared_size;
uint32_t g_written_size;
uint32_t g_short_write_limit;
int g_acquire_result;
int g_open_read_result;
int g_open_write_result;
int g_read_result;
int g_write_result;
int g_close_result;
uint32_t g_acquire_calls;
uint32_t g_release_calls;
uint32_t g_open_write_calls;
static uint32_t g_read_offset;

int notes_storage_host_acquire(uint8_t **address, uint32_t *size)
{
    g_acquire_calls++;
    if (g_acquire_result != 0)
    {
        return g_acquire_result;
    }
    *address = g_shared;
    *size = g_shared_size;
    return 0;
}

void notes_storage_host_release(void)
{
    g_release_calls++;
}

int notes_storage_host_open_read(const char *path)
{
    (void)path;
    g_read_offset = 0U;
    return g_open_read_result;
}

int notes_storage_host_open_write(const char *path)
{
    (void)path;
    g_open_write_calls++;
    g_written_size = 0U;
    return g_open_write_result;
}

uint32_t notes_storage_host_size(void)
{
    return g_file_size;
}

int notes_storage_host_read(uint8_t *data, uint32_t requested,
                            uint32_t *actual)
{
    uint32_t remaining;

    if (g_read_result != 0)
    {
        return g_read_result;
    }
    remaining = g_file_size - g_read_offset;
    if (requested > remaining)
    {
        requested = remaining;
    }
    memcpy(data, g_file_data + g_read_offset, requested);
    g_read_offset += requested;
    *actual = requested;
    return 0;
}

int notes_storage_host_write(const uint8_t *data, uint32_t requested,
                             uint32_t *actual)
{
    if (g_write_result != 0)
    {
        return g_write_result;
    }
    if ((g_short_write_limit != 0U) && (requested > g_short_write_limit))
    {
        requested = g_short_write_limit;
    }
    memcpy(g_written + g_written_size, data, requested);
    g_written_size += requested;
    *actual = requested;
    return 0;
}

int notes_storage_host_close(void)
{
    return g_close_result;
}

uint16_t notes_storage_host_oem2uni(uint16_t value)
{
    if (value == 0xD6D0U)
    {
        return 0x4E2DU;
    }
    if (value == 0xCEC4U)
    {
        return 0x6587U;
    }
    return 0U;
}

void reset_fixture(void)
{
    memset(g_shared, 0, sizeof(g_shared));
    memset(g_file_data, 0, sizeof(g_file_data));
    memset(g_written, 0, sizeof(g_written));
    g_file_size = 0U;
    g_shared_size = sizeof(g_shared);
    g_read_offset = 0U;
    g_written_size = 0U;
    g_short_write_limit = 0U;
    g_acquire_result = 0;
    g_open_read_result = 0;
    g_open_write_result = 0;
    g_read_result = 0;
    g_write_result = 0;
    g_close_result = 0;
    g_acquire_calls = 0U;
    g_release_calls = 0U;
    g_open_write_calls = 0U;
    notes_storage_reset_for_test();
}

int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n", name,
               (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}
