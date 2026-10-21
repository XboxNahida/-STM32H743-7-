#include "nand_cache_io.h"

#include <stddef.h>
#include <string.h>

#include "nand_readonly.h"

#if defined(NAND_CACHE_IO_HOST_TEST)
extern int nand_cache_io_platform_get_geometry(nand_geometry_t *geometry);
extern int nand_cache_io_platform_read_page(uint32_t page,
                                             uint32_t column,
                                             uint8_t *buffer,
                                             uint32_t length,
                                             uint32_t timeout_ms);
extern void nand_cache_io_platform_command(uint8_t value);
extern void nand_cache_io_platform_address(uint8_t value);
extern void nand_cache_io_platform_data_write(uint8_t value);
extern uint8_t nand_cache_io_platform_data_read(void);
extern uint8_t nand_cache_io_platform_ready(void);
extern uint32_t nand_cache_io_platform_tick(void);
extern void nand_cache_io_platform_barrier(void);
#else
#include "main.h"

#define NAND_CACHE_DATA_ADDRESS     0x80000000UL
#define NAND_CACHE_COMMAND_ADDRESS \
    (NAND_CACHE_DATA_ADDRESS | (1UL << 16))
#define NAND_CACHE_LATCH_ADDRESS \
    (NAND_CACHE_DATA_ADDRESS | (1UL << 17))

static int nand_cache_io_platform_get_geometry(nand_geometry_t *geometry)
{
    return nand_readonly_get_geometry(geometry);
}

static int nand_cache_io_platform_read_page(uint32_t page,
                                             uint32_t column,
                                             uint8_t *buffer,
                                             uint32_t length,
                                             uint32_t timeout_ms)
{
    return nand_readonly_read_page(page, column, buffer, length, timeout_ms);
}

static void nand_cache_io_platform_command(uint8_t value)
{
    *(volatile uint8_t *)NAND_CACHE_COMMAND_ADDRESS = value;
}

static void nand_cache_io_platform_address(uint8_t value)
{
    *(volatile uint8_t *)NAND_CACHE_LATCH_ADDRESS = value;
}

static void nand_cache_io_platform_data_write(uint8_t value)
{
    *(volatile uint8_t *)NAND_CACHE_DATA_ADDRESS = value;
}

static uint8_t nand_cache_io_platform_data_read(void)
{
    return *(volatile uint8_t *)NAND_CACHE_DATA_ADDRESS;
}

static uint8_t nand_cache_io_platform_ready(void)
{
    return (HAL_GPIO_ReadPin(NAND_RB_GPIO_Port, NAND_RB_Pin) == GPIO_PIN_SET) ?
           1U : 0U;
}

static uint32_t nand_cache_io_platform_tick(void)
{
    return HAL_GetTick();
}

static void nand_cache_io_platform_barrier(void)
{
    __DSB();
}
#endif

static nand_cache_io_diagnostics_t g_diagnostics;

static void diagnostics_begin(nand_cache_io_operation_t operation,
                              uint32_t target)
{
    memset(&g_diagnostics, 0, sizeof(g_diagnostics));
    g_diagnostics.operation = operation;
    g_diagnostics.target = target;
    g_diagnostics.result = NAND_CACHE_IO_NOT_READY;
}

static int diagnostics_finish(int result, uint8_t ready,
                              uint8_t status_valid, uint8_t status)
{
    g_diagnostics.result = result;
    g_diagnostics.ready = ready;
    g_diagnostics.status_valid = status_valid;
    g_diagnostics.status = status;
    return result;
}

static int nand_cache_io_get_geometry(nand_geometry_t *geometry)
{
    if (nand_cache_io_platform_get_geometry(geometry) != NAND_READONLY_OK)
    {
        return NAND_CACHE_IO_NOT_READY;
    }
    return NAND_CACHE_IO_OK;
}

static int nand_cache_io_wait_operation_complete(uint32_t timeout_ms)
{
    uint32_t start = nand_cache_io_platform_tick();

    while (nand_cache_io_platform_ready() != 0U)
    {
        if ((uint32_t)(nand_cache_io_platform_tick() - start) >= timeout_ms)
        {
            return NAND_CACHE_IO_TIMEOUT;
        }
    }
    while (nand_cache_io_platform_ready() == 0U)
    {
        if ((uint32_t)(nand_cache_io_platform_tick() - start) >= timeout_ms)
        {
            return NAND_CACHE_IO_TIMEOUT;
        }
    }
    return NAND_CACHE_IO_OK;
}

static void nand_cache_io_send_row(uint32_t page)
{
    nand_cache_io_platform_address((uint8_t)page);
    nand_cache_io_platform_address((uint8_t)(page >> 8));
    nand_cache_io_platform_address((uint8_t)(page >> 16));
}

static uint8_t nand_cache_io_read_status(void)
{
    nand_cache_io_platform_command(0x70U);
    nand_cache_io_platform_barrier();
    return nand_cache_io_platform_data_read();
}

