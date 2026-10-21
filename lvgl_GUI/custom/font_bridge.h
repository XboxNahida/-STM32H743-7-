/**
 ********************************************************************
 * @file    font_bridge.h
 * @brief   Runtime BIN font bridge
 ********************************************************************
 */
#ifndef __FONT_BRIDGE_H__
#define __FONT_BRIDGE_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern lv_font_t lv_font_myyh_12;
extern lv_font_t lv_font_myyh_14;
extern lv_font_t lv_font_myyh_16;
extern lv_font_t lv_font_myyh_18;
extern lv_font_t lv_font_myyh_20;
extern lv_font_t lv_font_myyh_30;

/**
 * @brief  Load one font from QSPI memory-mapped address.
 *         Caller must have called norflash_enter_memmap() first.
 * @param  dest        pointer to lv_font_t to fill
 * @param  qspi_offset offset within QSPI flash
 * @param  max_size    max bytes to read
 * @param  size_px     font size (for logging)
 */
bool font_bridge_load_one(lv_font_t *dest, uint32_t qspi_offset,
                          uint32_t max_size, int size_px);

#ifdef __cplusplus
}
#endif

#endif
