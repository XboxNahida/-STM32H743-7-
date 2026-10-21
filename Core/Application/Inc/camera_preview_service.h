#ifndef CAMERA_PREVIEW_SERVICE_H
#define CAMERA_PREVIEW_SERVICE_H

#include <stdint.h>

#define CAMERA_PREVIEW_OK 0

typedef enum
{
    CAMERA_PREVIEW_IDLE = 0,
    CAMERA_PREVIEW_STARTING,
    CAMERA_PREVIEW_RUNNING,
    CAMERA_PREVIEW_ERROR
} camera_preview_state_t;

typedef struct
{
    camera_preview_state_t state;
    uint8_t frame_valid;
    uint32_t published_frames;
    uint32_t dropped_frames;
    uint32_t generation;
    uintptr_t rgb_address;
    uint16_t width;
    uint16_t height;
    uint32_t bytes;
    int last_result;
} camera_preview_status_t;

typedef struct
{
    uintptr_t rgb_address;
    uint16_t width;
    uint16_t height;
    uint32_t bytes;
    uint32_t generation;
} camera_preview_frame_t;

int camera_preview_init(void);
int camera_preview_start(void);
int camera_preview_stop(void);
void camera_preview_poll(void);
int camera_preview_get_frame(camera_preview_frame_t *frame);
void camera_preview_get_status(camera_preview_status_t *status);

#endif
