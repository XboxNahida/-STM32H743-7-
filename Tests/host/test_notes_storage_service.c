#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "notes_storage_service.h"
#include "test_notes_storage_fixture.h"

static int test_busy_and_open_failure_release(void)
{
    int failures = 0;

    reset_fixture();
    g_acquire_result = 1;
    failures += expect_u32("busy", (uint32_t)notes_storage_open("0:/a.md"),
                           NOTES_STORAGE_BUSY);
    failures += expect_u32("busy_no_release", g_release_calls, 0U);

    reset_fixture();
    g_open_read_result = 1;
    failures += expect_u32("open_fail",
        (uint32_t)notes_storage_open("0:/a.md"), NOTES_STORAGE_IO);
    failures += expect_u32("open_fail_release", g_release_calls, 1U);

    reset_fixture();
    g_shared_size = TEST_SHARED_BYTES - 1U;
    failures += expect_u32("small_shared",
        (uint32_t)notes_storage_open("0:/a.md"), NOTES_STORAGE_CAPACITY);
    failures += expect_u32("small_shared_release", g_release_calls, 1U);
    return failures;
}

static int test_new_empty_document(void)
{
    notes_storage_status_t status;
    notes_document_t *document;
    int failures = 0;

    reset_fixture();
    failures += expect_u32("new_empty",
        (uint32_t)notes_storage_new(), NOTES_STORAGE_OK);
    document = notes_storage_get_document();
    failures += expect_u32("new_document", document != NULL, 1U);
    failures += expect_u32("new_length", notes_document_get_length(document), 0U);
    notes_storage_get_status(&status);
    failures += expect_u32("new_opened", status.opened, 1U);
    failures += expect_u32("new_writable", status.read_only, 0U);
    failures += expect_u32("new_busy",
        (uint32_t)notes_storage_new(), NOTES_STORAGE_BUSY);
    failures += expect_u32("new_save",
        (uint32_t)notes_storage_save("0:/Note/New.md"), NOTES_STORAGE_OK);
    notes_storage_close(0U);
    failures += expect_u32("new_release", g_release_calls, 1U);
    return failures;
}

static int test_utf8_edit_save_and_release(void)
{
    static const uint8_t initial[] = "hello\xE4\xB8\xAD";
    static const uint8_t suffix[] = "!";
    notes_storage_status_t status;
    notes_document_t *document;
    uint32_t cursor = 0U;
    int failures = 0;

    reset_fixture();
    memcpy(g_file_data, initial, sizeof(initial) - 1U);
    g_file_size = sizeof(initial) - 1U;
    failures += expect_u32("utf8_open",
        (uint32_t)notes_storage_open("0:/note.md"), NOTES_STORAGE_OK);
    document = notes_storage_get_document();
    failures += expect_u32("utf8_document", document != NULL, 1U);
    failures += expect_u32("utf8_length", notes_document_get_length(document),
                           sizeof(initial) - 1U);
    failures += expect_u32("append",
        (uint32_t)notes_document_replace_range(document,
            notes_document_get_length(document),
            notes_document_get_length(document), suffix, 1U, &cursor),
        NOTES_DOCUMENT_OK);
    failures += expect_u32("save",
        (uint32_t)notes_storage_save("0:/note.md"), NOTES_STORAGE_OK);
    failures += expect_u32("saved_bytes", g_written_size, sizeof(initial));
    failures += expect_u32("saved_suffix", g_written[g_written_size - 1U], '!');
    notes_storage_get_status(&status);
    failures += expect_u32("saved_not_modified", status.modified, 0U);
    notes_storage_close(0U);
    failures += expect_u32("close_release", g_release_calls, 1U);
    return failures;
}

