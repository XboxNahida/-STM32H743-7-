#include "nand_scan_service.h"

#include <stdio.h>
#include <string.h>

#include "nand_readonly.h"

#define NAND_SCAN_READ_TIMEOUT_MS       100U
#define NAND_SCAN_TIMEOUT_BLOCK_LIMIT     3U
#define NAND_SCAN_NO_ERROR_BLOCK 0xFFFFFFFFUL

static nand_geometry_t g_geometry;
static nand_scan_status_t g_status;
static uint32_t g_consecutive_timeout_blocks;

static const char *nand_scan_state_name(nand_scan_state_t state)
{
    switch (state)
    {
        case NAND_SCAN_IDLE: return "idle";
        case NAND_SCAN_RUNNING: return "running";
        case NAND_SCAN_COMPLETE: return "complete";
        case NAND_SCAN_STOPPED: return "stopped";
        case NAND_SCAN_ERROR: return "error";
        default: return "unknown";
    }
}

static void nand_scan_reset_status(void)
{
    memset(&g_status, 0, sizeof(g_status));
    g_status.state = NAND_SCAN_IDLE;
    g_status.first_error_block = NAND_SCAN_NO_ERROR_BLOCK;
    g_status.last_result = (int32_t)NAND_READONLY_OK;
    g_consecutive_timeout_blocks = 0U;
}

static void nand_scan_record_error(uint32_t block, int result)
{
    if (g_status.first_error_block == NAND_SCAN_NO_ERROR_BLOCK)
    {
        g_status.first_error_block = block;
    }
    if (result == NAND_READONLY_TIMEOUT)
    {
        g_status.timeout_errors++;
    }
    else
    {
        g_status.read_errors++;
    }
    g_status.last_result = (int32_t)result;
}

void nand_scan_init(void)
{
    memset(&g_geometry, 0, sizeof(g_geometry));
    nand_scan_reset_status();
}

int nand_scan_start(void)
{
    int result;

    if (g_status.state == NAND_SCAN_RUNNING)
    {
        return NAND_READONLY_BUSY;
    }

    memset(&g_geometry, 0, sizeof(g_geometry));
    nand_scan_reset_status();
    result = nand_readonly_get_geometry(&g_geometry);
    if (result != NAND_READONLY_OK)
    {
        g_status.state = NAND_SCAN_ERROR;
        g_status.last_result = (int32_t)result;
        return result;
    }
    if ((g_geometry.page_main_size == 0U) ||
        (g_geometry.pages_per_block < 2U) ||
        (g_geometry.block_count == 0U))
    {
        g_status.state = NAND_SCAN_ERROR;
        g_status.last_result = (int32_t)NAND_READONLY_INVALID_ARG;
        return NAND_READONLY_INVALID_ARG;
    }

    g_status.total_blocks = g_geometry.block_count;
    g_status.state = NAND_SCAN_RUNNING;
    return NAND_READONLY_OK;
}

int nand_scan_stop(void)
{
    if (g_status.state == NAND_SCAN_RUNNING)
    {
        g_status.state = NAND_SCAN_STOPPED;
        g_status.last_result = (int32_t)NAND_READONLY_STOPPED;
        return NAND_READONLY_OK;
    }
    if (g_status.state == NAND_SCAN_STOPPED)
    {
        return NAND_READONLY_OK;
    }
    return NAND_READONLY_STOPPED;
}

void nand_scan_poll(void)
{
    uint32_t block;
    uint32_t first_page;
    uint8_t marker;
    uint8_t bad_block = 0U;
    uint8_t block_timeout = 0U;
    int result;

    if (g_status.state != NAND_SCAN_RUNNING)
    {
        return;
    }
    if (g_status.scanned_blocks >= g_status.total_blocks)
    {
        g_status.state = NAND_SCAN_COMPLETE;
        return;
    }

    block = g_status.scanned_blocks;
    if (block > (0xFFFFFFFFUL / g_geometry.pages_per_block))
    {
        nand_scan_record_error(block, NAND_READONLY_INVALID_ARG);
        g_status.state = NAND_SCAN_ERROR;
        return;
    }
    first_page = block * g_geometry.pages_per_block;
    if (first_page == 0xFFFFFFFFUL)
    {
        nand_scan_record_error(block, NAND_READONLY_INVALID_ARG);
        g_status.state = NAND_SCAN_ERROR;
        return;
    }

    result = nand_readonly_read_page(first_page,
                                     g_geometry.page_main_size,
                                     &marker,
                                     1U,
                                     NAND_SCAN_READ_TIMEOUT_MS);
    if (result == NAND_READONLY_OK)
    {
        if (marker != 0xFFU)
        {
            bad_block = 1U;
        }
    }
    else
    {
        nand_scan_record_error(block, result);
        if (result == NAND_READONLY_TIMEOUT)
        {
            block_timeout = 1U;
        }
    }

    result = nand_readonly_read_page(first_page + 1U,
                                     g_geometry.page_main_size,
                                     &marker,
                                     1U,
                                     NAND_SCAN_READ_TIMEOUT_MS);
    if (result == NAND_READONLY_OK)
    {
        if (marker != 0xFFU)
        {
            bad_block = 1U;
        }
    }
    else
    {
        nand_scan_record_error(block, result);
        if (result == NAND_READONLY_TIMEOUT)
        {
            block_timeout = 1U;
        }
    }

    if (bad_block != 0U)
    {
        g_status.bad_blocks++;
    }
    g_status.scanned_blocks++;

    if (block_timeout != 0U)
    {
        g_consecutive_timeout_blocks++;
    }
    else
    {
        g_consecutive_timeout_blocks = 0U;
    }
    if (g_consecutive_timeout_blocks >= NAND_SCAN_TIMEOUT_BLOCK_LIMIT)
    {
        g_status.state = NAND_SCAN_ERROR;
    }
    else if (g_status.scanned_blocks >= g_status.total_blocks)
    {
        g_status.state = NAND_SCAN_COMPLETE;
    }
}

void nand_scan_get_status(nand_scan_status_t *status)
{
    if (status != NULL)
    {
        *status = g_status;
    }
}

void nand_scan_print_status(void)
{
    printf("nand state=%s total=%lu scanned=%lu bad=%lu read_errors=%lu timeout_errors=%lu first_error=%lu result=%ld\r\n",
           nand_scan_state_name(g_status.state),
           (unsigned long)g_status.total_blocks,
           (unsigned long)g_status.scanned_blocks,
           (unsigned long)g_status.bad_blocks,
           (unsigned long)g_status.read_errors,
           (unsigned long)g_status.timeout_errors,
           (unsigned long)g_status.first_error_block,
           (long)g_status.last_result);
}
