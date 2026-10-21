#ifndef NOTES_STORAGE_CODEC_H
#define NOTES_STORAGE_CODEC_H

#include <stdint.h>

#include "notes_storage_service.h"

typedef uint16_t (*notes_storage_oem2uni_fn)(uint16_t value);

notes_storage_result_t notes_storage_decode_text(
    const uint8_t *raw, uint32_t raw_bytes, uint32_t source_bytes,
    uint8_t *output, uint32_t output_capacity,
    notes_storage_oem2uni_fn oem2uni,
    uint32_t *output_bytes, uint8_t *truncated);

#endif