static int test_gbk_conversion_and_growth_limit(void)
{
    static const uint8_t expected[] =
        "\xE4\xB8\xAD\xE6\x96\x87";
    notes_storage_status_t status;
    const notes_document_t *document;
    uint32_t index;
    int failures = 0;

    reset_fixture();
    g_file_data[0] = 0xD6U;
    g_file_data[1] = 0xD0U;
    g_file_data[2] = 0xCEU;
    g_file_data[3] = 0xC4U;
    g_file_size = 4U;
    failures += expect_u32("gbk_open",
        (uint32_t)notes_storage_open("0:/gbk.txt"), NOTES_STORAGE_OK);
    document = notes_storage_get_document();
    failures += expect_u32("gbk_length", notes_document_get_length(document),
                           sizeof(expected) - 1U);
    failures += expect_u32("gbk_text",
        memcmp(notes_document_get_data(document), expected,
               sizeof(expected) - 1U), 0U);
    notes_storage_close(1U);

    reset_fixture();
    for (index = 0U; index < 200000U; index += 2U)
    {
        g_file_data[index] = 0xD6U;
        g_file_data[index + 1U] = 0xD0U;
    }
    g_file_size = 200000U;
    failures += expect_u32("gbk_growth_open",
        (uint32_t)notes_storage_open("0:/large.txt"), NOTES_STORAGE_OK);
    notes_storage_get_status(&status);
    failures += expect_u32("gbk_growth_read_only", status.read_only, 1U);
    failures += expect_u32("gbk_growth_truncated", status.truncated, 1U);
    failures += expect_u32("gbk_growth_aligned",
                           status.document_bytes % 3U, 0U);
    failures += expect_u32("gbk_growth_save_reject",
        (uint32_t)notes_storage_save("0:/large.txt"),
        NOTES_STORAGE_READ_ONLY);
    failures += expect_u32("gbk_no_write_open", g_open_write_calls, 0U);
    notes_storage_close(1U);
    return failures;
}

static int test_source_limit_and_save_failures(void)
{
    notes_storage_status_t status;
    notes_document_t *document;
    uint32_t cursor = 0U;
    static const uint8_t suffix[] = "x";
    int failures = 0;

    reset_fixture();
    memset(g_file_data, 'a', NOTES_DOCUMENT_MAX_BYTES + 1U);
    g_file_size = NOTES_DOCUMENT_MAX_BYTES + 1U;
    failures += expect_u32("large_utf8_open",
        (uint32_t)notes_storage_open("0:/large.md"), NOTES_STORAGE_OK);
    notes_storage_get_status(&status);
    failures += expect_u32("large_utf8_read_only", status.read_only, 1U);
    failures += expect_u32("large_utf8_length", status.document_bytes,
                           NOTES_DOCUMENT_MAX_BYTES);
    notes_storage_close(1U);

    reset_fixture();
    memcpy(g_file_data, "abc", 3U);
    g_file_size = 3U;
    failures += expect_u32("write_test_open",
        (uint32_t)notes_storage_open("0:/write.md"), NOTES_STORAGE_OK);
    document = notes_storage_get_document();
    (void)notes_document_replace_range(document, 3U, 3U,
                                       suffix, 1U, &cursor);
    g_write_result = 1;
    failures += expect_u32("write_fail",
        (uint32_t)notes_storage_save("0:/write.md"), NOTES_STORAGE_IO);
    notes_storage_get_status(&status);
    failures += expect_u32("write_fail_modified", status.modified, 1U);
    g_write_result = 0;
    g_short_write_limit = 2U;
    failures += expect_u32("short_write_loop",
        (uint32_t)notes_storage_save("0:/write.md"), NOTES_STORAGE_OK);
    failures += expect_u32("short_write_complete", g_written_size, 4U);
    (void)notes_document_replace_range(document, 4U, 4U,
                                       suffix, 1U, &cursor);
    g_short_write_limit = 0U;
    g_close_result = 1;
    failures += expect_u32("close_fail",
        (uint32_t)notes_storage_save("0:/write.md"), NOTES_STORAGE_IO);
    notes_storage_get_status(&status);
    failures += expect_u32("close_fail_modified", status.modified, 1U);
    notes_storage_close(1U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_busy_and_open_failure_release();
    failures += test_new_empty_document();
    failures += test_utf8_edit_save_and_release();
    failures += test_gbk_conversion_and_growth_limit();
    failures += test_source_limit_and_save_failures();
    if (failures != 0)
    {
        printf("FAIL notes_storage_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS notes_storage_service\n");
    return 0;
}
