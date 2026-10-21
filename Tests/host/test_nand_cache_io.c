#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nand_cache_io.h"
#include "nand_readonly.h"

static nand_geometry_t g_geometry;
static int g_geometry_result;
static uint8_t g_commands[8];
static uint8_t g_addresses[8];
static uint8_t g_written[32];
static uint32_t g_command_count;
static uint32_t g_address_count;
static uint32_t g_written_count;
static uint32_t g_tick;
static uint32_t g_busy_after;
static uint32_t g_ready_after;
static uint32_t g_ready_calls;
static uint8_t g_status;
static uint8_t g_bad_markers[2];
static uint32_t g_read_pages[2];
static uint32_t g_read_count;
static int g_read_result;

int nand_cache_io_platform_get_geometry(nand_geometry_t *geometry)
{
    if (g_geometry_result != 0) return g_geometry_result;
    *geometry = g_geometry;
    return 0;
}

int nand_cache_io_platform_read_page(uint32_t page, uint32_t column,
                                     uint8_t *buffer, uint32_t length,
                                     uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (g_read_result != 0) return g_read_result;
    if ((column != g_geometry.page_main_size) || (length != 1U) ||
        (g_read_count >= 2U)) return -1;
    g_read_pages[g_read_count] = page;
    buffer[0] = g_bad_markers[g_read_count];
    g_read_count++;
    return 0;
}

void nand_cache_io_platform_command(uint8_t value)
{
    if (g_command_count < sizeof(g_commands))
        g_commands[g_command_count] = value;
    g_command_count++;
}

void nand_cache_io_platform_address(uint8_t value)
{
    if (g_address_count < sizeof(g_addresses))
        g_addresses[g_address_count] = value;
    g_address_count++;
}

void nand_cache_io_platform_data_write(uint8_t value)
{
    if (g_written_count < sizeof(g_written))
        g_written[g_written_count] = value;
    g_written_count++;
}

uint8_t nand_cache_io_platform_data_read(void) { return g_status; }

uint8_t nand_cache_io_platform_ready(void)
{
    uint8_t ready;

    if (g_ready_calls < g_busy_after)
        ready = 1U;
    else if (g_ready_calls < g_ready_after)
        ready = 0U;
    else
        ready = 1U;
    g_ready_calls++;
    return ready;
}

uint32_t nand_cache_io_platform_tick(void) { return g_tick++; }
void nand_cache_io_platform_barrier(void) {}

static void reset_fake(void)
{
    memset(g_commands, 0, sizeof(g_commands));
    memset(g_addresses, 0, sizeof(g_addresses));
    memset(g_written, 0, sizeof(g_written));
    memset(g_read_pages, 0, sizeof(g_read_pages));
    g_geometry.page_main_size = 2048U;
    g_geometry.page_spare_size = 64U;
    g_geometry.pages_per_block = 64U;
    g_geometry.block_count = 4096U;
    g_geometry.row_address_cycles = 3U;
    g_geometry_result = 0;
    g_command_count = 0U;
    g_address_count = 0U;
    g_written_count = 0U;
    g_tick = 0U;
    g_busy_after = 2U;
    g_ready_after = 4U;
    g_ready_calls = 0U;
    g_status = 0x40U;
    g_bad_markers[0] = 0xFFU;
    g_bad_markers[1] = 0xFFU;
    g_read_count = 0U;
    g_read_result = 0;
}

