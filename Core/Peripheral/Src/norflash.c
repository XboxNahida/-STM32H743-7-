/**
 ********************************************************************
 * @file    norflash.c
 * @brief   NOR FLASH (W25Qxx/BY25Qxx) 驱动 + QSPI内存映射
 * @note    使用 qspi_drv.c 函数（对齐正点原子原版）
 *          流程与实验28 QSPI实验保持一致
 ********************************************************************
 */
#include "norflash.h"
#include "qspi_drv.h"
#include "font_bridge.h"
#include <string.h>
#include <stdio.h>

#include "ff.h"
#include "hander.h"

uint16_t g_norflash_type = 0U;
volatile uint8_t g_norflash_addrw = 2;
uint8_t  g_norflash_mode = 0;       /* 0=SPI, 1=QPI (对齐正点原子命名) */
static norflash_boot_status_t g_boot_status;
static norflash_progress_callback_t g_progress_callback;
static void *g_progress_context;

#define NORFLASH_BOOT_ITEM_WALLPAPER  1U
#define NORFLASH_BOOT_ITEM_IMG1       2U
#define NORFLASH_BOOT_ITEM_IMG2       3U
#define NORFLASH_BOOT_ITEM_IMG3       4U
#define NORFLASH_BOOT_ITEM_FONT12    12U
#define NORFLASH_BOOT_ITEM_FONT14    14U
#define NORFLASH_BOOT_ITEM_FONT16    16U
#define NORFLASH_BOOT_ITEM_FONT18    18U
#define NORFLASH_BOOT_ITEM_FONT20    20U
#define NORFLASH_BOOT_ITEM_FONT30    30U

#define NORFLASH_BOOT_ERROR_COPY      1
#define NORFLASH_BOOT_ERROR_PARSE     2

/* ---- 内部辅助 ---- */
static void norflash_boot_capture_error(uint8_t item, int32_t code)
{
    if (g_boot_status.first_error_code == 0)
    {
        g_boot_status.first_error_item = item;
        g_boot_status.first_error_code = code;
    }
}

static void norflash_report_progress(const char *item,
                                     norflash_progress_action_t action,
                                     uint32_t processed,
                                     uint32_t total)
{
    norflash_progress_t progress;

    if (g_progress_callback == NULL)
    {
        return;
    }

    progress.item = item;
    progress.action = action;
    progress.processed = processed;
    progress.total = total;
    progress.percent = 0U;
    if (action == NORFLASH_PROGRESS_DONE)
    {
        progress.percent = 100U;
    }
    else if (total != 0U)
    {
        uint64_t percent = ((uint64_t)processed * 100U) / total;

        progress.percent = (percent > 100U) ? 100U : (uint8_t)percent;
    }
    g_progress_callback(&progress, g_progress_context);
}

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
    if ((stareg2 & 0x02) == 0) {
        norflash_write_enable();
        stareg2 |= 1 << 1;
        norflash_write_sr(2, stareg2);
        uint8_t verify = norflash_read_sr(2);
        if ((verify & 0x02U) == 0U) {
            printf("[BOOT] storage FAIL item=qspi action=enable_qe code=%u\r\n",
                   (unsigned int)verify);
        }
    }

    /* SPI, 写command指令,地址为0,无数据_8位地址_无地址_单线传输指令,无空周期,0个字节数据 */
    qspi_send_cmd(FLASH_EnterQPIMode, 0, (0 << 6) | (0 << 4) | (0 << 2) | (1 << 0), 0);
    g_norflash_mode = 1;
}

/* ===== 公开函数 ===== */

void norflash_boot_status_reset(void)
{
    memset(&g_boot_status, 0, sizeof(g_boot_status));
}

void norflash_get_boot_status(norflash_boot_status_t *status)
{
    if (status != NULL)
    {
        *status = g_boot_status;
    }
}

void norflash_set_fonts_ready(uint8_t count)
{
    g_boot_status.fonts_ready = (count <= 6U) ? count : 6U;
}

void norflash_set_progress_callback(norflash_progress_callback_t callback,
                                    void *context)
{
    g_progress_callback = callback;
    g_progress_context = context;
}

void norflash_init(void)
{
    uint8_t temp;
    extern QSPI_HandleTypeDef hqspi;

    norflash_boot_status_reset();

    /* 重新初始化QSPI (对齐正点原子) */
    HAL_QSPI_Init(&hqspi);
    norflash_qspi_disable();
    norflash_qe_enable();
    g_norflash_type = norflash_read_id();

    if (g_norflash_type == W25Q256 || g_norflash_type == W25Q256JV ||
        g_norflash_type == W25Q256JW || g_norflash_type == BY25Q256)
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
        if (sr1 & 0x3C) {
            norflash_write_enable();
            norflash_write_sr(1, sr1 & ~0x3C);
            uint8_t verify = norflash_read_sr(1);
            if ((verify & 0x3CU) != 0U) {
                printf("[BOOT] storage FAIL item=qspi action=clear_protect code=%u\r\n",
                       (unsigned int)verify);
            }
        }
    }
}

