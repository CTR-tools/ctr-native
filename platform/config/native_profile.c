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

static NativeProfileStartupStatus s_profileStartupStatus = PROFILE_STARTUP_NO_PROFILE;
static ConfigLoadResult s_startupConfigResult = CONFIG_LOAD_NOT_FOUND;

static bool NativeProfile_PresetPath(ConfigProfile profile, char *path, size_t pathSize)
{
    const char *relativePath;
    switch (profile)
    {
        case CONFIG_PROFILE_VANILLA:
            relativePath = "presets/vanilla.ini";
            break;
        case CONFIG_PROFILE_EXPANDED:
            relativePath = "presets/expanded.ini";
            break;
        default:
            return false;
    }
    NativePath_Join(path, pathSize, NativeStr8_FromCString(NativeAssets_GetBaseDir()), NativeStr8_FromCString(relativePath));
    return true;
}

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

ConfigLoadResult NativeProfile_Select(ConfigProfile profile, bool apply)
{
    char path[512];
    if (!NativeProfile_PresetPath(profile, path, sizeof(path)))
    {
        return CONFIG_LOAD_INVALID;
    }
    NativeConfig previous = g_config;
    ConfigLoadResult result = NativeConfig_ApplyPresetFile(path);
    if (result != CONFIG_LOAD_OK)
    {
        return result;
    }
    if (!NativeConfig_SaveProfile(profile))
    {
        g_config = previous;
        return CONFIG_LOAD_FILE_ERROR;
    }
    if (!apply)
    {
        g_config = previous;
    }
    s_profileStartupStatus = PROFILE_STARTUP_READY;
    return CONFIG_LOAD_OK;
}

NativeProfileStartupStatus NativeProfile_LoadStartup(void)
{
    ConfigProfile profile;
    ConfigLoadResult profileResult = NativeConfig_LoadProfile(&profile);
    if (profileResult != CONFIG_LOAD_OK)
    {
        s_profileStartupStatus = profileResult == CONFIG_LOAD_NOT_FOUND
                                     ? PROFILE_STARTUP_NO_PROFILE : PROFILE_STARTUP_INVALID_PROFILE;
    }
    else
    {
        char path[512];
        NativeProfile_PresetPath(profile, path, sizeof(path));
        ConfigLoadResult presetResult = NativeConfig_ReadPresetDefaults(path);
        s_profileStartupStatus = presetResult == CONFIG_LOAD_OK ? PROFILE_STARTUP_READY : PROFILE_STARTUP_PRESET_ERROR;
        if (s_profileStartupStatus == PROFILE_STARTUP_PRESET_ERROR)
        {
            fprintf(stderr, "[Profile] Could not load preset: %s\n", path);
        }
    }
    s_startupConfigResult = NativeConfig_Load();
    return s_profileStartupStatus;
}

NativeProfileStartupStatus NativeProfile_GetStartupStatus(void)
{
    return s_profileStartupStatus;
}

ConfigLoadResult NativeProfile_GetStartupConfigResult(void)
{
    return s_startupConfigResult;
}
