#include "file_manager_path.h"

#include <stddef.h>
#include <string.h>

#define FILE_MANAGER_NAME_BYTE_LIMIT 255U

static uint8_t file_manager_is_reserved_char(uint8_t value)
{
    static const char reserved[] = "/\\:*?\"<>|";
    uint32_t index;

    if ((value < 0x20U) || (value == 0x7FU))
    {
        return 1U;
    }
    for (index = 0U; index < (uint32_t)(sizeof(reserved) - 1U); index++)
    {
        if (value == (uint8_t)reserved[index])
        {
            return 1U;
        }
    }
    return 0U;
}

static uint8_t file_manager_ascii_lower(uint8_t value)
{
    if ((value >= (uint8_t)'A') && (value <= (uint8_t)'Z'))
    {
        return (uint8_t)(value + ((uint8_t)'a' - (uint8_t)'A'));
    }
    return value;
}

static uint8_t file_manager_ascii_equal(const char *left, const char *right)
{
    uint32_t index = 0U;

    while ((left[index] != '\0') && (right[index] != '\0'))
    {
        if (file_manager_ascii_lower((uint8_t)left[index]) !=
            file_manager_ascii_lower((uint8_t)right[index]))
        {
            return 0U;
        }
        index++;
    }
    return ((left[index] == '\0') && (right[index] == '\0')) ? 1U : 0U;
}

