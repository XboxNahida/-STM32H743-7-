#ifndef CAMERA_UI_BRIDGE_H
#define CAMERA_UI_BRIDGE_H

#include <stdint.h>

#include "gui_guider.h"

typedef enum
{
    CAMERA_FLASH_MODE_OFF = 0,
    CAMERA_FLASH_MODE_ON,
    CAMERA_FLASH_MODE_AUTO
} camera_flash_mode_t;

void camera_ui_bridge_attach(lv_ui *ui);
void camera_ui_bridge_poll(void);
void camera_ui_bridge_request_leave(void);
camera_flash_mode_t camera_ui_bridge_get_flash_mode(void);

#endif
