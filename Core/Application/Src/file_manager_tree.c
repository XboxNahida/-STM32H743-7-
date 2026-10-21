#include "file_manager_tree.h"

#include <stddef.h>
#include <string.h>

#ifdef FILE_MANAGER_HOST_TEST
extern file_manager_result_t file_manager_tree_host_dir_open(
    uint32_t depth, const char *path);
extern file_manager_result_t file_manager_tree_host_dir_read(
    uint32_t depth, const char *path, char *name, uint32_t capacity,
    file_manager_entry_type_t *type, uint64_t *size, uint8_t *end);
extern void file_manager_tree_host_dir_close(uint32_t depth);
extern file_manager_result_t file_manager_tree_host_unlink(const char *path);
extern file_manager_result_t file_manager_tree_host_mkdir(const char *path);

#define file_manager_tree_platform_dir_open  file_manager_tree_host_dir_open
#define file_manager_tree_platform_dir_read  file_manager_tree_host_dir_read
#define file_manager_tree_platform_dir_close file_manager_tree_host_dir_close
#define file_manager_tree_platform_unlink    file_manager_tree_host_unlink
#define file_manager_tree_platform_mkdir     file_manager_tree_host_mkdir
#else
#include "file_manager_platform.h"
#define file_manager_tree_platform_dir_open  file_manager_platform_dir_open
#define file_manager_tree_platform_dir_read  file_manager_platform_dir_read
#define file_manager_tree_platform_dir_close file_manager_platform_dir_close
#define file_manager_tree_platform_unlink    file_manager_platform_unlink
#define file_manager_tree_platform_mkdir     file_manager_platform_mkdir
#endif

typedef struct
{
    uint16_t path_length;
    uint16_t destination_length;
    uint8_t opened;
} file_manager_tree_frame_t;

typedef struct
{
    file_manager_tree_mode_t mode;
    uint8_t active;
    uint8_t depth;
    char path[FILE_MANAGER_PATH_CAPACITY];
    char destination[FILE_MANAGER_PATH_CAPACITY];
    file_manager_tree_frame_t frames[FILE_MANAGER_MAX_DEPTH + 1U];
    uint64_t copy_processed;
    uint8_t copy_active;
} file_manager_tree_context_t;

static file_manager_tree_context_t g_tree;

file_manager_result_t file_manager_copy_task_start(
    const char *source, const char *destination_dir,
    file_manager_conflict_t conflict);
file_manager_result_t file_manager_copy_task_poll(void);
file_manager_result_t file_manager_copy_task_cancel(void);
void file_manager_copy_task_get_status(file_manager_status_t *status);

static file_manager_result_t file_manager_tree_fail(
    file_manager_result_t result)
{
    file_manager_tree_abort();
    return result;
}

