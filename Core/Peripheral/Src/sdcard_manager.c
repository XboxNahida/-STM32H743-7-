#include "sdcard_manager.h"

#include <stddef.h>
#include <string.h>

#define SDCARD_MANAGER_MAX_SESSIONS       8U
#define SDCARD_MANAGER_CHECK_INTERVAL_MS  1000U
#define SDCARD_MANAGER_RETRY_MIN_MS        2000U
#define SDCARD_MANAGER_RETRY_MAX_MS       10000U
#define SDCARD_MANAGER_REMOVE_THRESHOLD   3U
#define SDCARD_MANAGER_RESUME_DELAY_MS    100U

#ifndef SDCARD_MANAGER_HOST_TEST
#include "ff.h"
#include "sdcard.h"
#include "sdmmc.h"

static FATFS g_sdcard_manager_fs;

static int sdcard_manager_platform_init_card(void)
{
    if (sdcard_is_ready() != 0U)
    {
        return 0;
    }
    __HAL_RCC_SDMMC1_FORCE_RESET();
    __DSB();
    __HAL_RCC_SDMMC1_RELEASE_RESET();
    if (HAL_SD_Init(&hsd1) != HAL_OK)
    {
        sdcard_set_ready(0U);
        return 1;
    }
    sdcard_set_ready(1U);
    return 0;
}

static int sdcard_manager_platform_mount(void)
{
    return (f_mount(&g_sdcard_manager_fs, "0:", 1U) == FR_OK) ? 0 : 1;
}

static int sdcard_manager_platform_check_card(void)
{
    return (HAL_SD_GetCardState(&hsd1) == HAL_SD_CARD_TRANSFER) ? 0 : 1;
}

static void sdcard_manager_platform_unmount(void)
{
    (void)f_mount(NULL, "0:", 0U);
}

static void sdcard_manager_platform_deinit_card(void)
{
    sdcard_set_ready(0U);
    (void)HAL_SD_DeInit(&hsd1);
}
#else
extern int sdcard_manager_test_init_card(void);
extern int sdcard_manager_test_mount(void);
extern int sdcard_manager_test_check_card(void);
extern void sdcard_manager_test_unmount(void);
extern void sdcard_manager_test_deinit_card(void);

#define sdcard_manager_platform_init_card   sdcard_manager_test_init_card
#define sdcard_manager_platform_mount       sdcard_manager_test_mount
#define sdcard_manager_platform_check_card  sdcard_manager_test_check_card
#define sdcard_manager_platform_unmount     sdcard_manager_test_unmount
#define sdcard_manager_platform_deinit_card sdcard_manager_test_deinit_card
#endif

static sdcard_manager_status_t g_sdcard_manager_status;
static uint16_t g_sdcard_manager_sessions[SDCARD_MANAGER_MAX_SESSIONS];
static uint32_t g_sdcard_manager_retry_ms;
static uint32_t g_sdcard_manager_last_poll_ms;

static int sdcard_manager_time_reached(uint32_t now_ms, uint32_t deadline_ms)
{
    return ((int32_t)(now_ms - deadline_ms) >= 0) ? 1 : 0;
}

static void sdcard_manager_schedule_retry(uint32_t now_ms, int result)
{
    g_sdcard_manager_status.state = SDCARD_MANAGER_BACKOFF;
    g_sdcard_manager_status.mounted = 0U;
    g_sdcard_manager_status.last_result = result;
    g_sdcard_manager_status.next_action_ms = now_ms + g_sdcard_manager_retry_ms;
    if (g_sdcard_manager_retry_ms < SDCARD_MANAGER_RETRY_MAX_MS)
    {
        g_sdcard_manager_retry_ms *= 2U;
        if (g_sdcard_manager_retry_ms > SDCARD_MANAGER_RETRY_MAX_MS)
        {
            g_sdcard_manager_retry_ms = SDCARD_MANAGER_RETRY_MAX_MS;
        }
    }
}

