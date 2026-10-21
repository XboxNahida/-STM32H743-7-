#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "diagnostics.h"
#include "nand_readonly.h"

static uint8_t g_id_bytes[5];
static uint8_t g_status_value;
static uint8_t g_last_command;
static uint8_t g_commands[8];
static uint8_t g_addresses[8];
static uint32_t g_command_count;
static uint32_t g_address_count;
static uint32_t g_data_index;
static uint32_t g_tick;
static uint32_t g_ready_after;
static uint32_t g_ready_calls;
static uint32_t g_page_busy_after;
static uint32_t g_page_ready_after;
static uint8_t g_page_data[64];
static uint32_t g_page_data_index;

void nand_readonly_platform_command(uint8_t value)
{
    if (g_command_count < sizeof(g_commands))
    {
        g_commands[g_command_count] = value;
    }
    g_command_count++;
    g_last_command = value;
    if (value == 0x90U)
    {
        g_data_index = 0U;
    }
    if (value == 0x30U)
    {
        g_ready_calls = 0U;
    }
}

void nand_readonly_platform_address(uint8_t value)
{
    if (g_address_count < sizeof(g_addresses))
    {
        g_addresses[g_address_count] = value;
    }
    g_address_count++;
}

uint8_t nand_readonly_platform_data_read(void)
{
    if ((g_last_command == 0x90U) && (g_data_index < 5U))
    {
        return g_id_bytes[g_data_index++];
    }
    if ((g_last_command == 0x30U) &&
        (g_page_data_index < sizeof(g_page_data)))
    {
        return g_page_data[g_page_data_index++];
    }
    return g_status_value;
}

uint8_t nand_readonly_platform_ready(void)
{
    uint8_t ready;

    if (g_last_command == 0x30U)
    {
        if (g_ready_calls < g_page_busy_after)
            ready = 1U;
        else if (g_ready_calls < g_page_ready_after)
            ready = 0U;
        else
            ready = 1U;
    }
    else
    {
        ready = (g_ready_calls >= g_ready_after) ? 1U : 0U;
    }
    g_ready_calls++;
    return ready;
}

uint32_t nand_readonly_platform_tick(void)
{
    return g_tick++;
}

void nand_readonly_platform_barrier(void)
{
}

void nand_readonly_platform_reset_settle(void)
{
}

void nand_readonly_platform_read_settle(void)
{
}

static void reset_platform(const uint8_t id[5])
{
    memset(g_commands, 0, sizeof(g_commands));
    memset(g_addresses, 0, sizeof(g_addresses));
    memcpy(g_id_bytes, id, sizeof(g_id_bytes));
    g_status_value = 0x40U;
    g_last_command = 0U;
    g_command_count = 0U;
    g_address_count = 0U;
    g_data_index = 0U;
    g_tick = 0U;
    g_ready_after = 0U;
    g_ready_calls = 0U;
    g_page_busy_after = 2U;
    g_page_ready_after = 4U;
    g_page_data_index = 0U;
    for (g_data_index = 0U; g_data_index < sizeof(g_page_data); g_data_index++)
    {
        g_page_data[g_data_index] = (uint8_t)(0xA0U + g_data_index);
    }
    g_data_index = 0U;
    nand_readonly_init();
}

static int geometry_matches(uint32_t main_size,
                            uint32_t spare_size,
                            uint32_t pages_per_block,
                            uint32_t block_count)
{
    nand_geometry_t geometry;

    if (nand_readonly_get_geometry(&geometry) != NAND_READONLY_OK)
    {
        return 0;
    }
    return ((geometry.page_main_size == main_size) &&
            (geometry.page_spare_size == spare_size) &&
            (geometry.pages_per_block == pages_per_block) &&
            (geometry.block_count == block_count) &&
            (geometry.row_address_cycles == 3U)) ? 1 : 0;
}

