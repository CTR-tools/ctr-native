#include <stdio.h>
#include <platform/config/native_profile.h>
#include <platform/native_assets.h>
#include <platform/native_path.h>
#include "native_config_internal.h"

static const ConfigEnumValue kProfileValues[] = {
    {"Vanilla", CONFIG_PROFILE_VANILLA, 0},
    {"Expanded", CONFIG_PROFILE_EXPANDED, 0},
};
static const ConfigEntry kProfileEntry = {
    .section = "Profile",
    .key = "active",
    .type = CFG_ENUM,
    .enumValues = kProfileValues,
    .numEnumValues = sizeof(kProfileValues) / sizeof(kProfileValues[0]),
};

static void *NativeProfile_Destination(const ConfigEntry *entry, void *context)
{
    (void)entry;
    return context;
}

ConfigLoadResult NativeConfig_LoadProfile(ConfigProfile *profile)
{
    if (profile == NULL)
    {
        return CONFIG_LOAD_INVALID;
    }
    char path[512];
    NativePath_Join(path, sizeof(path), NativeStr8_FromCString(NativeAssets_GetBaseDir()), NATIVE_STR8_LIT("profile.ini"));
    int value = -1;
    ConfigLoadResult result = NativeConfig_ReadFile(path, &value, NativeProfile_Destination,
                                                     &kProfileEntry, 1, CONFIG_PARSE_STRICT, NULL);
    if (result != CONFIG_LOAD_OK)
    {
        return result;
    }
    if (value != CONFIG_PROFILE_VANILLA && value != CONFIG_PROFILE_EXPANDED)
    {
        return CONFIG_LOAD_INVALID;
    }
    *profile = (ConfigProfile)value;
    return CONFIG_LOAD_OK;
}

bool NativeConfig_SaveProfile(ConfigProfile profile)
{
    int value = (int)profile;
    if (NativeConfig_EnumName(&kProfileEntry, value) == NULL)
    {
        return false;
    }
    char path[512];
    NativePath_Join(path, sizeof(path), NativeStr8_FromCString(NativeAssets_GetBaseDir()), NATIVE_STR8_LIT("profile.ini"));
    FILE *file = fopen(path, "w");
    if (file == NULL)
    {
        return false;
    }
    bool written = fprintf(file, "[%s]\n", kProfileEntry.section) >= 0;
    if (!NativeConfig_WriteValue(file, &kProfileEntry, &value, NULL))
    {
        written = false;
    }
    return fclose(file) == 0 && written;
}