int main(void)
{
    static const uint8_t payload[4] = {0x12U, 0x34U, 0x56U, 0x78U};
    nand_cache_io_diagnostics_t diagnostics;
    uint8_t is_bad = 0U;
    int failures = 0;

    reset_fake();
    if ((nand_cache_io_program_page(0x010203U, 0x0123U, payload,
                                     sizeof(payload), 100U) != NAND_CACHE_IO_OK) ||
        (g_command_count != 3U) || (g_commands[0] != 0x80U) ||
        (g_commands[1] != 0x10U) || (g_commands[2] != 0x70U) ||
        (g_address_count != 5U) ||
        (g_addresses[0] != 0x23U) || (g_addresses[1] != 0x01U) ||
        (g_addresses[2] != 0x03U) || (g_addresses[3] != 0x02U) ||
        (g_addresses[4] != 0x01U) ||
        (g_written_count != sizeof(payload)) ||
        (memcmp(g_written, payload, sizeof(payload)) != 0) ||
        (g_ready_calls < 5U))
    {
        printf("FAIL program_sequence\n");
        failures++;
    }

    reset_fake();
    if ((nand_cache_io_erase_block(3U, 100U) != NAND_CACHE_IO_OK) ||
        (g_command_count != 3U) || (g_commands[0] != 0x60U) ||
        (g_commands[1] != 0xD0U) || (g_commands[2] != 0x70U) ||
        (g_address_count != 3U) || (g_addresses[0] != 0xC0U) ||
        (g_addresses[1] != 0x00U) || (g_addresses[2] != 0x00U))
    {
        printf("FAIL erase_sequence\n");
        failures++;
    }

    reset_fake();
    g_status = 0x41U;
    if (nand_cache_io_program_page(0U, 0U, payload, sizeof(payload), 100U) !=
        NAND_CACHE_IO_PROGRAM_FAILED)
    {
        printf("FAIL program_status_failure\n");
        failures++;
    }
    reset_fake();
    g_status = 0x41U;
    if (nand_cache_io_erase_block(0U, 100U) != NAND_CACHE_IO_ERASE_FAILED)
    {
        printf("FAIL erase_status_failure\n");
        failures++;
    }
    nand_cache_io_get_diagnostics(&diagnostics);
    if ((diagnostics.operation != NAND_CACHE_IO_OPERATION_ERASE) ||
        (diagnostics.target != 0U) ||
        (diagnostics.result != NAND_CACHE_IO_ERASE_FAILED) ||
        (diagnostics.status_valid != 1U) ||
        (diagnostics.status != 0x41U) ||
        (diagnostics.ready != 1U))
    {
        printf("FAIL erase_failure_diagnostics\n");
        failures++;
    }

    reset_fake();
    g_busy_after = 0U;
    g_ready_after = 0xFFFFFFFFU;
    if ((nand_cache_io_program_page(0U, 0U, payload, sizeof(payload), 5U) !=
         NAND_CACHE_IO_TIMEOUT) || (g_command_count != 2U))
    {
        printf("FAIL program_timeout\n");
        failures++;
    }
    nand_cache_io_get_diagnostics(&diagnostics);
    if ((diagnostics.operation != NAND_CACHE_IO_OPERATION_PROGRAM) ||
        (diagnostics.target != 0U) ||
        (diagnostics.result != NAND_CACHE_IO_TIMEOUT) ||
        (diagnostics.status_valid != 0U) ||
        (diagnostics.ready != 0U))
    {
        printf("FAIL program_timeout_diagnostics\n");
        failures++;
    }

    reset_fake();
    if ((nand_cache_io_block_is_bad(7U, 100U, &is_bad) != NAND_CACHE_IO_OK) ||
        (is_bad != 0U) || (g_read_count != 2U) ||
        (g_read_pages[0] != 448U) || (g_read_pages[1] != 449U))
    {
        printf("FAIL good_block_markers\n");
        failures++;
    }
    reset_fake();
    g_bad_markers[1] = 0x00U;
    if ((nand_cache_io_block_is_bad(7U, 100U, &is_bad) != NAND_CACHE_IO_OK) ||
        (is_bad != 1U))
    {
        printf("FAIL bad_second_marker\n");
        failures++;
    }

    reset_fake();
    if ((nand_cache_io_program_page(0U, 0U, NULL, 1U, 100U) !=
         NAND_CACHE_IO_INVALID_ARG) ||
        (nand_cache_io_program_page(0U, 2111U, payload, 2U, 100U) !=
         NAND_CACHE_IO_INVALID_ARG) ||
        (nand_cache_io_program_page(262144U, 0U, payload, 1U, 100U) !=
         NAND_CACHE_IO_INVALID_ARG) ||
        (nand_cache_io_erase_block(4096U, 100U) != NAND_CACHE_IO_INVALID_ARG) ||
        (nand_cache_io_block_is_bad(0U, 0U, &is_bad) !=
         NAND_CACHE_IO_INVALID_ARG) ||
        (nand_cache_io_block_is_bad(0U, 100U, NULL) !=
         NAND_CACHE_IO_INVALID_ARG) ||
        (g_command_count != 0U))
    {
        printf("FAIL bounds_and_args\n");
        failures++;
    }

    reset_fake();
    g_geometry_result = -1;
    if ((nand_cache_io_erase_block(0U, 100U) != NAND_CACHE_IO_NOT_READY) ||
        (nand_cache_io_program_page(0U, 0U, payload, 1U, 100U) !=
         NAND_CACHE_IO_NOT_READY))
    {
        printf("FAIL geometry_not_ready\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL nand_cache_io failures=%d\n", failures);
        return 1;
    }
    printf("PASS nand_cache_io\n");
    return 0;
}
