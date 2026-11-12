#include <shared/ctr_test.h>
#include <common.h>
#include <level_info.h>
#include <platform/native_track_library.h>

struct sData sdata_static;
struct Data data;
static struct NativeTrackRegistry *s_registry;

const struct NativeTrackRegistry *NativeTrackLibrary_Current(void)
{
	return s_registry;
}

#include <platform/native_track_registry.c>
#include <game/LevelInfo.c>

int main(void)
{
	CTR_TEST_SECTION("Retail and custom level lookup");
	char *strings[] = {"Retail title", "Other title"};
	char title[32];
	data.metaDataLEV[0].name_LNG = 1;
	struct LevelInfo info = LevelInfo_Lookup(0);
	CTR_TEST_CHECK(info.kind == LEVEL_INFO_RETAIL && info.retail == &data.metaDataLEV[0] && info.custom == NULL,
	               "retail IDs resolve only to retail metadata");
	CTR_TEST_CHECK(strcmp(LevelInfo_Title(&info, strings), "Other title") == 0, "retail titles use the active language table");
	CTR_TEST_CHECK(strcmp(LevelInfo_DisplayTitle(0, strings, title, sizeof(title)), "Other title") == 0,
	               "retail display titles retain their original localized text");
	data.metaDataLEV[0].name_LNG = -1;
	CTR_TEST_CHECK(LevelInfo_Title(&info, strings) == NULL, "retail levels without a title are not indexed into the language table");
	info = LevelInfo_Lookup(-1);
	CTR_TEST_CHECK(info.kind == LEVEL_INFO_UNKNOWN && info.retail == NULL && info.custom == NULL, "negative IDs are unknown");
	info = LevelInfo_Lookup((int)(sizeof(data.metaDataLEV) / sizeof(data.metaDataLEV[0])));
	CTR_TEST_CHECK(info.kind == LEVEL_INFO_UNKNOWN, "the first ID beyond the retail table is unknown");
	CTR_TEST_CHECK(LevelInfo_Title(&info, strings) == NULL, "unknown levels have no title");
	CTR_TEST_CHECK(strcmp(LevelInfo_DisplayTitle(-1, strings, title, sizeof(title)), "") == 0,
	               "unknown levels have a safe empty display title");

	s_registry = NativeTrackRegistry_Create();
	CTR_TEST_CHECK(s_registry != NULL, "custom registry can be created");
	if (s_registry != NULL)
	{
		struct NativeTrackMetadata metadata = {"custom-id", "Custom title", "track.lev", "track.vrm", NULL, NULL, 0, 0, NULL};
		int id = 0;
		CTR_TEST_CHECK(NativeTrackRegistry_Add(s_registry, &metadata, &id), "custom track is registered");
		info = LevelInfo_Lookup(id);
		CTR_TEST_CHECK(info.kind == LEVEL_INFO_CUSTOM && info.custom != NULL && info.retail == NULL,
		               "custom IDs resolve only to custom metadata");
		CTR_TEST_CHECK(strcmp(LevelInfo_Title(&info, NULL), "Custom title") == 0, "custom titles do not require a language table");
		CTR_TEST_CHECK(strcmp(LevelInfo_DisplayTitle(id, NULL, title, sizeof(title)), "Custom title") == 0,
		               "custom display titles preserve the complete text");
		CTR_TEST_CHECK(strcmp(LevelInfo_DisplayTitle(id, NULL, NULL, 0), "") == 0,
		               "custom display titles handle a missing buffer safely");
		struct NativeTrackMetadata special = {"special", "A\xc3\xa9 @B", "track.lev", "track.vrm", NULL, NULL, 0, 0, NULL};
		int specialID = -1;
		CTR_TEST_CHECK(NativeTrackRegistry_Add(s_registry, &special, &specialID) &&
		               strcmp(LevelInfo_DisplayTitle(specialID, NULL, title, sizeof(title)), "A? ?B") == 0,
		               "custom display titles are converted to safe font glyphs");
		NativeTrackRegistry_Destroy(s_registry);
		s_registry = NULL;
		info = LevelInfo_Lookup(id);
		CTR_TEST_CHECK(info.kind == LEVEL_INFO_UNKNOWN, "lookup does not retain custom IDs after a library replacement");
	}
	return CtrTest_Summary();
}
