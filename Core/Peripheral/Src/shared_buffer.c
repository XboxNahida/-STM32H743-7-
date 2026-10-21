#include "shared_buffer.h"

#include <stddef.h>
#include <stdio.h>

#include "sdram_layout.h"

static shared_buffer_owner_t g_shared_buffer_owner = SHARED_BUFFER_OWNER_NONE;

static const char *shared_buffer_owner_name(shared_buffer_owner_t owner)
{
    switch (owner)
    {
        case SHARED_BUFFER_OWNER_NONE:         return "none";
        case SHARED_BUFFER_OWNER_FILE_BROWSER: return "file_browser";
        case SHARED_BUFFER_OWNER_NOTES:        return "notes";
        case SHARED_BUFFER_OWNER_IMAGE_VIEWER: return "image_viewer";
        case SHARED_BUFFER_OWNER_AVI_PLAYER:   return "avi_player";
        case SHARED_BUFFER_OWNER_STORAGE_TEST: return "storage_test";
        case SHARED_BUFFER_OWNER_ALGORITHM:    return "algorithm";
        case SHARED_BUFFER_OWNER_NES:          return "nes";
        case SHARED_BUFFER_OWNER_WAV_PLAYER:   return "wav_player";
        case SHARED_BUFFER_OWNER_OSCILLOSCOPE: return "oscilloscope";
        default:                               return "invalid";
    }
}

shared_buffer_result_t shared_buffer_acquire(shared_buffer_owner_t owner,
                                             uint32_t requested_size,
                                             void **address)
{
    if (address == NULL)
    {
        return SHARED_BUFFER_INVALID_ARG;
    }

    *address = NULL;

    if ((owner <= SHARED_BUFFER_OWNER_NONE) ||
        (owner >= SHARED_BUFFER_OWNER_COUNT) ||
        (requested_size == 0U) ||
        (requested_size > SDRAM_SHARED_WORK_SIZE))
    {
        return SHARED_BUFFER_INVALID_ARG;
    }

    if ((g_shared_buffer_owner != SHARED_BUFFER_OWNER_NONE) &&
        (g_shared_buffer_owner != owner))
    {
        return SHARED_BUFFER_BUSY;
    }

    g_shared_buffer_owner = owner;
    *address = (void *)SDRAM_SHARED_WORK_ADDR;
    return SHARED_BUFFER_OK;
}

shared_buffer_result_t shared_buffer_release(shared_buffer_owner_t owner)
{
    if ((owner <= SHARED_BUFFER_OWNER_NONE) ||
        (owner >= SHARED_BUFFER_OWNER_COUNT))
    {
        return SHARED_BUFFER_INVALID_ARG;
    }

    if (g_shared_buffer_owner != owner)
    {
        return SHARED_BUFFER_NOT_OWNER;
    }

    g_shared_buffer_owner = SHARED_BUFFER_OWNER_NONE;
    return SHARED_BUFFER_OK;
}

shared_buffer_owner_t shared_buffer_get_owner(void)
{
    return g_shared_buffer_owner;
}

uint32_t shared_buffer_get_size(void)
{
    return SDRAM_SHARED_WORK_SIZE;
}

void shared_buffer_print_status(void)
{
    printf("buffer owner=%s address=0x%08lX size=%lu\r\n",
           shared_buffer_owner_name(g_shared_buffer_owner),
           (unsigned long)SDRAM_SHARED_WORK_ADDR,
           (unsigned long)SDRAM_SHARED_WORK_SIZE);
}

int shared_buffer_run_selftest(void)
{
    void *first_address = NULL;
    void *second_address = NULL;
    int failures = 0;

    if (g_shared_buffer_owner != SHARED_BUFFER_OWNER_NONE)
    {
        printf("ERROR test=shared_buffer reason=busy owner=%s\r\n",
               shared_buffer_owner_name(g_shared_buffer_owner));
        return 1;
    }

    if (shared_buffer_acquire(SHARED_BUFFER_OWNER_NONE,
                              1U,
                              &first_address) != SHARED_BUFFER_INVALID_ARG)
    {
        failures++;
    }

    if (shared_buffer_acquire(SHARED_BUFFER_OWNER_IMAGE_VIEWER,
                              SDRAM_SHARED_WORK_SIZE,
                              &first_address) != SHARED_BUFFER_OK)
    {
        failures++;
    }

    if (shared_buffer_acquire(SHARED_BUFFER_OWNER_IMAGE_VIEWER,
                              1U,
                              &second_address) != SHARED_BUFFER_OK)
    {
        failures++;
    }

    if ((first_address != second_address) ||
        ((uintptr_t)first_address != (uintptr_t)SDRAM_SHARED_WORK_ADDR))
    {
        failures++;
    }

    if (shared_buffer_acquire(SHARED_BUFFER_OWNER_NOTES,
                              1U,
                              &second_address) != SHARED_BUFFER_BUSY)
    {
        failures++;
    }

    if (shared_buffer_release(SHARED_BUFFER_OWNER_NOTES) !=
        SHARED_BUFFER_NOT_OWNER)
    {
        failures++;
    }

    if (shared_buffer_release(SHARED_BUFFER_OWNER_IMAGE_VIEWER) !=
        SHARED_BUFFER_OK)
    {
        failures++;
    }

    if (g_shared_buffer_owner != SHARED_BUFFER_OWNER_NONE)
    {
        failures++;
        g_shared_buffer_owner = SHARED_BUFFER_OWNER_NONE;
    }

    if (failures == 0)
    {
        printf("OK test=shared_buffer failures=0\r\n");
        return 0;
    }

    printf("ERROR test=shared_buffer failures=%d\r\n", failures);
    return failures;
}
