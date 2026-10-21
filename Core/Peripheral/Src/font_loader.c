/**
 ********************************************************************
 * @file    font_loader.c
 * @brief   LVGL binary font loader implementation
 * @note    Depends on FATFS (ff.h) for SD card access
 *          Uses LVGL v9 lv_binfont_create / lv_binfont_create_from_buffer
 ********************************************************************
 */

#include "font_loader.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

/* ================================================================
 *  Internal helpers
 * ================================================================ */

/* Build SD card path: "0:fonts/lv_font_{name}_{size}.bin" */
static void build_sd_path(char *out, uint32_t out_size,
                          const char *font_name, font_size_t size)
{
    snprintf(out, out_size, "0:fonts/lv_font_%s_%d.bin", font_name, (int)size);
}

/* Check if a FATFS file exists and get its size */
static int fatfs_get_file_size(const char *path, uint32_t *out_size)
{
    FIL     fil;
    FRESULT res;

    res = f_open(&fil, path, FA_READ);
    if (res != FR_OK) {
        return -1;
    }

    *out_size = (uint32_t)f_size(&fil);
    f_close(&fil);
    return 0;
}

/* Read entire file into SDRAM buffer, return pointer (caller frees via lv_free) */
static uint8_t *fatfs_read_entire_file(const char *path, uint32_t *out_size)
{
    FIL      fil;
    FRESULT  res;
    UINT     bytes_read;
    uint32_t file_size;
    uint8_t *buf;

    res = f_open(&fil, path, FA_READ);
    if (res != FR_OK) {
        printf("[FONT_LOADER] Cannot open: %s (err=%d)\r\n", path, (int)res);
        return NULL;
    }

    file_size = (uint32_t)f_size(&fil);

    /* Use LVGL allocator so that binfont can manage memory */
    buf = (uint8_t *)lv_malloc(file_size);
    if (buf == NULL) {
        printf("[FONT_LOADER] lv_malloc(%lu) failed for %s\r\n",
               (unsigned long)file_size, path);
        f_close(&fil);
        return NULL;
    }

    res = f_read(&fil, buf, file_size, &bytes_read);
    f_close(&fil);

    if (res != FR_OK || bytes_read != file_size) {
        printf("[FONT_LOADER] Read error: %s (got %u/%lu)\r\n",
               path, (unsigned int)bytes_read, (unsigned long)file_size);
        lv_free(buf);
        return NULL;
    }

    *out_size = file_size;
    return buf;
}

/* ================================================================
 *  Public API
 * ================================================================ */

/**
 * @brief  Load a BIN font from SD card (FATFS path)
 */
font_handle_t font_loader_load_from_sd(font_size_t size, const char *font_name)
{
    font_handle_t result = { .font = NULL, .size = size, .loaded = 0 };
    char          path[64];
    uint32_t      file_size;
    uint8_t      *buf;

    build_sd_path(path, sizeof(path), font_name, size);

    printf("[FONT_LOADER] Loading %s from SD...\r\n", path);

    buf = fatfs_read_entire_file(path, &file_size);
    if (buf == NULL) {
        printf("[FONT_LOADER] FAILED: %s\r\n", path);
        return result;
    }

    result.font = lv_binfont_create_from_buffer(buf, file_size);
    if (result.font == NULL) {
        printf("[FONT_LOADER] lv_binfont_create_from_buffer FAILED for %s\r\n", path);
        lv_free(buf);
        return result;
    }

    result.loaded = 1;
    printf("[FONT_LOADER] OK: %s (%lu bytes)\r\n", path, (unsigned long)file_size);
    return result;
}

/**
 * @brief  Load a BIN font from QSPI memory-mapped address
 */
font_handle_t font_loader_load_from_qspi(font_size_t size,
                                          uint32_t base_addr,
                                          uint32_t max_size)
{
    font_handle_t result = { .font = NULL, .size = size, .loaded = 0 };

    printf("[FONT_LOADER] Loading %dpx font from QSPI 0x%08lX...\r\n",
           (int)size, (unsigned long)base_addr);

    /* Check if first bytes look like a valid LVGL binfont header */
    const uint8_t *ptr = (const uint8_t *)base_addr;

    /* LVGL binfont: first 4 bytes = version, next 4 bytes = size */
    if (ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 0 && ptr[3] == 0) {
        printf("[FONT_LOADER] QSPI area at 0x%08lX appears empty\r\n",
               (unsigned long)base_addr);
        return result;
    }

    result.font = lv_binfont_create_from_buffer((void *)base_addr, max_size);
    if (result.font == NULL) {
        printf("[FONT_LOADER] lv_binfont_create_from_buffer FAILED for QSPI 0x%08lX\r\n",
               (unsigned long)base_addr);
        return result;
    }

    result.loaded = 1;
    printf("[FONT_LOADER] OK: QSPI 0x%08lX (%lpx)\r\n",
           (unsigned long)base_addr, (unsigned long)size);
    return result;
}

/**
 * @brief  Load all 6 font sizes from SD card
 */
void font_loader_load_all_from_sd(const char *font_name, font_handle_t handles[6])
{
    font_size_t sizes[] = {
        FONT_SIZE_12, FONT_SIZE_14, FONT_SIZE_16,
        FONT_SIZE_18, FONT_SIZE_20, FONT_SIZE_30
    };

    for (int i = 0; i < 6; i++) {
        handles[i] = font_loader_load_from_sd(sizes[i], font_name);
    }
}

/**
 * @brief  Unload a font and free resources
 */
void font_loader_unload(font_handle_t *handle)
{
    if (handle == NULL || handle->font == NULL) return;

    printf("[FONT_LOADER] Unloading %dpx font\r\n", (int)handle->size);
    lv_binfont_destroy(handle->font);
    handle->font   = NULL;
    handle->loaded = 0;
}
