#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "file_manager_tree.h"

typedef struct
{
    const char *name;
    file_manager_entry_type_t type;
    uint64_t size;
} host_entry_t;

static uint32_t g_indices[FILE_MANAGER_MAX_DEPTH + 1U];
static uint8_t g_deep_mode;
static uint32_t g_deep_limit;
static char g_deleted[16][FILE_MANAGER_PATH_CAPACITY];
static uint32_t g_delete_count;
static uint32_t g_session_count;
static char g_mkdir_paths[8][FILE_MANAGER_PATH_CAPACITY];
static uint32_t g_mkdir_count;
static char g_copy_sources[8][FILE_MANAGER_PATH_CAPACITY];
static char g_copy_destinations[8][FILE_MANAGER_PATH_CAPACITY];
static uint32_t g_copy_count;
static file_manager_status_t g_copy_status;
static int g_failures;

file_manager_result_t file_manager_tree_host_unlink(const char *path);

#define CHECK(condition)                                                     \
    do                                                                       \
    {                                                                        \
        if (!(condition))                                                    \
        {                                                                    \
            printf("FAIL line=%d condition=%s\n", __LINE__, #condition);    \
            g_failures++;                                                    \
        }                                                                    \
    } while (0)

static const host_entry_t *host_normal_entry(const char *path, uint32_t index)
{
    static const host_entry_t root[] = {
        {"A", FILE_MANAGER_ENTRY_DIRECTORY, 0U},
        {"root.bin", FILE_MANAGER_ENTRY_FILE, 10U}
    };
    static const host_entry_t a[] = {
        {"B", FILE_MANAGER_ENTRY_DIRECTORY, 0U},
        {"a.bin", FILE_MANAGER_ENTRY_FILE, 20U}
    };
    static const host_entry_t b[] = {
        {"b.bin", FILE_MANAGER_ENTRY_FILE, 30U}
    };

    if ((strcmp(path, "0:/tree") == 0) && (index < 2U)) return &root[index];
    if ((strcmp(path, "0:/tree/A") == 0) && (index < 2U)) return &a[index];
    if ((strcmp(path, "0:/tree/A/B") == 0) && (index < 1U)) return &b[index];
    return NULL;
}

file_manager_result_t file_manager_tree_host_dir_open(uint32_t depth,
                                                      const char *path)
{
    (void)path;
    g_indices[depth] = 0U;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_tree_host_dir_read(
    uint32_t depth, const char *path, char *name, uint32_t capacity,
    file_manager_entry_type_t *type, uint64_t *size, uint8_t *end)
{
    const host_entry_t *entry;
    int length;

    if (g_deep_mode != 0U)
    {
        if (depth >= g_deep_limit)
        {
            *end = 1U;
            return FILE_MANAGER_OK;
        }
        length = snprintf(name, capacity, "d%02lu", (unsigned long)depth);
        if ((length < 0) || ((uint32_t)length >= capacity))
            return FILE_MANAGER_PATH_TOO_LONG;
        if (g_indices[depth]++ == 0U)
        {
            *type = FILE_MANAGER_ENTRY_DIRECTORY;
            *size = 0U;
            *end = 0U;
        }
        else
        {
            *end = 1U;
        }
        return FILE_MANAGER_OK;
    }
    entry = host_normal_entry(path, g_indices[depth]++);
    if (entry == NULL)
    {
        *end = 1U;
        return FILE_MANAGER_OK;
    }
    length = snprintf(name, capacity, "%s", entry->name);
    if ((length < 0) || ((uint32_t)length >= capacity))
        return FILE_MANAGER_PATH_TOO_LONG;
    *type = entry->type;
    *size = entry->size;
    *end = 0U;
    return FILE_MANAGER_OK;
}

void file_manager_tree_host_dir_close(uint32_t depth)
{
    (void)depth;
}

file_manager_result_t file_manager_host_session_acquire(void)
{
    g_session_count++;
    return FILE_MANAGER_OK;
}

void file_manager_host_session_release(void)
{
    if (g_session_count > 0U) g_session_count--;
}

file_manager_result_t file_manager_host_stat(
    const char *path, file_manager_entry_type_t *type, uint64_t *size)
{
    if (strcmp(path, "0:/tree") == 0)
    {
        *type = FILE_MANAGER_ENTRY_DIRECTORY;
        *size = 0U;
        return FILE_MANAGER_OK;
    }
    return FILE_MANAGER_NOT_FOUND;
}

file_manager_result_t file_manager_host_unlink(const char *path)
{
    return file_manager_tree_host_unlink(path);
}

file_manager_result_t file_manager_host_rename(const char *old_path,
                                               const char *new_path)
{
    (void)old_path; (void)new_path;
    return FILE_MANAGER_IO_ERROR;
}

file_manager_result_t file_manager_copy_task_start(
    const char *source, const char *destination,
    file_manager_conflict_t conflict)
{
    size_t source_length = strlen(source);
    size_t destination_length = strlen(destination);
    uint64_t size = 0U;

    (void)conflict;
    if (strcmp(source, "0:/tree") == 0) return FILE_MANAGER_UNSUPPORTED;
    if ((g_copy_count >= 8U) ||
        (source_length >= sizeof(g_copy_sources[0])) ||
        (destination_length >= sizeof(g_copy_destinations[0])))
        return FILE_MANAGER_PATH_TOO_LONG;
    memcpy(g_copy_sources[g_copy_count], source, source_length + 1U);
    memcpy(g_copy_destinations[g_copy_count], destination,
           destination_length + 1U);
    if (strstr(source, "root.bin") != NULL) size = 10U;
    else if (strstr(source, "a.bin") != NULL) size = 20U;
    else if (strstr(source, "b.bin") != NULL) size = 30U;
    memset(&g_copy_status, 0, sizeof(g_copy_status));
    g_copy_status.state = FILE_MANAGER_TASK_COPYING;
    g_copy_status.total_bytes = size;
    g_copy_count++;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_copy_task_poll(void)
{
    g_copy_status.processed_bytes = g_copy_status.total_bytes;
    g_copy_status.processed_entries = 1U;
    g_copy_status.state = FILE_MANAGER_TASK_DONE;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_copy_task_cancel(void)
{
    g_copy_status.state = FILE_MANAGER_TASK_CANCELLED;
    return FILE_MANAGER_OK;
}

void file_manager_copy_task_get_status(file_manager_status_t *status)
{
    *status = g_copy_status;
}

file_manager_result_t file_manager_tree_host_mkdir(const char *path)
{
    size_t length = strlen(path);

    if ((g_mkdir_count >= 8U) || (length >= sizeof(g_mkdir_paths[0])))
        return FILE_MANAGER_PATH_TOO_LONG;
    memcpy(g_mkdir_paths[g_mkdir_count], path, length + 1U);
    g_mkdir_count++;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_tree_host_unlink(const char *path)
{
    size_t length;

    if (g_delete_count >= 16U) return FILE_MANAGER_IO_ERROR;
    length = strlen(path);
    if (length >= sizeof(g_deleted[0])) return FILE_MANAGER_PATH_TOO_LONG;
    memcpy(g_deleted[g_delete_count], path, length + 1U);
    g_delete_count++;
    return FILE_MANAGER_OK;
}

static file_manager_result_t run_tree(file_manager_status_t *status)
{
    uint32_t guard = 256U;
    file_manager_result_t result = FILE_MANAGER_OK;

    while ((file_manager_tree_active() != 0U) && (guard-- != 0U))
    {
        result = file_manager_tree_step(status);
        if (result != FILE_MANAGER_OK) break;
    }
    CHECK(guard != 0U);
    return result;
}

static void reset_fixture(void)
{
    memset(g_indices, 0, sizeof(g_indices));
    memset(g_deleted, 0, sizeof(g_deleted));
    g_deep_mode = 0U;
    g_deep_limit = 0U;
    g_delete_count = 0U;
    g_session_count = 0U;
    memset(g_mkdir_paths, 0, sizeof(g_mkdir_paths));
    memset(g_copy_sources, 0, sizeof(g_copy_sources));
    memset(g_copy_destinations, 0, sizeof(g_copy_destinations));
    memset(&g_copy_status, 0, sizeof(g_copy_status));
    g_mkdir_count = 0U;
    g_copy_count = 0U;
}

static void test_scan_totals(void)
{
    file_manager_status_t status;

    reset_fixture();
    memset(&status, 0, sizeof(status));
    CHECK(file_manager_tree_begin(FILE_MANAGER_TREE_SCAN, "0:/tree", NULL) ==
          FILE_MANAGER_OK);
    CHECK(run_tree(&status) == FILE_MANAGER_OK);
    CHECK(status.total_entries == 5U);
    CHECK(status.total_bytes == 60ULL);
}

static void test_depth_boundary(void)
{
    file_manager_status_t status;

    reset_fixture();
    g_deep_mode = 1U;
    g_deep_limit = FILE_MANAGER_MAX_DEPTH;
    memset(&status, 0, sizeof(status));
    CHECK(file_manager_tree_begin(FILE_MANAGER_TREE_SCAN, "0:/tree", NULL) ==
          FILE_MANAGER_OK);
    CHECK(run_tree(&status) == FILE_MANAGER_OK);

    reset_fixture();
    g_deep_mode = 1U;
    g_deep_limit = FILE_MANAGER_MAX_DEPTH + 1U;
    memset(&status, 0, sizeof(status));
    CHECK(file_manager_tree_begin(FILE_MANAGER_TREE_SCAN, "0:/tree", NULL) ==
          FILE_MANAGER_OK);
    CHECK(run_tree(&status) == FILE_MANAGER_TOO_DEEP);
}

static void test_delete_is_postorder(void)
{
    file_manager_status_t status;

    reset_fixture();
    memset(&status, 0, sizeof(status));
    CHECK(file_manager_tree_begin(FILE_MANAGER_TREE_DELETE, "0:/tree", NULL) ==
          FILE_MANAGER_OK);
    CHECK(run_tree(&status) == FILE_MANAGER_OK);
    CHECK(g_delete_count == 6U);
    CHECK(strcmp(g_deleted[0], "0:/tree/A/B/b.bin") == 0);
    CHECK(strcmp(g_deleted[1], "0:/tree/A/B") == 0);
    CHECK(strcmp(g_deleted[2], "0:/tree/A/a.bin") == 0);
    CHECK(strcmp(g_deleted[3], "0:/tree/A") == 0);
    CHECK(strcmp(g_deleted[4], "0:/tree/root.bin") == 0);
    CHECK(strcmp(g_deleted[5], "0:/tree") == 0);
}

static void test_delete_service_scans_and_rejects_cancel(void)
{
    file_manager_status_t status;
    uint32_t guard = 256U;

    reset_fixture();
    CHECK(file_manager_delete_start("0:/tree", 1U) == FILE_MANAGER_OK);
    CHECK(file_manager_cancel() == FILE_MANAGER_BUSY);
    do
    {
        CHECK(file_manager_poll() == FILE_MANAGER_OK);
        file_manager_get_status(&status);
    } while (((status.state == FILE_MANAGER_TASK_SCANNING) ||
              (status.state == FILE_MANAGER_TASK_DELETING)) &&
             (--guard != 0U));
    CHECK(guard != 0U);
    CHECK(status.state == FILE_MANAGER_TASK_DONE);
    CHECK(status.total_bytes == 60ULL);
    CHECK(g_delete_count == 6U);
    CHECK(g_session_count == 0U);

    reset_fixture();
    CHECK(file_manager_delete_start("0:/tree", 0U) ==
          FILE_MANAGER_INVALID_ARG);
    CHECK(g_delete_count == 0U);
}

static void test_copy_creates_tree_and_streams_files(void)
{
    file_manager_status_t status;

    reset_fixture();
    memset(&status, 0, sizeof(status));
    status.total_bytes = 60U;
    status.total_entries = 5U;
    CHECK(file_manager_tree_begin(FILE_MANAGER_TREE_COPY, "0:/tree",
                                  "0:/out/tree") == FILE_MANAGER_OK);
    CHECK(run_tree(&status) == FILE_MANAGER_OK);
    CHECK(g_mkdir_count == 3U);
    CHECK(strcmp(g_mkdir_paths[0], "0:/out/tree") == 0);
    CHECK(strcmp(g_mkdir_paths[1], "0:/out/tree/A") == 0);
    CHECK(strcmp(g_mkdir_paths[2], "0:/out/tree/A/B") == 0);
    CHECK(g_copy_count == 3U);
    CHECK(strcmp(g_copy_sources[0], "0:/tree/A/B/b.bin") == 0);
    CHECK(strcmp(g_copy_destinations[0], "0:/out/tree/A/B") == 0);
    CHECK(strcmp(g_copy_sources[1], "0:/tree/A/a.bin") == 0);
    CHECK(strcmp(g_copy_sources[2], "0:/tree/root.bin") == 0);
    CHECK(status.processed_bytes == 60U);
}

static void test_public_directory_copy(void)
{
    file_manager_status_t status;
    uint32_t guard = 256U;

    reset_fixture();
    CHECK(file_manager_copy_start("0:/tree", "0:/out",
                                  FILE_MANAGER_CONFLICT_RENAME) ==
          FILE_MANAGER_OK);
    do
    {
        CHECK(file_manager_poll() == FILE_MANAGER_OK);
        file_manager_get_status(&status);
    } while (((status.state == FILE_MANAGER_TASK_SCANNING) ||
              (status.state == FILE_MANAGER_TASK_COPYING)) &&
             (--guard != 0U));
    CHECK(guard != 0U);
    CHECK(status.state == FILE_MANAGER_TASK_DONE);
    CHECK(status.total_bytes == 60U);
    CHECK(status.processed_bytes == 60U);
    CHECK(g_mkdir_count == 3U);
    CHECK(strcmp(g_mkdir_paths[0], "0:/out/tree") == 0);
    CHECK(g_session_count == 0U);
}

static void test_public_directory_move_copies_then_deletes(void)
{
    file_manager_status_t status;
    uint32_t guard = 512U;

    reset_fixture();
    CHECK(file_manager_move_start("0:/tree", "0:/out",
                                  FILE_MANAGER_CONFLICT_RENAME) ==
          FILE_MANAGER_OK);
    do
    {
        CHECK(file_manager_poll() == FILE_MANAGER_OK);
        file_manager_get_status(&status);
    } while ((status.state != FILE_MANAGER_TASK_DONE) && (--guard != 0U));
    CHECK(guard != 0U);
    CHECK(status.state == FILE_MANAGER_TASK_DONE);
    CHECK(g_copy_count == 3U);
    CHECK(g_delete_count == 6U);
    CHECK(strcmp(g_deleted[5], "0:/tree") == 0);
    CHECK(g_session_count == 0U);
}

int main(void)
{
    test_scan_totals();
    test_depth_boundary();
    test_delete_is_postorder();
    test_delete_service_scans_and_rejects_cancel();
    test_copy_creates_tree_and_streams_files();
    test_public_directory_copy();
    test_public_directory_move_copies_then_deletes();
    if (g_failures != 0)
    {
        printf("FAIL file_manager_tree failures=%d\n", g_failures);
        return 1;
    }
    printf("PASS file_manager_tree\n");
    return 0;
}
