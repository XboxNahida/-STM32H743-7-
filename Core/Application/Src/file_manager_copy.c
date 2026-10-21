#include "file_manager_service.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef FILE_MANAGER_HOST_TEST
extern file_manager_result_t file_manager_host_session_acquire(void);
extern void file_manager_host_session_release(void);
extern uint8_t *file_manager_host_transfer_buffer(uint32_t *capacity);
extern file_manager_result_t file_manager_host_stat(
    const char *path, file_manager_entry_type_t *type, uint64_t *size);
extern file_manager_result_t file_manager_host_source_open(const char *path);
extern file_manager_result_t file_manager_host_source_read(
    uint8_t *data, uint32_t requested, uint32_t *actual);
extern void file_manager_host_source_close(void);
extern file_manager_result_t file_manager_host_target_open(const char *path);
extern file_manager_result_t file_manager_host_target_write(
    const uint8_t *data, uint32_t requested, uint32_t *actual);
extern file_manager_result_t file_manager_host_target_sync(void);
extern void file_manager_host_target_close(void);
extern file_manager_result_t file_manager_host_unlink(const char *path);
extern file_manager_result_t file_manager_host_rename(const char *old_path,
                                                      const char *new_path);

#define file_manager_platform_session_acquire file_manager_host_session_acquire
#define file_manager_platform_session_release file_manager_host_session_release
#define file_manager_platform_transfer_buffer file_manager_host_transfer_buffer
#define file_manager_platform_stat            file_manager_host_stat
#define file_manager_platform_source_open     file_manager_host_source_open
#define file_manager_platform_source_read     file_manager_host_source_read
#define file_manager_platform_source_close    file_manager_host_source_close
#define file_manager_platform_target_open     file_manager_host_target_open
#define file_manager_platform_target_write    file_manager_host_target_write
#define file_manager_platform_target_sync     file_manager_host_target_sync
#define file_manager_platform_target_close    file_manager_host_target_close
#define file_manager_platform_unlink          file_manager_host_unlink
#define file_manager_platform_rename          file_manager_host_rename
#else
#include "file_manager_platform.h"
#endif

#define FILE_MANAGER_TEMP_ATTEMPTS 1000U

typedef struct
{
    file_manager_status_t status;
    char source[FILE_MANAGER_PATH_CAPACITY];
    char final_path[FILE_MANAGER_PATH_CAPACITY];
    char temporary_path[FILE_MANAGER_PATH_CAPACITY];
    uint8_t *buffer;
    uint32_t buffer_capacity;
    uint8_t session_held;
    uint8_t source_open;
    uint8_t target_open;
    uint8_t overwrite;
} file_manager_copy_context_t;

