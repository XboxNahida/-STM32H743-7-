/**
 ********************************************************************
 * @file    norflash.c
 * @brief   NOR FLASH (W25Qxx/BY25Qxx) 驱动 + QSPI内存映射
 * @note    使用 qspi_drv.c 函数（对齐正点原子原版）
 *          流程与实验28 QSPI实验保持一致
 ********************************************************************
 */
#include "norflash.h"
#include "font_bridge.h"
#include "qspi_drv.h"
#include <string.h>
#include <stdio.h>

#include "ff.h"
#include "hander.h"

uint16_t g_norflash_type = BY25Q256;
volatile uint8_t g_norflash_addrw = 2;
uint8_t  g_norflash_mode = 0;       /* 0=SPI, 1=QPI (对齐正点原子命名) */

/* ---- 内部辅助 ---- */
static void norflash_wait_busy(void)
{
    while ((norflash_read_sr(1) & 0x01) == 0x01);
}

static void norflash_qspi_disable(void)
{
    /* QPI, 写command指令,地址为0,无数据_8位地址_无地址_4线传输指令,无空周期,0个字节数据 */
    qspi_send_cmd(FLASH_ExitQPIMode, 0, (0 << 6) | (0 << 4) | (0 << 2) | (3 << 0), 0);
    g_norflash_mode = 0;
}

static void norflash_qe_enable(void)
{
    uint8_t stareg2 = norflash_read_sr(2);
    printf("[NORFLASH] SR2 before QE: 0x%02X\r\n", stareg2);
    if ((stareg2 & 0x02) == 0) {
        norflash_write_enable();
        stareg2 |= 1 << 1;
        norflash_write_sr(2, stareg2);
        uint8_t verify = norflash_read_sr(2);
        printf("[NORFLASH] SR2 after  QE: 0x%02X %s\r\n", verify,
               (verify & 0x02) ? "OK" : "FAIL!");
    } else {
        printf("[NORFLASH] QE already set, SR2=0x%02X\r\n", stareg2);
    }

    /* SPI, 写command指令,地址为0,无数据_8位地址_无地址_单线传输指令,无空周期,0个字节数据 */
    qspi_send_cmd(FLASH_EnterQPIMode, 0, (0 << 6) | (0 << 4) | (0 << 2) | (1 << 0), 0);
    g_norflash_mode = 1;
    printf("[NORFLASH] Entered QPI mode\r\n");
}

/* ===== 公开函数 ===== */

void norflash_init(void)
{
    uint8_t temp;
    extern QSPI_HandleTypeDef hqspi;

    /* 重新初始化QSPI (对齐正点原子) */
    HAL_QSPI_Init(&hqspi);
    norflash_qspi_disable();
    norflash_qe_enable();
    g_norflash_type = norflash_read_id();

    if (g_norflash_type == W25Q256 || g_norflash_type == W25Q256JV || g_norflash_type == BY25Q256)
    {
        temp = norflash_read_sr(3);
        if ((temp & 0x01) == 0) {
            norflash_write_enable();
            /* QPI, 使能4字节地址指令,地址为0,无数据_8位地址_无地址_4线传输指令,无空周期,0个字节数据 */
            qspi_send_cmd(FLASH_Enable4ByteAddr, 0, (0 << 6) | (0 << 4) | (0 << 2) | (3 << 0), 0);
        }
        g_norflash_addrw = 3;   /* 256Mbit Flash: always use 4-byte addressing */
        norflash_write_enable();
        /* QPI, 设置读参数指令,地址为0,4线传数据_8位地址_无地址_4线传输指令,无空周期,1个字节数据 */
        qspi_send_cmd(FLASH_SetReadParam, 0, (3 << 6) | (0 << 4) | (0 << 2) | (3 << 0), 0);
        temp = 3 << 4;     /* P5=1, P4=1 → 8 dummy clocks (104MHz) */
        qspi_transmit(&temp, 1);
    }

    /* 清除 BP */
    {
        uint8_t sr1 = norflash_read_sr(1);
        printf("[NORFLASH] SR1=0x%02X (BP=%d)\r\n", sr1, (sr1 >> 2) & 0x0F);
        if (sr1 & 0x3C) {
            printf("[NORFLASH] Clearing block protect...\r\n");
            norflash_write_enable();
            norflash_write_sr(1, sr1 & ~0x3C);
            uint8_t verify = norflash_read_sr(1);
            printf("[NORFLASH] SR1 after clear: 0x%02X %s\r\n", verify,
                   (verify & 0x3C) ? "FAIL!" : "OK");
        }
    }

    printf("[NORFLASH] ID: 0x%04X\r\n", g_norflash_type);
}

