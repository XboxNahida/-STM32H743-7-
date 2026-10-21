#include <stdint.h>
#include <stdio.h>

#include "resource_manager.h"

static int restore_ok(void *context)
{
    uint32_t *calls = (uint32_t *)context;
    (*calls)++;
    return 0;
}

static int restore_fail(void *context)
{
    uint32_t *calls = (uint32_t *)context;
    (*calls)++;
    return -1;
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

int main(void)
{
    const resource_owner_t owner_sd = 10U;
    const resource_owner_t owner_camera = 20U;
    const resource_owner_t owner_default = 30U;
    const resource_owner_t owner_test = 40U;
    uint32_t restore_ok_calls = 0U;
    uint32_t restore_fail_calls = 0U;
    int failures = 0;

    failures += expect_int("init", resource_manager_init(), RESOURCE_OK);
    failures += expect_int("reject_invalid_id",
                           resource_acquire(RESOURCE_COUNT, owner_camera),
                           RESOURCE_INVALID_ARG);
    failures += expect_int("reject_none_owner",
                           resource_acquire(RESOURCE_TIM1_PA8,
                                            RESOURCE_OWNER_NONE),
                           RESOURCE_INVALID_ARG);
    failures += expect_int("configure_tim2_pa5",
                           resource_configure(RESOURCE_TIM2_PA5,
                                              RESOURCE_OWNER_NONE,
                                              NULL,
                                              NULL),
                           RESOURCE_OK);
    failures += expect_int("acquire_tim2_pa5",
                           resource_acquire(RESOURCE_TIM2_PA5, owner_test),
                           RESOURCE_OK);
    failures += expect_int("release_tim2_pa5",
                           resource_release(RESOURCE_TIM2_PA5, owner_test),
                           RESOURCE_OK);

    failures += expect_int("configure_default",
                           resource_configure(RESOURCE_SDMMC1_DCMI_PINS,
                                              owner_sd,
                                              restore_ok,
                                              &restore_ok_calls),
                           RESOURCE_OK);
    failures += expect_int("default_is_owner",
                           (int)resource_get_owner(RESOURCE_SDMMC1_DCMI_PINS),
                           (int)owner_sd);
    failures += expect_int("other_owner_busy",
                           resource_acquire(RESOURCE_SDMMC1_DCMI_PINS,
                                            owner_camera),
                           RESOURCE_BUSY);
    failures += expect_int("release_default",
                           resource_release(RESOURCE_SDMMC1_DCMI_PINS,
                                            owner_sd),
                           RESOURCE_OK);
    failures += expect_int("acquire_camera",
                           resource_acquire(RESOURCE_SDMMC1_DCMI_PINS,
                                            owner_camera),
                           RESOURCE_OK);
    failures += expect_int("repeat_camera",
                           resource_acquire(RESOURCE_SDMMC1_DCMI_PINS,
                                            owner_camera),
                           RESOURCE_OK);
    failures += expect_int("wrong_release",
                           resource_release(RESOURCE_SDMMC1_DCMI_PINS,
                                            owner_sd),
                           RESOURCE_NOT_OWNER);
    failures += expect_int("restore_default",
                           resource_restore_default(RESOURCE_SDMMC1_DCMI_PINS,
                                                    owner_camera),
                           RESOURCE_OK);
    failures += expect_int("restore_callback_called",
                           (int)restore_ok_calls,
                           1);
    failures += expect_int("owner_restored",
                           (int)resource_get_owner(RESOURCE_SDMMC1_DCMI_PINS),
                           (int)owner_sd);

    failures += expect_int("configure_failure_case",
                           resource_configure(RESOURCE_TIM1_PA8,
                                              owner_default,
                                              restore_fail,
                                              &restore_fail_calls),
                           RESOURCE_OK);
    failures += expect_int("release_failure_default",
                           resource_release(RESOURCE_TIM1_PA8, owner_default),
                           RESOURCE_OK);
    failures += expect_int("acquire_failure_test",
                           resource_acquire(RESOURCE_TIM1_PA8, owner_test),
                           RESOURCE_OK);
    failures += expect_int("restore_failure",
                           resource_restore_default(RESOURCE_TIM1_PA8,
                                                    owner_test),
                           RESOURCE_RESTORE_FAILED);
    failures += expect_int("failure_callback_called",
                           (int)restore_fail_calls,
                           1);
    failures += expect_int("failure_keeps_owner",
                           (int)resource_get_owner(RESOURCE_TIM1_PA8),
                           (int)owner_test);
    failures += expect_int("cleanup_failure_case",
                           resource_release(RESOURCE_TIM1_PA8, owner_test),
                           RESOURCE_OK);

    failures += expect_int("selftest",
                           resource_run_selftest(),
                           0);

    if (failures != 0)
    {
        printf("FAIL resource_manager failures=%d\n", failures);
        return 1;
    }

    printf("PASS resource_manager\n");
    return 0;
}