static void sdcard_manager_try_mount(uint32_t now_ms)
{
    g_sdcard_manager_status.state = SDCARD_MANAGER_INITIALIZING;
    g_sdcard_manager_status.init_attempts++;
    if (sdcard_manager_platform_init_card() != 0)
    {
        sdcard_manager_schedule_retry(now_ms, SDCARD_MANAGER_IO_ERROR);
        return;
    }
    if (sdcard_manager_platform_mount() != 0)
    {
        g_sdcard_manager_status.mount_failures++;
        sdcard_manager_platform_deinit_card();
        sdcard_manager_schedule_retry(now_ms, SDCARD_MANAGER_IO_ERROR);
        return;
    }

    g_sdcard_manager_retry_ms = SDCARD_MANAGER_RETRY_MIN_MS;
    g_sdcard_manager_last_poll_ms = 0U;
    g_sdcard_manager_status.state = SDCARD_MANAGER_MOUNTED;
    g_sdcard_manager_status.mounted = 1U;
    g_sdcard_manager_status.consecutive_check_failures = 0U;
    g_sdcard_manager_status.next_action_ms =
        now_ms + SDCARD_MANAGER_CHECK_INTERVAL_MS;
    g_sdcard_manager_status.last_result = SDCARD_MANAGER_OK;
}

void sdcard_manager_init(void)
{
    memset(&g_sdcard_manager_status, 0, sizeof(g_sdcard_manager_status));
    memset(g_sdcard_manager_sessions, 0, sizeof(g_sdcard_manager_sessions));
    g_sdcard_manager_retry_ms = SDCARD_MANAGER_RETRY_MIN_MS;
    g_sdcard_manager_status.state = SDCARD_MANAGER_ABSENT;
    g_sdcard_manager_status.last_result = SDCARD_MANAGER_NOT_READY;
}

void sdcard_manager_poll(uint32_t now_ms)
{
    g_sdcard_manager_last_poll_ms = now_ms;
    if (g_sdcard_manager_status.state == SDCARD_MANAGER_SUSPENDED)
    {
        return;
    }

    if ((g_sdcard_manager_status.state == SDCARD_MANAGER_ABSENT) ||
        (g_sdcard_manager_status.state == SDCARD_MANAGER_BACKOFF))
    {
        if (sdcard_manager_time_reached(
                now_ms, g_sdcard_manager_status.next_action_ms) != 0)
        {
            sdcard_manager_try_mount(now_ms);
        }
        return;
    }

    if ((g_sdcard_manager_status.state != SDCARD_MANAGER_MOUNTED) ||
        (sdcard_manager_time_reached(
             now_ms, g_sdcard_manager_status.next_action_ms) == 0))
    {
        return;
    }

    g_sdcard_manager_status.next_action_ms =
        now_ms + SDCARD_MANAGER_CHECK_INTERVAL_MS;
    if (sdcard_manager_platform_check_card() == 0)
    {
        g_sdcard_manager_status.consecutive_check_failures = 0U;
        return;
    }

    if (g_sdcard_manager_status.consecutive_check_failures < UINT8_MAX)
    {
        g_sdcard_manager_status.consecutive_check_failures++;
    }
    if (g_sdcard_manager_status.consecutive_check_failures <
        SDCARD_MANAGER_REMOVE_THRESHOLD)
    {
        return;
    }

    sdcard_manager_platform_unmount();
    sdcard_manager_platform_deinit_card();
    g_sdcard_manager_status.removal_count++;
    g_sdcard_manager_retry_ms = SDCARD_MANAGER_RETRY_MIN_MS;
    sdcard_manager_schedule_retry(now_ms, SDCARD_MANAGER_NOT_READY);
}

