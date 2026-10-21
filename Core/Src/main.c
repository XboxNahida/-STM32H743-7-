/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "dma2d.h"
#include "i2c.h"
#include "jpeg.h"
#include "ltdc.h"
#include "quadspi.h"
#include "rtc.h"
#include "sai.h"
#include "sdmmc.h"
#include "spdifrx.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "gt9xxx.h"
#include "hander.h"
#include "custom.h"           /* app_lvgl_init / app_lvgl_handler */
#include "command_parser.h"   /* uart_rx_buf */
#include "app_event.h"
#include "adc_measurement_service.h"
#include "audio_playback_service.h"
#include "audio_recording_service.h"
#include "ap3216c_service.h"
#include "at24c02_service.h"
#include "system_settings_service.h"
#include "diagnostics.h"
#include "resource_manager.h"
#include "app_runtime.h"
#include "nes_runtime.h"
#include "nand_readonly.h"
#include "nand_scan_service.h"
#include "boot_probe_service.h"
#include "boot_report_service.h"
#include "imu_service.h"
#include "ir_remote_service.h"
#include "sh3001_service.h"
#include "st480mc_service.h"
#include "tpad_service.h"
#include "pressure_service.h"
#include "norflash.h"
#include "sdcard_manager.h"
#include "camera_preview_service.h"
#include "camera_ui_bridge.h"
#include "boot_progress_service.h"
#include "boot_progress_ui.h"
#include "font_cache_service.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BOOT_PROGRESS_COPY_REFRESH_BYTES (256UL * 1024UL)
#define BOOT_PROGRESS_COPY_REFRESH_PERCENT 5U
#define BOOT_PROGRESS_COPY_REFRESH_MS 200U
#define FONT_CACHE_PROGRESS_STEP_PERCENT 5U
#define FONT_CACHE_PROGRESS_REFRESH_MS 500U
#define BOOT_ZH_READY "\xE6\xAD\xA3\xE5\xB8\xB8"
#define BOOT_ZH_INIT_FAILED "\xE5\x88\x9D\xE5\xA7\x8B\xE5\x8C\x96\xE5\xA4\xB1\xE8\xB4\xA5"
#define BOOT_ZH_NOT_DETECTED "\xE6\x9C\xAA\xE6\xA3\x80\xE6\xB5\x8B\xE5\x88\xB0"
#define BOOT_ZH_UNAVAILABLE "\xE4\xB8\x8D\xE5\x8F\xAF\xE7\x94\xA8"
#define BOOT_ZH_MOUNTED "\xE5\xB7\xB2\xE6\x8C\x82\xE8\xBD\xBD"
#define BOOT_ZH_ABSENT "\xE7\xBC\xBA\xE5\xA4\xB1"
#define BOOT_ZH_NOT_READY "\xE6\x9C\xAA\xE5\xB0\xB1\xE7\xBB\xAA"
#define BOOT_ZH_TIMEOUT "\xE8\xB6\x85\xE6\x97\xB6"
#define BOOT_ZH_UNSUPPORTED "\xE4\xB8\x8D\xE6\x94\xAF\xE6\x8C\x81"
#define BOOT_ZH_ERROR "\xE9\x94\x99\xE8\xAF\xAF"
#define BOOT_ZH_RESOURCES "\xE5\x90\xAF\xE5\x8A\xA8\xE8\xB5\x84\xE6\xBA\x90"
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* ---- 按键 ---- */
uint8_t  key_val, key_old, key_up, key_down;
uint16_t key_slow, time_1s;

/* ---- 状态机 ---- */
State_Machine g_meas_state    = MEAS_IDLE;
uint32_t      g_meas_timestamp = 0;
static uint8_t g_boot_sd_ready;
static norflash_boot_status_t g_boot_asset_status;
static app_lvgl_boot_status_t g_boot_lvgl_status;
static const char *g_boot_resource_item;
static norflash_progress_action_t g_boot_resource_action;
static uint32_t g_boot_resource_processed;
static uint32_t g_boot_resource_refresh_ms;
static uint8_t g_boot_resource_percent;
static uint32_t g_boot_font_cache_refresh_ms;
static uint8_t g_boot_font_cache_percent;
static uint8_t g_boot_chinese_ready;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void system_settings_capture_rtc(system_settings_t *settings)
{
    settings->rtc_year = g_rtc_year;
    settings->rtc_month = g_rtc_month;
    settings->rtc_day = g_rtc_day;
    settings->rtc_hour = g_rtc_hour;
    settings->rtc_minute = g_rtc_min;
    settings->rtc_second = g_rtc_sec;
}

