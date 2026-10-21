#include "file_manager_service.h"

#include <string.h>

file_manager_result_t file_manager_copy_task_start(
    const char *source, const char *destination_dir,
    file_manager_conflict_t conflict);
file_manager_result_t file_manager_copy_task_poll(void);
file_manager_result_t file_manager_copy_task_cancel(void);
void file_manager_copy_task_get_status(file_manager_status_t *status);
file_manager_result_t file_manager_delete_task_start(
    const char *path, uint8_t recursive_confirmed);
file_manager_result_t file_manager_delete_task_poll(void);
file_manager_result_t file_manager_delete_task_cancel(void);
void file_manager_delete_task_get_status(file_manager_status_t *status);
file_manager_result_t file_manager_directory_task_start(
    const char *source, const char *destination_dir,
    file_manager_conflict_t conflict, uint8_t move);
file_manager_result_t file_manager_directory_task_poll(void);
file_manager_result_t file_manager_directory_task_cancel(void);
void file_manager_directory_task_get_status(file_manager_status_t *status);

#ifdef FILE_MANAGER_HOST_TEST
extern file_manager_result_t file_manager_host_session_acquire(void);
extern void file_manager_host_session_release(void);
extern file_manager_result_t file_manager_host_rename(const char *old_path,
                                                      const char *new_path);
extern file_manager_result_t file_manager_host_unlink(const char *path);
#define file_manager_platform_session_acquire file_manager_host_session_acquire
#define file_manager_platform_session_release file_manager_host_session_release
#define file_manager_platform_rename          file_manager_host_rename
#define file_manager_platform_unlink          file_manager_host_unlink
#else
#include "file_manager_platform.h"
#endif

typedef enum
{
    FILE_MANAGER_OPERATION_NONE = 0,
    FILE_MANAGER_OPERATION_COPY,
    FILE_MANAGER_OPERATION_MOVE_COPY,
    FILE_MANAGER_OPERATION_DELETE,
    FILE_MANAGER_OPERATION_DIRECTORY
} file_manager_operation_t;

static file_manager_operation_t g_operation;
static char g_move_source[FILE_MANAGER_PATH_CAPACITY];
static file_manager_status_t g_operation_status;
static uint8_t g_operation_status_valid;

file_manager_result_t file_manager_copy_start(
    const char *source, const char *destination_dir,
    file_manager_conflict_t conflict)
{
    file_manager_result_t result = file_manager_directory_task_start(
        source, destination_dir, conflict, 0U);
    if (result == FILE_MANAGER_OK)
    {
        g_operation = FILE_MANAGER_OPERATION_DIRECTORY;
        g_operation_status_valid = 0U;
        return FILE_MANAGER_OK;
    }
    if (result != FILE_MANAGER_UNSUPPORTED) return result;
    result = file_manager_copy_task_start(source, destination_dir, conflict);
    if (result == FILE_MANAGER_OK)
    {
        g_operation = FILE_MANAGER_OPERATION_COPY;
        g_operation_status_valid = 0U;
    }
    return result;
}

