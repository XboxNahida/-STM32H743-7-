#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nand_cache_store.h"

typedef struct
{
    uint32_t page;
    uint8_t main_area[NAND_CACHE_PAGE_MAIN_SIZE];
    uint8_t spare_area[NAND_CACHE_PAGE_SPARE_SIZE];
    uint8_t used;
} stored_page_t;

typedef struct
{
    stored_page_t pages[4];
    const uint8_t *source;
    uint32_t source_size;
    uint32_t reader_calls;
    uint32_t erase_calls;
    uint8_t fail_reader;
} stream_fake_t;

static int fake_erase(uint32_t block, void *context)
{
    stream_fake_t *fake = context;
    (void)block;
    fake->erase_calls++;
    return 0;
}

static int fake_bad(uint32_t block, void *context)
{
    (void)block;
    (void)context;
    return 0;
}

static stored_page_t *find_page(stream_fake_t *fake, uint32_t page,
                                uint8_t create)
{
    uint32_t index;

    for (index = 0U; index < 4U; index++)
    {
        if ((fake->pages[index].used != 0U) &&
            (fake->pages[index].page == page))
            return &fake->pages[index];
        if ((create != 0U) && (fake->pages[index].used == 0U))
        {
            fake->pages[index].used = 1U;
            fake->pages[index].page = page;
            return &fake->pages[index];
        }
    }
    return NULL;
}

static int fake_program(uint32_t page, const uint8_t *main_area,
                        const uint8_t *spare_area, void *context)
{
    stored_page_t *stored = find_page(context, page, 1U);

    if (stored == NULL)
        return -1;
    memcpy(stored->main_area, main_area, sizeof(stored->main_area));
    memcpy(stored->spare_area, spare_area, sizeof(stored->spare_area));
    return 0;
}

static int fake_read(uint32_t page, uint8_t *main_area, uint8_t *spare_area,
                     void *context)
{
    stored_page_t *stored = find_page(context, page, 0U);

    if (stored == NULL)
        return -1;
    memcpy(main_area, stored->main_area, sizeof(stored->main_area));
    memcpy(spare_area, stored->spare_area, sizeof(stored->spare_area));
    return 0;
}

static int fake_reader(uint32_t offset, uint8_t *destination,
                       uint32_t length, void *context)
{
    stream_fake_t *fake = context;

    fake->reader_calls++;
    if ((fake->fail_reader != 0U) ||
        (offset > fake->source_size) ||
        (length > fake->source_size - offset))
        return -1;
    memcpy(destination, &fake->source[offset], length);
    return 0;
}

static int expect(const char *name, int actual, int expected)
{
    if (actual == expected)
        return 0;
    printf("FAIL %s actual=%d expected=%d\n", name, actual, expected);
    return 1;
}

int main(void)
{
    uint8_t source[3000];
    uint8_t loaded[3000];
    nand_cache_layout_t layout;
    nand_cache_store_ops_t ops;
    nand_cache_store_workspace_t workspace;
    nand_cache_metadata_t metadata;
    stream_fake_t fake;
    uint32_t index;
    int failures = 0;

    memset(&layout, 0, sizeof(layout));
    memset(&fake, 0, sizeof(fake));
    for (index = 0U; index < NAND_CACHE_SLOT_BLOCKS; index++)
    {
        layout.slot_blocks[0][index] = 10U + index;
        layout.slot_blocks[1][index] = 300U + index;
    }
    for (index = 0U; index < NAND_CACHE_METADATA_BLOCKS; index++)
        layout.metadata_blocks[index] = 600U + index;
    for (index = 0U; index < sizeof(source); index++)
        source[index] = (uint8_t)(index * 17U + 9U);
    fake.source = source;
    fake.source_size = sizeof(source);
    ops.erase_block = fake_erase;
    ops.program_page = fake_program;
    ops.read_page = fake_read;
    ops.block_is_bad = fake_bad;
    ops.context = &fake;

    failures += expect(
        "stream_write",
        nand_cache_store_write_stream(&layout, &ops, &workspace, NULL, 0U,
                                      sizeof(source), fake_reader, &fake,
                                      &metadata),
        NAND_CACHE_STORE_OK);
    if ((fake.reader_calls != 4U) ||
        (fake.erase_calls != NAND_CACHE_SLOT_BLOCKS + 1U))
    {
        printf("FAIL two_pass_counts reader=%lu erase=%lu\n",
               (unsigned long)fake.reader_calls,
               (unsigned long)fake.erase_calls);
        failures++;
    }
    failures += expect(
        "stream_load",
        nand_cache_store_load(&layout, &ops, &workspace, &metadata,
                              loaded, sizeof(loaded), NULL),
        NAND_CACHE_STORE_OK);
    if (memcmp(source, loaded, sizeof(source)) != 0)
    {
        printf("FAIL stream_payload\n");
        failures++;
    }

    memset(&fake.pages, 0, sizeof(fake.pages));
    fake.reader_calls = 0U;
    fake.erase_calls = 0U;
    fake.fail_reader = 1U;
    failures += expect(
        "reader_failure_before_erase",
        nand_cache_store_write_stream(&layout, &ops, &workspace, NULL, 0U,
                                      sizeof(source), fake_reader, &fake,
                                      &metadata),
        NAND_CACHE_STORE_READ_ERROR);
    if (fake.erase_calls != 0U)
    {
        printf("FAIL reader_failure_erased_slot\n");
        failures++;
    }

    if (failures != 0)
    {
        printf("FAIL nand_cache_store_stream failures=%d\n", failures);
        return 1;
    }
    printf("PASS nand_cache_store_stream\n");
    return 0;
}
