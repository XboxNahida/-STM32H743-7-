/**
 ********************************************************************
 * @file    norflash.h
 * @brief   NOR FLASH (W25Qxx/BY25Qxx) 驱动 + QSPI内存映射
 * @note    依赖 qspi_drv, 适配 HAL/CubeMX 框架
 *          移植自正点原子 norflash.c
 ********************************************************************
 */
#ifndef __NORFLASH_H__
#define __NORFLASH_H__

#include "main.h"
#include "lvgl.h"

/* ===== QSPI 内存映射基地址 ===== */
#define QSPI_BASE_ADDR          0x90000000UL
#define QSPI_WALLPAPER_OFFSET   0x00000000UL
#define QSPI_WALLPAPER_SIZE     (1024UL * 600UL * 2UL)  /* RGB565 */

/* ===== QSPI image offsets (after wallpaper) ===== */
#define QSPI_IMG1_OFFSET         0x00140000UL
#define QSPI_IMG2_OFFSET         0x00160000UL
#define QSPI_IMG3_OFFSET         0x00220000UL
#define QSPI_IMG1_SIZE           (285UL * 107UL * 2UL)   /* RGB565 */
#define QSPI_IMG2_SIZE           (700UL * 370UL * 2UL)
#define QSPI_IMG3_SIZE           (600UL * 220UL * 2UL)

/* ===== QSPI font offsets ===== */
#define QSPI_FONT12_OFFSET       0x00300000UL
#define QSPI_FONT12_MAX_SIZE     (2048UL * 1024UL)
#define QSPI_FONT14_OFFSET       0x00500000UL
#define QSPI_FONT14_MAX_SIZE     (2048UL * 1024UL)
#define QSPI_FONT16_OFFSET       0x00700000UL
#define QSPI_FONT16_MAX_SIZE     (2048UL * 1024UL)
#define QSPI_FONT18_OFFSET       0x00900000UL
#define QSPI_FONT18_MAX_SIZE     (2048UL * 1024UL)
#define QSPI_FONT20_OFFSET       0x00B00000UL
#define QSPI_FONT20_MAX_SIZE     (2048UL * 1024UL)
#define QSPI_FONT30_OFFSET       0x00D00000UL
#define QSPI_FONT30_MAX_SIZE     (4096UL * 1024UL)

/* ===== Flash 芯片 ID ===== */
#define W25Q80      0xEF13
#define W25Q16      0xEF14
#define W25Q32      0xEF15
#define W25Q64      0xEF16
#define W25Q128     0xEF17
#define W25Q256     0xEF18
#define W25Q256JV   0xEF40          /* W25Q256JV/JV 实际JEDEC ID首2字节 */
#define BY25Q64     0x6816
#define BY25Q128    0x6817
#define BY25Q256    0x6818
#define NM25Q64     0x5216
#define NM25Q128    0x5217

extern uint16_t g_norflash_type;
extern uint8_t  g_norflash_mode;

/* ===== Flash 指令表 ===== */
#define FLASH_WriteEnable           0x06
#define FLASH_WriteDisable          0x04
#define FLASH_ReadStatusReg1        0x05
#define FLASH_ReadStatusReg2        0x35
#define FLASH_ReadStatusReg3        0x15
#define FLASH_WriteStatusReg1       0x01
#define FLASH_WriteStatusReg2       0x31
#define FLASH_WriteStatusReg3       0x11
#define FLASH_ReadData              0x03
#define FLASH_FastReadData          0x0B
#define FLASH_FastReadDual          0x3B
#define FLASH_FastReadQuad          0xEB
#define FLASH_PageProgram           0x02
#define FLASH_PageProgramQuad       0x32
#define FLASH_BlockErase            0xD8
#define FLASH_SectorErase           0x20
#define FLASH_ChipErase             0xC7
#define FLASH_PowerDown             0xB9
#define FLASH_ReleasePowerDown      0xAB
#define FLASH_DeviceID              0xAB
#define FLASH_ManufactDeviceID      0x90
#define FLASH_JedecDeviceID         0x9F
#define FLASH_Enable4ByteAddr       0xB7
#define FLASH_Exit4ByteAddr         0xE9
#define FLASH_SetReadParam          0xC0
#define FLASH_EnterQPIMode          0x38
#define FLASH_ExitQPIMode           0xFF

/* ===== 用户函数 ===== */
void     norflash_init(void);
uint16_t norflash_read_id(void);
void     norflash_write_enable(void);
void     norflash_write_disable(void);
uint8_t  norflash_read_sr(uint8_t regno);
void     norflash_write_sr(uint8_t regno, uint8_t sr);
void     norflash_erase_chip(void);
void     norflash_erase_sector(uint32_t saddr);
void     norflash_read(uint8_t *pbuf, uint32_t addr, uint16_t datalen);
void     norflash_write(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/* ===== QSPI 内存映射（用于大图片直接访问） ===== */
void     norflash_enter_memmap(void);
void     norflash_preload_wallpaper(void);  /* 预加载壁纸到 SDRAM */
void     norflash_exit_memmap(void);


/* 壁纸 lv_image_dsc_t 工厂 (数据指向QSPI内存映射区) */
lv_image_dsc_t *norflash_get_wallpaper_dsc(void);

/* Image 1/2/3 migration: SD card -> QSPI */
lv_image_dsc_t *norflash_get_img1_dsc(void);
lv_image_dsc_t *norflash_get_img2_dsc(void);
lv_image_dsc_t *norflash_get_img3_dsc(void);
void     norflash_preload_images(void);

/* Font from QSPI */
void     norflash_init_font12(void);
void     norflash_init_font14(void);
void     norflash_init_font16(void);
void     norflash_init_font18(void);
void     norflash_init_font20(void);
void     norflash_init_font30(void);

#endif
