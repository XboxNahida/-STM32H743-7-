#include "nes_video.h"

#include <stddef.h>
#include <string.h>

typedef struct
{
    nes_video_status_t status;
    uint16_t *buffers[2];
    uint32_t buffer_generation[2];
    uint8_t render_index;
    nes_video_backend_ops_t direct_ops;
    void *direct_context;
} nes_video_context_t;

static nes_video_context_t g_nes_video;

int nes_video_init(uint16_t *buffer_a,
                   uint16_t *buffer_b,
                   uint32_t pixels_per_buffer)
{
    if ((buffer_a == NULL) || (buffer_b == NULL) ||
        (buffer_a == buffer_b) ||
        (pixels_per_buffer < NES_VIDEO_PIXEL_COUNT))
    {
        return NES_VIDEO_INVALID_ARG;
    }

    nes_video_deinit();
    memset(&g_nes_video, 0, sizeof(g_nes_video));
    g_nes_video.buffers[0] = buffer_a;
    g_nes_video.buffers[1] = buffer_b;
    g_nes_video.render_index = 0U;
    g_nes_video.status.initialized = 1U;
    g_nes_video.status.output_mode = NES_VIDEO_OUTPUT_LVGL_OVERLAY;
    return NES_VIDEO_OK;
}

void nes_video_deinit(void)
{
    if ((g_nes_video.status.initialized != 0U) &&
        (g_nes_video.status.output_mode == NES_VIDEO_OUTPUT_DIRECT) &&
        (g_nes_video.status.direct_registered != 0U))
    {
        g_nes_video.direct_ops.stop(g_nes_video.direct_context);
    }
    memset(&g_nes_video, 0, sizeof(g_nes_video));
}

int nes_video_acquire_render_buffer(uint16_t **pixels)
{
    if (pixels == NULL)
    {
        return NES_VIDEO_INVALID_ARG;
    }
    if (g_nes_video.status.initialized == 0U)
    {
        return NES_VIDEO_NOT_READY;
    }
    if (g_nes_video.status.render_acquired != 0U)
    {
        return NES_VIDEO_BUSY;
    }
    if (g_nes_video.buffer_generation[g_nes_video.render_index] >
        g_nes_video.status.consumed_generation)
    {
        return NES_VIDEO_BUSY;
    }

    *pixels = g_nes_video.buffers[g_nes_video.render_index];
    g_nes_video.status.render_acquired = 1U;
    return NES_VIDEO_OK;
}

static void nes_video_fill_frame(nes_video_frame_t *frame)
{
    uint8_t front_index = (uint8_t)(g_nes_video.render_index ^ 1U);

    frame->pixels = g_nes_video.buffers[front_index];
    frame->width = NES_VIDEO_WIDTH;
    frame->height = NES_VIDEO_HEIGHT;
    frame->stride_pixels = NES_VIDEO_WIDTH;
    frame->generation = g_nes_video.status.generation;
}

int nes_video_publish(void)
{
    nes_video_frame_t frame;
    int backend_result;

    if (g_nes_video.status.initialized == 0U)
    {
        return NES_VIDEO_NOT_READY;
    }
    if (g_nes_video.status.render_acquired == 0U)
    {
        return NES_VIDEO_NOT_READY;
    }

    g_nes_video.status.render_acquired = 0U;
    g_nes_video.status.generation++;
    g_nes_video.status.published_frames++;
    g_nes_video.status.frame_valid = 1U;
    g_nes_video.buffer_generation[g_nes_video.render_index] =
        g_nes_video.status.generation;
    g_nes_video.render_index ^= 1U;

    if (g_nes_video.status.output_mode != NES_VIDEO_OUTPUT_DIRECT)
    {
        return NES_VIDEO_OK;
    }

    nes_video_fill_frame(&frame);
    backend_result = g_nes_video.direct_ops.present(
        g_nes_video.direct_context, &frame);
    if (backend_result != 0)
    {
        g_nes_video.status.dropped_frames++;
        return NES_VIDEO_BACKEND_ERROR;
    }
    g_nes_video.status.consumed_generation =
        g_nes_video.status.generation;
    return NES_VIDEO_OK;
}

