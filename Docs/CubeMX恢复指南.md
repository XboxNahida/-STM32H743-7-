# CubeMX 重新生成后恢复指南

> 每次用 CubeMX 重新生成代码后，只有 `/* USER CODE BEGIN */ ... /* USER CODE END */` 之间的代码会被保留。
> 以下是会被覆盖的修改及恢复方法，**按文件列出，附上下文方便定位**。

---

## 1. main.c — D-Cache 透写

**位置**: `Core/Src/main.c`，在 `SCB_EnableDCache();` 之后

**上下文**（找到这段）:
```c
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();
  /* ↑ 在这行后面添加 ↓ */

  /* MCU Configuration--------------------------------------------------------*/
```

**恢复**: 在 `SCB_EnableDCache();` 后添加:
```c
  SCB->CACR |= 1 << 2;  /* 强制D-Cache透写, 避免DMA与Cache一致性问题 */
```

---

## 2. main.c — MPU_Config() 添加 QSPI 区域

**位置**: `Core/Src/main.c` → `void MPU_Config(void)` 函数末尾

**上下文**（找到这段）:
```c
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.SubRegionDisable = 0x00;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enables the MPU */
  HAL_MPU_Enable(MPU_HFNMI_PRIVDEF);
  /* ↑ 在这行前面插入 ↓ */
}
```

**恢复**: 在 `HAL_MPU_Enable(...)` **之前**插入:
```c
  /** Configure QSPI memory-mapped region (0x90000000) as Cacheable */
  MPU_InitStruct.Number = MPU_REGION_NUMBER7;
  MPU_InitStruct.BaseAddress = 0x90000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256MB;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.SubRegionDisable = 0x00;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);
```

---

## 3. sdmmc.c — SD 卡 1-bit 总线

**位置**: `Core/Src/sdmmc.c` → `MX_SDMMC1_SD_Init()` 函数内

**上下文**（找到这段）:
```c
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;   /* ← CubeMX 默认 4B */
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
```

**恢复**: 将 `_4B` 改为 `_1B`:
```c
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;
```
> 原因: 4-bit 模式会导致 `HAL_SD_ReadBlocks` 卡死

---

## 4. quadspi.c — QSPI 时钟分频

**位置**: `Core/Src/quadspi.c` → `MX_QUADSPI_Init()` 函数内

**上下文**（找到这段）:
```c
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 1;   /* ← CubeMX 默认 1 (75MHz) */
  hqspi.Init.FifoThreshold = 4;
```

**恢复**: 将 `ClockPrescaler` 改为 3:
```c
  hqspi.Init.ClockPrescaler = 3;   /* 37.5MHz, QPI 模式稳定 */
```
> 75MHz 在 QPI 4-4-4 模式下不稳定

---

## 5. rtc.c — 强制写入预分频器

**位置**: `Core/Src/rtc.c` → `MX_RTC_Init()` 函数内

**上下文**（找到这段）:
```c
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN Check_RTC_BKUP */
  /* ↑ 在这行前面插入 ↓ */
```

**恢复**: 在 `HAL_RTC_Init` 之后、`Check_RTC_BKUP` 之前插入:
```c
  /* 强制写入预分频器（HAL_RTC_Init 在 INITS=1 时会跳过） */
  if ((RTC->PRER & 0x7FFF) != ((127U << 16) | 255U)) {
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;
    SET_BIT(RTC->ISR, RTC_ISR_INIT);
    while (!(RTC->ISR & RTC_ISR_INITF));
    RTC->PRER = (127U << 16) | 255U;
    CLEAR_BIT(RTC->ISR, RTC_ISR_INIT);
    RTC->WPR = 0xFF;
  }
```

---

## 6. rtc.c — 首次上电时间初始化

**位置**: `Core/Src/rtc.c` → `/* USER CODE BEGIN Check_RTC_BKUP */` 区域内

**上下文**（找到这段）:
```c
  /* USER CODE BEGIN Check_RTC_BKUP */
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x5050) {
    sTime.Hours = 12;
```

**恢复**: 在设置默认时间后，同步全局变量:
```c
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x5050);
    g_rtc_hour  = 12;
    g_rtc_min   = 0;
    g_rtc_sec   = 0;
    g_rtc_year  = 26;
    g_rtc_month = 6;
    g_rtc_day   = 10;
    g_rtc_week  = 3;
```

---

## 7. rtc.c — 全局变量定义

**位置**: `Core/Src/rtc.c` → `/* USER CODE BEGIN 0 */` 区域内

