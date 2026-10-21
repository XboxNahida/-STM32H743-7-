#ifndef FILE_MANAGER_SERVICE_H
#define FILE_MANAGER_SERVICE_H

#include <stdint.h>

#include "file_manager_path.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_MANAGER_PAGE_SIZE       16U
#define FILE_MANAGER_NAME_CAPACITY   256U

typedef enum
{
    FILE_MANAGER_ENTRY_FILE = 0,
    FILE_MANAGER_ENTRY_DIRECTORY = 1
} file_manager_entry_type_t;

typedef struct
{
    char name[FILE_MANAGER_NAME_CAPACITY];
    file_manager_entry_type_t type;
    uint64_t size;
    uint16_t date;
    uint16_t time;
} file_manager_entry_t;

typedef struct
{
    file_manager_entry_t entries[FILE_MANAGER_PAGE_SIZE];
    uint16_t count;
    uint8_t has_more;
    uint32_t next_offset;
} file_manager_page_t;

typedef enum
{
    FILE_MANAGER_CONFLICT_OVERWRITE = 0,
    FILE_MANAGER_CONFLICT_RENAME,
    FILE_MANAGER_CONFLICT_CANCEL
} file_manager_conflict_t;

typedef enum
{
    FILE_MANAGER_TASK_IDLE = 0,
    FILE_MANAGER_TASK_SCANNING,
    FILE_MANAGER_TASK_COPYING,
    FILE_MANAGER_TASK_MOVING,
    FILE_MANAGER_TASK_DELETING,
    FILE_MANAGER_TASK_PAUSED,
    FILE_MANAGER_TASK_DONE,
    FILE_MANAGER_TASK_CANCELLED,
    FILE_MANAGER_TASK_ERROR
} file_manager_task_state_t;

typedef struct
{
    file_manager_task_state_t state;
    uint64_t total_bytes;
    uint64_t processed_bytes;
    uint32_t total_entries;
    uint32_t processed_entries;
    uint8_t percent;
    int result;
    char current_name[FILE_MANAGER_NAME_CAPACITY];
} file_manager_status_t;

file_manager_result_t file_manager_list_page(const char *path,
                                             uint32_t offset,
                                             file_manager_page_t *page);
file_manager_result_t file_manager_create(const char *directory,
                                          const char *name,
                                          file_manager_entry_type_t type);
file_manager_result_t file_manager_copy_start(
    const char *source, const char *destination_dir,
    file_manager_conflict_t conflict);
file_manager_result_t file_manager_move_start(
    const char *source, const char *destination_dir,
    file_manager_conflict_t conflict);
file_manager_result_t file_manager_delete_start(const char *path,
                                                uint8_t recursive_confirmed);
file_manager_result_t file_manager_poll(void);
file_manager_result_t file_manager_cancel(void);
void file_manager_get_status(file_manager_status_t *status);

#ifdef __cplusplus
}
#endif

#endif