static void system_settings_rtc_changed(void)
{
    system_settings_request_save();
}

static void system_settings_bootstrap(void)
{
    system_settings_t live_settings;
    system_settings_t effective_settings;
    uint8_t backup_valid;
    int result;

    memset(&live_settings, 0, sizeof(live_settings));
    live_settings.brightness_percent = 50U;
    live_settings.volume_percent = 50U;
    system_settings_capture_rtc(&live_settings);
    backup_valid = rtc_backup_was_valid();
    result = system_settings_runtime_start(backup_valid, &live_settings,
                                           &effective_settings,
                                           HAL_GetTick(), 30U);
    if ((result == SYSTEM_SETTINGS_OK) && (backup_valid == 0U))
    {
        (void)rtc_set_date(effective_settings.rtc_year,
                           effective_settings.rtc_month,
                           effective_settings.rtc_day);
        (void)rtc_set_time(effective_settings.rtc_hour,
                           effective_settings.rtc_minute,
                           effective_settings.rtc_second);
    }
    rtc_set_change_callback(system_settings_rtc_changed);
}

static void system_settings_checkpoint_poll(uint32_t now_ms)
{
    system_settings_t live_settings;

    memset(&live_settings, 0, sizeof(live_settings));
    system_settings_get(&live_settings);
    system_settings_capture_rtc(&live_settings);
    (void)system_settings_runtime_poll(&live_settings, now_ms, 30U);
}

/* ---- 按键处理 ---- */
void key_poc(void)
{
    if (key_slow) return;
    key_slow = 1;
    key_val  = key_read();
    key_down = key_old & (key_val ^ key_old);
    key_up   = ~key_old & (key_val ^ key_old);
    key_old  = key_val;
}

/* ---- 非阻塞状态机 ---- */
void Runtime_logic_optimization(void)
{
    switch (g_meas_state) {
    case MEAS_IDLE:
        break;
    }
}

static void boot_progress_refresh_snapshot(void)
{
    boot_progress_snapshot_t snapshot;

    boot_progress_get_snapshot(&snapshot);
    boot_progress_ui_refresh(&snapshot);
}

static void boot_progress_complete_internal(const char *name,
                                            uint8_t ready,
                                            uint8_t total_percent)
{
    boot_progress_complete_item(name,
                                (ready != 0U) ? BOOT_PROGRESS_SUCCESS :
                                                BOOT_PROGRESS_ERROR,
                                (ready != 0U) ?
                                (g_boot_chinese_ready ? BOOT_ZH_READY : "Ready") :
                                (g_boot_chinese_ready ? BOOT_ZH_INIT_FAILED :
                                                        "Init failed"),
                                total_percent);
    boot_progress_refresh_snapshot();
}

static void boot_progress_resource_range(const char *item,
                                         uint8_t *start,
                                         uint8_t *end)
{
    *start = 15U;
    *end = 20U;
    if (strcmp(item, "wallpaper") == 0) { *start = 20U; *end = 30U; }
    else if (strcmp(item, "font12") == 0) { *start = 30U; *end = 34U; }
    else if (strcmp(item, "img1") == 0) { *start = 55U; *end = 57U; }
    else if (strcmp(item, "img2") == 0) { *start = 57U; *end = 59U; }
    else if (strcmp(item, "img3") == 0) { *start = 59U; *end = 62U; }
    else if (strcmp(item, "font14") == 0) { *start = 62U; *end = 66U; }
    else if (strcmp(item, "font16") == 0) { *start = 66U; *end = 71U; }
    else if (strcmp(item, "font18") == 0) { *start = 71U; *end = 77U; }
    else if (strcmp(item, "font20") == 0) { *start = 77U; *end = 90U; }
    else if (strcmp(item, "font30") == 0) { *start = 90U; *end = 96U; }
}

static uint8_t boot_progress_should_refresh(const norflash_progress_t *progress,
                                            uint8_t phase_changed,
                                            uint32_t now_ms)
{
    if ((phase_changed != 0U) ||
        (progress->action == NORFLASH_PROGRESS_DONE) ||
        (progress->action == NORFLASH_PROGRESS_ERROR)) { return 1U; }
    if (progress->action == NORFLASH_PROGRESS_LOAD) { return 0U; }
    if ((progress->processed >= g_boot_resource_processed) &&
        ((progress->processed - g_boot_resource_processed) >=
         BOOT_PROGRESS_COPY_REFRESH_BYTES)) { return 1U; }
    if (progress->percent >= (uint8_t)(g_boot_resource_percent +
                                      BOOT_PROGRESS_COPY_REFRESH_PERCENT)) {
        return 1U;
    }
    return ((now_ms - g_boot_resource_refresh_ms) >=
            BOOT_PROGRESS_COPY_REFRESH_MS) ? 1U : 0U;
}

