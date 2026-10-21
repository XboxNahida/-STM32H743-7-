#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nand_cache_store.h"

#define FAKE_PAGE_CAPACITY 16U

typedef struct
{
    uint32_t page;
    uint8_t main_area[NAND_CACHE_PAGE_MAIN_SIZE];
    uint8_t spare_area[NAND_CACHE_PAGE_SPARE_SIZE];
    uint8_t used;
} fake_page_t;

typedef struct
{
    fake_page_t pages[FAKE_PAGE_CAPACITY];
    uint32_t erase_count;
    uint32_t program_count;
    uint32_t fail_program_at;
    uint32_t bad_block;
} fake_nand_t;

typedef struct
{
    uint32_t calls;
    uint32_t completed;
    uint32_t total;
} progress_state_t;

static void record_progress(uint32_t completed, uint32_t total, void *context)
{
    progress_state_t *state = (progress_state_t *)context;

    state->calls++;
    state->completed = completed;
    state->total = total;
}

static fake_page_t *fake_find(fake_nand_t *fake, uint32_t page, uint8_t create)
{
    uint32_t index;
    fake_page_t *free_page = NULL;

    for (index = 0U; index < FAKE_PAGE_CAPACITY; index++)
    {
        if ((fake->pages[index].used != 0U) &&
            (fake->pages[index].page == page))
            return &fake->pages[index];
        if ((free_page == NULL) && (fake->pages[index].used == 0U))
            free_page = &fake->pages[index];
    }
    if ((create == 0U) || (free_page == NULL))
        return NULL;
    free_page->used = 1U;
    free_page->page = page;
    memset(free_page->main_area, 0xFF, sizeof(free_page->main_area));
    memset(free_page->spare_area, 0xFF, sizeof(free_page->spare_area));
    return free_page;
}

static int fake_erase(uint32_t block, void *context)
{
    fake_nand_t *fake = (fake_nand_t *)context;
    uint32_t index;

    fake->erase_count++;
    for (index = 0U; index < FAKE_PAGE_CAPACITY; index++)
    {
        if ((fake->pages[index].used != 0U) &&
            ((fake->pages[index].page / 64U) == block))
            fake->pages[index].used = 0U;
    }
    return 0;
}

static int fake_program(uint32_t page, const uint8_t *main_area,
                        const uint8_t *spare_area, void *context)
{
    fake_nand_t *fake = (fake_nand_t *)context;
    fake_page_t *stored;

    fake->program_count++;
    if ((fake->fail_program_at != 0U) &&
        (fake->program_count == fake->fail_program_at))
        return -1;
    stored = fake_find(fake, page, 1U);
    if (stored == NULL)
        return -1;
    memcpy(stored->main_area, main_area, sizeof(stored->main_area));
    memcpy(stored->spare_area, spare_area, sizeof(stored->spare_area));
    return 0;
}

static int fake_read(uint32_t page, uint8_t *main_area, uint8_t *spare_area,
                     void *context)
{
    fake_page_t *stored = fake_find((fake_nand_t *)context, page, 0U);

    if (stored == NULL)
        return -1;
    memcpy(main_area, stored->main_area, sizeof(stored->main_area));
    memcpy(spare_area, stored->spare_area, sizeof(stored->spare_area));
    return 0;
}

static int fake_is_bad(uint32_t block, void *context)
{
    fake_nand_t *fake = (fake_nand_t *)context;
    return (block == fake->bad_block) ? 1 : 0;
}

