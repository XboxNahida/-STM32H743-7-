#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdram_layout.h"

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    char *data;
    long size;

    if ((file == NULL) || (fseek(file, 0L, SEEK_END) != 0) ||
        ((size = ftell(file)) < 0L) || (fseek(file, 0L, SEEK_SET) != 0))
    {
        if (file != NULL) fclose(file);
        return NULL;
    }
    data = (char *)malloc((size_t)size + 1U);
    if ((data == NULL) ||
        (fread(data, 1U, (size_t)size, file) != (size_t)size))
    {
        free(data);
        fclose(file);
        return NULL;
    }
    data[size] = '\0';
    fclose(file);
    return data;
}

static int expect(const char *name, int condition)
{
    if (!condition)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *lv_conf = read_file("lvgl/lv_conf.h");
    int failures = 0;

    failures += expect("lvgl pool size", SDRAM_LVGL_POOL_SIZE == 0x00F00000UL);
    failures += expect("lvgl back frame address",
        SDRAM_LVGL_BACK_FRAME_ADDR == 0xC1400000UL);
    failures += expect("lvgl frame size",
        SDRAM_LVGL_FRAME_SIZE == 0x0012C000UL);
    failures += expect("nes work address", SDRAM_NES_WORK_ADDR == 0xC1900000UL);
    failures += expect("nes work size", SDRAM_NES_WORK_SIZE == 0x00400000UL);
    failures += expect("pool meets back frame",
        SDRAM_LVGL_POOL_END == SDRAM_LVGL_BACK_FRAME_ADDR);
    failures += expect("back frame before nes",
        SDRAM_LVGL_BACK_FRAME_END <= SDRAM_NES_WORK_ADDR);
    failures += expect("nes meets video", SDRAM_NES_WORK_END == SDRAM_VIDEO_RGB_A_ADDR);
    failures += expect("lv conf open", lv_conf != NULL);
    failures += expect("lv conf matches reduced pool",
        (lv_conf != NULL) &&
        (strstr(lv_conf, "LV_MEM_SIZE (15 * 1024 * 1024U)") != NULL));
    free(lv_conf);
    if (failures == 0)
    {
        printf("PASS nes_sdram_layout_contract\n");
    }
    return failures;
}
