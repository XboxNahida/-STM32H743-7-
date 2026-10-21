#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nand_readonly.h"
#include "nand_scan_service.h"
#include "storage_service.h"

static uint8_t g_sd_ready;
static uint32_t g_sd_test_timeout;
static nand_id_t g_nand_id;
static nand_geometry_t g_nand_geometry;
static nand_scan_status_t g_nand_scan_status;
static int g_nand_id_result;
static int g_nand_geometry_result;
static int g_nand_status_result;
static uint8_t g_nand_status_value;
static uint32_t g_nand_status_calls;

void storage_platform_read_sd_status(storage_status_t *status)
{
    memset(status, 0, sizeof(*status));
    status->present = g_sd_ready;
    status->ready = g_sd_ready;
    status->last_result = g_sd_ready ? STORAGE_OK : STORAGE_NOT_READY;
    status->error_count = 3U;
    status->detail0 = 0x11U;
    status->detail1 = 0x22U;
}

void storage_platform_read_qspi_status(storage_status_t *status)
{
    memset(status, 0, sizeof(*status));
    status->present = 1U;
    status->ready = 1U;
    status->last_result = STORAGE_OK;
    status->detail0 = 0xEF18U;
    status->detail1 = 0x10001U;
}

int storage_platform_test_sd(uint32_t timeout_ms)
{
    g_sd_test_timeout = timeout_ms;
    return (g_sd_ready != 0U) ? STORAGE_OK : STORAGE_NOT_READY;
}

int nand_readonly_get_id(nand_id_t *id)
{
    if (g_nand_id_result != NAND_READONLY_OK)
    {
        return g_nand_id_result;
    }
    *id = g_nand_id;
    return NAND_READONLY_OK;
}

int nand_readonly_get_geometry(nand_geometry_t *geometry)
{
    if (g_nand_geometry_result != NAND_READONLY_OK)
    {
        return g_nand_geometry_result;
    }
    *geometry = g_nand_geometry;
    return NAND_READONLY_OK;
}

int nand_readonly_read_status(uint8_t *status)
{
    g_nand_status_calls++;
    if (g_nand_status_result != NAND_READONLY_OK)
    {
        return g_nand_status_result;
    }
    *status = g_nand_status_value;
    return NAND_READONLY_OK;
}

void nand_scan_get_status(nand_scan_status_t *status)
{
    *status = g_nand_scan_status;
}

static void reset_nand_fixture(void)
{
    memset(&g_nand_id, 0, sizeof(g_nand_id));
    memset(&g_nand_geometry, 0, sizeof(g_nand_geometry));
    memset(&g_nand_scan_status, 0, sizeof(g_nand_scan_status));
    g_nand_id.compact_id = 0xDC909556UL;
    g_nand_geometry.page_main_size = 2048U;
    g_nand_geometry.page_spare_size = 64U;
    g_nand_geometry.pages_per_block = 64U;
    g_nand_geometry.block_count = 4096U;
    g_nand_geometry.row_address_cycles = 3U;
    g_nand_scan_status.state = NAND_SCAN_RUNNING;
    g_nand_scan_status.scanned_blocks = 123U;
    g_nand_scan_status.bad_blocks = 2U;
    g_nand_scan_status.read_errors = 3U;
    g_nand_scan_status.timeout_errors = 4U;
    g_nand_id_result = NAND_READONLY_OK;
    g_nand_geometry_result = NAND_READONLY_OK;
    g_nand_status_result = NAND_READONLY_OK;
    g_nand_status_value = 0x40U;
    g_nand_status_calls = 0U;
}

