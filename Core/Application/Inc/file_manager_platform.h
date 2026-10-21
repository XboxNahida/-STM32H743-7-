#ifndef FILE_MANAGER_PLATFORM_H
#define FILE_MANAGER_PLATFORM_H

#include <stdint.h>

#include "file_manager_service.h"

#ifdef __cplusplus
extern "C" {
#endif

file_manager_result_t file_manager_platform_session_acquire(void);
void file_manager_platform_session_release(void);
uint8_t *file_manager_platform_transfer_buffer(uint32_t *capacity);
file_manager_result_t file_manager_platform_list_dir_open(const char *path);
file_manager_result_t file_manager_platform_list_dir_read(
    char *name, uint32_t capacity, file_manager_entry_type_t *type,
    uint64_t *size, uint16_t *date, uint16_t *time, uint8_t *end);
void file_manager_platform_list_dir_close(void);
file_manager_result_t file_manager_platform_dir_open(uint32_t depth, const char *path);
file_manager_result_t file_manager_platform_dir_read(uint32_t depth, const char *path,
    char *name, uint32_t capacity, file_manager_entry_type_t *type,
    uint64_t *size, uint8_t *end);
void file_manager_platform_dir_close(uint32_t depth);
file_manager_result_t file_manager_platform_create(const char *path, file_manager_entry_type_t type);
file_manager_result_t file_manager_platform_stat(const char *path, file_manager_entry_type_t *type, uint64_t *size);
file_manager_result_t file_manager_platform_unlink(const char *path);
file_manager_result_t file_manager_platform_rename(const char *old_path, const char *new_path);
file_manager_result_t file_manager_platform_mkdir(const char *path);
file_manager_result_t file_manager_platform_source_open(const char *path);
file_manager_result_t file_manager_platform_source_read(uint8_t *data, uint32_t requested, uint32_t *actual);
void file_manager_platform_source_close(void);
file_manager_result_t file_manager_platform_target_open(const char *path);
file_manager_result_t file_manager_platform_target_write(const uint8_t *data, uint32_t requested, uint32_t *actual);
file_manager_result_t file_manager_platform_target_sync(void);
void file_manager_platform_target_close(void);

#ifdef __cplusplus
}
#endif

#endif