int nand_cache_io_program_page(uint32_t page,
                               uint32_t column,
                               const uint8_t *data,
                               uint32_t length,
                               uint32_t timeout_ms)
{
    nand_geometry_t geometry;
    uint32_t page_count;
    uint32_t page_total_size;
    uint32_t index;
    uint8_t status;
    int result;

    diagnostics_begin(NAND_CACHE_IO_OPERATION_PROGRAM, page);
    if ((data == NULL) || (length == 0U) || (timeout_ms == 0U))
    {
        return diagnostics_finish(NAND_CACHE_IO_INVALID_ARG, 0U, 0U, 0U);
    }
    result = nand_cache_io_get_geometry(&geometry);
    if (result != NAND_CACHE_IO_OK)
    {
        return diagnostics_finish(result, 0U, 0U, 0U);
    }

    page_count = geometry.pages_per_block * geometry.block_count;
    page_total_size = geometry.page_main_size + geometry.page_spare_size;
    if ((page >= page_count) || (column >= page_total_size) ||
        (length > (page_total_size - column)))
    {
        return diagnostics_finish(NAND_CACHE_IO_INVALID_ARG, 0U, 0U, 0U);
    }

    nand_cache_io_platform_command(0x80U);
    nand_cache_io_platform_address((uint8_t)column);
    nand_cache_io_platform_address((uint8_t)(column >> 8));
    nand_cache_io_send_row(page);
    for (index = 0U; index < length; index++)
    {
        nand_cache_io_platform_data_write(data[index]);
    }
    nand_cache_io_platform_command(0x10U);
    nand_cache_io_platform_barrier();

    result = nand_cache_io_wait_operation_complete(timeout_ms);
    if (result != NAND_CACHE_IO_OK)
    {
        return diagnostics_finish(result, 0U, 0U, 0U);
    }
    status = nand_cache_io_read_status();
    if ((status & 0x01U) != 0U)
    {
        return diagnostics_finish(NAND_CACHE_IO_PROGRAM_FAILED, 1U, 1U,
                                  status);
    }
    return diagnostics_finish(NAND_CACHE_IO_OK, 1U, 1U, status);
}

int nand_cache_io_erase_block(uint32_t block, uint32_t timeout_ms)
{
    nand_geometry_t geometry;
    uint32_t page;
    uint8_t status;
    int result;

    diagnostics_begin(NAND_CACHE_IO_OPERATION_ERASE, block);
    if (timeout_ms == 0U)
    {
        return diagnostics_finish(NAND_CACHE_IO_INVALID_ARG, 0U, 0U, 0U);
    }
    result = nand_cache_io_get_geometry(&geometry);
    if (result != NAND_CACHE_IO_OK)
    {
        return diagnostics_finish(result, 0U, 0U, 0U);
    }
    if (block >= geometry.block_count)
    {
        return diagnostics_finish(NAND_CACHE_IO_INVALID_ARG, 0U, 0U, 0U);
    }

    page = block * geometry.pages_per_block;
    nand_cache_io_platform_command(0x60U);
    nand_cache_io_send_row(page);
    nand_cache_io_platform_command(0xD0U);
    nand_cache_io_platform_barrier();

    result = nand_cache_io_wait_operation_complete(timeout_ms);
    if (result != NAND_CACHE_IO_OK)
    {
        return diagnostics_finish(result, 0U, 0U, 0U);
    }
    status = nand_cache_io_read_status();
    if ((status & 0x01U) != 0U)
    {
        return diagnostics_finish(NAND_CACHE_IO_ERASE_FAILED, 1U, 1U,
                                  status);
    }
    return diagnostics_finish(NAND_CACHE_IO_OK, 1U, 1U, status);
}

int nand_cache_io_block_is_bad(uint32_t block,
                               uint32_t timeout_ms,
                               uint8_t *is_bad)
{
    nand_geometry_t geometry;
    uint32_t first_page;
    uint8_t marker;
    int result;

    if ((is_bad == NULL) || (timeout_ms == 0U))
    {
        return NAND_CACHE_IO_INVALID_ARG;
    }
    result = nand_cache_io_get_geometry(&geometry);
    if (result != NAND_CACHE_IO_OK)
    {
        return result;
    }
    if (block >= geometry.block_count)
    {
        return NAND_CACHE_IO_INVALID_ARG;
    }

    first_page = block * geometry.pages_per_block;
    *is_bad = 0U;
    result = nand_cache_io_platform_read_page(first_page,
                                               geometry.page_main_size,
                                               &marker, 1U, timeout_ms);
    if (result != NAND_READONLY_OK)
    {
        return (result == NAND_READONLY_TIMEOUT) ?
               NAND_CACHE_IO_TIMEOUT : NAND_CACHE_IO_READ_ERROR;
    }
    if (marker != 0xFFU)
    {
        *is_bad = 1U;
        return NAND_CACHE_IO_OK;
    }

    result = nand_cache_io_platform_read_page(first_page + 1U,
                                               geometry.page_main_size,
                                               &marker, 1U, timeout_ms);
    if (result != NAND_READONLY_OK)
    {
        return (result == NAND_READONLY_TIMEOUT) ?
               NAND_CACHE_IO_TIMEOUT : NAND_CACHE_IO_READ_ERROR;
    }
    *is_bad = (marker != 0xFFU) ? 1U : 0U;
    return NAND_CACHE_IO_OK;
}

void nand_cache_io_get_diagnostics(nand_cache_io_diagnostics_t *diagnostics)
{
    if (diagnostics != NULL)
        *diagnostics = g_diagnostics;
}
