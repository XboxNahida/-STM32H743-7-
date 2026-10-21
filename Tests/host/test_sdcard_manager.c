#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
int sdcard_manager_session_acquire(uint16_t owner);
int sdcard_manager_session_release(uint16_t owner);
void sdcard_manager_get_status(sdcard_manager_status_t *status);

static int g_init_result;
static int g_mount_result;
static int g_check_result;
static uint32_t g_init_calls;
static uint32_t g_mount_calls;
static uint32_t g_check_calls;
static uint32_t g_unmount_calls;
static uint32_t g_deinit_calls;

int sdcard_manager_test_init_card(void)
{
    g_init_calls++;
    return g_init_result;
}

int sdcard_manager_test_mount(void)
{
    g_mount_calls++;
    return g_mount_result;
}

int sdcard_manager_test_check_card(void)
{
    g_check_calls++;
    return g_check_result;
}

void sdcard_manager_test_unmount(void)
{
    g_unmount_calls++;
}

void sdcard_manager_test_deinit_card(void)
{
    g_deinit_calls++;
}

static int expect_int(const char *name, int actual, int expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%d expected=%d\n", name, actual, expected);
        return 1;
    }
    return 0;
}

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n",
               name, (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

static int expect_source_text(const char *path, const char *expected)
{
    FILE *file = fopen(path, "rb");
    char source[65536];
    size_t length;
    if (file == NULL)
    {
        printf("FAIL source_open path=%s\n", path);
        return 1;
    }
    length = fread(source, 1U, sizeof(source) - 1U, file);
    fclose(file);
    source[length] = '\0';
    if (strstr(source, expected) == NULL)
    {
        printf("FAIL source_text expected=%s\n", expected);
        return 1;
    }
    return 0;
}

int main(void)
{
    sdcard_manager_status_t status;
    uint32_t checks_before;
    int failures = 0;

    g_init_result = 1;
    g_mount_result = 0;
    g_check_result = 0;
    sdcard_manager_init();
    sdcard_manager_poll(0U);
    sdcard_manager_get_status(&status);
    failures += expect_int("boot_absent", status.state,
                           SDCARD_MANAGER_BACKOFF);
    failures += expect_u32("first_init_attempt", status.init_attempts, 1U);
    failures += expect_u32("retry_at_2000", status.next_action_ms, 2000U);

    g_init_result = 0;
    sdcard_manager_poll(1999U);
    failures += expect_u32("no_early_retry", g_init_calls, 1U);
    sdcard_manager_poll(2000U);
    sdcard_manager_get_status(&status);
    failures += expect_int("insert_mounted", status.state,
                           SDCARD_MANAGER_MOUNTED);
    failures += expect_u32("mounted_flag", status.mounted, 1U);
    failures += expect_u32("mount_once", g_mount_calls, 1U);

    failures += expect_int("session_acquire",
                           sdcard_manager_session_acquire(0x0501U),
                           SDCARD_MANAGER_OK);
    failures += expect_int("duplicate_session",
                           sdcard_manager_session_acquire(0x0501U),
                           SDCARD_MANAGER_OK);
    sdcard_manager_get_status(&status);
    failures += expect_u32("one_unique_session", status.session_count, 1U);
    failures += expect_int("suspend_busy",
                           sdcard_manager_request_suspend(),
                           SDCARD_MANAGER_BUSY);
    failures += expect_u32("busy_does_not_unmount", g_unmount_calls, 0U);

    failures += expect_int("session_release",
                           sdcard_manager_session_release(0x0501U),
                           SDCARD_MANAGER_OK);
    failures += expect_int("release_missing",
                           sdcard_manager_session_release(0x0501U),
                           SDCARD_MANAGER_NOT_OWNER);
    failures += expect_int("suspend_ok",
                           sdcard_manager_request_suspend(),
                           SDCARD_MANAGER_OK);
    sdcard_manager_get_status(&status);
    failures += expect_int("suspended", status.state,
                           SDCARD_MANAGER_SUSPENDED);
    failures += expect_u32("suspend_unmount", g_unmount_calls, 1U);
    failures += expect_u32("suspend_deinit", g_deinit_calls, 1U);

    checks_before = g_check_calls;
    sdcard_manager_poll(60000U);
    failures += expect_u32("suspended_no_probe", g_check_calls,
                           checks_before);

    sdcard_manager_resume();
    sdcard_manager_poll(60000U);
    failures += expect_u32("resume_cooldown", g_init_calls, 2U);
    sdcard_manager_poll(60099U);
    failures += expect_u32("resume_no_early_init", g_init_calls, 2U);
    sdcard_manager_poll(60100U);
    sdcard_manager_get_status(&status);
    failures += expect_int("resume_mounted", status.state,
                           SDCARD_MANAGER_MOUNTED);
    failures += expect_u32("resume_init", g_init_calls, 3U);
    failures += expect_u32("resume_mount", g_mount_calls, 2U);

    g_check_result = 1;
    sdcard_manager_poll(61100U);
    sdcard_manager_poll(62100U);
    sdcard_manager_get_status(&status);
    failures += expect_int("two_failures_still_mounted", status.state,
                           SDCARD_MANAGER_MOUNTED);
    failures += expect_u32("two_failures", status.consecutive_check_failures,
                           2U);
    sdcard_manager_poll(63100U);
    sdcard_manager_get_status(&status);
    failures += expect_int("removed_backoff", status.state,
                           SDCARD_MANAGER_BACKOFF);
    failures += expect_u32("removed_unmounted", status.mounted, 0U);
    failures += expect_u32("removal_count", status.removal_count, 1U);
    failures += expect_u32("remove_unmount", g_unmount_calls, 2U);
    failures += expect_u32("remove_deinit", g_deinit_calls, 2U);

    g_check_result = 0;
    g_mount_result = 1;
    sdcard_manager_poll(status.next_action_ms);
    sdcard_manager_get_status(&status);
    failures += expect_int("mount_failure_backoff", status.state,
                           SDCARD_MANAGER_BACKOFF);
    failures += expect_u32("mount_failure_count", status.mount_failures, 1U);
    failures += expect_u32("mount_failure_deinit", g_deinit_calls, 3U);
    failures += expect_source_text(
        "Core/Peripheral/Src/sdcard_manager.c",
        "__HAL_RCC_SDMMC1_FORCE_RESET()");
    failures += expect_source_text(
        "Core/Peripheral/Src/sdcard_manager.c",
        "SDCARD_MANAGER_RESUME_DELAY_MS");
    failures += expect_source_text(
        "Drivers/STM32H7xx_HAL_Driver/Inc/stm32h7xx_ll_sdmmc.h",
        "#define SDMMC_SWDATATIMEOUT");
    failures += expect_source_text(
        "Drivers/STM32H7xx_HAL_Driver/Inc/stm32h7xx_ll_sdmmc.h",
        "((uint32_t)1000U)");

    if (failures == 0)
    {
        printf("PASS sdcard_manager\n");
        return 0;
    }
    printf("FAIL sdcard_manager failures=%d\n", failures);
    return 1;
}
