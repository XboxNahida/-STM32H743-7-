#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nand_cache_metadata.h"

static int expect_result(const char *name, int actual, int expected)
{
    if (actual == expected)
    {
        return 0;
    }
    printf("FAIL %s actual=%d expected=%d\n", name, actual, expected);
    return 1;
}

static nand_cache_metadata_t make_metadata(uint8_t slot, uint32_t sequence)
{
    nand_cache_metadata_t metadata;

    memset(&metadata, 0, sizeof(metadata));
    metadata.slot = slot;
    metadata.sequence = sequence;
    metadata.data_length = 11021156U;
    metadata.page_count = (metadata.data_length + 2047U) / 2048U;
    metadata.data_crc = 0x89ABCDEFU;
    return metadata;
}

int main(void)
{
    nand_cache_metadata_t input;
    nand_cache_metadata_t output;
    nand_cache_metadata_t candidates[4];
    uint8_t valid[4] = {1U, 1U, 0U, 1U};
    uint8_t record[NAND_CACHE_METADATA_RECORD_SIZE];
    uint32_t selected = 0U;
    int failures = 0;

    input = make_metadata(1U, 7U);
    failures += expect_result(
        "encode_committed",
        nand_cache_metadata_encode(&input, 1U, record, sizeof(record)),
        NAND_CACHE_METADATA_OK);
    failures += expect_result(
        "decode_committed",
        nand_cache_metadata_decode(record, sizeof(record), &output),
        NAND_CACHE_METADATA_OK);
    if ((output.slot != input.slot) ||
        (output.sequence != input.sequence) ||
        (output.data_length != input.data_length) ||
        (output.page_count != input.page_count) ||
        (output.data_crc != input.data_crc))
    {
        printf("FAIL committed_roundtrip\n");
        failures++;
    }
    if ((record[0] != 'F') || (record[1] != 'C') ||
        (record[2] != 'M') || (record[3] != 'T') ||
        (record[8] != 7U) || (record[9] != 0U) ||
        (record[12] != 1U))
    {
        printf("FAIL explicit_little_endian_layout\n");
        failures++;
    }

    failures += expect_result(
        "uncommitted_rejected",
        nand_cache_metadata_encode(&input, 0U, record, sizeof(record)),
        NAND_CACHE_METADATA_OK);
    failures += expect_result(
        "decode_uncommitted",
        nand_cache_metadata_decode(record, sizeof(record), &output),
        NAND_CACHE_METADATA_NOT_COMMITTED);

    nand_cache_metadata_encode(&input, 1U, record, sizeof(record));
    record[16] ^= 0x40U;
    failures += expect_result(
        "crc_damage_rejected",
        nand_cache_metadata_decode(record, sizeof(record), &output),
        NAND_CACHE_METADATA_CRC_ERROR);

    input.page_count++;
    failures += expect_result(
        "length_page_mismatch_rejected",
        nand_cache_metadata_encode(&input, 1U, record, sizeof(record)),
        NAND_CACHE_METADATA_INVALID_ARG);

    candidates[0] = make_metadata(0U, 10U);
    candidates[1] = make_metadata(1U, 12U);
    candidates[2] = make_metadata(0U, 50U);
    candidates[3] = make_metadata(0U, 11U);
    failures += expect_result(
        "select_latest",
        nand_cache_metadata_select(candidates, valid, 4U, &selected),
        NAND_CACHE_METADATA_OK);
    if (selected != 1U)
    {
        printf("FAIL latest_index actual=%lu\n", (unsigned long)selected);
        failures++;
    }

    candidates[0] = make_metadata(0U, UINT32_MAX);
    candidates[1] = make_metadata(1U, 0U);
    valid[0] = 1U;
    valid[1] = 1U;
    failures += expect_result(
        "select_after_wrap",
        nand_cache_metadata_select(candidates, valid, 2U, &selected),
        NAND_CACHE_METADATA_OK);
    if (selected != 1U)
    {
        printf("FAIL wrap_index actual=%lu\n", (unsigned long)selected);
        failures++;
    }

    valid[0] = 0U;
    valid[1] = 0U;
    failures += expect_result(
        "no_valid_metadata",
        nand_cache_metadata_select(candidates, valid, 2U, &selected),
        NAND_CACHE_METADATA_NO_VALID_RECORD);
    failures += expect_result(
        "invalid_args",
        nand_cache_metadata_decode(NULL, sizeof(record), &output),
        NAND_CACHE_METADATA_INVALID_ARG);

    if (failures != 0)
    {
        printf("FAIL nand_cache_metadata failures=%d\n", failures);
        return 1;
    }
    printf("PASS nand_cache_metadata\n");
    return 0;
}
