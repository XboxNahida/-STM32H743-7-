/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
// ===== 非阻塞状态机 =====
typedef enum {
    MEAS_IDLE = 0,           // 空闲，等待触发
} State_Machine;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern uint8_t key_val;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TOUCH_RST_Pin GPIO_PIN_8
#define TOUCH_RST_GPIO_Port GPIOI
#define Key2_Pin GPIO_PIN_13
#define Key2_GPIO_Port GPIOC
#define WK_UP_Pin GPIO_PIN_0
#define WK_UP_GPIO_Port GPIOA
#define Key1_Pin GPIO_PIN_2
#define Key1_GPIO_Port GPIOH
#define Key0_Pin GPIO_PIN_3
#define Key0_GPIO_Port GPIOH
#define PCF8574_IIC_SCL_Pin GPIO_PIN_4
#define PCF8574_IIC_SCL_GPIO_Port GPIOH
#define PCF8574_IIC_SDA_Pin GPIO_PIN_5
#define PCF8574_IIC_SDA_GPIO_Port GPIOH
#define LED1_Pin GPIO_PIN_0
#define LED1_GPIO_Port GPIOB
#define LED0_Pin GPIO_PIN_1
#define LED0_GPIO_Port GPIOB
#define CT_IIC_SCL_Pin GPIO_PIN_6
#define CT_IIC_SCL_GPIO_Port GPIOH
#define TOUCH_INT_Pin GPIO_PIN_7
#define TOUCH_INT_GPIO_Port GPIOH
#define PCF8574_INT_Pin GPIO_PIN_12
#define PCF8574_INT_GPIO_Port GPIOB
#define CT_IIC_SDA_Pin GPIO_PIN_3
#define CT_IIC_SDA_GPIO_Port GPIOI
#define DCMI_RESET_Pin GPIO_PIN_15
#define DCMI_RESET_GPIO_Port GPIOA
#define NAND_RB_Pin GPIO_PIN_6
#define NAND_RB_GPIO_Port GPIOD
#define SCCB_SCL_Pin GPIO_PIN_4
#define SCCB_SCL_GPIO_Port GPIOB
#define SCCB_SDA_Pin GPIO_PIN_3
#define SCCB_SDA_GPIO_Port GPIOB
#define LCD_BL_Pin GPIO_PIN_5
#define LCD_BL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
// ====== 添加串口接收相关 ======
#define UART_RX_BUF_SIZE    128     // 接收缓冲区大小
#define UART_DISPLAY_BUF_SIZE  8192 // 显示缓冲区大小
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