static void make_layout(nand_cache_layout_t *layout)
{
    uint32_t index;

    memset(layout, 0, sizeof(*layout));
    for (index = 0U; index < NAND_CACHE_SLOT_BLOCKS; index++)
    {
        layout->slot_blocks[0][index] = 10U + index;
        layout->slot_blocks[1][index] = 300U + index;
    }
    for (index = 0U; index < NAND_CACHE_METADATA_BLOCKS; index++)
        layout->metadata_blocks[index] = 600U + index;
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
    nand_cache_layout_t layout;
    nand_cache_store_ops_t ops;
    nand_cache_store_workspace_t workspace;
    nand_cache_metadata_t active;
    nand_cache_metadata_t scanned;
    fake_nand_t fake;
    uint8_t source[3000];
    uint8_t loaded[3000];
    progress_state_t progress;
    uint32_t index;
    int failures = 0;

    make_layout(&layout);
    memset(&fake, 0, sizeof(fake));
    fake.bad_block = UINT32_MAX;
    ops.erase_block = fake_erase;
    ops.program_page = fake_program;
    ops.read_page = fake_read;
    ops.block_is_bad = fake_is_bad;
    ops.context = &fake;
    for (index = 0U; index < sizeof(source); index++)
        source[index] = (uint8_t)(index * 29U + 3U);

    failures += expect(
        "write_empty_to_a",
        nand_cache_store_write(&layout, &ops, &workspace, NULL, 0U,
                               source, sizeof(source), &active),
        NAND_CACHE_STORE_OK);
    if ((active.slot != 0U) || (active.sequence != 1U) ||
        (active.page_count != 2U) ||
        (fake.erase_count != NAND_CACHE_SLOT_BLOCKS + 1U) ||
        (fake.program_count != 3U))
    {
        printf("FAIL first_commit_order\n");
        failures++;
    }
    failures += expect(
        "scan_committed",
        nand_cache_store_scan(&layout, &ops, &workspace, &scanned),
        NAND_CACHE_STORE_OK);
    if ((scanned.slot != 0U) || (scanned.sequence != 1U))
    {
        printf("FAIL scanned_first_commit\n");
        failures++;
    }
    memset(&progress, 0, sizeof(progress));
    failures += expect(
        "load_committed",
        nand_cache_store_load_progress(&layout, &ops, &workspace, &scanned,
                                       loaded, sizeof(loaded), NULL,
                                       record_progress, &progress),
        NAND_CACHE_STORE_OK);
    if ((progress.calls != scanned.page_count) ||
        (progress.completed != scanned.page_count) ||
        (progress.total != scanned.page_count))
    {
        printf("FAIL load_progress calls=%lu completed=%lu total=%lu\n",
               (unsigned long)progress.calls,
               (unsigned long)progress.completed,
               (unsigned long)progress.total);
        failures++;
    }
    if (memcmp(source, loaded, sizeof(source)) != 0)
    {
        printf("FAIL loaded_payload\n");
        failures++;
    }

    fake.fail_program_at = fake.program_count + 2U;
    failures += expect(
        "power_loss_before_commit",
        nand_cache_store_write(&layout, &ops, &workspace, &active, 1U,
                               source, sizeof(source), &scanned),
        NAND_CACHE_STORE_PROGRAM_ERROR);
    failures += expect(
        "old_commit_survives",
        nand_cache_store_scan(&layout, &ops, &workspace, &scanned),
        NAND_CACHE_STORE_OK);
    if ((scanned.slot != 0U) || (scanned.sequence != 1U))
    {
        printf("FAIL old_commit_changed\n");
        failures++;
    }

    fake.fail_program_at = fake.program_count + 3U;
    failures += expect(
        "power_loss_at_metadata_commit",
        nand_cache_store_write(&layout, &ops, &workspace, &active, 1U,
                               source, sizeof(source), &scanned),
        NAND_CACHE_STORE_PROGRAM_ERROR);
    failures += expect(
        "uncommitted_complete_slot_ignored",
        nand_cache_store_scan(&layout, &ops, &workspace, &scanned),
        NAND_CACHE_STORE_OK);
    if ((scanned.slot != 0U) || (scanned.sequence != 1U))
    {
        printf("FAIL incomplete_commit_selected\n");
        failures++;
    }

    fake.fail_program_at = 0U;
    fake.bad_block = layout.slot_blocks[1][0];
    failures += expect(
        "new_bad_block_rejected",
        nand_cache_store_write(&layout, &ops, &workspace, &active, 1U,
                               source, sizeof(source), &scanned),
        NAND_CACHE_STORE_BAD_BLOCK);

    fake.bad_block = UINT32_MAX;
    failures += expect(
        "write_next_to_b",
        nand_cache_store_write(&layout, &ops, &workspace, &active, 1U,
                               source, sizeof(source), &scanned),
        NAND_CACHE_STORE_OK);
    if ((scanned.slot != 1U) || (scanned.sequence != 2U))
    {
        printf("FAIL second_commit\n");
        failures++;
    }

    {
        uint32_t data_page;
        uint32_t corrected = 0U;
        fake_page_t *stored;

        nand_cache_slot_page_address(&layout, 1U, 0U, &data_page);
        stored = fake_find(&fake, data_page, 0U);
        if (stored == NULL)
        {
            printf("FAIL slot_data_missing\n");
            failures++;
        }
        else
        {
            stored->main_area[123] ^= 0x04U;
            failures += expect(
                "single_bit_load_corrected",
                nand_cache_store_load(&layout, &ops, &workspace, &scanned,
                                      loaded, sizeof(loaded), &corrected),
                NAND_CACHE_STORE_OK);
            if ((corrected != 1U) ||
                (memcmp(source, loaded, sizeof(source)) != 0))
            {
                printf("FAIL corrected_payload\n");
                failures++;
            }
            stored->main_area[321] ^= 0x08U;
            failures += expect(
                "multi_bit_load_rejected",
                nand_cache_store_load(&layout, &ops, &workspace, &scanned,
                                      loaded, sizeof(loaded), NULL),
                NAND_CACHE_STORE_PAGE_ERROR);
        }
    }

    {
        uint32_t metadata_page;
        fake_page_t *latest;

        nand_cache_slot_metadata_page_address(&layout, 1U, &metadata_page);
        latest = fake_find(&fake, metadata_page, 0U);
        if (latest == NULL)
        {
            printf("FAIL latest_metadata_missing\n");
            failures++;
        }
        else
        {
            latest->main_area[0] ^= 0x01U;
            latest->main_area[1] ^= 0x02U;
            failures += expect(
                "damaged_latest_falls_back",
                nand_cache_store_scan(&layout, &ops, &workspace, &scanned),
                NAND_CACHE_STORE_OK);
            if ((scanned.slot != 0U) || (scanned.sequence != 1U))
            {
                printf("FAIL damaged_latest_selected\n");
                failures++;
            }
        }
    }

    if (failures != 0)
    {
        printf("FAIL nand_cache_store failures=%d\n", failures);
        return 1;
    }
    printf("PASS nand_cache_store\n");
    return 0;
}