int nes_video_get_frame(nes_video_frame_t *frame)
{
    if (frame == NULL)
    {
        return NES_VIDEO_INVALID_ARG;
    }
    if ((g_nes_video.status.initialized == 0U) ||
        (g_nes_video.status.frame_valid == 0U))
    {
        return NES_VIDEO_NOT_READY;
    }
    nes_video_fill_frame(frame);
    return NES_VIDEO_OK;
}

int nes_video_mark_consumed(uint32_t generation)
{
    if ((g_nes_video.status.initialized == 0U) ||
        (g_nes_video.status.frame_valid == 0U))
    {
        return NES_VIDEO_NOT_READY;
    }
    if ((generation == 0U) ||
        (generation > g_nes_video.status.generation))
    {
        return NES_VIDEO_INVALID_ARG;
    }
    if (generation > g_nes_video.status.consumed_generation)
    {
        g_nes_video.status.consumed_generation = generation;
    }
    return NES_VIDEO_OK;
}

int nes_video_register_direct_backend(const nes_video_backend_ops_t *ops,
                                      void *context)
{
    if ((ops == NULL) || (ops->start == NULL) ||
        (ops->present == NULL) || (ops->stop == NULL))
    {
        return NES_VIDEO_INVALID_ARG;
    }
    if (g_nes_video.status.initialized == 0U)
    {
        return NES_VIDEO_NOT_READY;
    }
    if (g_nes_video.status.output_mode == NES_VIDEO_OUTPUT_DIRECT)
    {
        return NES_VIDEO_BUSY;
    }

    g_nes_video.direct_ops = *ops;
    g_nes_video.direct_context = context;
    g_nes_video.status.direct_registered = 1U;
    return NES_VIDEO_OK;
}

int nes_video_set_output_mode(nes_video_output_mode_t mode)
{
    if ((mode != NES_VIDEO_OUTPUT_LVGL_OVERLAY) &&
        (mode != NES_VIDEO_OUTPUT_DIRECT))
    {
        return NES_VIDEO_INVALID_ARG;
    }
    if (g_nes_video.status.initialized == 0U)
    {
        return NES_VIDEO_NOT_READY;
    }
    if (mode == g_nes_video.status.output_mode)
    {
        return NES_VIDEO_OK;
    }

    if (mode == NES_VIDEO_OUTPUT_DIRECT)
    {
        if (g_nes_video.status.direct_registered == 0U)
        {
            return NES_VIDEO_NOT_READY;
        }
        if (g_nes_video.direct_ops.start(g_nes_video.direct_context) != 0)
        {
            return NES_VIDEO_BACKEND_ERROR;
        }
        g_nes_video.status.consumed_generation =
            g_nes_video.status.generation;
        g_nes_video.status.output_mode = NES_VIDEO_OUTPUT_DIRECT;
        return NES_VIDEO_OK;
    }

    g_nes_video.direct_ops.stop(g_nes_video.direct_context);
    g_nes_video.status.output_mode = NES_VIDEO_OUTPUT_LVGL_OVERLAY;
    return NES_VIDEO_OK;
}

int nes_video_should_use_direct(uint32_t max_pending_frames)
{
    uint32_t pending_frames;

    if ((g_nes_video.status.initialized == 0U) ||
        (g_nes_video.status.direct_registered == 0U) ||
        (g_nes_video.status.output_mode != NES_VIDEO_OUTPUT_LVGL_OVERLAY))
    {
        return 0;
    }
    pending_frames = g_nes_video.status.generation -
                     g_nes_video.status.consumed_generation;
    return (pending_frames > max_pending_frames) ? 1 : 0;
}

void nes_video_get_status(nes_video_status_t *status)
{
    if (status != NULL)
    {
        *status = g_nes_video.status;
    }
}
