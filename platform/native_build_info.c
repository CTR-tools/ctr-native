#include "native_build_info.h"

#include <ctr_build_info.h>

static char s_titleVersion[] = "v" CRASH_TEAM_RACING_EXPANDED_VERSION;

const char *NativeBuildInfo_GetVersion(void)
{
	return CRASH_TEAM_RACING_EXPANDED_VERSION;
}

const char *NativeBuildInfo_GetBuildId(void)
{
	return CRASH_TEAM_RACING_EXPANDED_BUILD_ID;
}

char *NativeBuildInfo_GetTitleVersion(void)
{
	return s_titleVersion;
}
