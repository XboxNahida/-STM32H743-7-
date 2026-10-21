#include "file_manager_platform.h"

#include <string.h>

#include "ff.h"
#include "sdcard_manager.h"
#include "sdram_layout.h"

#define FILE_MANAGER_SD_OWNER 0x464DU
#define FILE_MANAGER_MAX_DIRS 33U

static DIR g_directories[FILE_MANAGER_MAX_DIRS];
static DIR g_list_directory;
static FIL g_source;
static FIL g_target;
static uint8_t g_source_open;
static uint8_t g_target_open;
static uint8_t g_session_depth;

static file_manager_result_t file_manager_map_result(FRESULT result)
{
    switch (result)
    {
    case FR_OK: return FILE_MANAGER_OK;
    case FR_NOT_READY: case FR_NOT_ENABLED: return FILE_MANAGER_NOT_READY;
    case FR_NO_FILE: case FR_NO_PATH: return FILE_MANAGER_NOT_FOUND;
    case FR_EXIST: return FILE_MANAGER_EXISTS;
    case FR_INVALID_NAME: return FILE_MANAGER_INVALID_NAME;
    case FR_DENIED: return FILE_MANAGER_NO_SPACE;
    case FR_TIMEOUT: case FR_LOCKED: return FILE_MANAGER_BUSY;
    default: return FILE_MANAGER_IO_ERROR;
    }
}

file_manager_result_t file_manager_platform_session_acquire(void)
{
    if (g_session_depth != 0U)
    {
        if (g_session_depth == 0xFFU) return FILE_MANAGER_BUSY;
        g_session_depth++;
        return FILE_MANAGER_OK;
    }
    if (sdcard_manager_session_acquire(FILE_MANAGER_SD_OWNER) !=
        SDCARD_MANAGER_OK) return FILE_MANAGER_NOT_READY;
    g_session_depth = 1U;
    return FILE_MANAGER_OK;
}

void file_manager_platform_session_release(void)
{
    if (g_session_depth == 0U) return;
    g_session_depth--;
    if (g_session_depth == 0U)
        (void)sdcard_manager_session_release(FILE_MANAGER_SD_OWNER);
}

uint8_t *file_manager_platform_transfer_buffer(uint32_t *capacity)
{
    if (capacity != NULL) *capacity = SDRAM_FILE_TASK_SIZE;
    return (uint8_t *)SDRAM_FILE_TASK_ADDR;
}

file_manager_result_t file_manager_platform_list_dir_open(const char *path)
{
    if (path == NULL) return FILE_MANAGER_INVALID_ARG;
    return file_manager_map_result(f_opendir(&g_list_directory, path));
}

file_manager_result_t file_manager_platform_list_dir_read(
    char *name, uint32_t capacity, file_manager_entry_type_t *type,
    uint64_t *size, uint16_t *date, uint16_t *time, uint8_t *end)
{
    FILINFO info;
    FRESULT result;
    size_t length;

    if ((name == NULL) || (capacity == 0U) || (type == NULL) ||
        (size == NULL) || (date == NULL) || (time == NULL) || (end == NULL))
        return FILE_MANAGER_INVALID_ARG;
    memset(&info, 0, sizeof(info));
    result = f_readdir(&g_list_directory, &info);
    if (result != FR_OK) return file_manager_map_result(result);
    if (info.fname[0] == '\0') { *end = 1U; return FILE_MANAGER_OK; }
    length = strlen(info.fname);
    if (length >= capacity) return FILE_MANAGER_PATH_TOO_LONG;
    memcpy(name, info.fname, length + 1U);
    *type = ((info.fattrib & AM_DIR) != 0U) ? FILE_MANAGER_ENTRY_DIRECTORY : FILE_MANAGER_ENTRY_FILE;
    *size = (uint64_t)info.fsize;
    *date = info.fdate;
    *time = info.ftime;
    *end = 0U;
    return FILE_MANAGER_OK;
}

void file_manager_platform_list_dir_close(void)
{
    (void)f_closedir(&g_list_directory);
}

file_manager_result_t file_manager_platform_dir_open(uint32_t depth, const char *path)
{
    if ((depth >= FILE_MANAGER_MAX_DIRS) || (path == NULL)) return FILE_MANAGER_INVALID_ARG;
    return file_manager_map_result(f_opendir(&g_directories[depth], path));
}

