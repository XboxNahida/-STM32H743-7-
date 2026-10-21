#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    char *data;
    long size;

    if ((file == NULL) || (fseek(file, 0L, SEEK_END) != 0) ||
        ((size = ftell(file)) < 0L) || (fseek(file, 0L, SEEK_SET) != 0))
    {
        if (file != NULL) fclose(file);
        return NULL;
    }
    data = (char *)malloc((size_t)size + 1U);
    if ((data == NULL) ||
        (fread(data, 1U, (size_t)size, file) != (size_t)size))
    {
        free(data);
        fclose(file);
        return NULL;
    }
    data[size] = '\0';
    fclose(file);
    return data;
}

static unsigned int count_text(const char *text, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);

    while ((text = strstr(text, needle)) != NULL)
    {
        ++count;
        text += length;
    }
    return count;
}

static int expect(const char *name, int condition)
{
    if (!condition)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    static const char *const map_path =
        "..\\lvgl\\porting\\touch_coordinate_map.c";
    char *port = read_file("lvgl/porting/lv_port_indev.c");
    char *project = read_file("MDK-ARM/H743_LCD_Keil5.uvprojx");
    const char *group;
    const char *group_end;
    const char *map_entry;
    int failures = 0;

    if ((port == NULL) || (project == NULL))
    {
        printf("FAIL source_open\n");
        free(port);
        free(project);
        return 1;
    }
    failures += expect("map header included",
        strstr(port, "#include \"touch_coordinate_map.h\"") != NULL);
    failures += expect("shared mapper called",
        strstr(port, "touch_coordinate_map(raw_x, raw_y,") != NULL);
    failures += expect("old x normalization removed",
        strstr(port, "raw_y - 44") == NULL &&
        strstr(port, "1009 - 44") == NULL);
    failures += expect("old y normalization removed",
        strstr(port, "raw_x - 38") == NULL &&
        strstr(port, "581 - 38") == NULL);
    failures += expect("map source once", count_text(project, map_path) == 1U);
    group = strstr(project, "<GroupName>LVGL_PORT</GroupName>");
    group_end = (group != NULL) ? strstr(group, "</Group>") : NULL;
    map_entry = (group != NULL) ? strstr(group, map_path) : NULL;
    failures += expect("map source in LVGL_PORT",
        (group != NULL) && (group_end != NULL) &&
        (map_entry != NULL) && (map_entry < group_end));
    free(port);
    free(project);
    if (failures == 0)
    {
        printf("PASS touch_coordinate_integration_contract\n");
    }
    return failures;
}
