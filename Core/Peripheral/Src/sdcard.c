#include <stdio.h>
/**
 ********************************************************************
 * @file    sdcard.c
 * @brief   SD卡高层读写接口
 * @note    使用 HAL_SD_ReadBlocks / HAL_SD_WriteBlocks
 *          移植自正点原子 sdmmc_sdcard.c
 ********************************************************************
 */
#include "sdcard.h"
#include <string.h>

#define SDCARD_SECTOR_SIZE               512U
#define SDCARD_READ_BOUNCE_SECTORS       8U
#define SDCARD_SDRAM_START               0xC0000000UL
#define SDCARD_SDRAM_END                 0xC2000000UL

static uint8_t g_sdcard_ready = 0;      /* SD卡是否初始化成功 */
static sdcard_diag_t g_sdcard_diag;
static uint8_t g_sdcard_read_bounce[SDCARD_SECTOR_SIZE * SDCARD_READ_BOUNCE_SECTORS]
    __attribute__((aligned(32)));

static uint8_t sdcard_buffer_is_in_sdram(const uint8_t *buf, uint32_t count)
{
    uintptr_t start;
    uintptr_t size;
    uintptr_t end;

    if ((buf == NULL) || (count > (UINT32_MAX / SDCARD_SECTOR_SIZE)))
    {
        return 0U;
    }

    start = (uintptr_t)buf;
    size = (uintptr_t)count * SDCARD_SECTOR_SIZE;
    end = start + size;

    return ((start >= SDCARD_SDRAM_START) &&
            (end >= start) &&
            (end <= SDCARD_SDRAM_END)) ? 1U : 0U;
}

static const char *sdcard_op_name(sdcard_op_t operation)
{
    switch (operation)
    {
        case SDCARD_OP_INIT:  return "INIT";
        case SDCARD_OP_READ:  return "READ";
        case SDCARD_OP_WRITE: return "WRITE";
        case SDCARD_OP_IOCTL: return "IOCTL";
        case SDCARD_OP_MOUNT: return "MOUNT";
        default:              return "NONE";
    }
}

static void sdcard_print_first_error(void)
{
    printf("[SD-FIRST] op=%s tick=%lu sector=%lu count=%lu hal=0x%08lX state=%lu card=%lu sta=0x%08lX fatfs=%ld\r\n",
           sdcard_op_name((sdcard_op_t)g_sdcard_diag.operation),
           (unsigned long)g_sdcard_diag.first_error_tick,
           (unsigned long)g_sdcard_diag.sector,
           (unsigned long)g_sdcard_diag.count,
           (unsigned long)g_sdcard_diag.hal_error,
           (unsigned long)g_sdcard_diag.hal_state,
           (unsigned long)g_sdcard_diag.card_state,
           (unsigned long)g_sdcard_diag.sdmmc_sta,
           (long)g_sdcard_diag.fatfs_result);
}

static void sdcard_capture_first_error(sdcard_op_t operation,
                                       uint32_t sector,
                                       uint32_t count)
{
    uint32_t hal_error = hsd1.ErrorCode;
    uint32_t hal_state = (uint32_t)hsd1.State;
    uint32_t sdmmc_sta = 0;
    uint32_t card_state = (uint32_t)HAL_SD_CARD_ERROR;

    if (hsd1.Instance != NULL)
    {
        sdmmc_sta = hsd1.Instance->STA;
        if (operation != SDCARD_OP_INIT)
        {
            card_state = (uint32_t)HAL_SD_GetCardState(&hsd1);
        }
    }

    if (g_sdcard_diag.consecutive_errors < UINT16_MAX)
    {
        g_sdcard_diag.consecutive_errors++;
    }

    if (g_sdcard_diag.valid != 0U)
    {
        return;
    }

    g_sdcard_diag.first_error_tick = HAL_GetTick();
    g_sdcard_diag.hal_error = hal_error;
    g_sdcard_diag.hal_state = hal_state;
    g_sdcard_diag.card_state = card_state;
    g_sdcard_diag.sdmmc_sta = sdmmc_sta;
    g_sdcard_diag.sector = sector;
    g_sdcard_diag.count = count;
    g_sdcard_diag.fatfs_result = -1;
    g_sdcard_diag.operation = (uint8_t)operation;
    g_sdcard_diag.valid = 1U;
    if (operation != SDCARD_OP_INIT)
    {
        sdcard_print_first_error();
    }
}