static void boot_progress_resource_callback(const norflash_progress_t *progress,
                                            void *context)
{
    char detail[BOOT_PROGRESS_DETAIL_TEXT_SIZE];
    uint32_t now_ms;
    uint8_t range_start;
    uint8_t range_end;
    uint8_t total_percent;
    uint8_t phase_changed;
    uint8_t should_refresh;

    (void)context;
    if ((progress == NULL) || (progress->item == NULL)) { return; }
    now_ms = HAL_GetTick();
    phase_changed = ((g_boot_resource_item == NULL) ||
                     (strcmp(g_boot_resource_item, progress->item) != 0) ||
                     (g_boot_resource_action != progress->action)) ? 1U : 0U;
    should_refresh = boot_progress_should_refresh(progress, phase_changed,
                                                  now_ms);
    if (should_refresh == 0U) { return; }

    boot_progress_resource_range(progress->item, &range_start, &range_end);
    total_percent = (uint8_t)(range_start +
        (((uint32_t)(range_end - range_start) * progress->percent) / 100U));
    if ((g_boot_chinese_ready != 0U) &&
        (progress->action == NORFLASH_PROGRESS_COPY)) {
        (void)snprintf(detail, sizeof(detail),
                       "\xE6\xAD\xA3\xE5\x9C\xA8\xE4\xBB\x8ESD\xE5\x8D\xA1\xE5\xA4\x8D\xE5\x88\xB6 %s... %u%%",
                       progress->item, (unsigned int)progress->percent);
    } else if ((g_boot_chinese_ready != 0U) &&
               (progress->action == NORFLASH_PROGRESS_ERASE)) {
        (void)snprintf(detail, sizeof(detail),
                       "\xE6\xAD\xA3\xE5\x9C\xA8\xE6\x93\xA6\xE9\x99\xA4 %s... %u%%",
                       progress->item, (unsigned int)progress->percent);
    } else if ((g_boot_chinese_ready != 0U) &&
               (progress->action == NORFLASH_PROGRESS_VERIFY)) {
        (void)snprintf(detail, sizeof(detail),
                       "\xE6\xAD\xA3\xE5\x9C\xA8\xE9\xAA\x8C\xE8\xAF\x81 %s... %u%%",
                       progress->item, (unsigned int)progress->percent);
    } else if (g_boot_chinese_ready != 0U) {
        (void)snprintf(detail, sizeof(detail),
                       "\xE6\xAD\xA3\xE5\x9C\xA8\xE4\xBB\x8EQSPI\xE5\x8A\xA0\xE8\xBD\xBD %s... %u%%",
                       progress->item, (unsigned int)progress->percent);
    } else if (progress->action == NORFLASH_PROGRESS_COPY) {
        (void)snprintf(detail, sizeof(detail), "Copying %s from SD... %u%%",
                       progress->item, (unsigned int)progress->percent);
    } else if (progress->action == NORFLASH_PROGRESS_ERASE) {
        (void)snprintf(detail, sizeof(detail), "Erasing %s... %u%%",
                       progress->item, (unsigned int)progress->percent);
    } else if (progress->action == NORFLASH_PROGRESS_VERIFY) {
        (void)snprintf(detail, sizeof(detail), "Verifying %s... %u%%",
                       progress->item, (unsigned int)progress->percent);
    } else {
        (void)snprintf(detail, sizeof(detail), "Loading %s from QSPI... %u%%",
                       progress->item, (unsigned int)progress->percent);
    }
    boot_progress_set_current(g_boot_chinese_ready ? BOOT_ZH_RESOURCES :
                                                     "Boot resources",
                              detail, total_percent,
                              progress->percent);
    if (progress->action == NORFLASH_PROGRESS_ERROR) {
        boot_progress_complete_item(progress->item, BOOT_PROGRESS_ERROR,
                                    g_boot_chinese_ready ?
                                    "\xE8\xB5\x84\xE6\xBA\x90\xE5\xA4\xB1\xE8\xB4\xA5" :
                                    "Resource failed", total_percent);
    }
    boot_progress_refresh_snapshot();
    g_boot_resource_item = progress->item;
    g_boot_resource_action = progress->action;
    g_boot_resource_processed = progress->processed;
    g_boot_resource_percent = progress->percent;
    g_boot_resource_refresh_ms = now_ms;
}

