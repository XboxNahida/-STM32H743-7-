#ifndef SHARED_BUFFER_H
#define SHARED_BUFFER_H

#include <stdint.h>

typedef enum
{
    SHARED_BUFFER_OWNER_NONE = 0,
    SHARED_BUFFER_OWNER_FILE_BROWSER,
    SHARED_BUFFER_OWNER_NOTES,
    SHARED_BUFFER_OWNER_IMAGE_VIEWER,
    SHARED_BUFFER_OWNER_AVI_PLAYER,
    SHARED_BUFFER_OWNER_STORAGE_TEST,
    SHARED_BUFFER_OWNER_ALGORITHM,
    SHARED_BUFFER_OWNER_NES,
    SHARED_BUFFER_OWNER_WAV_PLAYER,
    SHARED_BUFFER_OWNER_OSCILLOSCOPE,
    SHARED_BUFFER_OWNER_COUNT
} shared_buffer_owner_t;

typedef enum
{
    SHARED_BUFFER_OK = 0,
    SHARED_BUFFER_INVALID_ARG,
    SHARED_BUFFER_BUSY,
    SHARED_BUFFER_NOT_OWNER
} shared_buffer_result_t;

shared_buffer_result_t shared_buffer_acquire(shared_buffer_owner_t owner,
                                             uint32_t requested_size,
                                             void **address);
shared_buffer_result_t shared_buffer_release(shared_buffer_owner_t owner);
shared_buffer_owner_t shared_buffer_get_owner(void);
uint32_t shared_buffer_get_size(void);
void shared_buffer_print_status(void);
int shared_buffer_run_selftest(void);

#endif