uint16_t norflash_read_id(void)
{
    uint8_t buf[2];

    if (g_norflash_mode)
    {
        /* QPI, 读取ID指令,地址为0,4线传数据_8位地址_无地址_4线传输指令,无空周期,2个字节数据 */
        qspi_send_cmd(FLASH_JedecDeviceID, 0, (3 << 6) | (0 << 4) | (0 << 2) | (3 << 0), 0);
    }
    else
    {
        /* SPI, 读取ID指令,地址为0,单线传数据_8位地址_无地址_单线传输指令,无空周期,2个字节数据 */
        qspi_send_cmd(FLASH_JedecDeviceID, 0, (1 << 6) | (0 << 4) | (0 << 2) | (1 << 0), 0);
    }
    qspi_receive(buf, 2);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

void norflash_write_enable(void)
{
    if (g_norflash_mode)
    {
        /* QPI, 写使能指令,地址为0,无数据_8位地址_无地址_4线传输指令,无空周期,0个字节数据 */
        qspi_send_cmd(FLASH_WriteEnable, 0, (0 << 6) | (0 << 4) | (0 << 2) | (3 << 0), 0);
    }
    else
    {
        /* SPI, 写使能指令,地址为0,无数据_8位地址_无地址_单线传输指令,无空周期,0个字节数据 */
        qspi_send_cmd(FLASH_WriteEnable, 0, (0 << 6) | (0 << 4) | (0 << 2) | (1 << 0), 0);
    }
}

void norflash_write_disable(void)
{
    if (g_norflash_mode)
    {
        /* QPI, 失能指令,地址为0,无数据_8位地址_无地址_4线传输指令,无空周期,0个字节数据 */
        qspi_send_cmd(FLASH_WriteDisable, 0, (0 << 6) | (0 << 4) | (0 << 2) | (3 << 0), 0);
    }
    else
    {
        /* SPI, 失能指令,地址为0,无数据_8位地址_无地址_单线传输指令,无空周期,0个字节数据 */
        qspi_send_cmd(FLASH_WriteDisable, 0, (0 << 6) | (0 << 4) | (0 << 2) | (1 << 0), 0);
    }
}

uint8_t norflash_read_sr(uint8_t regno)
{
    uint8_t byte = 0, command = 0;

    switch (regno)
    {
        case 1: command = FLASH_ReadStatusReg1; break;
        case 2: command = FLASH_ReadStatusReg2; break;
        case 3: command = FLASH_ReadStatusReg3; break;
        default: return 0;
    }

    if (g_norflash_mode)
    {
        /* QPI, 写command指令,地址为0,4线传数据_8位地址_无地址_4线传输指令,无空周期,1个字节数据 */
        qspi_send_cmd(command, 0, (3 << 6) | (0 << 4) | (0 << 2) | (3 << 0), 0);
    }
    else
    {
        /* SPI, 写command指令,地址为0,单线传数据_8位地址_无地址_单线传输指令,无空周期,1个字节数据 */
        qspi_send_cmd(command, 0, (1 << 6) | (0 << 4) | (0 << 2) | (1 << 0), 0);
    }
    qspi_receive(&byte, 1);
    return byte;
}

void norflash_write_sr(uint8_t regno, uint8_t sr)
{
    uint8_t command = 0;

    switch (regno)
    {
        case 1: command = FLASH_WriteStatusReg1; break;
        case 2: command = FLASH_WriteStatusReg2; break;
        case 3: command = FLASH_WriteStatusReg3; break;
        default: return;
    }

    if (g_norflash_mode)
    {
        /* QPI, 写command指令,地址为0,4线传数据_8位地址_无地址_4线传输指令,无空周期,1个字节数据 */
        qspi_send_cmd(command, 0, (3 << 6) | (0 << 4) | (0 << 2) | (3 << 0), 0);
    }
    else
    {
        /* SPI, 写command指令,地址为0,单线传数据_8位地址_无地址_单线传输指令,无空周期,1个字节数据 */
        qspi_send_cmd(command, 0, (1 << 6) | (0 << 4) | (0 << 2) | (1 << 0), 0);
    }
    qspi_transmit(&sr, 1);
}

void norflash_erase_chip(void)
{
    norflash_write_enable();
    norflash_wait_busy();

    if (g_norflash_mode)
    {
        /* QPI, 写全片擦除指令,地址为0,无数据_8位地址_无地址_4线传输指令,无空周期,0个字节数据 */
        qspi_send_cmd(FLASH_ChipErase, 0, (0 << 6) | (0 << 4) | (0 << 2) | (3 << 0), 0);
    }
    else
    {
        /* SPI, 写全片擦除指令,地址为0,无数据_8位地址_无地址_单线传输指令,无空周期,0个字节数据 */
        qspi_send_cmd(FLASH_ChipErase, 0, (0 << 6) | (0 << 4) | (0 << 2) | (1 << 0), 0);
    }
    norflash_wait_busy();
}

void norflash_erase_sector(uint32_t saddr)
{
    uint8_t addr_size = (g_norflash_addrw == 3) ? 3 : 2;

    saddr *= 4096;
    norflash_write_enable();
    norflash_wait_busy();

    if (g_norflash_mode)
    {
        /* QPI, 写扇区擦除指令,地址为saddr,无数据_32位地址_4线传输地址_4线传输指令,无空周期,0个字节数据 */
        qspi_send_cmd(FLASH_SectorErase, saddr, (0 << 6) | (addr_size << 4) | (3 << 2) | (3 << 0), 0);
    }
    else
    {
        /* SPI, 写扇区擦除指令,地址为saddr,无数据_24/32位地址_单线传输地址_单线传输指令,无空周期,0个字节数据 */
        qspi_send_cmd(FLASH_SectorErase, saddr, (0 << 6) | (addr_size << 4) | (1 << 2) | (1 << 0), 0);
    }

    norflash_wait_busy();
}

void norflash_read(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    if (g_norflash_mode)
    {
        /* QPI, Fast Read, 4-4-4, 8 dummy */
        qspi_send_cmd(FLASH_FastReadData, addr, (3 << 6) | (3 << 4) | (3 << 2) | (3 << 0), 8);
    }
    else
    {
        /* SPI, Standard Read, 1-1-1 */
        qspi_send_cmd(FLASH_ReadData, addr, (1 << 6) | (3 << 4) | (1 << 2) | (1 << 0), 0);
    }
    qspi_receive(pbuf, datalen);
}


static void write_page(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint8_t addr_size = (g_norflash_addrw == 3) ? 3 : 2;

    norflash_write_enable();

    if (g_norflash_mode)
    {
        /* QPI, 页编程指令,地址为addr,4线传数据_32位地址_4线传地址_4线传指令,无空周期,datalen个数据 */
        qspi_send_cmd(FLASH_PageProgram, addr, (3 << 6) | (addr_size << 4) | (3 << 2) | (3 << 0), 0);
    }
    else
    {
        /* SPI, 页编程指令,地址为addr,单线传数据_24/32位地址_单线传地址_单线传指令,无空周期,datalen个数据 */
        qspi_send_cmd(FLASH_PageProgram, addr, (1 << 6) | (addr_size << 4) | (1 << 2) | (1 << 0), 0);
    }
    qspi_transmit(pbuf, datalen);

    norflash_wait_busy();
}

static void write_nocheck(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t pageremain = 256 - (addr % 256);
    if (datalen <= pageremain) pageremain = datalen;
    while (1) {
        write_page(pbuf, addr, pageremain);
        if (datalen == pageremain) break;
        pbuf   += pageremain;
        addr   += pageremain;
        datalen -= pageremain;
        pageremain = (datalen > 256) ? 256 : datalen;
    }
}

uint8_t g_norflash_buf[4096];

void norflash_write(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint32_t secpos;
    uint16_t secoff, secremain, i;
    uint8_t *norflash_buf = g_norflash_buf;

    secpos    = addr / 4096;
    secoff    = addr % 4096;
    secremain = 4096 - secoff;
    if (datalen <= secremain) secremain = datalen;

    while (1) {
        norflash_read(norflash_buf, secpos * 4096, 4096);
        for (i = 0; i < secremain; i++)
            if (norflash_buf[secoff + i] != 0xFF) break;
        if (i < secremain) {
            norflash_erase_sector(secpos);
            for (i = 0; i < secremain; i++)
                norflash_buf[i + secoff] = pbuf[i];
            write_nocheck(norflash_buf, secpos * 4096, 4096);
        } else {
            write_nocheck(pbuf, addr, secremain);
        }
        if (datalen == secremain) break;
        secpos++;
        secoff = 0;
        pbuf   += secremain;
        addr   += secremain;
        datalen -= secremain;
        secremain = (datalen > 4096) ? 4096 : datalen;
    }
}

/* ================================================================
 *  QSPI 内存映射
 * ================================================================ */

void norflash_enter_memmap(void)
{
    uint32_t tempreg;

    while (QUADSPI->SR & (1 << 5));   /* wait BUSY idle */

    /* CCR bit layout (STM32H7): FMODE[27:26] DMODE[25:24] DCYC[23:18]
       ABSIZE[17:16] ABMODE[15:14] ADSIZE[13:12] ADMODE[11:10] IMODE[9:8] */
    tempreg  = 3 << 26;               /* FMODE=3 (memory-mapped) */
    tempreg |= 3 << 24;               /* DMODE=3 (quad data) */
    tempreg |= 8 << 18;               /* DCYC=8 */
    tempreg |= ((uint32_t)(g_norflash_addrw) & 0x03) << 12;  /* ADSIZE */
    tempreg |= 3 << 10;               /* ADMODE=3 (quad address) */
    tempreg |= 3 << 8;                /* IMODE=3 (quad instruction) */
    /* ABMODE[15:14] stays 0 (no alternate byte) */
    tempreg |= FLASH_FastReadQuad;    /* 0xEB */
    QUADSPI->CCR = tempreg;
}

void norflash_exit_memmap(void)
{
    QUADSPI->CCR &= ~(3 << 26);
}


/* ================================================================
 *  SD→QSPI fallback: copy from SD card when QSPI is empty
 * ================================================================ */

/* Check if QSPI area is erased (first check_len bytes all 0xFF) */
static uint8_t norflash_is_erased(uint32_t offset, uint32_t check_len)
{
    uint8_t buf[256];
    uint32_t i;
    uint32_t pos;

    for (pos = 0; pos < check_len; pos += 256) {
        uint32_t chunk;
        chunk = (check_len - pos > 256) ? 256 : (check_len - pos);
        norflash_read(buf, offset + pos, (uint16_t)chunk);
        for (i = 0; i < chunk; i++) {
            if (buf[i] != 0xFF) return 0;
        }
    }
    return 1;
}

/* Copy a file from SD card to QSPI flash */
static uint8_t sd_to_qspi_one(const char *sd_path, uint32_t qspi_offset,
                               uint32_t *out_size)
{
    FIL fil;
    FRESULT res;
    UINT br;
    uint32_t total;
    uint32_t sectors;
    uint32_t s;
    uint8_t buf[256];

    res = f_open(&fil, sd_path, FA_READ);
    if (res != FR_OK) {
        printf("[SD→QSPI] Cannot open %s (err=%d)\r\n", sd_path, res);
        return 1;
    }

    total = f_size(&fil);
    if (out_size) *out_size = total;
    sectors = (total + 4095) / 4096;

    printf("[SD→QSPI] Copying %s (%lu bytes) → QSPI@0x%06lX\r\n",
           sd_path, total, qspi_offset);

    /* Erase sectors */
    {
        uint32_t start_sector;
        start_sector = qspi_offset / 4096;
        for (s = 0; s < sectors; s++) {
            norflash_erase_sector(start_sector + s);
            led_poc(1, 2);
            if ((s & 0x0F) == 0) printf(".");
        }
    }
    printf("\r\n");

    /* Write data in 256-byte chunks */
    printf("[SD→QSPI] Writing...\r\n");
    total = 0;
    while (1) {
        res = f_read(&fil, buf, 256, &br);
        if (res != FR_OK || br == 0) break;
        norflash_write(buf, qspi_offset + total, (uint16_t)br);
        led_poc(1, 2);
        total += br;
        if ((total & 0x3FFF) == 0) printf(".");
    }
    f_close(&fil);
    printf("\r\n");

    /* Verify first 256 bytes */
    {
        uint8_t vbuf[256];
        uint32_t i;
        norflash_read(vbuf, qspi_offset, 256);
        f_open(&fil, sd_path, FA_READ);
        f_read(&fil, buf, 256, &br);
        f_close(&fil);
        for (i = 0; i < 256; i++) {
            if (vbuf[i] != buf[i]) {
                printf("[SD→QSPI] Verify FAIL at offset %lu\r\n", i);
                return 1;
            }
        }
    }
    printf("[SD→QSPI] %s OK!\r\n", sd_path);
    return 0;
}

/* ================================================================
 *  壁纸迁移
 * ================================================================ */


#define WALLPAPER_SDRAM_ADDR  0xC0200000UL  /* 2MB offset in SDRAM (MPU Region5, 32MB, Cacheable) */

/* 从 QSPI 预加载壁纸到 SDRAM (间接模式, 已验证可靠)
 * 若 QSPI 为空则自动从 SD 卡拷贝 */
void norflash_preload_wallpaper(void)
{
    uint32_t off;
    uint8_t *dst;
    uint32_t chunk;

    /* Check if QSPI has wallpaper data */
    if (norflash_is_erased(QSPI_WALLPAPER_OFFSET, 256)) {
        printf("[WALLPAPER] QSPI empty, copying from SD...\r\n");
        if (sd_to_qspi_one("0:/SYSTEM/IMG/_PPC_lock_RGB565_1024x600.bin",
                           QSPI_WALLPAPER_OFFSET, NULL) != 0) {
            printf("[WALLPAPER] SD fallback failed, QSPI may be empty\r\n");
        }
    }

    dst = (uint8_t *)WALLPAPER_SDRAM_ADDR;
    printf("[WALLPAPER] Loading from QSPI to SDRAM...\r\n");
    for (off = 0; off < QSPI_WALLPAPER_SIZE; off += 256) {
        chunk = (QSPI_WALLPAPER_SIZE - off > 256) ? 256 : (QSPI_WALLPAPER_SIZE - off);
        norflash_read(dst + off, QSPI_WALLPAPER_OFFSET + off, chunk);
        led_poc(1, 2);
        if ((off & 0x3FFF) == 0) printf(".");
    }
    printf("\r\n[WALLPAPER] SDRAM ready\r\n");
}





/* ================================================================
 *  lv_image_dsc_t 工厂
 * ================================================================ */

lv_image_dsc_t *norflash_get_wallpaper_dsc(void)
{
    static lv_image_dsc_t wallpaper_dsc = {
        .header.magic = LV_IMAGE_HEADER_MAGIC,
        .header.cf    = LV_COLOR_FORMAT_RGB565,
        .header.w     = 1024,
        .header.h     = 600,
        .header.stride = 1024 * 2,
        .data_size    = QSPI_WALLPAPER_SIZE,
        .data         = (const uint8_t *)WALLPAPER_SDRAM_ADDR,
    };
    return &wallpaper_dsc;
}

/* ================================================================
 *  Image 1/2/3 SDRAM addrs (after wallpaper 0xC0200000)
 * ================================================================ */
#define IMG1_SDRAM_ADDR  0xC0330000UL
#define IMG2_SDRAM_ADDR  0xC0350000UL
#define IMG3_SDRAM_ADDR  0xC0410000UL


static void norflash_preload_image(uint32_t qspi_off, uint32_t size,
                                    uint8_t *dst, const char *name)
{
    printf("[IMG] Loading %s QSPI->SDRAM...\r\n", name);
    for (uint32_t off = 0; off < size; off += 256) {
        uint32_t chunk = (size - off > 256) ? 256 : (size - off);
        norflash_read(dst + off, qspi_off + off, chunk);
        led_poc(1, 2);
        if ((off & 0xFFFF) == 0) printf(".");
    }
    printf("\r\n");
}


void norflash_preload_images(void)
{
    /* Check and fallback to SD if QSPI empty */
    if (norflash_is_erased(QSPI_IMG1_OFFSET, 256)) {
        printf("[IMG] img1 QSPI empty, copying from SD...\r\n");
        sd_to_qspi_one("0:/SYSTEM/IMG/_1_RGB565_285x107.bin",
                        QSPI_IMG1_OFFSET, NULL);
    }
    if (norflash_is_erased(QSPI_IMG2_OFFSET, 256)) {
        printf("[IMG] img2 QSPI empty, copying from SD...\r\n");
        sd_to_qspi_one("0:/SYSTEM/IMG/_2_RGB565_700x370.bin",
                        QSPI_IMG2_OFFSET, NULL);
    }
    if (norflash_is_erased(QSPI_IMG3_OFFSET, 256)) {
        printf("[IMG] img3 QSPI empty, copying from SD...\r\n");
        sd_to_qspi_one("0:/SYSTEM/IMG/_3_RGB565_600x220.bin",
                        QSPI_IMG3_OFFSET, NULL);
    }

    norflash_preload_image(QSPI_IMG1_OFFSET, QSPI_IMG1_SIZE,
                           (uint8_t *)IMG1_SDRAM_ADDR, "img1");
    norflash_preload_image(QSPI_IMG2_OFFSET, QSPI_IMG2_SIZE,
                           (uint8_t *)IMG2_SDRAM_ADDR, "img2");
    norflash_preload_image(QSPI_IMG3_OFFSET, QSPI_IMG3_SIZE,
                           (uint8_t *)IMG3_SDRAM_ADDR, "img3");
    /* D-Cache clean */
    SCB_CleanDCache_by_Addr((uint32_t *)IMG1_SDRAM_ADDR, QSPI_IMG1_SIZE);
    SCB_CleanDCache_by_Addr((uint32_t *)IMG2_SDRAM_ADDR, QSPI_IMG2_SIZE);
    SCB_CleanDCache_by_Addr((uint32_t *)IMG3_SDRAM_ADDR, QSPI_IMG3_SIZE);
}

lv_image_dsc_t *norflash_get_img1_dsc(void)
{
    static lv_image_dsc_t dsc = {
        .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                   .w = 285, .h = 107, .stride = 285 * 2},
        .data_size = QSPI_IMG1_SIZE,
        .data = (const uint8_t *)IMG1_SDRAM_ADDR,
    };
    return &dsc;
}

lv_image_dsc_t *norflash_get_img2_dsc(void)
{
    static lv_image_dsc_t dsc = {
        .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                   .w = 700, .h = 370, .stride = 700 * 2},
        .data_size = QSPI_IMG2_SIZE,
        .data = (const uint8_t *)IMG2_SDRAM_ADDR,
    };
    return &dsc;
}

lv_image_dsc_t *norflash_get_img3_dsc(void)
{
    static lv_image_dsc_t dsc = {
        .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_RGB565,
                   .w = 600, .h = 220, .stride = 600 * 2},
        .data_size = QSPI_IMG3_SIZE,
        .data = (const uint8_t *)IMG3_SDRAM_ADDR,
    };
    return &dsc;
}

/* ===== QSPI Font Init ===== */


/* ---- Font initialization (via BIN font bridge) ---- */

void norflash_init_font12(void) { font_bridge_init(); }
void norflash_init_font14(void) { }
void norflash_init_font16(void) { }
void norflash_init_font18(void) { }
void norflash_init_font20(void) { }
void norflash_init_font30(void) { }