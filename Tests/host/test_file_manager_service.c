#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "file_manager_service.h"

#define HOST_ENTRY_COUNT 300U

static uint32_t g_dir_index;
static uint32_t g_session_count;
static uint8_t g_ready = 1U;
static uint8_t g_session_busy;
static char g_created_path[FILE_MANAGER_PATH_CAPACITY];
static file_manager_entry_type_t g_created_type;
static int g_failures;

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
    if (g_ready == 0U) return FILE_MANAGER_NOT_READY;
    if (g_session_busy != 0U) return FILE_MANAGER_BUSY;
    g_session_count++;
    return FILE_MANAGER_OK;
}

void file_manager_host_session_release(void)
{
    if (g_session_count > 0U) g_session_count--;
}

file_manager_result_t file_manager_host_dir_open(const char *path)
{
    if (strcmp(path, "0:/many") != 0) return FILE_MANAGER_NOT_FOUND;
    g_dir_index = 0U;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_host_dir_read(char *name,
                                                 uint32_t name_capacity,
                                                 file_manager_entry_type_t *type,
                                                 uint64_t *size,
                                                 uint16_t *date,
                                                 uint16_t *time,
                                                 uint8_t *end)
{
    int length;

    if (g_dir_index >= HOST_ENTRY_COUNT)
    {
        *end = 1U;
        return FILE_MANAGER_OK;
    }
    length = snprintf(name, name_capacity, "item%03lu.txt",
                      (unsigned long)g_dir_index);
    if ((length < 0) || ((uint32_t)length >= name_capacity))
        return FILE_MANAGER_IO_ERROR;
    *type = FILE_MANAGER_ENTRY_FILE;
    *size = (uint64_t)g_dir_index * 100ULL;
    *date = 0x5821U;
    *time = 0x6000U;
    *end = 0U;
    g_dir_index++;
    return FILE_MANAGER_OK;
}

void file_manager_host_dir_close(void)
{
}

file_manager_result_t file_manager_host_create(const char *path,
                                               file_manager_entry_type_t type)
{
    size_t length = strlen(path);

    if (strstr(path, "exists") != NULL) return FILE_MANAGER_EXISTS;
    if (length >= sizeof(g_created_path)) return FILE_MANAGER_PATH_TOO_LONG;
    memcpy(g_created_path, path, length + 1U);
    g_created_type = type;
    return FILE_MANAGER_OK;
}

static void reset_fixture(void)
{
    g_dir_index = 0U;
    g_session_count = 0U;
    g_ready = 1U;
    g_session_busy = 0U;
    g_created_path[0] = '\0';
    g_created_type = FILE_MANAGER_ENTRY_FILE;
}

static void test_directory_paging_has_no_128_item_limit(void)
{
    file_manager_page_t page;

    reset_fixture();
    CHECK(file_manager_list_page("0:/many", 0U, &page) == FILE_MANAGER_OK);
    CHECK(page.count == FILE_MANAGER_PAGE_SIZE);
    CHECK(page.has_more == 1U);
    CHECK(page.next_offset == FILE_MANAGER_PAGE_SIZE);
    CHECK(strcmp(page.entries[0].name, "item000.txt") == 0);

    CHECK(file_manager_list_page("0:/many", 288U, &page) == FILE_MANAGER_OK);
    CHECK(page.count == 12U);
    CHECK(page.has_more == 0U);
    CHECK(page.next_offset == 300U);
    CHECK(strcmp(page.entries[11].name, "item299.txt") == 0);
    CHECK(g_session_count == 0U);
}

static void test_create_file_and_directory(void)
{
    reset_fixture();
    CHECK(file_manager_create("0:/", "新建.txt",
                              FILE_MANAGER_ENTRY_FILE) == FILE_MANAGER_OK);
    CHECK(strcmp(g_created_path, "0:/新建.txt") == 0);
    CHECK(g_created_type == FILE_MANAGER_ENTRY_FILE);
    CHECK(g_session_count == 0U);

    CHECK(file_manager_create("0:/", "新目录",
                              FILE_MANAGER_ENTRY_DIRECTORY) == FILE_MANAGER_OK);
    CHECK(g_created_type == FILE_MANAGER_ENTRY_DIRECTORY);
    CHECK(file_manager_create("0:/", "../bad",
                              FILE_MANAGER_ENTRY_FILE) ==
          FILE_MANAGER_INVALID_NAME);
    CHECK(file_manager_create("0:/", "exists.txt",
                              FILE_MANAGER_ENTRY_FILE) == FILE_MANAGER_EXISTS);
    CHECK(g_session_count == 0U);
}

static void test_session_failures_release_cleanly(void)
{
    file_manager_page_t page;

    reset_fixture();
    g_ready = 0U;
    CHECK(file_manager_list_page("0:/many", 0U, &page) ==
          FILE_MANAGER_NOT_READY);
    CHECK(g_session_count == 0U);
    g_ready = 1U;
    g_session_busy = 1U;
    CHECK(file_manager_create("0:/", "a.txt",
                              FILE_MANAGER_ENTRY_FILE) == FILE_MANAGER_BUSY);
    CHECK(g_session_count == 0U);
}

int main(void)
{
    test_directory_paging_has_no_128_item_limit();
    test_create_file_and_directory();
    test_session_failures_release_cleanly();

    if (g_failures != 0)
    {
        printf("FAIL file_manager_service failures=%d\n", g_failures);
        return 1;
    }
    printf("PASS file_manager_service list_create\n");
    return 0;
}
