#ifndef NES_VIDEO_H
#define NES_VIDEO_H

#include <stdint.h>

#define NES_VIDEO_WIDTH       512U
#define NES_VIDEO_HEIGHT      480U
#define NES_VIDEO_PIXEL_COUNT ((uint32_t)NES_VIDEO_WIDTH * NES_VIDEO_HEIGHT)

typedef enum
{
    NES_VIDEO_OK = 0,
    NES_VIDEO_INVALID_ARG,
    NES_VIDEO_BUSY,
    NES_VIDEO_NOT_READY,
    NES_VIDEO_BACKEND_ERROR
} nes_video_result_t;

typedef enum
{
    NES_VIDEO_OUTPUT_LVGL_OVERLAY = 0,
    NES_VIDEO_OUTPUT_DIRECT
} nes_video_output_mode_t;

typedef struct
{
    const uint16_t *pixels;
    uint16_t width;
    uint16_t height;
    uint16_t stride_pixels;
    uint32_t generation;
} nes_video_frame_t;

typedef struct
{
    int (*start)(void *context);
    int (*present)(void *context, const nes_video_frame_t *frame);
    void (*stop)(void *context);
} nes_video_backend_ops_t;

typedef struct
{
    uint8_t initialized;
    uint8_t frame_valid;
    uint8_t render_acquired;
    uint8_t direct_registered;
    nes_video_output_mode_t output_mode;
    uint32_t generation;
    uint32_t consumed_generation;
    uint32_t published_frames;
    uint32_t dropped_frames;
} nes_video_status_t;

int nes_video_init(uint16_t *buffer_a,
                   uint16_t *buffer_b,
                   uint32_t pixels_per_buffer);
void nes_video_deinit(void);
int nes_video_acquire_render_buffer(uint16_t **pixels);
int nes_video_publish(void);
int nes_video_get_frame(nes_video_frame_t *frame);
int nes_video_mark_consumed(uint32_t generation);
int nes_video_register_direct_backend(const nes_video_backend_ops_t *ops,
                                      void *context);
int nes_video_set_output_mode(nes_video_output_mode_t mode);
int nes_video_should_use_direct(uint32_t max_pending_frames);
void nes_video_get_status(nes_video_status_t *status);

#endif
