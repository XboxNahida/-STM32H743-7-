#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "file_manager_path.h"

static int g_failures;

#define CHECK(condition)                                                     \
    do                                                                       \
    {                                                                        \
        if (!(condition))                                                    \
        {                                                                    \
            printf("FAIL line=%d condition=%s\n", __LINE__, #condition);    \
            g_failures++;                                                    \
        }                                                                    \
    } while (0)

static void test_name_validation(void)
{
    static const char *const invalid_names[] = {
        "", ".", "..", "../bad", "a/b", "a\\b", "a:b", "a*b",
        "a?b", "a\"b", "a<b", "a>b", "a|b"
    };
    uint32_t index;

    CHECK(file_manager_validate_name("新建文件.txt") == FILE_MANAGER_OK);
    CHECK(file_manager_validate_name("DRAW0001.BMP") == FILE_MANAGER_OK);
    for (index = 0U;
         index < (sizeof(invalid_names) / sizeof(invalid_names[0]));
         index++)
    {
        CHECK(file_manager_validate_name(invalid_names[index]) ==
              FILE_MANAGER_INVALID_NAME);
    }
    CHECK(file_manager_validate_name(NULL) == FILE_MANAGER_INVALID_ARG);
}

static void test_path_normalization_and_join(void)
{
    char output[FILE_MANAGER_PATH_CAPACITY];
    char small[8];

    CHECK(file_manager_normalize_path("0:", output, sizeof(output)) ==
          FILE_MANAGER_OK);
    CHECK(strcmp(output, "0:/") == 0);
    CHECK(file_manager_normalize_path("0:/SYSTEM/NES/", output,
                                      sizeof(output)) == FILE_MANAGER_OK);
    CHECK(strcmp(output, "0:/SYSTEM/NES") == 0);
    CHECK(file_manager_normalize_path("0:/A/../B", output,
                                      sizeof(output)) ==
          FILE_MANAGER_INVALID_NAME);
    CHECK(file_manager_normalize_path("1:/SYSTEM", output,
                                      sizeof(output)) ==
          FILE_MANAGER_INVALID_NAME);
    CHECK(file_manager_normalize_path("SYSTEM", output, sizeof(output)) ==
          FILE_MANAGER_INVALID_NAME);
    CHECK(file_manager_normalize_path("0:/too-long", small,
                                      sizeof(small)) ==
          FILE_MANAGER_PATH_TOO_LONG);

    CHECK(file_manager_join_path("0:/SYSTEM", "NES", output,
                                 sizeof(output)) == FILE_MANAGER_OK);
    CHECK(strcmp(output, "0:/SYSTEM/NES") == 0);
    CHECK(file_manager_join_path("0:/", "测试.txt", output,
                                 sizeof(output)) == FILE_MANAGER_OK);
    CHECK(strcmp(output, "0:/测试.txt") == 0);
    CHECK(file_manager_join_path("0:/", "../bad", output,
                                 sizeof(output)) ==
          FILE_MANAGER_INVALID_NAME);
}

static void test_file_classification(void)
{
    CHECK(file_manager_classify("0:/note.MD") == FILE_MANAGER_KIND_TEXT);
    CHECK(file_manager_classify("0:/photo.JPEG") == FILE_MANAGER_KIND_JPEG);
    CHECK(file_manager_classify("0:/photo.PNG") ==
          FILE_MANAGER_KIND_IMAGE_UNSUPPORTED);
    CHECK(file_manager_classify("0:/anim.gif") ==
          FILE_MANAGER_KIND_IMAGE_UNSUPPORTED);
    CHECK(file_manager_classify("0:/PICTURE/DRAW0001.BMP") ==
          FILE_MANAGER_KIND_DRAWING_BMP);
    CHECK(file_manager_classify("0:/PICTURE/OTHER.BMP") ==
          FILE_MANAGER_KIND_IMAGE_UNSUPPORTED);
    CHECK(file_manager_classify("0:/movie.AVI") == FILE_MANAGER_KIND_AVI);
    CHECK(file_manager_classify("0:/music.mp3") == FILE_MANAGER_KIND_MP3);
    CHECK(file_manager_classify("0:/REC001.WAV") == FILE_MANAGER_KIND_WAV);
    CHECK(file_manager_classify("0:/game.NES") == FILE_MANAGER_KIND_NES);
    CHECK(file_manager_classify("0:/firmware.bin") ==
          FILE_MANAGER_KIND_OTHER);
    CHECK(file_manager_classify(NULL) == FILE_MANAGER_KIND_OTHER);
}

int main(void)
{
    test_name_validation();
    test_path_normalization_and_join();
    test_file_classification();

    if (g_failures != 0)
    {
        printf("FAIL file_manager_path failures=%d\n", g_failures);
        return 1;
    }
    printf("PASS file_manager_path\n");
    return 0;
}
