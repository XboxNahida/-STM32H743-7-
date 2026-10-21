/**
 ********************************************************************
 * @file    pcf8574.h
 * @brief   PCF8574 I2C GPIO扩展芯片驱动
 * @note    依赖 CubeMX 生成的 I2C2 (hi2c2) 和 GPIO 初始化
 *          移植自正点原子 pcf8574.c, 适配 HAL 框架
 ********************************************************************
 */
#ifndef __PCF8574_H__
#define __PCF8574_H__

#include <stdint.h>

#ifndef PCF8574_HOST_TEST
#include "main.h"
#include "i2c.h"

extern I2C_HandleTypeDef hi2c2;
#endif

/* ===== PCF8574 I2C地址(7位地址已左移1位) ===== */
#define PCF8574_ADDR     0x40

/* ===== PCF8574 各IO功能定义(可按实际硬件修改) ===== */
#define BEEP_IO          0       /* P0: 蜂鸣器         */
#define AP_INT_IO        1       /* P1: AP3216C中断    */
#define DCMI_PWDN_IO     2       /* P2: DCMI电源控制   */
#define USB_PWR_IO       3       /* P3: USB电源控制    */
#define EX_IO            4       /* P4: 扩展IO         */
#define MPU_INT_IO       5       /* P5: MPU9250中断    */
#define RS485_RE_IO      6       /* P6: RS485_RE       */
#define ETH_RESET_IO     7       /* P7: 以太网复位     */

/* ===== INT 引脚读取 ===== */
#define PCF8574_INT      HAL_GPIO_ReadPin(PCF8574_INT_GPIO_Port, PCF8574_INT_Pin)

uint8_t pcf8574_probe_readonly(uint32_t timeout_ms);
uint8_t pcf8574_init(void);
uint8_t pcf8574_read_byte(void);
void    pcf8574_write_byte(uint8_t data);
void    pcf8574_write_bit(uint8_t bit, uint8_t sta);
uint8_t pcf8574_read_bit(uint8_t bit);
int     pcf8574_write_bit_safe(uint8_t bit, uint8_t state);
uint8_t pcf8574_get_output_shadow(void);
int     pcf8574_beep_set(uint8_t enabled);

#endif
