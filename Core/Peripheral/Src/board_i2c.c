#include "board_i2c.h"

#include <stddef.h>

#if defined(BOARD_I2C_HOST_TEST)
extern int board_i2c_test_device_ready(uint16_t address,
                                       uint32_t trials,
                                       uint32_t timeout_ms);
extern int board_i2c_test_mem_read(uint16_t address,
                                   uint16_t reg,
                                   uint8_t reg_size,
                                   uint8_t *data,
                                   uint16_t length,
                                   uint32_t timeout_ms);
extern int board_i2c_test_mem_write(uint16_t address,
                                    uint16_t reg,
                                    uint8_t reg_size,
                                    const uint8_t *data,
                                    uint16_t length,
                                    uint32_t timeout_ms);
extern uint32_t board_i2c_test_error(void);

#define BOARD_I2C_HAL_OK       0
#define BOARD_I2C_HAL_ERROR    1
#define BOARD_I2C_HAL_BUSY     2
#define BOARD_I2C_HAL_TIMEOUT  3
#define BOARD_I2C_ERROR_AF      0x00000004UL
#define BOARD_I2C_ERROR_TIMEOUT 0x00000020UL

static int board_i2c_hal_device_ready(uint16_t address,
                                      uint32_t timeout_ms)
{
    return board_i2c_test_device_ready(address, 1U, timeout_ms);
}

static int board_i2c_hal_mem_read(uint16_t address,
                                  uint16_t reg,
                                  uint8_t reg_size,
                                  uint8_t *data,
                                  uint16_t length,
                                  uint32_t timeout_ms)
{
    return board_i2c_test_mem_read(address, reg, reg_size, data,
                                   length, timeout_ms);
}

static int board_i2c_hal_mem_write(uint16_t address,
                                   uint16_t reg,
                                   uint8_t reg_size,
                                   const uint8_t *data,
                                   uint16_t length,
                                   uint32_t timeout_ms)
{
    return board_i2c_test_mem_write(address, reg, reg_size, data,
                                    length, timeout_ms);
}

static uint32_t board_i2c_hal_error(void)
{
    return board_i2c_test_error();
}
#else
#include "i2c.h"

static int board_i2c_hal_device_ready(uint16_t address,
                                      uint32_t timeout_ms)
{
    return (int)HAL_I2C_IsDeviceReady(&hi2c2, address, 1U, timeout_ms);
}

static int board_i2c_hal_mem_read(uint16_t address,
                                  uint16_t reg,
                                  uint8_t reg_size,
                                  uint8_t *data,
                                  uint16_t length,
                                  uint32_t timeout_ms)
{
    uint32_t mem_address_size = (reg_size == 1U) ?
                                 I2C_MEMADD_SIZE_8BIT :
                                 I2C_MEMADD_SIZE_16BIT;

    return (int)HAL_I2C_Mem_Read(&hi2c2, address, reg,
                                 mem_address_size, data, length,
                                 timeout_ms);
}

static int board_i2c_hal_mem_write(uint16_t address,
                                   uint16_t reg,
                                   uint8_t reg_size,
                                   const uint8_t *data,
                                   uint16_t length,
                                   uint32_t timeout_ms)
{
    uint32_t mem_address_size = (reg_size == 1U) ?
                                 I2C_MEMADD_SIZE_8BIT :
                                 I2C_MEMADD_SIZE_16BIT;

    return (int)HAL_I2C_Mem_Write(&hi2c2, address, reg,
                                  mem_address_size,
                                  (uint8_t *)(void *)data, length,
                                  timeout_ms);
}

static uint32_t board_i2c_hal_error(void)
{
    return HAL_I2C_GetError(&hi2c2);
}
#define BOARD_I2C_HAL_OK       HAL_OK
#define BOARD_I2C_HAL_ERROR    HAL_ERROR
#define BOARD_I2C_HAL_BUSY     HAL_BUSY
#define BOARD_I2C_HAL_TIMEOUT  HAL_TIMEOUT
#define BOARD_I2C_ERROR_AF      HAL_I2C_ERROR_AF
#define BOARD_I2C_ERROR_TIMEOUT HAL_I2C_ERROR_TIMEOUT
#endif