static uint8_t sdcard_wait_transfer(sdcard_op_t operation,
                                    uint32_t sector,
                                    uint32_t count,
                                    uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();
    HAL_SD_CardStateTypeDef card_state;

    do
    {
        card_state = HAL_SD_GetCardState(&hsd1);
        if (card_state == HAL_SD_CARD_TRANSFER)
        {
            return 0;
        }

        if ((card_state == HAL_SD_CARD_ERROR) ||
            (card_state == HAL_SD_CARD_DISCONNECTED))
        {
            break;
        }
    } while ((HAL_GetTick() - start_tick) < timeout_ms);

    sdcard_capture_first_error(operation, sector, count);
    return 1;
}

/**
 * @brief  检查SD卡是否就绪
 * @retval 0=未就绪, 1=已就绪
 */
uint8_t sdcard_is_ready(void)
{
    return g_sdcard_ready;
}

uint8_t sdcard_sync(uint32_t timeout_ms)
{
    if ((g_sdcard_ready == 0U) || (timeout_ms == 0U))
    {
        return 1;
    }

    return sdcard_wait_transfer(SDCARD_OP_IOCTL, 0, 0, timeout_ms);
}

uint8_t sdcard_get_info(uint32_t *sector_count, uint32_t *erase_block_sectors)
{
    HAL_SD_CardInfoTypeDef card_info = {0};

    if ((g_sdcard_ready == 0U) ||
        (sector_count == NULL) ||
        (erase_block_sectors == NULL))
    {
        return 1;
    }

    if (HAL_SD_GetCardInfo(&hsd1, &card_info) != HAL_OK)
    {
        sdcard_capture_first_error(SDCARD_OP_IOCTL, 0, 0);
        return 1;
    }

    if ((card_info.LogBlockSize != 512U) || (card_info.LogBlockNbr == 0U))
    {
        return 1;
    }

    *sector_count = card_info.LogBlockNbr;
    *erase_block_sectors = 1U;
    return 0;
}

const sdcard_diag_t *sdcard_get_diag(void)
{
    return &g_sdcard_diag;
}

void sdcard_diag_clear(void)
{
    memset(&g_sdcard_diag, 0, sizeof(g_sdcard_diag));
    g_sdcard_diag.fatfs_result = -1;
}

void sdcard_diag_record_fatfs(uint8_t operation, int32_t result)
{
    if ((g_sdcard_diag.valid != 0U) &&
        (g_sdcard_diag.fatfs_result == -1) &&
        ((g_sdcard_diag.operation == operation) ||
         (g_sdcard_diag.operation == (uint8_t)SDCARD_OP_NONE)))
    {
        g_sdcard_diag.fatfs_result = result;
        printf("[SD-FATFS] op=%s result=%ld\r\n",
               sdcard_op_name((sdcard_op_t)operation),
               (long)result);
    }
}

/**
 * @brief  读SD卡(扇区为单位, 供FATFS等文件系统调用)
 * @param  buf    : 数据缓冲区
 * @param  sector : 起始扇区号
 * @param  count  : 扇区数量
 * @retval 0=成功, 1=失败
 */
/**
 * @brief  读SD卡(扇区为单位, 供FATFS等文件系统调用)
 * @param  buf    : 数据缓冲区
 * @param  sector : 起始扇区号
 * @param  count  : 扇区数量
 * @retval 0=成功, 1=失败
 */
