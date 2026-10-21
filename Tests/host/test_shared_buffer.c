#include <stdint.h>
#include <stdio.h>

#include "shared_buffer.h"
#include "sdram_layout.h"

static int expect_result(const char *name,
                         shared_buffer_result_t actual,
                         shared_buffer_result_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%d expected=%d\n",
               name,
               (int)actual,
               (int)expected);
        return 1;
    }

    return 0;
}

int main(void)
{
    void *first_address = NULL;
    void *second_address = NULL;
    int failures = 0;

    failures += expect_result("reject_none_owner",
                              shared_buffer_acquire(SHARED_BUFFER_OWNER_NONE,
                                                    1U,
                                                    &first_address),
                              SHARED_BUFFER_INVALID_ARG);
    failures += expect_result("reject_zero_size",
                              shared_buffer_acquire(SHARED_BUFFER_OWNER_IMAGE_VIEWER,
                                                    0U,
                                                    &first_address),
                              SHARED_BUFFER_INVALID_ARG);
    failures += expect_result("reject_oversize",
                              shared_buffer_acquire(SHARED_BUFFER_OWNER_IMAGE_VIEWER,
                                                    SDRAM_SHARED_WORK_SIZE + 1U,
                                                    &first_address),
                              SHARED_BUFFER_INVALID_ARG);
    failures += expect_result("reject_null_output",
                              shared_buffer_acquire(SHARED_BUFFER_OWNER_IMAGE_VIEWER,
                                                    1U,
                                                    NULL),
                              SHARED_BUFFER_INVALID_ARG);
    failures += expect_result("first_acquire",
                              shared_buffer_acquire(SHARED_BUFFER_OWNER_IMAGE_VIEWER,
                                                    4096U,
                                                    &first_address),
                              SHARED_BUFFER_OK);

    if ((uintptr_t)first_address != (uintptr_t)SDRAM_SHARED_WORK_ADDR)
    {
        printf("FAIL first_address actual=0x%08lX expected=0x%08lX\n",
               (unsigned long)(uintptr_t)first_address,
               (unsigned long)SDRAM_SHARED_WORK_ADDR);
        failures++;
    }

    failures += expect_result("same_owner_reacquire",
                              shared_buffer_acquire(SHARED_BUFFER_OWNER_IMAGE_VIEWER,
                                                    1024U,
                                                    &second_address),
                              SHARED_BUFFER_OK);
    if (second_address != first_address)
    {
        printf("FAIL same_owner_address_changed\n");
        failures++;
    }

    failures += expect_result("different_owner_busy",
                              shared_buffer_acquire(SHARED_BUFFER_OWNER_NOTES,
                                                    1024U,
                                                    &second_address),
                              SHARED_BUFFER_BUSY);
    failures += expect_result("wrong_owner_release",
                              shared_buffer_release(SHARED_BUFFER_OWNER_NOTES),
                              SHARED_BUFFER_NOT_OWNER);
    failures += expect_result("correct_release",
                              shared_buffer_release(SHARED_BUFFER_OWNER_IMAGE_VIEWER),
                              SHARED_BUFFER_OK);

    if (shared_buffer_get_owner() != SHARED_BUFFER_OWNER_NONE)
    {
        printf("FAIL owner_not_cleared actual=%d\n",
               (int)shared_buffer_get_owner());
        failures++;
    }

    if (shared_buffer_get_size() != SDRAM_SHARED_WORK_SIZE)
    {
        printf("FAIL size actual=%lu expected=%lu\n",
               (unsigned long)shared_buffer_get_size(),
               (unsigned long)SDRAM_SHARED_WORK_SIZE);
        failures++;
    }

    failures += expect_result("acquire_after_release",
                              shared_buffer_acquire(SHARED_BUFFER_OWNER_NOTES,
                                                    SDRAM_SHARED_WORK_SIZE,
                                                    &second_address),
                              SHARED_BUFFER_OK);
    failures += expect_result("final_release",
                              shared_buffer_release(SHARED_BUFFER_OWNER_NOTES),
                              SHARED_BUFFER_OK);
    failures += expect_result("nes_owner_acquire",
                              shared_buffer_acquire(SHARED_BUFFER_OWNER_NES,
                                                    SDRAM_SHARED_WORK_SIZE,
                                                    &second_address),
                              SHARED_BUFFER_OK);
    failures += expect_result("nes_owner_release",
                              shared_buffer_release(SHARED_BUFFER_OWNER_NES),
                              SHARED_BUFFER_OK);

    if (shared_buffer_run_selftest() != 0)
    {
        printf("FAIL shared_buffer_selftest\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL shared_buffer failures=%d\n", failures);
        return 1;
    }

    printf("PASS shared_buffer\n");
    return 0;
}
