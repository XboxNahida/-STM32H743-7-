#ifndef BOARD_I2C_H
#define BOARD_I2C_H

#include <stdint.h>

typedef enum
{
    BOARD_I2C_OK = 0,
    BOARD_I2C_INVALID_ARG,
    BOARD_I2C_NO_ACK,
    BOARD_I2C_NOT_READY,
    BOARD_I2C_TIMEOUT,
    BOARD_I2C_ERROR
} board_i2c_result_t;

board_i2c_result_t board_i2c_device_ready(uint8_t address_7bit,
                                          uint32_t timeout_ms);
board_i2c_result_t board_i2c_read_reg(uint8_t address_7bit,
                                      uint16_t reg,
                                      uint8_t reg_size,
                                      uint8_t *data,
                                      uint16_t length,
                                      uint32_t timeout_ms);
board_i2c_result_t board_i2c_command_read(uint8_t address_7bit,
                                          uint8_t command,
                                          uint8_t parameter,
                                          uint8_t *data,
                                          uint16_t length,
                                          uint32_t timeout_ms);
board_i2c_result_t board_i2c_write_reg(uint8_t address_7bit,
                                       uint16_t reg,
                                       uint8_t reg_size,
                                       const uint8_t *data,
                                       uint16_t length,
                                       uint32_t timeout_ms);

#endif
