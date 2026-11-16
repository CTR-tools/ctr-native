#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "native_config_internal.h"

static char *NativeConfig_TrimWhitespace(char *s)
{
    while (isspace((unsigned char)*s)) { s++; }
    if (*s == '\0') { return s; }
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) { end--; }
    *(end + 1) = '\0';
    return s;
}

static bool NativeConfig_ParseBool(const char *value, bool *destination, ConfigParseMode mode)
{
    if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0)
    {
        *destination = true;
        return true;
    }
    if (mode == CONFIG_PARSE_COMPATIBLE || strcmp(value, "false") == 0 || strcmp(value, "0") == 0)
    {
        *destination = false;
        return true;
    }
    return false;
}

static bool NativeConfig_ParseNumber(const char *value, int *destination)
{
    errno = 0;
    char *end;
    long number = strtol(value, &end, 10);
    if (end == value || *end != '\0' || errno == ERANGE || number < INT_MIN || number > INT_MAX)
    {
        return false;
    }
    *destination = (int)number;
    return true;
}

static bool NativeConfig_ParseInt(const ConfigEntry *entry, const char *value, int *destination, ConfigParseMode mode)
{
    if (mode == CONFIG_PARSE_COMPATIBLE)
    {
        *destination = atoi(value);
        return true;
    }
    int number;
    if (!NativeConfig_ParseNumber(value, &number) || number < entry->min || number > entry->max)
    {
        return false;
    }
    *destination = number;
    return true;
}

static bool NativeConfig_ParseEnum(const ConfigEntry *entry, const char *value, int *destination, ConfigParseMode mode)
{
    for (int i = 0; i < entry->numEnumValues; i++)
    {
        if (strcmp(value, entry->enumValues[i].name) == 0)
        {
            *destination = entry->enumValues[i].value;
            return true;
        }
    }
    if (mode == CONFIG_PARSE_COMPATIBLE)
    {
        *destination = atoi(value);
        return true;
    }
    int number;
    if (!NativeConfig_ParseNumber(value, &number)) { return false; }
    for (int i = 0; i < entry->numEnumValues; i++)
    {
        if (number == entry->enumValues[i].value)
        {
            *destination = number;
            return true;
        }
    }
    return false;
}

static bool NativeConfig_ParseValue(const ConfigEntry *entry, const char *value, void *destination,
                                    ConfigParseMode mode, ConfigParseOverride parseOverride)
{
    bool result;
    if (parseOverride != NULL && parseOverride(entry, value, destination, mode, &result)) { return result; }
    switch (entry->type)
    {
        case CFG_BOOL: return NativeConfig_ParseBool(value, destination, mode);
        case CFG_INT: return NativeConfig_ParseInt(entry, value, destination, mode);
        case CFG_ENUM: return NativeConfig_ParseEnum(entry, value, destination, mode);
        case CFG_ACTION:
        case CFG_SUBSECTION:
        default: return false;
    }
}

ConfigLoadResult NativeConfig_ReadFile(const char *path, void *context, ConfigDestination destination,
                                       const ConfigEntry *entries, int entryCount, ConfigParseMode mode,
                                       ConfigParseOverride parseOverride)
{
    FILE *file = fopen(path, "r");
    if (file == NULL) { return CONFIG_LOAD_FILE_ERROR; }
    ConfigLoadResult result = CONFIG_LOAD_OK;
    char line[256];
    char section[64] = "";
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (mode == CONFIG_PARSE_STRICT && strchr(line, '\n') == NULL && !feof(file))
        {
            result = CONFIG_LOAD_INVALID;
            break;
        }
        char *p = NativeConfig_TrimWhitespace(line);
        if (*p == '\0' || *p == ';' || *p == '#') { continue; }
        if (*p == '[')
        {
            char *end = strchr(p + 1, ']');
            if (mode == CONFIG_PARSE_STRICT && (end == NULL || *NativeConfig_TrimWhitespace(end + 1) != '\0' || end == p + 1 || (size_t)(end - p - 1) >= sizeof(section)))
            {
                result = CONFIG_LOAD_INVALID;
                break;
            }
            if (end != NULL)
            {
                *end = '\0';
                strncpy(section, p + 1, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';
            }
            continue;
        }
        char *eq = strchr(p, '=');
        if (eq == NULL)
        {
            if (mode == CONFIG_PARSE_STRICT) { result = CONFIG_LOAD_INVALID; break; }
            continue;
        }
        *eq = '\0';
        char *key = NativeConfig_TrimWhitespace(p);
        char *value = NativeConfig_TrimWhitespace(eq + 1);
        const ConfigEntry *entry = NULL;
        for (int i = 0; i < entryCount; i++)
        {
            if (strcmp(section, entries[i].section) == 0 && strcmp(key, entries[i].key) == 0)
            {
                entry = &entries[i];
                break;
            }
        }
        if (entry == NULL || entry->type == CFG_ACTION || entry->type == CFG_SUBSECTION)
        {
            if (mode == CONFIG_PARSE_STRICT) { result = CONFIG_LOAD_INVALID; break; }
            continue;
        }
        if (!NativeConfig_ParseValue(entry, value, destination(entry, context), mode, parseOverride))
        {
            result = CONFIG_LOAD_INVALID;
            break;
        }
    }
    if (ferror(file)) { result = CONFIG_LOAD_FILE_ERROR; }
    if (fclose(file) != 0) { result = CONFIG_LOAD_FILE_ERROR; }
    return result;
}

const char *NativeConfig_EnumName(const ConfigEntry *entry, int value)
{
    for (int i = 0; i < entry->numEnumValues; i++)
    {
        if (entry->enumValues[i].value == value) { return entry->enumValues[i].name; }
    }
    return NULL;
}

bool NativeConfig_WriteValue(FILE *file, const ConfigEntry *entry, const void *value, ConfigEnumNameOverride nameOverride)
{
    switch (entry->type)
    {
        case CFG_BOOL: return fprintf(file, "%s = %s\n", entry->key, *(const bool *)value ? "true" : "false") >= 0;
        case CFG_INT: return fprintf(file, "%s = %d\n", entry->key, *(const int *)value) >= 0;
        case CFG_ENUM:
        {
            int number = *(const int *)value;
            const char *name = NativeConfig_EnumName(entry, number);
            if (name == NULL && nameOverride != NULL) { name = nameOverride(entry, number); }
            return fprintf(file, "%s = %s\n", entry->key, name != NULL ? name : "?") >= 0;
        }
        case CFG_ACTION:
        case CFG_SUBSECTION:
        default: return false;
    }
}
