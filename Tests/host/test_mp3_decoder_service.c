#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mp3_decoder_service.h"

int mp3_decoder_test_id3v2_offset(const uint8_t *header,
                                  uint32_t header_size,
                                  uint32_t file_size,
                                  uint32_t *offset);
int mp3_decoder_test_should_retry_main_data(int error,
                                            uint32_t retry_count);

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

static int expect_i32(const char *name, int32_t actual, int32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%ld expected=%ld\n", name,
               (long)actual, (long)expected);
        return 1;
    }
    return 0;
}

static int test_path_and_status(void)
{
    mp3_decoder_status_t status;
    int16_t pcm[4608];
    uint32_t samples = 0U;
    uint8_t channels = 0U;
    int failures = 0;

    mp3_decoder_service_init();
    failures += expect_i32("open_path",
                           mp3_decoder_open("0:/MUSIC/Test.mp3"),
                           MP3_DECODER_OK);
    mp3_decoder_get_status(&status);
    failures += expect_u32("opened", status.opened, 1U);
    failures += expect_u32("sample_rate", status.sample_rate, 48000U);
    failures += expect_u32("duration_available", status.duration_ms > 0U,
                           1U);
    failures += expect_i32("decode_next",
                           mp3_decoder_decode_next(pcm, 4608U,
                                                   &samples, &channels),
                           MP3_DECODER_OK);
    failures += expect_u32("samples_nonzero", samples > 0U, 1U);
    failures += expect_u32("channels_valid",
                           (channels == 1U) || (channels == 2U), 1U);
    mp3_decoder_get_status(&status);
    failures += expect_u32("bitrate_available", status.bitrate > 0U, 1U);
    failures += expect_i32("suspend", mp3_decoder_suspend(),
                           MP3_DECODER_OK);
    mp3_decoder_get_status(&status);
    failures += expect_u32("suspend_closes_file", status.opened, 0U);
    failures += expect_u32("suspend_marks_resume", status.suspended, 1U);
    failures += expect_u32("suspend_saves_offset",
                           status.resume_offset > 0U, 1U);
    failures += expect_i32("resume", mp3_decoder_resume(),
                           MP3_DECODER_OK);
    mp3_decoder_get_status(&status);
    failures += expect_u32("resume_opens_file", status.opened, 1U);
    failures += expect_u32("resume_clears_marker", status.suspended, 0U);
    failures += expect_i32("close", mp3_decoder_close(), MP3_DECODER_OK);
    return failures;
}

static int test_invalid_arguments(void)
{
    int16_t pcm[8];
    uint32_t samples;
    uint8_t channels;
    int failures = 0;

    mp3_decoder_service_init();
    failures += expect_i32("null_path", mp3_decoder_open(NULL),
                           MP3_DECODER_INVALID_ARG);
    failures += expect_i32("empty_path", mp3_decoder_open(""),
                           MP3_DECODER_INVALID_ARG);
    failures += expect_i32("zero_capacity",
                           mp3_decoder_decode_next(pcm, 0U, &samples,
                                                   &channels),
                           MP3_DECODER_INVALID_ARG);
    failures += expect_i32("null_pcm",
                           mp3_decoder_decode_next(NULL, 8U, &samples,
                                                   &channels),
                           MP3_DECODER_INVALID_ARG);
    failures += expect_i32("close_idle", mp3_decoder_close(), MP3_DECODER_OK);
    return failures;
}

static int test_id3v2_offset(void)
{
    static const uint8_t no_tag[10] =
        {0xFFU, 0xFBU, 0x94U, 0x64U, 0U, 0U, 0U, 0U, 0U, 0U};
    static const uint8_t ys_tag[10] =
        {'I', 'D', '3', 3U, 0U, 0U, 0U, 0U, 1U, 0x4FU};
    static const uint8_t footer_tag[10] =
        {'I', 'D', '3', 4U, 0U, 0x10U, 0U, 0U, 0U, 5U};
    static const uint8_t invalid_size[10] =
        {'I', 'D', '3', 4U, 0U, 0U, 0x80U, 0U, 0U, 0U};
    uint32_t offset = UINT32_MAX;
    int failures = 0;

    failures += expect_i32("no_tag_result",
                           mp3_decoder_test_id3v2_offset(no_tag,
                                                         sizeof(no_tag),
                                                         1000U, &offset),
                           MP3_DECODER_OK);
    failures += expect_u32("no_tag_offset", offset, 0U);
    failures += expect_i32("ys_tag_result",
                           mp3_decoder_test_id3v2_offset(ys_tag,
                                                         sizeof(ys_tag),
                                                         1988569U, &offset),
                           MP3_DECODER_OK);
    failures += expect_u32("ys_tag_offset", offset, 217U);
    failures += expect_i32("footer_result",
                           mp3_decoder_test_id3v2_offset(footer_tag,
                                                         sizeof(footer_tag),
                                                         1000U, &offset),
                           MP3_DECODER_OK);
    failures += expect_u32("footer_offset", offset, 25U);
    failures += expect_i32("invalid_synchsafe",
                           mp3_decoder_test_id3v2_offset(invalid_size,
                                                         sizeof(invalid_size),
                                                         1000U, &offset),
                           MP3_DECODER_FORMAT_ERROR);
    failures += expect_i32("tag_out_of_bounds",
                           mp3_decoder_test_id3v2_offset(ys_tag,
                                                         sizeof(ys_tag),
                                                         216U, &offset),
                           MP3_DECODER_FORMAT_ERROR);
    return failures;
}

static int test_resume_main_data_warmup_is_bounded(void)
{
    int failures = 0;

    failures += expect_i32("retry_first_main_data_underflow",
                           mp3_decoder_test_should_retry_main_data(-2, 0U),
                           1);
    failures += expect_i32("retry_last_allowed_main_data_underflow",
                           mp3_decoder_test_should_retry_main_data(-2, 15U),
                           1);
    failures += expect_i32("stop_after_main_data_retry_limit",
                           mp3_decoder_test_should_retry_main_data(-2, 16U),
                           0);
    failures += expect_i32("do_not_retry_invalid_frame",
                           mp3_decoder_test_should_retry_main_data(-6, 0U),
                           0);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_path_and_status();
    failures += test_invalid_arguments();
    failures += test_id3v2_offset();
    failures += test_resume_main_data_warmup_is_bounded();
    if (failures != 0)
    {
        printf("FAIL mp3_decoder_service failures=%d\n", failures);
        return 1;
    }
    printf("PASS mp3_decoder_service\n");
    return 0;
}
