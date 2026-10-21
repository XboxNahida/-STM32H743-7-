#include "notes_utf8_internal.h"

#include <stddef.h>

uint8_t notes_utf8_is_continuation(uint8_t value)
{
    return ((value & 0xC0U) == 0x80U) ? 1U : 0U;
}

uint32_t notes_utf8_sequence_length(const uint8_t *text, uint32_t remaining)
{
    uint8_t first;

    if ((text == NULL) || (remaining == 0U))
    {
        return 0U;
    }
    first = text[0];
    if (first < 0x80U)
    {
        return 1U;
    }
    if ((first >= 0xC2U) && (first <= 0xDFU))
    {
        return ((remaining >= 2U) &&
                notes_utf8_is_continuation(text[1])) ? 2U : 0U;
    }
    if ((first >= 0xE0U) && (first <= 0xEFU))
    {
        if ((remaining < 3U) || !notes_utf8_is_continuation(text[1]) ||
            !notes_utf8_is_continuation(text[2]))
        {
            return 0U;
        }
        if (((first == 0xE0U) && (text[1] < 0xA0U)) ||
            ((first == 0xEDU) && (text[1] >= 0xA0U)))
        {
            return 0U;
        }
        return 3U;
    }
    if ((first >= 0xF0U) && (first <= 0xF4U))
    {
        if ((remaining < 4U) || !notes_utf8_is_continuation(text[1]) ||
            !notes_utf8_is_continuation(text[2]) ||
            !notes_utf8_is_continuation(text[3]))
        {
            return 0U;
        }
        if (((first == 0xF0U) && (text[1] < 0x90U)) ||
            ((first == 0xF4U) && (text[1] >= 0x90U)))
        {
            return 0U;
        }
        return 4U;
    }
    return 0U;
}

uint8_t notes_utf8_validate(const uint8_t *text, uint32_t length)
{
    uint32_t offset = 0U;

    if ((text == NULL) && (length != 0U))
    {
        return 0U;
    }
    while (offset < length)
    {
        uint32_t sequence = notes_utf8_sequence_length(text + offset,
                                                       length - offset);
        if (sequence == 0U)
        {
            return 0U;
        }
        offset += sequence;
    }
    return 1U;
}

uint8_t notes_utf8_is_boundary(const uint8_t *text, uint32_t length,
                               uint32_t position)
{
    if ((text == NULL) || (position > length))
    {
        return 0U;
    }
    if ((position == 0U) || (position == length))
    {
        return 1U;
    }
    return notes_utf8_is_continuation(text[position]) ? 0U : 1U;
}

uint32_t notes_utf8_align_down(const uint8_t *text, uint32_t position)
{
    while ((position > 0U) && notes_utf8_is_continuation(text[position]))
    {
        position--;
    }
    return position;
}

uint32_t notes_utf8_previous(const uint8_t *text, uint32_t position)
{
    if (position == 0U)
    {
        return 0U;
    }
    position--;
    while ((position > 0U) && notes_utf8_is_continuation(text[position]))
    {
        position--;
    }
    return position;
}

notes_document_result_t notes_utf8_character_to_byte(
    const uint8_t *text, uint32_t length, uint32_t character,
    uint32_t *byte_position)
{
    uint32_t offset = 0U;
    uint32_t index = 0U;

    if ((text == NULL) || (byte_position == NULL))
    {
        return NOTES_DOCUMENT_INVALID_ARG;
    }
    while ((offset < length) && (index < character))
    {
        uint32_t sequence = notes_utf8_sequence_length(text + offset,
                                                       length - offset);
        if (sequence == 0U)
        {
            return NOTES_DOCUMENT_INVALID_UTF8;
        }
        offset += sequence;
        index++;
    }
    if (index != character)
    {
        return NOTES_DOCUMENT_RANGE;
    }
    *byte_position = offset;
    return NOTES_DOCUMENT_OK;
}