int main(void)
{
    static const uint8_t mt29f4g[5] = {0x2CU, 0xDCU, 0x90U, 0x95U, 0x56U};
    static const uint8_t is34ml04g[5] = {0xC8U, 0xDCU, 0x90U, 0x95U, 0x56U};
    static const uint8_t mt29f16g[5] = {0x2CU, 0x48U, 0x00U, 0x26U, 0x89U};
    static const uint8_t fsns8b004g[5] = {0xADU, 0xDCU, 0x00U, 0xA2U, 0x62U};
    static const uint8_t unknown[5] = {0x98U, 0xAAU, 0x11U, 0x22U, 0x33U};
    static const uint8_t wrong_maker_same_device[5] =
        {0x98U, 0xDCU, 0x90U, 0x95U, 0x56U};
    static const uint8_t all_zero[5] = {0U, 0U, 0U, 0U, 0U};
    static const uint8_t all_ff[5] = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    nand_geometry_t geometry;
    nand_id_t id;
    const fault_snapshot_t *fault;
    uint8_t status;
    int failures = 0;

    diagnostics_init();
    reset_platform(mt29f4g);
    if ((nand_readonly_get_id(&id) != NAND_READONLY_NOT_INITIALIZED) ||
        (nand_readonly_get_geometry(&geometry) != NAND_READONLY_NOT_INITIALIZED))
    {
        printf("FAIL getters_before_probe\n");
        failures++;
    }

    if ((nand_readonly_probe(100U) != NAND_READONLY_OK) ||
        (g_command_count != 2U) ||
        (g_commands[0] != 0xFFU) || (g_commands[1] != 0x90U) ||
        (g_address_count != 1U) || (g_addresses[0] != 0x00U) ||
        (nand_readonly_get_id(&id) != NAND_READONLY_OK) ||
        (id.compact_id != 0xDC909556UL) ||
        (memcmp(id.bytes, mt29f4g, sizeof(mt29f4g)) != 0) ||
        (geometry_matches(2048U, 64U, 64U, 4096U) == 0))
    {
        printf("FAIL known_id_sequence\n");
        failures++;
    }

    if ((nand_readonly_read_status(&status) != NAND_READONLY_OK) ||
        (status != 0x40U) || (g_commands[g_command_count - 1U] != 0x70U))
    {
        printf("FAIL status_read\n");
        failures++;
    }

    reset_platform(mt29f16g);
    if ((nand_readonly_probe(100U) != NAND_READONLY_OK) ||
        (geometry_matches(4096U, 224U, 128U, 4096U) == 0))
    {
        printf("FAIL mt29f16_geometry\n");
        failures++;
    }

    reset_platform(is34ml04g);
    if ((nand_readonly_probe(100U) != NAND_READONLY_OK) ||
        (geometry_matches(2048U, 64U, 64U, 4096U) == 0))
    {
        printf("FAIL is34ml04g_full_id\n");
        failures++;
    }

    reset_platform(wrong_maker_same_device);
    if ((nand_readonly_probe(100U) != NAND_READONLY_UNKNOWN_ID) ||
        (nand_readonly_get_geometry(&geometry) != NAND_READONLY_UNKNOWN_ID))
    {
        printf("FAIL full_id_maker_required\n");
        failures++;
    }

    reset_platform(fsns8b004g);
    if ((nand_readonly_probe(100U) != NAND_READONLY_OK) ||
        (geometry_matches(4096U, 64U, 64U, 2048U) == 0))
    {
        printf("FAIL fsns_geometry\n");
        failures++;
    }

    reset_platform(unknown);
    if ((nand_readonly_probe(100U) != NAND_READONLY_UNKNOWN_ID) ||
        (nand_readonly_get_id(&id) != NAND_READONLY_OK) ||
        (id.compact_id != 0xAA112233UL) ||
        (nand_readonly_get_geometry(&geometry) != NAND_READONLY_UNKNOWN_ID))
    {
        printf("FAIL unknown_id\n");
        failures++;
    }

    reset_platform(all_zero);
    if (nand_readonly_probe(100U) != NAND_READONLY_NOT_PRESENT)
    {
        printf("FAIL all_zero_absent\n");
        failures++;
    }
    reset_platform(all_ff);
    if (nand_readonly_probe(100U) != NAND_READONLY_NOT_PRESENT)
    {
        printf("FAIL all_ff_absent\n");
        failures++;
    }

    reset_platform(mt29f4g);
    g_tick = 0xFFFFFFF0UL;
    g_ready_after = 0xFFFFFFFFUL;
    if ((nand_readonly_probe(100U) != NAND_READONLY_TIMEOUT) ||
        (g_command_count != 1U) || (g_commands[0] != 0xFFU) ||
        (g_ready_calls < 100U) ||
        (nand_readonly_get_id(&id) != NAND_READONLY_TIMEOUT) ||
        (nand_readonly_read_status(&status) != NAND_READONLY_TIMEOUT) ||
        (g_command_count != 1U))
    {
        printf("FAIL timeout_wrap\n");
        failures++;
    }
    fault = diagnostics_get_first_fault();
    if ((fault->valid == 0U) || (fault->module != 0x4E44U) ||
        (fault->code != NAND_READONLY_TIMEOUT) ||
        (fault->detail0 != 0xFFU) || (fault->detail1 != 100U))
    {
        printf("FAIL probe_timeout_diagnostic\n");
        failures++;
    }
    diagnostics_clear_first_fault();

    reset_platform(mt29f4g);
    if ((nand_readonly_probe(0U) != NAND_READONLY_INVALID_ARG) ||
        (nand_readonly_get_id(NULL) != NAND_READONLY_INVALID_ARG) ||
        (nand_readonly_read_status(NULL) != NAND_READONLY_INVALID_ARG))
    {
        printf("FAIL invalid_args\n");
        failures++;
    }

    reset_platform(mt29f4g);
    (void)nand_readonly_probe(100U);
    g_command_count = 0U;
    g_address_count = 0U;
    g_page_data_index = 0U;
    {
        uint8_t buffer[4] = {0U, 0U, 0U, 0U};
        if ((nand_readonly_read_page(0x010203UL, 0x0123U,
                                     buffer, sizeof(buffer), 100U) !=
             NAND_READONLY_OK) ||
            (g_command_count != 2U) ||
            (g_commands[0] != 0x00U) || (g_commands[1] != 0x30U) ||
            (g_address_count != 5U) ||
            (g_addresses[0] != 0x23U) || (g_addresses[1] != 0x01U) ||
            (g_addresses[2] != 0x03U) || (g_addresses[3] != 0x02U) ||
            (g_addresses[4] != 0x01U) ||
            (buffer[0] != 0xA0U) || (buffer[3] != 0xA3U) ||
            (g_ready_calls < 5U))
        {
            printf("FAIL page_read_sequence\n");
            failures++;
        }
    }

    g_command_count = 0U;
    if ((nand_readonly_read_page(0U, 0U, NULL, 1U, 100U) !=
         NAND_READONLY_INVALID_ARG) ||
        (nand_readonly_read_page(0U, 0U, g_page_data, 0U, 100U) !=
         NAND_READONLY_INVALID_ARG) ||
        (nand_readonly_read_page(0U, 0U, g_page_data, 1U, 0U) !=
         NAND_READONLY_INVALID_ARG) ||
        (nand_readonly_read_page(262144U, 0U, g_page_data, 1U, 100U) !=
         NAND_READONLY_INVALID_ARG) ||
        (nand_readonly_read_page(0U, 2112U, g_page_data, 1U, 100U) !=
         NAND_READONLY_INVALID_ARG) ||
        (nand_readonly_read_page(0U, 2110U, g_page_data, 3U, 100U) !=
         NAND_READONLY_INVALID_ARG) ||
        (g_command_count != 0U))
    {
        printf("FAIL page_read_bounds\n");
        failures++;
    }

    reset_platform(unknown);
    (void)nand_readonly_probe(100U);
    g_command_count = 0U;
    if ((nand_readonly_read_page(0U, 0U, g_page_data, 1U, 100U) !=
         NAND_READONLY_UNKNOWN_ID) || (g_command_count != 0U))
    {
        printf("FAIL unknown_id_read_blocked\n");
        failures++;
    }

    reset_platform(mt29f4g);
    (void)nand_readonly_probe(100U);
    g_command_count = 0U;
    g_ready_calls = 0U;
    g_page_busy_after = 0U;
    g_page_ready_after = 0xFFFFFFFFUL;
    if ((nand_readonly_read_page(7U, 2048U, g_page_data, 1U, 10U) !=
         NAND_READONLY_TIMEOUT) ||
        (g_command_count != 2U) || (g_page_data_index != 0U))
    {
        printf("FAIL page_read_timeout\n");
        failures++;
    }
    fault = diagnostics_get_first_fault();
    if ((fault->valid == 0U) || (fault->module != 0x4E44U) ||
        (fault->code != NAND_READONLY_TIMEOUT) ||
        (fault->detail0 != 7U) || (fault->detail1 != 2048U))
    {
        printf("FAIL page_timeout_diagnostic\n");
        failures++;
    }

    g_ready_calls = 0U;
    if (nand_readonly_read_page(8U, 2048U, g_page_data, 1U, 10U) !=
        NAND_READONLY_TIMEOUT)
    {
        printf("FAIL second_page_timeout\n");
        failures++;
    }
    fault = diagnostics_get_first_fault();
    if ((fault->detail0 != 7U) || (fault->detail1 != 2048U))
    {
        printf("FAIL page_timeout_diagnostic_overwrite\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL nand_readonly failures=%d\n", failures);
        return 1;
    }
    printf("PASS nand_readonly\n");
    return 0;
}
