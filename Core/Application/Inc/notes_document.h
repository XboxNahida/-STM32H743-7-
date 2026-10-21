#ifndef NOTES_DOCUMENT_H
#define NOTES_DOCUMENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NOTES_DOCUMENT_MAX_BYTES 262143U
#define NOTES_WINDOW_MAX_BYTES 32768U
#define NOTES_DOCUMENT_WORK_BYTES ((NOTES_WINDOW_MAX_BYTES + 1U) * 2U)

typedef enum
{
    NOTES_DOCUMENT_OK = 0,
    NOTES_DOCUMENT_INVALID_ARG = 1,
    NOTES_DOCUMENT_INVALID_UTF8 = 2,
    NOTES_DOCUMENT_CAPACITY = 3,
    NOTES_DOCUMENT_RANGE = 4
} notes_document_result_t;

typedef struct
{
    const char *text;
    uint32_t start_byte;
    uint32_t length_bytes;
} notes_document_window_t;

typedef struct
{
    uint8_t *storage;
    uint8_t *work;
    uint32_t storage_capacity;
    uint32_t work_capacity;
    uint32_t length;
    uint32_t window_start;
    uint32_t window_length;
    uint8_t window_active;
    uint8_t modified;
} notes_document_t;

notes_document_result_t notes_document_init(notes_document_t *document,
    uint8_t *storage, uint32_t storage_capacity,
    uint8_t *work, uint32_t work_capacity);
notes_document_result_t notes_document_load_utf8(notes_document_t *document,
    const uint8_t *data, uint32_t length);
notes_document_result_t notes_document_build_window(notes_document_t *document,
    uint32_t focus_byte, notes_document_window_t *window);
notes_document_result_t notes_document_commit_window(notes_document_t *document,
    const char *edited_text, uint32_t edited_bytes,
    uint32_t cursor_character, uint32_t *cursor_byte);
notes_document_result_t notes_document_read_range(const notes_document_t *document,
    uint32_t start_byte, uint32_t end_byte,
    uint8_t *output, uint32_t capacity, uint32_t *written);
notes_document_result_t notes_document_replace_range(notes_document_t *document,
    uint32_t start_byte, uint32_t end_byte,
    const uint8_t *replacement, uint32_t replacement_bytes,
    uint32_t *cursor_byte);
uint32_t notes_document_get_length(const notes_document_t *document);
const uint8_t *notes_document_get_data(const notes_document_t *document);
uint8_t notes_document_is_modified(const notes_document_t *document);
void notes_document_mark_saved(notes_document_t *document);

#ifdef __cplusplus
}
#endif

#endif
