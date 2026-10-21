#ifndef __TOUCH_I2C_H
#define __TOUCH_I2C_H

#include "main.h"

/* I2C引脚操作宏 - 使用CubeMX生成的User Label */
#define TOUCH_SCL_HIGH()    HAL_GPIO_WritePin(CT_IIC_SCL_GPIO_Port, CT_IIC_SCL_Pin, GPIO_PIN_SET)
#define TOUCH_SCL_LOW()     HAL_GPIO_WritePin(CT_IIC_SCL_GPIO_Port, CT_IIC_SCL_Pin, GPIO_PIN_RESET)
#define TOUCH_SDA_HIGH()    HAL_GPIO_WritePin(CT_IIC_SDA_GPIO_Port, CT_IIC_SDA_Pin, GPIO_PIN_SET)
#define TOUCH_SDA_LOW()     HAL_GPIO_WritePin(CT_IIC_SDA_GPIO_Port, CT_IIC_SDA_Pin, GPIO_PIN_RESET)
#define TOUCH_SDA_READ()    HAL_GPIO_ReadPin(CT_IIC_SDA_GPIO_Port, CT_IIC_SDA_Pin)

/* RST和INT引脚操作宏 */
#define TOUCH_RST_HIGH()    HAL_GPIO_WritePin(TOUCH_RST_GPIO_Port, TOUCH_RST_Pin, GPIO_PIN_SET)
#define TOUCH_RST_LOW()     HAL_GPIO_WritePin(TOUCH_RST_GPIO_Port, TOUCH_RST_Pin, GPIO_PIN_RESET)
#define TOUCH_INT_READ()    HAL_GPIO_ReadPin(TOUCH_INT_GPIO_Port, TOUCH_INT_Pin)

/* 函数声明 */
void touch_iic_delay(void);
void touch_iic_start(void);
void touch_iic_stop(void);
uint8_t touch_iic_wait_ack(void);
void touch_iic_ack(void);
void touch_iic_nack(void);
void touch_iic_send_byte(uint8_t data);
uint8_t touch_iic_read_byte(uint8_t ack);

#endif

