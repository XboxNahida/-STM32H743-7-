#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "notes_document.h"

static uint8_t g_storage[NOTES_DOCUMENT_MAX_BYTES + 1U];
static uint8_t g_work[NOTES_DOCUMENT_WORK_BYTES];
static uint8_t g_large[NOTES_DOCUMENT_MAX_BYTES + 1U];

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n", name,
               (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

static int expect_result(const char *name, notes_document_result_t actual,
                         notes_document_result_t expected)
{
    return expect_u32(name, (uint32_t)actual, (uint32_t)expected);
}

static int expect_bytes(const char *name, const uint8_t *actual,
                        const uint8_t *expected, uint32_t length)
{
    if (memcmp(actual, expected, length) != 0)
    {
        printf("FAIL %s length=%lu\n", name, (unsigned long)length);
        return 1;
    }
    return 0;
}

static int test_init_and_utf8_validation(void)
{
    static const uint8_t valid[] = "A\xE4\xB8\xAD\xE6\x96\x87";
    static const uint8_t invalid[] = {0x41U, 0xE4U, 0x41U};
    notes_document_t document;
    int failures = 0;

    failures += expect_result("small_storage",
        notes_document_init(&document, g_storage,
                            NOTES_DOCUMENT_MAX_BYTES,
                            g_work, sizeof(g_work)),
        NOTES_DOCUMENT_INVALID_ARG);
    failures += expect_result("small_work",
        notes_document_init(&document, g_storage, sizeof(g_storage),
                            g_work, NOTES_DOCUMENT_WORK_BYTES - 1U),
        NOTES_DOCUMENT_INVALID_ARG);
    failures += expect_result("init",
        notes_document_init(&document, g_storage, sizeof(g_storage),
                            g_work, sizeof(g_work)),
        NOTES_DOCUMENT_OK);
    failures += expect_result("load_valid",
        notes_document_load_utf8(&document, valid, sizeof(valid) - 1U),
        NOTES_DOCUMENT_OK);
    failures += expect_u32("valid_length", notes_document_get_length(&document),
                           sizeof(valid) - 1U);
    failures += expect_result("reject_invalid",
        notes_document_load_utf8(&document, invalid, sizeof(invalid)),
        NOTES_DOCUMENT_INVALID_UTF8);
    failures += expect_u32("invalid_keeps_length",
                           notes_document_get_length(&document),
                           sizeof(valid) - 1U);
    failures += expect_bytes("invalid_keeps_text", g_storage, valid,
                             sizeof(valid) - 1U);
    return failures;
}

static int test_window_is_bounded_and_utf8_aligned(void)
{
    notes_document_t document;
    notes_document_window_t window;
    uint32_t index;
    int failures = 0;

    for (index = 0U; index + 3U <= 60000U; index += 3U)
    {
        g_large[index] = 0xE4U;
        g_large[index + 1U] = 0xB8U;
        g_large[index + 2U] = 0xADU;
    }
    failures += expect_result("window_init",
        notes_document_init(&document, g_storage, sizeof(g_storage),
                            g_work, sizeof(g_work)), NOTES_DOCUMENT_OK);
    failures += expect_result("window_load",
        notes_document_load_utf8(&document, g_large, 60000U),
        NOTES_DOCUMENT_OK);
    failures += expect_result("window_build",
        notes_document_build_window(&document, 40002U, &window),
        NOTES_DOCUMENT_OK);
    failures += expect_u32("window_max",
                           (window.length_bytes <= NOTES_WINDOW_MAX_BYTES), 1U);
    failures += expect_u32("window_nonzero_start", (window.start_byte > 0U), 1U);
    failures += expect_u32("window_start_boundary", window.start_byte % 3U, 0U);
    failures += expect_u32("window_end_boundary",
                           (window.start_byte + window.length_bytes) % 3U, 0U);
    failures += expect_u32("window_contains_focus",
        (window.start_byte <= 40002U &&
         window.start_byte + window.length_bytes >= 40002U), 1U);
    failures += expect_bytes("window_content", (const uint8_t *)window.text,
                             g_storage + window.start_byte,
                             window.length_bytes);
    failures += expect_u32("window_terminated",
                           (uint8_t)window.text[window.length_bytes], 0U);
    return failures;
}

static int test_commit_window_and_cursor_translation(void)
{
    static const uint8_t initial[] = "hello\xE4\xB8\x96\xE7\x95\x8C";
    static const char edited[] = "hello,\xE4\xB8\x96\xE7\x95\x8C!";
    notes_document_t document;
    notes_document_window_t window;
    uint32_t cursor_byte = 0U;
    uint8_t output[64];
    uint32_t written = 0U;
    int failures = 0;

    failures += expect_result("commit_init",
        notes_document_init(&document, g_storage, sizeof(g_storage),
                            g_work, sizeof(g_work)), NOTES_DOCUMENT_OK);
    failures += expect_result("commit_load",
        notes_document_load_utf8(&document, initial, sizeof(initial) - 1U),
        NOTES_DOCUMENT_OK);
    failures += expect_result("commit_build",
        notes_document_build_window(&document, 0U, &window),
        NOTES_DOCUMENT_OK);
    failures += expect_result("commit_edit",
        notes_document_commit_window(&document, edited,
                                     (uint32_t)strlen(edited), 9U,
                                     &cursor_byte), NOTES_DOCUMENT_OK);
    failures += expect_u32("commit_length", notes_document_get_length(&document),
                           (uint32_t)strlen(edited));
    failures += expect_u32("cursor_at_end", cursor_byte,
                           (uint32_t)strlen(edited));
    failures += expect_result("commit_read",
        notes_document_read_range(&document, 0U,
                                  notes_document_get_length(&document),
                                  output, sizeof(output), &written),
        NOTES_DOCUMENT_OK);
    failures += expect_u32("commit_written", written,
                           (uint32_t)strlen(edited));
    failures += expect_bytes("commit_text", output,
                             (const uint8_t *)edited, written + 1U);
    return failures;
}

static int test_range_replace_reverse_and_atomic_capacity(void)
{
    static const uint8_t initial[] = "0123456789";
    static const uint8_t replacement[] = "AB";
    static const uint8_t expected[] = "012AB789";
    notes_document_t document;
    uint32_t cursor_byte = 0U;
    uint32_t written = 0U;
    uint8_t output[32];
    int failures = 0;

    failures += expect_result("range_init",
        notes_document_init(&document, g_storage, sizeof(g_storage),
                            g_work, sizeof(g_work)), NOTES_DOCUMENT_OK);
    failures += expect_result("range_load",
        notes_document_load_utf8(&document, initial, sizeof(initial) - 1U),
        NOTES_DOCUMENT_OK);
    failures += expect_result("reverse_replace",
        notes_document_replace_range(&document, 7U, 3U,
                                     replacement, sizeof(replacement) - 1U,
                                     &cursor_byte), NOTES_DOCUMENT_OK);
    failures += expect_u32("replace_cursor", cursor_byte, 5U);
    failures += expect_result("reverse_read",
        notes_document_read_range(&document, 8U, 2U,
                                  output, sizeof(output), &written),
        NOTES_DOCUMENT_OK);
    failures += expect_u32("reverse_read_length", written, 6U);
    failures += expect_bytes("replace_text", g_storage, expected,
                             sizeof(expected));

    memset(g_large, 'x', NOTES_DOCUMENT_MAX_BYTES);
    failures += expect_result("capacity_load",
        notes_document_load_utf8(&document, g_large,
                                 NOTES_DOCUMENT_MAX_BYTES),
        NOTES_DOCUMENT_OK);
    failures += expect_result("capacity_reject",
        notes_document_replace_range(&document, 1U, 1U,
                                     replacement, 1U, &cursor_byte),
        NOTES_DOCUMENT_CAPACITY);
    failures += expect_u32("capacity_length_unchanged",
                           notes_document_get_length(&document),
                           NOTES_DOCUMENT_MAX_BYTES);
    failures += expect_u32("capacity_data_unchanged", g_storage[1], 'x');
    return failures;
}

static int test_reject_mid_character_ranges(void)
{
    static const uint8_t text[] = "A\xE4\xB8\xAD" "B";
    static const uint8_t replacement[] = "X";
    notes_document_t document;
    uint32_t cursor_byte = 99U;
    uint8_t output[16];
    uint32_t written = 99U;
    int failures = 0;

    failures += expect_result("boundary_init",
        notes_document_init(&document, g_storage, sizeof(g_storage),
                            g_work, sizeof(g_work)), NOTES_DOCUMENT_OK);
    failures += expect_result("boundary_load",
        notes_document_load_utf8(&document, text, sizeof(text) - 1U),
        NOTES_DOCUMENT_OK);
    failures += expect_result("boundary_replace_reject",
        notes_document_replace_range(&document, 2U, 4U,
                                     replacement, sizeof(replacement) - 1U,
                                     &cursor_byte), NOTES_DOCUMENT_RANGE);
    failures += expect_result("boundary_read_reject",
        notes_document_read_range(&document, 0U, 2U,
                                  output, sizeof(output), &written),
        NOTES_DOCUMENT_RANGE);
    failures += expect_bytes("boundary_text_unchanged", g_storage, text,
                             sizeof(text));
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_init_and_utf8_validation();
    failures += test_window_is_bounded_and_utf8_aligned();
    failures += test_commit_window_and_cursor_translation();
    failures += test_range_replace_reverse_and_atomic_capacity();
    failures += test_reject_mid_character_ranges();

    if (failures != 0)
    {
        printf("FAIL notes_document failures=%d\n", failures);
        return 1;
    }
    printf("PASS notes_document\n");
    return 0;
}
