#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nand_readonly.h"
#include "nand_scan_service.h"

#define TEST_BLOCK_COUNT 4U
#define TEST_PAGES_PER_BLOCK 64U

static nand_geometry_t g_geometry;
static int g_geometry_result;
static uint8_t g_markers[TEST_BLOCK_COUNT][2];
static int g_read_results[TEST_BLOCK_COUNT][2];
static uint32_t g_read_pages[32];
static uint32_t g_read_columns[32];
static uint32_t g_read_lengths[32];
static uint32_t g_read_timeouts[32];
static uint32_t g_read_count;

int nand_readonly_get_geometry(nand_geometry_t *geometry)
{
    if (geometry == NULL)
    {
        return NAND_READONLY_INVALID_ARG;
    }
    if (g_geometry_result != NAND_READONLY_OK)
    {
        return g_geometry_result;
    }
    *geometry = g_geometry;
    return NAND_READONLY_OK;
}

int nand_readonly_read_page(uint32_t page,
                            uint32_t column,
                            uint8_t *buffer,
                            uint32_t length,
                            uint32_t timeout_ms)
{
    uint32_t block = page / TEST_PAGES_PER_BLOCK;
    uint32_t page_in_block = page % TEST_PAGES_PER_BLOCK;
    uint32_t marker_index = (page_in_block == 0U) ? 0U : 1U;
    int result;

    if (g_read_count < (sizeof(g_read_pages) / sizeof(g_read_pages[0])))
    {
        g_read_pages[g_read_count] = page;
        g_read_columns[g_read_count] = column;
        g_read_lengths[g_read_count] = length;
        g_read_timeouts[g_read_count] = timeout_ms;
    }
    g_read_count++;

    if ((block >= TEST_BLOCK_COUNT) || (page_in_block > 1U))
    {
        return NAND_READONLY_INVALID_ARG;
    }
    result = g_read_results[block][marker_index];
    if (result == NAND_READONLY_OK)
    {
        *buffer = g_markers[block][marker_index];
    }
    return result;
}

static void reset_fixture(void)
{
    uint32_t block;

    memset(&g_geometry, 0, sizeof(g_geometry));
    g_geometry.page_main_size = 2048U;
    g_geometry.page_spare_size = 64U;
    g_geometry.pages_per_block = TEST_PAGES_PER_BLOCK;
    g_geometry.block_count = TEST_BLOCK_COUNT;
    g_geometry.row_address_cycles = 3U;
    g_geometry_result = NAND_READONLY_OK;
    memset(g_markers, 0xFF, sizeof(g_markers));
    for (block = 0U; block < TEST_BLOCK_COUNT; block++)
    {
        g_read_results[block][0] = NAND_READONLY_OK;
        g_read_results[block][1] = NAND_READONLY_OK;
    }
    memset(g_read_pages, 0, sizeof(g_read_pages));
    memset(g_read_columns, 0, sizeof(g_read_columns));
    memset(g_read_lengths, 0, sizeof(g_read_lengths));
    memset(g_read_timeouts, 0, sizeof(g_read_timeouts));
    g_read_count = 0U;
    nand_scan_init();
}

static int expect_status(nand_scan_state_t state,
                         uint32_t scanned_blocks,
                         uint32_t bad_blocks,
                         uint32_t read_errors,
                         uint32_t timeout_errors,
                         uint32_t first_error_block,
                         int32_t last_result)
{
    nand_scan_status_t status;

    memset(&status, 0, sizeof(status));
    nand_scan_get_status(&status);
    return ((status.state == state) &&
            (status.total_blocks == TEST_BLOCK_COUNT) &&
            (status.scanned_blocks == scanned_blocks) &&
            (status.bad_blocks == bad_blocks) &&
            (status.read_errors == read_errors) &&
            (status.timeout_errors == timeout_errors) &&
            (status.first_error_block == first_error_block) &&
            (status.last_result == last_result)) ? 1 : 0;
}

