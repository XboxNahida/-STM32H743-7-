#include "file_manager_service.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "file_manager_tree.h"

#ifdef FILE_MANAGER_HOST_TEST
extern file_manager_result_t file_manager_host_session_acquire(void);
extern void file_manager_host_session_release(void);
extern file_manager_result_t file_manager_host_stat(
    const char *path, file_manager_entry_type_t *type, uint64_t *size);
#define file_manager_platform_session_acquire file_manager_host_session_acquire
#define file_manager_platform_session_release file_manager_host_session_release
#define file_manager_platform_stat            file_manager_host_stat
#else
#include "file_manager_platform.h"
#endif

typedef enum
{
    FILE_MANAGER_DIRECTORY_IDLE = 0,
    FILE_MANAGER_DIRECTORY_DELETE_TARGET,
    FILE_MANAGER_DIRECTORY_SCAN,
    FILE_MANAGER_DIRECTORY_COPY,
    FILE_MANAGER_DIRECTORY_DELETE_SOURCE
} file_manager_directory_phase_t;

typedef struct
{
    file_manager_status_t status;
    file_manager_status_t cleanup_status;
    file_manager_directory_phase_t phase;
    char source[FILE_MANAGER_PATH_CAPACITY];
    char target[FILE_MANAGER_PATH_CAPACITY];
    uint8_t move;
    uint8_t session_held;
} file_manager_directory_context_t;

static file_manager_directory_context_t g_directory;

file_manager_result_t file_manager_delete_task_start(
    const char *path, uint8_t recursive_confirmed);
file_manager_result_t file_manager_delete_task_poll(void);
void file_manager_delete_task_get_status(file_manager_status_t *status);

static const char *file_manager_directory_basename(const char *path)
{
    const char *name = path;
    const char *cursor;
    for (cursor = path; *cursor != '\0'; cursor++)
        if (*cursor == '/') name = cursor + 1;
    return name;
}

static void file_manager_directory_release(void)
{
    if (file_manager_tree_active() != 0U) file_manager_tree_abort();
    if (g_directory.session_held != 0U)
    {
        file_manager_platform_session_release();
        g_directory.session_held = 0U;
    }
    g_directory.phase = FILE_MANAGER_DIRECTORY_IDLE;
}

static file_manager_result_t file_manager_directory_fail(
    file_manager_result_t result)
{
    file_manager_directory_release();
    g_directory.status.state = FILE_MANAGER_TASK_ERROR;
    g_directory.status.result = result;
    return result;
}

static file_manager_result_t file_manager_directory_begin_scan(void)
{
    file_manager_result_t result = file_manager_tree_begin(
        FILE_MANAGER_TREE_SCAN, g_directory.source, NULL);
    if (result != FILE_MANAGER_OK) return result;
    memset(&g_directory.status, 0, sizeof(g_directory.status));
    g_directory.status.state = FILE_MANAGER_TASK_SCANNING;
    g_directory.status.result = FILE_MANAGER_OK;
    g_directory.phase = FILE_MANAGER_DIRECTORY_SCAN;
    return FILE_MANAGER_OK;
}

static file_manager_result_t file_manager_directory_select_renamed(
    const char *destination, const char *name)
{
    uint32_t index;
    for (index = 1U; index <= 9999U; index++)
    {
        char candidate[FILE_MANAGER_NAME_CAPACITY];
        file_manager_entry_type_t type;
        uint64_t size;
        int length = snprintf(candidate, sizeof(candidate), "%s (%lu)",
                              name, (unsigned long)index);
        file_manager_result_t result;
        if ((length < 0) || ((uint32_t)length >= sizeof(candidate)))
            return FILE_MANAGER_PATH_TOO_LONG;
        result = file_manager_join_path(destination, candidate,
                                        g_directory.target,
                                        sizeof(g_directory.target));
        if (result != FILE_MANAGER_OK) return result;
        result = file_manager_platform_stat(g_directory.target, &type, &size);
        if (result == FILE_MANAGER_NOT_FOUND) return FILE_MANAGER_OK;
        if (result != FILE_MANAGER_OK) return result;
    }
    return FILE_MANAGER_EXISTS;
}

