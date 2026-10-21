#include "storage_service.h"

#include <stdio.h>
#include <string.h>

#include "nand_readonly.h"
#include "nand_scan_service.h"

#if defined(STORAGE_SERVICE_HOST_TEST)
extern void storage_platform_read_sd_status(storage_status_t *status);
extern void storage_platform_read_qspi_status(storage_status_t *status);
extern int storage_platform_test_sd(uint32_t timeout_ms);
#else
#include "norflash.h"
#include "quadspi.h"
#include "sdcard.h"

static void storage_platform_read_sd_status(storage_status_t *status)
{
    const sdcard_diag_t *diag = sdcard_get_diag();

    memset(status, 0, sizeof(*status));
    status->present = sdcard_is_ready();
    status->ready = status->present;
    status->last_result = STORAGE_OK;

    if (diag != NULL)
    {
        status->error_count = diag->consecutive_errors;
        status->detail0 = diag->hal_error;
        status->detail1 = diag->card_state;
        if (diag->valid != 0U)
        {
            status->last_result = (diag->fatfs_result != -1) ?
                                  diag->fatfs_result :
                                  (int32_t)diag->hal_error;
        }
    }
    if (status->ready == 0U)
    {
        status->last_result = STORAGE_NOT_READY;
    }
}

static void storage_platform_read_qspi_status(storage_status_t *status)
{
    HAL_QSPI_StateTypeDef hal_state = hqspi.State;
    uint32_t memmap_active =
        ((QUADSPI->CCR & QUADSPI_CCR_FMODE_Msk) ==
         (3UL << QUADSPI_CCR_FMODE_Pos)) ? 1U : 0U;

    memset(status, 0, sizeof(*status));
    status->present = ((g_norflash_type != 0U) &&
                       (g_norflash_type != 0xFFFFU)) ? 1U : 0U;
    status->ready = ((status->present != 0U) &&
                     ((hal_state == HAL_QSPI_STATE_READY) ||
                      (hal_state == HAL_QSPI_STATE_BUSY_MEM_MAPPED))) ? 1U : 0U;
    status->last_result = (status->ready != 0U) ?
                          STORAGE_OK : STORAGE_NOT_READY;
    status->detail0 = g_norflash_type;
    status->detail1 = ((uint32_t)hal_state & 0xFFU) |
                      (((uint32_t)g_norflash_mode & 0x01U) << 8) |
                      (memmap_active << 9);
}

static int storage_platform_test_sd(uint32_t timeout_ms)
{
    uint32_t sector_count;
    uint32_t erase_block_sectors;

    if (sdcard_is_ready() == 0U)
    {
        return STORAGE_NOT_READY;
    }
    if (sdcard_get_info(&sector_count, &erase_block_sectors) != 0U)
    {
        return STORAGE_IO_ERROR;
    }
    if ((sector_count == 0U) || (erase_block_sectors == 0U))
    {
        return STORAGE_IO_ERROR;
    }
    return (sdcard_sync(timeout_ms) == 0U) ?
           STORAGE_OK : STORAGE_IO_ERROR;
}
#endif

static uint32_t storage_saturating_add(uint32_t left, uint32_t right)
{
    if (right > (0xFFFFFFFFUL - left))
    {
        return 0xFFFFFFFFUL;
    }
    return left + right;
}

static uint32_t storage_saturate_field(uint32_t value, uint32_t maximum)
{
    return (value > maximum) ? maximum : value;
}

static void storage_platform_read_nand_status(storage_status_t *status)
{
    nand_id_t id;
    nand_geometry_t geometry;
    nand_scan_status_t scan_status;
    uint8_t nand_status;
    int result;

    memset(status, 0, sizeof(*status));
    memset(&id, 0, sizeof(id));
    memset(&geometry, 0, sizeof(geometry));
    memset(&scan_status, 0, sizeof(scan_status));
    nand_scan_get_status(&scan_status);

    status->error_count = storage_saturating_add(
        scan_status.read_errors, scan_status.timeout_errors);
    status->detail1 =
        (((uint32_t)scan_status.state & 0x0FU) << 28) |
        (storage_saturate_field(scan_status.bad_blocks, 0x0FFFU) << 16) |
        storage_saturate_field(scan_status.scanned_blocks, 0xFFFFU);

    result = nand_readonly_get_id(&id);
    if (result != NAND_READONLY_OK)
    {
        status->last_result = STORAGE_NOT_READY;
        return;
    }

    status->present = 1U;
    status->detail0 = id.compact_id;
    result = nand_readonly_get_geometry(&geometry);
    if (result != NAND_READONLY_OK)
    {
        status->last_result = (result == NAND_READONLY_UNKNOWN_ID) ?
                              STORAGE_NOT_READY : STORAGE_IO_ERROR;
        return;
    }

    result = nand_readonly_read_status(&nand_status);
    if (result != NAND_READONLY_OK)
    {
        status->last_result = STORAGE_IO_ERROR;
        return;
    }

    status->ready = ((nand_status & 0x40U) != 0U) ? 1U : 0U;
    status->last_result = (status->ready != 0U) ?
                          STORAGE_OK : STORAGE_NOT_READY;
}

int storage_get_status(storage_id_t id, storage_status_t *status)
{
    if ((status == NULL) || ((uint32_t)id >= (uint32_t)STORAGE_COUNT))
    {
        return STORAGE_INVALID_ARG;
    }

    switch (id)
    {
        case STORAGE_SD:
            storage_platform_read_sd_status(status);
            return status->last_result;

        case STORAGE_NAND:
            storage_platform_read_nand_status(status);
            return status->last_result;

        case STORAGE_QSPI:
            storage_platform_read_qspi_status(status);
            return status->last_result;

        default:
            return STORAGE_INVALID_ARG;
    }
}

int storage_test_sd(uint32_t timeout_ms)
{
    if (timeout_ms == 0U)
    {
        return STORAGE_INVALID_ARG;
    }
    return storage_platform_test_sd(timeout_ms);
}

void storage_print_status(void)
{
    static const char *names[STORAGE_COUNT] = {"sd", "nand", "qspi"};
    storage_status_t status;
    uint32_t id;

    for (id = 0U; id < (uint32_t)STORAGE_COUNT; id++)
    {
        (void)storage_get_status((storage_id_t)id, &status);
        printf("storage id=%s present=%u ready=%u result=%ld errors=%lu detail0=0x%08lX detail1=0x%08lX\r\n",
               names[id],
               (unsigned int)status.present,
               (unsigned int)status.ready,
               (long)status.last_result,
               (unsigned long)status.error_count,
               (unsigned long)status.detail0,
               (unsigned long)status.detail1);
    }
}