file_manager_result_t file_manager_tree_begin(file_manager_tree_mode_t mode,
                                              const char *source,
                                              const char *destination)
{
    file_manager_result_t result;

    if (source == NULL)
    {
        return FILE_MANAGER_INVALID_ARG;
    }
    if (g_tree.active != 0U)
    {
        return FILE_MANAGER_BUSY;
    }
    if ((mode != FILE_MANAGER_TREE_SCAN) &&
        (mode != FILE_MANAGER_TREE_COPY) &&
        (mode != FILE_MANAGER_TREE_DELETE))
    {
        return FILE_MANAGER_INVALID_ARG;
    }
    memset(&g_tree, 0, sizeof(g_tree));
    result = file_manager_normalize_path(source, g_tree.path,
                                         sizeof(g_tree.path));
    if (result != FILE_MANAGER_OK)
    {
        return result;
    }
    if (mode == FILE_MANAGER_TREE_COPY)
    {
        if (destination == NULL) return FILE_MANAGER_INVALID_ARG;
        result = file_manager_normalize_path(destination, g_tree.destination,
                                             sizeof(g_tree.destination));
        if (result != FILE_MANAGER_OK) return result;
    }
    g_tree.mode = mode;
    g_tree.frames[0].path_length = (uint16_t)strlen(g_tree.path);
    g_tree.frames[0].destination_length =
        (uint16_t)strlen(g_tree.destination);
    g_tree.active = 1U;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_tree_step(file_manager_status_t *status)
{
    file_manager_tree_frame_t *frame;
    char name[FILE_MANAGER_NAME_CAPACITY];
    char child[FILE_MANAGER_PATH_CAPACITY];
    char destination_child[FILE_MANAGER_PATH_CAPACITY];
    file_manager_entry_type_t type = FILE_MANAGER_ENTRY_FILE;
    uint64_t size = 0U;
    uint8_t end = 0U;
    file_manager_result_t result;

    if ((status == NULL) || (g_tree.active == 0U))
    {
        return FILE_MANAGER_NOT_READY;
    }
    if (g_tree.copy_active != 0U)
    {
        file_manager_status_t copy_status;

        result = file_manager_copy_task_poll();
        if (result != FILE_MANAGER_OK)
            return file_manager_tree_fail(result);
        file_manager_copy_task_get_status(&copy_status);
        if (copy_status.processed_bytes >= g_tree.copy_processed)
            status->processed_bytes +=
                copy_status.processed_bytes - g_tree.copy_processed;
        g_tree.copy_processed = copy_status.processed_bytes;
        if (copy_status.state == FILE_MANAGER_TASK_DONE)
        {
            status->processed_entries++;
            g_tree.copy_active = 0U;
        }
        else if ((copy_status.state == FILE_MANAGER_TASK_ERROR) ||
                 (copy_status.state == FILE_MANAGER_TASK_CANCELLED))
        {
            return file_manager_tree_fail(
                (file_manager_result_t)copy_status.result);
        }
        return FILE_MANAGER_OK;
    }
    frame = &g_tree.frames[g_tree.depth];
    if (frame->opened == 0U)
    {
        if (g_tree.mode == FILE_MANAGER_TREE_COPY)
        {
            result = file_manager_tree_platform_mkdir(g_tree.destination);
            if ((result != FILE_MANAGER_OK) &&
                (result != FILE_MANAGER_EXISTS))
                return file_manager_tree_fail(result);
        }
        result = file_manager_tree_platform_dir_open(g_tree.depth,
                                                     g_tree.path);
        if (result != FILE_MANAGER_OK)
        {
            return file_manager_tree_fail(result);
        }
        frame->opened = 1U;
    }
    result = file_manager_tree_platform_dir_read(
        g_tree.depth, g_tree.path, name, sizeof(name), &type, &size, &end);
    if (result != FILE_MANAGER_OK)
    {
        return file_manager_tree_fail(result);
    }
    if (end == 0U)
    {
        result = file_manager_join_path(g_tree.path, name, child,
                                        sizeof(child));
        if (result != FILE_MANAGER_OK)
        {
            return file_manager_tree_fail(result);
        }
        if (type == FILE_MANAGER_ENTRY_DIRECTORY)
        {
            if (g_tree.mode == FILE_MANAGER_TREE_SCAN)
            {
                status->total_entries++;
            }
            if (g_tree.depth >= FILE_MANAGER_MAX_DEPTH)
            {
                return file_manager_tree_fail(FILE_MANAGER_TOO_DEEP);
            }
            if (g_tree.mode == FILE_MANAGER_TREE_COPY)
            {
                result = file_manager_join_path(g_tree.destination, name,
                    destination_child, sizeof(destination_child));
                if (result != FILE_MANAGER_OK)
                    return file_manager_tree_fail(result);
            }
            g_tree.depth++;
            memcpy(g_tree.path, child, strlen(child) + 1U);
            if (g_tree.mode == FILE_MANAGER_TREE_COPY)
                memcpy(g_tree.destination, destination_child,
                       strlen(destination_child) + 1U);
            g_tree.frames[g_tree.depth].path_length =
                (uint16_t)strlen(g_tree.path);
            g_tree.frames[g_tree.depth].destination_length =
                (uint16_t)strlen(g_tree.destination);
            g_tree.frames[g_tree.depth].opened = 0U;
            return FILE_MANAGER_OK;
        }
        if (g_tree.mode == FILE_MANAGER_TREE_SCAN)
        {
            status->total_entries++;
            status->total_bytes += size;
        }
        if (g_tree.mode == FILE_MANAGER_TREE_DELETE)
        {
            result = file_manager_tree_platform_unlink(child);
            if (result != FILE_MANAGER_OK)
            {
                return file_manager_tree_fail(result);
            }
            status->processed_entries++;
            status->processed_bytes += size;
        }
        else if (g_tree.mode == FILE_MANAGER_TREE_COPY)
        {
            result = file_manager_copy_task_start(
                child, g_tree.destination, FILE_MANAGER_CONFLICT_CANCEL);
            if (result != FILE_MANAGER_OK)
                return file_manager_tree_fail(result);
            g_tree.copy_processed = 0U;
            g_tree.copy_active = 1U;
        }
        return FILE_MANAGER_OK;
    }

    file_manager_tree_platform_dir_close(g_tree.depth);
    frame->opened = 0U;
    if (g_tree.mode == FILE_MANAGER_TREE_DELETE)
    {
        result = file_manager_tree_platform_unlink(g_tree.path);
        if (result != FILE_MANAGER_OK)
        {
            return file_manager_tree_fail(result);
        }
        status->processed_entries++;
    }
    if (g_tree.depth == 0U)
    {
        g_tree.active = 0U;
        return FILE_MANAGER_OK;
    }
    g_tree.depth--;
    g_tree.path[g_tree.frames[g_tree.depth].path_length] = '\0';
    if (g_tree.mode == FILE_MANAGER_TREE_COPY)
        g_tree.destination[
            g_tree.frames[g_tree.depth].destination_length] = '\0';
    return FILE_MANAGER_OK;
}

void file_manager_tree_abort(void)
{
    if (g_tree.copy_active != 0U)
    {
        (void)file_manager_copy_task_cancel();
        g_tree.copy_active = 0U;
    }
    while (g_tree.active != 0U)
    {
        if (g_tree.frames[g_tree.depth].opened != 0U)
        {
            file_manager_tree_platform_dir_close(g_tree.depth);
            g_tree.frames[g_tree.depth].opened = 0U;
        }
        if (g_tree.depth == 0U)
        {
            g_tree.active = 0U;
        }
        else
        {
            g_tree.depth--;
        }
    }
}

uint8_t file_manager_tree_active(void)
{
    return g_tree.active;
}
