#ifndef IMAGE_VIEWER_UI_H
#define IMAGE_VIEWER_UI_H

#include <stdint.h>
#include "image_viewer_app.h"

void image_viewer_ui_init(void);
void image_viewer_ui_poll(void);
int image_viewer_ui_open_album(const char *directory);
int image_viewer_ui_open_file(const char *path);
int image_viewer_ui_open_unsupported(const char *path);
int image_viewer_ui_next(void);
int image_viewer_ui_previous(void);
void image_viewer_ui_stop(void);

#endif
