#ifndef NOTES_STORAGE_SERVICE_H
#define NOTES_STORAGE_SERVICE_H

#include <stdint.h>

#include "notes_document.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    NOTES_STORAGE_OK = 0,
    NOTES_STORAGE_INVALID_ARG = 1,
    NOTES_STORAGE_BUSY = 2,
    NOTES_STORAGE_IO = 3,
    NOTES_STORAGE_INVALID_ENCODING = 4,
    NOTES_STORAGE_CAPACITY = 5,
    NOTES_STORAGE_READ_ONLY = 6,
    NOTES_STORAGE_NOT_OPEN = 7
} notes_storage_result_t;

typedef struct
{
    uint8_t opened;
    uint8_t read_only;
    uint8_t truncated;
    uint8_t modified;
    uint32_t source_bytes;
    uint32_t document_bytes;
    int result;
} notes_storage_status_t;

int notes_storage_open(const char *path);
int notes_storage_new(void);
int notes_storage_save(const char *path);
void notes_storage_close(uint8_t discard_changes);
notes_document_t *notes_storage_get_document(void);
void notes_storage_get_status(notes_storage_status_t *status);

#ifdef NOTES_STORAGE_HOST_TEST
void notes_storage_reset_for_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