uint8_t norflash_probe_readonly(uint16_t *id, uint32_t timeout_ms)
{
    uint8_t buf[2];
    uint8_t mode;
    uint32_t start_ms;
    uint32_t elapsed_ms;

    if ((id == NULL) || (timeout_ms == 0U))
    {
        return 1U;
    }
    *id = 0U;

    if ((g_norflash_type != 0U) && (g_norflash_type != 0xFFFFU))
    {
        *id = g_norflash_type;
        return 0U;
    }
    if (hqspi.State != HAL_QSPI_STATE_READY)
    {
        return 1U;
    }

    mode = (g_norflash_mode != 0U) ?
           (uint8_t)((3U << 6) | (3U << 0)) :
           (uint8_t)((1U << 6) | (1U << 0));
    start_ms = HAL_GetTick();
    if (qspi_send_cmd_timeout(FLASH_JedecDeviceID, 0U, mode, 0U,
                              timeout_ms) != 0U)
    {
        return 1U;
    }

    elapsed_ms = HAL_GetTick() - start_ms;
    if (elapsed_ms >= timeout_ms)
    {
        return 1U;
    }
    if (qspi_receive_timeout(buf, 2U, timeout_ms - elapsed_ms) != 0U)
    {
        return 1U;
    }

    *id = ((uint16_t)buf[0] << 8) | buf[1];
    return 0U;
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

static void norflash_abort_memmap(void)
{
    volatile uint32_t timeout;

    if (QUADSPI->SR & (1 << 5)) {
        QUADSPI->CR |= (1 << 1);           /* ABORT */
        timeout = 1000000;
        while ((QUADSPI->SR & (1 << 5)) && --timeout);
    }
    QUADSPI->CCR &= ~(3 << 26);            /* FMODE=0 */
}

void norflash_enter_memmap(void)
{
    uint32_t tempreg;

    /* Stop any prefetch left by a prior memory-mapped read. */
    norflash_abort_memmap();

    /* CCR bit layout (STM32H7): FMODE[27:26] DMODE[25:24] DCYC[23:18]
       ABSIZE[17:16] ABMODE[15:14] ADSIZE[13:12] ADMODE[11:10] IMODE[9:8] */
    tempreg  = 3 << 26;                    /* FMODE=3 (memory-mapped) */
    tempreg |= 3 << 24;                    /* DMODE=3 (quad data) */
    tempreg |= 8 << 18;                    /* DCYC=8 */
    tempreg |= ((uint32_t)(g_norflash_addrw) & 0x03) << 12;  /* ADSIZE */
    tempreg |= 3 << 10;                    /* ADMODE=3 (quad address) */
    tempreg |= 3 << 8;                     /* IMODE=3 (quad instruction) */
    /* ABMODE[15:14] stays 0 (no alternate byte) */
    tempreg |= FLASH_FastReadQuad;         /* 0xEB */
    QUADSPI->CCR = tempreg;
}

void norflash_exit_memmap(void)
{
    norflash_abort_memmap();
}

typedef struct {
    const char *item;
    uint32_t offset;
    uint32_t size;
} norflash_asset_range_t;

static uint8_t norflash_verify_erased_range(uint32_t offset, uint32_t size)
{
    uint8_t buffer[256];
    uint32_t position;
    uint32_t index;
    uint32_t chunk;

    for (position = 0U; position < size; position += chunk) {
        chunk = (size - position > sizeof(buffer)) ?
                (uint32_t)sizeof(buffer) : (size - position);
        norflash_read(buffer, offset + position, (uint16_t)chunk);
        for (index = 0U; index < chunk; index++) {
            if (buffer[index] != 0xFFU) {
                printf("ERROR qspi action=erase target=assets stage=verify addr=0x%08lX actual=%02X\r\n",
                       (unsigned long)(offset + position + index),
                       (unsigned int)buffer[index]);
                return 1U;
            }
        }
    }
    return 0U;
}

static uint8_t norflash_erase_asset_range(const norflash_asset_range_t *range)
{
    uint32_t first_sector;
    uint32_t sector_count;
    uint32_t index;

    first_sector = range->offset / 4096U;
    sector_count = (range->size + 4095U) / 4096U;
    printf("qspi erase progress item=%s phase=erase sectors=%lu\r\n",
           range->item, (unsigned long)sector_count);
    for (index = 0U; index < sector_count; index++) {
        norflash_erase_sector(first_sector + index);
    }
    printf("qspi erase progress item=%s phase=verify bytes=%lu\r\n",
           range->item, (unsigned long)range->size);
    return norflash_verify_erased_range(range->offset, range->size);
}

uint8_t norflash_erase_assets(void)
{
    static const norflash_asset_range_t ranges[] = {
        {"wallpaper", QSPI_WALLPAPER_OFFSET, QSPI_WALLPAPER_SIZE},
        {"img1", QSPI_IMG1_OFFSET, QSPI_IMG1_SIZE},
        {"img2", QSPI_IMG2_OFFSET, QSPI_IMG2_SIZE},
        {"img3", QSPI_IMG3_OFFSET, QSPI_IMG3_SIZE}
    };
    uint32_t index;
    uint8_t result = 0U;

    norflash_exit_memmap();
    for (index = 0U; index < (sizeof(ranges) / sizeof(ranges[0])); index++) {
        if (norflash_erase_asset_range(&ranges[index]) != 0U) {
            result = 1U;
            break;
        }
    }
    norflash_enter_memmap();
    return result;
}


/* ================================================================
 *  SD→QSPI fallback: copy from SD card when QSPI is empty
 * ================================================================ */

#define QSPI_WALLPAPER_CRC32  0x83A48888UL
#define QSPI_IMG1_CRC32       0xC9991D3DUL
#define QSPI_IMG2_CRC32       0x154F3C06UL
#define QSPI_IMG3_CRC32       0xBD4625B2UL

#define NORFLASH_ASSET_FAILED  0U
#define NORFLASH_ASSET_HIT     1U
#define NORFLASH_ASSET_BUILT   2U

static uint32_t norflash_crc32_update(uint32_t crc, const uint8_t *data,
                                      uint32_t length)
{
    uint32_t i;
    uint32_t bit;

    for (i = 0U; i < length; i++) {
        crc ^= data[i];
        for (bit = 0U; bit < 8U; bit++) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static uint32_t norflash_read_crc32(const char *item, uint32_t qspi_offset,
                                    uint32_t size, uint8_t *destination,
                                    norflash_progress_action_t action)
{
    uint8_t buffer[256];
    uint8_t *chunk_data;
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t offset;
    uint32_t chunk;

    norflash_report_progress(item, action, 0U, size);
    for (offset = 0U; offset < size; offset += chunk) {
        chunk = (size - offset > sizeof(buffer)) ?
                (uint32_t)sizeof(buffer) : (size - offset);
        if (destination != NULL) {
            chunk_data = destination + offset;
        } else {
            chunk_data = buffer;
        }
        norflash_read(chunk_data, qspi_offset + offset, (uint16_t)chunk);
        crc = norflash_crc32_update(crc, chunk_data, chunk);
        led_poc(1, 2);
        norflash_report_progress(item, action, offset + chunk, size);
    }
    return crc ^ 0xFFFFFFFFUL;
}

static uint8_t sd_to_qspi_asset(const char *item, const char *sd_path,
                                uint32_t qspi_offset, uint32_t expected_size,
                                uint32_t expected_crc)
{
    FIL fil;
    FRESULT res;
    UINT br;
    uint8_t buffer[256];
    uint32_t source_crc = 0xFFFFFFFFUL;
    uint32_t file_size;
    uint32_t source_offset = 0U;
    uint32_t total;
    uint32_t sectors;
    uint32_t sector;
    uint8_t legacy_wrapped = 0U;

    res = f_open(&fil, sd_path, FA_READ);
    if (res != FR_OK) {
        norflash_report_progress(item, NORFLASH_PROGRESS_ERROR, 0U, 0U);
        printf("[BOOT] assets FAIL item=%s action=open code=%d\r\n",
               item, res);
        return 1U;
    }
    file_size = f_size(&fil);
    if (file_size == expected_size) {
        source_offset = 0U;
    } else if (file_size == (expected_size + 7U)) {
        source_offset = 4U;
        legacy_wrapped = 1U;
    } else {
        f_close(&fil);
        norflash_report_progress(item, NORFLASH_PROGRESS_ERROR,
                                 file_size, expected_size);
        printf("[BOOT] assets FAIL item=%s action=size code=%lu expected=%lu\r\n",
               item, (unsigned long)file_size,
               (unsigned long)expected_size);
        return 1U;
    }
    res = f_lseek(&fil, source_offset);
    if (res != FR_OK) {
        f_close(&fil);
        norflash_report_progress(item, NORFLASH_PROGRESS_ERROR, 0U, expected_size);
        printf("[BOOT] assets FAIL item=%s action=seek code=%d\r\n",
               item, res);
        return 1U;
    }

    total = 0U;
    norflash_report_progress(item, NORFLASH_PROGRESS_VERIFY, 0U, expected_size);
    while (total < expected_size) {
        uint32_t remaining = expected_size - total;
        UINT request = (remaining > sizeof(buffer)) ?
                       (UINT)sizeof(buffer) : (UINT)remaining;

        res = f_read(&fil, buffer, request, &br);
        if ((res != FR_OK) || (br != request)) {
            f_close(&fil);
            norflash_report_progress(item, NORFLASH_PROGRESS_ERROR,
                                     total, expected_size);
            printf("[BOOT] assets FAIL item=%s action=read code=%d\r\n",
                   item, res);
            return 1U;
        }
        source_crc = norflash_crc32_update(source_crc, buffer, br);
        total += br;
        norflash_report_progress(item, NORFLASH_PROGRESS_VERIFY,
                                 total, expected_size);
    }
    source_crc ^= 0xFFFFFFFFUL;
    if (source_crc != expected_crc) {
        f_close(&fil);
        norflash_report_progress(item, NORFLASH_PROGRESS_ERROR,
                                 expected_size, expected_size);
        printf("[BOOT] assets FAIL item=%s action=source_crc code=%08lX expected=%08lX\r\n",
               item, (unsigned long)source_crc,
               (unsigned long)expected_crc);
        return 1U;
    }
    if (legacy_wrapped != 0U) {
        printf("[BOOT] assets WARN item=%s source=legacy_wrapped prefix=4 suffix=3\r\n",
               item);
    }
    res = f_lseek(&fil, source_offset);
    if (res != FR_OK) {
        f_close(&fil);
        norflash_report_progress(item, NORFLASH_PROGRESS_ERROR, 0U, expected_size);
        printf("[BOOT] assets FAIL item=%s action=seek code=%d\r\n",
               item, res);
        return 1U;
    }

    norflash_exit_memmap();
    sectors = (expected_size + 4095U) / 4096U;
    norflash_report_progress(item, NORFLASH_PROGRESS_ERASE, 0U, expected_size);
    for (sector = 0U; sector < sectors; sector++) {
        uint32_t erased = (sector + 1U) * 4096U;

        norflash_erase_sector((qspi_offset / 4096U) + sector);
        if (erased > expected_size) {
            erased = expected_size;
        }
        led_poc(1, 2);
        norflash_report_progress(item, NORFLASH_PROGRESS_ERASE,
                                 erased, expected_size);
    }

    total = 0U;
    norflash_report_progress(item, NORFLASH_PROGRESS_COPY, 0U, expected_size);
    while (total < expected_size) {
        uint32_t remaining = expected_size - total;
        UINT request = (remaining > sizeof(buffer)) ?
                       (UINT)sizeof(buffer) : (UINT)remaining;

        res = f_read(&fil, buffer, request, &br);
        if ((res != FR_OK) || (br != request)) {
            f_close(&fil);
            norflash_report_progress(item, NORFLASH_PROGRESS_ERROR,
                                     total, expected_size);
            printf("[BOOT] assets FAIL item=%s action=copy_read code=%d\r\n",
                   item, res);
            return 1U;
        }
        norflash_write(buffer, qspi_offset + total, (uint16_t)br);
        total += br;
        led_poc(1, 2);
        norflash_report_progress(item, NORFLASH_PROGRESS_COPY,
                                 total, expected_size);
    }
    f_close(&fil);
    return 0U;
}

/* Copy a file from SD card to QSPI flash */
static uint8_t sd_to_qspi_one(const char *item, const char *sd_path,
                              uint32_t qspi_offset, uint32_t *out_size)
{
    FIL fil;
    FRESULT res;
    UINT br;
    uint32_t total;
    uint32_t file_size;
    uint32_t sectors;
    uint32_t s;
    uint8_t buf[256];

    res = f_open(&fil, sd_path, FA_READ);
    if (res != FR_OK) {
        norflash_report_progress(item, NORFLASH_PROGRESS_ERROR, 0U, 0U);
        printf("[BOOT] assets FAIL item=%s action=open code=%d\r\n",
               item, res);
        return 1;
    }

    total = f_size(&fil);
    file_size = total;
    if (out_size) *out_size = total;
    sectors = (total + 4095) / 4096;
    norflash_report_progress(item, NORFLASH_PROGRESS_ERASE, 0U, file_size);

    /* Erase sectors */
    {
        uint32_t start_sector;
        start_sector = qspi_offset / 4096;
        for (s = 0; s < sectors; s++) {
            uint32_t erased;

            norflash_erase_sector(start_sector + s);
            led_poc(1, 2);
            erased = (s + 1U) * 4096U;
            if (erased > file_size) {
                erased = file_size;
            }
            norflash_report_progress(item, NORFLASH_PROGRESS_ERASE,
                                     erased, file_size);
        }
    }

    /* Write data in 256-byte chunks */
    total = 0;
    norflash_report_progress(item, NORFLASH_PROGRESS_COPY, 0U, file_size);
    while (1) {
        res = f_read(&fil, buf, 256, &br);
        if (res != FR_OK) {
            f_close(&fil);
            norflash_report_progress(item, NORFLASH_PROGRESS_ERROR,
                                     total, file_size);
            return 1;
        }
        if (br == 0) break;
        norflash_write(buf, qspi_offset + total, (uint16_t)br);
        led_poc(1, 2);
        total += br;
        norflash_report_progress(item, NORFLASH_PROGRESS_COPY,
                                 total, file_size);
    }
    f_close(&fil);

    /* Verify first 256 bytes */
    {
        uint8_t vbuf[256];
        uint32_t i;
        norflash_report_progress(item, NORFLASH_PROGRESS_VERIFY, 0U, 256U);
        norflash_read(vbuf, qspi_offset, 256);
        f_open(&fil, sd_path, FA_READ);
        f_read(&fil, buf, 256, &br);
        f_close(&fil);
        for (i = 0; i < 256; i++) {
            if (vbuf[i] != buf[i]) {
                norflash_report_progress(item, NORFLASH_PROGRESS_ERROR,
                                         i, 256U);
                printf("[BOOT] assets FAIL item=%s action=verify code=%lu\r\n",
                       item, (unsigned long)i);
                return 1;
            }
        }
        norflash_report_progress(item, NORFLASH_PROGRESS_VERIFY, 256U, 256U);
    }
    norflash_report_progress(item, NORFLASH_PROGRESS_DONE,
                             file_size, file_size);
    return 0;
}

static uint8_t norflash_prepare_asset(const char *item, const char *sd_path,
                                      uint32_t qspi_offset, uint32_t size,
                                      uint32_t expected_crc, uint8_t *destination,
                                      uint8_t boot_item)
{
    uint32_t actual_crc;

    actual_crc = norflash_read_crc32(item, qspi_offset, size, destination,
                                     NORFLASH_PROGRESS_LOAD);
    if (actual_crc == expected_crc) {
        printf("[BOOT] asset_cache HIT item=%s crc=%08lX\r\n",
               item, (unsigned long)actual_crc);
        norflash_report_progress(item, NORFLASH_PROGRESS_DONE, size, size);
        return NORFLASH_ASSET_HIT;
    }

    printf("[BOOT] asset_cache MISS item=%s expected=%08lX actual=%08lX\r\n",
           item, (unsigned long)expected_crc, (unsigned long)actual_crc);
    if (sd_to_qspi_asset(item, sd_path, qspi_offset, size, expected_crc) != 0U) {
        norflash_boot_capture_error(boot_item, NORFLASH_BOOT_ERROR_COPY);
        return NORFLASH_ASSET_FAILED;
    }

    actual_crc = norflash_read_crc32(item, qspi_offset, size, destination,
                                     NORFLASH_PROGRESS_VERIFY);
    if (actual_crc != expected_crc) {
        norflash_report_progress(item, NORFLASH_PROGRESS_ERROR, size, size);
        printf("[BOOT] assets FAIL item=%s action=verify_crc code=%08lX expected=%08lX\r\n",
               item, (unsigned long)actual_crc, (unsigned long)expected_crc);
        norflash_boot_capture_error(boot_item, NORFLASH_BOOT_ERROR_COPY);
        return NORFLASH_ASSET_FAILED;
    }

    printf("[BOOT] asset_cache BUILT item=%s crc=%08lX bytes=%lu\r\n",
           item, (unsigned long)actual_crc, (unsigned long)size);
    norflash_report_progress(item, NORFLASH_PROGRESS_DONE, size, size);
    return NORFLASH_ASSET_BUILT;
}

/* ================================================================
 *  壁纸迁移
 * ================================================================ */


#define WALLPAPER_SDRAM_ADDR  0xC0200000UL  /* 2MB offset in SDRAM (MPU Region5, 32MB, Cacheable) */

/* 从 QSPI 预加载壁纸到 SDRAM (间接模式, 已验证可靠)
 * 若 QSPI 为空则自动从 SD 卡拷贝 */
void norflash_preload_wallpaper(void)
{
    uint8_t result;

    result = norflash_prepare_asset(
        "wallpaper", "0:/SYSTEM/IMG/_PPC_lock_RGB565_1024x600.bin",
        QSPI_WALLPAPER_OFFSET, QSPI_WALLPAPER_SIZE, QSPI_WALLPAPER_CRC32,
        (uint8_t *)WALLPAPER_SDRAM_ADDR, NORFLASH_BOOT_ITEM_WALLPAPER);
    if (result != NORFLASH_ASSET_FAILED) {
        g_boot_status.wallpaper_ready = 1U;
    }
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


void norflash_preload_images(void)
{
    uint8_t img1_result;
    uint8_t img2_result;
    uint8_t img3_result;

    img1_result = norflash_prepare_asset(
        "img1", "0:/SYSTEM/IMG/_1_RGB565_285x107.bin",
        QSPI_IMG1_OFFSET, QSPI_IMG1_SIZE, QSPI_IMG1_CRC32,
        (uint8_t *)IMG1_SDRAM_ADDR, NORFLASH_BOOT_ITEM_IMG1);
    img2_result = norflash_prepare_asset(
        "img2", "0:/SYSTEM/IMG/_2_RGB565_700x370.bin",
        QSPI_IMG2_OFFSET, QSPI_IMG2_SIZE, QSPI_IMG2_CRC32,
        (uint8_t *)IMG2_SDRAM_ADDR, NORFLASH_BOOT_ITEM_IMG2);
    img3_result = norflash_prepare_asset(
        "img3", "0:/SYSTEM/IMG/_3_RGB565_600x220.bin",
        QSPI_IMG3_OFFSET, QSPI_IMG3_SIZE, QSPI_IMG3_CRC32,
        (uint8_t *)IMG3_SDRAM_ADDR, NORFLASH_BOOT_ITEM_IMG3);

    /* D-Cache clean */
    SCB_CleanDCache_by_Addr((uint32_t *)IMG1_SDRAM_ADDR, QSPI_IMG1_SIZE);
    SCB_CleanDCache_by_Addr((uint32_t *)IMG2_SDRAM_ADDR, QSPI_IMG2_SIZE);
    SCB_CleanDCache_by_Addr((uint32_t *)IMG3_SDRAM_ADDR, QSPI_IMG3_SIZE);
    if (img1_result != NORFLASH_ASSET_FAILED) {
        g_boot_status.images_ready++;
    }
    if (img2_result != NORFLASH_ASSET_FAILED) {
        g_boot_status.images_ready++;
    }
    if (img3_result != NORFLASH_ASSET_FAILED) {
        g_boot_status.images_ready++;
    }
    if (img1_result == NORFLASH_ASSET_BUILT) {
        g_boot_status.images_copied++;
    }
    if (img2_result == NORFLASH_ASSET_BUILT) {
        g_boot_status.images_copied++;
    }
    if (img3_result == NORFLASH_ASSET_BUILT) {
        g_boot_status.images_copied++;
    }
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



/* ---- Font init: copy from SD to QSPI, then load via font_bridge ---- */

void norflash_init_font12(void)
{
    uint8_t copy_failed;

    norflash_enter_memmap();
#if !FORCE_QSPI_FONT_COPY
    norflash_report_progress("font12", NORFLASH_PROGRESS_LOAD, 0U, 1U);
    if (font_bridge_load_one(&lv_font_myyh_12, QSPI_FONT12_OFFSET, QSPI_FONT12_MAX_SIZE, 12)) {
        g_boot_status.fonts_ready++;
        norflash_report_progress("font12", NORFLASH_PROGRESS_DONE, 1U, 1U);
        return;
    }
#endif
    norflash_exit_memmap();
    copy_failed = sd_to_qspi_one("font12",
                                 "0:/SYSTEM/FONT/lv_font_myh_ui_12.bin",
                                 QSPI_FONT12_OFFSET, NULL);
    if (copy_failed != 0U) {
        norflash_boot_capture_error(NORFLASH_BOOT_ITEM_FONT12,
                                    NORFLASH_BOOT_ERROR_COPY);
    }
    norflash_enter_memmap();
    norflash_report_progress("font12", NORFLASH_PROGRESS_LOAD, 0U, 1U);
    if (font_bridge_load_one(&lv_font_myyh_12, QSPI_FONT12_OFFSET, QSPI_FONT12_MAX_SIZE, 12)) {
        g_boot_status.fonts_ready++;
        norflash_report_progress("font12", NORFLASH_PROGRESS_DONE, 1U, 1U);
    } else {
        norflash_report_progress("font12", NORFLASH_PROGRESS_ERROR, 0U, 1U);
        norflash_boot_capture_error(NORFLASH_BOOT_ITEM_FONT12,
                                    NORFLASH_BOOT_ERROR_PARSE);
        if (copy_failed == 0U) {
            printf("[BOOT] assets FAIL item=font12 action=parse code=2\r\n");
        }
    }
}

void norflash_init_font14(void)
{
    uint8_t copy_failed;

    norflash_enter_memmap();
#if !FORCE_QSPI_FONT_COPY
    norflash_report_progress("font14", NORFLASH_PROGRESS_LOAD, 0U, 1U);
    if (font_bridge_load_one(&lv_font_myyh_14, QSPI_FONT14_OFFSET, QSPI_FONT14_MAX_SIZE, 14)) {
        g_boot_status.fonts_ready++;
        norflash_report_progress("font14", NORFLASH_PROGRESS_DONE, 1U, 1U);
        return;
    }
#endif
    norflash_exit_memmap();
    copy_failed = sd_to_qspi_one("font14",
                                 "0:/SYSTEM/FONT/lv_font_myh_ui_14.bin",
                                 QSPI_FONT14_OFFSET, NULL);
    if (copy_failed != 0U) {
        norflash_boot_capture_error(NORFLASH_BOOT_ITEM_FONT14,
                                    NORFLASH_BOOT_ERROR_COPY);
    }
    norflash_enter_memmap();
    norflash_report_progress("font14", NORFLASH_PROGRESS_LOAD, 0U, 1U);
    if (font_bridge_load_one(&lv_font_myyh_14, QSPI_FONT14_OFFSET, QSPI_FONT14_MAX_SIZE, 14)) {
        g_boot_status.fonts_ready++;
        norflash_report_progress("font14", NORFLASH_PROGRESS_DONE, 1U, 1U);
    } else {
        norflash_report_progress("font14", NORFLASH_PROGRESS_ERROR, 0U, 1U);
        norflash_boot_capture_error(NORFLASH_BOOT_ITEM_FONT14,
                                    NORFLASH_BOOT_ERROR_PARSE);
        if (copy_failed == 0U) {
            printf("[BOOT] assets FAIL item=font14 action=parse code=2\r\n");
        }
    }
}

void norflash_init_font16(void)
{
    uint8_t copy_failed;

    norflash_enter_memmap();
#if !FORCE_QSPI_FONT_COPY
    norflash_report_progress("font16", NORFLASH_PROGRESS_LOAD, 0U, 1U);
    if (font_bridge_load_one(&lv_font_myyh_16, QSPI_FONT16_OFFSET, QSPI_FONT16_MAX_SIZE, 16)) {
        g_boot_status.fonts_ready++;
        norflash_report_progress("font16", NORFLASH_PROGRESS_DONE, 1U, 1U);
        return;
    }
#endif
    norflash_exit_memmap();
    copy_failed = sd_to_qspi_one("font16",
                                 "0:/SYSTEM/FONT/lv_font_myh_ui_16.bin",
                                 QSPI_FONT16_OFFSET, NULL);
    if (copy_failed != 0U) {
        norflash_boot_capture_error(NORFLASH_BOOT_ITEM_FONT16,
                                    NORFLASH_BOOT_ERROR_COPY);
    }
    norflash_enter_memmap();
    norflash_report_progress("font16", NORFLASH_PROGRESS_LOAD, 0U, 1U);
    if (font_bridge_load_one(&lv_font_myyh_16, QSPI_FONT16_OFFSET, QSPI_FONT16_MAX_SIZE, 16)) {
        g_boot_status.fonts_ready++;
        norflash_report_progress("font16", NORFLASH_PROGRESS_DONE, 1U, 1U);
    } else {
        norflash_report_progress("font16", NORFLASH_PROGRESS_ERROR, 0U, 1U);
        norflash_boot_capture_error(NORFLASH_BOOT_ITEM_FONT16,
                                    NORFLASH_BOOT_ERROR_PARSE);
        if (copy_failed == 0U) {
            printf("[BOOT] assets FAIL item=font16 action=parse code=2\r\n");
        }
    }
}

void norflash_init_font18(void)
{
    uint8_t copy_failed;

    norflash_enter_memmap();
#if !FORCE_QSPI_FONT_COPY
    norflash_report_progress("font18", NORFLASH_PROGRESS_LOAD, 0U, 1U);
    if (font_bridge_load_one(&lv_font_myyh_18, QSPI_FONT18_OFFSET, QSPI_FONT18_MAX_SIZE, 18)) {
        g_boot_status.fonts_ready++;
        norflash_report_progress("font18", NORFLASH_PROGRESS_DONE, 1U, 1U);
        return;
    }
#endif
    norflash_exit_memmap();
    copy_failed = sd_to_qspi_one("font18",
                                 "0:/SYSTEM/FONT/lv_font_myh_ui_18.bin",
                                 QSPI_FONT18_OFFSET, NULL);
    if (copy_failed != 0U) {
        norflash_boot_capture_error(NORFLASH_BOOT_ITEM_FONT18,
                                    NORFLASH_BOOT_ERROR_COPY);
    }
    norflash_enter_memmap();
    norflash_report_progress("font18", NORFLASH_PROGRESS_LOAD, 0U, 1U);
    if (font_bridge_load_one(&lv_font_myyh_18, QSPI_FONT18_OFFSET, QSPI_FONT18_MAX_SIZE, 18)) {
        g_boot_status.fonts_ready++;
        norflash_report_progress("font18", NORFLASH_PROGRESS_DONE, 1U, 1U);
    } else {
        norflash_report_progress("font18", NORFLASH_PROGRESS_ERROR, 0U, 1U);
        norflash_boot_capture_error(NORFLASH_BOOT_ITEM_FONT18,
                                    NORFLASH_BOOT_ERROR_PARSE);
        if (copy_failed == 0U) {
            printf("[BOOT] assets FAIL item=font18 action=parse code=2\r\n");
        }
    }
}

void norflash_init_font20(void)
{
    uint8_t copy_failed;

    norflash_enter_memmap();
#if !FORCE_QSPI_FONT_COPY
    norflash_report_progress("font20", NORFLASH_PROGRESS_LOAD, 0U, 1U);
    if (font_bridge_load_one(&lv_font_myyh_20, QSPI_FONT20_OFFSET, QSPI_FONT20_MAX_SIZE, 20)) {
        g_boot_status.fonts_ready++;
        norflash_report_progress("font20", NORFLASH_PROGRESS_DONE, 1U, 1U);
        return;
    }
#endif
    norflash_exit_memmap();
    copy_failed = sd_to_qspi_one("font20",
                                 "0:/SYSTEM/FONT/lv_font_myh_ui_20.bin",
                                 QSPI_FONT20_OFFSET, NULL);
    if (copy_failed != 0U) {
        norflash_boot_capture_error(NORFLASH_BOOT_ITEM_FONT20,
                                    NORFLASH_BOOT_ERROR_COPY);
    }
    norflash_enter_memmap();
    norflash_report_progress("font20", NORFLASH_PROGRESS_LOAD, 0U, 1U);
    if (font_bridge_load_one(&lv_font_myyh_20, QSPI_FONT20_OFFSET, QSPI_FONT20_MAX_SIZE, 20)) {
        g_boot_status.fonts_ready++;
        norflash_report_progress("font20", NORFLASH_PROGRESS_DONE, 1U, 1U);
    } else {
        norflash_report_progress("font20", NORFLASH_PROGRESS_ERROR, 0U, 1U);
        norflash_boot_capture_error(NORFLASH_BOOT_ITEM_FONT20,
                                    NORFLASH_BOOT_ERROR_PARSE);
        if (copy_failed == 0U) {
            printf("[BOOT] assets FAIL item=font20 action=parse code=2\r\n");
        }
    }
}

void norflash_init_font30(void)
{
    uint8_t copy_failed;

    norflash_enter_memmap();
#if !FORCE_QSPI_FONT_COPY
    norflash_report_progress("font30", NORFLASH_PROGRESS_LOAD, 0U, 1U);
    if (font_bridge_load_one(&lv_font_myyh_30, QSPI_FONT30_OFFSET, QSPI_FONT30_MAX_SIZE, 30)) {
        g_boot_status.fonts_ready++;
        norflash_report_progress("font30", NORFLASH_PROGRESS_DONE, 1U, 1U);
        return;
    }
#endif
    norflash_exit_memmap();
    copy_failed = sd_to_qspi_one("font30",
                                 "0:/SYSTEM/FONT/lv_font_myh_ui_30.bin",
                                 QSPI_FONT30_OFFSET, NULL);
    if (copy_failed != 0U) {
        norflash_boot_capture_error(NORFLASH_BOOT_ITEM_FONT30,
                                    NORFLASH_BOOT_ERROR_COPY);
    }
    norflash_enter_memmap();
    norflash_report_progress("font30", NORFLASH_PROGRESS_LOAD, 0U, 1U);
    if (font_bridge_load_one(&lv_font_myyh_30, QSPI_FONT30_OFFSET, QSPI_FONT30_MAX_SIZE, 30)) {
        g_boot_status.fonts_ready++;
        norflash_report_progress("font30", NORFLASH_PROGRESS_DONE, 1U, 1U);
    } else {
        norflash_report_progress("font30", NORFLASH_PROGRESS_ERROR, 0U, 1U);
        norflash_boot_capture_error(NORFLASH_BOOT_ITEM_FONT30,
                                    NORFLASH_BOOT_ERROR_PARSE);
        if (copy_failed == 0U) {
            printf("[BOOT] assets FAIL item=font30 action=parse code=2\r\n");
        }
    }
}
