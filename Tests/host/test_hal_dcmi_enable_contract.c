#include <stdio.h>
#include <string.h>

static int active_dcmi_define(char *source)
{
    char *line = strtok(source, "\r\n");
    while (line != NULL)
    {
        while ((*line == ' ') || (*line == '\t'))
        {
            line++;
        }
        if (strncmp(line, "#define HAL_DCMI_MODULE_ENABLED", 32U) == 0)
        {
            return 1;
        }
        line = strtok(NULL, "\r\n");
    }
    return 0;
}

int main(void)
{
    FILE *file = fopen("Core/Inc/stm32h7xx_hal_conf.h", "rb");
    char source[8192];
    size_t length;
    int failures = 0;
    if (file == NULL)
    {
        printf("FAIL open_hal_conf\n");
        return 1;
    }
    length = fread(source, 1U, sizeof(source) - 1U, file);
    fclose(file);
    source[length] = '\0';
    if (!active_dcmi_define(source))
    {
        printf("FAIL dcmi_module_missing\n");
        failures++;
    }
    if (failures == 0)
    {
        printf("PASS hal_dcmi_enable_contract\n");
        return 0;
    }
    printf("FAIL hal_dcmi_enable_contract failures=%d\n", failures);
    return 1;
}
