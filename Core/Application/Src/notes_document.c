#include "notes_document.h"
#include "notes_utf8_internal.h"

#include <stddef.h>
#include <string.h>

notes_document_result_t notes_document_init(notes_document_t *document,
    uint8_t *storage, uint32_t storage_capacity,
    uint8_t *work, uint32_t work_capacity)
{
    if ((document == NULL) || (storage == NULL) || (work == NULL) ||
        (storage_capacity < (NOTES_DOCUMENT_MAX_BYTES + 1U)) ||
        (work_capacity < NOTES_DOCUMENT_WORK_BYTES))
    {
        return NOTES_DOCUMENT_INVALID_ARG;
    }
    memset(document, 0, sizeof(*document));
    document->storage = storage;
    document->work = work;
    document->storage_capacity = storage_capacity;
    document->work_capacity = work_capacity;
    storage[0] = 0U;
    work[0] = 0U;
    return NOTES_DOCUMENT_OK;
}

notes_document_result_t notes_document_load_utf8(notes_document_t *document,
    const uint8_t *data, uint32_t length)
{
    if ((document == NULL) || (document->storage == NULL) ||
        ((data == NULL) && (length != 0U)))
    {
        return NOTES_DOCUMENT_INVALID_ARG;
    }
    if ((length > NOTES_DOCUMENT_MAX_BYTES) ||
        (length >= document->storage_capacity))
    {
        return NOTES_DOCUMENT_CAPACITY;
    }
    if (!notes_utf8_validate(data, length))
    {
        return NOTES_DOCUMENT_INVALID_UTF8;
    }
    if (length != 0U)
    {
        memmove(document->storage, data, length);
    }
    document->storage[length] = 0U;
    document->length = length;
    document->window_start = 0U;
    document->window_length = 0U;
    document->window_active = 0U;
    document->modified = 0U;
    return NOTES_DOCUMENT_OK;
}

notes_document_result_t notes_document_build_window(notes_document_t *document,
    uint32_t focus_byte, notes_document_window_t *window)
{
    uint32_t start;
    uint32_t end;
    uint8_t *view;
    uint8_t *shadow;

    if ((document == NULL) || (window == NULL) ||
        (document->storage == NULL) || (document->work == NULL) ||
        (focus_byte > document->length))
    {
        return NOTES_DOCUMENT_INVALID_ARG;
    }
    focus_byte = notes_utf8_align_down(document->storage, focus_byte);
    if (document->length <= NOTES_WINDOW_MAX_BYTES)
    {
        start = 0U;
        end = document->length;
    }
    else
    {
        start = (focus_byte > (NOTES_WINDOW_MAX_BYTES / 2U)) ?
            (focus_byte - (NOTES_WINDOW_MAX_BYTES / 2U)) : 0U;
        if ((start + NOTES_WINDOW_MAX_BYTES) > document->length)
        {
            start = document->length - NOTES_WINDOW_MAX_BYTES;
        }
        start = notes_utf8_align_down(document->storage, start);
        end = start + NOTES_WINDOW_MAX_BYTES;
        if (end > document->length)
        {
            end = document->length;
        }
        if (end < document->length)
        {
            end = notes_utf8_align_down(document->storage, end);
        }
    }
    view = document->work;
    shadow = document->work + NOTES_WINDOW_MAX_BYTES + 1U;
    if (end > start)
    {
        memcpy(view, document->storage + start, end - start);
        memcpy(shadow, document->storage + start, end - start);
    }
    view[end - start] = 0U;
    shadow[end - start] = 0U;
    document->window_start = start;
    document->window_length = end - start;
    document->window_active = 1U;
    window->text = (const char *)view;
    window->start_byte = start;
    window->length_bytes = end - start;
    return NOTES_DOCUMENT_OK;
}

notes_document_result_t notes_document_replace_range(notes_document_t *document,
    uint32_t start_byte, uint32_t end_byte,
    const uint8_t *replacement, uint32_t replacement_bytes,
    uint32_t *cursor_byte)
{
    uint32_t remove_bytes;
    uint32_t new_length;
    uint32_t tail_bytes;
    uint32_t temporary;

    if ((document == NULL) || (document->storage == NULL) ||
        (cursor_byte == NULL) ||
        ((replacement == NULL) && (replacement_bytes != 0U)))
    {
        return NOTES_DOCUMENT_INVALID_ARG;
    }
    if (start_byte > end_byte)
    {
        temporary = start_byte;
        start_byte = end_byte;
        end_byte = temporary;
    }
    if ((end_byte > document->length) ||
        !notes_utf8_is_boundary(document->storage, document->length,
                                start_byte) ||
        !notes_utf8_is_boundary(document->storage, document->length,
                                end_byte))
    {
        return NOTES_DOCUMENT_RANGE;
    }
    if (!notes_utf8_validate(replacement, replacement_bytes))
    {
        return NOTES_DOCUMENT_INVALID_UTF8;
    }
    remove_bytes = end_byte - start_byte;
    if (replacement_bytes >
        (NOTES_DOCUMENT_MAX_BYTES - (document->length - remove_bytes)))
    {
        return NOTES_DOCUMENT_CAPACITY;
    }
    new_length = document->length - remove_bytes + replacement_bytes;
    tail_bytes = document->length - end_byte;
    memmove(document->storage + start_byte + replacement_bytes,
            document->storage + end_byte, tail_bytes + 1U);
    if (replacement_bytes != 0U)
    {
        memcpy(document->storage + start_byte, replacement,
               replacement_bytes);
    }
    document->length = new_length;
    document->storage[new_length] = 0U;
    document->modified = 1U;
    *cursor_byte = start_byte + replacement_bytes;
    return NOTES_DOCUMENT_OK;
}

