#ifndef SDRAM_LAYOUT_H
#define SDRAM_LAYOUT_H

#define SDRAM_BASE_ADDR                    0xC0000000UL
#define SDRAM_END_ADDR                     0xC2000000UL
#define SDRAM_DMA_ALIGNMENT                32UL

#define SDRAM_LVGL_FRAME_ADDR              0xC0000000UL
#define SDRAM_LVGL_FRAME_SIZE              0x0012C000UL
#define SDRAM_LVGL_FRAME_END               (SDRAM_LVGL_FRAME_ADDR + SDRAM_LVGL_FRAME_SIZE)
#define SDRAM_SHARED_WORK_ADDR             0xC0460000UL
#define SDRAM_SHARED_WORK_SIZE             0x00080000UL
#define SDRAM_SHARED_WORK_END              (SDRAM_SHARED_WORK_ADDR + SDRAM_SHARED_WORK_SIZE)
#define SDRAM_LVGL_POOL_ADDR               0xC0500000UL
#define SDRAM_LVGL_POOL_SIZE               0x00F00000UL
#define SDRAM_LVGL_POOL_END                (SDRAM_LVGL_POOL_ADDR + SDRAM_LVGL_POOL_SIZE)
#define SDRAM_LVGL_BACK_FRAME_ADDR         SDRAM_LVGL_POOL_END
#define SDRAM_LVGL_BACK_FRAME_END          (SDRAM_LVGL_BACK_FRAME_ADDR + SDRAM_LVGL_FRAME_SIZE)
#define SDRAM_FILE_TASK_ADDR               0xC1540000UL
#define SDRAM_FILE_TASK_SIZE               0x00010000UL
#define SDRAM_FILE_TASK_END                (SDRAM_FILE_TASK_ADDR + SDRAM_FILE_TASK_SIZE)
#define SDRAM_NES_WORK_ADDR                0xC1900000UL
#define SDRAM_NES_WORK_SIZE                0x00400000UL
#define SDRAM_NES_WORK_END                 (SDRAM_NES_WORK_ADDR + SDRAM_NES_WORK_SIZE)
#define SDRAM_VIDEO_RGB_A_ADDR             0xC1D00000UL
#define SDRAM_VIDEO_RGB_B_ADDR             0xC1E30000UL
#define SDRAM_VIDEO_RGB_FRAME_SIZE         0x0012C000UL
#define SDRAM_VIDEO_YUV0_ADDR              0xC1F60000UL
#define SDRAM_VIDEO_YUV1_ADDR              0xC1F70000UL
#define SDRAM_VIDEO_YUV_BUFFER_SIZE        0x00010000UL
#define SDRAM_VIDEO_RESERVED_ADDR          0xC1F80000UL

#if (SDRAM_SHARED_WORK_ADDR < SDRAM_BASE_ADDR)
#error "Shared workspace starts before SDRAM"
#endif

#if (SDRAM_LVGL_FRAME_END > SDRAM_SHARED_WORK_ADDR)
#error "LVGL front frame overlaps shared workspace"
#endif

#if (SDRAM_SHARED_WORK_END > SDRAM_LVGL_POOL_ADDR)
#error "Shared workspace overlaps LVGL pool"
#endif

#if (SDRAM_LVGL_POOL_END > SDRAM_LVGL_BACK_FRAME_ADDR)
#error "LVGL pool overlaps back frame"
#endif

#if (SDRAM_LVGL_BACK_FRAME_END > SDRAM_FILE_TASK_ADDR)
#error "LVGL back frame overlaps file task workspace"
#endif

#if (SDRAM_FILE_TASK_END > SDRAM_NES_WORK_ADDR)
#error "File task workspace overlaps NES workspace"
#endif

#if (SDRAM_NES_WORK_END > SDRAM_VIDEO_RGB_A_ADDR)
#error "NES workspace overlaps video RGB A"
#endif

#if ((SDRAM_SHARED_WORK_ADDR % SDRAM_DMA_ALIGNMENT) != 0UL)
#error "Shared workspace is not DMA aligned"
#endif

#if ((SDRAM_LVGL_POOL_ADDR % SDRAM_DMA_ALIGNMENT) != 0UL)
#error "LVGL pool is not DMA aligned"
#endif

#if ((SDRAM_LVGL_FRAME_ADDR % SDRAM_DMA_ALIGNMENT) != 0UL) || \
    ((SDRAM_LVGL_BACK_FRAME_ADDR % SDRAM_DMA_ALIGNMENT) != 0UL)
#error "LVGL frame buffer is not DMA aligned"
#endif

#if ((SDRAM_NES_WORK_ADDR % SDRAM_DMA_ALIGNMENT) != 0UL)
#error "NES workspace is not aligned"
#endif

#if ((SDRAM_FILE_TASK_ADDR % SDRAM_DMA_ALIGNMENT) != 0UL)
#error "File task workspace is not aligned"
#endif

#if ((SDRAM_VIDEO_RGB_A_ADDR % SDRAM_DMA_ALIGNMENT) != 0UL) || \
    ((SDRAM_VIDEO_RGB_B_ADDR % SDRAM_DMA_ALIGNMENT) != 0UL)
#error "Video RGB buffer is not DMA aligned"
#endif

#if ((SDRAM_VIDEO_YUV0_ADDR % SDRAM_DMA_ALIGNMENT) != 0UL) || \
    ((SDRAM_VIDEO_YUV1_ADDR % SDRAM_DMA_ALIGNMENT) != 0UL)
#error "Video YUV buffer is not DMA aligned"
#endif

#if ((SDRAM_VIDEO_RGB_A_ADDR + SDRAM_VIDEO_RGB_FRAME_SIZE) > SDRAM_VIDEO_RGB_B_ADDR)
#error "Video RGB A overlaps RGB B"
#endif

#if ((SDRAM_VIDEO_RGB_B_ADDR + SDRAM_VIDEO_RGB_FRAME_SIZE) > SDRAM_VIDEO_YUV0_ADDR)
#error "Video RGB B overlaps YUV0"
#endif

#if ((SDRAM_VIDEO_YUV0_ADDR + SDRAM_VIDEO_YUV_BUFFER_SIZE) > SDRAM_VIDEO_YUV1_ADDR)
#error "Video YUV0 overlaps YUV1"
#endif

#if ((SDRAM_VIDEO_YUV1_ADDR + SDRAM_VIDEO_YUV_BUFFER_SIZE) > SDRAM_VIDEO_RESERVED_ADDR)
#error "Video YUV1 overlaps reserved SDRAM"
#endif

#if (SDRAM_VIDEO_RESERVED_ADDR >= SDRAM_END_ADDR)
#error "Video workspace exceeds SDRAM"
#endif

#endif
