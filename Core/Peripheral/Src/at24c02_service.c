#include "at24c02_service.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#if defined(AT24C02_SERVICE_HOST_TEST)
extern uint32_t at24c02_test_now_ms(void);

static uint32_t at24c02_now_ms(void)
{
    return at24c02_test_now_ms();
}
#else
#include "main.h"

static uint32_t at24c02_now_ms(void)
{
    return HAL_GetTick();
}
#endif

#define AT24C02_ADDRESS_7BIT  0x50U
#define AT24C02_CAPACITY      256U
#define AT24C02_MAX_READ      64U
#define AT24C02_MAX_WRITE     64U
#define AT24C02_PAGE_SIZE     8U

static at24c02_status_t g_at24c02_status;

static int at24c02_map_board_result(board_i2c_result_t result)
{
    switch (result)
    {
        case BOARD_I2C_NO_ACK:
            return AT24C02_RESULT_ABSENT;
        case BOARD_I2C_NOT_READY:
            return AT24C02_RESULT_NOT_READY;
        case BOARD_I2C_TIMEOUT:
            return AT24C02_RESULT_TIMEOUT;
        case BOARD_I2C_ERROR:
            return AT24C02_RESULT_ERROR;
        case BOARD_I2C_OK:
            return AT24C02_RESULT_OK;
        case BOARD_I2C_INVALID_ARG:
        default:
            return AT24C02_RESULT_INVALID_ARG;
    }
}

static void at24c02_increment_saturated(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        (*value)++;
    }
}

void at24c02_service_init(void)
{
    memset(&g_at24c02_status, 0, sizeof(g_at24c02_status));
    g_at24c02_status.result = AT24C02_RESULT_NOT_READY;
}

int at24c02_probe_readonly(uint32_t timeout_ms)
{
    at24c02_status_t snapshot;
    board_i2c_result_t bus_result;
    uint32_t start_ms;
    int result;

    if (timeout_ms == 0U)
    {
        return AT24C02_RESULT_INVALID_ARG;
    }

    snapshot = g_at24c02_status;
    snapshot.present = 0U;
    snapshot.ready = 0U;
    start_ms = at24c02_now_ms();
    bus_result = board_i2c_device_ready(AT24C02_ADDRESS_7BIT, timeout_ms);
    result = at24c02_map_board_result(bus_result);
    if (result == AT24C02_RESULT_OK)
    {
        snapshot.present = 1U;
        snapshot.ready = 1U;
    }
    snapshot.result = result;
    snapshot.elapsed_ms = at24c02_now_ms() - start_ms;
    at24c02_increment_saturated(&snapshot.probe_count);
    g_at24c02_status = snapshot;
    return result;
}

int at24c02_read(uint16_t offset, uint8_t *data, uint16_t length,
                 uint32_t timeout_ms)
{
    at24c02_status_t snapshot;
    board_i2c_result_t bus_result;
    uint32_t end_offset;
    uint32_t start_ms;
    int result;

    end_offset = (uint32_t)offset + (uint32_t)length;
    if ((data == NULL) || (length == 0U) ||
        (length > AT24C02_MAX_READ) ||
        ((uint32_t)offset >= AT24C02_CAPACITY) ||
        (end_offset > AT24C02_CAPACITY) || (timeout_ms == 0U))
    {
        return AT24C02_RESULT_INVALID_ARG;
    }

    snapshot = g_at24c02_status;
    snapshot.ready = 0U;
    start_ms = at24c02_now_ms();
    bus_result = board_i2c_read_reg(AT24C02_ADDRESS_7BIT, offset, 1U,
                                    data, length, timeout_ms);
    result = at24c02_map_board_result(bus_result);
    if (result == AT24C02_RESULT_OK)
    {
        snapshot.present = 1U;
        snapshot.ready = 1U;
        snapshot.last_offset = offset;
        snapshot.last_length = length;
    }
    else if (result == AT24C02_RESULT_ABSENT)
    {
        snapshot.present = 0U;
    }
    snapshot.result = result;
    snapshot.elapsed_ms = at24c02_now_ms() - start_ms;
    at24c02_increment_saturated(&snapshot.read_count);
    g_at24c02_status = snapshot;
    return result;
}

