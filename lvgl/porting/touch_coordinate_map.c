#include "touch_coordinate_map.h"

#include <stddef.h>

static int32_t touch_coordinate_clamp(int32_t value, int32_t maximum)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > maximum)
    {
        return maximum;
    }
    return value;
}

int touch_coordinate_map(int32_t raw_x, int32_t raw_y,
                         int32_t *screen_x, int32_t *screen_y)
{
    if ((screen_x == NULL) || (screen_y == NULL))
    {
        return 1;
    }
    *screen_x = touch_coordinate_clamp(
        raw_y, TOUCH_COORDINATE_SCREEN_WIDTH - 1);
    *screen_y = touch_coordinate_clamp(
        raw_x, TOUCH_COORDINATE_SCREEN_HEIGHT - 1);
    return 0;
}
