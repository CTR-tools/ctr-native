#ifndef NATIVE_CONFIG_INTERNAL_H
#define NATIVE_CONFIG_INTERNAL_H

#include <stdio.h>
#include <platform/config/native_config.h>

typedef enum {
    CONFIG_PARSE_COMPATIBLE,
    CONFIG_PARSE_STRICT,
} ConfigParseMode;

typedef void *(*ConfigDestination)(const ConfigEntry *entry, void *context);
typedef bool (*ConfigParseOverride)(const ConfigEntry *entry, const char *value, void *destination,
                                    ConfigParseMode mode, bool *result);
typedef const char *(*ConfigEnumNameOverride)(const ConfigEntry *entry, int value);

ConfigLoadResult NativeConfig_ReadFile(const char *path, void *context, ConfigDestination destination,
                                      const ConfigEntry *entries, int entryCount, ConfigParseMode mode,
                                      ConfigParseOverride parseOverride);
const char *NativeConfig_EnumName(const ConfigEntry *entry, int value);
bool NativeConfig_WriteValue(FILE *file, const ConfigEntry *entry, const void *value,
                             ConfigEnumNameOverride nameOverride);

#endif