int at24c02_write(uint16_t offset, const uint8_t *data, uint16_t length,
                  uint32_t timeout_ms)
{
    at24c02_status_t snapshot;
    board_i2c_result_t bus_result;
    uint32_t end_offset;
    uint32_t start_ms;
    uint32_t elapsed_ms;
    uint16_t written;
    int result;

    end_offset = (uint32_t)offset + (uint32_t)length;
    if ((data == NULL) || (length == 0U) ||
        (length > AT24C02_MAX_WRITE) ||
        ((uint32_t)offset >= AT24C02_CAPACITY) ||
        (end_offset > AT24C02_CAPACITY) || (timeout_ms == 0U))
    {
        return AT24C02_RESULT_INVALID_ARG;
    }

    snapshot = g_at24c02_status;
    snapshot.ready = 0U;
    start_ms = at24c02_now_ms();
    written = 0U;
    result = AT24C02_RESULT_OK;

    while (written < length)
    {
        uint16_t page_offset;
        uint16_t page_remaining;
        uint16_t remaining;
        uint16_t chunk_length;
        uint32_t operation_timeout;

        elapsed_ms = at24c02_now_ms() - start_ms;
        if (elapsed_ms >= timeout_ms)
        {
            result = AT24C02_RESULT_TIMEOUT;
            break;
        }
        operation_timeout = timeout_ms - elapsed_ms;
        page_offset = (uint16_t)((offset + written) % AT24C02_PAGE_SIZE);
        page_remaining = (uint16_t)(AT24C02_PAGE_SIZE - page_offset);
        remaining = (uint16_t)(length - written);
        chunk_length = (remaining < page_remaining) ? remaining : page_remaining;

        bus_result = board_i2c_write_reg(AT24C02_ADDRESS_7BIT,
                                         (uint16_t)(offset + written), 1U,
                                         &data[written], chunk_length,
                                         operation_timeout);
        result = at24c02_map_board_result(bus_result);
        if (result != AT24C02_RESULT_OK)
        {
            break;
        }
        written = (uint16_t)(written + chunk_length);

        for (;;)
        {
            elapsed_ms = at24c02_now_ms() - start_ms;
            if (elapsed_ms >= timeout_ms)
            {
                result = AT24C02_RESULT_TIMEOUT;
                break;
            }
            operation_timeout = timeout_ms - elapsed_ms;
            bus_result = board_i2c_device_ready(AT24C02_ADDRESS_7BIT,
                                                operation_timeout);
            if (bus_result == BOARD_I2C_OK)
            {
                result = AT24C02_RESULT_OK;
                break;
            }
            if (bus_result != BOARD_I2C_NO_ACK)
            {
                result = at24c02_map_board_result(bus_result);
                break;
            }
        }
        if (result != AT24C02_RESULT_OK)
        {
            break;
        }
    }

    if (result == AT24C02_RESULT_OK)
    {
        snapshot.present = 1U;
        snapshot.ready = 1U;
        snapshot.last_offset = offset;
        snapshot.last_length = length;
    }
    else if (result == AT24C02_RESULT_ABSENT)
    {
        snapshot.present = 0U;
    }
    snapshot.result = result;
    snapshot.elapsed_ms = at24c02_now_ms() - start_ms;
    at24c02_increment_saturated(&snapshot.write_count);
    g_at24c02_status = snapshot;
    return result;
}

void at24c02_get_status(at24c02_status_t *status)
{
    if (status != NULL)
    {
        *status = g_at24c02_status;
    }
}

void at24c02_print_status(void)
{
    printf("eeprom status model=at24c02 present=%u ready=%u result=%ld elapsed_ms=%lu probe_count=%lu read_count=%lu write_count=%lu last_offset=%u last_length=%u\r\n",
           (unsigned int)g_at24c02_status.present,
           (unsigned int)g_at24c02_status.ready,
           (long)g_at24c02_status.result,
           (unsigned long)g_at24c02_status.elapsed_ms,
           (unsigned long)g_at24c02_status.probe_count,
           (unsigned long)g_at24c02_status.read_count,
           (unsigned long)g_at24c02_status.write_count,
           (unsigned int)g_at24c02_status.last_offset,
           (unsigned int)g_at24c02_status.last_length);
}