static file_manager_result_t file_manager_validate_component(
    const char *name, uint32_t length)
{
    uint32_t index;

    if ((name == NULL) || (length == 0U) ||
        (length > FILE_MANAGER_NAME_BYTE_LIMIT))
    {
        return FILE_MANAGER_INVALID_NAME;
    }
    if (((length == 1U) && (name[0] == '.')) ||
        ((length == 2U) && (name[0] == '.') && (name[1] == '.')) ||
        (name[length - 1U] == ' ') || (name[length - 1U] == '.'))
    {
        return FILE_MANAGER_INVALID_NAME;
    }
    for (index = 0U; index < length; index++)
    {
        if (file_manager_is_reserved_char((uint8_t)name[index]) != 0U)
        {
            return FILE_MANAGER_INVALID_NAME;
        }
    }
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_validate_name(const char *name)
{
    size_t length;

    if (name == NULL)
    {
        return FILE_MANAGER_INVALID_ARG;
    }
    length = strlen(name);
    if (length > UINT32_MAX)
    {
        return FILE_MANAGER_INVALID_NAME;
    }
    return file_manager_validate_component(name, (uint32_t)length);
}

file_manager_result_t file_manager_normalize_path(const char *input,
                                                  char *output,
                                                  uint32_t capacity)
{
    size_t input_length;
    uint32_t index;
    uint32_t component_start;
    uint32_t normalized_length;
    file_manager_result_t result;

    if ((input == NULL) || (output == NULL) || (capacity == 0U))
    {
        return FILE_MANAGER_INVALID_ARG;
    }
    input_length = strlen(input);
    if ((input_length >= FILE_MANAGER_PATH_CAPACITY) ||
        (input_length > UINT32_MAX))
    {
        return FILE_MANAGER_PATH_TOO_LONG;
    }
    if ((strcmp(input, "0:") == 0) || (strcmp(input, "0:/") == 0))
    {
        if (capacity < 4U)
        {
            return FILE_MANAGER_PATH_TOO_LONG;
        }
        memcpy(output, "0:/", 4U);
        return FILE_MANAGER_OK;
    }
    if ((input_length < 4U) || (input[0] != '0') ||
        (input[1] != ':') || (input[2] != '/'))
    {
        return FILE_MANAGER_INVALID_NAME;
    }

    component_start = 3U;
    for (index = 3U; index <= (uint32_t)input_length; index++)
    {
        if ((input[index] != '/') && (input[index] != '\0'))
        {
            continue;
        }
        if ((index == (uint32_t)input_length) &&
            (index == component_start))
        {
            break;
        }
        result = file_manager_validate_component(&input[component_start],
                                                 index - component_start);
        if (result != FILE_MANAGER_OK)
        {
            return result;
        }
        component_start = index + 1U;
    }

    normalized_length = (uint32_t)input_length;
    while ((normalized_length > 3U) &&
           (input[normalized_length - 1U] == '/'))
    {
        normalized_length--;
    }
    if ((normalized_length + 1U > capacity) ||
        (normalized_length >= FILE_MANAGER_PATH_CAPACITY))
    {
        return FILE_MANAGER_PATH_TOO_LONG;
    }
    memcpy(output, input, normalized_length);
    output[normalized_length] = '\0';
    return FILE_MANAGER_OK;
}

file_manager_result_t file_manager_join_path(const char *directory,
                                             const char *name,
                                             char *output,
                                             uint32_t capacity)
{
    char normalized[FILE_MANAGER_PATH_CAPACITY];
    size_t directory_length;
    size_t name_length;
    uint32_t separator_length;
    file_manager_result_t result;

    if ((directory == NULL) || (name == NULL) || (output == NULL))
    {
        return FILE_MANAGER_INVALID_ARG;
    }
    result = file_manager_validate_name(name);
    if (result != FILE_MANAGER_OK)
    {
        return result;
    }
    result = file_manager_normalize_path(directory, normalized,
                                         sizeof(normalized));
    if (result != FILE_MANAGER_OK)
    {
        return result;
    }
    directory_length = strlen(normalized);
    name_length = strlen(name);
    separator_length = (normalized[directory_length - 1U] == '/') ? 0U : 1U;
    if ((directory_length + separator_length + name_length + 1U > capacity) ||
        (directory_length + separator_length + name_length >=
         FILE_MANAGER_PATH_CAPACITY))
    {
        return FILE_MANAGER_PATH_TOO_LONG;
    }
    memcpy(output, normalized, directory_length);
    if (separator_length != 0U)
    {
        output[directory_length] = '/';
    }
    memcpy(&output[directory_length + separator_length], name,
           name_length + 1U);
    return FILE_MANAGER_OK;
}

static uint8_t file_manager_is_drawing_bmp(const char *path,
                                           const char *name)
{
    static const char prefix[] = "0:/picture/";
    uint32_t index;

    if ((strlen(name) != 12U) ||
        (file_manager_ascii_lower((uint8_t)name[0]) != 'd') ||
        (file_manager_ascii_lower((uint8_t)name[1]) != 'r') ||
        (file_manager_ascii_lower((uint8_t)name[2]) != 'a') ||
        (file_manager_ascii_lower((uint8_t)name[3]) != 'w'))
    {
        return 0U;
    }
    for (index = 4U; index < 8U; index++)
    {
        if ((name[index] < '0') || (name[index] > '9'))
        {
            return 0U;
        }
    }
    for (index = 0U; index < (uint32_t)(sizeof(prefix) - 1U); index++)
    {
        if (file_manager_ascii_lower((uint8_t)path[index]) !=
            (uint8_t)prefix[index])
        {
            return 0U;
        }
    }
    return 1U;
}

file_manager_kind_t file_manager_classify(const char *path)
{
    static const char *const text_extensions[] = {
        ".txt", ".md", ".c", ".h", ".cpp", ".json", ".xml",
        ".csv", ".ini", ".cfg", ".log"
    };
    const char *name;
    const char *extension;
    const char *cursor;
    uint32_t index;

    if ((path == NULL) || (path[0] == '\0'))
    {
        return FILE_MANAGER_KIND_OTHER;
    }
    name = path;
    extension = NULL;
    for (cursor = path; *cursor != '\0'; cursor++)
    {
        if (*cursor == '/')
        {
            name = cursor + 1;
            extension = NULL;
        }
        else if (*cursor == '.')
        {
            extension = cursor;
        }
    }
    if (extension == NULL)
    {
        return FILE_MANAGER_KIND_OTHER;
    }
    for (index = 0U;
         index < (uint32_t)(sizeof(text_extensions) /
                            sizeof(text_extensions[0]));
         index++)
    {
        if (file_manager_ascii_equal(extension, text_extensions[index]) != 0U)
        {
            return FILE_MANAGER_KIND_TEXT;
        }
    }
    if ((file_manager_ascii_equal(extension, ".jpg") != 0U) ||
        (file_manager_ascii_equal(extension, ".jpeg") != 0U))
    {
        return FILE_MANAGER_KIND_JPEG;
    }
    if ((file_manager_ascii_equal(extension, ".bmp") != 0U) &&
        (file_manager_is_drawing_bmp(path, name) != 0U))
    {
        return FILE_MANAGER_KIND_DRAWING_BMP;
    }
    if ((file_manager_ascii_equal(extension, ".png") != 0U) ||
        (file_manager_ascii_equal(extension, ".gif") != 0U) ||
        (file_manager_ascii_equal(extension, ".bmp") != 0U) ||
        (file_manager_ascii_equal(extension, ".webp") != 0U))
    {
        return FILE_MANAGER_KIND_IMAGE_UNSUPPORTED;
    }
    if (file_manager_ascii_equal(extension, ".avi") != 0U)
    {
        return FILE_MANAGER_KIND_AVI;
    }
    if (file_manager_ascii_equal(extension, ".mp3") != 0U)
    {
        return FILE_MANAGER_KIND_MP3;
    }
    if (file_manager_ascii_equal(extension, ".wav") != 0U)
    {
        return FILE_MANAGER_KIND_WAV;
    }
    if (file_manager_ascii_equal(extension, ".nes") != 0U)
    {
        return FILE_MANAGER_KIND_NES;
    }
    return FILE_MANAGER_KIND_OTHER;
}
