#include "notes_storage_service.h"

#include <stddef.h>
#include <string.h>

#include "notes_storage_codec.h"
#include "notes_storage_platform.h"

#define NOTES_STORAGE_PATH_BYTES 300U
#define NOTES_STORAGE_HALF_BYTES (NOTES_DOCUMENT_MAX_BYTES + 1U)

static notes_storage_status_t g_notes_status;
static notes_document_t g_notes_document;
static uint8_t *g_notes_shared;
static uint8_t g_notes_owner_held;
static char g_notes_path[NOTES_STORAGE_PATH_BYTES];

static uint8_t notes_storage_copy_path(char *destination, const char *path)
{
    uint32_t index;

    if ((destination == NULL) || (path == NULL) || (path[0] == '\0'))
    {
        return 0U;
    }
    for (index = 0U; index < (NOTES_STORAGE_PATH_BYTES - 1U); index++)
    {
        destination[index] = path[index];
        if (path[index] == '\0')
        {
            return 1U;
        }
    }
    destination[0] = '\0';
    return 0U;
}

static int notes_storage_fail_open(notes_storage_result_t result,
                                   uint8_t file_open)
{
    if (file_open != 0U)
    {
        (void)notes_platform_close();
    }
    if (g_notes_owner_held != 0U)
    {
        notes_platform_release();
        g_notes_owner_held = 0U;
    }
    g_notes_status.opened = 0U;
    g_notes_status.result = (int)result;
    return (int)result;
}

int notes_storage_open(const char *path)
{
    uint8_t *raw;
    uint32_t shared_size = 0U;
    uint32_t source_bytes;
    uint32_t target_bytes;
    uint32_t total_read = 0U;
    uint32_t document_bytes = 0U;
    uint8_t truncated = 0U;
    notes_storage_result_t decode_result;

    if (!notes_storage_copy_path(g_notes_path, path))
    {
        return NOTES_STORAGE_INVALID_ARG;
    }
    if (g_notes_status.opened != 0U)
    {
        return NOTES_STORAGE_BUSY;
    }
    memset(&g_notes_status, 0, sizeof(g_notes_status));
    if ((notes_platform_acquire(&g_notes_shared, &shared_size) != 0) ||
        (g_notes_shared == NULL))
    {
        g_notes_shared = NULL;
        g_notes_status.result = NOTES_STORAGE_BUSY;
        return NOTES_STORAGE_BUSY;
    }
    g_notes_owner_held = 1U;
    if (shared_size < (NOTES_STORAGE_HALF_BYTES * 2U))
    {
        return notes_storage_fail_open(NOTES_STORAGE_CAPACITY, 0U);
    }
    if (notes_document_init(&g_notes_document, g_notes_shared,
                            NOTES_STORAGE_HALF_BYTES,
                            g_notes_shared + NOTES_STORAGE_HALF_BYTES,
                            NOTES_STORAGE_HALF_BYTES) != NOTES_DOCUMENT_OK)
    {
        return notes_storage_fail_open(NOTES_STORAGE_CAPACITY, 0U);
    }
    if (notes_platform_open_read(path) != 0)
    {
        return notes_storage_fail_open(NOTES_STORAGE_IO, 0U);
    }
    source_bytes = notes_platform_size();
    target_bytes = source_bytes;
    if (target_bytes > NOTES_STORAGE_HALF_BYTES)
    {
        target_bytes = NOTES_STORAGE_HALF_BYTES;
    }
    raw = g_notes_shared + NOTES_STORAGE_HALF_BYTES;
    while (total_read < target_bytes)
    {
        uint32_t actual = 0U;
        if ((notes_platform_read(raw + total_read, target_bytes - total_read,
                                 &actual) != 0) || (actual == 0U))
        {
            return notes_storage_fail_open(NOTES_STORAGE_IO, 1U);
        }
        total_read += actual;
    }
    if (notes_platform_close() != 0)
    {
        return notes_storage_fail_open(NOTES_STORAGE_IO, 0U);
    }
    decode_result = notes_storage_decode_text(raw, total_read, source_bytes,
        g_notes_shared, NOTES_STORAGE_HALF_BYTES, notes_platform_oem2uni,
        &document_bytes, &truncated);
    if ((decode_result != NOTES_STORAGE_OK) ||
        (notes_document_load_utf8(&g_notes_document, g_notes_shared,
                                  document_bytes) != NOTES_DOCUMENT_OK))
    {
        return notes_storage_fail_open(NOTES_STORAGE_INVALID_ENCODING, 0U);
    }
    g_notes_status.opened = 1U;
    g_notes_status.read_only = truncated;
    g_notes_status.truncated = truncated;
    g_notes_status.source_bytes = source_bytes;
    g_notes_status.document_bytes = document_bytes;
    g_notes_status.result = NOTES_STORAGE_OK;
    return NOTES_STORAGE_OK;
}

