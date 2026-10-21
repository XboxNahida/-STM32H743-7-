#ifndef CAMERA_DCMI_DRIVER_H
#define CAMERA_DCMI_DRIVER_H
#include <stdint.h>
#define CAMERA_DCMI_WIDTH       600U
#define CAMERA_DCMI_HEIGHT      400U
#define CAMERA_DCMI_FRAME_WORDS \
    ((CAMERA_DCMI_WIDTH * CAMERA_DCMI_HEIGHT * 2U) / 4U)
#define CAMERA_DCMI_FRAME_BYTES (CAMERA_DCMI_FRAME_WORDS * 4U)
#define CAMERA_DCMI_CACHE_LINE  32U
#define CAMERA_DCMI_LINE_WORDS  (CAMERA_DCMI_WIDTH / 2U)
#define CAMERA_DCMI_LINE_BYTES  (CAMERA_DCMI_WIDTH * 2U)
#define CAMERA_DCMI_CHUNK_LINES 40U
#define CAMERA_DCMI_CHUNK_BYTES \
    (CAMERA_DCMI_LINE_BYTES * CAMERA_DCMI_CHUNK_LINES)
#define CAMERA_DCMI_CHUNK_WORDS (CAMERA_DCMI_CHUNK_BYTES / 4U)
#define CAMERA_DCMI_CHUNKS_PER_FRAME \
    (CAMERA_DCMI_HEIGHT / CAMERA_DCMI_CHUNK_LINES)

#if ((CAMERA_DCMI_HEIGHT % CAMERA_DCMI_CHUNK_LINES) != 0U)
#error "Camera height must contain whole DMA chunks"
#endif

#if ((CAMERA_DCMI_CHUNK_BYTES % CAMERA_DCMI_CACHE_LINE) != 0U)
#error "Camera DMA chunks must be cache-line aligned"
#endif

#define CAMERA_DCMI_EVENT_FRAME    0x04U
#define CAMERA_DCMI_EVENT_ERROR    0x08U
typedef enum
{
    CAMERA_DCMI_OK = 0,
    CAMERA_DCMI_INVALID_ARG,
    CAMERA_DCMI_NOT_READY,
    CAMERA_DCMI_BUSY,
    CAMERA_DCMI_HAL_ERROR
} camera_dcmi_result_t;
typedef enum
{
    CAMERA_DCMI_BUSINESS_ERROR_NONE = 0,
    CAMERA_DCMI_BUSINESS_ERROR_INVALID_BUFFER,
    CAMERA_DCMI_BUSINESS_ERROR_COPY_BUSY,
    CAMERA_DCMI_BUSINESS_ERROR_COPY_FAILED
} camera_dcmi_business_error_t;
typedef struct
{
    uint8_t initialized;
    uint8_t running;
    uint32_t frame_events;
    uint32_t error_events;
    uint32_t dma_errors;
    uint32_t extra_tc_events;
    uint32_t business_error_code;
    uint32_t frame_words;
    uint32_t frame_bytes;
    uint32_t cache_line;
    uint32_t dcmi_error_code;
    uint32_t dma_error_code;
    uint32_t dcmi_risr;
    uint32_t dma_lisr;
    uint32_t line_count;
    uint32_t frame_seen;
    uint32_t dcmi_cr;
    uint32_t dcmi_sr;
    uint32_t dma_ndtr;
    uint32_t dma_cr;
    int last_result;
} camera_dcmi_status_t;
int camera_dcmi_init(void);
int camera_dcmi_start(void);
int camera_dcmi_stop(void);
int camera_dcmi_poll_event(uint32_t *events);
void camera_dcmi_get_status(camera_dcmi_status_t *status);

void camera_dcmi_frame_callback(void);
void camera_dcmi_chunk_callback(uint8_t completed_buffer);
void camera_dcmi_error_callback(void);
#endif
