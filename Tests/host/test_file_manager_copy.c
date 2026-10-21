#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "file_manager_service.h"

#define HOST_SOURCE_SIZE   204800U
#define HOST_BUFFER_SIZE    65536U

static uint8_t g_source[HOST_SOURCE_SIZE];
static uint8_t g_target[HOST_SOURCE_SIZE];
static uint8_t g_transfer[HOST_BUFFER_SIZE];
static uint64_t g_reported_size;
static uint32_t g_source_offset;
static uint32_t g_target_size;
static uint32_t g_last_transfer;
static uint32_t g_session_count;
static uint32_t g_unlink_count;
static uint8_t g_final_exists;
static uint8_t g_committed;
static uint8_t g_source_rename_fails;
static uint8_t g_source_deleted;
static char g_final_path[FILE_MANAGER_PATH_CAPACITY];
static int g_failures;

file_manager_result_t file_manager_directory_task_start(
    const char *source, const char *destination_dir,
    file_manager_conflict_t conflict, uint8_t move)
{
    (void)source;
    (void)destination_dir;
    (void)conflict;
    (void)move;
    return FILE_MANAGER_UNSUPPORTED;
}

file_manager_result_t file_manager_directory_task_poll(void)
{ return FILE_MANAGER_OK; }

file_manager_result_t file_manager_directory_task_cancel(void)
{ return FILE_MANAGER_NOT_READY; }

void file_manager_directory_task_get_status(file_manager_status_t *status)
{ (void)status; }

file_manager_result_t file_manager_delete_task_start(
    const char *path, uint8_t recursive_confirmed)
{
    (void)path;
    (void)recursive_confirmed;
    return FILE_MANAGER_UNSUPPORTED;
}

file_manager_result_t file_manager_delete_task_poll(void)
{ return FILE_MANAGER_OK; }

file_manager_result_t file_manager_delete_task_cancel(void)
{ return FILE_MANAGER_NOT_READY; }

void file_manager_delete_task_get_status(file_manager_status_t *status)
{ (void)status; }

