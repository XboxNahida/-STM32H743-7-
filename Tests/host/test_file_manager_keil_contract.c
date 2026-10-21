#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char *const g_files[] = {
    "file_manager_path.c", "file_manager_service.c",
    "file_manager_copy.c", "file_manager_tree.c",
    "file_manager_delete.c", "file_manager_directory.c",
    "file_manager_operations.c", "file_manager_platform.c",
    "file_manager_ui.c", "file_open_router.c",
    "wav_playback_service.c", "drawing_bmp_reader.c"
};

int main(int argc, char **argv)
{
    FILE *file;
    char *text;
    long length;
    size_t index;
    int failures = 0;

    if (argc != 2) return 2;
    file = fopen(argv[1], "rb");
    if (file == NULL) return 2;
    if ((fseek(file, 0, SEEK_END) != 0) || ((length = ftell(file)) < 0) ||
        (fseek(file, 0, SEEK_SET) != 0)) { fclose(file); return 2; }
    text = (char *)malloc((size_t)length + 1U);
    if (text == NULL) { fclose(file); return 2; }
    if (fread(text, 1U, (size_t)length, file) != (size_t)length)
    { free(text); fclose(file); return 2; }
    text[length] = '\0';
    fclose(file);
    for (index = 0U; index < sizeof(g_files) / sizeof(g_files[0]); index++)
    {
        const char *cursor = text;
        uint32_t count = 0U;
        while ((cursor = strstr(cursor, g_files[index])) != NULL)
        {
            count++;
            cursor += strlen(g_files[index]);
        }
        if (count != 2U)
        {
            printf("FAIL keil entry=%s\n", g_files[index]);
            failures++;
        }
    }
    free(text);
    if (failures != 0) return 1;
    printf("PASS file_manager_keil_contract\n");
    return 0;
}
