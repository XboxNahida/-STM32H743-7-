#include <stdint.h>
#include <stdio.h>

#include "touch_coordinate_map.h"

static int expect_point(const char *name, int32_t raw_x, int32_t raw_y,
                        int32_t expected_x, int32_t expected_y)
{
    int32_t screen_x = -1;
    int32_t screen_y = -1;

    if ((touch_coordinate_map(raw_x, raw_y, &screen_x, &screen_y) != 0) ||
        (screen_x != expected_x) || (screen_y != expected_y))
    {
        printf("FAIL %s raw=(%ld,%ld) actual=(%ld,%ld) expected=(%ld,%ld)\n",
               name, (long)raw_x, (long)raw_y,
               (long)screen_x, (long)screen_y,
               (long)expected_x, (long)expected_y);
        return 1;
    }
    return 0;
}

int main(void)
{
    int32_t output = 0;
    int failures = 0;

    failures += expect_point("origin", 0, 0, 0, 0);
    failures += expect_point("center", 300, 512, 512, 300);
    failures += expect_point("last pixel", 599, 1023, 1023, 599);
    failures += expect_point("old measured minimum stays pixel", 38, 44,
                             44, 38);
    failures += expect_point("negative clamp", -20, -30, 0, 0);
    failures += expect_point("upper clamp", 900, 1400, 1023, 599);
    if (touch_coordinate_map(0, 0, NULL, &output) == 0)
    {
        printf("FAIL null screen_x accepted\n");
        failures++;
    }
    if (touch_coordinate_map(0, 0, &output, NULL) == 0)
    {
        printf("FAIL null screen_y accepted\n");
        failures++;
    }
    if (failures == 0)
    {
        printf("PASS touch_coordinate_map\n");
    }
    return failures;
}
