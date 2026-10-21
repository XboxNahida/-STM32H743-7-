#include <stdint.h>
#include <stdio.h>

#include "diagnostics.h"

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n",
               name,
               (unsigned long)actual,
               (unsigned long)expected);
        return 1;
    }

    return 0;
}

int main(void)
{
    const fault_snapshot_t *fault;
    int failures = 0;

    diagnostics_init();
    failures += expect_u32("default_level",
                           (uint32_t)diagnostics_get_log_level(),
                           (uint32_t)LOG_INFO);

    diagnostics_set_log_level(LOG_TRACE);
    failures += expect_u32("set_trace",
                           (uint32_t)diagnostics_get_log_level(),
                           (uint32_t)LOG_TRACE);

    diagnostics_set_log_level((log_level_t)99);
    failures += expect_u32("invalid_level_ignored",
                           (uint32_t)diagnostics_get_log_level(),
                           (uint32_t)LOG_TRACE);

    diagnostics_set_log_level((log_level_t)-1);
    failures += expect_u32("negative_level_ignored",
                           (uint32_t)diagnostics_get_log_level(),
                           (uint32_t)LOG_TRACE);

    diagnostics_capture_first(7U, -12, 0x12345678UL, 0x87654321UL);
    fault = diagnostics_get_first_fault();
    failures += expect_u32("first_valid", fault->valid, 1U);
    failures += expect_u32("first_module", fault->module, 7U);
    failures += expect_u32("first_code", (uint32_t)fault->code, (uint32_t)-12);
    failures += expect_u32("first_detail0", fault->detail0, 0x12345678UL);
    failures += expect_u32("first_detail1", fault->detail1, 0x87654321UL);

    diagnostics_capture_first(8U, -20, 1U, 2U);
    fault = diagnostics_get_first_fault();
    failures += expect_u32("second_does_not_override_module", fault->module, 7U);
    failures += expect_u32("second_does_not_override_detail", fault->detail0,
                           0x12345678UL);

    diagnostics_clear_first_fault();
    fault = diagnostics_get_first_fault();
    failures += expect_u32("clear_valid", fault->valid, 0U);

    diagnostics_capture_first(9U, -30, 3U, 4U);
    fault = diagnostics_get_first_fault();
    failures += expect_u32("capture_after_clear", fault->module, 9U);

    if (failures != 0)
    {
        printf("FAIL diagnostics failures=%d\n", failures);
        return 1;
    }

    printf("PASS diagnostics\n");
    return 0;
}
