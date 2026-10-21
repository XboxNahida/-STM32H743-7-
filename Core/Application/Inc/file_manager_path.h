#ifndef FILE_MANAGER_PATH_H
#define FILE_MANAGER_PATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_MANAGER_PATH_CAPACITY 512U

typedef enum
{
    FILE_MANAGER_OK = 0,
    FILE_MANAGER_INVALID_ARG,
    FILE_MANAGER_INVALID_NAME,
    FILE_MANAGER_PATH_TOO_LONG,
    FILE_MANAGER_NOT_READY,
    FILE_MANAGER_BUSY,
    FILE_MANAGER_NOT_FOUND,
    FILE_MANAGER_EXISTS,
    FILE_MANAGER_NO_SPACE,
    FILE_MANAGER_IO_ERROR,
    FILE_MANAGER_TOO_DEEP,
    FILE_MANAGER_CANCELLED,
    FILE_MANAGER_UNSUPPORTED
} file_manager_result_t;

typedef enum
{
    FILE_MANAGER_KIND_DIRECTORY = 0,
    FILE_MANAGER_KIND_TEXT,
    FILE_MANAGER_KIND_JPEG,
    FILE_MANAGER_KIND_IMAGE_UNSUPPORTED,
    FILE_MANAGER_KIND_DRAWING_BMP,
    FILE_MANAGER_KIND_AVI,
    FILE_MANAGER_KIND_MP3,
    FILE_MANAGER_KIND_WAV,
    FILE_MANAGER_KIND_NES,
    FILE_MANAGER_KIND_OTHER
} file_manager_kind_t;

file_manager_result_t file_manager_validate_name(const char *name);
file_manager_result_t file_manager_normalize_path(const char *input,
                                                  char *output,
                                                  uint32_t capacity);
file_manager_result_t file_manager_join_path(const char *directory,
                                             const char *name,
                                             char *output,
                                             uint32_t capacity);
file_manager_kind_t file_manager_classify(const char *path);

#ifdef __cplusplus
}
#endif

#endif
