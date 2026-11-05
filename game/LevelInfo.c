#include <common.h>
#include <level_info.h>
#include <platform/native_track_library.h>

struct LevelInfo LevelInfo_Lookup(int levelID)
{
	struct LevelInfo info = {0};
	if (levelID >= 0 && (size_t)levelID < sizeof(data.metaDataLEV) / sizeof(data.metaDataLEV[0]))
	{
		info.kind = LEVEL_INFO_RETAIL;
		info.retail = &data.metaDataLEV[levelID];
	}
	else
	{
		info.custom = NativeTrackRegistry_Get(NativeTrackLibrary_Current(), levelID);
		if (info.custom != NULL)
		{
			info.kind = LEVEL_INFO_CUSTOM;
		}
	}
	return info;
}

const char *LevelInfo_Title(const struct LevelInfo *info, char *const *languageStrings)
{
	if (info != NULL && info->kind == LEVEL_INFO_CUSTOM && info->custom != NULL)
	{
		return info->custom->title;
	}
	if (info != NULL && info->kind == LEVEL_INFO_RETAIL && info->retail != NULL &&
	    languageStrings != NULL && info->retail->name_LNG >= 0)
	{
		return languageStrings[info->retail->name_LNG];
	}
	return NULL;
}
