#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_text(const char *path)
{
    FILE *file;
    long length;
    char *text;

    file = fopen(path, "rb");
    if (file == NULL) return NULL;
    if (fseek(file, 0L, SEEK_END) != 0) { fclose(file); return NULL; }
    length = ftell(file);
    if ((length < 0L) || (fseek(file, 0L, SEEK_SET) != 0)) {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)length + 1U);
    if (text == NULL) { fclose(file); return NULL; }
    if (fread(text, 1U, (size_t)length, file) != (size_t)length) {
        free(text);
        fclose(file);
        return NULL;
    }
    text[length] = '\0';
    fclose(file);
    return text;
}

static int expect_true(const char *name, int condition)
{
    if (condition == 0) { printf("FAIL %s\n", name); return 1; }
    return 0;
}

int main(void)
{
    char *source = read_text("Core/Middlewares/NES/nes_ppu.c");
    char *header = read_text("Core/Middlewares/NES/nes_ppu.h");
    int failures = 0;

    failures += expect_true("source_read", source != NULL);
    failures += expect_true("header_read", header != NULL);
    if ((source != NULL) && (header != NULL))
    {
        failures += expect_true("no_old_lcd_include",
                                strstr(source, "./BSP/LCD/") == NULL);
        failures += expect_true("no_lcd_register_write",
                                strstr(source, "LCD->LCD_RAM") == NULL);
        failures += expect_true("no_ltdc_framebuffer",
                                strstr(source, "g_ltdc_framebuf") == NULL);
        failures += expect_true("uses_ppu_output",
            strstr(source, "nes_ppu_output_write_line") != NULL);
        failures += expect_true("bind_declared",
            strstr(header, "PPU_bind_framebuffer") != NULL);
        failures += expect_true("unbind_declared",
            strstr(header, "PPU_unbind_framebuffer") != NULL);
    }
    free(source);
    free(header);

    if (failures != 0) {
        printf("FAIL nes_ppu_port_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS nes_ppu_port_contract\n");
    return 0;
}
