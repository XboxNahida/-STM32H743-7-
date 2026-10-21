#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *file;
    long length;
    char *text;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)length + 1U);
    if (text == NULL)
    {
        fclose(file);
        return NULL;
    }
    if (fread(text, 1U, (size_t)length, file) != (size_t)length)
    {
        free(text);
        fclose(file);
        return NULL;
    }
    text[length] = '\0';
    fclose(file);
    return text;
}

static int require_text(const char *source, const char *needle,
                        const char *label)
{
    if (strstr(source, needle) == NULL)
    {
        printf("FAIL %s\n", label);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *source = read_file("Core/Peripheral/Src/command_parser.c");
    int failures = 0;

    if (source == NULL)
    {
        printf("FAIL read command_parser.c\n");
        return 1;
    }

    failures += require_text(source, "#include \"lvgl.h\"",
                             "lvgl public api included");
    failures += require_text(source, "static int command_lvgl_handler",
                             "lvgl handler exists");
    failures += require_text(source, "lv_mem_monitor(&monitor);",
                             "lvgl heap inspected");
    failures += require_text(source, "disp_get_diagnostics(&display);",
                             "display reload diagnostics inspected");
    failures += require_text(
        source,
        "lvgl memory total=%lu free=%lu biggest=%lu max_used=%lu used_pct=%u frag_pct=%u\\r\\n",
        "all memory fields printed");
    failures += require_text(
        source,
        "lvgl display requests=%lu callbacks=%lu timeouts=%lu failures=%lu pending=%u enabled=%u active=0x%08lX target=0x%08lX cfbar=0x%08lX srcr=0x%08lX isr=0x%08lX\\r\\n",
        "display reload diagnostics printed");
    failures += require_text(source, "INVALID_ARG usage=lvgl status\\r\\n",
                             "invalid usage reported");
    failures += require_text(source, "{\"lvgl\", \"lvgl status",
                             "lvgl command registered");

    free(source);
    if (failures != 0)
    {
        return 1;
    }
    printf("PASS lvgl_memory_command_contract\n");
    return 0;
}