int main(void)
{
    storage_status_t status;
    int failures = 0;

    g_sd_ready = 1U;
    reset_nand_fixture();
    if ((storage_get_status(STORAGE_SD, &status) != STORAGE_OK) ||
        (status.present != 1U) || (status.ready != 1U) ||
        (status.error_count != 3U) ||
        (status.detail0 != 0x11U) || (status.detail1 != 0x22U))
    {
        printf("FAIL sd_status\n");
        failures++;
    }

    if ((storage_get_status(STORAGE_QSPI, &status) != STORAGE_OK) ||
        (status.ready != 1U) || (status.detail0 != 0xEF18U))
    {
        printf("FAIL qspi_status\n");
        failures++;
    }

    if ((storage_get_status(STORAGE_NAND, &status) != STORAGE_OK) ||
        (status.present != 1U) || (status.ready != 1U) ||
        (status.last_result != STORAGE_OK) ||
        (status.error_count != 7U) ||
        (status.detail0 != 0xDC909556UL) ||
        (status.detail1 != 0x1002007BUL) ||
        (g_nand_status_calls != 1U))
    {
        printf("FAIL nand_known_status\n");
        failures++;
    }

    reset_nand_fixture();
    g_nand_scan_status.state = NAND_SCAN_COMPLETE;
    g_nand_scan_status.bad_blocks = 5000U;
    g_nand_scan_status.scanned_blocks = 70000U;
    g_nand_scan_status.read_errors = 0xFFFFFFFFUL;
    g_nand_scan_status.timeout_errors = 1U;
    if ((storage_get_status(STORAGE_NAND, &status) != STORAGE_OK) ||
        (status.error_count != 0xFFFFFFFFUL) ||
        (status.detail1 != 0x2FFFFFFFUL))
    {
        printf("FAIL nand_saturation\n");
        failures++;
    }

    reset_nand_fixture();
    g_nand_geometry_result = NAND_READONLY_UNKNOWN_ID;
    g_nand_scan_status.state = NAND_SCAN_ERROR;
    g_nand_scan_status.scanned_blocks = 0U;
    g_nand_scan_status.bad_blocks = 0U;
    if ((storage_get_status(STORAGE_NAND, &status) != STORAGE_NOT_READY) ||
        (status.present != 1U) || (status.ready != 0U) ||
        (status.last_result != STORAGE_NOT_READY) ||
        (status.detail0 != 0xDC909556UL) ||
        (status.detail1 != 0x40000000UL) ||
        (g_nand_status_calls != 0U))
    {
        printf("FAIL nand_unknown_id\n");
        failures++;
    }

    reset_nand_fixture();
    g_nand_id_result = NAND_READONLY_NOT_INITIALIZED;
    if ((storage_get_status(STORAGE_NAND, &status) != STORAGE_NOT_READY) ||
        (status.present != 0U) || (status.ready != 0U) ||
        (status.detail0 != 0U) || (g_nand_status_calls != 0U))
    {
        printf("FAIL nand_not_initialized\n");
        failures++;
    }

    reset_nand_fixture();
    g_nand_status_result = NAND_READONLY_TIMEOUT;
    if ((storage_get_status(STORAGE_NAND, &status) != STORAGE_IO_ERROR) ||
        (status.present != 1U) || (status.ready != 0U) ||
        (status.last_result != STORAGE_IO_ERROR))
    {
        printf("FAIL nand_status_error\n");
        failures++;
    }

    reset_nand_fixture();
    g_nand_status_value = 0x00U;
    if ((storage_get_status(STORAGE_NAND, &status) != STORAGE_NOT_READY) ||
        (status.present != 1U) || (status.ready != 0U))
    {
        printf("FAIL nand_busy_status\n");
        failures++;
    }

    if ((storage_get_status((storage_id_t)99, &status) !=
         STORAGE_INVALID_ARG) ||
        (storage_get_status(STORAGE_SD, NULL) != STORAGE_INVALID_ARG))
    {
        printf("FAIL invalid_status_arg\n");
        failures++;
    }

    if ((storage_test_sd(0U) != STORAGE_INVALID_ARG) ||
        (g_sd_test_timeout != 0U))
    {
        printf("FAIL zero_timeout\n");
        failures++;
    }

    if ((storage_test_sd(250U) != STORAGE_OK) ||
        (g_sd_test_timeout != 250U))
    {
        printf("FAIL sd_test_success\n");
        failures++;
    }

    g_sd_ready = 0U;
    if (storage_test_sd(100U) != STORAGE_NOT_READY)
    {
        printf("FAIL sd_test_not_ready\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL storage_service failures=%d\n", failures);
        return 1;
    }

    printf("PASS storage_service\n");
    return 0;
}