static file_manager_copy_context_t g_copy;
static const char *file_manager_basename(const char *path)
{
    const char *name = path;
    const char *cursor;

    for (cursor = path; *cursor != '\0'; cursor++)
    {
        if (*cursor == '/') name = cursor + 1;
    }
    return name;
}
static void file_manager_copy_release(uint8_t remove_temporary)
{
    if (g_copy.target_open != 0U)
    {
        file_manager_platform_target_close();
        g_copy.target_open = 0U;
    }
    if (g_copy.source_open != 0U)
    {
        file_manager_platform_source_close();
        g_copy.source_open = 0U;
    }
    if ((remove_temporary != 0U) && (g_copy.temporary_path[0] != '\0'))
    {
        (void)file_manager_platform_unlink(g_copy.temporary_path);
    }
    if (g_copy.session_held != 0U)
    {
        file_manager_platform_session_release();
        g_copy.session_held = 0U;
    }
}
static file_manager_result_t file_manager_copy_fail(
    file_manager_result_t result)
{
    file_manager_copy_release(1U);
    g_copy.status.state = FILE_MANAGER_TASK_ERROR;
    g_copy.status.result = result;
    return result;
}
static file_manager_result_t file_manager_select_renamed_path(
    const char *directory, const char *name)
{
    const char *extension = strrchr(name, '.');
    size_t stem_length = (extension != NULL) ?
        (size_t)(extension - name) : strlen(name);
    uint32_t index;

    for (index = 1U; index <= 9999U; index++)
    {
        char candidate[FILE_MANAGER_NAME_CAPACITY];
        file_manager_entry_type_t type;
        uint64_t size;
        int length = snprintf(candidate, sizeof(candidate), "%.*s (%lu)%s",
            (int)stem_length, name, (unsigned long)index,
            (extension != NULL) ? extension : "");
        file_manager_result_t result;

        if ((length < 0) || ((uint32_t)length >= sizeof(candidate)))
            return FILE_MANAGER_PATH_TOO_LONG;
        result = file_manager_join_path(directory, candidate, g_copy.final_path,
                                        sizeof(g_copy.final_path));
        if (result != FILE_MANAGER_OK) return result;
        result = file_manager_platform_stat(g_copy.final_path, &type, &size);
        if (result == FILE_MANAGER_NOT_FOUND) return FILE_MANAGER_OK;
        if (result != FILE_MANAGER_OK) return result;
    }
    return FILE_MANAGER_EXISTS;
}
static file_manager_result_t file_manager_select_temporary_path(
    const char *directory)
{
    uint32_t index;

    for (index = 1U; index <= FILE_MANAGER_TEMP_ATTEMPTS; index++)
    {
        char name[32];
        file_manager_entry_type_t type;
        uint64_t size;
        int length = snprintf(name, sizeof(name), ".fm_tmp_%04lu",
                              (unsigned long)index);
        file_manager_result_t result;

        if ((length < 0) || ((uint32_t)length >= sizeof(name)))
            return FILE_MANAGER_PATH_TOO_LONG;
        result = file_manager_join_path(directory, name,
            g_copy.temporary_path, sizeof(g_copy.temporary_path));
        if (result != FILE_MANAGER_OK) return result;
        result = file_manager_platform_stat(g_copy.temporary_path, &type, &size);
        if (result == FILE_MANAGER_NOT_FOUND) return FILE_MANAGER_OK;
        if (result != FILE_MANAGER_OK) return result;
    }
    return FILE_MANAGER_EXISTS;
}
file_manager_result_t file_manager_copy_task_start(
    const char *source, const char *destination_dir,
    file_manager_conflict_t conflict)
{
    char destination[FILE_MANAGER_PATH_CAPACITY];
    const char *name;
    file_manager_entry_type_t type;
    uint64_t size;
    file_manager_result_t result;

    if ((source == NULL) || (destination_dir == NULL) ||
        (conflict > FILE_MANAGER_CONFLICT_CANCEL))
        return FILE_MANAGER_INVALID_ARG;
    if ((g_copy.status.state == FILE_MANAGER_TASK_COPYING) ||
        (g_copy.status.state == FILE_MANAGER_TASK_MOVING) ||
        (g_copy.status.state == FILE_MANAGER_TASK_DELETING))
        return FILE_MANAGER_BUSY;
    memset(&g_copy, 0, sizeof(g_copy));
    result = file_manager_normalize_path(source, g_copy.source,
                                         sizeof(g_copy.source));
    if (result != FILE_MANAGER_OK) return result;
    result = file_manager_normalize_path(destination_dir, destination,
                                         sizeof(destination));
    if (result != FILE_MANAGER_OK) return result;
    name = file_manager_basename(g_copy.source);
    result = file_manager_join_path(destination, name, g_copy.final_path,
                                    sizeof(g_copy.final_path));
    if (result != FILE_MANAGER_OK) return result;
    result = file_manager_platform_session_acquire();
    if (result != FILE_MANAGER_OK) return result;
    g_copy.session_held = 1U;
    result = file_manager_platform_stat(g_copy.source, &type, &size);
    if (result != FILE_MANAGER_OK) return file_manager_copy_fail(result);
    if (type != FILE_MANAGER_ENTRY_FILE)
        return file_manager_copy_fail(FILE_MANAGER_UNSUPPORTED);
    result = file_manager_platform_stat(g_copy.final_path, &type, &size);
    if (result == FILE_MANAGER_OK)
    {
        if (conflict == FILE_MANAGER_CONFLICT_CANCEL)
        {
            file_manager_copy_release(0U);
            g_copy.status.state = FILE_MANAGER_TASK_CANCELLED;
            g_copy.status.result = FILE_MANAGER_CANCELLED;
            return FILE_MANAGER_CANCELLED;
        }
        if (conflict == FILE_MANAGER_CONFLICT_RENAME)
        {
            result = file_manager_select_renamed_path(destination, name);
            if (result != FILE_MANAGER_OK) return file_manager_copy_fail(result);
        }
        else
        {
            g_copy.overwrite = 1U;
        }
    }
    else if (result != FILE_MANAGER_NOT_FOUND)
    {
        return file_manager_copy_fail(result);
    }
    result = file_manager_platform_stat(g_copy.source, &type, &size);
    if (result != FILE_MANAGER_OK) return file_manager_copy_fail(result);
    g_copy.status.total_bytes = size;
    g_copy.status.total_entries = 1U;
    memcpy(g_copy.status.current_name, name, strlen(name) + 1U);
    g_copy.buffer = file_manager_platform_transfer_buffer(
        &g_copy.buffer_capacity);
    if ((g_copy.buffer == NULL) || (g_copy.buffer_capacity == 0U))
        return file_manager_copy_fail(FILE_MANAGER_NOT_READY);
    result = file_manager_select_temporary_path(destination);
    if (result != FILE_MANAGER_OK) return file_manager_copy_fail(result);
    result = file_manager_platform_source_open(g_copy.source);
    if (result != FILE_MANAGER_OK) return file_manager_copy_fail(result);
    g_copy.source_open = 1U;
    result = file_manager_platform_target_open(g_copy.temporary_path);
    if (result != FILE_MANAGER_OK) return file_manager_copy_fail(result);
    g_copy.target_open = 1U;
    g_copy.status.state = FILE_MANAGER_TASK_COPYING;
    g_copy.status.result = FILE_MANAGER_OK;
    return FILE_MANAGER_OK;
}
file_manager_result_t file_manager_copy_task_poll(void)
{
    uint64_t remaining;
    uint32_t requested;
    uint32_t actual = 0U;
    uint32_t written = 0U;
    file_manager_result_t result;

    if (g_copy.status.state != FILE_MANAGER_TASK_COPYING)
        return FILE_MANAGER_OK;
    remaining = g_copy.status.total_bytes - g_copy.status.processed_bytes;
    requested = (remaining < g_copy.buffer_capacity) ?
        (uint32_t)remaining : g_copy.buffer_capacity;
    if (requested != 0U)
    {
        result = file_manager_platform_source_read(g_copy.buffer, requested,
                                                   &actual);
        if (result != FILE_MANAGER_OK) return file_manager_copy_fail(result);
        if ((actual == 0U) || (actual > requested))
            return file_manager_copy_fail(FILE_MANAGER_IO_ERROR);
        result = file_manager_platform_target_write(g_copy.buffer, actual,
                                                    &written);
        if ((result != FILE_MANAGER_OK) || (written != actual))
            return file_manager_copy_fail((result != FILE_MANAGER_OK) ?
                                          result : FILE_MANAGER_IO_ERROR);
        g_copy.status.processed_bytes += written;
        g_copy.status.percent = (uint8_t)(
            (g_copy.status.processed_bytes * 100ULL) /
            g_copy.status.total_bytes);
        return FILE_MANAGER_OK;
    }
    result = file_manager_platform_target_sync();
    if (result != FILE_MANAGER_OK) return file_manager_copy_fail(result);
    file_manager_platform_target_close();
    g_copy.target_open = 0U;
    file_manager_platform_source_close();
    g_copy.source_open = 0U;
    if (g_copy.overwrite != 0U)
    {
        result = file_manager_platform_unlink(g_copy.final_path);
        if (result != FILE_MANAGER_OK) return file_manager_copy_fail(result);
    }
    result = file_manager_platform_rename(g_copy.temporary_path,
                                          g_copy.final_path);
    if (result != FILE_MANAGER_OK) return file_manager_copy_fail(result);
    g_copy.temporary_path[0] = '\0';
    file_manager_copy_release(0U);
    g_copy.status.processed_entries = 1U;
    g_copy.status.percent = 100U;
    g_copy.status.state = FILE_MANAGER_TASK_DONE;
    return FILE_MANAGER_OK;
}
file_manager_result_t file_manager_copy_task_cancel(void)
{
    if (g_copy.status.state != FILE_MANAGER_TASK_COPYING)
        return FILE_MANAGER_NOT_READY;
    file_manager_copy_release(1U);
    g_copy.status.state = FILE_MANAGER_TASK_CANCELLED;
    g_copy.status.result = FILE_MANAGER_CANCELLED;
    return FILE_MANAGER_OK;
}
void file_manager_copy_task_get_status(file_manager_status_t *status)
{
    if (status != NULL) *status = g_copy.status;
}
