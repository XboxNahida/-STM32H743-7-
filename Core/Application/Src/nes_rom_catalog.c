#include "nes_rom_catalog.h"

#include <stddef.h>
#include <string.h>

#define NES_ROM_CATALOG_SD_OWNER 0x4E46U

#ifdef NES_ROM_CATALOG_HOST_TEST
extern int nes_rom_catalog_platform_open(const char *path);
extern int nes_rom_catalog_platform_read(char *name, uint32_t name_size,
                                         uint32_t *size,
                                         uint8_t *directory,
                                         uint8_t *end);
extern void nes_rom_catalog_platform_close(void);
#else
#include "ff.h"
#include "sdcard_manager.h"

static DIR g_nes_catalog_directory;
static FILINFO g_nes_catalog_info;
static uint8_t g_nes_catalog_session;
static uint8_t g_nes_catalog_opened;

static int nes_rom_catalog_platform_open(const char *path)
{
    if (sdcard_manager_session_acquire(NES_ROM_CATALOG_SD_OWNER) !=
        SDCARD_MANAGER_OK)
    {
        return NES_ROM_CATALOG_NOT_READY;
    }
    g_nes_catalog_session = 1U;
    if (f_opendir(&g_nes_catalog_directory, path) != FR_OK)
    {
        (void)sdcard_manager_session_release(NES_ROM_CATALOG_SD_OWNER);
        g_nes_catalog_session = 0U;
        return NES_ROM_CATALOG_IO_ERROR;
    }
    g_nes_catalog_opened = 1U;
    return NES_ROM_CATALOG_OK;
}

static int nes_rom_catalog_platform_read(char *name, uint32_t name_size,
                                         uint32_t *size,
                                         uint8_t *directory,
                                         uint8_t *end)
{
    size_t length;

    if (f_readdir(&g_nes_catalog_directory, &g_nes_catalog_info) != FR_OK)
    {
        return NES_ROM_CATALOG_IO_ERROR;
    }
    if (g_nes_catalog_info.fname[0] == '\0')
    {
        name[0] = '\0';
        *size = 0U;
        *directory = 0U;
        *end = 1U;
        return NES_ROM_CATALOG_OK;
    }
    length = strlen(g_nes_catalog_info.fname);
    if (length >= name_size)
    {
        return NES_ROM_CATALOG_PATH_TOO_LONG;
    }
    memcpy(name, g_nes_catalog_info.fname, length + 1U);
    *size = (uint32_t)g_nes_catalog_info.fsize;
    *directory = ((g_nes_catalog_info.fattrib & AM_DIR) != 0U) ? 1U : 0U;
    *end = 0U;
    return NES_ROM_CATALOG_OK;
}

static void nes_rom_catalog_platform_close(void)
{
    if (g_nes_catalog_opened != 0U)
    {
        (void)f_closedir(&g_nes_catalog_directory);
        g_nes_catalog_opened = 0U;
    }
    if (g_nes_catalog_session != 0U)
    {
        (void)sdcard_manager_session_release(NES_ROM_CATALOG_SD_OWNER);
        g_nes_catalog_session = 0U;
    }
}
#endif

typedef struct
{
    nes_rom_catalog_status_t status;
    nes_rom_catalog_entry_t entries[NES_ROM_CATALOG_PAGE_SIZE];
} nes_rom_catalog_context_t;

static nes_rom_catalog_context_t g_nes_catalog;

static int nes_rom_catalog_is_nes(const char *name)
{
    size_t length = strlen(name);
    char a;
    char b;
    char c;

    if ((length < 4U) || (name[length - 4U] != '.'))
    {
        return 0;
    }
    a = name[length - 3U];
    b = name[length - 2U];
    c = name[length - 1U];
    if ((a >= 'A') && (a <= 'Z')) a = (char)(a + ('a' - 'A'));
    if ((b >= 'A') && (b <= 'Z')) b = (char)(b + ('a' - 'A'));
    if ((c >= 'A') && (c <= 'Z')) c = (char)(c + ('a' - 'A'));
    return ((a == 'n') && (b == 'e') && (c == 's')) ? 1 : 0;
}

