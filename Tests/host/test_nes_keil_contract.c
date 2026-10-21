#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    char *data;
    long size;

    if (file == NULL)
    {
        return NULL;
    }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((size = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        (void)fclose(file);
        return NULL;
    }
    data = (char *)malloc((size_t)size + 1U);
    if (data == NULL)
    {
        (void)fclose(file);
        return NULL;
    }
    if (fread(data, 1U, (size_t)size, file) != (size_t)size)
    {
        free(data);
        (void)fclose(file);
        return NULL;
    }
    data[size] = '\0';
    (void)fclose(file);
    return data;
}

static unsigned int count_text(const char *text, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);

    while ((text = strstr(text, needle)) != NULL)
    {
        count++;
        text += length;
    }
    return count;
}

static int group_contains(const char *project, const char *group_name,
                          const char *file_path)
{
    char marker[96];
    const char *group;
    const char *end;
    const char *file;

    (void)snprintf(marker, sizeof(marker), "<GroupName>%s</GroupName>",
                   group_name);
    group = strstr(project, marker);
    if (group == NULL)
    {
        return 0;
    }
    end = strstr(group, "</Group>");
    if (end == NULL)
    {
        return 0;
    }
    file = strstr(group, file_path);
    return ((file != NULL) && (file < end)) ? 1 : 0;
}

int main(void)
{
    static const struct
    {
        const char *path;
        const char *group;
    } core_files[] =
    {
        { "..\\Core\\Application\\Src\\nes_core_runtime.c", "Application_Services" },
        { "..\\Core\\Application\\Src\\nes_core_engine.c", "Application_Services" },
        { "..\\Core\\Middlewares\\NES\\nes_core_arena.c", "NES_Core" },
        { "..\\Core\\Middlewares\\NES\\nes_ppu_output.c", "NES_Core" },
        { "..\\Core\\Middlewares\\NES\\nes_ppu.c", "NES_Core" },
        { "..\\Core\\Middlewares\\NES\\nes_apu.c", "NES_Core" },
        { "..\\Core\\Middlewares\\NES\\nes_mapper.c", "NES_Core" },
        { "..\\Core\\Middlewares\\NES\\6502.s", "NES_Core" }
    };
    static const char *const input_path =
        "..\\Core\\Application\\Src\\nes_input.c";
    static const char *const rom_path =
        "..\\Core\\Application\\Src\\nes_rom.c";
    static const char *const video_path =
        "..\\Core\\Application\\Src\\nes_video.c";
    static const char *const runtime_path =
        "..\\Core\\Application\\Src\\nes_runtime.c";
    static const char *const platform_path =
        "..\\Core\\Application\\Src\\nes_platform.c";
    static const char *const catalog_path =
        "..\\Core\\Application\\Src\\nes_rom_catalog.c";
    static const char *const controls_path =
        "..\\Core\\Application\\Src\\nes_player_controls.c";
    static const char *const controller_path =
        "..\\Core\\Application\\Src\\nes_controller_port.c";
    static const char *const audio_path =
        "..\\Core\\Application\\Src\\nes_audio_service.c";
    static const char *const ui_path =
        "..\\lvgl_GUI\\custom\\nes_player_ui.c";
    char *project = read_source("MDK-ARM/H743_LCD_Keil5.uvprojx");
    char *main_source = read_source("Core/Src/main.c");
    size_t index;
    int failures = 0;

    if ((project == NULL) || (main_source == NULL))
    {
        printf("FAIL project_open\n");
        free(project);
        free(main_source);
        return 1;
    }
    if (count_text(project, input_path) != 1U)
    {
        printf("FAIL nes_input_once\n");
        failures++;
    }
    if (group_contains(project, "Application_Services", input_path) == 0)
    {
        printf("FAIL nes_input_group\n");
        failures++;
    }
    if (count_text(project, rom_path) != 1U)
    {
        printf("FAIL nes_rom_once\n");
        failures++;
    }
    if (group_contains(project, "Application_Services", rom_path) == 0)
    {
        printf("FAIL nes_rom_group\n");
        failures++;
    }
    if (count_text(project, video_path) != 1U)
    {
        printf("FAIL nes_video_once\n");
        failures++;
    }
    if (group_contains(project, "Application_Services", video_path) == 0)
    {
        printf("FAIL nes_video_group\n");
        failures++;
    }
    if ((count_text(project, runtime_path) != 1U) ||
        (group_contains(project, "Application_Services", runtime_path) == 0))
    {
        printf("FAIL nes_runtime_project\n");
        failures++;
    }
    if ((count_text(project, platform_path) != 1U) ||
        (group_contains(project, "Application_Services", platform_path) == 0))
    {
        printf("FAIL nes_platform_project\n");
        failures++;
    }
    if ((count_text(project, catalog_path) != 1U) ||
        (group_contains(project, "Application_Services", catalog_path) == 0))
    {
        printf("FAIL nes_catalog_project\n");
        failures++;
    }
    if ((count_text(project, controls_path) != 1U) ||
        (group_contains(project, "Application_Services", controls_path) == 0))
    {
        printf("FAIL nes_controls_project\n");
        failures++;
    }
    if ((count_text(project, controller_path) != 1U) ||
        (group_contains(project, "Application_Services", controller_path) == 0))
    {
        printf("FAIL nes_controller_project\n");
        failures++;
    }
    if ((count_text(project, audio_path) != 1U) ||
        (group_contains(project, "Application_Services", audio_path) == 0))
    {
        printf("FAIL nes_audio_project\n");
        failures++;
    }
    if ((count_text(project, ui_path) != 1U) ||
        (group_contains(project, "LVGL_GUI", ui_path) == 0))
    {
        printf("FAIL nes_ui_project\n");
        failures++;
    }
    for (index = 0U; index < sizeof(core_files) / sizeof(core_files[0]); ++index)
    {
        if (count_text(project, core_files[index].path) != 1U)
        {
            printf("FAIL nes_core_file_%u\n", (unsigned int)index);
            failures++;
        }
        if (group_contains(project, core_files[index].group,
                           core_files[index].path) == 0)
        {
            printf("FAIL nes_core_group_%u\n", (unsigned int)index);
            failures++;
        }
    }
    if (strstr(project, "../Core/Middlewares/NES") == NULL)
    {
        printf("FAIL nes_core_include_path\n");
        failures++;
    }
    if (strstr(project, "\\NES\\nes_main.c") != NULL)
    {
        printf("FAIL blocking_nes_main_present\n");
        failures++;
    }
    if ((count_text(main_source, "#include \"nes_runtime.h\"") != 1U) ||
        (count_text(main_source, "nes_runtime_init();") != 1U) ||
        (count_text(main_source, "nes_runtime_poll(HAL_GetTick());") != 1U))
    {
        printf("FAIL nes_runtime_main_loop\n");
        failures++;
    }
    free(project);
    free(main_source);

    if (failures != 0)
    {
        printf("FAIL nes_keil_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS nes_keil_contract\n");
    return 0;
}
