#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nand_cache_layout.h"

typedef struct
{
    uint8_t bad[512];
    int32_t fail_at;
} fake_scan_t;

static int fake_is_bad(uint32_t block, void *context)
{
    fake_scan_t *scan = (fake_scan_t *)context;

    if ((scan->fail_at >= 0) && (block == (uint32_t)scan->fail_at))
    {
        return -1;
    }
    return (scan->bad[block] != 0U) ? 1 : 0;
}

int main(void)
{
    nand_cache_layout_t layout;
    fake_scan_t scan;
    uint32_t index;
    int failures = 0;

    memset(&scan, 0, sizeof(scan));
    scan.fail_at = -1;
    scan.bad[0] = 1U;
    scan.bad[2] = 1U;
    scan.bad[200] = 1U;

    memset(&layout, 0xA5, sizeof(layout));
    if ((nand_cache_layout_build(2048U, 64U, 512U, fake_is_bad,
                                 &scan, &layout) != NAND_CACHE_LAYOUT_OK) ||
        (layout.slot_blocks[0][0] != 1U) ||
        (layout.slot_blocks[0][1] != 3U) ||
        (layout.slot_blocks[1][0] != 194U) ||
        (layout.metadata_blocks[0] != 387U) ||
        (layout.metadata_blocks[3] != 390U) ||
        (layout.scanned_blocks != 391U) ||
        (layout.good_blocks != 388U) ||
        (layout.bad_blocks != 3U))
    {
        printf("FAIL skips_bad_blocks\n");
        failures++;
    }

    for (index = 1U; index < NAND_CACHE_SLOT_BLOCKS; index++)
    {
        if (layout.slot_blocks[0][index] <= layout.slot_blocks[0][index - 1U])
        {
            printf("FAIL slot_order\n");
            failures++;
            break;
        }
    }

    memset(&scan, 0, sizeof(scan));
    scan.fail_at = -1;
    if ((nand_cache_layout_build(2048U, 64U, 388U, fake_is_bad,
                                 &scan, &layout) != NAND_CACHE_LAYOUT_OK) ||
        (layout.scanned_blocks != 388U) ||
        (layout.metadata_blocks[3] != 387U))
    {
        printf("FAIL exact_capacity\n");
        failures++;
    }

    scan.bad[7] = 1U;
    if (nand_cache_layout_build(2048U, 64U, 388U, fake_is_bad,
                                &scan, &layout) != NAND_CACHE_LAYOUT_NO_SPACE)
    {
        printf("FAIL insufficient_good_blocks\n");
        failures++;
    }

    memset(&scan, 0, sizeof(scan));
    scan.fail_at = 17;
    if (nand_cache_layout_build(2048U, 64U, 512U, fake_is_bad,
                                &scan, &layout) != NAND_CACHE_LAYOUT_SCAN_ERROR)
    {
        printf("FAIL scan_error\n");
        failures++;
    }

    scan.fail_at = -1;
    if ((nand_cache_layout_build(4096U, 64U, 512U, fake_is_bad,
                                 &scan, &layout) !=
         NAND_CACHE_LAYOUT_UNSUPPORTED_GEOMETRY) ||
        (nand_cache_layout_build(2048U, 128U, 512U, fake_is_bad,
                                 &scan, &layout) !=
         NAND_CACHE_LAYOUT_UNSUPPORTED_GEOMETRY) ||
        (nand_cache_layout_build(2048U, 64U, 512U, NULL,
                                 &scan, &layout) != NAND_CACHE_LAYOUT_INVALID_ARG) ||
        (nand_cache_layout_build(2048U, 64U, 512U, fake_is_bad,
                                 &scan, NULL) != NAND_CACHE_LAYOUT_INVALID_ARG))
    {
        printf("FAIL invalid_and_geometry\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL nand_cache_layout failures=%d\n", failures);
        return 1;
    }
    printf("PASS nand_cache_layout\n");
    return 0;
}