static boot_progress_level_t boot_progress_probe_level(int32_t result)
{
    if (result == BOOT_PROBE_OK) { return BOOT_PROGRESS_SUCCESS; }
    if ((result == BOOT_PROBE_TIMEOUT) || (result == BOOT_PROBE_ERROR)) {
        return BOOT_PROGRESS_ERROR;
    }
    return BOOT_PROGRESS_WARNING;
}

static const char *boot_progress_probe_detail(int32_t result)
{
    if (g_boot_chinese_ready != 0U) {
        switch (result) {
            case BOOT_PROBE_OK: return BOOT_ZH_READY;
            case BOOT_PROBE_ABSENT: return BOOT_ZH_ABSENT;
            case BOOT_PROBE_NOT_READY: return BOOT_ZH_NOT_READY;
            case BOOT_PROBE_TIMEOUT: return BOOT_ZH_TIMEOUT;
            case BOOT_PROBE_UNSUPPORTED: return BOOT_ZH_UNSUPPORTED;
            default: return BOOT_ZH_ERROR;
        }
    }
    switch (result) {
        case BOOT_PROBE_OK: return "Ready";
        case BOOT_PROBE_ABSENT: return "Absent";
        case BOOT_PROBE_NOT_READY: return "Not ready";
        case BOOT_PROBE_TIMEOUT: return "Timeout";
        case BOOT_PROBE_UNSUPPORTED: return "Unsupported";
        default: return "Error";
    }
}

static void boot_progress_apply_probe_status(uint32_t index)
{
    boot_probe_status_t status;
    boot_progress_device_id_t device_id;
    uint8_t mapped = 1U;

    if (boot_probe_get_status(index, &status) != BOOT_PROBE_OK) { return; }
    if (strcmp(status.id, "sd") == 0) { device_id = BOOT_DEVICE_SD; }
    else if (strcmp(status.id, "qspi_flash") == 0) { device_id = BOOT_DEVICE_QSPI; }
    else if (strcmp(status.id, "nand") == 0) { device_id = BOOT_DEVICE_NAND; }
    else if (strcmp(status.id, "gt9xxx") == 0) { device_id = BOOT_DEVICE_GT911; }
    else if (strcmp(status.id, "pcf8574") == 0) { device_id = BOOT_DEVICE_PCF8574; }
    else if (strcmp(status.id, "ov5640") == 0) { device_id = BOOT_DEVICE_OV5640; }
    else if (strcmp(status.id, "es8388") == 0) { device_id = BOOT_DEVICE_ES8388; }
    else if (strcmp(status.id, "ap3216c") == 0) { device_id = BOOT_DEVICE_AP3216C; }
    else if (strcmp(status.id, "24c02") == 0) { device_id = BOOT_DEVICE_AT24C02; }
    else if (strcmp(status.id, "imu") == 0) {
        device_id = BOOT_DEVICE_SH3001;
        if ((status.detail0 >> 16) != (uint32_t)IMU_MODEL_SH3001) {
            boot_progress_set_device(device_id, BOOT_PROGRESS_WARNING,
                                     g_boot_chinese_ready ? BOOT_ZH_NOT_DETECTED :
                                                            "Not detected");
            boot_progress_refresh_snapshot();
            return;
        }
    } else { mapped = 0U; device_id = BOOT_DEVICE_SD; }

    if (mapped != 0U) {
        boot_progress_set_device(device_id,
                                 boot_progress_probe_level(status.result),
                                 boot_progress_probe_detail(status.result));
        boot_progress_refresh_snapshot();
    }
}

static uint8_t boot_progress_load_font(const char *name,
                                       void (*init_font)(void),
                                       const lv_font_t *display_font,
                                       uint8_t total_percent)
{
    norflash_boot_status_t before;
    uint8_t ready;

    norflash_get_boot_status(&before);
    init_font();
    norflash_get_boot_status(&g_boot_asset_status);
    ready = (g_boot_asset_status.fonts_ready > before.fonts_ready) ? 1U : 0U;
    boot_progress_complete_internal(name, ready, total_percent);
    if ((ready != 0U) && (display_font != NULL)) {
        boot_progress_ui_set_font(display_font);
        boot_progress_refresh_snapshot();
    }
    return ready;
}

