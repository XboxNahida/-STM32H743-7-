#include "file_manager_service.h"

#include <stddef.h>
#include <string.h>

#ifdef FILE_MANAGER_HOST_TEST
extern file_manager_result_t file_manager_host_session_acquire(void);
extern void file_manager_host_session_release(void);
extern file_manager_result_t file_manager_host_dir_open(const char *path);
extern file_manager_result_t file_manager_host_dir_read(
    char *name, uint32_t name_capacity, file_manager_entry_type_t *type,
    uint64_t *size, uint16_t *date, uint16_t *time, uint8_t *end);
extern void file_manager_host_dir_close(void);
extern file_manager_result_t file_manager_host_create(
    const char *path, file_manager_entry_type_t type);

#define file_manager_platform_session_acquire file_manager_host_session_acquire
#define file_manager_platform_session_release file_manager_host_session_release
#define file_manager_platform_dir_open         file_manager_host_dir_open
#define file_manager_platform_dir_read         file_manager_host_dir_read
#define file_manager_platform_dir_close        file_manager_host_dir_close
#define file_manager_platform_create           file_manager_host_create
#else
#include "file_manager_platform.h"
#define file_manager_platform_dir_open  file_manager_platform_list_dir_open
#define file_manager_platform_dir_read  file_manager_platform_list_dir_read
#define file_manager_platform_dir_close file_manager_platform_list_dir_close
#endif

file_manager_result_t file_manager_list_page(const char *path,
                                             uint32_t offset,
                                             file_manager_page_t *page)
{
    char normalized[FILE_MANAGER_PATH_CAPACITY];
    file_manager_entry_t entry;
    file_manager_result_t result;
    uint32_t visible_index = 0U;
    uint8_t end = 0U;
    uint8_t session_held = 0U;
    uint8_t directory_open = 0U;

    if ((path == NULL) || (page == NULL))
    {
        return FILE_MANAGER_INVALID_ARG;
    }
    memset(page, 0, sizeof(*page));
    result = file_manager_normalize_path(path, normalized,
                                         sizeof(normalized));
    if (result != FILE_MANAGER_OK)
    {
        return result;
    }
    result = file_manager_platform_session_acquire();
    if (result != FILE_MANAGER_OK)
    {
        return result;
    }
    session_held = 1U;
    result = file_manager_platform_dir_open(normalized);
    if (result != FILE_MANAGER_OK)
    {
        goto cleanup;
    }
    directory_open = 1U;

    while (end == 0U)
    {
        memset(&entry, 0, sizeof(entry));
        result = file_manager_platform_dir_read(
            entry.name, sizeof(entry.name), &entry.type, &entry.size,
            &entry.date, &entry.time, &end);
        if (result != FILE_MANAGER_OK)
        {
            goto cleanup;
        }
        if (end != 0U)
        {
            break;
        }
        if (visible_index++ < offset)
        {
            continue;
        }
        if (page->count < FILE_MANAGER_PAGE_SIZE)
        {
            page->entries[page->count] = entry;
            page->count++;
            continue;
        }
        page->has_more = 1U;
        break;
    }
    page->next_offset = offset + page->count;

cleanup:
    if (directory_open != 0U)
    {
        file_manager_platform_dir_close();
    }
    if (session_held != 0U)
    {
        file_manager_platform_session_release();
    }
    return result;
}

file_manager_result_t file_manager_create(const char *directory,
                                          const char *name,
                                          file_manager_entry_type_t type)
{
    char path[FILE_MANAGER_PATH_CAPACITY];
    file_manager_result_t result;

    if ((directory == NULL) || (name == NULL) ||
        ((type != FILE_MANAGER_ENTRY_FILE) &&
         (type != FILE_MANAGER_ENTRY_DIRECTORY)))
    {
        return FILE_MANAGER_INVALID_ARG;
    }
    result = file_manager_join_path(directory, name, path, sizeof(path));
    if (result != FILE_MANAGER_OK)
    {
        return result;
    }
    result = file_manager_platform_session_acquire();
    if (result != FILE_MANAGER_OK)
    {
        return result;
    }
    result = file_manager_platform_create(path, type);
    file_manager_platform_session_release();
    return result;
}