int main(void)
{
    nand_scan_status_t status;
    uint32_t reads_before;
    int failures = 0;

    reset_fixture();
    nand_scan_get_status(&status);
    if ((status.state != NAND_SCAN_IDLE) ||
        (status.total_blocks != 0U) ||
        (status.first_error_block != 0xFFFFFFFFUL))
    {
        printf("FAIL init_state\n");
        failures++;
    }

    g_markers[1][0] = 0x00U;
    g_markers[2][1] = 0x7FU;
    g_markers[3][0] = 0x01U;
    g_markers[3][1] = 0x02U;
    if ((nand_scan_start() != NAND_READONLY_OK) ||
        (nand_scan_start() != NAND_READONLY_BUSY))
    {
        printf("FAIL start_busy\n");
        failures++;
    }
    nand_scan_poll();
    if (!expect_status(NAND_SCAN_RUNNING, 1U, 0U, 0U, 0U,
                       0xFFFFFFFFUL, NAND_READONLY_OK) ||
        (g_read_count != 2U) ||
        (g_read_pages[0] != 0U) || (g_read_pages[1] != 1U) ||
        (g_read_columns[0] != 2048U) ||
        (g_read_lengths[0] != 1U) || (g_read_timeouts[0] != 100U))
    {
        printf("FAIL one_block_per_poll\n");
        failures++;
    }
    nand_scan_poll();
    nand_scan_poll();
    nand_scan_poll();
    if (!expect_status(NAND_SCAN_COMPLETE, 4U, 3U, 0U, 0U,
                       0xFFFFFFFFUL, NAND_READONLY_OK) ||
        (g_read_count != 8U))
    {
        printf("FAIL bad_markers_complete\n");
        failures++;
    }
    nand_scan_poll();
    if (g_read_count != 8U)
    {
        printf("FAIL complete_no_side_effect\n");
        failures++;
    }

    reset_fixture();
    (void)nand_scan_start();
    nand_scan_poll();
    reads_before = g_read_count;
    if ((nand_scan_stop() != NAND_READONLY_OK) ||
        !expect_status(NAND_SCAN_STOPPED, 1U, 0U, 0U, 0U,
                       0xFFFFFFFFUL, NAND_READONLY_STOPPED))
    {
        printf("FAIL stop_state\n");
        failures++;
    }
    nand_scan_poll();
    if (g_read_count != reads_before)
    {
        printf("FAIL stopped_no_side_effect\n");
        failures++;
    }

    reset_fixture();
    g_geometry_result = NAND_READONLY_UNKNOWN_ID;
    if (nand_scan_start() != NAND_READONLY_UNKNOWN_ID)
    {
        printf("FAIL unknown_id_result\n");
        failures++;
    }
    nand_scan_get_status(&status);
    if ((status.state != NAND_SCAN_ERROR) ||
        (status.last_result != NAND_READONLY_UNKNOWN_ID) ||
        (g_read_count != 0U))
    {
        printf("FAIL unknown_id_blocked\n");
        failures++;
    }

    reset_fixture();
    g_read_results[0][0] = NAND_READONLY_IO_ERROR;
    g_read_results[1][1] = NAND_READONLY_IO_ERROR;
    (void)nand_scan_start();
    nand_scan_poll();
    nand_scan_poll();
    nand_scan_poll();
    if (!expect_status(NAND_SCAN_RUNNING, 3U, 0U, 2U, 0U, 0U,
                       NAND_READONLY_IO_ERROR))
    {
        printf("FAIL read_errors_first_only\n");
        failures++;
    }

    reset_fixture();
    g_read_results[0][0] = NAND_READONLY_TIMEOUT;
    g_read_results[0][1] = NAND_READONLY_TIMEOUT;
    g_read_results[1][0] = NAND_READONLY_TIMEOUT;
    g_read_results[1][1] = NAND_READONLY_TIMEOUT;
    g_read_results[2][0] = NAND_READONLY_TIMEOUT;
    g_read_results[2][1] = NAND_READONLY_TIMEOUT;
    (void)nand_scan_start();
    nand_scan_poll();
    nand_scan_poll();
    nand_scan_poll();
    if (!expect_status(NAND_SCAN_ERROR, 3U, 0U, 0U, 6U, 0U,
                       NAND_READONLY_TIMEOUT) ||
        (g_read_count != 6U))
    {
        printf("FAIL consecutive_timeout_error\n");
        failures++;
    }
    nand_scan_poll();
    if (g_read_count != 6U)
    {
        printf("FAIL error_no_side_effect\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL nand_scan_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS nand_scan_service\n");
    return 0;
}
