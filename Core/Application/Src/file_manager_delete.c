#include "file_manager_service.h"

#include <string.h>

#include "file_manager_tree.h"

#ifdef FILE_MANAGER_HOST_TEST
extern file_manager_result_t file_manager_host_session_acquire(void);
extern void file_manager_host_session_release(void);
extern file_manager_result_t file_manager_host_stat(
    const char *path, file_manager_entry_type_t *type, uint64_t *size);
extern file_manager_result_t file_manager_host_unlink(const char *path);
#define file_manager_platform_session_acquire file_manager_host_session_acquire
#define file_manager_platform_session_release file_manager_host_session_release
#define file_manager_platform_stat            file_manager_host_stat
#define file_manager_platform_unlink          file_manager_host_unlink
#else
#include "file_manager_platform.h"
#endif

typedef struct
{
    file_manager_status_t status;
    char path[FILE_MANAGER_PATH_CAPACITY];
    file_manager_entry_type_t type;
    uint8_t session_held;
    uint8_t active;
} file_manager_delete_context_t;

static file_manager_delete_context_t g_delete;

static void file_manager_delete_release(void)
{
    if (file_manager_tree_active() != 0U) file_manager_tree_abort();
    if (g_delete.session_held != 0U)
    {
        file_manager_platform_session_release();
        g_delete.session_held = 0U;
    }
    g_delete.active = 0U;
}

static file_manager_result_t file_manager_delete_fail(
    file_manager_result_t result)
{
    file_manager_delete_release();
    g_delete.status.state = FILE_MANAGER_TASK_ERROR;
    g_delete.status.result = result;
    return result;
}

file_manager_result_t file_manager_delete_task_start(
    const char *path, uint8_t recursive_confirmed)
{
    uint64_t size = 0U;
    file_manager_result_t result;

    if ((path == NULL) || (recursive_confirmed == 0U))
        return FILE_MANAGER_INVALID_ARG;
    if (g_delete.active != 0U) return FILE_MANAGER_BUSY;
    memset(&g_delete, 0, sizeof(g_delete));
    result = file_manager_normalize_path(path, g_delete.path,
                                         sizeof(g_delete.path));
    if (result != FILE_MANAGER_OK) return result;
    if (strcmp(g_delete.path, "0:/") == 0) return FILE_MANAGER_INVALID_ARG;
    result = file_manager_platform_session_acquire();
    if (result != FILE_MANAGER_OK) return result;
    g_delete.session_held = 1U;
    result = file_manager_platform_stat(g_delete.path, &g_delete.type, &size);
    if (result != FILE_MANAGER_OK) return file_manager_delete_fail(result);
    g_delete.status.total_bytes = size;
    g_delete.status.total_entries = 1U;
    g_delete.status.result = FILE_MANAGER_OK;
    g_delete.active = 1U;
    if (g_delete.type == FILE_MANAGER_ENTRY_FILE)
    {
        g_delete.status.state = FILE_MANAGER_TASK_DELETING;
        return FILE_MANAGER_OK;
    }
    memset(&g_delete.status, 0, sizeof(g_delete.status));
    g_delete.status.state = FILE_MANAGER_TASK_SCANNING;
    result = file_manager_tree_begin(FILE_MANAGER_TREE_SCAN,
                                     g_delete.path, NULL);
    if (result != FILE_MANAGER_OK) return file_manager_delete_fail(result);
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_delete_task_poll(void)
{
    file_manager_result_t result;

    if (g_delete.active == 0U) return FILE_MANAGER_OK;
    if (g_delete.status.state == FILE_MANAGER_TASK_SCANNING)
    {
        result = file_manager_tree_step(&g_delete.status);
        if (result != FILE_MANAGER_OK) return file_manager_delete_fail(result);
        if (file_manager_tree_active() != 0U) return FILE_MANAGER_OK;
        g_delete.status.total_entries++;
        result = file_manager_tree_begin(FILE_MANAGER_TREE_DELETE,
                                         g_delete.path, NULL);
        if (result != FILE_MANAGER_OK) return file_manager_delete_fail(result);
        g_delete.status.state = FILE_MANAGER_TASK_DELETING;
        return FILE_MANAGER_OK;
    }
    if (g_delete.type == FILE_MANAGER_ENTRY_FILE)
    {
        result = file_manager_platform_unlink(g_delete.path);
        if (result != FILE_MANAGER_OK) return file_manager_delete_fail(result);
        g_delete.status.processed_bytes = g_delete.status.total_bytes;
        g_delete.status.processed_entries = 1U;
    }
    else
    {
        result = file_manager_tree_step(&g_delete.status);
        if (result != FILE_MANAGER_OK) return file_manager_delete_fail(result);
        if (file_manager_tree_active() != 0U)
        {
            if (g_delete.status.total_bytes != 0U)
                g_delete.status.percent = (uint8_t)(
                    (g_delete.status.processed_bytes * 100ULL) /
                    g_delete.status.total_bytes);
            return FILE_MANAGER_OK;
        }
    }
    file_manager_delete_release();
    g_delete.status.percent = 100U;
    g_delete.status.state = FILE_MANAGER_TASK_DONE;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_delete_task_cancel(void)
{
    if (g_delete.active == 0U) return FILE_MANAGER_NOT_READY;
    return FILE_MANAGER_BUSY;
}

void file_manager_delete_task_get_status(file_manager_status_t *status)
{
    if (status != NULL) *status = g_delete.status;
}
