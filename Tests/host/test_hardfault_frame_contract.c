#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *text;
    if (file == NULL) return NULL;
    if (fseek(file, 0L, SEEK_END) != 0 || (size = ftell(file)) < 0L ||
        fseek(file, 0L, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    text = malloc((size_t)size + 1U);
    if (text == NULL || fread(text, 1U, (size_t)size, file) != (size_t)size) {
        free(text);
        fclose(file);
        return NULL;
    }
    text[size] = '\0';
    fclose(file);
    return text;
}

static int expect(const char *name, int condition)
{
    if (condition) return 0;
    printf("FAIL %s\n", name);
    return 1;
}

int main(void)
{
    char *source = read_source("Core/Src/stm32h7xx_it.c");
    int failures = 0;
    failures += expect("read_source", source != NULL);
    if (source != NULL) {
        failures += expect("naked_exception_entry",
            strstr(source, "__asm void HardFault_Handler(void)") != NULL &&
            strstr(source, "TST     lr, #4") != NULL &&
            strstr(source, "MRSEQ   r0, MSP") != NULL &&
            strstr(source, "MRSNE   r0, PSP") != NULL &&
            strstr(source, "MOV     r1, lr") != NULL &&
            strstr(source, "B       hardfault_debug") != NULL);
        failures += expect("no_late_return_address_guess",
            strstr(source, "__return_address()") == NULL);
        failures += expect("extended_fpu_core_frame_layout",
            strstr(source, "(exc_return & (1UL << 4)) == 0U") != NULL &&
            strstr(source, "core_frame = raw_words;") != NULL &&
            strstr(source, "core_frame += 18U;") == NULL);
        failures += expect("raw_frame_snapshot",
            strstr(source, "hardfault_raw_words") != NULL &&
            strstr(source, "raw_words[i] = raw_sp[i];") != NULL);
        failures += expect("fault_context_output",
            strstr(source, "EXC_RETURN=0x%08lX") != NULL &&
            strstr(source, "RAW_SP=0x%08lX") != NULL &&
            strstr(source, "FRAME=%s") != NULL &&
            strstr(source, "ABFSR=0x%08lX") != NULL);
        failures += expect("frame_range_guard",
            strstr(source, "hardfault_frame_readable") != NULL &&
            strstr(source, "frame valid=0") != NULL);
    }
    free(source);
    if (failures != 0) {
        printf("hardfault_frame_contract failures=%d\n", failures);
        return 1;
    }
    puts("PASS hardfault_frame_contract");
    return 0;
}
