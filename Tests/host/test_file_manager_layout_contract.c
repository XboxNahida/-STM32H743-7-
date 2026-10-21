#include <stdint.h>
#include <stdio.h>

#include "sdram_layout.h"

int main(void)
{
    int failures = 0;

    if (SDRAM_FILE_TASK_SIZE != 65536UL) failures++;
    if ((SDRAM_FILE_TASK_ADDR % SDRAM_DMA_ALIGNMENT) != 0UL) failures++;
    if (SDRAM_FILE_TASK_ADDR < SDRAM_LVGL_BACK_FRAME_END) failures++;
    if (SDRAM_FILE_TASK_END > SDRAM_NES_WORK_ADDR) failures++;
    if ((SDRAM_FILE_TASK_END - SDRAM_FILE_TASK_ADDR) !=
        SDRAM_FILE_TASK_SIZE) failures++;
    if (failures != 0)
    {
        printf("FAIL file_manager_layout_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS file_manager_layout_contract\n");
    return 0;
}
