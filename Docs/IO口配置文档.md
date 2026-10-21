# IO 引脚对照文档

> 适配当前 CubeMX 配置，正点原子 STM32H743IIT6 核心板 + RGB 屏

---

## 普通 GPIO

| STM32 引脚 | CubeMX 标识 | 说明 | 方向 |
|---|---|---|---|
| PA0 | WK_UP | 按键 WK_UP | 输入 |
| PB0 | LED1 | LED1 | 输出 |
| PB1 | LED0 | LED0 | 输出 |
| PB5 | LCD_BL | LCD 背光 (TIM3_CH2 PWM) | 复用输出 |
| PB12 | PCF8574_INT | PCF8574 中断 | 输入 |
| PC13 | Key2 | 按键 Key2 | 输入 |
| PH2 | Key1 | 按键 Key1 (上拉) | 输入 |
| PH3 | Key0 | 按键 Key0 (上拉) | 输入 |
| PH4 | PCF8574_IIC_SCL | PCF8574 I2C SCL | 开漏输出 |
| PH5 | PCF8574_IIC_SDA | PCF8574 I2C SDA | 开漏输出 |
| PH6 | CT_IIC_SCL | 触摸 I2C SCL | 开漏输出 |
| PH7 | TOUCH_INT | 触摸中断 (上拉) | 输入 |
| PI3 | CT_IIC_SDA | 触摸 I2C SDA | 开漏输出 |
| PI8 | TOUCH_RST | 触摸屏复位 | 推挽输出 |

---

## 外设功能引脚

### TIM 定时器

| STM32 引脚 | 定时器通道 | 功能 | 说明 |
|---|---|---|---|
| PB5 | TIM3_CH2 | LCD_BL | LCD 屏幕背光亮度 PWM |

### UART

| STM32 引脚 | UART 总线 | 说明 |
|---|---|---|
| PA9 | USART1_TX | 调试串口输出 |
| PA10 | USART1_RX | 调试串口输入 |

### I2C

| STM32 引脚 | I2C 总线 | 说明 |
|---|---|---|
| PH4 | I2C2_SCL | PCF8574 IO 扩展 |
| PH5 | I2C2_SDA | PCF8574 IO 扩展 |
| PH6 | I2C2_SCL | 触摸屏 GT9xxx |
| PI3 | I2C2_SDA | 触摸屏 GT9xxx |

### DEBUG (SWD)

| STM32 引脚 | 功能 |
|---|---|
| PA13 | SWDIO（调试数据） |
| PA14 | SWCLK（调试时钟） |

### RTC 时钟

| STM32 引脚 | 功能 |
|---|---|
| PC14 | RTC_OSC32_IN (LSE 32.768kHz) |
| PC15 | RTC_OSC32_OUT |
| PH0 | OSC_IN (HSE 25MHz) |
| PH1 | OSC_OUT |

---

## FMC SDRAM 引脚

| STM32 引脚 | 功能 | STM32 引脚 | 功能 |
|---|---|---|---|
| PC0 | FMC_SDNWE | PE10 | FMC_D7 |
| PC2 | FMC_SDNE0 | PE11 | FMC_D8 |
| PC3 | FMC_SDCKE0 | PE12 | FMC_D9 |
| PD0 | FMC_D2 | PE13 | FMC_D10 |
| PD1 | FMC_D3 | PE14 | FMC_D11 |
| PD8 | FMC_D13 | PE15 | FMC_D12 |
| PD9 | FMC_D14 | PF0 | FMC_A0 |
| PD10 | FMC_D15 | PF1 | FMC_A1 |
| PD14 | FMC_D0 | PF2 | FMC_A2 |
| PD15 | FMC_D1 | PF3 | FMC_A3 |
| PE0 | FMC_NBL0 | PF4 | FMC_A4 |
| PE1 | FMC_NBL1 | PF5 | FMC_A5 |
| PE7 | FMC_D4 | PF11 | FMC_SDNRAS |
| PE8 | FMC_D5 | PF12 | FMC_A6 |
| PE9 | FMC_D6 | PF13 | FMC_A7 |
| — | — | PF14 | FMC_A8 |
| PF15 | FMC_A9 | PG2 | FMC_A12 |
| PG0 | FMC_A10 | PG4 | FMC_BA0 |
| PG1 | FMC_A11 | PG5 | FMC_BA1 |
| PG8 | FMC_SDCLK | PG15 | FMC_SDNCAS |

---

## LTDC RGB565 引脚

| STM32 引脚 | 功能 | STM32 引脚 | 功能 |
|---|---|---|---|
| PF10 | LTDC_DE | PH15 | LTDC_G4 |
| PG6 | LTDC_R7 | PI0 | LTDC_G5 |
| PG7 | LTDC_CLK | PI1 | LTDC_G6 |
| PG11 | LTDC_B3 | PI2 | LTDC_G7 |
| PH9 | LTDC_R3 | PI4 | LTDC_B4 |
| PH10 | LTDC_R4 | PI5 | LTDC_B5 |
| PH11 | LTDC_R5 | PI6 | LTDC_B6 |
| PH12 | LTDC_R6 | PI7 | LTDC_B7 |
| PH13 | LTDC_G2 | PI9 | LTDC_VSYNC |
| PH14 | LTDC_G3 | PI10 | LTDC_HSYNC |

---

## QSPI 引脚

| STM32 引脚 | 功能 |
|---|---|
| PB2 | QUADSPI_CLK |
| PB6 | QUADSPI_NCS |
| PD11 | QUADSPI_IO0 |
| PD12 | QUADSPI_IO1 |
| PD13 | QUADSPI_IO3 |
| PE2 | QUADSPI_IO2 |

---

## SDMMC1 引脚

| STM32 引脚 | 功能 |
|---|---|
| PC8 | SDMMC1_D0 |
| PC9 | SDMMC1_D1 |
| PC10 | SDMMC1_D2 |
| PC11 | SDMMC1_D3 |
| PC12 | SDMMC1_CK |
| PD2 | SDMMC1_CMD |

---

## SAI 引脚

| STM32 引脚 | 功能 |
|---|---|
| PA4 | SAI1_FS_A |
| PA5 | SAI1_SCK_A |
| PA6 | SAI1_SD_A |
