#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file;
    long length;
    char *buffer;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        (void)fclose(file);
        return NULL;
    }
    buffer = (char *)malloc((size_t)length + 1U);
    if (buffer == NULL)
    {
        (void)fclose(file);
        return NULL;
    }
    if (fread(buffer, 1U, (size_t)length, file) != (size_t)length)
    {
        free(buffer);
        (void)fclose(file);
        return NULL;
    }
    buffer[length] = '\0';
    (void)fclose(file);
    return buffer;
}

static int expect_contains(const char *name, const char *source,
                           const char *needle)
{
    if (strstr(source, needle) == NULL)
    {
        printf("FAIL %s missing=%s\n", name, needle);
        return 1;
    }
    return 0;
}

static int expect_absent(const char *name, const char *source,
                         const char *needle)
{
    if (strstr(source, needle) != NULL)
    {
        printf("FAIL %s forbidden=%s\n", name, needle);
        return 1;
    }
    return 0;
}

static int function_body_contains(const char *source,
                                  const char *function_name,
                                  const char *needle)
{
    const char *name = strstr(source, function_name);
    const char *body;
    const char *cursor;
    unsigned int depth = 0U;

    if (name == NULL)
    {
        return 0;
    }
    body = strchr(name, '{');
    if (body == NULL)
    {
        return 0;
    }
    for (cursor = body; *cursor != '\0'; cursor++)
    {
        if (*cursor == '{')
        {
            depth++;
        }
        else if (*cursor == '}')
        {
            depth--;
            if (depth == 0U)
            {
                const char *found = strstr(body, needle);
                return ((found != NULL) && (found < cursor)) ? 1 : 0;
            }
        }
    }
    return 0;
}

int main(void)
{
    char *ioc = read_source("H743_LCD_Keil5.ioc");
    char *adc = read_source("Core/Src/adc.c");
    char *tim = read_source("Core/Src/tim.c");
    char *gpio = read_source("Core/Src/gpio.c");
    int failures = 0;

    if ((ioc == NULL) || (adc == NULL) || (tim == NULL) || (gpio == NULL))
    {
        printf("FAIL adc_mapping_contract read_source\n");
        free(ioc);
        free(adc);
        free(tim);
        free(gpio);
        return 1;
    }

    failures += expect_contains("ioc_adc1_channel19", ioc,
        "ADC1.Channel-0\\#ChannelRegularConversion=ADC_CHANNEL_19");
    failures += expect_contains("ioc_pa5_adc", ioc,
                                "PA5.Signal=ADCx_INP19");
    failures += expect_contains("ioc_pa4_analog", ioc,
                                "PA4.Signal=GPIO_Analog");
    failures += expect_contains("ioc_adc_input19", ioc,
        "SH.ADCx_INP19.0=ADC1_INP19,IN19-Single-Ended");
    failures += expect_absent("ioc_no_adc_input18", ioc,
                              "SH.ADCx_INP18");
    failures += expect_absent("ioc_no_tim2_channel", ioc,
                              "S_TIM2_CH1_ETR");
    failures += expect_contains("ioc_tim2_preserved", ioc,
                                "TIM2.IPParameters=Prescaler");

    failures += expect_contains("adc_channel19", adc,
                                "sConfig.Channel = ADC_CHANNEL_19;");
    failures += expect_contains("adc_pa5_comment", adc,
                                "PA5     ------> ADC1_INP19");
    failures += expect_absent("adc_no_channel18", adc,
                              "ADC_CHANNEL_18");
    failures += expect_absent("adc_no_pa4_comment", adc,
                              "PA4     ------> ADC1_INP18");

    failures += expect_contains("tim2_instance_preserved", tim,
                                "void MX_TIM2_Init(void)");
    failures += expect_contains("tim2_base_preserved", tim,
                                "HAL_TIM_Base_Init(&htim2)");
    failures += expect_absent("tim2_no_ic_init", tim,
                              "HAL_TIM_IC_Init(&htim2)");
    failures += expect_absent("tim2_no_ic_channel", tim,
                              "HAL_TIM_IC_ConfigChannel(&htim2");
    failures += expect_absent("tim2_no_pa5_af", tim,
                              "GPIO_AF1_TIM2");
    if (function_body_contains(tim, "void MX_TIM1_Init(void)",
                               "TIM_IC_InitTypeDef sConfigIC") == 0)
    {
        printf("FAIL tim1_ic_declaration_preserved\n");
        failures++;
    }
    if (function_body_contains(tim, "void MX_TIM2_Init(void)",
                               "TIM_IC_InitTypeDef sConfigIC") != 0)
    {
        printf("FAIL tim2_ic_declaration_removed\n");
        failures++;
    }

    failures += expect_contains("gpio_pa4_pin", gpio,
                                "GPIO_InitStruct.Pin = GPIO_PIN_4;");
    failures += expect_contains("gpio_pa4_analog", gpio,
                                "GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;");

    free(ioc);
    free(adc);
    free(tim);
    free(gpio);

    if (failures != 0)
    {
        printf("FAIL adc_mapping_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS adc_mapping_contract\n");
    return 0;
}