**上下文**（找到这段）:
```c
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
```

**恢复**: 在 BEGIN 0 区域内添加:
```c
volatile uint8_t g_rtc_hour  = 12;
volatile uint8_t g_rtc_min   = 0;
volatile uint8_t g_rtc_sec   = 0;
volatile uint8_t g_rtc_year  = 26;
volatile uint8_t g_rtc_month = 6;
volatile uint8_t g_rtc_day   = 10;
volatile uint8_t g_rtc_week  = 3;
```

---

## 8. rtc.c — rtc_set_time() 同步

**位置**: `Core/Src/rtc.c` → `rtc_set_time()` 函数末尾

**上下文**（找到这段）:
```c
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
      return 1;
    }
    /* ↑ 在这个 return 前或成功后插入 ↓ */
```

**恢复**: 成功写入 RTC 后更新全局变量:
```c
    g_rtc_hour = hour;
    g_rtc_min  = min;
    g_rtc_sec  = sec;
```

---

## 9. rtc.c — rtc_set_date() 同步

**位置**: `Core/Src/rtc.c` → `rtc_set_date()` 函数末尾

**恢复**: 成功写入 RTC 后更新全局变量:
```c
    g_rtc_year  = year;
    g_rtc_month = month;
    g_rtc_day   = day;
    g_rtc_week  = wk;
```

---

## 10. rtc.c — rtc_tick() / rtc_update()

**位置**: `Core/Src/rtc.c` → `/* USER CODE BEGIN 1 */` 区域内（文件末尾附近）

**上下文**（找到这段）:
```c
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
```

**恢复**: 在 BEGIN 1 区域内添加完整的 `rtc_tick()` 和 `rtc_update()` 函数（参考当前文件）

---

## 11. rtc.h — 外部声明

**位置**: `Core/Inc/rtc.h`

**上下文**（找到这段）:
```c
/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */
```

**恢复**: 在 Prototypes 区域添加:
```c
extern volatile uint8_t g_rtc_hour, g_rtc_min, g_rtc_sec;
extern volatile uint8_t g_rtc_year, g_rtc_month, g_rtc_day, g_rtc_week;
void rtc_tick(void);
void rtc_update(void);
```

---

## 受保护区域（不会被覆盖）

以下代码在 USER CODE 标记内，CubeMX 会保留:

| 文件 | 标记 | 内容 |
|------|------|------|
| `main.c` | `BEGIN Includes` | `gt9xxx.h`, `hander.h`, `custom.h`, `command_parser.h`, `norflash.h`, `ff.h` |
| `main.c` | `BEGIN PV` | 全局变量 (`key_val`, `g_meas_state` 等) |
| `main.c` | `BEGIN 0` | `key_poc()`, `Runtime_logic_optimization()` |
| `main.c` | `BEGIN 2` | SD挂载、norflash预加载、LVGL初始化、定时器启动 |
| `main.c` | `BEGIN WHILE` | 主循环 |
| `main.c` | `BEGIN 4` | `g_tim6_ticks`, `HAL_TIM_PeriodElapsedCallback` |
| `sdmmc.c` | `BEGIN SDMMC1_Init 2` | 容错SD初始化 |
| `stm32h7xx_it.c` | `BEGIN SysTick_IRQn 0` | `lv_tick_inc(1)` |
| `stm32h7xx_it.c` | 各 IRQ handler | `led_poc()` 调用 |
| `ltdc.c` | `BEGIN 0` | `g_ltdc_framebuf[2]` + 视频双缓冲交换函数 |
| `ltdc.h` | `BEGIN Private defines` | `extern uint32_t *g_ltdc_framebuf[2]` |
| `ltdc.h` | `BEGIN Prototypes` | `ltdc_switch_to_video_fb()` 等 4 个函数声明 |
| `usart.c` | `BEGIN 0` | `fputc` printf 重定向 |

---

## CubeMX 配置速查

| 外设 | 配置项 | 值 |
|------|--------|-----|
| RCC | LSI | Enable |
| RTC | Activate Clock Source | LSI |
| RTC | AsynchPrediv / SynchPrediv | 127 / 255 |
| RTC | Hour Format | 24h |
| RTC | Internal Alarm | Disable |
| NVIC | TIM6 interrupt | Enable |
| NVIC | USART1 interrupt | Enable |
| NVIC | RTC global interrupt | Disable |
| QSPI | ClockPrescaler | 3 (CubeMX 默认 1, 需手动改) |
| SDMMC1 | BusWide | 1-bit (CubeMX 默认 4-bit, 需手动改) |