static void boot_progress_font_cache_callback(uint32_t completed_pages,
                                              uint32_t total_pages,
                                              void *context)
{
    uint32_t now;
    uint8_t item_percent;
    uint8_t total_percent;
    char detail[64];

    (void)context;
    if ((total_pages == 0U) || (completed_pages > total_pages)) {
        return;
    }
    now = HAL_GetTick();
    item_percent = (uint8_t)(((uint64_t)completed_pages * 100U) /
                             total_pages);
    if ((completed_pages != total_pages) &&
        (item_percent < (uint8_t)(g_boot_font_cache_percent +
                                  FONT_CACHE_PROGRESS_STEP_PERCENT)) &&
        ((now - g_boot_font_cache_refresh_ms) <
         FONT_CACHE_PROGRESS_REFRESH_MS)) {
        return;
    }
    total_percent = (uint8_t)(62U + (((uint32_t)32U * item_percent) / 100U));
    (void)snprintf(detail, sizeof(detail),
                   "Loading fonts from NAND... %u%%",
                   (unsigned int)item_percent);
    boot_progress_set_current("Font cache", detail, total_percent,
                              item_percent);
    boot_progress_refresh_snapshot();
    g_boot_font_cache_percent = item_percent;
    g_boot_font_cache_refresh_ms = now;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* Keep D-Cache write-through for DMA-heavy LTDC/JPEG/SD paths. */
  SCB->CACR |= 1U << 2;

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  boot_report_init(HAL_GetTick());

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_FMC_Init();
  MX_LTDC_Init();
  MX_TIM3_Init();
  MX_TIM6_Init();
  MX_USART1_UART_Init();
  MX_DMA2D_Init();
  /* BOOT_PROGRESS USER PATCH BEGIN
   * CubeMX regeneration requires restoring this progress hook.
   */
  boot_progress_init();
  if (!app_lvgl_bootstrap_init())
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2) != HAL_OK)
  {
    boot_progress_complete_internal("Backlight", 0U, 8U);
    Error_Handler();
  }
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 499);
  boot_progress_complete_internal("DMA2D",
                                  (hdma2d.State == HAL_DMA2D_STATE_READY), 8U);
  MX_I2C2_Init();
  boot_progress_complete_internal("I2C2",
                                  (hi2c2.State == HAL_I2C_STATE_READY), 9U);
  MX_QUADSPI_Init();
  boot_progress_complete_internal("QSPI",
                                  (hqspi.State == HAL_QSPI_STATE_READY), 9U);
  MX_SAI1_Init();
  boot_progress_complete_internal("SAI1",
                                  (hsai_BlockA1.State == HAL_SAI_STATE_READY), 10U);
  MX_SDMMC1_SD_Init();
  boot_progress_complete_internal("SDMMC1",
                                  (hsd1.State == HAL_SD_STATE_READY), 10U);
  MX_RTC_Init();
  boot_progress_complete_internal("RTC",
                                  (hrtc.State == HAL_RTC_STATE_READY), 11U);
  MX_JPEG_Init();
  boot_progress_complete_internal("JPEG",
                                  (hjpeg.State == HAL_JPEG_STATE_READY), 12U);
  MX_ADC1_Init();
  boot_progress_complete_internal("ADC1",
                                  ((hadc1.State & HAL_ADC_STATE_READY) != 0U), 12U);
  MX_ADC3_Init();
  boot_progress_complete_internal("ADC3",
                                  ((hadc3.State & HAL_ADC_STATE_READY) != 0U), 13U);
  MX_SPDIFRX1_Init();
  boot_progress_complete_internal("SPDIFRX1",
                                  (hspdif1.State == HAL_SPDIFRX_STATE_READY), 14U);
  MX_TIM1_Init();
  boot_progress_complete_internal("TIM1",
                                  (htim1.State == HAL_TIM_STATE_READY), 14U);
  MX_TIM2_Init();
  boot_progress_complete_internal("TIM2",
                                  (htim2.State == HAL_TIM_STATE_READY), 15U);
  /* BOOT_PROGRESS USER PATCH END */
  /* USER CODE BEGIN 2 */
  uint8_t font_cache_hit = 0U;
  int font_cache_result = 0;
  font_cache_service_status_t font_cache_status;

  diagnostics_init();
  boot_progress_complete_internal("Diagnostics", 1U, 16U);
  boot_progress_complete_internal("Resource manager",
                                  (resource_manager_init() == RESOURCE_OK), 16U);
  sdcard_manager_init();
  boot_progress_complete_internal("SD manager", 1U, 17U);
  app_event_init();
  boot_progress_complete_internal("Event service", 1U, 17U);
  app_runtime_init();
  boot_progress_complete_internal("App runtime", 1U, 18U);
  nes_runtime_init();
  command_parser_init();
  boot_progress_complete_internal("Command parser", 1U, 18U);
  boot_probe_init();
  boot_progress_complete_internal("Probe service", 1U, 19U);
  norflash_set_progress_callback(boot_progress_resource_callback, NULL);
  norflash_init();
  boot_progress_set_device(BOOT_DEVICE_QSPI,
                           ((g_norflash_type != 0U) &&
                            (g_norflash_type != 0xFFFFU)) ?
                           BOOT_PROGRESS_SUCCESS : BOOT_PROGRESS_ERROR,
                           ((g_norflash_type != 0U) &&
                            (g_norflash_type != 0xFFFFU)) ? "Ready" :
                                                           "Not detected");
  boot_progress_refresh_snapshot();
  {
    sdcard_manager_status_t sd_status;

    sdcard_manager_poll(HAL_GetTick());
    sdcard_manager_get_status(&sd_status);
    g_boot_sd_ready = sd_status.mounted;
    boot_progress_set_device(BOOT_DEVICE_SD,
                             (sd_status.mounted != 0U) ? BOOT_PROGRESS_SUCCESS :
                                                        BOOT_PROGRESS_WARNING,
                             (sd_status.mounted != 0U) ? "Mounted" : "Unavailable");
    boot_progress_refresh_snapshot();
  }
  boot_report_set_sd_ready(g_boot_sd_ready);
  boot_progress_complete_internal("Base services", 1U, 20U);

  norflash_preload_wallpaper();
  norflash_get_boot_status(&g_boot_asset_status);
  boot_progress_complete_internal("Wallpaper",
                                  g_boot_asset_status.wallpaper_ready, 30U);
  if (g_boot_asset_status.wallpaper_ready != 0U) {
    boot_progress_ui_set_wallpaper(norflash_get_wallpaper_dsc());
    boot_progress_refresh_snapshot();
  }

  (void)camera_preview_init();
  boot_progress_complete_internal("Camera service", 1U, 35U);
  nand_scan_init();
  boot_progress_complete_internal("NAND service", 1U, 35U);
  audio_service_init();
  audio_recording_service_init();
  boot_progress_complete_internal("Audio service", 1U, 36U);
  adc_measurement_service_init();
  tpad_service_init();
  ir_remote_service_init();
  boot_progress_complete_internal("Input service", 1U, 36U);
  sh3001_service_init();
  imu_service_init();
  (void)imu_probe_readonly(10U);
  st480mc_service_init();
  boot_progress_complete_internal("Motion sensors", 1U, 37U);
  pressure_service_init();
  (void)pressure_probe_readonly(10U);
  ap3216c_service_init();
  (void)ap3216c_probe_readonly(10U);
  at24c02_service_init();
  (void)at24c02_probe_readonly(10U);
  system_settings_bootstrap();
  boot_progress_complete_internal("I2C devices", 1U, 38U);
  gt9xxx_init();
  boot_progress_complete_internal("Touch service", 1U, 39U);
  (void)HAL_TIM_Base_Start_IT(&htim6);
  boot_progress_complete_internal("Runtime timer", 1U, 40U);

  boot_probe_start_quiet(HAL_GetTick());
  while (boot_probe_is_complete() == 0U)
  {
    uint32_t probe_completed;
    uint32_t probe_total;
    uint8_t total_percent;
    uint8_t item_percent;

    boot_probe_poll(HAL_GetTick());
    boot_probe_get_progress(&probe_completed, &probe_total);
    item_percent = (probe_total != 0U) ?
                   (uint8_t)((probe_completed * 100U) / probe_total) : 100U;
    total_percent = (uint8_t)(40U +
                    (((uint32_t)15U * item_percent) / 100U));
    boot_progress_set_current("Peripheral probe",
                              "Probing peripherals...",
                              total_percent, item_percent);
    boot_progress_refresh_snapshot();
    if (probe_completed != 0U) {
      boot_progress_apply_probe_status(probe_completed - 1U);
    }
  }
  boot_report_capture_probe();

  norflash_preload_images();
  norflash_get_boot_status(&g_boot_asset_status);
  boot_progress_complete_internal("Image resources",
                                  (g_boot_asset_status.images_ready == 3U), 62U);

  boot_progress_set_current("Font cache", "Loading fonts from NAND...",
                            62U, 0U);
  boot_progress_refresh_snapshot();
  font_cache_service_init();
  g_boot_font_cache_percent = 0U;
  g_boot_font_cache_refresh_ms = HAL_GetTick();
  font_cache_service_set_load_progress_callback(
      boot_progress_font_cache_callback, NULL);
  font_cache_hit = (font_cache_service_try_load() != 0) ? 1U : 0U;
  font_cache_service_set_load_progress_callback(NULL, NULL);
  font_cache_service_get_status(&font_cache_status);
  if (font_cache_hit != 0U) {
    norflash_set_fonts_ready(6U);
    g_boot_chinese_ready = 1U;
    boot_progress_ui_set_font(&lv_font_myyh_20);
    boot_progress_ui_set_chinese(true);
    boot_progress_complete_internal("Cached fonts", 1U, 96U);
    boot_progress_refresh_snapshot();
    printf("[BOOT] font_cache HIT slot=%u sequence=%lu bytes=%lu corrected=%lu elapsed_ms=%lu\r\n",
           (unsigned int)font_cache_status.active_slot,
           (unsigned long)font_cache_status.sequence,
           (unsigned long)font_cache_status.package_size,
           (unsigned long)font_cache_status.corrected_pages,
           (unsigned long)font_cache_status.load_elapsed_ms);
  } else {
    printf("[BOOT] font_cache MISS result=%ld elapsed_ms=%lu\r\n",
           (long)font_cache_status.last_result,
           (unsigned long)font_cache_status.load_elapsed_ms);
    if (boot_progress_load_font("Font 12", norflash_init_font12,
                                &lv_font_myyh_12, 66U) != 0U) {
      g_boot_chinese_ready = 1U;
      boot_progress_ui_set_chinese(true);
      boot_progress_set_device(BOOT_DEVICE_QSPI,
                               ((g_norflash_type != 0U) &&
                                (g_norflash_type != 0xFFFFU)) ?
                               BOOT_PROGRESS_SUCCESS : BOOT_PROGRESS_ERROR,
                               ((g_norflash_type != 0U) &&
                                (g_norflash_type != 0xFFFFU)) ? BOOT_ZH_READY :
                                                               BOOT_ZH_NOT_DETECTED);
      boot_progress_set_device(BOOT_DEVICE_SD,
                               (g_boot_sd_ready != 0U) ? BOOT_PROGRESS_SUCCESS :
                                                        BOOT_PROGRESS_WARNING,
                               (g_boot_sd_ready != 0U) ? BOOT_ZH_MOUNTED :
                                                        BOOT_ZH_UNAVAILABLE);
      boot_progress_refresh_snapshot();
    }
    (void)boot_progress_load_font("14\xE5\x8F\xB7\xE5\xAD\x97\xE4\xBD\x93", norflash_init_font14,
                                  &lv_font_myyh_14, 72U);
    (void)boot_progress_load_font("16\xE5\x8F\xB7\xE5\xAD\x97\xE4\xBD\x93", norflash_init_font16,
                                  &lv_font_myyh_16, 78U);
    (void)boot_progress_load_font("18\xE5\x8F\xB7\xE5\xAD\x97\xE4\xBD\x93", norflash_init_font18,
                                  &lv_font_myyh_18, 84U);
    (void)boot_progress_load_font("20\xE5\x8F\xB7\xE5\xAD\x97\xE4\xBD\x93", norflash_init_font20,
                                  &lv_font_myyh_20, 90U);
    (void)boot_progress_load_font("30\xE5\x8F\xB7\xE5\xAD\x97\xE4\xBD\x93", norflash_init_font30, NULL, 96U);
    norflash_get_boot_status(&g_boot_asset_status);
    if (g_boot_asset_status.fonts_ready == 6U) {
      font_cache_result = font_cache_service_rebuild();
      font_cache_service_get_status(&font_cache_status);
      printf("[BOOT] font_cache %s result=%d slot=%u sequence=%lu bytes=%lu elapsed_ms=%lu\r\n",
             (font_cache_result == 0) ? "BUILT" : "BUILD_FAIL",
             font_cache_result,
             (unsigned int)font_cache_status.active_slot,
             (unsigned long)font_cache_status.sequence,
             (unsigned long)font_cache_status.package_size,
             (unsigned long)font_cache_status.rebuild_elapsed_ms);
      if (font_cache_result != 0) {
        printf("[BOOT] font_cache_diag stage=%u target=%lu io_result=%ld nand_status=0x%02X status_valid=%u ready=%u\r\n",
               (unsigned int)font_cache_status.rebuild.stage,
               (unsigned long)font_cache_status.rebuild.target,
               (long)font_cache_status.rebuild.detail_result,
               (unsigned int)font_cache_status.rebuild.nand_status,
               (unsigned int)font_cache_status.rebuild.nand_status_valid,
               (unsigned int)font_cache_status.rebuild.nand_ready);
      }
    }
  }
  norflash_set_progress_callback(NULL, NULL);

  boot_progress_set_current("\xE5\x90\xAF\xE5\x8A\xA8\xE5\xAE\x8C\xE6\x88\x90",
                            "\xE6\xAD\xA3\xE5\x9C\xA8\xE8\xBF\x9B\xE5\x85\xA5\xE6\xA1\x8C\xE9\x9D\xA2...",
                            100U, 100U);
  boot_progress_refresh_snapshot();
  HAL_Delay(300);
  app_lvgl_finish_init();
  boot_progress_ui_destroy();

  norflash_get_boot_status(&g_boot_asset_status);
  app_lvgl_get_boot_status(&g_boot_lvgl_status);
  boot_report_set_assets(g_boot_asset_status.wallpaper_ready,
                         g_boot_asset_status.images_ready,
                         g_boot_asset_status.fonts_ready,
                         g_boot_asset_status.images_copied,
                         (g_boot_asset_status.first_error_code != 0) ? 1U : 0U);
  boot_report_set_display(g_boot_lvgl_status.display_ready,
                          g_boot_lvgl_status.sdram_ready,
                          g_boot_lvgl_status.lvgl_ready);
  led_poc(1, 0);  /* 所有加载完成, 熄灭LED */

  boot_report_print(HAL_GetTick());
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    command_parser_poll_at(HAL_GetTick());
    system_settings_checkpoint_poll(HAL_GetTick());
    camera_preview_poll();
    sdcard_manager_poll(HAL_GetTick());
    camera_ui_bridge_poll();
    audio_poll();
    audio_record_poll();
    boot_probe_poll(HAL_GetTick());
    boot_probe_print_next();
    app_runtime_poll();
    nes_runtime_poll(HAL_GetTick());
    nand_scan_poll();
    key_poc();
    if (g_printf) { g_printf = 0; }
    Runtime_logic_optimization();
    app_lvgl_handler();   /* LVGL tick + delay */
    led_blink_proc();   /* LED heartbeat */
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_QSPI|RCC_PERIPHCLK_SDMMC
                              |RCC_PERIPHCLK_SAI1|RCC_PERIPHCLK_CKPER
                              |RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLL2.PLL2M = 2;
  PeriphClkInitStruct.PLL2.PLL2N = 12;
  PeriphClkInitStruct.PLL2.PLL2P = 5;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 1;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOMEDIUM;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.PLL3.PLL3M = 5;
  PeriphClkInitStruct.PLL3.PLL3N = 122;
  PeriphClkInitStruct.PLL3.PLL3P = 50;
  PeriphClkInitStruct.PLL3.PLL3Q = 4;
  PeriphClkInitStruct.PLL3.PLL3R = 15;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_2;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 7209;
  PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_PLL2;
  PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL2;
  PeriphClkInitStruct.CkperClockSelection = RCC_CLKPSOURCE_HSI;
  PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL3;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* ---- 定时器中断回调 ---- */
