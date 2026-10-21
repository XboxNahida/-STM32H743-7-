/**
 ********************************************************************
 * @file    font_loader.h
 * @brief   LVGL binary font loader - loads BIN fonts from SD/QSPI
 * @note    Uses lv_binfont_create() to load --format bin output
 *          No font data needs to be compiled into firmware.
 ********************************************************************
 */
#ifndef __FONT_LOADER_H__
#define __FONT_LOADER_H__

#include "lvgl.h"

/* Font sizes available */
typedef enum {
    FONT_SIZE_12 = 12,
    FONT_SIZE_14 = 14,
    FONT_SIZE_16 = 16,
    FONT_SIZE_18 = 18,
    FONT_SIZE_20 = 20,
    FONT_SIZE_30 = 30,
} font_size_t;

/* Font source type */
typedef enum {
    FONT_SRC_SD,      /* Load from SD card via FATFS path */
    FONT_SRC_QSPI,    /* Load from QSPI memory-mapped address */
    FONT_SRC_BUFFER,  /* Load from pre-loaded memory buffer */
} font_src_type_t;

/* Font descriptor returned by loader */
typedef struct {
    lv_font_t   *font;          /* LVGL font object */
    font_size_t  size;          /* Font size in pixels */
    uint8_t      loaded;        /* 1 = successfully loaded */
} font_handle_t;

/* ===== API ===== */

/**
 * @brief  Load a BIN font from SD card (FATFS path)
 * @param  size       Font size (12/14/16/18/20/30)
 * @param  font_name  Font name prefix (e.g., "myh_ui")
 * @return font_handle_t with .font pointing to LVGL font
 * @note   Path format: "0:fonts/lv_font_{font_name}_{size}.bin"
 */
font_handle_t font_loader_load_from_sd(font_size_t size, const char *font_name);

/**
 * @brief  Load a BIN font from QSPI memory-mapped address
 * @param  size       Font size (12/14/16/18/20/30)
 * @param  base_addr  Memory-mapped base address of the BIN data
 * @param  max_size   Maximum size of the BIN data area
 * @return font_handle_t with .font pointing to LVGL font
 * @note   Uses lv_binfont_create_from_buffer() with the QSPI address
 */
font_handle_t font_loader_load_from_qspi(font_size_t size, uint32_t base_addr, uint32_t max_size);

/**
 * @brief  Load all font sizes for a given font name from SD card
 * @param  font_name  Font name prefix (e.g., "myh_ui")
 * @param  handles    Output array of 6 font_handle_t
 * @note   Loads sizes 12, 14, 16, 18, 20, 30
 */
void font_loader_load_all_from_sd(const char *font_name, font_handle_t handles[6]);

/**
 * @brief  Unload a font and free resources
 * @param  handle  Font handle from font_loader_load_*
 */
void font_loader_unload(font_handle_t *handle);

#endif /* __FONT_LOADER_H__ */
