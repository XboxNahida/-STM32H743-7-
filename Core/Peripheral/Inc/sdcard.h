/**
 ********************************************************************
 * @file    sdcard.h
 * @brief   SD卡高层读写接口
 * @note    依赖 CubeMX 生成的 sdmmc.c (hsd1)
 *          移植自正点原子 sdmmc_sdcard.c, 适配 HAL 框架
 ********************************************************************
 */
#ifndef __SDCARD_H__
#define __SDCARD_H__

#include "main.h"
#include "sdmmc.h"

extern SD_HandleTypeDef hsd1;

typedef enum
{
    SDCARD_OP_NONE = 0,
    SDCARD_OP_INIT,
    SDCARD_OP_READ,
    SDCARD_OP_WRITE,
    SDCARD_OP_IOCTL,
    SDCARD_OP_MOUNT
} sdcard_op_t;

typedef struct
{
    uint32_t first_error_tick;
    uint32_t hal_error;
    uint32_t hal_state;
    uint32_t card_state;
    uint32_t sdmmc_sta;
    uint32_t sector;
    uint32_t count;
    int32_t  fatfs_result;
    uint16_t consecutive_errors;
    uint8_t  operation;
    uint8_t  valid;
} sdcard_diag_t;

uint8_t sdcard_is_ready(void);                          /* 检查SD卡是否就绪 */
uint8_t sdcard_read_disk(uint8_t *buf, uint32_t sector, uint32_t count);   /* 读扇区 */
uint8_t sdcard_write_disk(uint8_t *buf, uint32_t sector, uint32_t count);  /* 写扇区 */
uint8_t sdcard_sync(uint32_t timeout_ms);
uint8_t sdcard_get_info(uint32_t *sector_count, uint32_t *erase_block_sectors);
void    sdcard_set_ready(uint8_t ready);                    /* 设置就绪状态 */
const sdcard_diag_t *sdcard_get_diag(void);
void    sdcard_diag_clear(void);
void    sdcard_diag_record_fatfs(uint8_t operation, int32_t result);

#endif