volatile uint32_t g_tim6_ticks = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1) {
        ir_remote_timeout();
    }
    if (htim->Instance == TIM6) {
        /* TIM6@2Hz: 2 ticks = 1 second precise timebase */
        g_tim6_ticks++;
        if (g_tim6_ticks >= 2) {
            g_tim6_ticks -= 2;
            rtc_tick();  /* increment software RTC globals */
        }
    }
}
/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x20000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x24000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER3;
  MPU_InitStruct.BaseAddress = 0x38000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER4;
  MPU_InitStruct.BaseAddress = 0x60000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256MB;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER5;
  MPU_InitStruct.BaseAddress = 0xC0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER6;
  MPU_InitStruct.BaseAddress = 0x80000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256MB;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER7;
  MPU_InitStruct.BaseAddress = 0x90000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_HFNMI_PRIVDEF);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    /* LED0 fast blink + LED1 steady: HAL error indication */
  __disable_irq();
    led_poc(1, 1);  /* LED1 solid ON */
  while (1)
  {
    led_poc(0, 1);  /* LED0 ON */
    for (volatile uint32_t d = 0; d < 12000000; d++) { __NOP(); }  /* ~100ms */
    led_poc(0, 0);  /* LED0 OFF */
    for (volatile uint32_t d = 0; d < 12000000; d++) { __NOP(); }  /* ~100ms */
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
