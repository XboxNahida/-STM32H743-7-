#include <stdio.h>
#include <string.h>

#include "nes_rom_catalog.h"

static uint32_t g_index;
static int g_open_result;
static int g_read_fail_at;
static int g_session_owned;
static int g_directory_open;
static uint32_t g_entry_limit;
static int g_long_name_at;

int nes_rom_catalog_platform_open(const char *path)
{
    (void)path;
    if (g_open_result != 0)
    {
        return g_open_result;
    }
    g_session_owned = 1;
    g_directory_open = 1;
    g_index = 0U;
    return 0;
}

int nes_rom_catalog_platform_read(char *name, uint32_t name_size,
                                  uint32_t *size, uint8_t *directory,
                                  uint8_t *end)
{
    int length;

    if ((g_read_fail_at >= 0) && ((int)g_index == g_read_fail_at))
    {
        return NES_ROM_CATALOG_IO_ERROR;
    }
    if (g_index >= g_entry_limit)
    {
        *end = 1U;
        return 0;
    }
    *directory = (g_index == 3U) ? 1U : 0U;
    if ((int)g_index == g_long_name_at)
    {
        if (name_size <= 100U)
        {
            return NES_ROM_CATALOG_PATH_TOO_LONG;
        }
        memset(name, 'L', 96U);
        memcpy(name + 96U, ".nes", 5U);
        length = 100;
    }
    else if (g_index == 1U)
    {
        length = snprintf(name, name_size, "README.txt");
    }
    else
    {
        length = snprintf(name, name_size, "game%03lu.nes",
                          (unsigned long)g_index);
    }
    *size = 1000U + g_index;
    ++g_index;
    if ((length < 0) || ((uint32_t)length >= name_size))
    {
        return 1;
    }
    *end = 0U;
    return 0;
}

void nes_rom_catalog_platform_close(void)
{
    g_directory_open = 0;
    g_session_owned = 0;
}

static void reset_fixture(void)
{
    g_index = 0U;
    g_open_result = 0;
    g_read_fail_at = -1;
    g_session_owned = 0;
    g_directory_open = 0;
    g_entry_limit = 17U;
    g_long_name_at = -1;
    nes_rom_catalog_init();
}

static int expect(const char *name, int condition)
{
    if (!condition)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    const nes_rom_catalog_entry_t *entry;
    nes_rom_catalog_status_t status;
    char long_path[NES_ROM_CATALOG_PATH_SIZE];
    int failures = 0;

    reset_fixture();
    failures += expect("null path",
        nes_rom_catalog_scan_page(NULL, 0U) == NES_ROM_CATALOG_INVALID_ARG);
    memset(long_path, 'x', sizeof(long_path));
    long_path[sizeof(long_path) - 1U] = '\0';
    failures += expect("long path",
        nes_rom_catalog_scan_page(long_path, 0U) ==
        NES_ROM_CATALOG_PATH_TOO_LONG);

    failures += expect("first page",
        nes_rom_catalog_scan_page("0:/SYSTEM/NES", 0U) == 0);
    failures += expect("first page count",
        nes_rom_catalog_count() == NES_ROM_CATALOG_PAGE_SIZE);
    nes_rom_catalog_get_status(&status);
    failures += expect("first page has more",
        status.first_index == 0U && status.has_more == 1U &&
        status.total_count == 15U);
    entry = nes_rom_catalog_get(0U);
    failures += expect("first entry",
        entry != NULL && strcmp(entry->name, "game000.nes") == 0 &&
        strcmp(entry->path, "0:/SYSTEM/NES/game000.nes") == 0);
    failures += expect("bounds",
        nes_rom_catalog_get(NES_ROM_CATALOG_PAGE_SIZE) == NULL);
    failures += expect("scan releases", !g_session_owned && !g_directory_open);

    failures += expect("second page",
        nes_rom_catalog_scan_page("0:/SYSTEM/NES", 12U) == 0);
    nes_rom_catalog_get_status(&status);
    entry = nes_rom_catalog_get(0U);
    failures += expect("second page contents",
        status.first_index == 12U && status.count == 3U &&
        status.has_more == 0U && entry != NULL &&
        strcmp(entry->name, "game014.nes") == 0);

    reset_fixture();
    g_entry_limit = 153U;
    g_long_name_at = 100;
    failures += expect("long name does not truncate catalog",
        nes_rom_catalog_scan_page("0:/SYSTEM/NES", 144U) ==
        NES_ROM_CATALOG_OK);
    nes_rom_catalog_get_status(&status);
    entry = nes_rom_catalog_get(0U);
    failures += expect("all 151 games reachable",
        status.first_index == 144U && status.count == 7U &&
        status.has_more == 0U && status.total_count == 151U && entry != NULL &&
        strcmp(entry->name, "game146.nes") == 0);

    reset_fixture();
    g_read_fail_at = 1;
    failures += expect("read failure",
        nes_rom_catalog_scan_page("0:/SYSTEM/NES", 0U) ==
        NES_ROM_CATALOG_IO_ERROR);
    nes_rom_catalog_get_status(&status);
    failures += expect("failure clears and releases",
        status.count == 0U && !g_session_owned && !g_directory_open);

    if (failures == 0)
    {
        printf("PASS nes_rom_catalog\n");
    }
    return failures;
}