file_manager_result_t file_manager_platform_dir_read(uint32_t depth, const char *path,
    char *name, uint32_t capacity, file_manager_entry_type_t *type,
    uint64_t *size, uint8_t *end)
{
    FILINFO info;
    FRESULT result;
    size_t length;

    (void)path;
    if ((depth >= FILE_MANAGER_MAX_DIRS) || (name == NULL) || (capacity == 0U) ||
        (type == NULL) || (size == NULL) || (end == NULL)) return FILE_MANAGER_INVALID_ARG;
    memset(&info, 0, sizeof(info));
    result = f_readdir(&g_directories[depth], &info);
    if (result != FR_OK) return file_manager_map_result(result);
    if (info.fname[0] == '\0') { *end = 1U; return FILE_MANAGER_OK; }
    length = strlen(info.fname);
    if (length >= capacity) return FILE_MANAGER_PATH_TOO_LONG;
    memcpy(name, info.fname, length + 1U);
    *type = ((info.fattrib & AM_DIR) != 0U) ? FILE_MANAGER_ENTRY_DIRECTORY : FILE_MANAGER_ENTRY_FILE;
    *size = (uint64_t)info.fsize;
    *end = 0U;
    return FILE_MANAGER_OK;
}

void file_manager_platform_dir_close(uint32_t depth)
{
    if (depth < FILE_MANAGER_MAX_DIRS) (void)f_closedir(&g_directories[depth]);
}

file_manager_result_t file_manager_platform_create(const char *path, file_manager_entry_type_t type)
{
    FIL file;
    FRESULT result;

    if ((path == NULL) || ((type != FILE_MANAGER_ENTRY_FILE) && (type != FILE_MANAGER_ENTRY_DIRECTORY)))
        return FILE_MANAGER_INVALID_ARG;
    if (type == FILE_MANAGER_ENTRY_DIRECTORY) return file_manager_map_result(f_mkdir(path));
    result = f_open(&file, path, FA_CREATE_NEW | FA_WRITE);
    if (result == FR_OK) result = f_close(&file);
    return file_manager_map_result(result);
}

file_manager_result_t file_manager_platform_stat(const char *path, file_manager_entry_type_t *type, uint64_t *size)
{
    FILINFO info;
    FRESULT result;

    if ((path == NULL) || (type == NULL) || (size == NULL)) return FILE_MANAGER_INVALID_ARG;
    memset(&info, 0, sizeof(info));
    result = f_stat(path, &info);
    if (result != FR_OK) return file_manager_map_result(result);
    *type = ((info.fattrib & AM_DIR) != 0U) ? FILE_MANAGER_ENTRY_DIRECTORY : FILE_MANAGER_ENTRY_FILE;
    *size = (uint64_t)info.fsize;
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_platform_unlink(const char *path)
{ return (path == NULL) ? FILE_MANAGER_INVALID_ARG : file_manager_map_result(f_unlink(path)); }

file_manager_result_t file_manager_platform_rename(const char *old_path, const char *new_path)
{
    if ((old_path == NULL) || (new_path == NULL)) return FILE_MANAGER_INVALID_ARG;
    return file_manager_map_result(f_rename(old_path, new_path));
}

file_manager_result_t file_manager_platform_mkdir(const char *path)
{ return (path == NULL) ? FILE_MANAGER_INVALID_ARG : file_manager_map_result(f_mkdir(path)); }

file_manager_result_t file_manager_platform_source_open(const char *path)
{
    FRESULT result;
    if (g_source_open != 0U) return FILE_MANAGER_BUSY;
    result = f_open(&g_source, path, FA_READ);
    if (result == FR_OK) g_source_open = 1U;
    return file_manager_map_result(result);
}

file_manager_result_t file_manager_platform_source_read(uint8_t *data, uint32_t requested, uint32_t *actual)
{
    UINT bytes = 0U;
    FRESULT result;
    if ((g_source_open == 0U) || (data == NULL) || (actual == NULL)) return FILE_MANAGER_NOT_READY;
    result = f_read(&g_source, data, (UINT)requested, &bytes);
    *actual = (uint32_t)bytes;
    return file_manager_map_result(result);
}

void file_manager_platform_source_close(void)
{
    if (g_source_open != 0U) { (void)f_close(&g_source); g_source_open = 0U; }
}

file_manager_result_t file_manager_platform_target_open(const char *path)
{
    FRESULT result;
    if (g_target_open != 0U) return FILE_MANAGER_BUSY;
    result = f_open(&g_target, path, FA_CREATE_NEW | FA_WRITE);
    if (result == FR_OK) g_target_open = 1U;
    return file_manager_map_result(result);
}

file_manager_result_t file_manager_platform_target_write(const uint8_t *data, uint32_t requested, uint32_t *actual)
{
    UINT bytes = 0U;
    FRESULT result;
    if ((g_target_open == 0U) || (data == NULL) || (actual == NULL)) return FILE_MANAGER_NOT_READY;
    result = f_write(&g_target, data, (UINT)requested, &bytes);
    *actual = (uint32_t)bytes;
    return file_manager_map_result(result);
}

file_manager_result_t file_manager_platform_target_sync(void)
{ return (g_target_open == 0U) ? FILE_MANAGER_NOT_READY : file_manager_map_result(f_sync(&g_target)); }

void file_manager_platform_target_close(void)
{
    if (g_target_open != 0U) { (void)f_close(&g_target); g_target_open = 0U; }
}
