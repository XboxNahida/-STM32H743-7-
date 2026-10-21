#ifndef NOTES_UTF8_INTERNAL_H
#define NOTES_UTF8_INTERNAL_H

#include <stdint.h>

#include "notes_document.h"

uint8_t notes_utf8_is_continuation(uint8_t value);
uint32_t notes_utf8_sequence_length(const uint8_t *text, uint32_t remaining);
uint8_t notes_utf8_validate(const uint8_t *text, uint32_t length);
uint8_t notes_utf8_is_boundary(const uint8_t *text, uint32_t length,
                               uint32_t position);
uint32_t notes_utf8_align_down(const uint8_t *text, uint32_t position);
uint32_t notes_utf8_previous(const uint8_t *text, uint32_t position);
notes_document_result_t notes_utf8_character_to_byte(
    const uint8_t *text, uint32_t length, uint32_t character,
    uint32_t *byte_position);

#endif
