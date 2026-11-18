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

// Validate the preset and save the profile. If apply is true, update the
// current settings as well; the caller is responsible for saving config.ini.
ConfigLoadResult NativeProfile_Select(ConfigProfile profile, bool apply);

typedef enum {
    PROFILE_STARTUP_READY,
    PROFILE_STARTUP_NO_PROFILE,
    PROFILE_STARTUP_INVALID_PROFILE,
    PROFILE_STARTUP_PRESET_ERROR,
} NativeProfileStartupStatus;

// Always loads config.ini. The status is retained for the main-menu UI.
NativeProfileStartupStatus NativeProfile_LoadStartup(void);
NativeProfileStartupStatus NativeProfile_GetStartupStatus(void);
// The startup read result remains unchanged if config.ini is saved later.
ConfigLoadResult NativeProfile_GetStartupConfigResult(void);

#endif
