#ifndef IMAGE_VIEWER_APP_H
#define IMAGE_VIEWER_APP_H

#include <stdint.h>

#define IMAGE_VIEWER_PATH_SIZE 256U
#define IMAGE_VIEWER_MAX_FILE_SIZE (512U * 1024U)
#define IMAGE_VIEWER_READ_CHUNK_SIZE (32U * 1024U)

typedef enum
{
    IMAGE_VIEWER_IDLE = 0,
    IMAGE_VIEWER_SCANNING,
    IMAGE_VIEWER_OPENING,
    IMAGE_VIEWER_READING,
    IMAGE_VIEWER_DECODING,
    IMAGE_VIEWER_READY,
    IMAGE_VIEWER_STOPPING,
    IMAGE_VIEWER_ERROR
} image_viewer_state_t;

typedef struct
{
    image_viewer_state_t state;
    int32_t result;
    uint32_t file_size;
    uint32_t bytes_read;
    uint16_t width;
    uint16_t height;
    uint32_t rgb_address;
    uint32_t rgb_stride;
    const char *display_name;
} image_viewer_status_t;

typedef enum
{
    IMAGE_VIEWER_OK = 0,
    IMAGE_VIEWER_INVALID_ARG,
    IMAGE_VIEWER_BUSY,
    IMAGE_VIEWER_NOT_READY,
    IMAGE_VIEWER_PATH_TOO_LONG,
    IMAGE_VIEWER_FILE_TOO_LARGE,
    IMAGE_VIEWER_IO_ERROR,
    IMAGE_VIEWER_UNSUPPORTED,
    IMAGE_VIEWER_DECODE_ERROR,
    IMAGE_VIEWER_EMPTY_ALBUM
} image_viewer_result_t;

void image_viewer_app_init(void);
int image_viewer_app_open_album(const char *directory);
int image_viewer_app_open_file(const char *path);
int image_viewer_app_next(void);
int image_viewer_app_previous(void);
int image_viewer_app_stop(void);
void image_viewer_app_poll(void);
void image_viewer_app_get_status(image_viewer_status_t *status);
void image_viewer_app_print_status(void);

#endif
