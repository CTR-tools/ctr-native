#ifndef NATIVE_PROFILE_H
#define NATIVE_PROFILE_H

#include <stdbool.h>
#include <platform/config/native_config.h>

typedef enum {
    CONFIG_PROFILE_VANILLA,
    CONFIG_PROFILE_EXPANDED,
} ConfigProfile;

ConfigLoadResult NativeConfig_LoadProfile(ConfigProfile *profile);
bool NativeConfig_SaveProfile(ConfigProfile profile);

// Load config.ini after preset defaults; the config still loads if the preset fails.
ConfigLoadResult NativeProfile_LoadConfiguration(const char *presetPath);

#endif
