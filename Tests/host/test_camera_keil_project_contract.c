#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    char *text;
    if (file == NULL || fseek(file, 0L, SEEK_END) != 0)
    {
        if (file != NULL) fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size < 0L || fseek(file, 0L, SEEK_SET) != 0)
    {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)size + 1U);
    if (text != NULL)
    {
        (void)fread(text, 1U, (size_t)size, file);
        text[size] = '\0';
    }
    fclose(file);
    return text;
}

static int count_text(const char *source, const char *text)
{
    int count = 0;
    size_t length = strlen(text);
    const char *cursor = source;
    while ((cursor = strstr(cursor, text)) != NULL)
    {
        count++;
        cursor += length;
    }
    return count;
}

int main(void)
{
    static const char *files[] = {
        "sdcard_manager.c", "sccb.c", "ov5640_registers.c",
        "ov5640_service.c", "camera_resource_service.c",
        "camera_dcmi_driver.c", "camera_preview_service.c",
        "stm32h7xx_hal_dcmi.c"
    };
    char *project = read_file("MDK-ARM/H743_LCD_Keil5.uvprojx");
    char *main_header = read_file("Core/Inc/main.h");
    char *ov5640_service = read_file("Core/Peripheral/Src/ov5640_service.c");
    char *gpio_source = read_file("Core/Src/gpio.c");
    unsigned int index;
    int failures = 0;
    if ((project == NULL) || (main_header == NULL) ||
        (ov5640_service == NULL) || (gpio_source == NULL))
    {
        printf("FAIL source_open\n");
        free(project);
        free(main_header);
        free(ov5640_service);
        free(gpio_source);
        return 1;
    }
    for (index = 0U; index < sizeof(files) / sizeof(files[0]); index++)
    {
        char node[96];
        int count;
        (void)snprintf(node, sizeof(node), "<FileName>%s</FileName>",
                       files[index]);
        count = count_text(project, node);
        if (count != 1)
        {
            printf("FAIL file=%s count=%d expected=1\n", files[index], count);
            failures++;
        }
    }
    if (strstr(main_header,
               "#define SCCB_SCL_Pin GPIO_PIN_4") == NULL)
    {
        printf("FAIL sccb_scl_pin expected=PB4\n");
        failures++;
    }
    if (strstr(main_header,
               "#define SCCB_SDA_Pin GPIO_PIN_3") == NULL)
    {
        printf("FAIL sccb_sda_pin expected=PB3\n");
        failures++;
    }
    if (strstr(main_header, "#define DCMI_PWDN_Pin") != NULL)
    {
        printf("FAIL stale_dcmi_pwdn_pin\n");
        failures++;
    }
    if (strstr(main_header, "#define DCMI_PWDN_GPIO_Port") != NULL)
    {
        printf("FAIL stale_dcmi_pwdn_port\n");
        failures++;
    }
    if (strstr(ov5640_service, "DCMI_PWDN_Pin") != NULL)
    {
        printf("FAIL ov5640_pwdn_direct_gpio\n");
        failures++;
    }
    if (strstr(ov5640_service,
               "HAL_GPIO_WritePin(DCMI_PWDN_GPIO_Port") != NULL)
    {
        printf("FAIL ov5640_pwdn_gpio_write\n");
        failures++;
    }
    if (strstr(ov5640_service, "sccb_init();\n    HAL_Delay(5U);") == NULL)
    {
        printf("FAIL ov5640_power_sequence\n");
        failures++;
    }
    if (strstr(gpio_source, "DCMI_PWDN_Pin") != NULL)
    {
        printf("FAIL gpio_stale_dcmi_pwdn\n");
        failures++;
    }
    if (strstr(gpio_source,
               "GPIO_InitStruct.Pin = SCCB_SCL_Pin;\n  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;") == NULL)
    {
        printf("FAIL sccb_scl_mode expected=push_pull\n");
        failures++;
    }
    if (strstr(gpio_source,
               "GPIO_InitStruct.Pin = SCCB_SDA_Pin;\n  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;") == NULL)
    {
        printf("FAIL sccb_sda_mode expected=open_drain\n");
        failures++;
    }
    free(project);
    free(main_header);
    free(ov5640_service);
    free(gpio_source);
    if (failures == 0)
    {
        printf("PASS camera_keil_project_contract\n");
        return 0;
    }
    printf("FAIL camera_keil_project_contract failures=%d\n", failures);
    return 1;
}