#define CHECK(condition)                                                     \
    do                                                                       \
    {                                                                        \
        if (!(condition))                                                    \
        {                                                                    \
            printf("FAIL line=%d condition=%s\n", __LINE__, #condition);    \
            g_failures++;                                                    \
        }                                                                    \
    } while (0)

file_manager_result_t file_manager_host_session_acquire(void)
{
    g_session_count++;
    return FILE_MANAGER_OK;
}

void file_manager_host_session_release(void)
{
    if (g_session_count > 0U) g_session_count--;
}

uint8_t *file_manager_host_transfer_buffer(uint32_t *capacity)
{
    *capacity = sizeof(g_transfer);
    return g_transfer;
}

file_manager_result_t file_manager_host_stat(
    const char *path, file_manager_entry_type_t *type, uint64_t *size)
{
    if (strcmp(path, "0:/source.bin") == 0)
    {
        *type = FILE_MANAGER_ENTRY_FILE;
        *size = g_reported_size;
        return FILE_MANAGER_OK;
    }
    if ((strcmp(path, "0:/dst/source.bin") == 0) &&
        (g_final_exists != 0U))
    {
        *type = FILE_MANAGER_ENTRY_FILE;
        *size = 1U;
        return FILE_MANAGER_OK;
    }
    return FILE_MANAGER_NOT_FOUND;
}

file_manager_result_t file_manager_host_source_open(const char *path)
{
    if (strcmp(path, "0:/source.bin") != 0) return FILE_MANAGER_NOT_FOUND;
    g_source_offset = 0U;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_host_source_read(
    uint8_t *data, uint32_t requested, uint32_t *actual)
{
    uint32_t remaining = HOST_SOURCE_SIZE - g_source_offset;
    uint32_t amount = (requested < remaining) ? requested : remaining;

    memcpy(data, &g_source[g_source_offset], amount);
    g_source_offset += amount;
    g_last_transfer = amount;
    *actual = amount;
    return FILE_MANAGER_OK;
}

void file_manager_host_source_close(void)
{
}

file_manager_result_t file_manager_host_target_open(const char *path)
{
    if (strstr(path, "/.fm_tmp_") == NULL) return FILE_MANAGER_INVALID_NAME;
    g_target_size = 0U;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_host_target_write(
    const uint8_t *data, uint32_t requested, uint32_t *actual)
{
    if (requested > HOST_SOURCE_SIZE - g_target_size)
        return FILE_MANAGER_NO_SPACE;
    memcpy(&g_target[g_target_size], data, requested);
    g_target_size += requested;
    *actual = requested;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_host_target_sync(void)
{
    return FILE_MANAGER_OK;
}

void file_manager_host_target_close(void)
{
}

file_manager_result_t file_manager_host_unlink(const char *path)
{
    if (strcmp(path, "0:/source.bin") == 0) g_source_deleted = 1U;
    g_unlink_count++;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_host_rename(const char *old_path,
                                               const char *new_path)
{
    size_t length = strlen(new_path);

    if ((strcmp(old_path, "0:/source.bin") == 0) &&
        (g_source_rename_fails != 0U)) return FILE_MANAGER_IO_ERROR;
    if ((strstr(old_path, "/.fm_tmp_") == NULL) &&
        (strcmp(old_path, "0:/source.bin") != 0))
        return FILE_MANAGER_INVALID_NAME;
    if (length >= sizeof(g_final_path)) return FILE_MANAGER_PATH_TOO_LONG;
    memcpy(g_final_path, new_path, length + 1U);
    g_committed = 1U;
    return FILE_MANAGER_OK;
}

/* Unused list/create platform hooks required by the shared service object. */
file_manager_result_t file_manager_host_dir_open(const char *path)
{
    (void)path;
    return FILE_MANAGER_NOT_FOUND;
}

file_manager_result_t file_manager_host_dir_read(
    char *name, uint32_t capacity, file_manager_entry_type_t *type,
    uint64_t *size, uint16_t *date, uint16_t *time, uint8_t *end)
{
    (void)name; (void)capacity; (void)type; (void)size; (void)date; (void)time;
    *end = 1U;
    return FILE_MANAGER_OK;
}

void file_manager_host_dir_close(void) {}

file_manager_result_t file_manager_host_create(
    const char *path, file_manager_entry_type_t type)
{
    (void)path; (void)type;
    return FILE_MANAGER_UNSUPPORTED;
}

static void reset_fixture(void)
{
    uint32_t index;

    for (index = 0U; index < HOST_SOURCE_SIZE; index++)
        g_source[index] = (uint8_t)(index * 17U + 3U);
    memset(g_target, 0, sizeof(g_target));
    g_reported_size = HOST_SOURCE_SIZE;
    g_source_offset = 0U;
    g_target_size = 0U;
    g_last_transfer = 0U;
    g_session_count = 0U;
    g_unlink_count = 0U;
    g_final_exists = 0U;
    g_committed = 0U;
    g_source_rename_fails = 0U;
    g_source_deleted = 0U;
    g_final_path[0] = '\0';
}

static void poll_until_stopped(file_manager_status_t *status)
{
    uint32_t guard = 16U;

    do
    {
        CHECK(file_manager_poll() == FILE_MANAGER_OK);
        CHECK(g_last_transfer <= HOST_BUFFER_SIZE);
        file_manager_get_status(status);
    } while ((status->state == FILE_MANAGER_TASK_COPYING) && (--guard != 0U));
    CHECK(guard != 0U);
}

static void test_stream_copy_and_commit(void)
{
    file_manager_status_t status;

    reset_fixture();
    CHECK(file_manager_copy_start("0:/source.bin", "0:/dst",
                                  FILE_MANAGER_CONFLICT_RENAME) ==
          FILE_MANAGER_OK);
    poll_until_stopped(&status);
    CHECK(status.state == FILE_MANAGER_TASK_DONE);
    CHECK(status.processed_bytes == HOST_SOURCE_SIZE);
    CHECK(status.percent == 100U);
    CHECK(g_target_size == HOST_SOURCE_SIZE);
    CHECK(memcmp(g_source, g_target, HOST_SOURCE_SIZE) == 0);
    CHECK(g_committed == 1U);
    CHECK(strcmp(g_final_path, "0:/dst/source.bin") == 0);
    CHECK(g_session_count == 0U);
}

static void test_cancel_cleans_temporary_target(void)
{
    file_manager_status_t status;

    reset_fixture();
    CHECK(file_manager_copy_start("0:/source.bin", "0:/dst",
                                  FILE_MANAGER_CONFLICT_RENAME) ==
          FILE_MANAGER_OK);
    CHECK(file_manager_poll() == FILE_MANAGER_OK);
    CHECK(file_manager_cancel() == FILE_MANAGER_OK);
    file_manager_get_status(&status);
    CHECK(status.state == FILE_MANAGER_TASK_CANCELLED);
    CHECK(g_unlink_count == 1U);
    CHECK(g_committed == 0U);
    CHECK(g_session_count == 0U);
}

static void test_conflict_and_64_bit_size(void)
{
    file_manager_status_t status;

    reset_fixture();
    g_final_exists = 1U;
    CHECK(file_manager_copy_start("0:/source.bin", "0:/dst",
                                  FILE_MANAGER_CONFLICT_CANCEL) ==
          FILE_MANAGER_CANCELLED);
    CHECK(g_session_count == 0U);
    CHECK(file_manager_copy_start("0:/source.bin", "0:/dst",
                                  FILE_MANAGER_CONFLICT_RENAME) ==
          FILE_MANAGER_OK);
    CHECK(file_manager_cancel() == FILE_MANAGER_OK);

    reset_fixture();
    g_reported_size = 5ULL * 1024ULL * 1024ULL * 1024ULL;
    CHECK(file_manager_copy_start("0:/source.bin", "0:/dst",
                                  FILE_MANAGER_CONFLICT_RENAME) ==
          FILE_MANAGER_OK);
    file_manager_get_status(&status);
    CHECK(status.total_bytes == 5ULL * 1024ULL * 1024ULL * 1024ULL);
    CHECK(file_manager_cancel() == FILE_MANAGER_OK);
}

static void test_move_fast_path_and_copy_fallback(void)
{
    file_manager_status_t status;
    uint32_t guard;

    reset_fixture();
    CHECK(file_manager_move_start("0:/source.bin", "0:/dst",
                                  FILE_MANAGER_CONFLICT_RENAME) ==
          FILE_MANAGER_OK);
    file_manager_get_status(&status);
    CHECK(status.state == FILE_MANAGER_TASK_DONE);
    CHECK(strcmp(g_final_path, "0:/dst/source.bin") == 0);
    CHECK(g_source_deleted == 0U);
    CHECK(g_session_count == 0U);

    reset_fixture();
    g_source_rename_fails = 1U;
    CHECK(file_manager_move_start("0:/source.bin", "0:/dst",
                                  FILE_MANAGER_CONFLICT_RENAME) ==
          FILE_MANAGER_OK);
    guard = 16U;
    do
    {
        CHECK(file_manager_poll() == FILE_MANAGER_OK);
        file_manager_get_status(&status);
    } while ((status.state == FILE_MANAGER_TASK_MOVING) && (--guard != 0U));
    CHECK(guard != 0U);
    CHECK(status.state == FILE_MANAGER_TASK_DONE);
    CHECK(g_source_deleted == 1U);
    CHECK(g_committed == 1U);
    CHECK(g_session_count == 0U);
}

int main(void)
{
    test_stream_copy_and_commit();
    test_cancel_cleans_temporary_target();
    test_conflict_and_64_bit_size();
    test_move_fast_path_and_copy_fallback();

    if (g_failures != 0)
    {
        printf("FAIL file_manager_copy failures=%d\n", g_failures);
        return 1;
    }
    printf("PASS file_manager_copy\n");
    return 0;
}
