#ifndef TOUCH_COORDINATE_MAP_H
#define TOUCH_COORDINATE_MAP_H

#include <stdint.h>

#define TOUCH_COORDINATE_SCREEN_WIDTH  1024
#define TOUCH_COORDINATE_SCREEN_HEIGHT  600

int touch_coordinate_map(int32_t raw_x, int32_t raw_y,
                         int32_t *screen_x, int32_t *screen_y);

#endif
