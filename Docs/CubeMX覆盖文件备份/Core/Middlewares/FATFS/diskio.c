/**
 ********************************************************************
 * @file    diskio.c
 * @brief   FATFS disk I/O layer for PPC
 * @note    Uses sdcard.c for SD card access
 ********************************************************************
 */
#include "diskio.h"
#include "sdcard.h"
#include "main.h"

#define SD_CARD  0

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv == SD_CARD) {
        if (sdcard_is_ready()) return 0;
        return STA_NOINIT;
    }
    return STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv == SD_CARD) {
        if (sdcard_is_ready()) return 0;
        return STA_NOINIT;
    }
    return STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != SD_CARD) return RES_PARERR;
    if (sdcard_read_disk(buff, sector, count) == 0)
        return RES_OK;
    return RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != SD_CARD) return RES_PARERR;
    if (sdcard_write_disk((uint8_t *)buff, sector, count) == 0)
        return RES_OK;
    return RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != SD_CARD) return RES_PARERR;
    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_COUNT:
        *(DWORD *)buff = 0;  /* placeholder */
        return RES_OK;
    case GET_SECTOR_SIZE:
        *(WORD *)buff = 512;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}


/* ---- FATFS system call stubs ---- */
#include <stdlib.h>

void *ff_memalloc(UINT msize)
{
    return malloc(msize);
}

void ff_memfree(void *mblock)
{
    free(mblock);
}

/* Re-entrancy stubs (not used with FF_FS_REENTRANT=0, but linked) */
uint32_t ff_enter(void) { return 0; }
void ff_leave(uint32_t sync) { (void)sync; }
DWORD get_fattime(void)
{
    return 0;
}