static int nes_rom_catalog_store(const char *directory,
                                 const char *name,
                                 uint32_t size)
{
    nes_rom_catalog_entry_t *entry;
    size_t directory_length = strlen(directory);
    size_t name_length = strlen(name);
    uint8_t separator = (directory[directory_length - 1U] == '/') ? 0U : 1U;

    if ((name_length >= NES_ROM_CATALOG_NAME_SIZE) ||
        (directory_length + separator + name_length + 1U >
         NES_ROM_CATALOG_PATH_SIZE))
    {
        return NES_ROM_CATALOG_PATH_TOO_LONG;
    }
    entry = &g_nes_catalog.entries[g_nes_catalog.status.count];
    memcpy(entry->name, name, name_length + 1U);
    memcpy(entry->path, directory, directory_length);
    if (separator != 0U)
    {
        entry->path[directory_length++] = '/';
    }
    memcpy(entry->path + directory_length, name, name_length + 1U);
    entry->size = size;
    ++g_nes_catalog.status.count;
    return NES_ROM_CATALOG_OK;
}

void nes_rom_catalog_init(void)
{
    memset(&g_nes_catalog, 0, sizeof(g_nes_catalog));
    g_nes_catalog.status.result = NES_ROM_CATALOG_OK;
}

int nes_rom_catalog_scan_page(const char *directory, uint32_t first_index)
{
    char name[NES_ROM_CATALOG_NAME_SIZE];
    uint32_t size;
    uint8_t is_directory;
    uint8_t end;
    size_t directory_length;
    uint32_t valid_index = 0U;
    int result;

    if ((directory == NULL) || (directory[0] == '\0'))
    {
        return NES_ROM_CATALOG_INVALID_ARG;
    }
    directory_length = strlen(directory);
    if (directory_length >= (NES_ROM_CATALOG_PATH_SIZE - 1U))
    {
        return NES_ROM_CATALOG_PATH_TOO_LONG;
    }
    nes_rom_catalog_init();
    g_nes_catalog.status.first_index = first_index;
    result = nes_rom_catalog_platform_open(directory);
    if (result != NES_ROM_CATALOG_OK)
    {
        g_nes_catalog.status.result = result;
        return result;
    }

    for (;;)
    {
        result = nes_rom_catalog_platform_read(name, sizeof(name), &size,
                                               &is_directory, &end);
        if (result != NES_ROM_CATALOG_OK)
        {
            g_nes_catalog.status.count = 0U;
            break;
        }
        if (end != 0U)
        {
            break;
        }
        if ((is_directory != 0U) || (size == 0U) ||
            (nes_rom_catalog_is_nes(name) == 0))
        {
            continue;
        }
        if ((valid_index >= first_index) &&
            (g_nes_catalog.status.count < NES_ROM_CATALOG_PAGE_SIZE))
        {
            result = nes_rom_catalog_store(directory, name, size);
            if (result != NES_ROM_CATALOG_OK)
            {
                g_nes_catalog.status.count = 0U;
                break;
            }
        }
        ++valid_index;
    }
    nes_rom_catalog_platform_close();
    g_nes_catalog.status.total_count = valid_index;
    if (result == NES_ROM_CATALOG_OK)
    {
        g_nes_catalog.status.has_more =
            ((uint64_t)first_index + g_nes_catalog.status.count <
             g_nes_catalog.status.total_count) ? 1U : 0U;
    }
    g_nes_catalog.status.result = result;
    return result;
}

uint8_t nes_rom_catalog_count(void)
{
    return g_nes_catalog.status.count;
}

const nes_rom_catalog_entry_t *nes_rom_catalog_get(uint8_t index)
{
    return (index < g_nes_catalog.status.count) ?
        &g_nes_catalog.entries[index] : NULL;
}

void nes_rom_catalog_get_status(nes_rom_catalog_status_t *status)
{
    if (status != NULL)
    {
        *status = g_nes_catalog.status;
    }
}
