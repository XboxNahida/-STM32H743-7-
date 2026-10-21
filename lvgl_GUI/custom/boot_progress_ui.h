#ifndef BOOT_PROGRESS_UI_H
#define BOOT_PROGRESS_UI_H

#include <stdbool.h>

#include "lvgl.h"
#include "boot_progress_service.h"

bool boot_progress_ui_create(void);
void boot_progress_ui_set_wallpaper(const lv_image_dsc_t *wallpaper);
void boot_progress_ui_set_font(const lv_font_t *font);
void boot_progress_ui_set_chinese(bool enabled);
void boot_progress_ui_refresh(const boot_progress_snapshot_t *snapshot);
void boot_progress_ui_destroy(void);

#endif
