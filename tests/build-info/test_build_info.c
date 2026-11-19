#include <shared/ctr_test.h>
#include <platform/native_build_info.h>
#include "ctr_build_info.h"

#include <string.h>

int main(void)
{
	CTR_TEST_SECTION("Build metadata preserves version suffixes and title formatting");
	CTR_TEST_CHECK(strcmp(NativeBuildInfo_GetVersion(), CRASH_TEAM_RACING_EXPANDED_VERSION) == 0,
	    "logs and version output receive the complete version string");
	CTR_TEST_CHECK(strcmp(NativeBuildInfo_GetBuildId(), CRASH_TEAM_RACING_EXPANDED_BUILD_ID) == 0,
	    "replays receive the complete build ID including the dirty suffix");
	CTR_TEST_CHECK(strcmp(NativeBuildInfo_GetTitleVersion(), "v" CRASH_TEAM_RACING_EXPANDED_VERSION) == 0,
	    "title branding prefixes the complete version with exactly one v");
	return CtrTest_Summary();
}