file_manager_result_t file_manager_directory_task_start(
    const char *source, const char *destination_dir,
    file_manager_conflict_t conflict, uint8_t move)
{
    char destination[FILE_MANAGER_PATH_CAPACITY];
    const char *name;
    file_manager_entry_type_t type;
    uint64_t size;
    size_t source_length;
    file_manager_result_t result;

    if ((source == NULL) || (destination_dir == NULL) ||
        (conflict > FILE_MANAGER_CONFLICT_CANCEL)) return FILE_MANAGER_INVALID_ARG;
    if (g_directory.phase != FILE_MANAGER_DIRECTORY_IDLE) return FILE_MANAGER_BUSY;
    memset(&g_directory, 0, sizeof(g_directory));
    result = file_manager_normalize_path(source, g_directory.source,
                                         sizeof(g_directory.source));
    if (result != FILE_MANAGER_OK) return result;
    result = file_manager_normalize_path(destination_dir, destination,
                                         sizeof(destination));
    if (result != FILE_MANAGER_OK) return result;
    result = file_manager_platform_session_acquire();
    if (result != FILE_MANAGER_OK) return result;
    g_directory.session_held = 1U;
    result = file_manager_platform_stat(g_directory.source, &type, &size);
    if (result != FILE_MANAGER_OK) return file_manager_directory_fail(result);
    if (type != FILE_MANAGER_ENTRY_DIRECTORY)
    {
        file_manager_directory_release();
        return FILE_MANAGER_UNSUPPORTED;
    }
    name = file_manager_directory_basename(g_directory.source);
    result = file_manager_join_path(destination, name, g_directory.target,
                                    sizeof(g_directory.target));
    if (result != FILE_MANAGER_OK) return file_manager_directory_fail(result);
    source_length = strlen(g_directory.source);
    if ((strcmp(g_directory.source, g_directory.target) == 0) ||
        ((strncmp(g_directory.source, g_directory.target, source_length) == 0) &&
         (g_directory.target[source_length] == '/')))
        return file_manager_directory_fail(FILE_MANAGER_INVALID_ARG);
    g_directory.move = move;
    memcpy(g_directory.status.current_name, name, strlen(name) + 1U);
    result = file_manager_platform_stat(g_directory.target, &type, &size);
    if (result == FILE_MANAGER_OK)
    {
        if (conflict == FILE_MANAGER_CONFLICT_CANCEL)
        {
            file_manager_directory_release();
            return FILE_MANAGER_CANCELLED;
        }
        if (conflict == FILE_MANAGER_CONFLICT_RENAME)
        {
            result = file_manager_directory_select_renamed(destination, name);
            if (result != FILE_MANAGER_OK) return file_manager_directory_fail(result);
        }
        else
        {
            result = file_manager_delete_task_start(g_directory.target, 1U);
            if (result != FILE_MANAGER_OK) return file_manager_directory_fail(result);
            g_directory.phase = FILE_MANAGER_DIRECTORY_DELETE_TARGET;
            g_directory.status.state = FILE_MANAGER_TASK_DELETING;
            return FILE_MANAGER_OK;
        }
    }
    else if (result != FILE_MANAGER_NOT_FOUND)
        return file_manager_directory_fail(result);
    result = file_manager_directory_begin_scan();
    return (result == FILE_MANAGER_OK) ? result : file_manager_directory_fail(result);
}

file_manager_result_t file_manager_directory_task_poll(void)
{
    file_manager_result_t result;

    if (g_directory.phase == FILE_MANAGER_DIRECTORY_IDLE) return FILE_MANAGER_OK;
    if (g_directory.phase == FILE_MANAGER_DIRECTORY_DELETE_TARGET)
    {
        result = file_manager_delete_task_poll();
        if (result != FILE_MANAGER_OK) return file_manager_directory_fail(result);
        file_manager_delete_task_get_status(&g_directory.cleanup_status);
        if (g_directory.cleanup_status.state != FILE_MANAGER_TASK_DONE)
            return FILE_MANAGER_OK;
        result = file_manager_directory_begin_scan();
        return (result == FILE_MANAGER_OK) ? result : file_manager_directory_fail(result);
    }
    if (g_directory.phase == FILE_MANAGER_DIRECTORY_SCAN)
    {
        result = file_manager_tree_step(&g_directory.status);
        if (result != FILE_MANAGER_OK) return file_manager_directory_fail(result);
        if (file_manager_tree_active() != 0U) return FILE_MANAGER_OK;
        result = file_manager_tree_begin(FILE_MANAGER_TREE_COPY,
            g_directory.source, g_directory.target);
        if (result != FILE_MANAGER_OK) return file_manager_directory_fail(result);
        g_directory.phase = FILE_MANAGER_DIRECTORY_COPY;
        g_directory.status.state = g_directory.move ?
            FILE_MANAGER_TASK_MOVING : FILE_MANAGER_TASK_COPYING;
        return FILE_MANAGER_OK;
    }
    if (g_directory.phase == FILE_MANAGER_DIRECTORY_COPY)
    {
        result = file_manager_tree_step(&g_directory.status);
        if (result != FILE_MANAGER_OK) return file_manager_directory_fail(result);
        if (g_directory.status.total_bytes != 0U)
            g_directory.status.percent = (uint8_t)(
                (g_directory.status.processed_bytes * 100ULL) /
                g_directory.status.total_bytes);
        if (file_manager_tree_active() != 0U) return FILE_MANAGER_OK;
        if (g_directory.move == 0U)
        {
            file_manager_directory_release();
            g_directory.status.percent = 100U;
            g_directory.status.state = FILE_MANAGER_TASK_DONE;
            return FILE_MANAGER_OK;
        }
        memset(&g_directory.cleanup_status, 0, sizeof(g_directory.cleanup_status));
        result = file_manager_tree_begin(FILE_MANAGER_TREE_DELETE,
                                         g_directory.source, NULL);
        if (result != FILE_MANAGER_OK) return file_manager_directory_fail(result);
        g_directory.phase = FILE_MANAGER_DIRECTORY_DELETE_SOURCE;
        return FILE_MANAGER_OK;
    }
    result = file_manager_tree_step(&g_directory.cleanup_status);
    if (result != FILE_MANAGER_OK) return file_manager_directory_fail(result);
    if (file_manager_tree_active() == 0U)
    {
        file_manager_directory_release();
        g_directory.status.percent = 100U;
        g_directory.status.state = FILE_MANAGER_TASK_DONE;
    }
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_directory_task_cancel(void)
{
    if (g_directory.phase == FILE_MANAGER_DIRECTORY_IDLE) return FILE_MANAGER_NOT_READY;
    if ((g_directory.phase == FILE_MANAGER_DIRECTORY_DELETE_TARGET) ||
        (g_directory.phase == FILE_MANAGER_DIRECTORY_DELETE_SOURCE)) return FILE_MANAGER_BUSY;
    file_manager_directory_release();
    g_directory.status.state = FILE_MANAGER_TASK_CANCELLED;
    g_directory.status.result = FILE_MANAGER_CANCELLED;
    return FILE_MANAGER_OK;
}

void file_manager_directory_task_get_status(file_manager_status_t *status)
{
    if (status != NULL) *status = g_directory.status;
}