uint8_t sdcard_read_disk(uint8_t *buf, uint32_t sector, uint32_t count)
{
    HAL_StatusTypeDef ret;
    uint8_t *read_buf = buf;
    uint32_t read_sector = sector;
    uint32_t remaining = count;
    uint32_t chunk_count;
    uint32_t chunk_size;
    uint8_t use_bounce;
    int retry;

    if ((g_sdcard_ready == 0U) || (buf == NULL) || (count == 0U) ||
        (count > (UINT32_MAX / SDCARD_SECTOR_SIZE))) return 1;

    use_bounce = sdcard_buffer_is_in_sdram(buf, count);

    while (remaining > 0U)
    {
        chunk_count = remaining;
        if ((use_bounce != 0U) && (chunk_count > SDCARD_READ_BOUNCE_SECTORS))
        {
            chunk_count = SDCARD_READ_BOUNCE_SECTORS;
        }
        chunk_size = chunk_count * SDCARD_SECTOR_SIZE;

        for (retry = 0; retry < 8; retry++)
        {
            __DSB();  /* flush pending bus transactions before SDIO access */
            ret = HAL_SD_ReadBlocks(&hsd1,
                                    (use_bounce != 0U) ? g_sdcard_read_bounce : read_buf,
                                    read_sector, chunk_count, 5000);
            if (ret == HAL_OK)
            {
                break;
            }
            sdcard_capture_first_error(SDCARD_OP_READ, read_sector, chunk_count);
            HAL_SD_Abort(&hsd1);
            HAL_Delay(50 * (retry + 1));  /* 50..400ms increasing delay */
        }

        if (ret != HAL_OK)
        {
            printf("[SD] Read error at sector %lu (ret=%d)\r\n", read_sector, ret);
            return 1;
        }

        if (use_bounce != 0U)
        {
            memcpy(read_buf, g_sdcard_read_bounce, chunk_size);
        }

        read_buf += chunk_size;
        read_sector += chunk_count;
        remaining -= chunk_count;
    }

    g_sdcard_diag.consecutive_errors = 0;
    return 0;
}


/**
 * @brief  写SD卡(扇区为单位, 供FATFS等文件系统调用)
 * @param  buf    : 数据缓冲区
 * @param  sector : 起始扇区号
 * @param  count  : 扇区数量
 * @retval 0=成功, 1=失败
 */
uint8_t sdcard_write_disk(uint8_t *buf, uint32_t sector, uint32_t count)
{
    HAL_StatusTypeDef ret;

    if ((g_sdcard_ready == 0U) || (buf == NULL) || (count == 0U)) return 1;

    ret = HAL_SD_WriteBlocks(&hsd1, buf, sector, count, 5000);
    if (ret != HAL_OK)
    {
        sdcard_capture_first_error(SDCARD_OP_WRITE, sector, count);
        printf("[SD] Write error at sector %lu\r\n", sector);
        return 1;
    }

    if (sdcard_wait_transfer(SDCARD_OP_WRITE, sector, count, 5000U) != 0U)
    {
        HAL_SD_Abort(&hsd1);
        printf("[SD] Write sync timeout at sector %lu\r\n", sector);
        return 1;
    }

    g_sdcard_diag.consecutive_errors = 0;
    return 0;
}

/**
 * @brief  SD卡就绪标记设置 (由 sdmmc.c 中 USER CODE SDMMC1_Init_2 调用)
 * @param  ready: 0=失败, 1=成功
 * @note   在 MX_SDMMC1_SD_Init() 的 Init_2 区域调用本函数
 */
void sdcard_set_ready(uint8_t ready)
{
    g_sdcard_ready = ready;
    if (ready == 0U)
    {
        sdcard_capture_first_error(SDCARD_OP_INIT, 0, 0);
    }
    else
    {
        g_sdcard_diag.consecutive_errors = 0;
    }
}