int notes_storage_new(void)
{
    uint32_t shared_size = 0U;

    if (g_notes_status.opened != 0U)
    {
        return NOTES_STORAGE_BUSY;
    }
    memset(&g_notes_status, 0, sizeof(g_notes_status));
    if ((notes_platform_acquire(&g_notes_shared, &shared_size) != 0) ||
        (g_notes_shared == NULL))
    {
        g_notes_shared = NULL;
        g_notes_status.result = NOTES_STORAGE_BUSY;
        return NOTES_STORAGE_BUSY;
    }
    g_notes_owner_held = 1U;
    if ((shared_size < (NOTES_STORAGE_HALF_BYTES * 2U)) ||
        (notes_document_init(&g_notes_document, g_notes_shared,
                             NOTES_STORAGE_HALF_BYTES,
                             g_notes_shared + NOTES_STORAGE_HALF_BYTES,
                             NOTES_STORAGE_HALF_BYTES) != NOTES_DOCUMENT_OK))
    {
        return notes_storage_fail_open(NOTES_STORAGE_CAPACITY, 0U);
    }
    g_notes_status.opened = 1U;
    g_notes_status.result = NOTES_STORAGE_OK;
    g_notes_path[0] = '\0';
    return NOTES_STORAGE_OK;
}

int notes_storage_save(const char *path)
{
    const uint8_t *data;
    uint32_t length;
    uint32_t offset = 0U;
    int write_failed = 0;

    if (g_notes_status.opened == 0U)
    {
        return NOTES_STORAGE_NOT_OPEN;
    }
    if (g_notes_status.read_only != 0U)
    {
        g_notes_status.result = NOTES_STORAGE_READ_ONLY;
        return NOTES_STORAGE_READ_ONLY;
    }
    if (!notes_storage_copy_path(g_notes_path, path))
    {
        return NOTES_STORAGE_INVALID_ARG;
    }
    if (notes_platform_open_write(path) != 0)
    {
        g_notes_status.result = NOTES_STORAGE_IO;
        return NOTES_STORAGE_IO;
    }
    data = notes_document_get_data(&g_notes_document);
    length = notes_document_get_length(&g_notes_document);
    while (offset < length)
    {
        uint32_t actual = 0U;
        if ((notes_platform_write(data + offset, length - offset,
                                  &actual) != 0) || (actual == 0U))
        {
            write_failed = 1;
            break;
        }
        offset += actual;
    }
    if (notes_platform_close() != 0)
    {
        write_failed = 1;
    }
    if (write_failed != 0)
    {
        g_notes_status.result = NOTES_STORAGE_IO;
        return NOTES_STORAGE_IO;
    }
    notes_document_mark_saved(&g_notes_document);
    g_notes_status.document_bytes = length;
    g_notes_status.modified = 0U;
    g_notes_status.result = NOTES_STORAGE_OK;
    return NOTES_STORAGE_OK;
}

void notes_storage_close(uint8_t discard_changes)
{
    (void)discard_changes;
    if (g_notes_owner_held != 0U)
    {
        notes_platform_release();
    }
    g_notes_owner_held = 0U;
    g_notes_shared = NULL;
    g_notes_status.opened = 0U;
    memset(&g_notes_document, 0, sizeof(g_notes_document));
}

notes_document_t *notes_storage_get_document(void)
{
    return (g_notes_status.opened != 0U) ? &g_notes_document : NULL;
}

void notes_storage_get_status(notes_storage_status_t *status)
{
    if (status != NULL)
    {
        g_notes_status.modified = notes_document_is_modified(&g_notes_document);
        g_notes_status.document_bytes = notes_document_get_length(
            &g_notes_document);
        *status = g_notes_status;
    }
}

#ifdef NOTES_STORAGE_HOST_TEST
void notes_storage_reset_for_test(void)
{
    memset(&g_notes_status, 0, sizeof(g_notes_status));
    memset(&g_notes_document, 0, sizeof(g_notes_document));
    memset(g_notes_path, 0, sizeof(g_notes_path));
    g_notes_shared = NULL;
    g_notes_owner_held = 0U;
}
#endif
