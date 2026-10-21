#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    char *data;
    long size;

    if (file == NULL) { return NULL; }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((size = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0)) {
        (void)fclose(file);
        return NULL;
    }
    data = (char *)malloc((size_t)size + 1U);
    if (data == NULL) { (void)fclose(file); return NULL; }
    if (fread(data, 1U, (size_t)size, file) != (size_t)size) {
        free(data);
        (void)fclose(file);
        return NULL;
    }
    data[size] = '\0';
    (void)fclose(file);
    return data;
}

static int require_text(const char *path, const char *needle)
{
    char *source = read_source(path);
    int ok = (source != NULL) && (strstr(source, needle) != NULL);

    free(source);
    if (ok == 0) { printf("FAIL required path=%s\n", path); return 1; }
    return 0;
}

static int require_absent_from_project(const char *project,
                                       const char *file_name)
{
    if (strstr(project, file_name) != NULL) {
        printf("FAIL premature_keil_entry file=%s\n", file_name);
        return 1;
    }
    return 0;
}

static unsigned int count_text(const char *text, const char *needle)
{
    unsigned int count = 0U;
    size_t length = strlen(needle);

    while ((text = strstr(text, needle)) != NULL) {
        count++;
        text += length;
    }
    return count;
}

int main(void)
{
    static const char *const core_files[] = {
        "6502.s", "nes_main.c", "nes_main.h", "nes_ppu.c", "nes_ppu.h",
        "nes_apu.c", "nes_apu.h", "nes_mapper.c", "nes_mapper.h",
        "nes_crc32.c"
    };
    static const char *const mapper_files[] = {
        "000.cpp", "003.cpp", "004.cpp", "023.cpp"
    };
    char path[256];
    char *project;
    char *mapper;
    size_t index;
    int failures = 0;

    for (index = 0U; index < sizeof(core_files) / sizeof(core_files[0]); index++) {
        (void)snprintf(path, sizeof(path), "Core/Middlewares/NES/%s",
                       core_files[index]);
        failures += require_text(path,
            (strcmp(core_files[index], "6502.s") == 0) ? "cpu6502_init" : "NES");
    }
    for (index = 0U; index < sizeof(mapper_files) / sizeof(mapper_files[0]); index++) {
        (void)snprintf(path, sizeof(path), "Core/Middlewares/NES/mapper/%s",
                       mapper_files[index]);
        failures += require_text(path, "MAP");
    }

    project = read_source("MDK-ARM/H743_LCD_Keil5.uvprojx");
    if (project == NULL) {
        printf("FAIL project_open\n");
        return 1;
    }
    failures += require_absent_from_project(project, "nes_main.c");
    failures += require_absent_from_project(project, "nes_ppu.c");
    failures += require_absent_from_project(project, "nes_apu.c");
    failures += require_absent_from_project(project, "nes_mapper.c");
    failures += require_absent_from_project(project, "6502.s");
    free(project);

    mapper = read_source("Core/Middlewares/NES/nes_mapper.c");
    if (mapper == NULL) {
        printf("FAIL mapper_open\n");
        return 1;
    }
    if ((count_text(mapper, "#include \"mapper/") != 4U) ||
        (strstr(mapper, "mapper/000.cpp") == NULL) ||
        (strstr(mapper, "mapper/003.cpp") == NULL) ||
        (strstr(mapper, "mapper/004.cpp") == NULL) ||
        (strstr(mapper, "mapper/023.cpp") == NULL)) {
        printf("FAIL mapper_include_set\n");
        failures++;
    }
    if ((strstr(mapper, "0, 3, 4, 23, -1") == NULL) ||
        (count_text(mapper, "case ") != 4U) ||
        (strstr(mapper, "case 1") != NULL)) {
        printf("FAIL mapper_dispatch_set\n");
        failures++;
    }
    free(mapper);

    if (failures != 0) {
        printf("FAIL nes_core_import_contract failures=%d\n", failures);
        return 1;
    }
    printf("PASS nes_core_import_contract\n");
    return 0;
}
