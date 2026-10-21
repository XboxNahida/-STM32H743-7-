#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    const char *path = "Core/Middlewares/NES/nes_apu.c";
    FILE *file = fopen(path, "rb");
    char *text;
    long length;
    int failures = 0;

    if (file == NULL) { printf("FAIL source_open\n"); return 1; }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0)) {
        fclose(file);
        printf("FAIL source_size\n");
        return 1;
    }
    text = (char *)malloc((size_t)length + 1U);
    if (text == NULL) { fclose(file); return 1; }
    if (fread(text, 1U, (size_t)length, file) != (size_t)length) {
        free(text);
        fclose(file);
        return 1;
    }
    text[length] = '\0';
    fclose(file);

    if (strstr(text, "nes_apu_fill_buffer") != NULL) {
        printf("FAIL old_audio_sink\n");
        failures++;
    }
    if (strstr(text, "apu_process(wave_buffers, APU_PCMBUF_SIZE)") == NULL) {
        printf("FAIL apu_processing_removed\n");
        failures++;
    }
    if (strstr(text, "clocks = 0") == NULL) {
        printf("FAIL clock_reset_removed\n");
        failures++;
    }
    free(text);

    if (failures != 0) {
        printf("FAIL nes_apu_port_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS nes_apu_port_contract\n");
    return 0;
}