int sdcard_manager_request_suspend(void)
{
    if (g_sdcard_manager_status.session_count != 0U)
    {
        return SDCARD_MANAGER_BUSY;
    }
    if (g_sdcard_manager_status.state == SDCARD_MANAGER_SUSPENDED)
    {
        return SDCARD_MANAGER_OK;
    }
    if (g_sdcard_manager_status.mounted != 0U)
    {
        sdcard_manager_platform_unmount();
        sdcard_manager_platform_deinit_card();
    }
    else if (g_sdcard_manager_status.state == SDCARD_MANAGER_BACKOFF)
    {
        sdcard_manager_platform_deinit_card();
    }
    g_sdcard_manager_status.state = SDCARD_MANAGER_SUSPENDED;
    g_sdcard_manager_status.mounted = 0U;
    g_sdcard_manager_status.consecutive_check_failures = 0U;
    g_sdcard_manager_status.last_result = SDCARD_MANAGER_OK;
    return SDCARD_MANAGER_OK;
}

void sdcard_manager_resume(void)
{
    if (g_sdcard_manager_status.state != SDCARD_MANAGER_SUSPENDED)
    {
        return;
    }
    g_sdcard_manager_retry_ms = SDCARD_MANAGER_RETRY_MIN_MS;
    g_sdcard_manager_status.state = SDCARD_MANAGER_ABSENT;
    g_sdcard_manager_status.next_action_ms =
        g_sdcard_manager_last_poll_ms + SDCARD_MANAGER_RESUME_DELAY_MS;
    g_sdcard_manager_status.last_result = SDCARD_MANAGER_NOT_READY;
}

void sdcard_manager_request_reprobe(void)
{
    if (g_sdcard_manager_status.state == SDCARD_MANAGER_SUSPENDED)
    {
        return;
    }
    if (g_sdcard_manager_status.mounted != 0U)
    {
        sdcard_manager_platform_unmount();
        sdcard_manager_platform_deinit_card();
    }
    g_sdcard_manager_retry_ms = SDCARD_MANAGER_RETRY_MIN_MS;
    g_sdcard_manager_status.state = SDCARD_MANAGER_ABSENT;
    g_sdcard_manager_status.mounted = 0U;
    g_sdcard_manager_status.next_action_ms = 0U;
}

int sdcard_manager_session_acquire(uint16_t owner)
{
    uint32_t index;
    uint32_t free_index = SDCARD_MANAGER_MAX_SESSIONS;

    if (owner == 0U)
    {
        return SDCARD_MANAGER_INVALID_ARG;
    }
    if (g_sdcard_manager_status.state != SDCARD_MANAGER_MOUNTED)
    {
        return SDCARD_MANAGER_NOT_READY;
    }
    for (index = 0U; index < SDCARD_MANAGER_MAX_SESSIONS; index++)
    {
        if (g_sdcard_manager_sessions[index] == owner)
        {
            return SDCARD_MANAGER_OK;
        }
        if ((free_index == SDCARD_MANAGER_MAX_SESSIONS) &&
            (g_sdcard_manager_sessions[index] == 0U))
        {
            free_index = index;
        }
    }
    if (free_index == SDCARD_MANAGER_MAX_SESSIONS)
    {
        return SDCARD_MANAGER_BUSY;
    }
    g_sdcard_manager_sessions[free_index] = owner;
    g_sdcard_manager_status.session_count++;
    return SDCARD_MANAGER_OK;
}

int sdcard_manager_session_release(uint16_t owner)
{
    uint32_t index;

    if (owner == 0U)
    {
        return SDCARD_MANAGER_INVALID_ARG;
    }
    for (index = 0U; index < SDCARD_MANAGER_MAX_SESSIONS; index++)
    {
        if (g_sdcard_manager_sessions[index] == owner)
        {
            g_sdcard_manager_sessions[index] = 0U;
            if (g_sdcard_manager_status.session_count > 0U)
            {
                g_sdcard_manager_status.session_count--;
            }
            return SDCARD_MANAGER_OK;
        }
    }
    return SDCARD_MANAGER_NOT_OWNER;
}

void sdcard_manager_get_status(sdcard_manager_status_t *status)
{
    if (status != NULL)
    {
        *status = g_sdcard_manager_status;
    }
}