notes_document_result_t notes_document_commit_window(notes_document_t *document,
    const char *edited_text, uint32_t edited_bytes,
    uint32_t cursor_character, uint32_t *cursor_byte)
{
    const uint8_t *edited = (const uint8_t *)edited_text;
    const uint8_t *shadow;
    uint32_t old_prefix = 0U;
    uint32_t new_prefix = 0U;
    uint32_t old_end;
    uint32_t new_end;
    uint32_t local_cursor;
    uint32_t ignored_cursor;
    notes_document_result_t result;

    if ((document == NULL) || (edited_text == NULL) ||
        (cursor_byte == NULL) || !document->window_active ||
        (edited_bytes > NOTES_WINDOW_MAX_BYTES))
    {
        return NOTES_DOCUMENT_INVALID_ARG;
    }
    if (!notes_utf8_validate(edited, edited_bytes))
    {
        return NOTES_DOCUMENT_INVALID_UTF8;
    }
    result = notes_utf8_character_to_byte(edited, edited_bytes,
                                          cursor_character, &local_cursor);
    if (result != NOTES_DOCUMENT_OK)
    {
        return result;
    }
    shadow = document->work + NOTES_WINDOW_MAX_BYTES + 1U;
    while ((old_prefix < document->window_length) &&
           (new_prefix < edited_bytes))
    {
        uint32_t old_sequence = notes_utf8_sequence_length(
            shadow + old_prefix, document->window_length - old_prefix);
        uint32_t new_sequence = notes_utf8_sequence_length(
            edited + new_prefix, edited_bytes - new_prefix);
        if ((old_sequence != new_sequence) ||
            (memcmp(shadow + old_prefix, edited + new_prefix,
                    old_sequence) != 0))
        {
            break;
        }
        old_prefix += old_sequence;
        new_prefix += new_sequence;
    }
    old_end = document->window_length;
    new_end = edited_bytes;
    while ((old_end > old_prefix) && (new_end > new_prefix))
    {
        uint32_t old_start = notes_utf8_previous(shadow, old_end);
        uint32_t new_start = notes_utf8_previous(edited, new_end);
        uint32_t old_sequence = old_end - old_start;
        uint32_t new_sequence = new_end - new_start;
        if ((old_sequence != new_sequence) ||
            (memcmp(shadow + old_start, edited + new_start,
                    old_sequence) != 0))
        {
            break;
        }
        old_end = old_start;
        new_end = new_start;
    }
    result = notes_document_replace_range(document,
        document->window_start + old_prefix,
        document->window_start + old_end,
        edited + new_prefix, new_end - new_prefix, &ignored_cursor);
    if (result != NOTES_DOCUMENT_OK)
    {
        return result;
    }
    memmove(document->work, edited, edited_bytes);
    memmove(document->work + NOTES_WINDOW_MAX_BYTES + 1U,
            edited, edited_bytes);
    document->work[edited_bytes] = 0U;
    document->work[NOTES_WINDOW_MAX_BYTES + 1U + edited_bytes] = 0U;
    document->window_length = edited_bytes;
    *cursor_byte = document->window_start + local_cursor;
    return NOTES_DOCUMENT_OK;
}

notes_document_result_t notes_document_read_range(const notes_document_t *document,
    uint32_t start_byte, uint32_t end_byte,
    uint8_t *output, uint32_t capacity, uint32_t *written)
{
    uint32_t length;
    uint32_t temporary;

    if ((document == NULL) || (document->storage == NULL) ||
        (output == NULL) || (written == NULL))
    {
        return NOTES_DOCUMENT_INVALID_ARG;
    }
    if (start_byte > end_byte)
    {
        temporary = start_byte;
        start_byte = end_byte;
        end_byte = temporary;
    }
    if ((end_byte > document->length) ||
        !notes_utf8_is_boundary(document->storage, document->length,
                                start_byte) ||
        !notes_utf8_is_boundary(document->storage, document->length,
                                end_byte))
    {
        return NOTES_DOCUMENT_RANGE;
    }
    length = end_byte - start_byte;
    if (capacity <= length)
    {
        return NOTES_DOCUMENT_CAPACITY;
    }
    if (length != 0U)
    {
        memcpy(output, document->storage + start_byte, length);
    }
    output[length] = 0U;
    *written = length;
    return NOTES_DOCUMENT_OK;
}

uint32_t notes_document_get_length(const notes_document_t *document)
{
    return (document != NULL) ? document->length : 0U;
}

const uint8_t *notes_document_get_data(const notes_document_t *document)
{
    return (document != NULL) ? document->storage : NULL;
}

uint8_t notes_document_is_modified(const notes_document_t *document)
{
    return (document != NULL) ? document->modified : 0U;
}

void notes_document_mark_saved(notes_document_t *document)
{
    if (document != NULL)
    {
        document->modified = 0U;
    }
}