file_manager_result_t file_manager_move_start(
    const char *source, const char *destination_dir,
    file_manager_conflict_t conflict)
{
    char normalized_source[FILE_MANAGER_PATH_CAPACITY];
    char normalized_destination[FILE_MANAGER_PATH_CAPACITY];
    char target[FILE_MANAGER_PATH_CAPACITY];
    const char *name;
    const char *cursor;
    file_manager_result_t result;
    uint8_t directory_started = 0U;

    result = file_manager_normalize_path(source, normalized_source,
                                         sizeof(normalized_source));
    if (result != FILE_MANAGER_OK) return result;
    result = file_manager_normalize_path(destination_dir,
                                         normalized_destination,
                                         sizeof(normalized_destination));
    if (result != FILE_MANAGER_OK) return result;
    name = normalized_source;
    for (cursor = normalized_source; *cursor != '\0'; cursor++)
        if (*cursor == '/') name = cursor + 1;
    result = file_manager_join_path(normalized_destination, name, target,
                                    sizeof(target));
    if (result != FILE_MANAGER_OK) return result;
    result = file_manager_platform_session_acquire();
    if (result != FILE_MANAGER_OK) return result;
    result = file_manager_platform_rename(normalized_source, target);
    file_manager_platform_session_release();
    if (result == FILE_MANAGER_OK)
    {
        memset(&g_operation_status, 0, sizeof(g_operation_status));
        g_operation_status.state = FILE_MANAGER_TASK_DONE;
        g_operation_status.total_entries = 1U;
        g_operation_status.processed_entries = 1U;
        g_operation_status.percent = 100U;
        g_operation_status.result = FILE_MANAGER_OK;
        memcpy(g_operation_status.current_name, name, strlen(name) + 1U);
        g_operation_status_valid = 1U;
        g_operation = FILE_MANAGER_OPERATION_NONE;
        return FILE_MANAGER_OK;
    }
    result = file_manager_copy_task_start(normalized_source,
                                          normalized_destination, conflict);
    if (result == FILE_MANAGER_UNSUPPORTED)
    {
        result = file_manager_directory_task_start(normalized_source,
            normalized_destination, conflict, 1U);
        if (result == FILE_MANAGER_OK) directory_started = 1U;
    }
    if (result != FILE_MANAGER_OK) return result;
    if (directory_started != 0U)
    {
        g_operation = FILE_MANAGER_OPERATION_DIRECTORY;
        g_operation_status_valid = 0U;
        return FILE_MANAGER_OK;
    }
    memcpy(g_move_source, normalized_source, strlen(normalized_source) + 1U);
    g_operation = FILE_MANAGER_OPERATION_MOVE_COPY;
    g_operation_status_valid = 0U;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_poll(void)
{
    file_manager_status_t status;
    file_manager_result_t result;

    if (g_operation == FILE_MANAGER_OPERATION_DELETE)
        return file_manager_delete_task_poll();
    if (g_operation == FILE_MANAGER_OPERATION_DIRECTORY)
        return file_manager_directory_task_poll();
    result = file_manager_copy_task_poll();
    if (result != FILE_MANAGER_OK) return result;
    if (g_operation != FILE_MANAGER_OPERATION_MOVE_COPY) return result;
    file_manager_copy_task_get_status(&status);
    if (status.state == FILE_MANAGER_TASK_DONE)
    {
        result = file_manager_platform_session_acquire();
        if (result != FILE_MANAGER_OK) return result;
        result = file_manager_platform_unlink(g_move_source);
        file_manager_platform_session_release();
        if (result != FILE_MANAGER_OK) return result;
        g_operation = FILE_MANAGER_OPERATION_NONE;
    }
    return result;
}

file_manager_result_t file_manager_delete_start(const char *path,
                                                uint8_t recursive_confirmed)
{
    file_manager_result_t result = file_manager_delete_task_start(
        path, recursive_confirmed);
    if (result == FILE_MANAGER_OK)
    {
        g_operation = FILE_MANAGER_OPERATION_DELETE;
        g_operation_status_valid = 0U;
    }
    return result;
}

file_manager_result_t file_manager_cancel(void)
{
    file_manager_result_t result;

    if (g_operation == FILE_MANAGER_OPERATION_DELETE)
        return file_manager_delete_task_cancel();
    if (g_operation == FILE_MANAGER_OPERATION_DIRECTORY)
        return file_manager_directory_task_cancel();
    result = file_manager_copy_task_cancel();
    if (result == FILE_MANAGER_OK) g_operation = FILE_MANAGER_OPERATION_NONE;
    return result;
}

void file_manager_get_status(file_manager_status_t *status)
{
    if ((status != NULL) && (g_operation_status_valid != 0U))
    {
        *status = g_operation_status;
        return;
    }
    if (g_operation == FILE_MANAGER_OPERATION_DELETE)
    {
        file_manager_delete_task_get_status(status);
        return;
    }
    if (g_operation == FILE_MANAGER_OPERATION_DIRECTORY)
    {
        file_manager_directory_task_get_status(status);
        return;
    }
    file_manager_copy_task_get_status(status);
    if ((status != NULL) &&
        (g_operation == FILE_MANAGER_OPERATION_MOVE_COPY) &&
        (status->state == FILE_MANAGER_TASK_COPYING))
        status->state = FILE_MANAGER_TASK_MOVING;
}