static board_i2c_result_t board_i2c_map_status(int hal_status)
{
    uint32_t error;

    if (hal_status == BOARD_I2C_HAL_OK)
    {
        return BOARD_I2C_OK;
    }
    if (hal_status == BOARD_I2C_HAL_BUSY)
    {
        return BOARD_I2C_NOT_READY;
    }
    if (hal_status == BOARD_I2C_HAL_TIMEOUT)
    {
        return BOARD_I2C_TIMEOUT;
    }
    error = board_i2c_hal_error();
    if ((hal_status == BOARD_I2C_HAL_ERROR) &&
        ((error & BOARD_I2C_ERROR_AF) != 0U))
    {
        return BOARD_I2C_NO_ACK;
    }
    return BOARD_I2C_ERROR;
}

static board_i2c_result_t board_i2c_map_ready_status(int hal_status)
{
    uint32_t error;

    if (hal_status == BOARD_I2C_HAL_ERROR)
    {
        error = board_i2c_hal_error();
        if ((error & (BOARD_I2C_ERROR_AF | BOARD_I2C_ERROR_TIMEOUT)) != 0U)
        {
            return BOARD_I2C_NO_ACK;
        }
    }
    return board_i2c_map_status(hal_status);
}

static board_i2c_result_t board_i2c_validate(uint8_t address_7bit,
                                             uint8_t reg_size,
                                             const uint8_t *data,
                                             uint16_t length,
                                             uint32_t timeout_ms)
{
    if ((address_7bit >= 0x80U) ||
        ((reg_size != 1U) && (reg_size != 2U)) ||
        (data == NULL) || (length == 0U) || (timeout_ms == 0U))
    {
        return BOARD_I2C_INVALID_ARG;
    }
    return BOARD_I2C_OK;
}

board_i2c_result_t board_i2c_device_ready(uint8_t address_7bit,
                                          uint32_t timeout_ms)
{
    int hal_status;

    if ((address_7bit >= 0x80U) || (timeout_ms == 0U))
    {
        return BOARD_I2C_INVALID_ARG;
    }
    hal_status = board_i2c_hal_device_ready((uint16_t)address_7bit << 1,
                                            timeout_ms);
    return board_i2c_map_ready_status(hal_status);
}

board_i2c_result_t board_i2c_read_reg(uint8_t address_7bit,
                                      uint16_t reg,
                                      uint8_t reg_size,
                                      uint8_t *data,
                                      uint16_t length,
                                      uint32_t timeout_ms)
{
    int hal_status;
    board_i2c_result_t validation;

    validation = board_i2c_validate(address_7bit, reg_size, data,
                                    length, timeout_ms);
    if (validation != BOARD_I2C_OK)
    {
        return validation;
    }
    hal_status = board_i2c_hal_mem_read((uint16_t)address_7bit << 1,
                                        reg, reg_size, data, length,
                                        timeout_ms);
    return board_i2c_map_status(hal_status);
}

board_i2c_result_t board_i2c_command_read(uint8_t address_7bit,
                                          uint8_t command,
                                          uint8_t parameter,
                                          uint8_t *data,
                                          uint16_t length,
                                          uint32_t timeout_ms)
{
    uint16_t combined = ((uint16_t)command << 8) | parameter;

    return board_i2c_read_reg(address_7bit, combined, 2U,
                              data, length, timeout_ms);
}

board_i2c_result_t board_i2c_write_reg(uint8_t address_7bit,
                                       uint16_t reg,
                                       uint8_t reg_size,
                                       const uint8_t *data,
                                       uint16_t length,
                                       uint32_t timeout_ms)
{
    int hal_status;
    board_i2c_result_t validation;

    validation = board_i2c_validate(address_7bit, reg_size, data,
                                    length, timeout_ms);
    if (validation != BOARD_I2C_OK)
    {
        return validation;
    }
    hal_status = board_i2c_hal_mem_write((uint16_t)address_7bit << 1,
                                         reg, reg_size, data, length,
                                         timeout_ms);
    return board_i2c_map_status(hal_status);
}
