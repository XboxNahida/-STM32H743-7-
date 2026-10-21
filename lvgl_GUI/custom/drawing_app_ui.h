#ifndef DRAWING_APP_UI_H
#define DRAWING_APP_UI_H

#include "gui_guider.h"

#define DRAWING_COLOR_COUNT 6U

int drawing_app_ui_attach(lv_ui *ui,
                          lv_obj_t *swatches[DRAWING_COLOR_COUNT]);
int drawing_app_ui_request_open_file(const char *path);

#endif
