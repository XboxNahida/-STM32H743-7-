#ifndef SDCARD_MANAGER_H
#define SDCARD_MANAGER_H

#include <stdint.h>

typedef enum
{
    SDCARD_MANAGER_OK = 0,
    SDCARD_MANAGER_INVALID_ARG,
    SDCARD_MANAGER_BUSY,
    SDCARD_MANAGER_NOT_READY,
    SDCARD_MANAGER_NOT_OWNER,
    SDCARD_MANAGER_IO_ERROR
} sdcard_manager_result_t;

typedef enum
{
    SDCARD_MANAGER_ABSENT = 0,
    SDCARD_MANAGER_INITIALIZING,
    SDCARD_MANAGER_MOUNTED,
    SDCARD_MANAGER_SUSPENDED,
    SDCARD_MANAGER_BACKOFF
} sdcard_manager_state_t;

typedef struct
{
    sdcard_manager_state_t state;
    uint8_t mounted;
    uint8_t session_count;
    uint8_t consecutive_check_failures;
    uint32_t init_attempts;
    uint32_t mount_failures;
    uint32_t removal_count;
    uint32_t next_action_ms;
    int last_result;
} sdcard_manager_status_t;

void sdcard_manager_init(void);
void sdcard_manager_poll(uint32_t now_ms);
int sdcard_manager_request_suspend(void);
void sdcard_manager_resume(void);
void sdcard_manager_request_reprobe(void);
int sdcard_manager_session_acquire(uint16_t owner);
int sdcard_manager_session_release(uint16_t owner);
void sdcard_manager_get_status(sdcard_manager_status_t *status);

#endif
